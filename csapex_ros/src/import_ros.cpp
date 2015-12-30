/// HEADER
#include "import_ros.h"

/// COMPONENT
#include <csapex_ros/ros_handler.h>
#include <csapex_ros/ros_message_conversion.h>

/// PROJECT
#include <csapex/msg/io.h>
#include <csapex/factory/message_factory.h>
#include <csapex/utility/stream_interceptor.h>
#include <csapex/msg/message.h>
#include <csapex/param/parameter_factory.h>
#include <csapex/param/set_parameter.h>
#include <csapex/model/node_modifier.h>
#include <csapex/utility/register_apex_plugin.h>
#include <csapex_ros/time_stamp_message.h>
#include <csapex/utility/timer.h>
#include <csapex/msg/any_message.h>
#include <csapex/utility/interlude.hpp>

/// SYSTEM
#include <yaml-cpp/eventhandler.h>

CSAPEX_REGISTER_CLASS(csapex::ImportRos, csapex::Node)

using namespace csapex;

const std::string ImportRos::no_topic_("-----");

namespace {
ros::Time rosTime(u_int64_t stamp_micro_seconds) {
    ros::Time t;
    t.fromNSec(stamp_micro_seconds * 1e3);
    return t;
}
}

ImportRos::ImportRos()
    : connector_(nullptr), retries_(0), buffer_size_(1024), running_(true)
{
}

void ImportRos::setup(NodeModifier& node_modifier)
{
    input_time_ = node_modifier.addOptionalInput<connection_types::TimeStampMessage>("time");
    connector_ = node_modifier.addOutput<connection_types::AnyMessage>("Something");
}

void ImportRos::setupParameters(Parameterizable& parameters)
{
    std::vector<std::string> set;
    set.push_back(no_topic_);
    parameters.addParameter(csapex::param::ParameterFactory::declareParameterStringSet("topic", set),
                 std::bind(&ImportRos::update, this));

    parameters.addParameter(csapex::param::ParameterFactory::declareTrigger("refresh"),
                 std::bind(&ImportRos::refresh, this));

    parameters.addParameter(csapex::param::ParameterFactory::declareRange("rate", 0.1, 100.0, 60.0, 0.1),
                 std::bind(&ImportRos::updateRate, this));
    parameters.addParameter(csapex::param::ParameterFactory::declareRange("queue", 0, 30, 1, 1),
                 std::bind(&ImportRos::updateSubscriber, this));
    parameters.addParameter(csapex::param::ParameterFactory::declareBool("latch", false));

    std::function<bool()> connected_condition = [&]() { return msg::isConnected(input_time_); };
    csapex::param::Parameter::Ptr buffer_p = csapex::param::ParameterFactory::declareRange("buffer/length", 0.0, 10.0, 1.0, 0.1);
    parameters.addConditionalParameter(buffer_p, connected_condition);
    csapex::param::Parameter::Ptr max_wait_p = csapex::param::ParameterFactory::declareRange("buffer/max_wait", 0.0, 10.0, 1.0, 0.1);
    parameters.addConditionalParameter(max_wait_p, connected_condition);
}

void ImportRos::setupROS()
{
    refresh();
}

void ImportRos::refresh()
{
    ROSHandler& rh = getRosHandler();

    if(rh.nh()) {
        std::string old_topic = readParameter<std::string>("topic");

        ros::master::V_TopicInfo topics;
        ros::master::getTopics(topics);

        param::SetParameter::Ptr setp = std::dynamic_pointer_cast<param::SetParameter>(getParameter("topic"));
        if(setp) {
            modifier_->setNoError();
            bool found = false;
            std::vector<std::string> topics_str;
            topics_str.push_back(no_topic_);
            for(ros::master::V_TopicInfo::const_iterator it = topics.begin(); it != topics.end(); ++it) {
                topics_str.push_back(it->name);
                if(it->name == old_topic) {
                    found = true;
                }
            }
            if(!found) {
                topics_str.push_back(old_topic);
            }
            setp->setSet(topics_str);

            if(old_topic != no_topic_) {
                setp->set(old_topic);
            }
            return;
        }
    }

    modifier_->setWarning("no ROS connection");
}

void ImportRos::update()
{
    retries_ = 5;
    waitForTopic();
}

void ImportRos::updateRate()
{
    setTickFrequency(readParameter<double>("rate"));
}

void ImportRos::updateSubscriber()
{
    if(!current_topic_.name.empty()) {
        current_subscriber = RosMessageConversion::instance().subscribe(current_topic_, readParameter<int>("queue"), std::bind(&ImportRos::callback, this, std::placeholders::_1));
    }
}

bool ImportRos::doSetTopic()
{
    getRosHandler().refresh();

    if(!getRosHandler().isConnected()) {
        modifier_->setError("no connection to ROS");
        return false;
    }

    ros::master::V_TopicInfo topics;
    ros::master::getTopics(topics);

    std::string topic = readParameter<std::string>("topic");

    if(topic == no_topic_) {
        return true;
    }

    for(ros::master::V_TopicInfo::iterator it = topics.begin(); it != topics.end(); ++it) {
        if(it->name == topic) {
            setTopic(*it);
            retries_ = 0;
            return true;
        }
    }

    std::stringstream ss;
    ss << "cannot set topic, " << topic << " doesn't exist";
    if(retries_ > 0) {
        ss << ", " << retries_ << " retries left";
    }
    ss << ".";

    modifier_->setWarning(ss.str());
    return false;
}

void ImportRos::processROS()
{
    std::unique_lock<std::recursive_mutex> lock(msgs_mtx_);
    INTERLUDE("process");

    // first check if connected -> if not connected, we only use tick
    if(!msg::isConnected(input_time_)) {
        return;
    }

    // now that we are connected, check that we have a valid message
    if(!msg::hasMessage(input_time_)) {
        return;
    }

    // INPUT CONNECTED
    connection_types::TimeStampMessage::ConstPtr time = msg::getMessage<connection_types::TimeStampMessage>(input_time_);

    if(msgs_.empty()) {
        return;
    }

    if(time->value == ros::Time(0)) {
        modifier_->setWarning("incoming time is 0, using default behaviour");
        publishLatestMessage();
        return;
    }

    if(rosTime(msgs_.back()->stamp_micro_seconds) == ros::Time(0)) {
        modifier_->setWarning("buffered time is 0, using default behaviour");
        publishLatestMessage();
        return;
    }

    // drop old messages
    ros::Duration keep_duration(readParameter<double>("buffer/length"));
    while(!msgs_.empty() && rosTime(msgs_.front()->stamp_micro_seconds) + keep_duration < time->value) {
        msgs_.pop_front();
    }

    if(!msgs_.empty()) {
        if(rosTime(msgs_.front()->stamp_micro_seconds) > time->value) {
            // aerr << "time stamp " << time->value << " is too old, oldest buffered is " << rosTime(msgs_.front()->stamp_micro_seconds) << std::endl;
            // return;
        }

        ros::Duration max_wait_duration(readParameter<double>("buffer/max_wait"));
        if(rosTime(msgs_.front()->stamp_micro_seconds) + max_wait_duration < time->value) {
            // aerr << "[1] time stamp " << time->value << " is too new" << std::endl;
            // return;
        }
    }



    // wait until we have a message
    if(!isStampCovered(time->value)) {
        ros::Rate r(10);
        while(!isStampCovered(time->value) && running_) {
            r.sleep();
            ros::spinOnce();

            if(!msgs_.empty()) {
                ros::Duration max_wait_duration(readParameter<double>("buffer/max_wait"));
                if(rosTime(msgs_.back()->stamp_micro_seconds) + max_wait_duration < time->value) {
                    aerr << "[2] time stamp " << time->value << " is too new, latest stamp is " << rosTime(msgs_.back()->stamp_micro_seconds) << std::endl;
                    return;
                }
            }
        }
    }

    if(msgs_.empty()) {
        modifier_->setWarning("No messages received");
        return;
    }

    std::deque<connection_types::Message::ConstPtr>::iterator first_after = msgs_.begin();
    while(rosTime((*first_after)->stamp_micro_seconds) < time->value) {
        ++first_after;
    }

    if(first_after == msgs_.begin()) {
        msg::publish(connector_, *first_after);
        return;

    } else if(first_after == msgs_.end()) {
        assert(false);
        modifier_->setWarning("Should not happen.....");
        return;

    } else {
        std::deque<connection_types::Message::ConstPtr>::iterator last_before = first_after - 1;

        ros::Duration diff1 = rosTime((*first_after)->stamp_micro_seconds) - time->value;
        ros::Duration diff2 = rosTime((*last_before)->stamp_micro_seconds) - time->value;

        if(diff1 < diff2) {
            msg::publish(connector_, *first_after);
        } else {
            msg::publish(connector_, *last_before);
        }
    }
}

bool ImportRos::isStampCovered(const ros::Time &stamp)
{
    std::unique_lock<std::recursive_mutex> lock(msgs_mtx_);
    return rosTime(msgs_.back()->stamp_micro_seconds) >= stamp;
}

void ImportRos::waitForTopic()
{
    INTERLUDE("wait");
    ros::WallDuration poll_wait(0.5);
    while(retries_ --> 0) {
        bool topic_exists = doSetTopic();

        if(topic_exists) {
            return;
        } else {
            ROS_WARN_STREAM("waiting for topic " << readParameter<std::string>("topic"));
            poll_wait.sleep();
        }
    }
}

bool ImportRos::canTick()
{
    std::unique_lock<std::recursive_mutex> lock(msgs_mtx_);
    return !msg::isConnected(input_time_) && !msgs_.empty();
}

void ImportRos::tickROS()
{
    INTERLUDE("tick");

    if(retries_ > 0) {
        waitForTopic();
    }

    if(msg::isConnected(input_time_)) {
        return;
    }

    // NO INPUT CONNECTED -> ONLY KEEP CURRENT MESSAGE
    {
        INTERLUDE("pop");
        while(msgs_.size() > 1) {
            msgs_.pop_front();
        }
    }

    if(!current_topic_.name.empty()) {
        publishLatestMessage();
    }
}

void ImportRos::publishLatestMessage()
{
    std::unique_lock<std::recursive_mutex> lock(msgs_mtx_);

    INTERLUDE("publish");

    if(msgs_.empty()) {
        INTERLUDE("wait for message");
        ros::WallRate r(10);
        while(msgs_.empty() && running_) {
            r.sleep();
            ros::spinOnce();
        }

        if(!running_) {
            return;
        }
    }

    msg::publish(connector_, msgs_.back());

    if(!readParameter<bool>("latch")) {
        msgs_.clear();
    }
}

// FIXME: this can be still called after the object is destroyed!
void ImportRos::callback(ConnectionTypeConstPtr message)
{
    connection_types::Message::ConstPtr msg = std::dynamic_pointer_cast<connection_types::Message const>(message);
    if(msg) {
        std::unique_lock<std::recursive_mutex> lock(msgs_mtx_);

        if(!msgs_.empty() && msg->stamp_micro_seconds < msgs_.front()->stamp_micro_seconds) {
            awarn << "detected time anomaly -> reset";
            msgs_.clear();
        }

        if(!msg::isConnected(input_time_)) {
            msgs_.clear();
        }

        msgs_.push_back(msg);

        while((int) msgs_.size() > buffer_size_) {
            msgs_.pop_front();
        }
    }
}

void ImportRos::setTopic(const ros::master::TopicInfo &topic)
{
    if(topic.name == current_topic_.name) {
        return;
    }

    current_subscriber.shutdown();

    //if(RosMessageConversion::instance().isTopicTypeRegistered(topic)) {
        modifier_->setNoError();

        current_topic_ = topic;
        updateSubscriber();

//    } else {
//        modifier_->setError(std::string("cannot import topic of type ") + topic.datatype);
//        return;
//    }

}


void ImportRos::setParameterState(Memento::Ptr memento)
{
    Node::setParameterState(memento);
}

void ImportRos::abort()
{
    running_ = false;
}
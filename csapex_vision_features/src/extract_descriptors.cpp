/// HEADER
#include "extract_descriptors.h"

/// COMPONENT
#include <csapex_vision_features/keypoint_message.h>
#include <csapex_vision_features/descriptor_message.h>

/// PROJECT
#include <utils_vision/utils/extractor.h>
#include <utils_vision/utils/extractor_factory.h>
#include <utils_vision/utils/extractor_manager.h>
#include <utils_param/range_parameter.h>
#include <utils_param/io.h>
#include <csapex/msg/output.h>
#include <csapex/msg/input.h>
#include <csapex_vision/cv_mat_message.h>
#include <csapex/utility/qt_helper.hpp>
#include <csapex/utility/q_signal_relay.h>
#include <utils_param/parameter_factory.h>
#include <utils_param/set_parameter.h>
#include <csapex/utility/register_apex_plugin.h>
#include <csapex/model/node_modifier.h>

/// SYSTEM
#include <boost/lambda/lambda.hpp>

CSAPEX_REGISTER_CLASS(csapex::ExtractDescriptors, csapex::Node)

using namespace csapex;
using namespace connection_types;

ExtractDescriptors::ExtractDescriptors()
    : refresh_(true)
{
    ExtractorManager& manager = ExtractorManager::instance();
    std::vector<std::string> methods;

    typedef std::pair<std::string, ExtractorManager::ExtractorInitializer> Pair;
    Q_FOREACH(Pair fc, manager.descriptorExtractors()) {
        std::string key = fc.second.getType();
        methods.push_back(key);
    }

    param::Parameter::Ptr method = param::ParameterFactory::declareParameterStringSet("method", methods);
    addParameter(method, boost::bind(&ExtractDescriptors::update, this));

    Q_FOREACH(Pair fc, manager.descriptorExtractors()) {
        std::string key = fc.second.getType();
        std::function<bool()> condition = (boost::bind(&param::Parameter::as<std::string>, method.get()) == key);

        Q_FOREACH(param::Parameter::Ptr param, manager.featureDescriptorParameters(key)) {
            param::Parameter::Ptr param_clone = param::ParameterFactory::clone(param);
            addConditionalParameter(param_clone, condition, boost::bind(&ExtractDescriptors::update, this));
        }
    }
}

void ExtractDescriptors::setup()
{
    in_img = modifier_->addInput<CvMatMessage>("Image");
    in_key = modifier_->addInput<KeypointMessage>("Keypoints");

    out_des = modifier_->addOutput<DescriptorMessage>("Descriptors");
}


void ExtractDescriptors::process()
{
    if(refresh_) {
        refresh_ = false;

        std::string method = readParameter<std::string>("method");
        Extractor::Ptr next = ExtractorFactory::create("", method, param::StaticParameterProvider(getParameters()));

        extractor = next;
    }

    if(!extractor) {
        setError(true, "no extractor set");
        return;
    }

    setError(false);

    CvMatMessage::ConstPtr img_msg = in_img->getMessage<CvMatMessage>();

    DescriptorMessage::Ptr des_msg(new DescriptorMessage);

    // need to clone keypoints, extractDescriptors will modify the vector
    KeypointMessage::Ptr key_msg = in_key->getClonedMessage<KeypointMessage>();

    extractor->extractDescriptors(img_msg->value, key_msg->value, des_msg->value);

    out_des->publish(des_msg);
}

void ExtractDescriptors::update()
{
    refresh_ = true;
}

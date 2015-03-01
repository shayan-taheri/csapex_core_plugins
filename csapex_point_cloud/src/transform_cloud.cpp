/// HEADER
#include "transform_cloud.h"

/// PROJECT
#include <csapex/msg/io.h>
#include <csapex_transform/transform_message.h>
#include <csapex/model/node_modifier.h>
#include <csapex/utility/register_apex_plugin.h>

/// SYSTEM
#include <boost/mpl/for_each.hpp>
#define BOOST_SIGNALS_NO_DEPRECATION_WARNING
#include <pcl_ros/transforms.h>

CSAPEX_REGISTER_CLASS(csapex::TransformCloud, csapex::Node)

using namespace csapex;
using namespace csapex::connection_types;

TransformCloud::TransformCloud()
{
}

void TransformCloud::setup(NodeModifier& node_modifier)
{
    input_cloud_ = node_modifier.addInput<PointCloudMessage>("PointCloud");
    input_transform_ = node_modifier.addInput<TransformMessage>("Transformation");

    output_ = node_modifier.addOutput<PointCloudMessage>("PointCloud");
}

void TransformCloud::process()
{
    PointCloudMessage::ConstPtr msg = msg::getMessage<PointCloudMessage>(input_cloud_);

    boost::apply_visitor (PointCloudMessage::Dispatch<TransformCloud>(this, msg), msg->value);
}

template <class PointT>
void TransformCloud::inputCloud(typename pcl::PointCloud<PointT>::ConstPtr cloud)
{
    TransformMessage::ConstPtr transform = msg::getMessage<TransformMessage>(input_transform_);
    const tf::Transform& t = transform->value;

    typename pcl::PointCloud<PointT>::Ptr out(new pcl::PointCloud<PointT>);
    pcl_ros::transformPointCloud(*cloud, *out, t);

    std::string frame = cloud->header.frame_id;

    if(frame == transform->child_frame) {
        frame = transform->frame_id;
    }

    PointCloudMessage::Ptr msg(new PointCloudMessage(frame, cloud->header.stamp));
    msg->value = out;

    msg::publish(output_, msg);
}

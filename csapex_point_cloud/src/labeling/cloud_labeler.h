#ifndef CLOUD_LABELER_H
#define CLOUD_LABELER_H

/// PROJECT
#include <csapex_core_plugins/interactive_node.h>
#include <csapex_opencv/cv_mat_message.h>
#include <csapex_point_cloud/msg/point_cloud_message.h>

namespace csapex
{
class CloudLabeler : public InteractiveNode
{
public:
    CloudLabeler();

    virtual void setup(csapex::NodeModifier& node_modifier) override;
    virtual void setupParameters(Parameterizable& parameters);

    virtual void beginProcess(csapex::NodeModifier& node_modifier, Parameterizable& parameters) override;
    virtual void finishProcess(csapex::NodeModifier& node_modifier, Parameterizable& parameters) override;

    void setResult(const pcl::PointCloud<pcl::PointXYZL>::Ptr& labeled);

    connection_types::PointCloudMessage::ConstPtr getMessage() const;
    bool isOutputConnected() const;

private:
    void refresh();

private:
    Input* input_;
    Output* output_;

public:
    slim_signal::Signal<void()> display_request;
    slim_signal::Signal<void()> refresh_request;
    slim_signal::Signal<void()> done_request;

private:
    mutable std::mutex message_mutex_;
    connection_types::PointCloudMessage::ConstPtr message_;

    connection_types::PointCloudMessage::Ptr result_;
};

}  // namespace csapex

#endif  // CLOUD_LABELER_H

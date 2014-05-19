#ifndef STATISTICAL_OUTLIER_REMOVAL_H
#define STATISTICAL_OUTLIER_REMOVAL_H

/// PROJECT
#include <csapex/model/node.h>
#include <csapex_point_cloud/point_cloud_message.h>

namespace csapex {
class StatisticalOutlierRemoval : public Node
{
public:
    StatisticalOutlierRemoval();

    virtual void setup();
    virtual void process();

    template <class PointT>
    void inputCloud(typename pcl::PointCloud<PointT>::Ptr cloud);

protected:
    ConnectorIn*  input_cloud_;
    ConnectorIn*  input_indeces_;
    ConnectorOut* output_cloud_;
    ConnectorOut* output_indeces_;

};
}
#endif // STATISTICAL_OUTLIER_REMOVAL_H

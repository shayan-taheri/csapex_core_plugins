#ifndef GRADIENT_BOOSTED_TREES_TRAINER_H
#define GRADIENT_BOOSTED_TREES_TRAINER_H

/// PROJECT
#include <csapex_core_plugins/collection_node.h>
#include <csapex_ml/features_message.h>

/// SYSTEM
#include <opencv2/opencv.hpp>

namespace csapex
{
class CSAPEX_EXPORT_PLUGIN GradientBoostedTreesTrainer : public CollectionNode<connection_types::FeaturesMessage>
{
public:
    GradientBoostedTreesTrainer();

    void setupParameters(Parameterizable& parameters) override;

protected:
    virtual bool processCollection(std::vector<connection_types::FeaturesMessage>& collection) override;

private:
    std::string path_;
    std::vector<param::ParameterPtr> priors_params_;
    std::vector<float> priors_;

    double subsample_portion_;
    double shrinkage_;
    int classes_;
    double regression_accuracy_;

    cv::GradientBoostingTreeParams params_;

    void updatePriors();
    void udpatePriorValues();
};
}  // namespace csapex

#endif  // GRADIENT_BOOSTED_TREES_TRAINER_Hs

#ifndef RANDOM_TREES_H
#define RANDOM_TREES_H

/// COMPONENT
#include <csapex_ml/features_message.h>

/// PROJECT
#include <csapex/model/node.h>

/// SYSTEM
#include <opencv2/opencv.hpp>

namespace csapex {


class CSAPEX_EXPORT_PLUGIN RandomTrees : public csapex::Node
{
public:
    RandomTrees();

    void setupParameters(Parameterizable& parameters);
    void setup(csapex::NodeModifier& node_modifier) override;
    virtual void process() override;

private:
    void reloadTree();
    connection_types::FeaturesMessage classify(const connection_types::FeaturesMessage& input);

private:
    Input*  in_;
    Output* out_;

    Slot* reload_;

    cv::RandomTrees random_trees_;
    std::string     path_;
    bool            loaded_;
};


}

#endif // RANDOM_TREES_H

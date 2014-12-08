#ifndef DECISION_TREE_H
#define DECISION_TREE_H

/// COMPONENT
#include <csapex_ml/features_message.h>

/// PROJECT
#include <csapex/model/node.h>

/// SYSTEM
#include <opencv2/opencv.hpp>

namespace csapex {


class DecisionTree : public csapex::Node
{
public:
    DecisionTree();

    void setupParameters();
    void setup();
    void process();

private:
    void loadTree();
    connection_types::FeaturesMessage classify(const connection_types::FeaturesMessage& input);

private:
    Input* in_;
    Output* out_;

    Slot* reload_;

    cv::DecisionTree dtree_;
    bool loaded_;
};


}

#endif // DECISION_TREE_H

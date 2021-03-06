#ifndef RESIZE_H
#define RESIZE_H

/// COMPONENT
#include <csapex/model/node.h>

/// SYSTEM
#include <opencv2/core/core.hpp>

namespace csapex
{
class Resize : public csapex::Node
{
public:
    Resize();

    virtual void process() override;
    virtual void setup(csapex::NodeModifier& node_modifier) override;
    virtual void setupParameters(Parameterizable& parameters) override;

protected:
    csapex::Output* output_;
    csapex::Input* input_;

    cv::Size size_;
    int mode_;
    void update();
};

}  // namespace csapex
#endif  // RESIZE_H

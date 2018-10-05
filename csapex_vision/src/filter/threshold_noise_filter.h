#ifndef WEIGHTED_NOISE_FILTER_H
#define WEIGHTED_NOISE_FILTER_H

/// COMPONENT
#include <csapex/model/node.h>

namespace csapex
{
class ThresholdNoiseFilter : public csapex::Node
{
public:
    ThresholdNoiseFilter();

    virtual void process() override;
    virtual void setup(csapex::NodeModifier& node_modifier) override;
    virtual void setupParameters(Parameterizable& parameters);

protected:
    csapex::Output* output_;
    csapex::Input* input_;
    csapex::Input* threshold_;
};
}  // namespace csapex
#endif  // WEIGHTED_NOISE_FILTER_H

#ifndef GRAB_CUT_H
#define GRAB_CUT_H

/// COMPONENT

/// PROJECT
#include <csapex/model/node.h>

/// SYSTEM

namespace csapex
{
class GrabCut : public csapex::Node
{
public:
    GrabCut();

    void setupParameters(Parameterizable& parameters);
    void setup(csapex::NodeModifier& node_modifier) override;
    virtual void process() override;

private:
    Input* in_;
    Input* in_fg_;
    Input* in_bg_;

    Input* in_roi_;

    Output* out_fg_;
    Output* out_bg_;
};

}  // namespace csapex

#endif  // GRAB_CUT_H

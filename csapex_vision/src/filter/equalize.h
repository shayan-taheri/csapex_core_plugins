#ifndef EQUALIZE_H
#define EQUALIZE_H

/// COMPONENT
#include <csapex/model/node.h>

namespace csapex
{
class Equalize : public csapex::Node
{
public:
    Equalize();

    virtual void process() override;
    virtual void setup(csapex::NodeModifier& node_modifier) override;

protected:
    csapex::Output* output_;
    csapex::Input* input_;
};

}  // namespace csapex

#endif  // EQUALIZE_H

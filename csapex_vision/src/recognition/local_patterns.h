#ifndef LOCALPATTERNS_H
#define LOCALPATTERNS_H

/// PROJECT
#include <csapex/model/node.h>

namespace csapex
{
class LocalPatterns : public csapex::Node
{
public:
    LocalPatterns();

    virtual void process() override;
    virtual void setup(csapex::NodeModifier& node_modifier) override;
    virtual void setupParameters(Parameterizable& parameters);

protected:
    enum Type
    {
        LBP,
        LBP_EXT,
        LBP_VAR,
        LBP_CS,
        LTP,
        LTP_EXT,
        LTP_SHORT,
        LTP_CS,
        WLD,
        WLD_SHORT,
        WLD_ORIENTED,
        HOMOGENITY,
        HOMOGENITY_TEX
    };

    csapex::Input* in_;
    csapex::Output* out_;
};
}  // namespace csapex
#endif  // LOCALPATTERNS_H

#ifndef EXPORT_FILE_H_
#define EXPORT_FILE_H_

/// PROJECT
#include <csapex/model/node.h>
#include <csapex/model/token_data.h>
#include <csapex/msg/generic_vector_message.hpp>

namespace csapex
{
class CSAPEX_EXPORT_PLUGIN ExportFile : public Node
{
public:
    ExportFile();

    void setup(csapex::NodeModifier& node_modifier) override;
    void setupParameters(Parameterizable& parameters) override;
    void process() override;

protected:
    void setExportPath();

    void exportMessage(const TokenData::ConstPtr& msg);
    void exportVector(const connection_types::GenericVectorMessage::ConstPtr& vector);
    void exportSingle(const TokenData::ConstPtr& msg);

private:
    Input* connector_;

    std::string path_;
    std::string base_;

    serialization::Format target_format_;

    int suffix_;

    bool oneshot_;
    bool oneshot_allowed_;
    bool vector_as_single_file_;
    bool compress_;

    TokenData::ConstPtr last_message_;
};

}  // namespace csapex

#endif  // EXPORT_FILE_H_

/// HEADER
#include <csapex_transform/transform_message.h>

/// PROJECT
#include <csapex/utility/assert.h>

/// SYSTEM
#include <tf/transform_datatypes.h>

using namespace csapex;
using namespace connection_types;

TransformMessage::TransformMessage(const std::string& from_frame, const std::string& to_frame)
    : MessageTemplate<tf::Transform, TransformMessage> (from_frame), child_frame(to_frame)
{}

TransformMessage::TransformMessage()
    : MessageTemplate<tf::Transform, TransformMessage> ("/"), child_frame("/")
{}

ConnectionType::Ptr TransformMessage::clone() {
    Ptr new_msg(new TransformMessage(frame_id, child_frame));
    new_msg->value = value;
    return new_msg;
}

void TransformMessage::writeYaml(YAML::Emitter& yaml) const {
    const tf::Quaternion& q = value.getRotation();
    const tf::Vector3& t = value.getOrigin();

    yaml << YAML::Key << "orientation" << YAML::Value << YAML::Flow << YAML::BeginSeq << q.x() << q.y() << q.z() << q.w() << YAML::EndSeq;
    yaml << YAML::Key << "translation" << YAML::Value << YAML::Flow << YAML::BeginSeq << t.x() << t.y() << t.z() << YAML::EndSeq;;
}

void TransformMessage::readYaml(const YAML::Node& node) {
    double qx,qy,qz,qw,x,y,z;
    if(exists(node, "orientation")) {
        const YAML::Node& doc = node["orientation"];
        apex_assert_hard(doc.Type() == YAML::NodeType::Sequence);
        doc[0] >> qx;
        doc[1] >> qy;
        doc[2] >> qz;
        doc[3] >> qw;
    }
    if(exists(node, "translation")) {
        const YAML::Node& doc = node["translation"];
        apex_assert_hard(doc.Type() == YAML::NodeType::Sequence);
        doc[0] >> x;
        doc[1] >> y;
        doc[2] >> z;
    }

    value = tf::Transform(tf::Quaternion(qx,qy,qz,qw), tf::Vector3(x, y, z));
}

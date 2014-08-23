#ifndef SCAN_MESSAGE_H
#define SCAN_MESSAGE_H

/// COMPONENT
#include <utils_laser_processing/data/scan.h>

/// PROJECT
#include <csapex/msg/message_template.hpp>

namespace csapex {
namespace connection_types {


struct ScanMessage : public MessageTemplate<lib_laser_processing::Scan, ScanMessage>
{
    ScanMessage();
};

/// TRAITS
template <>
struct type<ScanMessage> {
    static std::string name() {
        return "Scan";
    }
};

}
}


/// YAML
namespace YAML {
template<>
struct convert<csapex::connection_types::ScanMessage> {
  static Node encode(const csapex::connection_types::ScanMessage& rhs);
  static bool decode(const Node& node, csapex::connection_types::ScanMessage& rhs);
};
}

#endif // SCAN_MESSAGE_H

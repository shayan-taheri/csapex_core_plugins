/// HEADER
#include "display_keypoints.h"

/// PROJECT
#include <csapex/model/node_modifier.h>
#include <csapex/msg/io.h>
#include <csapex/param/bitset_parameter.h>
#include <csapex/param/parameter_factory.h>
#include <csapex/utility/register_apex_plugin.h>
#include <csapex_opencv/cv_mat_message.h>
#include <csapex_vision_features/keypoint_message.h>

CSAPEX_REGISTER_CLASS(csapex::DisplayKeypoints, csapex::Node)

using namespace csapex;
using namespace connection_types;

DisplayKeypoints::DisplayKeypoints() : in_key(nullptr)
{
}

void DisplayKeypoints::setupParameters(Parameterizable& parameters)
{
    parameters.addParameter(csapex::param::ParameterFactory::declareColorParameter("color", 255, 0, 0));
    parameters.addParameter(csapex::param::ParameterFactory::declareBool("random color", true));

    std::map<std::string, std::pair<int, bool>> flags;
    //    flags["DRAW_OVER_OUTIMG"] = (int)
    //    cv::DrawMatchesFlags::DRAW_OVER_OUTIMG;
    flags["NOT_DRAW_SINGLE_POINTS"] = std::make_pair((int)cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS, false);
    flags["DRAW_RICH_KEYPOINTS"] = std::make_pair((int)cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS, true);
    parameters.addParameter(csapex::param::ParameterFactory::declareParameterBitSet("flags", flags));
}

void DisplayKeypoints::process()
{
    CvMatMessage::ConstPtr img_msg = msg::getMessage<CvMatMessage>(in_img);
    KeypointMessage::ConstPtr key_msg = msg::getMessage<KeypointMessage>(in_key);

    CvMatMessage::Ptr out(new CvMatMessage(img_msg->getEncoding(), img_msg->frame_id, img_msg->stamp_micro_seconds));

    cv::Scalar color(-1, -1, -1, -1);
    if (!readParameter<bool>("random color")) {
        const std::vector<int>& c = readParameter<std::vector<int>>("color");
        color = cv::Scalar(c[2], c[1], c[0]);
    }

    int flags = readParameter<int>("flags");

    cv::drawKeypoints(img_msg->value, key_msg->value, out->value, color, flags);

    msg::publish(out_img, out);
}

void DisplayKeypoints::setup(NodeModifier& node_modifier)
{
    in_img = node_modifier.addInput<CvMatMessage>("Image");
    in_key = node_modifier.addInput<KeypointMessage>("Keypoints");

    out_img = node_modifier.addOutput<CvMatMessage>("Image");
}

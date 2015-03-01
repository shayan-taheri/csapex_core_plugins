/// PROJECT
#include <csapex/model/node.h>
#include <csapex/utility/register_apex_plugin.h>
#include <utils_param/parameter_factory.h>
#include <csapex/model/node_modifier.h>
#include <csapex_vision/cv_mat_message.h>
#include <csapex/msg/io.h>

using namespace csapex::connection_types;


namespace csapex {

class RotateImage : public csapex::Node
{
public:
    RotateImage()
    {
    }

    void setupParameters(Parameterizable& parameters)
    {
        parameters.addParameter(param::ParameterFactory::declareRange("angle", -M_PI, M_PI, 0.0, 0.001));
        std::map<std::string, int> modes = boost::assign::map_list_of
                ("nearest", (int) cv::INTER_NEAREST)
                ("linear", (int) cv::INTER_LINEAR)
                ("area", (int) cv::INTER_AREA)
                ("cubic", (int) cv::INTER_CUBIC)
                ("lanczos4", (int) cv::INTER_LANCZOS4);
        parameters.addParameter(param::ParameterFactory::declareParameterSet("mode", modes, (int) cv::INTER_NEAREST));
    }

    void setup(csapex::NodeModifier& node_modifier)
    {
        in_  = node_modifier.addInput<CvMatMessage>("image");
        out_ = node_modifier.addOutput<CvMatMessage>("rotated image");
    }

    void process()
    {
        CvMatMessage::ConstPtr src = msg::getMessage<CvMatMessage>(in_);
        CvMatMessage::Ptr dst(new CvMatMessage(src->getEncoding(), src->stamp_micro_seconds));

        double angle = readParameter<double>("angle");
        int mode = readParameter<int>("mode");

        process(src, angle, mode, dst);
    }

    void process(const CvMatMessage::ConstPtr& src,
                 double angle,
                 int mode,
                 CvMatMessage::Ptr& dst)
    {
        int dim = std::max(src->value.cols, src->value.rows);

        cv::Point2d pt(dim/2.0, dim/2.0);
        cv::Mat r = cv::getRotationMatrix2D(pt, angle / M_PI * 180.0, 1.0);

        cv::warpAffine(src->value, dst->value, r, cv::Size(dim, dim), mode);

        msg::publish(out_, dst);
    }

private:
    Input* in_;
    Output* out_;
};

}

CSAPEX_REGISTER_CLASS(csapex::RotateImage, csapex::Node)

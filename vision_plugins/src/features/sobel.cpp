#include "sobel.h"

/// PROJECT
#include <csapex/utility/register_apex_plugin.h>
#include <csapex/msg/io.h>
#include <utils_param/parameter_factory.h>
#include <csapex_vision/cv_mat_message.h>

using namespace csapex;
using namespace csapex::connection_types;
using namespace vision_plugins;

CSAPEX_REGISTER_CLASS(vision_plugins::Sobel, csapex::Node)

Sobel::Sobel() :
    dx_(1),
    dy_(1)
{
}


void Sobel::process()
{
    CvMatMessage::ConstPtr in = msg::getMessage<connection_types::CvMatMessage>(input_);
    CvMatMessage::Ptr out(new connection_types::CvMatMessage(enc::mono, in->stamp_micro_seconds));
    int depth = in->value.type() & 7;
    cv::Sobel(in->value, out->value, depth, dx_, dy_, ksize_, scale_,delta_);
    msg::publish(output_, out);
}

void Sobel::setupParameters(Parameterizable& parameters)
{
    Operator::setupParameters(parameters);
    parameters.addParameter(param::ParameterFactory::declareRange("dx", 0, 5, dx_, 1),
                 std::bind(&Sobel::update, this));
    parameters.addParameter(param::ParameterFactory::declareRange("dy", 0, 5, dy_, 1),
                 std::bind(&Sobel::update, this));
}

void  Sobel::update()
{
    Operator::update();
    dx_     = readParameter<int>("dx");
    dy_     = readParameter<int>("dy");
}

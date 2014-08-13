/// HEADER
#include "hog_training_rois.h"

/// PROJECT
#include <csapex/msg/input.h>
#include <csapex/msg/output.h>
#include <csapex/utility/register_apex_plugin.h>
#include <utils_param/parameter_factory.h>
#include <csapex/model/node_modifier.h>
#include <csapex_vision/cv_mat_message.h>
#include <csapex_vision/roi_message.h>
#include <csapex_core_plugins/vector_message.h>

/// SYSTEM
#include <boost/assign.hpp>

CSAPEX_REGISTER_CLASS(csapex::HOGTrainingRois, csapex::Node)

using namespace csapex;
using namespace csapex::connection_types;


HOGTrainingRois::HOGTrainingRois()
{
}

void HOGTrainingRois::setupParameters()
{

    addParameter(param::ParameterFactory::declareRange("overlap",
                                                       param::ParameterDescription("Overlap in % of neg. training examples with \
                                                                                    pos. example in the middle."),
                                                       0, 100, 50, 1));
}

void HOGTrainingRois::setup()
{
    in_image_ = modifier_->addInput<CvMatMessage>("image", true);
    in_roi_   = modifier_->addInput<RoiMessage>("roi");
    out_      = modifier_->addOutput<VectorMessage, RoiMessage>("rois");
}

void HOGTrainingRois::process()
{
    RoiMessage::Ptr in_roi = in_roi_->getMessage<RoiMessage>();
    Roi &roi = in_roi->value;

    int limit_x = std::numeric_limits<int>::max();
    int limit_y = std::numeric_limits<int>::max();

    if(in_image_->hasMessage()) {
        CvMatMessage::Ptr in_image = in_image_->getMessage<CvMatMessage>();
        limit_x = in_image->value.cols;
        limit_y = in_image->value.rows;
    }

    double overlap  = readParameter<int>("overlap") / 100.0;
    double dx       = (1.0 - overlap) * roi.w();
    double dy       = (1.0 - overlap) * roi.h();


    VectorMessage::Ptr out(VectorMessage::make<RoiMessage>());
    roi.setColor(cv::Scalar(0,255,0));
    roi.setClassification(0);
    RoiMessage::Ptr msg(new RoiMessage);

    const static double xs[] = {0.0,0.0,1.0,-1.0};
    const static double ys[] = {1.0,-1.0,0.0,0.0};

    for(unsigned int i = 0 ; i < 4 ; ++i) {
        msg.reset(new RoiMessage);
        int x = roi.x() + xs[i] * dx;
        int y = roi.y() + ys[i] * dy;

        if(x >= 0 && y >= 0) {
            if(x + roi.w() <= limit_x &&
               y + roi.h() <= limit_y) {
                Roi r(x, y, roi.w(), roi.h(),cv::Scalar(0,0,255), 1);
                msg->value = r;
                out->value.push_back(msg);
            }
        }
    }

    msg.reset(new RoiMessage);
    msg->value = roi;
    out->value.push_back(msg);

    out_->publish(out);
}


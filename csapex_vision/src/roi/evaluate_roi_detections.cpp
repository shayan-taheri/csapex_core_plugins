#include "evaluate_roi_detections.h"

#include <csapex/msg/io.h>
#include <csapex/param/parameter_factory.h>
#include <csapex/utility/register_apex_plugin.h>
#include <csapex_opencv/roi_message.h>
#include <csapex_evaluation/confusion_matrix_message.h>
#include <csapex/msg/generic_vector_message.hpp>

#include <fstream>

using namespace csapex;
using namespace csapex::connection_types;

CSAPEX_REGISTER_CLASS(csapex::EvaluateROIDetections, csapex::Node)

EvaluateROIDetections::EvaluateROIDetections() :
    human_parts_found_({0,0})
{
}

void EvaluateROIDetections::setup(NodeModifier &node_modifier)
{
    in_prediction_  = node_modifier.addInput<GenericVectorMessage, RoiMessage>("prediction");
    in_groundtruth_ = node_modifier.addInput<GenericVectorMessage, RoiMessage>("groundtruth");
    out_confusion_ = node_modifier.addOutput<ConfusionMatrixMessage>("confusion");
}

void EvaluateROIDetections::setupParameters(Parameterizable &parameters)
{
    std::map<std::string, int> mode_types =
    {
        {"IGNORE_PARTLY_VISIBLE", IGNORE_PARTLY_VISIBLE},
        {"INTEGRATE_PARTLY_VISIBLE", INTEGRATE_PARTLY_VISIBLE}
    };

    parameters.addParameter(param::ParameterFactory::declareParameterSet("mode",
                                                                         mode_types,
                                                                         (int) IGNORE_PARTLY_VISIBLE),
                            (int&) mode_);
    parameters.addParameter(param::ParameterFactory::declareFileOutputPath("statistic path",
                                                                           "",
                                                                           "*.txt"),
                            path_of_statistic_);
    parameters.addParameter(param::ParameterFactory::declareRange("overlap", 0.25, 1.0, 0.7, 0.01),
                            percentage_of_overlap_);


    parameters.addParameter(param::ParameterFactory::declareTrigger("save"),
                            std::bind(&EvaluateROIDetections::save, this));
    parameters.addParameter(param::ParameterFactory::declareTrigger("reset"),
                            std::bind(&EvaluateROIDetections::resetConfusion, this));

}

using RoiMessagesConstPtr = std::shared_ptr<std::vector<RoiMessage> const>;

namespace impl {
inline double overlap(const cv::Rect &prediction,
                      const cv::Rect &groundtruth)
{
    return (prediction & groundtruth).area() / (double) std::max(prediction.area(),
                                                                 groundtruth.area());
}

inline void evaluateIgnoringPartlyVisible(const RoiMessagesConstPtr &groundtruth,
                                          const RoiMessagesConstPtr &predition,
                                          const double min_overlap,
                                          ConfusionMatrix &confury,
                                          std::array<std::size_t, 2> &human_parts)
{
    /// step 1 : remove all partly visible entries
    const std::size_t size_groundtruth = groundtruth->size();
    const std::size_t size_prediction  = predition->size();

    std::vector<char> predictions_mask(size_prediction,  1);

    /// groundtruth data can simply be kept, we only need to check for HUMAN_PART
    for(std::size_t i = 0 ; i < size_groundtruth ; ++i) {
        const RoiMessage &roi_gt = groundtruth->at(i);
        if(roi_gt.value.classification() == EvaluateROIDetections::HUMAN_PART) {
            /// find the prediction with maximum overlap and remove it
            std::size_t  max_idx;
            double       max_overlap = std::numeric_limits<double>::lowest();
            for(std::size_t j = 0 ; j < size_prediction ; ++j) {
                /// no refill
                if(predictions_mask[j] == 0)
                    continue;

                const RoiMessage &roi_pred = predition->at(j);
                const double overlap = impl::overlap(roi_pred.value.rect(),
                                                     roi_gt.value.rect());
                if(overlap > max_overlap) {
                    max_idx = j;
                    max_overlap = overlap;
                }
            }
            /// two possibilities here: either overlap or not
            /// if so, a prediction roi has been assigned no matter what prediction
            if(max_overlap > min_overlap) {
                predictions_mask[max_idx] = 0;

                const RoiMessage &roi_pred = predition->at(max_idx);
                if(roi_pred.value.classification() == EvaluateROIDetections::HUMAN) {
                    ++human_parts[1];
                }
                ++human_parts[0];
            }
        }
    }
    /// all predictions with HUMAN_PART overlap are out of the game now
    /// now we can check all HUMAN groundtruth rois
    for(std::size_t i = 0 ; i < size_groundtruth ; ++i) {
        const RoiMessage &roi_gt = groundtruth->at(i);

        if(roi_gt.value.classification() == EvaluateROIDetections::HUMAN_PART) {
            /// these can be ignored now
            continue;
        }

        /// find the predcition with maximum overlap
        std::size_t  max_idx;
        double       max_overlap = std::numeric_limits<double>::lowest();
        for(std::size_t j = 0 ; j < size_prediction ; ++j) {
            /// no refill
            if(predictions_mask[j] == 0)
                continue;

            const RoiMessage &roi_pred = predition->at(j);
            const double overlap = impl::overlap(roi_pred.value.rect(),
                                                 roi_gt.value.rect());
            if(overlap > max_overlap) {
                max_idx = j;
                max_overlap = overlap;
            }
        }

        /// two possibilities here: either overlap or not
        if(max_overlap > min_overlap) {
            const RoiMessage &roi_pred = predition->at(max_idx);
            confury.reportClassification(roi_gt.value.classification(), roi_pred.value.classification());
            /// roi has been assigned, so we have to erase it from the mask
            predictions_mask[max_idx] = 0;
        } else {
            /// roi was not found at all
            confury.reportClassification(roi_gt.value.classification(), EvaluateROIDetections::BACKGROUND);
        }
    }

    /// now its time to evaluate the leftovers
    for(std::size_t i = 0 ; i < size_prediction ; ++i) {
        /// still no refill
        if(predictions_mask[i] == 0)
            continue;
        /// if a roi hasn't been assigned yet, we have to check wether it is
        /// human or not
        const RoiMessage &roi_pred = predition->at(i);
        confury.reportClassification(EvaluateROIDetections::BACKGROUND, roi_pred.value.classification());
    }
}

inline void evaluteIntegratingPartlyVisible(const RoiMessagesConstPtr &groundtruth,
                                            const RoiMessagesConstPtr &prediction,
                                            const double min_overlap,
                                            ConfusionMatrix &confury)

{
    /// step 1 : find out which groundtruth rios were found
    const std::size_t size_groundtruth = groundtruth->size();
    const std::size_t size_prediction  = prediction->size();

    std::vector<char> predictions_mask(size_prediction,  1);
    for(std::size_t i = 0 ; i < size_groundtruth ; ++i) {
        /// a maximum overlap prediction
        const RoiMessage &roi_gt = groundtruth->at(i);
        std::size_t  max_idx;
        double       max_overlap = std::numeric_limits<double>::lowest();
        for(std::size_t j = 0 ; j < size_prediction ; ++j) {
            /// no refill
            if(predictions_mask[j] == 0)
                continue;

            const RoiMessage &roi_pred = prediction->at(j);
            const double overlap = impl::overlap(roi_pred.value.rect(),
                                                 roi_gt.value.rect());
            if(overlap > max_overlap) {
                max_idx = j;
                max_overlap = overlap;
            }
        }
        if(max_overlap > min_overlap) {
            const RoiMessage &roi_pred = prediction->at(max_idx);
            if(roi_gt.value.classification() == EvaluateROIDetections::HUMAN) {
                /// we found a match, now the detection has to match
                if(roi_pred.value.classification() == EvaluateROIDetections::HUMAN) {
                    confury.reportClassification(1,1);
                } else {
                    confury.reportClassification(1,0);
                }
            } else {
                /// we can find a match but it has not to be valid, if so good, otherwise doesn't matter
                if(roi_pred.value.classification() == EvaluateROIDetections::HUMAN) {
                    confury.reportClassification(1,1);
                }
            }
            predictions_mask[max_idx] = 0;
        } else {
            /// we did not find a match, if it should be detected, this is bad
            if(roi_gt.value.classification() == EvaluateROIDetections::HUMAN) {
                confury.reportClassification(1,0);
            }
        }
    }

    /// now its time to eat the leftovers
    for(std::size_t i = 0 ; i < size_prediction ; ++i) {
        /// still no refill
        if(predictions_mask[i] == 0)
            continue;
        /// if a roi hasn't been assigned yet, we have to check wether it is
        /// human or not
        const RoiMessage &roi_pred = prediction->at(i);
        if(roi_pred.value.classification() == EvaluateROIDetections::BACKGROUND) {
            confury.reportClassification(0, 0);
        } else {
            confury.reportClassification(0, 1);
        }
    }
}
}

void EvaluateROIDetections::process()
{
    /// TODO : add rejected and found clusters
    RoiMessagesConstPtr in_prediction =
            msg::getMessage<GenericVectorMessage, RoiMessage>(in_prediction_);
    RoiMessagesConstPtr in_groundtruth =
            msg::getMessage<GenericVectorMessage, RoiMessage>(in_groundtruth_);

    switch(mode_) {
    case IGNORE_PARTLY_VISIBLE:
        impl::evaluateIgnoringPartlyVisible(in_groundtruth,
                                            in_prediction,
                                            percentage_of_overlap_,
                                            confusion_,
                                            human_parts_found_);
        break;
    case INTEGRATE_PARTLY_VISIBLE:
        impl::evaluteIntegratingPartlyVisible(in_groundtruth,
                                              in_prediction,
                                              percentage_of_overlap_,
                                              confusion_);
        break;
    default:
        throw std::runtime_error("Unknown mode type!");
    }

    ConfusionMatrixMessage::Ptr confusion(new ConfusionMatrixMessage);
    confusion->confusion = confusion_;
    msg::publish(out_confusion_, confusion);
}

void EvaluateROIDetections::save() const
{
    std::ofstream out(path_of_statistic_);
    if(!out.is_open())
        throw std::runtime_error("Cannot open path '" + path_of_statistic_ + "'!");

    out << "actual : predicted" << std::endl;
    for(const auto &e : confusion_.histogram) {
        out << e.first.first << " : "
            << e.first.second << " "
            << e.second << std::endl;
    }
    out << "human parts found : human parts seen" << std::endl;
    out << human_parts_found_[0] << " " << human_parts_found_[1] << std::endl;
    out.close();
}

void EvaluateROIDetections::resetConfusion()
{
    confusion_.reset();
    human_parts_found_[0] = 0;
    human_parts_found_[1] = 0;
}

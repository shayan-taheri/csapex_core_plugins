/// HEADER
#include "evaluate_binary_classifier.h"

/// PROJECT
#include <csapex/msg/input.h>
#include <csapex/utility/register_apex_plugin.h>
#include <utils_param/parameter_factory.h>
#include <csapex/model/node_modifier.h>

/// SYSTEM
//#include <boost/assign.hpp>

CSAPEX_REGISTER_CLASS(csapex::EvaluateBinaryClassifier, csapex::Node)

using namespace csapex;
using namespace csapex::connection_types;


EvaluateBinaryClassifier::EvaluateBinaryClassifier()
{
}

void EvaluateBinaryClassifier::setupParameters()
{
}

void EvaluateBinaryClassifier::setup()
{
    in_ = modifier_->addInput<ConfusionMatrixMessage>("Confusion Matrix");
}

void EvaluateBinaryClassifier::process()
{
    connection_types::ConfusionMatrixMessage::Ptr msg = in_->getMessage<connection_types::ConfusionMatrixMessage>();

    const ConfusionMatrix& cm = msg->confusion;

    if(cm.classes.size() != 2) {
        throw std::logic_error("needs a confusion matrix with exactly 2 classes");
    }

    metrics_.clear();

    int tp = cm.histogram.at(std::make_pair(1, 1));
    int tn = cm.histogram.at(std::make_pair(0, 0));
    int fp = cm.histogram.at(std::make_pair(0, 1));
    int fn = cm.histogram.at(std::make_pair(1, 0));
    int p = tp + fn;
    int n = tn + fp;

    metrics_.push_back(Metric("Sensitivity", "True positive rate, hit rate, recall", 0.0, 1.0,
                              tp / double(p)));
    metrics_.push_back(Metric("specificity", "(SPC) or True Negative Rate", 0.0, 1.0,
                              tn / double(n)));
    metrics_.push_back(Metric("precision", "positive predictive value (PPV)", 0.0, 1.0,
                              tp / double(tp + fp)));
    metrics_.push_back(Metric("negative predictive value", "NPV", 0.0, 1.0,
                              tn / double(tn + fn)));
    metrics_.push_back(Metric("fall-out", "false positive rate (FPR)", 1.0, 0.0,
                              fp / double(n)));
    metrics_.push_back(Metric("false discovery rate", "FDR", 1.0, 0.0,
                              fp / double(fp + tp)));
    metrics_.push_back(Metric("Miss Rate", "False Negative Rate (FNR)", 1.0, 0.0,
                              fn / double(p)));

    metrics_.push_back(Metric("accuracy", "ACC", 0.0, 1.0,
                              (tp + tn) / double(p + n)));
    metrics_.push_back(Metric("F1 score", "the harmonic mean of precision and sensitivity", 0.0, 1.0,
                              (2 * tp) / double(2 * tp + fp + fn)));
    metrics_.push_back(Metric("Matthews correlation coefficient", "MCC", -1.0, 1.0,
                              (tp * tn - fp * fn)
                              /
                              std::sqrt(double(tp + fp) * double(tp + fn) * double(tn + fp) * double(tn + fn))));

    display_request();
}

const EvaluateBinaryClassifier::Metrics& EvaluateBinaryClassifier::getMetrics() const
{
    return metrics_;
}

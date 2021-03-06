/// COMPONENT
#include <csapex/model/node.h>

/// COMPONENT
#include <csapex/msg/generic_vector_message.hpp>
#include <csapex_opencv/cv_mat_message.h>
#include <csapex_opencv/roi_message.h>

/// PROJECT
#include <csapex/model/node_modifier.h>
#include <csapex/msg/io.h>
#include <csapex/param/parameter_factory.h>
#include <csapex/param/range_parameter.h>
#include <csapex/utility/register_apex_plugin.h>

namespace csapex
{
using namespace connection_types;

class GridArrangedRois : public csapex::Node
{
public:
    GridArrangedRois()
    {
    }

public:
    virtual void setup(csapex::NodeModifier& node_modifier) override
    {
        input_ = node_modifier.addInput<CvMatMessage>("Image");
        output_ = node_modifier.addOutput<GenericVectorMessage, RoiMessage>("ROIs");
    }

    virtual void setupParameters(Parameterizable& parameters) override
    {
        parameters.addParameter(csapex::param::factory::declareRange("dimension x", 1, 1000, 64, 1));
        parameters.addParameter(csapex::param::factory::declareRange("dimension y", 1, 1000, 48, 1));
        parameters.addParameter(csapex::param::factory::declareRange("class id", -1, 255, -1, 1));
        parameters.addParameter(csapex::param::factory::declareColorParameter("color", 255, 0, 0));
    }

    virtual void process() override
    {
        CvMatMessage::ConstPtr in = msg::getMessage<connection_types::CvMatMessage>(input_);
        std::shared_ptr<std::vector<RoiMessage>> out(new std::vector<RoiMessage>);

        int dim_x = readParameter<int>("dimension x");
        int dim_y = readParameter<int>("dimension y");
        int class_id = readParameter<int>("class id");
        const std::vector<int>& c = readParameter<std::vector<int>>("color");
        cv::Scalar color = cv::Scalar(c[2], c[1], c[0]);
        int cell_height = in->value.rows / dim_y;
        int rest_height = in->value.rows % dim_y;
        int cell_width = in->value.cols / dim_x;
        int rest_witdh = in->value.cols % dim_x;

        cv::Rect rect;
        for (int i = 0; i < dim_y; ++i) {
            for (int j = 0; j < dim_x; ++j) {
                RoiMessage roi;
                rect.x = cell_width * j;
                rect.y = cell_height * i;
                rect.width = cell_width + ((j == dim_x - 1) ? rest_witdh : 0);
                rect.height = cell_height + ((i == dim_y - 1) ? rest_height : 0);
                roi.value.setRect(rect);
                roi.value.setColor(color);
                roi.value.setClassification(class_id);
                out->push_back(roi);
            }
        }

        msg::publish<GenericVectorMessage, RoiMessage>(output_, out);
    }

private:
    Input* input_;
    Output* output_;
};
}  // namespace csapex

CSAPEX_REGISTER_CLASS(csapex::GridArrangedRois, csapex::Node)

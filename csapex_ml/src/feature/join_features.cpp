#include <csapex/model/node.h>
#include <csapex/model/node_modifier.h>
#include <csapex/model/variadic_io.h>
#include <csapex/msg/io.h>
#include <csapex/param/parameter_factory.h>
#include <csapex/utility/register_apex_plugin.h>

#include <csapex/msg/generic_vector_message.hpp>
#include <csapex_ml/features_message.h>

using namespace csapex::connection_types;

namespace csapex
{
class JoinFeatures : public Node, public VariadicInputs
{
public:
    virtual void setup(csapex::NodeModifier& node_modifier) override
    {
        VariadicInputs::setupVariadic(node_modifier);
        output_features_ = node_modifier.addOutput<GenericVectorMessage, FeaturesMessage>("Features");
    }
    virtual void setupParameters(Parameterizable& parameters) override
    {
        VariadicInputs::setupVariadicParameters(parameters);
    }

    virtual void process() override
    {
        std::shared_ptr<std::vector<FeaturesMessage>> output_features(new std::vector<FeaturesMessage>);

        int vectorSize = -1;
        for (const InputPtr& input : variadic_inputs_) {
            int inputSize;
            if (msg::isMessage<FeaturesMessage>(input.get()))
                inputSize = 1;
            else {
                auto msg = msg::getMessage<GenericVectorMessage, FeaturesMessage>(input.get());
                inputSize = msg->size();
            }

            if (vectorSize == -1)
                vectorSize = inputSize;
            else
                apex_assert(inputSize == vectorSize);
        }

        std::vector<std::vector<FeaturesMessage>> input_features;
        input_features.resize(vectorSize);

        for (const InputPtr& input : variadic_inputs_) {
            if (msg::isMessage<FeaturesMessage>(input.get())) {
                FeaturesMessage::ConstPtr feature = msg::getMessage<FeaturesMessage>(input.get());
                input_features[0].push_back(*feature);
            } else {
                std::shared_ptr<std::vector<FeaturesMessage> const> features = msg::getMessage<GenericVectorMessage, FeaturesMessage>(input.get());

                for (std::size_t i = 0; i < features->size(); ++i)
                    input_features[i].push_back((*features)[i]);
            }
        }

        for (auto& input_set : input_features) {
            int ref_class;
            if (input_set.empty()) {
                ref_class = 0;
            } else {
                apex_assert(input_set[0].type == FeaturesMessage::Type::CLASSIFICATION);
                ref_class = input_set[0].classification;
            }

            FeaturesMessage feature;
            feature.classification = ref_class;
            feature.confidence = 0;

            for (auto& part : input_set) {
                apex_assert(part.type == FeaturesMessage::Type::CLASSIFICATION);
                if (feature.classification != ref_class)
                    throw std::runtime_error("Feature classification mismatch");

                feature.value.insert(feature.value.end(), part.value.begin(), part.value.end());
            }

            output_features->emplace_back(std::move(feature));
        }

        msg::publish<GenericVectorMessage, FeaturesMessage>(output_features_, output_features);
    }

    Input* createVariadicInput(TokenDataConstPtr type, const std::string& label, bool /*optional*/) override
    {
        return VariadicInputs::createVariadicInput(multi_type::make<GenericVectorMessage, FeaturesMessage>(), label.empty() ? "Features" : label, getVariadicInputCount() == 0 ? false : true);
    }

private:
    Output* output_features_;
};

}  // namespace csapex

CSAPEX_REGISTER_CLASS(csapex::JoinFeatures, csapex::Node)

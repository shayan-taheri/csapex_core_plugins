#ifndef ANTSAC_HPP
#define ANTSAC_HPP

/// PROJECT
#include "sac.hpp"
#include "delegate.hpp"

/// SYSTEM
#include <random>
#include <set>
#include <vector>
#include <limits>

namespace csapex_sample_consensus {
struct AntsacParameters : public Parameters {
    double      outlier_probability      = 0.99;
    bool        use_outlier_probability  = false;
    int         maximum_sampling_retries = 100;

    double      rho = 0.9;
    double      alpha = 0.1;
    double      theta = 0.025;

    AntsacParameters() = default;
};

template <typename PointT>
class Antsac : public SampleConsensus<PointT>
{
public:
    using Ptr   = std::shared_ptr<Antsac>;
    using Base  = SampleConsensus<PointT>;
    using Model = typename Base::Model;

    Antsac(const std::vector<int> &indices,
           const AntsacParameters &parameters,
           std::default_random_engine &rng) :
        Base(indices),
        parameters_(parameters),
        distribution_(0.0, 1.0),
        rng_(rng),
        mean_inliers_(0.0),
        one_over_indices_(1.0 / static_cast<double>(Base::indices_.size())),
        tau_(Base::indices_.size(), one_over_indices_),
        tau_sum_(1.0),
        distances_(Base::indices_.size(), std::numeric_limits<double>::max())
    {
    }

    virtual void setIndices(const std::vector<int> &indices) override
    {
        Base::setIndices(indices);
        mean_inliers_ = 0.0;
        const std::size_t indices_size = Base::indices_.size();
        one_over_indices_ = 1.0 / static_cast<double>(indices_size);
        tau_.resize(indices_size, one_over_indices_);
        tau_sum_ = 1.0;
        distances_.resize(indices_size, std::numeric_limits<double>::max());
    }

    virtual bool computeModel(typename SampleConsensusModel<PointT>::Ptr &model) override
    {

        const std::size_t model_dimension = model->getModelDimension();
        if(Base::indices_.size() < model_dimension)
            return false;


        InternalParameters internal_params(parameters_, model_dimension);
        delegate<bool()> termination  = [&internal_params, this](){
            bool terminate_max_skipped          = internal_params.skipped >= internal_params.maximum_skipped;
            bool terminate_max_iteration        = internal_params.iteration >= parameters_.maximum_iterations;
            bool terminate_outlier_probability  = parameters_.use_outlier_probability &&
                    internal_params.iteration >= internal_params.k_outlier;
            bool terminate_mean_model_distance  = parameters_.use_mean_model_distance &&
                    internal_params.mean_model_distance < parameters_.mean_model_distance;
            return terminate_max_skipped || terminate_max_iteration || terminate_outlier_probability || terminate_mean_model_distance;
        };

        delegate<void()> update_internal_paramters = [&internal_params, this]() {
            if(parameters_.use_outlier_probability) {
                double maximum_inlier_ratio = internal_params.maximum_inliers * one_over_indices_;
                double p_no_outliers = 1.0 - std::pow(maximum_inlier_ratio, static_cast<double>(internal_params.model_dimension));
                p_no_outliers = std::max(std::numeric_limits<double>::epsilon (), p_no_outliers);
                p_no_outliers = std::min(1.0 - std::numeric_limits<double>::epsilon (), p_no_outliers);
                internal_params.k_outlier = internal_params.log_outlier_probability / std::log(p_no_outliers);
            }
        };

        while(!termination()) {
            if(!selectSamples(model, internal_params.model_dimension, internal_params.model_samples)) {
                break;
            }

            if(!model->computeModelCoefficients(internal_params.model_samples)) {
                ++internal_params.skipped;
                continue;
            }

            typename SampleConsensusModel<PointT>::InlierStatistic stat;
            model->getInlierStatistic(Base::indices_, parameters_.model_search_distance, stat);
            internal_params.updateMeanInliers(stat.count);

            if(stat.count > internal_params.maximum_inliers) {
                internal_params.maximum_inliers = stat.count;
                internal_params.best_model = model->clone();
                internal_params.mean_model_distance = stat.mean_distance;
                update_internal_paramters();
                updateTau(stat.count);
            }


            ++internal_params.iteration;
        }

        std::swap(model, internal_params.best_model);
        return model.get() != nullptr;
    }

protected:
    struct InternalParameters {
        const double log_outlier_probability;
        const std::size_t maximum_skipped;
        const std::size_t model_dimension;

        double      k_outlier = 1.0;
        std::size_t skipped = 0;
        std::size_t iteration = 0;

        std::size_t maximum_inliers = 0;
        typename Model::Ptr best_model;

        std::vector<int> model_samples;

        double mean_model_distance = std::numeric_limits<double>::max();

        double mean_inliers = 0.0;

        InternalParameters(const AntsacParameters &params,
                           const std::size_t model_dimension) :
            log_outlier_probability(std::log(1.0 - params.outlier_probability)),
            maximum_skipped(params.maximum_iterations * 10),
            model_dimension(model_dimension)
        {
        }

        void updateMeanInliers(const std::size_t current_inliers)
        {
            mean_inliers = (mean_inliers * iteration + current_inliers) / (1.0 + iteration);
        }


    };

    AntsacParameters                           parameters_;
    std::uniform_real_distribution<double>     distribution_;
    std::default_random_engine                &rng_;




    double              mean_inliers_;
    double              one_over_indices_;
    std::vector<double> tau_;
    double              tau_sum_;
    std::vector<double> distances_;

    inline void updateTau(const std::size_t inliers_size)
    {
        const std::size_t indices_size = Base::indices_.size();
        double delta_tau = inliers_size / (indices_size + mean_inliers_);
        tau_sum_ = 0.0;
        for(std::size_t i = 0 ; i < indices_size ; ++i) {
            tau_[i] = parameters_.rho * tau_[i] + delta_tau * std::exp(-0.5 * (distances_[i] / parameters_.theta));
            tau_sum_ += tau_[i];
        }
    }

    inline bool selectSamples(const typename Model::Ptr &model,
                              const std::size_t         samples,
                              std::vector<int>          &indices)
    {
        indices.clear();
        std::set<int> triple;
        auto drawTriple = [&triple, samples, this] () {
            const std::size_t size = tau_.size();

//            int rouletteSelect(double[] weight) {
//                // calculate the total weight
//                double weight_sum = 0;
//                for(int i=0; i<weight.length; i++) {
//                    weight_sum += weight[i];
//                }
//                // get a random value
//                double value = randUniformPositive() * weight_sum;
//                // locate the random value based on the weights
//                for(int i=0; i<weight.length; i++) {
//                    value -= weight[i];
//                    if(value <= 0) return i;
//                }
//                // when rounding errors occur, we return the last item's index
//                return weight.length - 1;
//            }
            while(triple.size() < samples) {
                std::size_t index = 0;
                double beta = tau_sum_ * distribution_(rng_);
                while (beta > 0.0) {
                    beta -= tau_[index];
                    index = (index + 1) % size;
                }
                triple.insert(index);
            }
        };


        for(std::size_t i = 0 ; i < parameters_.maximum_sampling_retries ; ++i) {
            drawTriple();
            if(model->validateSamples(triple)) {
                for(int i : triple)
                    indices.emplace_back(i);
                return true;
            }
        }
        return false;
    }
};

}

#endif // ANTSAC_HPP

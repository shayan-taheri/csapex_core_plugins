#ifndef TIME_PLOT_H
#define TIME_PLOT_H

/// PROJECT
#include "plot.h"

/// SYSTEM
#include <QColor>
#include <chrono>
#include <deque>
#include <qwt_plot_curve.h>
#include <qwt_plot_scaleitem.h>
#include <qwt_scale_map.h>

namespace csapex
{
class TimePlot : public Plot
{
private:
    typedef std::chrono::system_clock::time_point timepoint;

public:
    void setup(NodeModifier& node_modifier);
    void setupParameters(Parameterizable& parameters);
    void process();

    double getLineWidth() const;

    const double* getTData() const;
    const double* getVData(std::size_t idx) const;
    std::size_t getVDataCountNumCurves() const;
    std::size_t getCount() const;

protected:
    void reset();
    void init();

    void preparePlot();
    void renderAndSend();

private:
    bool initialize_;
    std::size_t deque_size_;
    Input* in_;
    Output* out_;

    double line_width_;

    bool time_relative_;
    bool time_seconds_;

    double start_t_;
    std::deque<double> data_t_raw_;

    std::vector<double> data_t_;
    std::vector<std::deque<double>> deque_v_;
    std::vector<std::vector<double>> data_v_;
};

}  // namespace csapex

#endif  // TIME_PLOT_H

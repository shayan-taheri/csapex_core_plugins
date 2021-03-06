/// HEADER
#include "confusion_matrix_display_adapter.h"

/// PROJECT
#include <csapex/model/node_facade_impl.h>
#include <csapex/msg/io.h>
#include <csapex/view/utility/register_node_adapter.h>

/// SYSTEM
#include <QApplication>
#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QPainter>
#include <QTableView>

using namespace csapex;

CSAPEX_REGISTER_LOCAL_NODE_ADAPTER(ConfusionMatrixDisplayAdapter, csapex::ConfusionMatrixDisplay)

ConfusionMatrixTableModel::ConfusionMatrixTableModel() : dim(0)
{
}

void ConfusionMatrixTableModel::update(const ConfusionMatrix& confusion)
{
    confusion_ = confusion;

    int new_dim = confusion_.classes.size();

    if (dim < new_dim) {
        beginInsertRows(QModelIndex(), dim, new_dim - 1);
        beginInsertColumns(QModelIndex(), dim, new_dim - 1);

        endInsertRows();
        endInsertColumns();

    } else if (new_dim < dim) {
        beginRemoveRows(QModelIndex(), new_dim, dim - 1);
        beginRemoveColumns(QModelIndex(), new_dim, dim - 1);

        endRemoveRows();
        endRemoveColumns();
    }

    dim = new_dim;

    sum.clear();
    sum.resize(dim);

    try {
        for (int col = 0; col < dim; ++col) {
            sum[col] = 0;
            for (int row = 0; row < dim; ++row) {
                sum[col] += confusion_.histogram[std::make_pair(confusion_.classes[row], confusion_.classes[col])];
            }
        }
    } catch (const std::exception& e) {
        sum.clear();
        dim = 0;
    }
}

int ConfusionMatrixTableModel::rowCount(const QModelIndex& parent) const
{
    return dim;
}

int ConfusionMatrixTableModel::columnCount(const QModelIndex& parent) const
{
    return dim;
}

QVariant ConfusionMatrixTableModel::data(const QModelIndex& index, int role) const
{
    if (role != Qt::ForegroundRole && role != Qt::BackgroundColorRole && role != Qt::DisplayRole) {
        return QVariant();
    }

    auto actual = confusion_.classes.at(index.column());
    auto prediction = confusion_.classes.at(index.row());
    auto key = std::make_pair(actual, prediction);
    int entry = 0;
    if (confusion_.histogram.find(key) != confusion_.histogram.end()) {
        entry = confusion_.histogram.at(key);
    }
    if (role == Qt::DisplayRole) {
        return entry;
    }
    auto it = std::find(confusion_.classes.begin(), confusion_.classes.end(), actual);
    std::size_t id = it - confusion_.classes.begin();
    double f = entry / double(sum[id]);
    //    double f = entry / double(sum[actual]);

    static QColor min_color = QColor::fromRgb(255, 255, 255);
    static QColor max_color = QColor::fromRgb(0, 0, 0);

    int grey = std::min(255, std::max(0, int(min_color.red() * (1.0 - f) + max_color.red() * f)));

    if (role == Qt::ForegroundRole) {
        int v = grey < 100 ? 255 : 0;
        return QVariant::fromValue(QColor::fromRgb(v, v, v));
    } else {
        return QVariant::fromValue(QColor::fromRgb(grey, grey, grey));
    }
}

QVariant ConfusionMatrixTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    int c = confusion_.classes.at(section);
    auto pos = confusion_.class_names.find(c);
    if (pos == confusion_.class_names.end()) {
        return c;
    } else {
        return QString::fromStdString(pos->second);
    }
}

ConfusionMatrixDisplayAdapter::ConfusionMatrixDisplayAdapter(NodeFacadeImplementationPtr worker, NodeBox* parent, std::weak_ptr<ConfusionMatrixDisplay> node)
  : DefaultNodeAdapter(worker, parent), wrapped_(node)
{
    auto n = wrapped_.lock();
    // translate to UI thread via Qt signal
    observe(n->display_request, this, &ConfusionMatrixDisplayAdapter::displayRequest);
    observe(n->export_request, this, &ConfusionMatrixDisplayAdapter::exportRequest);
}

void ConfusionMatrixDisplayAdapter::setupUi(QBoxLayout* layout)
{
    model_ = new ConfusionMatrixTableModel;

    table_ = new QTableView;
    table_->setModel(model_);
    table_->showGrid();

    table_->setMinimumSize(0, 0);
    table_->viewport()->setMinimumSize(0, 0);
    table_->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);

    QGridLayout* grid = new QGridLayout;
    layout->addLayout(grid);

    auto actual = new QLabel("actual");

    grid->addWidget(actual, 0, 1);
    grid->addWidget(new QLabel("predicted"), 1, 0);

    grid->addWidget(table_, 1, 1);

    connect(this, SIGNAL(displayRequest()), this, SLOT(display()));
    connect(this, SIGNAL(exportRequest()), this, SLOT(exportCsv()));

    DefaultNodeAdapter::setupUi(layout);
}

void ConfusionMatrixDisplayAdapter::display()
{
    auto node = wrapped_.lock();
    if (!node) {
        return;
    }

    model_->update(node->getConfusionMatrix());

    table_->resizeColumnsToContents();
    table_->resizeRowsToContents();
    table_->viewport()->update();

    table_->adjustSize();
}

void ConfusionMatrixDisplayAdapter::exportCsv()
{
    QString filename = QFileDialog::getSaveFileName(0, "Save CSV File", "", "*.csv", 0, QFileDialog::DontUseNativeDialog);
    if (!filename.isEmpty()) {
        if (auto node = wrapped_.lock()) {
            node->exportCsv(filename.toStdString());
        }
    }
}

/// MOC
#include "moc_confusion_matrix_display_adapter.cpp"

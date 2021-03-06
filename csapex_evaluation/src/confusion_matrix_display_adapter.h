#ifndef CONFUSION_MATRIX_DISPLAY_ADAPTER_H
#define CONFUSION_MATRIX_DISPLAY_ADAPTER_H

/// PROJECT
#include <csapex/view/node/default_node_adapter.h>

/// COMPONENT
#include "confusion_matrix_display.h"

/// SYSTEM
#include <QStyledItemDelegate>
#include <QTableView>

namespace csapex
{
class ConfusionMatrixTableModel : public QAbstractTableModel
{
public:
    ConfusionMatrixTableModel();

    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;

    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    void update(const ConfusionMatrix& confusion_matrix);

private:
    ConfusionMatrix confusion_;

    std::vector<int> sum;
    int dim;
};

class ConfusionMatrixDisplayAdapter : public QObject, public DefaultNodeAdapter
{
    Q_OBJECT

public:
    ConfusionMatrixDisplayAdapter(NodeFacadeImplementationPtr worker, NodeBox* parent, std::weak_ptr<ConfusionMatrixDisplay> node);

    virtual void setupUi(QBoxLayout* layout);

public Q_SLOTS:
    void display();
    void exportCsv();

Q_SIGNALS:
    void displayRequest();
    void exportRequest();

protected:
    std::weak_ptr<ConfusionMatrixDisplay> wrapped_;

private:
    ConfusionMatrixTableModel* model_;
    QTableView* table_;
};

}  // namespace csapex
#endif  // CONFUSION_MATRIX_DISPLAY_ADAPTER_H

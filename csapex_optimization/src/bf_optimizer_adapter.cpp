/// HEADER
#include "bf_optimizer_adapter.h"

/// COMPONENT
#include <csapex_optimization/parameter_dialog.h>

/// PROJECT
#include <csapex/utility/register_node_adapter.h>
#include <utils_param/range_parameter.h>

/// SYSTEM
#include <QPushButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QFormLayout>
#include <QProgressBar>

using namespace csapex;

CSAPEX_REGISTER_NODE_ADAPTER(BFOptimizerAdapter, csapex::BFOptimizer)


BFOptimizerAdapter::BFOptimizerAdapter(NodeWorker* worker, BFOptimizer *node, WidgetController* widget_ctrl)
    : DefaultNodeAdapter(worker, widget_ctrl), wrapped_(node)
{
    wrapped_->step.connect(std::bind(&BFOptimizerAdapter::triggerStep, this, std::placeholders::_1));
}



void BFOptimizerAdapter::setupUi(QBoxLayout* layout)
{
    DefaultNodeAdapter::setupUi(layout);

    QPushButton* btn_add_param = new QPushButton("+");
    layout->addWidget(btn_add_param);
    QObject::connect(btn_add_param, SIGNAL(clicked()), this, SLOT(createParameter()));


    QPushButton* btn_start_optimization = new QPushButton("start");
    layout->addWidget(btn_start_optimization);
    QObject::connect(btn_start_optimization, SIGNAL(clicked()), this, SLOT(startOptimization()));


    QPushButton* btn_stop_optimization = new QPushButton("stop");
    layout->addWidget(btn_stop_optimization);
    QObject::connect(btn_stop_optimization, SIGNAL(clicked()), this, SLOT(stopOptimization()));


    progress_ = new QProgressBar;
    layout->addWidget(progress_);
}

void BFOptimizerAdapter::startOptimization()
{
    wrapped_->start();
}


void BFOptimizerAdapter::stopOptimization()
{
    wrapped_->stop();
}

QDialog* BFOptimizerAdapter::makeTypeDialog()
{
    QVBoxLayout* layout = new QVBoxLayout;

    QFormLayout* form = new QFormLayout;

    QComboBox* type = new QComboBox;
    type->addItem("double");
    form->addRow("Type", type);

    QObject::connect(type, SIGNAL(currentIndexChanged(QString)), this, SLOT(setNextParameterType(QString)));
    next_type_ = type->currentText().toStdString();

    layout->addLayout(form);

    QDialogButtonBox* buttons = new QDialogButtonBox;
    buttons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);


    QDialog* dialog = new QDialog;
    dialog->setWindowTitle("Create Parameter");
    dialog->setLayout(layout);
    dialog->setModal(true);

    QObject::connect(buttons, SIGNAL(accepted()), dialog, SLOT(accept()));
    QObject::connect(buttons, SIGNAL(rejected()), dialog, SLOT(reject()));

    return dialog;
}

void BFOptimizerAdapter::setNextParameterType(const QString &type)
{
    next_type_ = type.toStdString();
}


void BFOptimizerAdapter::createParameter()
{
    QDialog* type_dialog = makeTypeDialog();
    if(type_dialog->exec() == QDialog::Accepted) {

        ParameterDialog diag(next_type_);
        if(diag.exec() == QDialog::Accepted) {
            param::Parameter::Ptr param = diag.getParameter();
            wrapped_->addPersistentParameter(param);

            progress_->setMaximum(wrapped_->stepsNecessary());
        }
    }
}

void BFOptimizerAdapter::triggerStep(int s)
{
    progress_->setValue(s);
    progress_->setMaximum(wrapped_->stepsNecessary());
}

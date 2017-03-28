//License: MIT, (c) Oleksiy Zakharov, 2016, alexzkhr@gmail.com

#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "globalsettings.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    readSettings(this);

    const StaticSettingsMap& sett = StaticSettingsMap::getGlobalSetts();
    auto w = new QWidget();
    ui->scrollSettings->setWidget(w);
    auto layout = new QVBoxLayout();
    layout->setAlignment(Qt::AlignTop);
    w->setLayout(layout);

    auto wl = sett.createWidgets();
    for (const auto& wi : wl)
    {
        wi->setParent(this);
        layout->addWidget(wi);
    }
}

SettingsDialog::~SettingsDialog()
{
    writeSettings(this);
    delete ui;
}

void SettingsDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
        case QEvent::LanguageChange:
            ui->retranslateUi(this);
            break;
        default:
            break;
    }
}

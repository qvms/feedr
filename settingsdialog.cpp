#include "settingsdialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QSpinBox>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(const AppSettings &settings, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Settings"));
    setModal(true);
    resize(420, 240);

    reopenTabs_ = new QCheckBox(QStringLiteral("Reopen tabs from the last session"), this);
    showExplorer_ = new QCheckBox(QStringLiteral("Show the file explorer when feedr starts"), this);
    lightMode_ = new QCheckBox(QStringLiteral("Light mode"), this);
    lightMode_->setToolTip(QStringLiteral("Turn on if you want light mode"));
    reopenTabs_->setChecked(settings.reopenTabs);
    showExplorer_->setChecked(settings.showExplorer);
    lightMode_->setChecked(settings.lightMode);

    searchLimit_ = new QSpinBox(this);
    searchLimit_->setRange(1, 200);
    searchLimit_->setValue(settings.searchLimit);

    fontSize_ = new QSpinBox(this);
    fontSize_->setRange(10, 28);
    fontSize_->setSuffix(QStringLiteral(" pt"));
    fontSize_->setValue(settings.fontSize);

    recentLimit_ = new QSpinBox(this);
    recentLimit_->setRange(4, 40);
    recentLimit_->setValue(settings.recentLimit);

    auto *form = new QFormLayout();
    form->addRow(reopenTabs_);
    form->addRow(showExplorer_);
    form->addRow(lightMode_);
    form->addRow(QStringLiteral("Search results"), searchLimit_);
    form->addRow(QStringLiteral("Reader font"), fontSize_);
    form->addRow(QStringLiteral("Previous files kept"), recentLimit_);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

AppSettings SettingsDialog::settings() const
{
    AppSettings value;
    value.reopenTabs = reopenTabs_->isChecked();
    value.showExplorer = showExplorer_->isChecked();
    value.lightMode = lightMode_->isChecked();
    value.searchLimit = searchLimit_->value();
    value.fontSize = fontSize_->value();
    value.recentLimit = recentLimit_->value();
    return value;
}

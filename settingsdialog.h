#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "library.h"

#include <QDialog>

class QCheckBox;
class QSpinBox;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(const AppSettings &settings, QWidget *parent = nullptr);

    AppSettings settings() const;

private:
    QCheckBox *reopenTabs_ = nullptr;
    QCheckBox *showExplorer_ = nullptr;
    QCheckBox *lightMode_ = nullptr;
    QSpinBox *searchLimit_ = nullptr;
    QSpinBox *fontSize_ = nullptr;
    QSpinBox *recentLimit_ = nullptr;
};

#endif

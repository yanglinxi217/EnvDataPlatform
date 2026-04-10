#pragma once

#include <QDialog>
#include <QDoubleSpinBox>
#include <QLabel>

struct AlarmThresholds {
    double temperature = 35.0;
    double humidity    = 80.0;
    double pm25        = 75.0;
    double co2         = 1000.0;
};

class AlarmSettingDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AlarmSettingDialog(const AlarmThresholds &current, QWidget *parent = nullptr);

    AlarmThresholds thresholds() const;

private:
    void setupUi(const AlarmThresholds &current);

    QDoubleSpinBox *m_tempSpin;
    QDoubleSpinBox *m_humiSpin;
    QDoubleSpinBox *m_pm25Spin;
    QDoubleSpinBox *m_co2Spin;
};

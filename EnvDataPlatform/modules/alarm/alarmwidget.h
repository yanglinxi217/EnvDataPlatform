#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QDateTimeEdit>
#include "alarmsettingdialog.h"
#include "../../core/models/envdata.h"
#include "../../core/models/alarmrecord.h"

class AlarmWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AlarmWidget(QWidget *parent = nullptr);

    void checkData(const EnvData &data);
    AlarmThresholds thresholds() const { return m_thresholds; }

signals:
    void thresholdsChanged(const AlarmThresholds &t);
    void newAlarmCount(int total);

private slots:
    void onSetThresholds();
    void onClearAlarms();
    void onRefresh();
    void onQueryByRange();

private:
    void setupUi();
    void triggerAlarm(const AlarmRecord &record);
    void appendAlarmRow(const AlarmRecord &r);

    QTableWidget  *m_table;
    QPushButton   *m_settingsBtn;
    QPushButton   *m_clearBtn;
    QPushButton   *m_refreshBtn;
    QLabel        *m_totalLabel;

    // Threshold display labels
    QLabel *m_tempThreshLabel;
    QLabel *m_humiThreshLabel;
    QLabel *m_pm25ThreshLabel;
    QLabel *m_co2ThreshLabel;

    // Date filter
    QDateTimeEdit *m_filterFrom;
    QDateTimeEdit *m_filterTo;

    AlarmThresholds m_thresholds;
    int             m_alarmCount = 0;

    // Debounce: prevent alarm flood (min 10s between same-param alarms)
    QMap<QString, qint64> m_lastAlarmTime;
};

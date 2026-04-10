#pragma once

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QComboBox>
#include <QPushButton>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QDateTimeAxis>
#include <QList>
#include "../../core/models/envdata.h"

class RealtimeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RealtimeWidget(QWidget *parent = nullptr);

    void updateData(const EnvData &data);
    void setAlarmThresholds(double temp, double humi, double pm25, double co2);

signals:
    void intervalChanged(int ms);

private:
    void setupUi();
    void setupCharts();
    void updateCard(int index, double value);
    void appendToSeries(const EnvData &data);
    void updateProgressBar(QProgressBar *bar, double value, double max,
                           double warnThreshold, double dangerThreshold);

    // Cards: [0]=temp [1]=humi [2]=pm25 [3]=co2
    struct SensorCard {
        QLabel      *nameLabel;
        QLabel      *valueLabel;
        QLabel      *unitLabel;
        QLabel      *statusLabel;
        QProgressBar*bar;
        double       alarmThreshold;
    };
    SensorCard m_cards[4];

    QComboBox  *m_intervalBox;
    QPushButton*m_pauseBtn;
    bool        m_paused = false;

    // Charts (one per parameter)
    QChart      *m_chart;
    QChartView  *m_chartView;
    QLineSeries *m_tempSeries;
    QLineSeries *m_humiSeries;
    QLineSeries *m_pm25Series;
    QLineSeries *m_co2Series;
    QDateTimeAxis *m_axisX;
    QValueAxis    *m_axisY;

    static const int kMaxPoints = 60;
    QList<EnvData>   m_history;
};

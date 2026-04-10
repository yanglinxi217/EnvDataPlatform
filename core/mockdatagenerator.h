#pragma once

#include <QObject>
#include <QTimer>
#include <QRandomGenerator>
#include "core/models/envdata.h"

/**
 * MockDataGenerator
 *
 * Simulates environmental sensor readings.
 * Emits newData() at a configurable interval.
 * Data values slowly drift to create realistic-looking time-series.
 */
class MockDataGenerator : public QObject
{
    Q_OBJECT
public:
    explicit MockDataGenerator(QObject *parent = nullptr);

    void start(int intervalMs = 2000);
    void stop();
    void setInterval(int intervalMs);
    int  interval() const { return m_timer.interval(); }

signals:
    void newData(const EnvData &data);

private slots:
    void generateData();

private:
    double driftValue(double current, double min, double max,
                      double step, double spike_prob = 0.05);

    QTimer  m_timer;

    // Current "drifting" sensor values
    double  m_temperature = 24.0;
    double  m_humidity    = 55.0;
    double  m_pm25        = 30.0;
    double  m_co2         = 650.0;
};

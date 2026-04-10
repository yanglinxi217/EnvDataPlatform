#include "mockdatagenerator.h"
#include <cmath>

MockDataGenerator::MockDataGenerator(QObject *parent)
    : QObject(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &MockDataGenerator::generateData);
}

void MockDataGenerator::start(int intervalMs)
{
    m_timer.start(intervalMs);
}

void MockDataGenerator::stop()
{
    m_timer.stop();
}

void MockDataGenerator::setInterval(int intervalMs)
{
    bool wasActive = m_timer.isActive();
    m_timer.stop();
    m_timer.setInterval(intervalMs);
    if (wasActive) m_timer.start();
}

/**
 * Slowly drifts 'current' toward a random target within [min, max].
 * Occasionally introduces a spike (abnormal value) to trigger alarms.
 */
double MockDataGenerator::driftValue(double current, double min, double max,
                                     double step, double spike_prob)
{
    QRandomGenerator *rng = QRandomGenerator::global();

    // 5% chance of generating a spike value outside normal range
    if (rng->generateDouble() < spike_prob) {
        // Spike: go outside normal range
        double spikeDelta = (rng->generateDouble() * 0.3 + 0.15) * (max - min);
        bool   above = rng->bounded(2) == 0;
        return above ? max + spikeDelta : min - spikeDelta / 2.0;
    }

    // Normal drift: random walk clamped to [min, max]
    double delta = (rng->generateDouble() * 2.0 - 1.0) * step;
    double next  = current + delta;
    // Soft clamp: bias back toward center when near edges
    double center = (min + max) / 2.0;
    next += (center - next) * 0.02;
    return qBound(min, next, max);
}

void MockDataGenerator::generateData()
{
    // Normal operating ranges for drift
    m_temperature = driftValue(m_temperature, 18.0, 38.0, 0.5, 0.04);
    m_humidity    = driftValue(m_humidity,    35.0, 75.0, 1.0, 0.03);
    m_pm25        = driftValue(m_pm25,         5.0, 80.0, 3.0, 0.04);
    m_co2         = driftValue(m_co2,        400.0,1200.0,20.0, 0.04);

    EnvData data;
    // Round to 1 decimal place
    data.temperature = std::round(m_temperature * 10.0) / 10.0;
    data.humidity    = std::round(m_humidity    * 10.0) / 10.0;
    data.pm25        = std::round(m_pm25        * 10.0) / 10.0;
    data.co2         = std::round(m_co2         * 10.0) / 10.0;
    data.recordedAt  = QDateTime::currentDateTime();

    emit newData(data);
}

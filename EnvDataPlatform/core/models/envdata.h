#pragma once
#include <QString>
#include <QDateTime>

struct EnvData {
    int     id          = 0;
    double  temperature = 0.0;   // °C
    double  humidity    = 0.0;   // %
    double  pm25        = 0.0;   // µg/m³
    double  co2         = 0.0;   // ppm
    QDateTime recordedAt;

    EnvData() = default;
    EnvData(double t, double h, double p, double c)
        : temperature(t), humidity(h), pm25(p), co2(c)
        , recordedAt(QDateTime::currentDateTime()) {}
};

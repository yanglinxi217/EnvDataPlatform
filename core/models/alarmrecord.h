#pragma once
#include <QString>
#include <QDateTime>

struct AlarmRecord {
    int       id        = 0;
    QString   parameter;     // "temperature" | "humidity" | "pm25" | "co2"
    double    value     = 0.0;
    double    threshold = 0.0;
    QDateTime alarmTime;

    // Human-readable parameter name
    QString parameterDisplayName() const {
        if (parameter == "temperature") return QStringLiteral("温度");
        if (parameter == "humidity")    return QStringLiteral("湿度");
        if (parameter == "pm25")        return QStringLiteral("PM2.5");
        if (parameter == "co2")         return QStringLiteral("CO₂");
        return parameter;
    }

    QString unit() const {
        if (parameter == "temperature") return QStringLiteral("°C");
        if (parameter == "humidity")    return QStringLiteral("%");
        if (parameter == "pm25")        return QStringLiteral("µg/m³");
        if (parameter == "co2")         return QStringLiteral("ppm");
        return QString();
    }
};

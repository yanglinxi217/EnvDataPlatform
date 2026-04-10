#pragma once

#include <QObject>
#include <QList>
#include "../../core/models/envdata.h"

class DataExporter : public QObject
{
    Q_OBJECT
public:
    explicit DataExporter(QObject *parent = nullptr);

    // Returns true on success. Shows file dialog internally.
    bool exportToCsv(const QList<EnvData> &data, QWidget *parentWidget = nullptr);

    // Export all historical data from DB
    bool exportAllToCsv(QWidget *parentWidget = nullptr);
};

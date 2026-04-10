#include "dataexporter.h"
#include "../../core/database/databasemanager.h"

#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDateTime>
#include <QWidget>

DataExporter::DataExporter(QObject *parent)
    : QObject(parent)
{}

bool DataExporter::exportToCsv(const QList<EnvData> &data, QWidget *parentWidget)
{
    if (data.isEmpty()) {
        QMessageBox::information(parentWidget, "提示", "没有可导出的数据");
        return false;
    }

    QString defaultName = QString("env_data_%1.csv")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));

    QString path = QFileDialog::getSaveFileName(
        parentWidget, "导出CSV文件", defaultName,
        "CSV文件 (*.csv);;所有文件 (*.*)");

    if (path.isEmpty()) return false;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(parentWidget, "导出失败",
            "无法写入文件：\n" + path + "\n\n" + file.errorString());
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // BOM for Excel UTF-8 recognition
    out << "\xEF\xBB\xBF";

    // Header
    out << "序号,记录时间,温度(°C),湿度(%),PM2.5(µg/m³),CO₂(ppm)\n";

    // Data rows
    for (int i = 0; i < data.size(); ++i) {
        const auto &d = data[i];
        out << (i + 1) << ","
            << d.recordedAt.toString("yyyy-MM-dd hh:mm:ss") << ","
            << QString::number(d.temperature, 'f', 1) << ","
            << QString::number(d.humidity,    'f', 1) << ","
            << QString::number(d.pm25,        'f', 1) << ","
            << QString::number(d.co2,         'f', 1) << "\n";
    }

    file.close();

    QMessageBox::information(parentWidget, "导出成功",
        QString("已成功导出 %1 条数据到：\n%2").arg(data.size()).arg(path));
    return true;
}

bool DataExporter::exportAllToCsv(QWidget *parentWidget)
{
    QDateTime from(QDate(2000, 1, 1), QTime(0, 0, 0));
    QDateTime to = QDateTime::currentDateTime();
    auto data = DatabaseManager::instance().queryEnvData(from, to);
    return exportToCsv(data, parentWidget);
}

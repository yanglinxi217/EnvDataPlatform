#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QDateTimeEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include "../../core/models/envdata.h"

class HistoryWidget : public QWidget
{
    Q_OBJECT
public:
    explicit HistoryWidget(QWidget *parent = nullptr);

signals:
    void exportRequested(const QList<EnvData> &data);

private slots:
    void onQuery();
    void onQueryPreset(int days);

private:
    void setupUi();
    void setupStatsChart();
    void populateTable(const QList<EnvData> &data);
    void updateStatsChart(const QDateTime &from, const QDateTime &to);

    QDateTimeEdit *m_fromEdit;
    QDateTimeEdit *m_toEdit;
    QComboBox     *m_paramBox;
    QPushButton   *m_queryBtn;
    QPushButton   *m_exportBtn;
    QLabel        *m_resultLabel;

    QTableWidget  *m_table;

    QChart        *m_statsChart;
    QChartView    *m_statsView;

    QList<EnvData> m_currentData;
};

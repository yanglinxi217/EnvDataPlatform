#include "historywidget.h"
#include "../../core/database/databasemanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QHeaderView>
#include <QMessageBox>
#include <QTabWidget>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLegend>

HistoryWidget::HistoryWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    setupStatsChart();
}

void HistoryWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(12);

    // Section title
    auto *title = new QLabel("历史数据查询");
    title->setObjectName("sectionTitle");
    mainLayout->addWidget(title);

    // --- Query panel ---
    auto *queryGroup = new QGroupBox("查询条件");
    auto *queryLayout = new QGridLayout(queryGroup);
    queryLayout->setHorizontalSpacing(12);
    queryLayout->setVerticalSpacing(8);

    queryLayout->addWidget(new QLabel("开始时间："), 0, 0);
    m_fromEdit = new QDateTimeEdit(QDateTime::currentDateTime().addDays(-1));
    m_fromEdit->setDisplayFormat("yyyy-MM-dd hh:mm:ss");
    m_fromEdit->setCalendarPopup(true);
    m_fromEdit->setFixedHeight(36);
    queryLayout->addWidget(m_fromEdit, 0, 1);

    queryLayout->addWidget(new QLabel("结束时间："), 0, 2);
    m_toEdit = new QDateTimeEdit(QDateTime::currentDateTime());
    m_toEdit->setDisplayFormat("yyyy-MM-dd hh:mm:ss");
    m_toEdit->setCalendarPopup(true);
    m_toEdit->setFixedHeight(36);
    queryLayout->addWidget(m_toEdit, 0, 3);

    queryLayout->addWidget(new QLabel("参数筛选："), 1, 0);
    m_paramBox = new QComboBox;
    m_paramBox->addItem("全部参数", "all");
    m_paramBox->addItem("温度",     "temperature");
    m_paramBox->addItem("湿度",     "humidity");
    m_paramBox->addItem("PM2.5",   "pm25");
    m_paramBox->addItem("CO₂",     "co2");
    m_paramBox->setFixedHeight(36);
    queryLayout->addWidget(m_paramBox, 1, 1);

    // Preset buttons
    auto *presetLayout = new QHBoxLayout;
    auto *todayBtn = new QPushButton("今天");
    auto *weekBtn  = new QPushButton("近7天");
    auto *monthBtn = new QPushButton("近30天");
    todayBtn->setObjectName("secondaryButton"); todayBtn->setFixedHeight(32);
    weekBtn->setObjectName("secondaryButton");  weekBtn->setFixedHeight(32);
    monthBtn->setObjectName("secondaryButton"); monthBtn->setFixedHeight(32);
    presetLayout->addWidget(new QLabel("快速选择："));
    presetLayout->addWidget(todayBtn);
    presetLayout->addWidget(weekBtn);
    presetLayout->addWidget(monthBtn);
    presetLayout->addStretch();
    queryLayout->addLayout(presetLayout, 1, 2, 1, 2);

    // Query & Export buttons
    auto *btnLayout = new QHBoxLayout;
    m_queryBtn = new QPushButton("查  询");
    m_queryBtn->setFixedHeight(38);
    m_queryBtn->setFixedWidth(100);

    m_exportBtn = new QPushButton("导出CSV");
    m_exportBtn->setObjectName("secondaryButton");
    m_exportBtn->setFixedHeight(38);
    m_exportBtn->setFixedWidth(100);

    m_resultLabel = new QLabel("请设置查询条件后点击查询");
    m_resultLabel->setStyleSheet("color: #7F8C8D; font-size: 12px;");

    btnLayout->addWidget(m_queryBtn);
    btnLayout->addWidget(m_exportBtn);
    btnLayout->addSpacing(16);
    btnLayout->addWidget(m_resultLabel);
    btnLayout->addStretch();
    queryLayout->addLayout(btnLayout, 2, 0, 1, 4);

    mainLayout->addWidget(queryGroup);

    // --- Tab: data table + stats chart ---
    auto *tabs = new QTabWidget;

    // Tab 1: Data table
    m_table = new QTableWidget;
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({"序号", "记录时间", "温度(°C)", "湿度(%)", "PM2.5(µg/m³)", "CO₂(ppm)"});
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->hide();
    tabs->addTab(m_table, "数据明细");

    // Tab 2: Statistics chart
    m_statsView = new QChartView;
    m_statsView->setRenderHint(QPainter::Antialiasing);
    m_statsView->setMinimumHeight(300);
    tabs->addTab(m_statsView, "统计图表");

    mainLayout->addWidget(tabs, 1);

    // Connections
    connect(m_queryBtn,  &QPushButton::clicked, this, &HistoryWidget::onQuery);
    connect(m_exportBtn, &QPushButton::clicked, this, [this](){
        if (m_currentData.isEmpty()) {
            QMessageBox::information(this, "提示", "没有可导出的数据，请先查询");
            return;
        }
        emit exportRequested(m_currentData);
    });
    connect(todayBtn, &QPushButton::clicked, this, [this](){ onQueryPreset(0); });
    connect(weekBtn,  &QPushButton::clicked, this, [this](){ onQueryPreset(7); });
    connect(monthBtn, &QPushButton::clicked, this, [this](){ onQueryPreset(30); });
}

void HistoryWidget::setupStatsChart()
{
    m_statsChart = new QChart;
    m_statsChart->setTitle("各参数统计（最大值/最小值/平均值）");
    m_statsChart->legend()->setVisible(true);
    m_statsChart->legend()->setAlignment(Qt::AlignBottom);
    m_statsView->setChart(m_statsChart);
}

void HistoryWidget::onQueryPreset(int days)
{
    QDateTime to   = QDateTime::currentDateTime();
    QDateTime from = (days == 0) ?
        QDateTime(QDate::currentDate(), QTime(0, 0, 0)) :
        to.addDays(-days);
    m_fromEdit->setDateTime(from);
    m_toEdit->setDateTime(to);
    onQuery();
}

void HistoryWidget::onQuery()
{
    QDateTime from = m_fromEdit->dateTime();
    QDateTime to   = m_toEdit->dateTime();

    if (from >= to) {
        QMessageBox::warning(this, "时间无效", "开始时间必须早于结束时间");
        return;
    }
    if (to.toSecsSinceEpoch() - from.toSecsSinceEpoch() > 90LL * 24 * 3600) {
        QMessageBox::warning(this, "范围过大", "查询时间范围不能超过90天，建议分段查询");
        return;
    }

    auto data = DatabaseManager::instance().queryEnvData(from, to);

    // Filter by parameter if needed (columns hidden for partial display)
    m_currentData = data;
    populateTable(data);
    updateStatsChart(from, to);

    m_resultLabel->setText(data.isEmpty() ?
        "未找到符合条件的数据" :
        QString("共查询到 %1 条记录").arg(data.size()));
    m_resultLabel->setStyleSheet(data.isEmpty() ?
        "color: #E67E22; font-size: 12px;" :
        "color: #27AE60; font-size: 12px;");
}

void HistoryWidget::populateTable(const QList<EnvData> &data)
{
    QString param = m_paramBox->currentData().toString();

    // Adjust column visibility
    bool showAll  = (param == "all");
    m_table->setColumnHidden(2, !showAll && param != "temperature");
    m_table->setColumnHidden(3, !showAll && param != "humidity");
    m_table->setColumnHidden(4, !showAll && param != "pm25");
    m_table->setColumnHidden(5, !showAll && param != "co2");

    m_table->setRowCount(data.size());
    for (int i = 0; i < data.size(); ++i) {
        const auto &d = data[i];
        m_table->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
        m_table->setItem(i, 1, new QTableWidgetItem(d.recordedAt.toString("yyyy-MM-dd hh:mm:ss")));
        m_table->setItem(i, 2, new QTableWidgetItem(QString::number(d.temperature, 'f', 1)));
        m_table->setItem(i, 3, new QTableWidgetItem(QString::number(d.humidity,    'f', 1)));
        m_table->setItem(i, 4, new QTableWidgetItem(QString::number(d.pm25,        'f', 1)));
        m_table->setItem(i, 5, new QTableWidgetItem(QString::number(d.co2,         'f', 1)));

        // Center align numbers
        for (int c = 0; c < 6; ++c)
            if (m_table->item(i, c))
                m_table->item(i, c)->setTextAlignment(Qt::AlignCenter);
    }
}

void HistoryWidget::updateStatsChart(const QDateTime &from, const QDateTime &to)
{
    m_statsChart->removeAllSeries();
    for (auto *ax : m_statsChart->axes()) m_statsChart->removeAxis(ax);

    auto stats = DatabaseManager::instance().queryStats(from, to);
    if (stats.isEmpty()) return;

    auto *minSet  = new QBarSet("最小值");
    auto *maxSet  = new QBarSet("最大值");
    auto *avgSet  = new QBarSet("平均值");

    minSet->setColor(QColor("#3498DB"));
    maxSet->setColor(QColor("#E74C3C"));
    avgSet->setColor(QColor("#2ECC71"));

    QStringList categories;
    for (const auto &s : stats) {
        *minSet << s.minVal;
        *maxSet << s.maxVal;
        *avgSet << s.avgVal;
        categories << s.parameter;
    }

    auto *series = new QBarSeries;
    series->append(minSet);
    series->append(avgSet);
    series->append(maxSet);

    m_statsChart->addSeries(series);

    auto *axisX = new QBarCategoryAxis;
    axisX->append(categories);
    m_statsChart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto *axisY = new QValueAxis;
    axisY->setTitleText("数值");
    m_statsChart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
}

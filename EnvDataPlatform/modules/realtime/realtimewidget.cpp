#include "realtimewidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QGroupBox>
#include <QDateTime>
#include <QScrollArea>

RealtimeWidget::RealtimeWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    setupCharts();

    // Default alarm thresholds
    setAlarmThresholds(35.0, 80.0, 75.0, 1000.0);
}

static QWidget* makeCard(const QString &name, const QString &unit,
                          const QString &color,
                          QLabel *&nameOut, QLabel *&valueOut,
                          QLabel *&unitOut, QLabel *&statusOut,
                          QProgressBar *&barOut)
{
    auto *card = new QWidget;
    card->setObjectName("cardWidget");
    card->setMinimumWidth(170);

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(6);

    // Color indicator stripe
    auto *stripe = new QFrame;
    stripe->setFixedHeight(4);
    stripe->setStyleSheet(QString("background-color: %1; border-radius: 2px;").arg(color));

    nameOut = new QLabel(name);
    nameOut->setObjectName("cardTitle");

    valueOut = new QLabel("--");
    valueOut->setObjectName("valueLabel");
    valueOut->setStyleSheet(QString("font-size: 32px; font-weight: bold; color: %1;").arg(color));

    unitOut = new QLabel(unit);
    unitOut->setObjectName("unitLabel");

    statusOut = new QLabel("正常");
    statusOut->setObjectName("statusNormal");

    barOut = new QProgressBar;
    barOut->setFixedHeight(8);
    barOut->setTextVisible(false);
    barOut->setRange(0, 100);

    auto *valueRow = new QHBoxLayout;
    valueRow->setSpacing(4);
    valueRow->addWidget(valueOut);
    valueRow->addWidget(unitOut, 0, Qt::AlignBottom);
    valueRow->addStretch();

    layout->addWidget(stripe);
    layout->addWidget(nameOut);
    layout->addLayout(valueRow);
    layout->addWidget(statusOut);
    layout->addWidget(barOut);

    return card;
}

void RealtimeWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(14);

    // --- Section header ---
    auto *headerLayout = new QHBoxLayout;
    auto *title = new QLabel("实时环境监测");
    title->setObjectName("sectionTitle");

    m_intervalBox = new QComboBox;
    m_intervalBox->addItem("刷新: 1秒",  1000);
    m_intervalBox->addItem("刷新: 2秒",  2000);
    m_intervalBox->addItem("刷新: 5秒",  5000);
    m_intervalBox->addItem("刷新: 10秒", 10000);
    m_intervalBox->setCurrentIndex(1);
    m_intervalBox->setFixedWidth(120);

    m_pauseBtn = new QPushButton("暂  停");
    m_pauseBtn->setFixedWidth(80);
    m_pauseBtn->setObjectName("secondaryButton");

    headerLayout->addWidget(title);
    headerLayout->addStretch();
    headerLayout->addWidget(new QLabel("数据刷新:"));
    headerLayout->addWidget(m_intervalBox);
    headerLayout->addWidget(m_pauseBtn);

    // --- 4 sensor cards ---
    auto *cardsLayout = new QHBoxLayout;
    cardsLayout->setSpacing(12);

    const QString names[] = {"温  度", "湿  度", "PM2.5", "CO₂"};
    const QString units[] = {"°C", "%", "µg/m³", "ppm"};
    const QString colors[]= {"#E74C3C", "#3498DB", "#9B59B6", "#E67E22"};

    for (int i = 0; i < 4; ++i) {
        auto *card = makeCard(names[i], units[i], colors[i],
                              m_cards[i].nameLabel,
                              m_cards[i].valueLabel,
                              m_cards[i].unitLabel,
                              m_cards[i].statusLabel,
                              m_cards[i].bar);
        cardsLayout->addWidget(card);
    }

    // --- Chart area ---
    auto *chartGroup = new QGroupBox("历史趋势图（最近60条数据）");
    auto *chartGroupLayout = new QVBoxLayout(chartGroup);
    chartGroupLayout->setContentsMargins(8, 8, 8, 8);

    // We'll use a single chart and show all series, use legend to toggle
    m_chartView = new QChartView;
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumHeight(280);

    chartGroupLayout->addWidget(m_chartView);

    // --- Timestamp label ---
    auto *bottomLayout = new QHBoxLayout;
    auto *tsLabel = new QLabel;
    tsLabel->setObjectName("tsLabel");
    tsLabel->setStyleSheet("color: #7F8C8D; font-size: 11px;");
    bottomLayout->addStretch();
    bottomLayout->addWidget(tsLabel);

    mainLayout->addLayout(headerLayout);
    mainLayout->addLayout(cardsLayout);
    mainLayout->addWidget(chartGroup, 1);
    mainLayout->addLayout(bottomLayout);

    // Connections
    connect(m_intervalBox, &QComboBox::currentIndexChanged, this, [this](){
        int ms = m_intervalBox->currentData().toInt();
        emit intervalChanged(ms);
    });
    connect(m_pauseBtn, &QPushButton::clicked, this, [this](){
        m_paused = !m_paused;
        m_pauseBtn->setText(m_paused ? "继  续" : "暂  停");
        if (m_paused)
            m_pauseBtn->setStyleSheet("QPushButton { background-color: #E67E22; color: white; border-radius: 5px; padding: 8px 18px; }");
        else
            m_pauseBtn->setObjectName("secondaryButton");
        emit intervalChanged(m_paused ? 0 : m_intervalBox->currentData().toInt());
    });
}

void RealtimeWidget::setupCharts()
{
    m_chart = new QChart;
    m_chart->setTitle("环境参数趋势");
    m_chart->legend()->setVisible(true);
    m_chart->legend()->setAlignment(Qt::AlignBottom);
    m_chart->setAnimationOptions(QChart::NoAnimation);  // No animation for real-time
    m_chart->setMargins(QMargins(4, 4, 4, 4));

    // Series
    m_tempSeries = new QLineSeries;  m_tempSeries->setName("温度(°C)");
    m_humiSeries = new QLineSeries;  m_humiSeries->setName("湿度(%)");
    m_pm25Series = new QLineSeries;  m_pm25Series->setName("PM2.5");
    m_co2Series  = new QLineSeries;  m_co2Series->setName("CO₂/10");  // Scale for display

    QPen tempPen(QColor("#E74C3C")); tempPen.setWidth(2);
    QPen humiPen(QColor("#3498DB")); humiPen.setWidth(2);
    QPen pm25Pen(QColor("#9B59B6")); pm25Pen.setWidth(2);
    QPen co2Pen (QColor("#E67E22")); co2Pen.setWidth(2);

    m_tempSeries->setPen(tempPen);
    m_humiSeries->setPen(humiPen);
    m_pm25Series->setPen(pm25Pen);
    m_co2Series->setPen(co2Pen);

    m_chart->addSeries(m_tempSeries);
    m_chart->addSeries(m_humiSeries);
    m_chart->addSeries(m_pm25Series);
    m_chart->addSeries(m_co2Series);

    // X axis: DateTime
    m_axisX = new QDateTimeAxis;
    m_axisX->setFormat("hh:mm:ss");
    m_axisX->setTitleText("时间");
    m_axisX->setTickCount(6);
    m_chart->addAxis(m_axisX, Qt::AlignBottom);

    // Y axis: value
    m_axisY = new QValueAxis;
    m_axisY->setRange(0, 150);
    m_axisY->setTitleText("数值");
    m_axisY->setTickCount(6);
    m_chart->addAxis(m_axisY, Qt::AlignLeft);

    m_tempSeries->attachAxis(m_axisX);
    m_tempSeries->attachAxis(m_axisY);
    m_humiSeries->attachAxis(m_axisX);
    m_humiSeries->attachAxis(m_axisY);
    m_pm25Series->attachAxis(m_axisX);
    m_pm25Series->attachAxis(m_axisY);
    m_co2Series->attachAxis(m_axisX);
    m_co2Series->attachAxis(m_axisY);

    m_chartView->setChart(m_chart);
}

void RealtimeWidget::setAlarmThresholds(double temp, double humi, double pm25, double co2)
{
    m_cards[0].alarmThreshold = temp;
    m_cards[1].alarmThreshold = humi;
    m_cards[2].alarmThreshold = pm25;
    m_cards[3].alarmThreshold = co2;
}

void RealtimeWidget::updateData(const EnvData &data)
{
    if (m_paused) return;

    m_history.append(data);
    if (m_history.size() > kMaxPoints)
        m_history.removeFirst();

    // Update value labels
    const double values[] = { data.temperature, data.humidity, data.pm25, data.co2 };
    const double maxVals[] = { 50.0, 100.0, 300.0, 2000.0 };
    const double warnVals[] = { 30.0, 65.0, 35.0, 800.0 };

    for (int i = 0; i < 4; ++i) {
        double v = values[i];
        m_cards[i].valueLabel->setText(QString::number(v, 'f', 1));

        // Bar progress
        int pct = qBound(0, static_cast<int>(v / maxVals[i] * 100), 100);
        m_cards[i].bar->setValue(pct);

        // Status text
        bool alarm = (v > m_cards[i].alarmThreshold);
        bool warn  = (v > warnVals[i]);
        if (alarm) {
            m_cards[i].statusLabel->setText("⚠ 超出阈值");
            m_cards[i].statusLabel->setObjectName("statusDanger");
            m_cards[i].bar->setObjectName("dangerBar");
        } else if (warn) {
            m_cards[i].statusLabel->setText("注意偏高");
            m_cards[i].statusLabel->setObjectName("statusWarning");
            m_cards[i].bar->setObjectName("warningBar");
        } else {
            m_cards[i].statusLabel->setText("正常");
            m_cards[i].statusLabel->setObjectName("statusNormal");
            m_cards[i].bar->setObjectName("");
        }
        // Force style refresh
        m_cards[i].statusLabel->style()->unpolish(m_cards[i].statusLabel);
        m_cards[i].statusLabel->style()->polish(m_cards[i].statusLabel);
        m_cards[i].bar->style()->unpolish(m_cards[i].bar);
        m_cards[i].bar->style()->polish(m_cards[i].bar);
    }

    // Update chart
    appendToSeries(data);

    // Update timestamp
    if (auto *ts = findChild<QLabel*>("tsLabel"))
        ts->setText("最后更新: " + data.recordedAt.toString("yyyy-MM-dd hh:mm:ss"));
}

void RealtimeWidget::appendToSeries(const EnvData &data)
{
    qint64 msec = data.recordedAt.toMSecsSinceEpoch();

    m_tempSeries->append(msec, data.temperature);
    m_humiSeries->append(msec, data.humidity);
    m_pm25Series->append(msec, data.pm25);
    m_co2Series->append(msec, data.co2 / 10.0);  // Scale CO2 to be comparable

    // Keep series at kMaxPoints
    auto trimSeries = [](QLineSeries *s, int max){
        while (s->count() > max) s->remove(0);
    };
    trimSeries(m_tempSeries, kMaxPoints);
    trimSeries(m_humiSeries, kMaxPoints);
    trimSeries(m_pm25Series, kMaxPoints);
    trimSeries(m_co2Series,  kMaxPoints);

    // Update axes
    if (!m_history.isEmpty()) {
        QDateTime tMin = m_history.first().recordedAt;
        QDateTime tMax = m_history.last().recordedAt;
        // Ensure at least 10s range for visibility
        if (tMin == tMax) tMax = tMin.addSecs(10);
        m_axisX->setRange(tMin, tMax);

        // Auto-scale Y across all visible values
        double yMin = 0, yMax = 150;
        for (const auto &d : m_history) {
            double localMax = d.temperature;
            localMax = qMax(localMax, d.humidity);
            localMax = qMax(localMax, d.pm25);
            localMax = qMax(localMax, d.co2 / 10.0);
            yMax = qMax(yMax, localMax);
        }
        m_axisY->setRange(yMin, yMax * 1.1);
    }
}

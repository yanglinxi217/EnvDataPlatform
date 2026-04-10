#include "alarmwidget.h"
#include "../../core/database/databasemanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QDateTime>
#include <QLabel>
#include <QApplication>

AlarmWidget::AlarmWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    onRefresh();
}

void AlarmWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(12);

    auto *title = new QLabel("异常报警管理");
    title->setObjectName("sectionTitle");

    // --- Thresholds display panel ---
    auto *threshGroup = new QGroupBox("当前报警阈值");
    auto *threshLayout = new QGridLayout(threshGroup);
    threshLayout->setSpacing(10);

    auto makeThreshWidget = [&](const QString &icon, const QString &name, QLabel *&valueLabel) {
        auto *w = new QWidget;
        w->setObjectName("cardWidget");
        auto *l = new QHBoxLayout(w);
        l->setContentsMargins(12, 8, 12, 8);
        auto *ic  = new QLabel(icon);
        ic->setStyleSheet("font-size: 20px;");
        auto *nm  = new QLabel(name);
        nm->setStyleSheet("font-weight: bold;");
        valueLabel = new QLabel("--");
        valueLabel->setStyleSheet("color: #E74C3C; font-weight: bold;");
        l->addWidget(ic);
        l->addWidget(nm);
        l->addStretch();
        l->addWidget(valueLabel);
        return w;
    };

    threshLayout->addWidget(makeThreshWidget("🌡", "温度上限",   m_tempThreshLabel), 0, 0);
    threshLayout->addWidget(makeThreshWidget("💧", "湿度上限",   m_humiThreshLabel), 0, 1);
    threshLayout->addWidget(makeThreshWidget("🌫", "PM2.5上限", m_pm25ThreshLabel), 0, 2);
    threshLayout->addWidget(makeThreshWidget("💨", "CO₂上限",   m_co2ThreshLabel),  0, 3);

    // Update threshold display
    auto updateLabels = [this](){
        m_tempThreshLabel->setText(QString::number(m_thresholds.temperature, 'f', 1) + " °C");
        m_humiThreshLabel->setText(QString::number(m_thresholds.humidity,    'f', 1) + " %");
        m_pm25ThreshLabel->setText(QString::number(m_thresholds.pm25,        'f', 1) + " µg/m³");
        m_co2ThreshLabel->setText(QString::number(m_thresholds.co2,          'f', 0) + " ppm");
    };
    updateLabels();
    // Will be called again when thresholds change
    connect(this, &AlarmWidget::thresholdsChanged, this, [updateLabels](const AlarmThresholds&){ updateLabels(); });

    // --- Filter row ---
    auto *filterGroup = new QGroupBox("报警记录查询");
    auto *filterLayout = new QHBoxLayout(filterGroup);
    filterLayout->setSpacing(10);

    filterLayout->addWidget(new QLabel("开始："));
    m_filterFrom = new QDateTimeEdit(QDateTime::currentDateTime().addDays(-7));
    m_filterFrom->setDisplayFormat("yyyy-MM-dd hh:mm");
    m_filterFrom->setCalendarPopup(true);
    m_filterFrom->setFixedHeight(34);

    filterLayout->addWidget(m_filterFrom);
    filterLayout->addWidget(new QLabel("结束："));

    m_filterTo = new QDateTimeEdit(QDateTime::currentDateTime());
    m_filterTo->setDisplayFormat("yyyy-MM-dd hh:mm");
    m_filterTo->setCalendarPopup(true);
    m_filterTo->setFixedHeight(34);
    filterLayout->addWidget(m_filterTo);

    auto *queryRangeBtn = new QPushButton("按时间查询");
    queryRangeBtn->setFixedHeight(34);
    filterLayout->addWidget(queryRangeBtn);
    filterLayout->addStretch();

    // --- Buttons ---
    auto *btnLayout = new QHBoxLayout;
    m_settingsBtn = new QPushButton("⚙ 设置阈值");
    m_settingsBtn->setFixedHeight(36);

    m_refreshBtn  = new QPushButton("刷  新");
    m_refreshBtn->setObjectName("secondaryButton");
    m_refreshBtn->setFixedHeight(36);

    m_clearBtn = new QPushButton("清空记录");
    m_clearBtn->setObjectName("dangerButton");
    m_clearBtn->setFixedHeight(36);

    m_totalLabel = new QLabel("共 0 条报警记录");
    m_totalLabel->setStyleSheet("color: #7F8C8D; font-size: 12px;");

    btnLayout->addWidget(m_settingsBtn);
    btnLayout->addWidget(m_refreshBtn);
    btnLayout->addWidget(m_clearBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(m_totalLabel);

    // --- Alarm table ---
    m_table = new QTableWidget;
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({"序号", "报警时间", "参数", "异常值", "阈值"});
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->hide();
    m_table->setColumnWidth(0, 60);
    m_table->setColumnWidth(2, 100);
    m_table->setColumnWidth(3, 120);
    m_table->setColumnWidth(4, 120);

    mainLayout->addWidget(title);
    mainLayout->addWidget(threshGroup);
    mainLayout->addWidget(filterGroup);
    mainLayout->addLayout(btnLayout);
    mainLayout->addWidget(m_table, 1);

    // Connections
    connect(m_settingsBtn,  &QPushButton::clicked, this, &AlarmWidget::onSetThresholds);
    connect(m_clearBtn,     &QPushButton::clicked, this, &AlarmWidget::onClearAlarms);
    connect(m_refreshBtn,   &QPushButton::clicked, this, &AlarmWidget::onRefresh);
    connect(queryRangeBtn,  &QPushButton::clicked, this, &AlarmWidget::onQueryByRange);
}

void AlarmWidget::checkData(const EnvData &data)
{
    struct Check {
        QString param;
        double  value;
        double  threshold;
    };
    QList<Check> checks = {
        {"temperature", data.temperature, m_thresholds.temperature},
        {"humidity",    data.humidity,    m_thresholds.humidity},
        {"pm25",        data.pm25,        m_thresholds.pm25},
        {"co2",         data.co2,         m_thresholds.co2},
    };

    for (const auto &c : checks) {
        if (c.value > c.threshold) {
            // Debounce: minimum 10 seconds between same-param alarms
            qint64 now = QDateTime::currentMSecsSinceEpoch();
            if (m_lastAlarmTime.contains(c.param) &&
                now - m_lastAlarmTime[c.param] < 10000) continue;
            m_lastAlarmTime[c.param] = now;

            AlarmRecord rec;
            rec.parameter = c.param;
            rec.value     = c.value;
            rec.threshold = c.threshold;
            rec.alarmTime = data.recordedAt;
            triggerAlarm(rec);
        }
    }
}

void AlarmWidget::triggerAlarm(const AlarmRecord &record)
{
    // Save to database
    DatabaseManager::instance().insertAlarmRecord(record);

    m_alarmCount++;
    emit newAlarmCount(m_alarmCount);

    // Add to table
    appendAlarmRow(record);

    // Show warning
    QString msg = QString("参数【%1】异常！\n当前值: %2 %3\n报警阈值: %4 %3")
        .arg(record.parameterDisplayName())
        .arg(record.value, 0, 'f', 1)
        .arg(record.unit())
        .arg(record.threshold, 0, 'f', 1);

    QMessageBox *box = new QMessageBox(QMessageBox::Warning, "⚠ 环境异常报警", msg,
                                       QMessageBox::Ok, this);
    box->setAttribute(Qt::WA_DeleteOnClose);
    box->setWindowModality(Qt::NonModal);
    box->show();
}

void AlarmWidget::appendAlarmRow(const AlarmRecord &r)
{
    int row = m_table->rowCount();
    m_table->insertRow(row);

    m_table->setItem(row, 0, new QTableWidgetItem(QString::number(row + 1)));
    m_table->setItem(row, 1, new QTableWidgetItem(r.alarmTime.toString("yyyy-MM-dd hh:mm:ss")));
    m_table->setItem(row, 2, new QTableWidgetItem(r.parameterDisplayName()));
    m_table->setItem(row, 3, new QTableWidgetItem(
        QString::number(r.value, 'f', 1) + " " + r.unit()));
    m_table->setItem(row, 4, new QTableWidgetItem(
        QString::number(r.threshold, 'f', 1) + " " + r.unit()));

    // Color the row red
    for (int c = 0; c < 5; ++c) {
        if (m_table->item(row, c)) {
            m_table->item(row, c)->setForeground(QColor("#E74C3C"));
            m_table->item(row, c)->setTextAlignment(Qt::AlignCenter);
        }
    }
    m_table->scrollToBottom();
    m_totalLabel->setText(QString("共 %1 条报警记录").arg(m_table->rowCount()));
}

void AlarmWidget::onSetThresholds()
{
    AlarmSettingDialog dlg(m_thresholds, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_thresholds = dlg.thresholds();
        emit thresholdsChanged(m_thresholds);
    }
}

void AlarmWidget::onClearAlarms()
{
    if (m_table->rowCount() == 0) return;
    auto ret = QMessageBox::question(this, "确认清空",
        "确定要清空所有报警记录吗？此操作不可恢复。");
    if (ret != QMessageBox::Yes) return;

    DatabaseManager::instance().clearAlarmRecords();
    m_table->setRowCount(0);
    m_alarmCount = 0;
    m_totalLabel->setText("共 0 条报警记录");
    emit newAlarmCount(0);
}

void AlarmWidget::onRefresh()
{
    auto records = DatabaseManager::instance().queryAllAlarmRecords();
    m_table->setRowCount(0);
    for (const auto &r : records)
        appendAlarmRow(r);
    m_alarmCount = records.size();
    emit newAlarmCount(m_alarmCount);
}

void AlarmWidget::onQueryByRange()
{
    QDateTime from = m_filterFrom->dateTime();
    QDateTime to   = m_filterTo->dateTime();
    if (from >= to) {
        QMessageBox::warning(this, "时间无效", "开始时间必须早于结束时间");
        return;
    }
    auto records = DatabaseManager::instance().queryAlarmRecords(from, to);
    m_table->setRowCount(0);
    for (const auto &r : records)
        appendAlarmRow(r);
    m_totalLabel->setText(QString("查询结果: %1 条").arg(records.size()));
}

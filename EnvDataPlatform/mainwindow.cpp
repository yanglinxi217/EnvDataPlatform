#include "mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QGroupBox>
#include <QApplication>
#include <QFile>
#include <QDateTime>
#include <QStatusBar>
#include <QSettings>
#include <QMessageBox>
#include "core/database/databasemanager.h"

MainWindow::MainWindow(const UserInfo &user, QWidget *parent)
    : QMainWindow(parent)
    , m_user(user)
    , m_exporter(new DataExporter(this))
{
    setWindowTitle("环境数据信息平台");
    setMinimumSize(1100, 700);
    resize(1280, 760);

    setupUi();

    // Load saved settings
    QSettings settings("EnvDataPlatform", "Settings");
    QString theme   = settings.value("theme", "light").toString();
    int intervalMs  = settings.value("refresh_interval", 2000).toInt();

    applyTheme(theme);

    // Start data generator
    connect(&m_generator, &MockDataGenerator::newData,
            this, &MainWindow::onNewData);
    m_generator.start(intervalMs);

    // Status bar clock
    connect(&m_statusTimer, &QTimer::timeout, this, [this](){
        m_statusTimeLabel->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd  hh:mm:ss"));
    });
    m_statusTimer.start(1000);
    m_statusTimeLabel->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd  hh:mm:ss"));
}

MainWindow::~MainWindow()
{
    m_generator.stop();
}

void MainWindow::setupUi()
{
    auto *central = new QWidget;
    setCentralWidget(central);

    auto *rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // === Left Navigation ===
    m_navWidget = new QWidget;
    m_navWidget->setObjectName("navWidget");
    m_navWidget->setFixedWidth(190);

    auto *navLayout = new QVBoxLayout(m_navWidget);
    navLayout->setContentsMargins(0, 0, 0, 0);
    navLayout->setSpacing(0);

    // App title area
    auto *appTitleLabel = new QLabel("环境数据\n信息平台");
    appTitleLabel->setObjectName("appTitle");
    appTitleLabel->setAlignment(Qt::AlignCenter);
    appTitleLabel->setFixedHeight(80);

    auto *userInfoLabel = new QLabel(QString("👤 %1  %2")
        .arg(m_user.username)
        .arg(m_user.isAdmin() ? "[管理员]" : "[普通用户]"));
    userInfoLabel->setObjectName("appSubTitle");
    userInfoLabel->setAlignment(Qt::AlignCenter);
    userInfoLabel->setWordWrap(true);

    navLayout->addWidget(appTitleLabel);
    navLayout->addWidget(userInfoLabel);

    // Separator
    auto *sep1 = new QFrame;
    sep1->setFrameShape(QFrame::HLine);
    sep1->setStyleSheet("background-color: #34495E; margin: 0;");
    sep1->setFixedHeight(1);
    navLayout->addWidget(sep1);
    navLayout->addSpacing(8);

    // Navigation buttons
    struct NavItem { QString icon; QString text; };
    QList<NavItem> items = {
        {"📊", "实时监测"},
        {"📋", "历史查询"},
        {"🔔", "报警管理"},
        {"📤", "数据导出"},
        {"⚙",  "系统设置"},
    };
    if (m_user.isAdmin())
        items.append(NavItem{"👥", "用户管理"});

    for (int i = 0; i < items.size(); ++i) {
        auto *btn = makeNavButton(items[i].icon, items[i].text, i);
        m_navButtons.append(btn);
        navLayout->addWidget(btn);
    }

    navLayout->addStretch();

    // Bottom logout hint
    auto *versionLabel = new QLabel("v1.0.0");
    versionLabel->setAlignment(Qt::AlignCenter);
    versionLabel->setStyleSheet("color: #4A5568; font-size: 11px; padding: 8px;");
    navLayout->addWidget(versionLabel);

    // === Right Content (QStackedWidget) ===
    m_realtimeWidget  = new RealtimeWidget;
    m_historyWidget   = new HistoryWidget;
    m_alarmWidget     = new AlarmWidget;
    m_settingsWidget  = new SettingsWidget;
    m_userMgrWidget   = new UserManagerWindow;

    // Export widget (a simple wrapper page)
    auto *exportPage = new QWidget;
    auto *exportLayout = new QVBoxLayout(exportPage);
    exportLayout->setContentsMargins(20, 20, 20, 20);
    exportLayout->setSpacing(14);

    auto *exportTitle = new QLabel("数据导出");
    exportTitle->setObjectName("sectionTitle");

    auto *exportDesc = new QLabel(
        "将环境数据导出为 CSV 格式文件。CSV 文件可用 Excel 或其他表格软件打开。");
    exportDesc->setWordWrap(true);
    exportDesc->setStyleSheet("color: #7F8C8D; font-size: 13px;");

    auto *exportGroup = new QGroupBox("导出选项");
    auto *exportGroupLayout = new QVBoxLayout(exportGroup);

    auto *exportAllBtn = new QPushButton("导出全部历史数据");
    exportAllBtn->setFixedHeight(44);
    exportAllBtn->setFixedWidth(200);

    auto *exportQueryBtn = new QPushButton("从历史查询模块导出");
    exportQueryBtn->setObjectName("secondaryButton");
    exportQueryBtn->setFixedHeight(44);
    exportQueryBtn->setFixedWidth(200);
    exportQueryBtn->setToolTip("请先在[历史查询]模块查询数据，然后点击该模块内的[导出CSV]按钮");

    auto *exportHint = new QLabel(
        "💡 提示：也可在「历史查询」模块中按条件查询后，点击「导出CSV」按钮导出指定范围的数据。");
    exportHint->setWordWrap(true);
    exportHint->setStyleSheet("color: #7F8C8D; font-size: 12px; padding: 8px; "
                              "background-color: rgba(26,188,156,0.1); border-radius: 4px;");

    exportGroupLayout->addWidget(exportAllBtn);
    exportGroupLayout->addWidget(exportQueryBtn);
    exportGroupLayout->addWidget(exportHint);
    exportGroupLayout->addStretch();

    exportLayout->addWidget(exportTitle);
    exportLayout->addWidget(exportDesc);
    exportLayout->addWidget(exportGroup);
    exportLayout->addStretch();

    connect(exportAllBtn,   &QPushButton::clicked, this, [this](){
        m_exporter->exportAllToCsv(this);
    });
    connect(exportQueryBtn, &QPushButton::clicked, this, [this](){
        onNavClicked(1);  // Go to history
        QMessageBox::information(this, "提示", "请在历史查询模块设置查询条件后，点击[导出CSV]按钮");
    });

    m_stack = new QStackedWidget;
    m_stack->addWidget(m_realtimeWidget);   // 0
    m_stack->addWidget(m_historyWidget);    // 1
    m_stack->addWidget(m_alarmWidget);      // 2
    m_stack->addWidget(exportPage);         // 3
    m_stack->addWidget(m_settingsWidget);   // 4
    if (m_user.isAdmin())
        m_stack->addWidget(m_userMgrWidget);// 5

    rootLayout->addWidget(m_navWidget);
    rootLayout->addWidget(m_stack, 1);

    // === Status Bar ===
    m_statusUserLabel = new QLabel(
        QString("  登录用户: %1  |  %2  ")
        .arg(m_user.username)
        .arg(m_user.isAdmin() ? "管理员" : "普通用户"));
    m_statusUserLabel->setStyleSheet("color: #BDC3C7;");

    m_statusTimeLabel = new QLabel;
    m_statusTimeLabel->setStyleSheet("color: #BDC3C7;  ");

    statusBar()->addWidget(m_statusUserLabel);
    statusBar()->addPermanentWidget(m_statusTimeLabel);

    // Activate first nav button
    if (!m_navButtons.isEmpty())
        m_navButtons[0]->setChecked(true);

    // Connections
    connect(m_realtimeWidget, &RealtimeWidget::intervalChanged,
            this, &MainWindow::onIntervalChanged);

    connect(m_historyWidget, &HistoryWidget::exportRequested,
            this, [this](const QList<EnvData> &data){
        m_exporter->exportToCsv(data, this);
    });

    connect(m_alarmWidget, &AlarmWidget::thresholdsChanged,
            this, [this](const AlarmThresholds &t){
        m_realtimeWidget->setAlarmThresholds(t.temperature, t.humidity, t.pm25, t.co2);
        // Save to QSettings
        QSettings s("EnvDataPlatform", "Settings");
        s.setValue("alarm/temperature", t.temperature);
        s.setValue("alarm/humidity",    t.humidity);
        s.setValue("alarm/pm25",        t.pm25);
        s.setValue("alarm/co2",         t.co2);
    });

    connect(m_alarmWidget, &AlarmWidget::newAlarmCount,
            this, [this](int count){
        if (count > 0)
            m_navButtons.size() > 2 ?
                m_navButtons[2]->setText(QString("  🔔 报警管理  (%1)").arg(count)) :
                void();
    });

    connect(m_settingsWidget, &SettingsWidget::intervalChanged,
            this, &MainWindow::onIntervalChanged);

    connect(m_settingsWidget, &SettingsWidget::themeChanged,
            this, &MainWindow::applyTheme);

    // Load saved alarm thresholds
    QSettings s("EnvDataPlatform", "Settings");
    AlarmThresholds savedThresh;
    savedThresh.temperature = s.value("alarm/temperature", 35.0).toDouble();
    savedThresh.humidity    = s.value("alarm/humidity",    80.0).toDouble();
    savedThresh.pm25        = s.value("alarm/pm25",        75.0).toDouble();
    savedThresh.co2         = s.value("alarm/co2",       1000.0).toDouble();
    m_realtimeWidget->setAlarmThresholds(
        savedThresh.temperature, savedThresh.humidity,
        savedThresh.pm25, savedThresh.co2);
}

QPushButton* MainWindow::makeNavButton(const QString &icon, const QString &text, int index)
{
    auto *btn = new QPushButton(QString("  %1  %2").arg(icon, text));
    btn->setObjectName("navButton");
    btn->setCheckable(true);
    btn->setFixedHeight(48);
    btn->setAutoExclusive(true);
    btn->setFlat(true);
    btn->setCursor(Qt::PointingHandCursor);

    connect(btn, &QPushButton::clicked, this, [this, index](){
        onNavClicked(index);
    });
    return btn;
}

void MainWindow::onNavClicked(int index)
{
    m_currentNav = index;
    m_stack->setCurrentIndex(index);

    if (index < m_navButtons.size())
        m_navButtons[index]->setChecked(true);

    // Refresh user manager when visiting it
    if (m_user.isAdmin() && index == 5)
        m_userMgrWidget->refresh();
}

void MainWindow::onNewData(const EnvData &data)
{
    // Save to database
    DatabaseManager::instance().insertEnvData(data);

    // Update realtime display
    m_realtimeWidget->updateData(data);

    // Check alarm thresholds
    m_alarmWidget->checkData(data);
}

void MainWindow::onIntervalChanged(int ms)
{
    if (ms <= 0)
        m_generator.stop();
    else {
        m_generator.setInterval(ms);
        if (!m_generator.property("active").toBool())
            m_generator.start(ms);
    }
}

void MainWindow::applyTheme(const QString &theme)
{
    QString qssFile = (theme == "dark") ? ":/styles/dark.qss" : ":/styles/light.qss";
    QFile f(qssFile);
    if (f.open(QFile::ReadOnly)) {
        qApp->setStyleSheet(QString::fromUtf8(f.readAll()));
        f.close();
    }
}

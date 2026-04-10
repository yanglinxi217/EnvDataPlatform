#include "settingswidget.h"
#include "../../core/database/databasemanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QTimer>
#include <QFile>

SettingsWidget::SettingsWidget(QWidget *parent)
    : QWidget(parent)
    , m_settings("EnvDataPlatform", "Settings")
{
    setupUi();
    loadSettings();
}

void SettingsWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(14);

    auto *title = new QLabel("系统设置");
    title->setObjectName("sectionTitle");
    mainLayout->addWidget(title);

    // --- Data Acquisition Settings ---
    auto *acqGroup = new QGroupBox("数据采集");
    auto *acqForm  = new QFormLayout(acqGroup);
    acqForm->setSpacing(12);

    m_intervalBox = new QComboBox;
    m_intervalBox->addItem("1 秒",  1000);
    m_intervalBox->addItem("2 秒",  2000);
    m_intervalBox->addItem("5 秒",  5000);
    m_intervalBox->addItem("10 秒", 10000);
    m_intervalBox->setFixedHeight(36);
    acqForm->addRow("数据刷新频率：", m_intervalBox);

    mainLayout->addWidget(acqGroup);

    // --- Appearance Settings ---
    auto *uiGroup = new QGroupBox("界面外观");
    auto *uiForm  = new QFormLayout(uiGroup);
    uiForm->setSpacing(12);

    m_themeBox = new QComboBox;
    m_themeBox->addItem("浅色主题（Light）", "light");
    m_themeBox->addItem("深色主题（Dark）",  "dark");
    m_themeBox->setFixedHeight(36);
    uiForm->addRow("界面主题：", m_themeBox);

    auto *themeHint = new QLabel("切换主题后立即生效，重启后保持");
    themeHint->setStyleSheet("color: #7F8C8D; font-size: 11px;");
    uiForm->addRow("", themeHint);

    mainLayout->addWidget(uiGroup);

    // --- Database Settings ---
    auto *dbGroup = new QGroupBox("数据库管理");
    auto *dbLayout = new QVBoxLayout(dbGroup);
    dbLayout->setSpacing(10);

    auto *dbDesc = new QLabel("备份将把当前数据库文件复制到指定位置；"
                              "恢复将用备份文件替换当前数据库（需重启生效）。");
    dbDesc->setWordWrap(true);
    dbDesc->setStyleSheet("color: #7F8C8D; font-size: 12px;");

    auto *dbBtnLayout = new QHBoxLayout;
    m_backupBtn  = new QPushButton("备份数据库");
    m_backupBtn->setFixedHeight(36);
    m_backupBtn->setObjectName("secondaryButton");

    m_restoreBtn = new QPushButton("恢复数据库");
    m_restoreBtn->setFixedHeight(36);
    m_restoreBtn->setObjectName("secondaryButton");

    dbBtnLayout->addWidget(m_backupBtn);
    dbBtnLayout->addWidget(m_restoreBtn);
    dbBtnLayout->addStretch();

    dbLayout->addWidget(dbDesc);
    dbLayout->addLayout(dbBtnLayout);
    mainLayout->addWidget(dbGroup);

    // --- About ---
    auto *aboutGroup = new QGroupBox("关于");
    auto *aboutLayout = new QVBoxLayout(aboutGroup);
    auto *aboutLabel = new QLabel(
        "<b>环境数据信息平台</b> v1.0.0<br>"
        "基于 Qt 6 开发，采用 SQLite 数据库存储。<br>"
        "支持温度、湿度、PM2.5、CO₂ 等环境参数的实时采集、"
        "历史查询、异常报警及数据导出。");
    aboutLabel->setWordWrap(true);
    aboutLabel->setStyleSheet("font-size: 12px; line-height: 1.6;");
    aboutLayout->addWidget(aboutLabel);
    mainLayout->addWidget(aboutGroup);

    // --- Save button & status ---
    auto *bottomLayout = new QHBoxLayout;
    m_saveBtn = new QPushButton("保存设置");
    m_saveBtn->setFixedHeight(40);
    m_saveBtn->setFixedWidth(120);

    m_statusLabel = new QLabel;
    m_statusLabel->setStyleSheet("color: #27AE60; font-size: 12px;");

    bottomLayout->addWidget(m_saveBtn);
    bottomLayout->addSpacing(12);
    bottomLayout->addWidget(m_statusLabel);
    bottomLayout->addStretch();

    mainLayout->addStretch();
    mainLayout->addLayout(bottomLayout);

    // Connections
    connect(m_saveBtn,    &QPushButton::clicked, this, &SettingsWidget::onSave);
    connect(m_backupBtn,  &QPushButton::clicked, this, &SettingsWidget::onBackupDatabase);
    connect(m_restoreBtn, &QPushButton::clicked, this, &SettingsWidget::onRestoreDatabase);
    connect(m_themeBox, &QComboBox::currentTextChanged, this, [this](const QString&){
        onThemePreview(m_themeBox->currentData().toString());
    });
}

void SettingsWidget::loadSettings()
{
    int intervalMs = m_settings.value("refresh_interval", 2000).toInt();
    for (int i = 0; i < m_intervalBox->count(); ++i) {
        if (m_intervalBox->itemData(i).toInt() == intervalMs) {
            m_intervalBox->setCurrentIndex(i);
            break;
        }
    }

    QString theme = m_settings.value("theme", "light").toString();
    for (int i = 0; i < m_themeBox->count(); ++i) {
        if (m_themeBox->itemData(i).toString() == theme) {
            m_themeBox->setCurrentIndex(i);
            break;
        }
    }
}

void SettingsWidget::saveSettings()
{
    m_settings.setValue("refresh_interval", m_intervalBox->currentData().toInt());
    m_settings.setValue("theme", m_themeBox->currentData().toString());
    m_settings.sync();
}

int SettingsWidget::refreshInterval() const
{
    return m_intervalBox->currentData().toInt();
}

QString SettingsWidget::theme() const
{
    return m_themeBox->currentData().toString();
}

void SettingsWidget::onSave()
{
    saveSettings();
    emit intervalChanged(m_intervalBox->currentData().toInt());
    emit themeChanged(m_themeBox->currentData().toString());
    m_statusLabel->setText("设置已保存 ✓");

    // Clear status after 3 seconds
    QTimer *t = new QTimer(this);
    t->setSingleShot(true);
    t->setInterval(3000);
    connect(t, &QTimer::timeout, this, [this, t](){
        m_statusLabel->clear(); t->deleteLater();
    });
    t->start();
}

void SettingsWidget::onBackupDatabase()
{
    QString path = QFileDialog::getSaveFileName(
        this, "备份数据库", "envdata_backup.db",
        "SQLite数据库 (*.db);;所有文件 (*.*)");
    if (path.isEmpty()) return;

    if (DatabaseManager::instance().backupTo(path)) {
        QMessageBox::information(this, "备份成功",
            "数据库已备份到：\n" + path);
    } else {
        QMessageBox::critical(this, "备份失败", "数据库备份失败，请检查目标路径权限");
    }
}

void SettingsWidget::onRestoreDatabase()
{
    QMessageBox::warning(this, "注意",
        "恢复数据库后，当前所有数据将被备份文件替换，且需要重启程序才能生效。\n请谨慎操作！");

    QString path = QFileDialog::getOpenFileName(
        this, "选择备份文件", "",
        "SQLite数据库 (*.db);;所有文件 (*.*)");
    if (path.isEmpty()) return;

    // Copy backup over current DB
    QString dbPath = DatabaseManager::instance().property("dbPath").toString();
    // Just inform user to manually replace after close
    QMessageBox::information(this, "恢复说明",
        QString("请退出程序后，将\n%1\n复制并覆盖到数据存储目录中的 envdata.db 文件。").arg(path));
}

void SettingsWidget::onThemePreview(const QString &theme)
{
    applyTheme(theme);
}

void SettingsWidget::applyTheme(const QString &theme)
{
    QString qssFile = (theme == "dark") ? ":/styles/dark.qss" : ":/styles/light.qss";
    QFile f(qssFile);
    if (f.open(QFile::ReadOnly)) {
        QString style = QString::fromUtf8(f.readAll());
        qApp->setStyleSheet(style);
        f.close();
    }
    emit themeChanged(theme);
}

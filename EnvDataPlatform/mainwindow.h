#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QLabel>
#include <QTimer>
#include "core/models/userinfo.h"
#include "core/mockdatagenerator.h"
#include "modules/realtime/realtimewidget.h"
#include "modules/history/historywidget.h"
#include "modules/alarm/alarmwidget.h"
#include "modules/settings/settingswidget.h"
#include "modules/login/usermanagerwindow.h"
#include "modules/export/dataexporter.h"

class QPushButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(const UserInfo &user, QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onNewData(const EnvData &data);
    void onNavClicked(int index);
    void onIntervalChanged(int ms);

private:
    void setupUi();
    void setupNavigation();
    void applyTheme(const QString &theme);
    QPushButton* makeNavButton(const QString &icon, const QString &text, int index);

    UserInfo         m_user;
    QStackedWidget  *m_stack;
    QWidget         *m_navWidget;

    RealtimeWidget   *m_realtimeWidget;
    HistoryWidget    *m_historyWidget;
    AlarmWidget      *m_alarmWidget;
    SettingsWidget   *m_settingsWidget;
    UserManagerWindow*m_userMgrWidget;
    DataExporter     *m_exporter;

    MockDataGenerator m_generator;

    QLabel   *m_statusUserLabel;
    QLabel   *m_statusTimeLabel;
    QTimer    m_statusTimer;

    QList<QPushButton*> m_navButtons;
    int m_currentNav = 0;
};

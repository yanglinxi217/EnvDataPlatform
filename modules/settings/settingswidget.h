#pragma once

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSettings>

class SettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsWidget(QWidget *parent = nullptr);

    void loadSettings();
    void saveSettings();

    int     refreshInterval() const;
    QString theme() const;

signals:
    void intervalChanged(int ms);
    void themeChanged(const QString &theme);

private slots:
    void onSave();
    void onBackupDatabase();
    void onRestoreDatabase();
    void onThemePreview(const QString &theme);

private:
    void setupUi();
    void applyTheme(const QString &theme);

    QComboBox   *m_intervalBox;
    QComboBox   *m_themeBox;
    QPushButton *m_saveBtn;
    QPushButton *m_backupBtn;
    QPushButton *m_restoreBtn;
    QLabel      *m_statusLabel;

    QSettings    m_settings;
};

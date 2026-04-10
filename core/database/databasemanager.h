#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QList>
#include <QDateTime>
#include "../models/envdata.h"
#include "../models/userinfo.h"
#include "../models/alarmrecord.h"

class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    static DatabaseManager& instance();

    bool        initialize(const QString &dbPath = QString());
    bool        isOpen() const;

    // ---- User management ----
    bool        addUser(const QString &username, const QString &password, UserRole role = UserRole::Normal);
    bool        updateUserRole(int userId, UserRole role);
    bool        deleteUser(int userId);
    bool        validateUser(const QString &username, const QString &password, UserInfo &outUser);
    bool        userExists(const QString &username);
    QList<UserInfo> getAllUsers();

    // ---- Environment data ----
    bool        insertEnvData(const EnvData &data);
    QList<EnvData> queryEnvData(const QDateTime &from, const QDateTime &to);
    QList<EnvData> queryEnvDataLatest(int count = 60);

    struct StatResult {
        QString parameter;
        double  minVal  = 0;
        double  maxVal  = 0;
        double  avgVal  = 0;
    };
    QList<StatResult> queryStats(const QDateTime &from, const QDateTime &to);

    // ---- Alarm records ----
    bool        insertAlarmRecord(const AlarmRecord &record);
    QList<AlarmRecord> queryAlarmRecords(const QDateTime &from, const QDateTime &to);
    QList<AlarmRecord> queryAllAlarmRecords();
    bool        clearAlarmRecords();

    // ---- Database backup ----
    bool        backupTo(const QString &destPath);

    // ---- Static helper ----
    static QString hashPassword(const QString &plainText);

private:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();

    bool createTables();
    bool seedDefaultUsers();

    QSqlDatabase m_db;
    static DatabaseManager *s_instance;
};

#include "databasemanager.h"

#include <QCryptographicHash>
#include <QSqlRecord>
#include <QFile>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>

DatabaseManager *DatabaseManager::s_instance = nullptr;

DatabaseManager &DatabaseManager::instance()
{
    if (!s_instance)
        s_instance = new DatabaseManager();
    return *s_instance;
}

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
{}

DatabaseManager::~DatabaseManager()
{
    if (m_db.isOpen())
        m_db.close();
}

bool DatabaseManager::initialize(const QString &dbPath)
{
    QString path = dbPath;
    if (path.isEmpty()) {
        QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dataDir);
        path = dataDir + "/envdata.db";
    }

    m_db = QSqlDatabase::addDatabase("QSQLITE", "main_connection");
    m_db.setDatabaseName(path);

    if (!m_db.open()) {
        qCritical() << "Cannot open database:" << m_db.lastError().text();
        return false;
    }

    // Enable WAL mode for better performance
    QSqlQuery q(m_db);
    q.exec("PRAGMA journal_mode=WAL");
    q.exec("PRAGMA foreign_keys=ON");

    if (!createTables()) return false;
    seedDefaultUsers();
    return true;
}

bool DatabaseManager::isOpen() const
{
    return m_db.isOpen();
}

bool DatabaseManager::createTables()
{
    QSqlQuery q(m_db);

    // Users table
    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS users (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            username    TEXT    UNIQUE NOT NULL,
            password    TEXT    NOT NULL,
            role        INTEGER DEFAULT 0,
            created_at  DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )")) {
        qCritical() << "Create users table failed:" << q.lastError().text();
        return false;
    }

    // Environment data table
    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS env_data (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            temperature REAL,
            humidity    REAL,
            pm25        REAL,
            co2         REAL,
            recorded_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )")) {
        qCritical() << "Create env_data table failed:" << q.lastError().text();
        return false;
    }

    // Index on recorded_at for fast range queries
    q.exec("CREATE INDEX IF NOT EXISTS idx_env_time ON env_data(recorded_at)");

    // Alarm records table
    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS alarm_records (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            parameter   TEXT,
            value       REAL,
            threshold   REAL,
            alarm_time  DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )")) {
        qCritical() << "Create alarm_records table failed:" << q.lastError().text();
        return false;
    }

    q.exec("CREATE INDEX IF NOT EXISTS idx_alarm_time ON alarm_records(alarm_time)");

    return true;
}

bool DatabaseManager::seedDefaultUsers()
{
    if (!userExists("admin")) {
        addUser("admin", "admin123", UserRole::Admin);
    }
    if (!userExists("user")) {
        addUser("user", "user123", UserRole::Normal);
    }
    return true;
}

// ---------- User management ----------

QString DatabaseManager::hashPassword(const QString &plainText)
{
    return QString(QCryptographicHash::hash(
        plainText.toUtf8(), QCryptographicHash::Md5).toHex());
}

bool DatabaseManager::addUser(const QString &username, const QString &password, UserRole role)
{
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO users (username, password, role) VALUES (:u, :p, :r)");
    q.bindValue(":u", username);
    q.bindValue(":p", hashPassword(password));
    q.bindValue(":r", static_cast<int>(role));
    if (!q.exec()) {
        qWarning() << "addUser failed:" << q.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::updateUserRole(int userId, UserRole role)
{
    QSqlQuery q(m_db);
    q.prepare("UPDATE users SET role = :r WHERE id = :id");
    q.bindValue(":r", static_cast<int>(role));
    q.bindValue(":id", userId);
    return q.exec();
}

bool DatabaseManager::deleteUser(int userId)
{
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM users WHERE id = :id");
    q.bindValue(":id", userId);
    return q.exec();
}

bool DatabaseManager::validateUser(const QString &username, const QString &password, UserInfo &outUser)
{
    QSqlQuery q(m_db);
    q.prepare("SELECT id, username, password, role, created_at FROM users WHERE username = :u");
    q.bindValue(":u", username);
    if (!q.exec() || !q.next()) return false;

    QString storedHash = q.value("password").toString();
    if (storedHash != hashPassword(password)) return false;

    outUser.id       = q.value("id").toInt();
    outUser.username = q.value("username").toString();
    outUser.password = storedHash;
    outUser.role     = static_cast<UserRole>(q.value("role").toInt());
    outUser.createdAt= q.value("created_at").toDateTime();
    return true;
}

bool DatabaseManager::userExists(const QString &username)
{
    QSqlQuery q(m_db);
    q.prepare("SELECT COUNT(*) FROM users WHERE username = :u");
    q.bindValue(":u", username);
    if (q.exec() && q.next())
        return q.value(0).toInt() > 0;
    return false;
}

QList<UserInfo> DatabaseManager::getAllUsers()
{
    QList<UserInfo> list;
    QSqlQuery q(m_db);
    q.exec("SELECT id, username, password, role, created_at FROM users");
    while (q.next()) {
        UserInfo u;
        u.id        = q.value("id").toInt();
        u.username  = q.value("username").toString();
        u.password  = q.value("password").toString();
        u.role      = static_cast<UserRole>(q.value("role").toInt());
        u.createdAt = q.value("created_at").toDateTime();
        list.append(u);
    }
    return list;
}

// ---------- Environment data ----------

bool DatabaseManager::insertEnvData(const EnvData &data)
{
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO env_data (temperature, humidity, pm25, co2, recorded_at) "
              "VALUES (:t, :h, :p, :c, :r)");
    q.bindValue(":t", data.temperature);
    q.bindValue(":h", data.humidity);
    q.bindValue(":p", data.pm25);
    q.bindValue(":c", data.co2);
    q.bindValue(":r", data.recordedAt.toString(Qt::ISODate));
    if (!q.exec()) {
        qWarning() << "insertEnvData failed:" << q.lastError().text();
        return false;
    }
    return true;
}

QList<EnvData> DatabaseManager::queryEnvData(const QDateTime &from, const QDateTime &to)
{
    QList<EnvData> list;
    QSqlQuery q(m_db);
    q.prepare("SELECT id, temperature, humidity, pm25, co2, recorded_at "
              "FROM env_data WHERE recorded_at BETWEEN :f AND :t ORDER BY recorded_at ASC");
    q.bindValue(":f", from.toString(Qt::ISODate));
    q.bindValue(":t", to.toString(Qt::ISODate));
    if (!q.exec()) {
        qWarning() << "queryEnvData failed:" << q.lastError().text();
        return list;
    }
    while (q.next()) {
        EnvData d;
        d.id          = q.value("id").toInt();
        d.temperature = q.value("temperature").toDouble();
        d.humidity    = q.value("humidity").toDouble();
        d.pm25        = q.value("pm25").toDouble();
        d.co2         = q.value("co2").toDouble();
        d.recordedAt  = QDateTime::fromString(q.value("recorded_at").toString(), Qt::ISODate);
        list.append(d);
    }
    return list;
}

QList<EnvData> DatabaseManager::queryEnvDataLatest(int count)
{
    QList<EnvData> list;
    QSqlQuery q(m_db);
    q.prepare("SELECT id, temperature, humidity, pm25, co2, recorded_at "
              "FROM env_data ORDER BY recorded_at DESC LIMIT :n");
    q.bindValue(":n", count);
    if (!q.exec()) return list;

    // Collect in reverse order so oldest is first
    while (q.next()) {
        EnvData d;
        d.id          = q.value("id").toInt();
        d.temperature = q.value("temperature").toDouble();
        d.humidity    = q.value("humidity").toDouble();
        d.pm25        = q.value("pm25").toDouble();
        d.co2         = q.value("co2").toDouble();
        d.recordedAt  = QDateTime::fromString(q.value("recorded_at").toString(), Qt::ISODate);
        list.prepend(d);   // Maintain chronological order
    }
    return list;
}

QList<DatabaseManager::StatResult> DatabaseManager::queryStats(const QDateTime &from, const QDateTime &to)
{
    QList<StatResult> results;
    QString fromStr = from.toString(Qt::ISODate);
    QString toStr   = to.toString(Qt::ISODate);

    QStringList params = {"temperature", "humidity", "pm25", "co2"};
    QStringList labels = {"温度", "湿度", "PM2.5", "CO₂"};

    for (int i = 0; i < params.size(); ++i) {
        QString col = params[i];
        QSqlQuery q(m_db);
        q.prepare(QString("SELECT MIN(%1), MAX(%1), AVG(%1) FROM env_data "
                          "WHERE recorded_at BETWEEN :f AND :t").arg(col));
        q.bindValue(":f", fromStr);
        q.bindValue(":t", toStr);
        if (q.exec() && q.next()) {
            StatResult r;
            r.parameter = labels[i];
            r.minVal    = q.value(0).toDouble();
            r.maxVal    = q.value(1).toDouble();
            r.avgVal    = q.value(2).toDouble();
            results.append(r);
        }
    }
    return results;
}

// ---------- Alarm records ----------

bool DatabaseManager::insertAlarmRecord(const AlarmRecord &record)
{
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO alarm_records (parameter, value, threshold, alarm_time) "
              "VALUES (:p, :v, :th, :t)");
    q.bindValue(":p",  record.parameter);
    q.bindValue(":v",  record.value);
    q.bindValue(":th", record.threshold);
    q.bindValue(":t",  record.alarmTime.toString(Qt::ISODate));
    return q.exec();
}

QList<AlarmRecord> DatabaseManager::queryAlarmRecords(const QDateTime &from, const QDateTime &to)
{
    QList<AlarmRecord> list;
    QSqlQuery q(m_db);
    q.prepare("SELECT id, parameter, value, threshold, alarm_time FROM alarm_records "
              "WHERE alarm_time BETWEEN :f AND :t ORDER BY alarm_time DESC");
    q.bindValue(":f", from.toString(Qt::ISODate));
    q.bindValue(":t", to.toString(Qt::ISODate));
    if (!q.exec()) return list;
    while (q.next()) {
        AlarmRecord r;
        r.id        = q.value("id").toInt();
        r.parameter = q.value("parameter").toString();
        r.value     = q.value("value").toDouble();
        r.threshold = q.value("threshold").toDouble();
        r.alarmTime = QDateTime::fromString(q.value("alarm_time").toString(), Qt::ISODate);
        list.append(r);
    }
    return list;
}

QList<AlarmRecord> DatabaseManager::queryAllAlarmRecords()
{
    return queryAlarmRecords(
        QDateTime(QDate(2000, 1, 1), QTime(0, 0, 0)),
        QDateTime::currentDateTime());
}

bool DatabaseManager::clearAlarmRecords()
{
    QSqlQuery q(m_db);
    return q.exec("DELETE FROM alarm_records");
}

bool DatabaseManager::backupTo(const QString &destPath)
{
    if (!m_db.isOpen()) return false;
    QString srcPath = m_db.databaseName();
    if (QFile::exists(destPath)) QFile::remove(destPath);
    return QFile::copy(srcPath, destPath);
}

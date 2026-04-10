#pragma once
#include <QString>
#include <QDateTime>

enum class UserRole {
    Normal = 0,
    Admin  = 1
};

struct UserInfo {
    int      id       = 0;
    QString  username;
    QString  password;   // MD5 hex
    UserRole role     = UserRole::Normal;
    QDateTime createdAt;

    bool isAdmin() const { return role == UserRole::Admin; }
};

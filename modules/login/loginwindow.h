#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "../../core/models/userinfo.h"

class LoginWindow : public QDialog
{
    Q_OBJECT
public:
    explicit LoginWindow(QWidget *parent = nullptr);

    UserInfo loggedInUser() const { return m_user; }

private slots:
    void onLogin();
    void onShowRegister();

private:
    void setupUi();

    QLineEdit   *m_userEdit;
    QLineEdit   *m_passEdit;
    QPushButton *m_loginBtn;
    QPushButton *m_registerBtn;
    QLabel      *m_errorLabel;

    UserInfo m_user;
};

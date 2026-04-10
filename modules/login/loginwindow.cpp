#include "loginwindow.h"
#include "registerwindow.h"
#include "../../core/database/databasemanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFrame>
#include <QMessageBox>

LoginWindow::LoginWindow(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("环境数据信息平台 - 登录");
    setFixedSize(420, 520);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setupUi();
}

void LoginWindow::setupUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ---- Header banner ----
    auto *header = new QWidget;
    header->setFixedHeight(160);
    header->setStyleSheet("background-color: #1ABC9C;");
    auto *headerLayout = new QVBoxLayout(header);
    headerLayout->setAlignment(Qt::AlignCenter);

    auto *iconLabel = new QLabel("🌿");
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet("font-size: 48px; background: transparent;");

    auto *titleLabel = new QLabel("环境数据信息平台");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: white; background: transparent;");

    auto *subtitleLabel = new QLabel("Environmental Data Platform");
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setStyleSheet("font-size: 12px; color: rgba(255,255,255,0.8); background: transparent;");

    headerLayout->addWidget(iconLabel);
    headerLayout->addWidget(titleLabel);
    headerLayout->addWidget(subtitleLabel);

    // ---- Form area ----
    auto *formWidget = new QWidget;
    auto *formLayout = new QVBoxLayout(formWidget);
    formLayout->setContentsMargins(40, 30, 40, 30);
    formLayout->setSpacing(14);

    auto *loginTitle = new QLabel("用户登录");
    loginTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #2C3E50;");

    // Username
    auto *userLabel = new QLabel("用户名");
    userLabel->setStyleSheet("font-size: 12px; color: #7F8C8D; margin-bottom: 2px;");
    m_userEdit = new QLineEdit;
    m_userEdit->setPlaceholderText("请输入用户名");
    m_userEdit->setFixedHeight(40);

    // Password
    auto *passLabel = new QLabel("密码");
    passLabel->setStyleSheet("font-size: 12px; color: #7F8C8D; margin-bottom: 2px;");
    m_passEdit = new QLineEdit;
    m_passEdit->setEchoMode(QLineEdit::Password);
    m_passEdit->setPlaceholderText("请输入密码");
    m_passEdit->setFixedHeight(40);

    // Error label
    m_errorLabel = new QLabel;
    m_errorLabel->setStyleSheet("color: #E74C3C; font-size: 12px;");
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->hide();

    // Login button
    m_loginBtn = new QPushButton("登  录");
    m_loginBtn->setFixedHeight(42);
    m_loginBtn->setStyleSheet(
        "QPushButton { background-color: #1ABC9C; color: white; border-radius: 5px; font-size: 15px; font-weight: bold; }"
        "QPushButton:hover { background-color: #16A085; }"
        "QPushButton:pressed { background-color: #148F77; }");

    // Register link
    auto *regLayout = new QHBoxLayout;
    auto *regHintLabel = new QLabel("还没有账号？");
    regHintLabel->setStyleSheet("color: #7F8C8D; font-size: 12px;");
    m_registerBtn = new QPushButton("立即注册");
    m_registerBtn->setFlat(true);
    m_registerBtn->setStyleSheet(
        "QPushButton { color: #1ABC9C; font-size: 12px; background: transparent; border: none; }"
        "QPushButton:hover { color: #16A085; }");
    regLayout->addStretch();
    regLayout->addWidget(regHintLabel);
    regLayout->addWidget(m_registerBtn);
    regLayout->addStretch();

    // Hint
    auto *hintLabel = new QLabel("默认账号: admin/admin123 或 user/user123");
    hintLabel->setAlignment(Qt::AlignCenter);
    hintLabel->setStyleSheet("color: #BDC3C7; font-size: 11px;");

    formLayout->addWidget(loginTitle);
    formLayout->addSpacing(4);
    formLayout->addWidget(userLabel);
    formLayout->addWidget(m_userEdit);
    formLayout->addWidget(passLabel);
    formLayout->addWidget(m_passEdit);
    formLayout->addWidget(m_errorLabel);
    formLayout->addSpacing(6);
    formLayout->addWidget(m_loginBtn);
    formLayout->addLayout(regLayout);
    formLayout->addStretch();
    formLayout->addWidget(hintLabel);

    rootLayout->addWidget(header);
    rootLayout->addWidget(formWidget, 1);

    // Connections
    connect(m_loginBtn,    &QPushButton::clicked, this, &LoginWindow::onLogin);
    connect(m_registerBtn, &QPushButton::clicked, this, &LoginWindow::onShowRegister);
    connect(m_passEdit, &QLineEdit::returnPressed, this, &LoginWindow::onLogin);
    connect(m_userEdit, &QLineEdit::returnPressed, m_passEdit, static_cast<void(QLineEdit::*)()>(&QLineEdit::setFocus));
}

void LoginWindow::onLogin()
{
    QString username = m_userEdit->text().trimmed();
    QString password = m_passEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        m_errorLabel->setText("用户名和密码不能为空");
        m_errorLabel->show();
        return;
    }

    UserInfo user;
    if (DatabaseManager::instance().validateUser(username, password, user)) {
        m_user = user;
        m_errorLabel->hide();
        accept();
    } else {
        m_errorLabel->setText("用户名或密码错误，请重试");
        m_errorLabel->show();
        m_passEdit->clear();
        m_passEdit->setFocus();
    }
}

void LoginWindow::onShowRegister()
{
    RegisterWindow dlg(this);
    dlg.exec();
}

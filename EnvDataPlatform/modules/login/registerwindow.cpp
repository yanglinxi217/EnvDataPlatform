#include "registerwindow.h"
#include "../../core/database/databasemanager.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QMessageBox>

RegisterWindow::RegisterWindow(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("注册新用户");
    setFixedSize(360, 400);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setupUi();
}

void RegisterWindow::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(36, 30, 36, 30);
    layout->setSpacing(12);

    auto *title = new QLabel("创建账号");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2C3E50;");

    auto *sub = new QLabel("注册后使用普通用户权限登录平台");
    sub->setStyleSheet("color: #7F8C8D; font-size: 12px; margin-bottom: 8px;");

    auto *userLabel = new QLabel("用户名");
    userLabel->setStyleSheet("font-size: 12px; color: #7F8C8D;");
    m_userEdit = new QLineEdit;
    m_userEdit->setPlaceholderText("3~20个字符");
    m_userEdit->setFixedHeight(38);
    m_userEdit->setMaxLength(20);

    auto *passLabel = new QLabel("密码");
    passLabel->setStyleSheet("font-size: 12px; color: #7F8C8D;");
    m_passEdit = new QLineEdit;
    m_passEdit->setEchoMode(QLineEdit::Password);
    m_passEdit->setPlaceholderText("至少6个字符");
    m_passEdit->setFixedHeight(38);

    auto *confirmLabel = new QLabel("确认密码");
    confirmLabel->setStyleSheet("font-size: 12px; color: #7F8C8D;");
    m_confirmEdit = new QLineEdit;
    m_confirmEdit->setEchoMode(QLineEdit::Password);
    m_confirmEdit->setPlaceholderText("再次输入密码");
    m_confirmEdit->setFixedHeight(38);

    m_errorLabel = new QLabel;
    m_errorLabel->setStyleSheet("color: #E74C3C; font-size: 12px;");
    m_errorLabel->setWordWrap(true);
    m_errorLabel->hide();

    m_registerBtn = new QPushButton("注 册");
    m_registerBtn->setFixedHeight(40);
    m_registerBtn->setStyleSheet(
        "QPushButton { background-color: #1ABC9C; color: white; border-radius: 5px; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background-color: #16A085; }");

    m_cancelBtn = new QPushButton("取 消");
    m_cancelBtn->setFixedHeight(40);
    m_cancelBtn->setObjectName("secondaryButton");

    layout->addWidget(title);
    layout->addWidget(sub);
    layout->addWidget(userLabel);
    layout->addWidget(m_userEdit);
    layout->addWidget(passLabel);
    layout->addWidget(m_passEdit);
    layout->addWidget(confirmLabel);
    layout->addWidget(m_confirmEdit);
    layout->addWidget(m_errorLabel);
    layout->addStretch();
    layout->addWidget(m_registerBtn);
    layout->addWidget(m_cancelBtn);

    connect(m_registerBtn, &QPushButton::clicked, this, &RegisterWindow::onRegister);
    connect(m_cancelBtn,   &QPushButton::clicked, this, &QDialog::reject);
}

void RegisterWindow::onRegister()
{
    QString username = m_userEdit->text().trimmed();
    QString password = m_passEdit->text();
    QString confirm  = m_confirmEdit->text();

    if (username.length() < 3) {
        m_errorLabel->setText("用户名至少需要3个字符");
        m_errorLabel->show(); return;
    }
    if (password.length() < 6) {
        m_errorLabel->setText("密码至少需要6个字符");
        m_errorLabel->show(); return;
    }
    if (password != confirm) {
        m_errorLabel->setText("两次输入的密码不一致");
        m_errorLabel->show(); return;
    }

    auto &db = DatabaseManager::instance();
    if (db.userExists(username)) {
        m_errorLabel->setText("该用户名已被注册");
        m_errorLabel->show(); return;
    }

    if (db.addUser(username, password, UserRole::Normal)) {
        QMessageBox::information(this, "注册成功",
            QString("账号 [%1] 注册成功！\n请返回登录页面登录。").arg(username));
        accept();
    } else {
        m_errorLabel->setText("注册失败，请稍后重试");
        m_errorLabel->show();
    }
}

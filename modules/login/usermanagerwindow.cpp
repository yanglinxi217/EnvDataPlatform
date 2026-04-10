#include "usermanagerwindow.h"
#include "../../core/database/databasemanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QInputDialog>
#include <QMessageBox>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QDialogButtonBox>

UserManagerWindow::UserManagerWindow(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    refresh();
}

void UserManagerWindow::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);

    auto *title = new QLabel("用户管理");
    title->setObjectName("sectionTitle");

    auto *desc = new QLabel("管理平台用户账号与权限（仅管理员可见）");
    desc->setStyleSheet("color: #7F8C8D; font-size: 12px;");

    // Table
    m_table = new QTableWidget;
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({"ID", "用户名", "角色", "创建时间", "操作"});
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->hide();
    m_table->setColumnWidth(0, 50);
    m_table->setColumnWidth(2, 100);
    m_table->setColumnWidth(4, 80);

    // Buttons
    auto *btnLayout = new QHBoxLayout;
    m_addBtn = new QPushButton("+ 添加用户");
    m_addBtn->setFixedHeight(36);

    m_deleteBtn = new QPushButton("删除用户");
    m_deleteBtn->setObjectName("dangerButton");
    m_deleteBtn->setFixedHeight(36);

    m_roleBtn = new QPushButton("修改权限");
    m_roleBtn->setObjectName("secondaryButton");
    m_roleBtn->setFixedHeight(36);

    auto *refreshBtn = new QPushButton("刷新");
    refreshBtn->setObjectName("secondaryButton");
    refreshBtn->setFixedHeight(36);

    btnLayout->addWidget(m_addBtn);
    btnLayout->addWidget(m_roleBtn);
    btnLayout->addWidget(m_deleteBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(refreshBtn);

    layout->addWidget(title);
    layout->addWidget(desc);
    layout->addLayout(btnLayout);
    layout->addWidget(m_table);

    connect(m_addBtn,    &QPushButton::clicked, this, &UserManagerWindow::onAddUser);
    connect(m_deleteBtn, &QPushButton::clicked, this, &UserManagerWindow::onDeleteUser);
    connect(m_roleBtn,   &QPushButton::clicked, this, &UserManagerWindow::onChangeRole);
    connect(refreshBtn,  &QPushButton::clicked, this, &UserManagerWindow::refresh);
}

void UserManagerWindow::refresh()
{
    auto users = DatabaseManager::instance().getAllUsers();
    m_table->setRowCount(users.size());

    for (int i = 0; i < users.size(); ++i) {
        const auto &u = users[i];
        m_table->setItem(i, 0, new QTableWidgetItem(QString::number(u.id)));
        m_table->setItem(i, 1, new QTableWidgetItem(u.username));

        auto *roleItem = new QTableWidgetItem(u.isAdmin() ? "管理员" : "普通用户");
        roleItem->setForeground(u.isAdmin() ? QColor("#E67E22") : QColor("#27AE60"));
        m_table->setItem(i, 2, roleItem);
        m_table->setItem(i, 3, new QTableWidgetItem(
            u.createdAt.isValid() ? u.createdAt.toString("yyyy-MM-dd hh:mm") : "-"));

        // Store user ID in hidden column data
        m_table->item(i, 0)->setData(Qt::UserRole, u.id);
    }
}

void UserManagerWindow::onAddUser()
{
    QDialog dlg(this);
    dlg.setWindowTitle("添加用户");
    dlg.setFixedSize(320, 240);

    auto *form = new QFormLayout(&dlg);
    form->setContentsMargins(20, 20, 20, 20);
    form->setSpacing(12);

    auto *userEdit = new QLineEdit;
    userEdit->setFixedHeight(36);
    auto *passEdit = new QLineEdit;
    passEdit->setEchoMode(QLineEdit::Password);
    passEdit->setFixedHeight(36);
    auto *roleBox = new QComboBox;
    roleBox->addItem("普通用户", 0);
    roleBox->addItem("管理员",   1);
    roleBox->setFixedHeight(36);

    form->addRow("用户名：", userEdit);
    form->addRow("密  码：", passEdit);
    form->addRow("角  色：", roleBox);

    auto *bbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    form->addRow(bbox);

    connect(bbox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bbox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return;

    QString username = userEdit->text().trimmed();
    QString password = passEdit->text();
    auto    role     = static_cast<UserRole>(roleBox->currentData().toInt());

    if (username.length() < 3 || password.length() < 6) {
        QMessageBox::warning(this, "输入无效", "用户名至少3个字符，密码至少6个字符");
        return;
    }
    if (DatabaseManager::instance().userExists(username)) {
        QMessageBox::warning(this, "用户名冲突", "该用户名已存在");
        return;
    }
    if (DatabaseManager::instance().addUser(username, password, role)) {
        QMessageBox::information(this, "成功", "用户添加成功");
        refresh();
    } else {
        QMessageBox::critical(this, "错误", "添加用户失败");
    }
}

void UserManagerWindow::onDeleteUser()
{
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "提示", "请先选择要删除的用户");
        return;
    }
    QString username = m_table->item(row, 1)->text();
    if (username == "admin") {
        QMessageBox::warning(this, "禁止操作", "不能删除内置管理员账号");
        return;
    }
    int userId = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    auto ret = QMessageBox::question(this, "确认删除",
        QString("确定要删除用户 [%1] 吗？此操作不可恢复。").arg(username));
    if (ret != QMessageBox::Yes) return;

    if (DatabaseManager::instance().deleteUser(userId)) {
        QMessageBox::information(this, "成功", "用户已删除");
        refresh();
    } else {
        QMessageBox::critical(this, "错误", "删除用户失败");
    }
}

void UserManagerWindow::onChangeRole()
{
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "提示", "请先选择要修改权限的用户");
        return;
    }
    QString username = m_table->item(row, 1)->text();
    if (username == "admin") {
        QMessageBox::warning(this, "禁止操作", "不能修改内置管理员权限");
        return;
    }
    int userId = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    QString currentRole = m_table->item(row, 2)->text();

    QStringList roles = {"普通用户", "管理员"};
    bool ok;
    QString chosen = QInputDialog::getItem(this, "修改权限",
        QString("修改用户 [%1] 的角色：").arg(username),
        roles, currentRole == "管理员" ? 1 : 0, false, &ok);
    if (!ok) return;

    UserRole newRole = (chosen == "管理员") ? UserRole::Admin : UserRole::Normal;
    if (DatabaseManager::instance().updateUserRole(userId, newRole)) {
        QMessageBox::information(this, "成功", "用户权限已更新");
        refresh();
    } else {
        QMessageBox::critical(this, "错误", "更新权限失败");
    }
}

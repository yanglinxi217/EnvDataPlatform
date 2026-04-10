#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QComboBox>
#include "../../core/models/userinfo.h"

class UserManagerWindow : public QWidget
{
    Q_OBJECT
public:
    explicit UserManagerWindow(QWidget *parent = nullptr);

    void refresh();

private slots:
    void onAddUser();
    void onDeleteUser();
    void onChangeRole();

private:
    void setupUi();

    QTableWidget *m_table;
    QPushButton  *m_addBtn;
    QPushButton  *m_deleteBtn;
    QPushButton  *m_roleBtn;
};

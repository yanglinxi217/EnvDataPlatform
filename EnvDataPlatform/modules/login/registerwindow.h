#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class RegisterWindow : public QDialog
{
    Q_OBJECT
public:
    explicit RegisterWindow(QWidget *parent = nullptr);

private slots:
    void onRegister();

private:
    void setupUi();

    QLineEdit   *m_userEdit;
    QLineEdit   *m_passEdit;
    QLineEdit   *m_confirmEdit;
    QPushButton *m_registerBtn;
    QPushButton *m_cancelBtn;
    QLabel      *m_errorLabel;
};

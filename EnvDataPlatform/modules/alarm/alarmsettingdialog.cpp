#include "alarmsettingdialog.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>

AlarmSettingDialog::AlarmSettingDialog(const AlarmThresholds &current, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("报警阈值设置");
    setFixedSize(360, 340);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setupUi(current);
}

void AlarmSettingDialog::setupUi(const AlarmThresholds &current)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(12);

    auto *title = new QLabel("设置各参数报警阈值");
    title->setStyleSheet("font-size: 15px; font-weight: bold;");

    auto *hint = new QLabel("当实时数据超过以下阈值时将触发报警提示");
    hint->setStyleSheet("color: #7F8C8D; font-size: 12px;");
    hint->setWordWrap(true);

    auto *group = new QGroupBox("阈值设定");
    auto *form  = new QFormLayout(group);
    form->setSpacing(12);

    m_tempSpin = new QDoubleSpinBox;
    m_tempSpin->setRange(0, 60);
    m_tempSpin->setValue(current.temperature);
    m_tempSpin->setSuffix("  °C");
    m_tempSpin->setDecimals(1);
    m_tempSpin->setFixedHeight(36);

    m_humiSpin = new QDoubleSpinBox;
    m_humiSpin->setRange(0, 100);
    m_humiSpin->setValue(current.humidity);
    m_humiSpin->setSuffix("  %");
    m_humiSpin->setDecimals(1);
    m_humiSpin->setFixedHeight(36);

    m_pm25Spin = new QDoubleSpinBox;
    m_pm25Spin->setRange(0, 500);
    m_pm25Spin->setValue(current.pm25);
    m_pm25Spin->setSuffix("  µg/m³");
    m_pm25Spin->setDecimals(1);
    m_pm25Spin->setFixedHeight(36);

    m_co2Spin = new QDoubleSpinBox;
    m_co2Spin->setRange(300, 5000);
    m_co2Spin->setValue(current.co2);
    m_co2Spin->setSuffix("  ppm");
    m_co2Spin->setDecimals(0);
    m_co2Spin->setFixedHeight(36);

    form->addRow("温度上限：",  m_tempSpin);
    form->addRow("湿度上限：",  m_humiSpin);
    form->addRow("PM2.5 上限：", m_pm25Spin);
    form->addRow("CO₂ 上限：",  m_co2Spin);

    auto *bbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    bbox->button(QDialogButtonBox::Ok)->setText("保  存");
    bbox->button(QDialogButtonBox::Cancel)->setText("取  消");

    layout->addWidget(title);
    layout->addWidget(hint);
    layout->addWidget(group);
    layout->addStretch();
    layout->addWidget(bbox);

    connect(bbox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bbox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

AlarmThresholds AlarmSettingDialog::thresholds() const
{
    return {
        m_tempSpin->value(),
        m_humiSpin->value(),
        m_pm25Spin->value(),
        m_co2Spin->value()
    };
}

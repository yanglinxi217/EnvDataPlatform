# 环境数据信息平台 (EnvDataPlatform)

基于 Qt 6 开发的桌面端环境数据监测与管理平台，支持实时数据展示、历史数据查询、报警管理、数据导出及系统配置等功能。

---

## 技术栈

| 项目 | 版本/说明 |
|------|-----------|
| Qt | 6.10.2 (MinGW 64-bit) |
| 构建工具 | qmake + mingw32-make |
| 数据库 | SQLite（Qt 内置驱动 `qsqlite`） |
| 图表 | Qt Charts（内置，无第三方依赖） |
| 数据来源 | 本地 Mock 模拟（QTimer + 随机漂移，无真实传感器） |
| 导出格式 | CSV（UTF-8 BOM，兼容 Excel 直接打开） |
| 主题 | QSS 文件切换（亮色 / 暗色） |
| 密码存储 | MD5 哈希（QCryptographicHash） |

---

## 构建方法

```bash
# 1. 生成 Makefile
"C:\Qt\6.10.2\mingw_64\bin\qmake.exe" EnvDataPlatform.pro -spec win32-g++

# 2. 编译
"C:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe" -j4

# 3. 部署 Qt 运行时 DLL（首次部署后无需重复）
"C:\Qt\6.10.2\mingw_64\bin\windeployqt.exe" release\EnvDataPlatform.exe --release
```

生成物：`release\EnvDataPlatform.exe`

---

## 项目结构

```
EnvDataPlatform/
├── main.cpp                          # 程序入口
├── mainwindow.h / mainwindow.cpp     # 主窗口（导航栏 + QStackedWidget）
├── EnvDataPlatform.pro               # qmake 项目文件
├── resources/
│   ├── resources.qrc
│   └── styles/
│       ├── light.qss                 # 亮色主题
│       └── dark.qss                  # 暗色主题
├── core/
│   ├── models/
│   │   ├── envdata.h                 # 环境数据结构体
│   │   ├── userinfo.h                # 用户信息结构体 + 角色枚举
│   │   └── alarmrecord.h             # 报警记录结构体
│   ├── database/
│   │   ├── databasemanager.h/.cpp    # SQLite 数据库单例管理器
│   └── mockdatagenerator.h/.cpp      # Mock 传感器数据生成器
└── modules/
    ├── login/
    │   ├── loginwindow.h/.cpp        # 登录窗口
    │   ├── registerwindow.h/.cpp     # 注册对话框
    │   └── usermanagerwindow.h/.cpp  # 用户管理（仅管理员）
    ├── realtime/
    │   └── realtimewidget.h/.cpp     # 实时数据展示
    ├── history/
    │   └── historywidget.h/.cpp      # 历史数据查询
    ├── alarm/
    │   ├── alarmsettingdialog.h/.cpp # 阈值设置对话框
    │   └── alarmwidget.h/.cpp        # 报警管理
    ├── export/
    │   └── dataexporter.h/.cpp       # CSV 数据导出
    └── settings/
        └── settingswidget.h/.cpp     # 系统设置
```

---

## 默认账户

| 用户名 | 密码 | 角色 |
|--------|------|------|
| admin | admin123 | 管理员 |
| user | user123 | 普通用户 |

密码以 MD5 哈希形式存储于 SQLite 数据库，首次运行时由 `DatabaseManager::seedDefaultUsers()` 自动写入。

---

## 数据模型

### EnvData（`core/models/envdata.h`）

```cpp
struct EnvData {
    int       id;
    double    temperature;  // 温度，单位 °C
    double    humidity;     // 湿度，单位 %
    double    pm25;         // PM2.5，单位 µg/m³
    double    co2;          // CO₂ 浓度，单位 ppm
    QDateTime recordedAt;   // 记录时间戳
};
```

### UserInfo（`core/models/userinfo.h`）

```cpp
enum class UserRole { Normal = 0, Admin = 1 };

struct UserInfo {
    int       id;
    QString   username;
    QString   password;   // MD5 十六进制字符串
    UserRole  role;
    QDateTime createdAt;
};
```

### AlarmRecord（`core/models/alarmrecord.h`）

```cpp
struct AlarmRecord {
    int       id;
    QString   parameter;   // "temperature" | "humidity" | "pm25" | "co2"
    double    value;       // 触发时的实测值
    double    threshold;   // 触发时的阈值
    QDateTime alarmTime;
};
```

---

## 模块实现细节

### 模块一：登录 / 注册 / 用户管理

**相关文件：** `modules/login/`

#### LoginWindow

- 继承 `QDialog`，`exec()` 返回 `Accepted` 表示登录成功。
- 调用 `DatabaseManager::validateUser()` 进行身份验证，密码在传入前先做 MD5 哈希后与数据库比对。
- 登录失败时通过 `m_errorLabel` 显示红色错误文本，不弹出额外对话框。
- 成功后将 `UserInfo` 对象保存为成员，供 `main.cpp` 通过 `loggedInUser()` 取得。
- 底部提供"注册账号"按钮，点击后弹出 `RegisterWindow`。

#### RegisterWindow

- 继承 `QDialog`，提供用户名、密码、确认密码三个输入框。
- 注册前校验：用户名非空、密码长度 >= 6 位、两次密码一致、用户名未被占用（`DatabaseManager::userExists()`）。
- 新用户角色固定为 `UserRole::Normal`，密码写入时经 `DatabaseManager::hashPassword()` MD5 转换。

#### UserManagerWindow

- 继承 `QWidget`，仅对 `isAdmin()` 为真的用户在主窗口导航栏可见。
- 使用 `QTableWidget` 展示所有用户（ID、用户名、角色、创建时间）。
- 支持三项操作：
  - **添加用户**：弹出输入对话框，填写用户名与初始密码。
  - **删除用户**：不允许删除当前登录用户自身及最后一个管理员。
  - **切换角色**：在 Normal / Admin 之间切换，调用 `DatabaseManager::updateUserRole()`。

---

### 模块二：实时数据展示

**相关文件：** `modules/realtime/realtimewidget.h/.cpp`，`core/mockdatagenerator.h/.cpp`

#### MockDataGenerator

- 使用 `QTimer` 以可配置间隔（默认 2000ms）周期性触发 `generateData()`，发出 `newData(EnvData)` 信号。
- 每个参数维护一个当前值，通过 `driftValue()` 实现随机漂移：
  - **正常漂移**：在 `[-step, +step]` 范围内随机游走，并以 2% 系数向中心值偏移，防止数值漂出边界。
  - **异常尖峰**（spike）：以约 4% 的概率生成超出正常范围 15%~45% 的极值，用于触发报警测试。
- 各参数正常漂移范围：

  | 参数 | 正常范围 | 单步最大变化 | Spike 概率 |
  |------|----------|------------|------------|
  | 温度 | 18–38 °C | ±0.5 | 4% |
  | 湿度 | 35–75 % | ±1.0 | 3% |
  | PM2.5 | 5–80 µg/m³ | ±3.0 | 4% |
  | CO₂ | 400–1200 ppm | ±20 | 4% |

#### RealtimeWidget

- 页面分三层：**标题栏**（刷新频率选择器 + 暂停/继续按钮）、**四块传感器卡片**、**折线图区域**。
- 每块卡片（`SensorCard` 结构体）包含：参数名、彩色数值大字体、单位、状态标签（正常 / 注意偏高 / ⚠ 超出阈值）、进度条。
  - 进度条和状态标签颜色通过 `setObjectName()` 设置 CSS 类名，结合 QSS 样式文件动态着色，切换后调用 `style()->unpolish/polish()` 强制刷新。
- 折线图使用 `QChart` + `QLineSeries`，四条数据线共享一个 `QDateTimeAxis`（X 轴）和 `QValueAxis`（Y 轴）：
  - X 轴自动跟随历史数据时间范围。
  - Y 轴自动缩放至当前最大值的 1.1 倍；CO₂ 数值除以 10 后入图，避免量纲差距导致其他曲线被压扁。
  - 最多保留最近 `kMaxPoints = 60` 个点，超出时从头部移除。
- **暂停/继续**：`m_paused` 标志位控制 `updateData()` 是否处理新数据，同时向主窗口发出 `intervalChanged(0)` 停止定时器，恢复时恢复原间隔。
- 阈值由主窗口通过 `setAlarmThresholds()` 注入，与报警模块阈值联动。

---

### 模块三：历史数据查询

**相关文件：** `modules/history/historywidget.h/.cpp`

- 顶部提供起止时间选择器（`QDateTimeEdit`）、参数过滤下拉框（全部 / 温度 / 湿度 / PM2.5 / CO₂）、快捷按钮（近 1 天 / 近 7 天 / 近 30 天）。
- 点击查询后调用 `DatabaseManager::queryEnvData(from, to)`，将结果填入 `QTableWidget`，列为：序号、时间、温度、湿度、PM2.5、CO₂。
- 结果区上方显示命中条数统计标签。
- 底部展示统计柱状图（`QBarSeries`）：调用 `DatabaseManager::queryStats()` 取各参数的最小值、最大值、平均值，以分组柱状图形式呈现，X 轴为参数名，Y 轴为数值。
- 提供"导出当前结果"按钮，通过 `exportRequested(data)` 信号通知主窗口调用 `DataExporter`。

---

### 模块四：报警管理

**相关文件：** `modules/alarm/alarmwidget.h/.cpp`，`modules/alarm/alarmsettingdialog.h/.cpp`

#### AlarmSettingDialog

- 继承 `QDialog`，为四个参数各提供一个 `QDoubleSpinBox` 设置上限阈值。
- 默认阈值：温度 35 °C、湿度 80%、PM2.5 75 µg/m³、CO₂ 1000 ppm。
- 确认后通过 `thresholds()` 返回 `AlarmThresholds` 结构体，由 `AlarmWidget` 保存并广播。

#### AlarmWidget

- **阈值展示区**：四个只读标签实时显示当前各参数阈值，右侧"设置阈值"按钮打开 `AlarmSettingDialog`，确认后触发 `thresholdsChanged` 信号，通知主窗口同步更新实时页面的卡片着色。
- **报警检测**：每次收到新数据（`checkData(EnvData)`）时逐参数与阈值比对，超限则触发报警。
  - **防抖机制**：`m_lastAlarmTime` 记录各参数上次报警的 Unix 时间戳（毫秒），同一参数两次报警间隔不足 10 秒则跳过，防止持续超限导致报警记录爆炸。
  - 触发后：插入 `AlarmRecord` 到数据库、追加到表格首行、弹出 `QMessageBox` 警告对话框、发出 `newAlarmCount(int)` 信号更新导航栏徽标计数。
- **记录表格**：`QTableWidget` 展示历史报警记录，列为：参数、实测值、阈值、报警时间。
- 提供按时间范围过滤查询和一键清空功能（`DatabaseManager::clearAlarmRecords()`）。

---

### 模块五：数据导出

**相关文件：** `modules/export/dataexporter.h/.cpp`

- `DataExporter` 为轻量 `QObject`，提供两个公有方法：
  - `exportToCsv(data, parentWidget)`：导出传入的 `QList<EnvData>`。
  - `exportAllToCsv(parentWidget)`：从数据库查全量数据后调用前者。
- 文件路径通过 `QFileDialog::getSaveFileName()` 由用户选择，默认文件名格式为 `env_data_yyyyMMdd_HHmmss.csv`。
- 写入逻辑：
  - 首先写入 UTF-8 BOM（`0xEF 0xBB 0xBF`），使 Excel 直接打开不乱码。
  - 写入中文表头：`序号,记录时间,温度(°C),湿度(%),PM2.5(µg/m³),CO₂(ppm)`。
  - 每行数据均保留一位小数（`'f', 1`）。
- 导出成功或失败均通过 `QMessageBox` 提示用户。

---

### 模块六：系统设置

**相关文件：** `modules/settings/settingswidget.h/.cpp`

- 使用 `QSettings`（`INI` 格式）持久化存储配置，应用名 `EnvDataPlatform`，组名 `Settings`，配置项：
  - `refreshInterval`：数据刷新间隔（ms），可选 1000 / 2000 / 5000 / 10000。
  - `theme`：主题名称（`"light"` / `"dark"`）。
- **主题预览**：切换下拉框时实时调用 `applyTheme()` 加载对应 QSS 文件，无需点击保存即可预览效果。
- **保存设置**：点击保存按钮写入 `QSettings`，并发出 `intervalChanged` 和 `themeChanged` 信号，主窗口监听后同步调整 `MockDataGenerator` 的定时间隔。
- **数据库备份**：调用 `DatabaseManager::backupTo(destPath)`，内部以二进制方式完整拷贝 `.db` 文件，目标路径由 `QFileDialog` 指定。
- **数据库恢复**：选择一个备份文件后关闭当前连接，覆盖当前数据库文件，重新初始化连接。

---

## 主窗口架构

**相关文件：** `mainwindow.h/.cpp`，`main.cpp`

- `main.cpp` 启动流程：
  1. 初始化 SQLite 数据库（`DatabaseManager::instance().initialize()`）。
  2. 从 `QSettings` 读取已保存主题，加载 QSS 应用于 `QApplication`。
  3. 显示 `LoginWindow`，取消则直接退出。
  4. 登录成功后以 `UserInfo` 构造 `MainWindow` 并 `show()`。
- `MainWindow` 布局：左侧固定宽度导航侧边栏 + 右侧 `QStackedWidget`。
- 导航栏每个按钮对应一个页面索引，点击后切换 `currentIndex` 并更新按钮选中样式。
- 管理员账户额外显示"用户管理"导航项；普通用户不可见。
- 状态栏左侧显示当前登录用户名与角色，右侧通过 `QTimer`（1 秒刷新）显示系统时间。
- `MockDataGenerator` 在主窗口中实例化，`newData` 信号同时连接至：
  - `RealtimeWidget::updateData()` — 更新卡片与图表。
  - `AlarmWidget::checkData()` — 触发报警检测。
  - `DatabaseManager::insertEnvData()` — 持久化到 SQLite。
- 报警阈值变更时，`AlarmWidget::thresholdsChanged` 信号触发 `RealtimeWidget::setAlarmThresholds()`，保持两个模块阈值同步。

---

## 运行截图（模块对应页面）

| 页面 | 说明 |
|------|------|
| 登录页 | 用户名/密码输入，错误提示内联展示 |
| 实时监测 | 4 块传感器卡片 + 60 点滚动折线图 |
| 历史查询 | 时间范围筛选表格 + 分组统计柱状图 |
| 报警管理 | 阈值展示/设置 + 报警记录表 + 防抖过滤 |
| 数据导出 | 一键导出全量或当前查询结果为 CSV |
| 系统设置 | 刷新率、主题实时预览、数据库备份/恢复 |
| 用户管理 | 仅管理员可见，支持增删用户及角色切换 |

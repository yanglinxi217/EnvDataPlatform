QT += core gui widgets sql charts multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

TARGET = EnvDataPlatform
TEMPLATE = app

# Make project root available as include path so all files can use
# relative-to-root includes like "core/models/envdata.h"
INCLUDEPATH += .

# Source files
SOURCES += \
    main.cpp \
    mainwindow.cpp \
    core/database/databasemanager.cpp \
    core/mockdatagenerator.cpp \
    modules/login/loginwindow.cpp \
    modules/login/registerwindow.cpp \
    modules/login/usermanagerwindow.cpp \
    modules/realtime/realtimewidget.cpp \
    modules/history/historywidget.cpp \
    modules/alarm/alarmwidget.cpp \
    modules/alarm/alarmsettingdialog.cpp \
    modules/export/dataexporter.cpp \
    modules/settings/settingswidget.cpp

HEADERS += \
    mainwindow.h \
    core/database/databasemanager.h \
    core/models/envdata.h \
    core/models/userinfo.h \
    core/models/alarmrecord.h \
    core/mockdatagenerator.h \
    modules/login/loginwindow.h \
    modules/login/registerwindow.h \
    modules/login/usermanagerwindow.h \
    modules/realtime/realtimewidget.h \
    modules/history/historywidget.h \
    modules/alarm/alarmwidget.h \
    modules/alarm/alarmsettingdialog.h \
    modules/export/dataexporter.h \
    modules/settings/settingswidget.h

RESOURCES += \
    resources/resources.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

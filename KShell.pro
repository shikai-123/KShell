QT       += core gui widgets network sql xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# Qt6 需要 core5compat 模块来支持 QTextCodec 等旧 API
greaterThan(QT_MAJOR_VERSION, 5): QT += core5compat

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

TARGET = KShell
TEMPLATE = app

# 源文件路径
SOURCES += \
    KShell/main.cpp \
    KShell/SSHWindow.cpp \
    KShell/SetingWindow.cpp \
    KShell/Tray.cpp \
    KShell/Ftp.cpp \
    KShell/FTPTreeList.cpp \
    KShell/FTPTreeWork.cpp \
    KShell/HelpWindows.cpp \
    KShell/HomePage.cpp \
    KShell/Log.cpp \
    KShell/NewUser.cpp \
    KShell/SshClient.cpp \
    KShell/UPDate.cpp

# 头文件路径
HEADERS += \
    KShell/DataStruck.h \
    KShell/SSHWindow.h \
    KShell/SetingWindow.h \
    KShell/Tray.h \
    KShell/Ftp.h \
    KShell/FTPTreeList.h \
    KShell/FTPTreeWork.h \
    KShell/HelpWindows.h \
    KShell/HomePage.h \
    KShell/Log.h \
    KShell/NewUser.h \
    KShell/SshClient.h \
    KShell/UPDate.h \
    KShell/resource.h

# UI 文件
FORMS += \
    KShell/SSHWindow.ui \
    KShell/SetingWindow.ui \
    KShell/HelpWindows.ui \
    KShell/HomePage.ui \
    KShell/NewUser.ui

# 资源文件
RESOURCES += \
    KShell/img.qrc

# Windows 资源文件 (图标)
win32:RC_FILE = KShell/logo.rc

# QSsh 库配置
INCLUDEPATH += $$PWD/KShell/include/ssh
DEPENDPATH += $$PWD/KShell/include/ssh

# libcurl 库配置
INCLUDEPATH += $$PWD/KShell/include

# Qt6 private headers (用于 QZipReader/QZipWriter)
greaterThan(QT_MAJOR_VERSION, 5) {
    QT += core-private
}

# 根据构建配置链接不同的库
CONFIG(debug, debug|release) {
    # Debug 配置
    LIBS += -L$$PWD/KShell/lib/debug/ -lQSshd
    LIBS += -L$$PWD/KShell/lib/debug/ -llibcurl_debug
    LIBS += -L$$PWD/KShell/lib/debug/ -lzlib
} else {
    # Release 配置
    LIBS += -L$$PWD/KShell/lib/release/ -lQSsh
    LIBS += -L$$PWD/KShell/lib/release/ -llibcurl
    LIBS += -L$$PWD/KShell/lib/release/ -lzlib
}

# Botan 加密库 (QSsh 依赖)
CONFIG(debug, debug|release) {
    LIBS += -L$$PWD/Debug/ -lBotand
} else {
    LIBS += -L$$PWD/KShell/lib/release/ -lBotand
}

# Windows 特定配置
win32 {
    LIBS += -lws2_32 -ladvapi32 -luser32
}

# 输出目录
CONFIG(debug, debug|release) {
    DESTDIR = $$PWD/build/debug
} else {
    DESTDIR = $$PWD/build/release
}

# Qt6 兼容性定义
greaterThan(QT_MAJOR_VERSION, 5) {
    DEFINES += QT6_COMPAT
}

# Default rules for deployment
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

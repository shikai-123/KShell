#pragma once
#include <qaction.h>
#include <qmenu.h>
#include "HomePage.h"
#include <QTextCodec>
#include <qsystemtrayicon.h>
#include "SSHWindow.h"
#include "FTPTreeList.h"
#include "SetingWindow.h"
#include "HelpWindows.h"
#include <qmainwindow.h>

class Tray :public QMainWindow
{
	Q_OBJECT

public:
	Tray(HomePage& Home_Page);
	~Tray();
private:
	QSystemTrayIcon *trayIcon;
	QAction *quitAction;//退出
	QAction *qa_Guidwindow;
	QAction *qa_SSHwindow;
	QAction *qa_FTPwindow;
	QAction *qa_MonitorData;
	QAction *qa_Setting;
	QAction *qa_Help;
	QAction *qa_ViewModfiyData;
	QAction *qa_CJQConf;
	QAction *qa_ReOpen;
	QMenu   *trayIconMenu;//创建菜单 for 托盘

private slots:
	void trayiconActivated(QSystemTrayIcon::ActivationReason reason);
};
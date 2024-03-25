#include "Tray.h"
#include "UPDate.h"
#pragma execution_character_set("utf-8")//如果托盘不显示 注意可能是这个地方的问题

extern HomePage *g_Home_Page;
extern SSHWindow* g_SSHWindow;
extern FTPTreeList * g_FTPTreeList;
extern SetingWindow* g_SetingWindow;
extern HelpWindows* g_HelpWindows;


Tray::Tray(HomePage& Home_Page)
{
	QIcon icon_logo = QIcon(":/img/logo.ico");//:/img/logo.ico
	QIcon icon_resev = QIcon(":/img/还原.png");
	QIcon icon_quite = QIcon(":/img/退出.png");
	QIcon icon_home = QIcon(":/img/首页.png");
	QIcon icon_zuixiaohua = QIcon(":/img/最小化.png");
	QIcon icon_SSH = QIcon(":/img/SSH终端.png");
	QIcon icon_FTP = QIcon(":/img/ftp.png");
	QIcon icon_Moniyot = QIcon(":/img/数据图.png");
	QIcon icon_Setting = QIcon(":/img/设置.png");
	QIcon icon_Help = QIcon(":/img/帮助.png");
	QIcon icon_View = QIcon(":/img/查看.png");
	QIcon icon_CJQConf = QIcon(":/img/文件配置.png");
	QIcon icon_Reopen = QIcon(":/img/还原.png");



	trayIcon = new QSystemTrayIcon(this);// 创建托盘对象
	trayIcon->setIcon(icon_logo);
	trayIcon->setToolTip(tr("采集器综合管理工具"));
	QString titlec = tr("采集器综合管理工具");
	QString textc = tr("Designed by SK");
	trayIcon->show();
	trayIcon->showMessage(titlec, textc, QSystemTrayIcon::Information, 50000); //弹出气泡提示
	connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayiconActivated(QSystemTrayIcon::ActivationReason)));  //添加单/双击鼠标相应
	quitAction = new QAction(tr("退出"), this);
	quitAction->setIcon(icon_quite);
	connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));

	qa_Guidwindow = new QAction(tr("首页"), this);
	qa_Guidwindow->setIcon(icon_home);
	connect(qa_Guidwindow, &QAction::triggered, [&]() {
		Home_Page.showNormal();//隐藏当前窗口
		Home_Page.raise();//置于最上层
	});
	qa_SSHwindow = new QAction(tr("SSH"), this);
	qa_SSHwindow->setIcon(icon_SSH);//
	connect(qa_SSHwindow, &QAction::triggered, [&]() {
		g_SSHWindow->showNormal();
		g_SSHWindow->raise();
	});
	qa_FTPwindow = new QAction(tr("FTP"), this);
	qa_FTPwindow->setIcon(icon_FTP);//
	connect(qa_FTPwindow, &QAction::triggered, [&]() {
		g_FTPTreeList->showNormal();
		g_FTPTreeList->raise();
	});
	qa_MonitorData = new QAction(tr("实时数据"), this);
	qa_MonitorData->setIcon(icon_Moniyot);//

	qa_Setting = new QAction(tr("设置"), this);
	qa_Setting->setIcon(icon_Setting);//
	connect(qa_Setting, &QAction::triggered, [&]() {
		g_SetingWindow->LoadData();
		g_SetingWindow->showNormal();
		g_SetingWindow->raise();
	});
	qa_Help = new QAction(tr("帮助"), this);
	qa_Help->setIcon(icon_Help);//
	connect(qa_Help, &QAction::triggered, [&]() {
		g_HelpWindows->showNormal();
		g_HelpWindows->raise();
	});


	qa_ReOpen = new QAction(tr("重启"), this);
	qa_ReOpen->setIcon(icon_Reopen);
	connect(qa_ReOpen, &QAction::triggered, [&]() {
		QString program = QApplication::applicationFilePath();
		QStringList arguments = QApplication::arguments();
		QString workingDirectory = QDir::currentPath();
		QProcess::startDetached(program, arguments, workingDirectory);
		QApplication::exit();
	});

	//创建右键弹出菜单
	//trayIconMenu = new QMenu(this);
	//trayIconMenu->addSeparator();//添加分割线
	//trayIconMenu->addAction(qa_Guidwindow);
	//trayIconMenu->addAction(qa_SSHwindow);
	//trayIconMenu->addAction(qa_FTPwindow);
	//trayIconMenu->addAction(qa_CJQConf);
	////trayIconMenu->addAction(qa_Setting);
	////trayIconMenu->addAction(qa_Help);
	//trayIconMenu->addSeparator();//添加分割线
	//trayIconMenu->addAction(qa_ReOpen);
	//trayIconMenu->addAction(quitAction);

	//trayIcon->setContextMenu(trayIconMenu);//把菜单填充到托盘图标中
}

void Tray::trayiconActivated(QSystemTrayIcon::ActivationReason reason)
{
	switch (reason)
	{
	case QSystemTrayIcon::Trigger:
		//单击托盘图标
	case QSystemTrayIcon::DoubleClick:
		//双击托盘图标
		g_Home_Page->showNormal();
		g_Home_Page->raise();
		break;
	default:
		break;
	}
}

Tray::~Tray()
{
	if (UPDate::m_IsUpdatedEXE)
	{
		qDebug() << "程序退出,开始替换exe文件";
		if (QFile::exists(("./../Tools/UPdateForEXE.bat")))//!!要注意的是通过qt打开后，脚本的执行位置和脚本所在的位置是不一样的。
		{
			QDesktopServices::openUrl(QUrl::fromLocalFile("./../Tools/UPdateForEXE.bat"));
		}
		else
		{
			QMessageBox::critical(this, "自动升级", "exe升级失败，没有升级脚本！检查该文件./../Tools/UPdateForEXE.bat");
		}
	}
}

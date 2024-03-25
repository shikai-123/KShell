#include "HomePage.h"
#include <QPushButton>
#include "SSHWindow.h"
#include <qsettings.h>//ini
#include <qdebug.h>
#include "DataStruck.h"
#include "SshClient.h"
#include "NewUser.h"
#include "SetingWindow.h"
#include "HelpWindows.h"
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <qdir.h>
#include "FTPTreeList.h"
#include "FTPTreeWork.h"
#include "UPDate.h"
#include "CJQConf.h"
#pragma execution_character_set("utf-8")

CConnectionForSshClient *g_SshSocket;//ssh客户端的全局指针
QString g_ProjectRegion, g_ElecRoomID, g_DevIP, g_N2NIP, g_ConnetIP, g_DevPort, g_UserName, g_UserWord, g_UserListNote, g_DevName, g_DevIndex, g_ProjectName, g_FTPHead;//用于采集登录信息 g_FTPHead:用来区别ftp和sftp
//ProStatusSsh* g_ProStatusSsh = NULL;
QTreeWidgetItem *g_CurrentItem = nullptr;
int g_ItemRow, g_TopLevelItemRow;//子条目在对应父条目中的位置
int g_WorM = -1;//是写数据库还是修改数据库文件的标志 0写 1修改 Write or Midfiy
extern SSHWindow* g_SSHWindow;//SSH窗口的全局指针
extern SetingWindow* g_SetingWindow;
extern HelpWindows* g_HelpWindows;
extern FTPTreeList * g_FTPTreeList;
extern FTPTreeWork * g_FTPTreeWork;
extern NewUser* g_NewUser;
extern UPDate * g_UPDate;
extern void CheckDir(QString DirName);
extern QMap<QString, QString> g_ProjectNameCSV;//key工程名 value项目编号
extern void Delay(int mSeconds);
extern LogSetting g_LogSetting;
extern OtherToolsSetting g_OtherToolsSetting;
extern CheckProSetting g_CheckProSetting;
extern DataMonitSetting g_DataMonitSetting;
extern FontSetting g_FontSetting;
extern QString g_SSHFTPJzhsDefaultPath;//jzhs默认目录
extern bool g_IsSHHTogetherFTP;//SHH,FTP窗口是否聚合
extern QString g_N2NIPSec;//N2NIP的最后一部分
extern QString g_LastUsedTime;//软件最后一次打开时间for自动更新检测
extern bool g_ISUpDate;

QSqlDatabase g_UserListDB;

HomePage::HomePage(QMainWindow *parent)
{
	ui.setupUi(this);//setupUi(this)是由.ui文件生成的类的构造函数，这个函数的作用是对界面进行初始化，它按照我们在Qt设计器里设计的样子把窗体画出来，把我们在Qt设计器里面定义的信号和槽建立起来。也可以说，setupUi 是我们画界面和写程序之间的桥梁。
	this->setWindowTitle("首页");
	InitStatusBar();
	InitLayout();
	InitreeWidge();//设置用户列表控件的头信息
	PBInit();//操作按钮初始化
	MessageInit();//更新窗口初始化
	AboutMySoftware = new QWidget();//软件介绍
	StartFTPThread();//启动FTP线程
	CreatUserListDB();//创造数据库
	StartUPDateThread();//启动自动升级线程
	slotLoadDBtoUserList();//读取数据库文件，装载到用户列表控件中
	MenuBar();//菜单栏操作--放在最后，因为要等待对应类初始化完毕
	connect(g_NewUser, &NewUser::sigAddUserItem, this, &HomePage::slotLoadOrderDBtoUserList);
	connect(g_NewUser, &NewUser::sigDelted, this, &HomePage::slotDeletOrderDBtoUserList);
	connect(g_NewUser, &NewUser::sigModifed, this, &HomePage::slotModfiedOrderDBtoUserList);
	connect(ui.treeWidgetUserList, SIGNAL(itemPressed(QTreeWidgetItem*, int)), this, SLOT(slotitemDoubleClicked(QTreeWidgetItem*, int)));/*双击--用户列表*/
	connect(g_UPDate, &UPDate::sigUpdateProcess, this, &HomePage::slotShowUpdateProcess);
	connect(g_UPDate, &UPDate::sitStopTimerForUpdate, this, &HomePage::slotStopTimer);

	//setAttribute(Qt::WA_DeleteOnClose);//本窗口关闭后，无论软件有没有退出都要析构
	/*测试按钮*/
	//connect(ui.pb_TEST, SIGNAL(clicked()), this, SLOT(solttest()));
}

HomePage::~HomePage()
{
	//其他的函数都能在软件退出后，执行析构，就他不可以，我也不知道为啥
}

/*创造用户数据库*/
void HomePage::CreatUserListDB()
{
	//创建一个数据库
	g_UserListDB = QSqlDatabase::addDatabase("QSQLITE", "UserList");//注意：只有在第一次打开数据库的时候，才是addDatabase,也就是添加数据库，其他的时候都是“获取指向数据库连接”
	g_UserListDB.setDatabaseName("../conf/UserList.db");//数据库的名字
	bool ok = g_UserListDB.open();//如果不存在就创建，存在就打开
	if (!ok)
	{
		qDebug() << g_UserListDB.lastError().text();//调用上一次出错的原因
		QMessageBox::critical(this, "用户列表错误", "检查数据库文件\n" + g_UserListDB.lastError().text());
		exit(-1);
		g_UserListDB.close();
		QSqlDatabase::removeDatabase("QSQLITE");
	}
	QSqlQuery query(g_UserListDB);
	query.exec("create table UserList(ProjectRegion text, ProjectName text,ElecRoomID text,DevIndex text,IP text,N2NIP text,Port text,UserName text,PassWord text,DevName text,Note text)");//创建表,如果表存在了，就不创建
	g_UserListDB.close();
	QSqlDatabase::removeDatabase("QSQLITE");
}

/*初始化布局*/
void HomePage::InitLayout()
{
	qgl_PushButtonLayout = new QGridLayout();
	qgl_PushButtonLayout->setSpacing(10);//设置间距
	ui.SSH_pb->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	ui.pb_FTPFileManeger->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	qgl_PushButtonLayout->addWidget(ui.SSH_pb, 0, 0);
	qgl_PushButtonLayout->addWidget(ui.pb_FTPFileManeger, 2, 0);

	qbl_PBandUserListLayout = new QHBoxLayout();
	qbl_PBandUserListLayout->addWidget(ui.treeWidgetUserList);
	qbl_PBandUserListLayout->addLayout(qgl_PushButtonLayout);
	qbl_PBandUserListLayout->setStretchFactor(ui.treeWidgetUserList, 5);
	qbl_PBandUserListLayout->setStretchFactor(qgl_PushButtonLayout, 1);

	qvl_GuidWindowLayout = new QVBoxLayout();
	qvl_GuidWindowLayout->addWidget(ui.Guid_lb);
	qvl_GuidWindowLayout->addLayout(qbl_PBandUserListLayout);
	this->centralWidget()->setLayout(qvl_GuidWindowLayout);
}

/*按钮操作初始化*/
void HomePage::PBInit()
{
	/*SSH窗口*/
	connect(ui.SSH_pb, &QPushButton::clicked, [&]() {
		g_SSHWindow->showNormal();
		g_SSHWindow->raise();
	});

	/*文件管理窗口*/
	connect(ui.pb_FTPFileManeger, &QPushButton::clicked, [&]() {
		g_FTPTreeList->slotOpenFTPWindows();
	});

}

/*显示更新过程窗口初始化*/
void HomePage::MessageInit()
{
	qmb_UpdateProcess = new QMessageBox(this);
	qmb_UpdateProcess->setText("");
	qmb_UpdateProcess->addButton(QMessageBox::Ok);
}

/*初始化状态栏*/
void HomePage::InitStatusBar()
{
	sb_StatusBar = new QStatusBar(this);
	this->setStatusBar(sb_StatusBar);
	lb_StatusBarText = new QLabel("IP未选择", this);
	sb_StatusBar->addWidget(lb_StatusBarText);
}

/*启动自动升级---每次打开程序启动一次*/
void HomePage::StartUpdte()
{
	TimeToUpdate = new QTimer();
	TimeToUpdate->setInterval(1000 * 2);
	connect(TimeToUpdate, SIGNAL(timeout()), g_UPDate, SLOT(slotTimeToUpdate()));
	TimeToUpdate->start();
}

//设置用户列表控件的头信息
void HomePage::InitreeWidge()
{
	//设置树控件的头信息
	QStringList list;
	list << "地区、用户" << "配电室" << "采集器" << "IP" << "N2N" << "Port" << "User" << "PW" << "设备型号" << "备注";
	ui.treeWidgetUserList->setHeaderLabels(list);
	ui.treeWidgetUserList->clear();
	ui.treeWidgetUserList->setSortingEnabled(true);
	ui.treeWidgetUserList->header()->setSortIndicatorShown(true);
	ui.treeWidgetUserList->header()->setSortIndicator(3, Qt::AscendingOrder);
	ui.treeWidgetUserList->setColumnHidden(5, 1);//隐藏端口列
	ui.treeWidgetUserList->setColumnHidden(6, 1);//隐藏用户名列
	ui.treeWidgetUserList->setColumnHidden(7, 1);//隐藏密码列
}

/*调用powershell*/
void HomePage::slotOpenPowershell()
{
	qDebug() << " HomePage::slotOpenPowershell()";
	QDesktopServices::openUrl(QUrl("powershell"));//！fromLocalFile从文档来看，他就是告诉openUrl我是一个本地文档，并且支持中文路径，
}

/*调用管理员powershell*/
void HomePage::slotOpenPowershellAdmin()
{
	QProcess p;
	p.startDetached("powershell", QStringList() << "start-process PowerShell -verb runas");

}

/*调用ToolForCJQ*/
void HomePage::slotOpenoolForCJQ()
{
	//取消了QProcess 直接用QDesktopServices 来打开本地文件 能打开，但是软件本身去读ini失败，猜测原因是 ToolforCJQ1的相对路径不是 下面的路径了 而是我工具的路径 把工具放我的目录中应该就行了
	qDebug() << " HomePage::slotOpenoolForCJQ()";
	//QDesktopServices::openUrl(QUrl::fromLocalFile("C:/Users/45428/Desktop/tool4CJQ/tool4CJQ/ToolforCJQ1.0.11.exe"));
	QDesktopServices::openUrl(QUrl::fromLocalFile(g_OtherToolsSetting.ToolForCJQ));
	//g_OtherToolsSetting.ToolForCJQ

}

/*开启状态检测线程*/
void HomePage::StartProStatusThread()
{
	//m_pProStatusThread = new QThread();//创建线程对象
	//g_ProStatusSsh->moveToThread(m_pProStatusThread);//把g_ProStatusSsh对象 整体 移动到m_pProStatusThread上
	//m_pProStatusThread->start();//启动线程
}

/*开启FTP线程*/
void HomePage::StartFTPThread()
{
	m_pFTPThread = new QThread();//创建线程对象
	g_FTPTreeWork->moveToThread(m_pFTPThread);//把g_FTPTreeWork对象 整体 移动到m_pFTPThread上
	m_pFTPThread->start();//启动线程
}

/*开启自动升级线程*/
void HomePage::StartUPDateThread()
{
	m_pUPWorkThread = new QThread;
	g_UPDate->moveToThread(m_pUPWorkThread);//把g_FTPTreeWork对象 整体 移动到m_pFTPThread上
	m_pUPWorkThread->start();//启动线程
}

/*菜单栏操作*/
void HomePage::MenuBar()
{
	m_MenuBar = new QMenuBar(this);
	this->setMenuBar(m_MenuBar);

	m_MenuNew = new QMenu("项目", this);
	m_MenuBar->addMenu(m_MenuNew);
	m_ActNew = new QAction("新建项目", this);
	m_MenuNew->addAction(m_ActNew);


	m_MenuTool = new QMenu("工具", this);
	m_MenuBar->addMenu(m_MenuTool);
	m_ActPowerShell = new QAction("PowerShell", this);
	m_MenuTool->addAction(m_ActPowerShell);
	m_ActPowerShellAdmin = new QAction("PowerShell(管理员)", this);
	m_MenuTool->addAction(m_ActPowerShellAdmin);
	m_ActToolForCJQ = new QAction("ToolForCJQ", this);
	m_MenuTool->addAction(m_ActToolForCJQ);

	m_MenuSet = new QMenu("设置", this);
	m_MenuBar->addMenu(m_MenuSet);
	m_ActSet = new QAction("设置", this);
	m_MenuSet->addAction(m_ActSet);

	m_MenuHelp = new QMenu("帮助", this);
	m_MenuBar->addMenu(m_MenuHelp);
	m_ActHelp = new QAction("帮助", this);
	m_MenuHelp->addAction(m_ActHelp);
	m_ActModbusTypeHelp = new QAction("ModbusType类型说明", this);
	m_MenuHelp->addAction(m_ActModbusTypeHelp);
	m_ActOpenLogDir = new QAction("打开log目录", this);
	m_MenuHelp->addAction(m_ActOpenLogDir);


	m_About = new QMenu("关于", this);
	m_MenuBar->addMenu(m_About);
	m_CheckUpdate = new QAction("检查更新", this);
	m_About->addAction(m_CheckUpdate);
	m_AboutMySoftware = new QAction("软件说明", this);
	m_About->addAction(m_AboutMySoftware);

	connect(m_ActNew, SIGNAL(triggered()), g_NewUser, SLOT(show()));//打开新建项目窗口
	//connect(m_ActSet, SIGNAL(triggered()), g_SetingWindow, SLOT(show()));/*打开设置窗口*/
	connect(m_ActSet, &QAction::triggered, g_SetingWindow, [&] {
		g_SetingWindow->LoadData();
		g_SetingWindow->show();
	});/*启动自动升级线程*/

	connect(m_ActHelp, SIGNAL(triggered()), g_HelpWindows, SLOT(show()));/*打开帮助窗口*/
	connect(m_ActPowerShell, SIGNAL(triggered()), this, SLOT(slotOpenPowershell()));/*打开powershell*/
	connect(m_ActPowerShellAdmin, SIGNAL(triggered()), this, SLOT(slotOpenPowershellAdmin()));/*打开powershell-admin*/
	connect(m_ActToolForCJQ, SIGNAL(triggered()), this, SLOT(slotOpenoolForCJQ()));/*打开toforcjq*/
	connect(m_CheckUpdate, &QAction::triggered, g_UPDate, &UPDate::slotStartUPDate);/*启动自动升级线程*/
	connect(m_AboutMySoftware, &QAction::triggered, this, &HomePage::slotAboutMySoftware);/*打开软件介绍窗口*/
	connect(m_ActModbusTypeHelp, &QAction::triggered, this, &HomePage::slotOpenModbusTypeDoc);/*打开modbus类型说明文件*/
	connect(m_ActOpenLogDir, &QAction::triggered, this, &HomePage::slotOpenLogDir);/*打开log目录*/

}

/*鼠标右键*/
void HomePage::contextMenuEvent(QContextMenuEvent *)
{
	QMenu *menu = new QMenu(ui.treeWidgetUserList);
	QScopedPointer<QAction> m_ActDel(new QAction("删除", ui.treeWidgetUserList));//！！？？怎么限制 只在用户列表打开--已经做出来 参考其他的代码
	QScopedPointer<QAction> m_ActChange(new QAction("修改", ui.treeWidgetUserList));
	QScopedPointer<QAction> m_ActPing(new QAction("Ping", ui.treeWidgetUserList));
	QScopedPointer<QAction> m_ActOpenProjectFile(new QAction("工程目录", ui.treeWidgetUserList));
	QScopedPointer<QAction> m_ActStartN2N(new QAction("连接N2N", ui.treeWidgetUserList));
	QScopedPointer<QAction> m_ActRebootDev(new QAction("重启采集器", ui.treeWidgetUserList));
	QScopedPointer<QAction> m_PoweshellSSH(new QAction("用CMD连接SSH", ui.treeWidgetUserList));
	connect(m_ActNew, SIGNAL(triggered()), g_NewUser, SLOT(show()));//右键活菜单栏新建打开新建项目窗口
	connect(m_ActNew, &QAction::triggered, [&]() {
		g_WorM = 0;
	});
	connect(m_ActDel.get(), SIGNAL(triggered()), g_NewUser, SLOT(soltDeletProjectFromDB()));//右键删除
	connect(m_ActChange.get(), SIGNAL(triggered()), g_NewUser, SLOT(show()));//右键修改打开新建项目窗口
	connect(m_ActChange.get(), SIGNAL(triggered()), g_NewUser, SLOT(slotFillDate()));//右键修改填充新建项目窗口数据
	connect(m_ActPing.get(), SIGNAL(triggered()), this, SLOT(slotCmdPing()), Qt::UniqueConnection);//右键PIng选中的IP
	connect(m_ActOpenProjectFile.get(), SIGNAL(triggered()), this, SLOT(slotOpenProjectFile()), Qt::UniqueConnection);//右键查看选中的IP的工程文件夹
	connect(m_ActStartN2N.get(), SIGNAL(triggered()), this, SLOT(slotStartN2N()), Qt::UniqueConnection);//启动N2N
	connect(m_PoweshellSSH.get(), SIGNAL(triggered()), this, SLOT(slotOpenSSH()), Qt::UniqueConnection);//右键启动SSH

	connect(m_ActChange.get(), &QAction::triggered, [&]() {g_WorM = 1; });
	menu->addAction(m_ActNew);
	menu->addAction(m_ActDel.get());
	menu->addAction(m_ActChange.get());
	menu->addSeparator();
	menu->addAction(m_ActPing.get());
	menu->addAction(m_ActOpenProjectFile.get());
	menu->addSeparator();
	menu->addAction(m_ActStartN2N.get());
	menu->addSeparator();
	menu->addAction(m_ActRebootDev.get());
	menu->addSeparator();
	menu->addAction(m_PoweshellSSH.get());
	menu->exec(QCursor::pos());//！！！！connect函数必须放在 这句话的上面，至于为啥，暂不清楚！--因为执行到这个地方，右键才出来，如果在后面connect的话，connect是无效的。

	delete menu;
}

/*测试槽函数*/
void HomePage::solttest()
{
	qDebug() << "调用HomePage::solttest()";
}

/*CMD线程--执行ping操作*/
void HomePage::slotCmdPing()
{
	QProcess p;
	if (g_ConnetIP == "")
	{
		QMessageBox::critical(NULL, "错误", "IP为空");
		return;
	}

	//p->start("powershell", QStringList() << "/c" << "ping "<< g_DevIP);
	//p->startDetached("cmd", QStringList() << "/c" << " start Ping "<< g_DevIP<< " -t");
	//！！最大的好处就是打开一个新的cmd窗口去执行文件。这样在程序发布后去掉cmd窗口后，还是有个提示
	p.startDetached("powershell", QStringList() << "Start-Process -FilePath Ping.exe -ArgumentList " << g_ConnetIP);//不用在g_ConnetIP左右在反斜杠
	/*
	！！在用start的时候一定要加上后面这两个语句，如果不加的话 就会出现QProcess: Destroyed while process (\"powershell\") is still running.
	原因是这个函数结束之后，会回收powershell的资源 如果中间不阻塞一下的话，那就出现还在执行的过程中，去回收这个资源，导致报上述错误。 用detach就没有问题，因为他是独立出来的
	p.start("powershell", QStringList() << "Start-Process -FilePath Ping.exe -ArgumentList " << " \" " << g_DevIP << " -t \" ");
	p.waitForStarted();
	p.waitForFinished();
	*/
	/*
	Start-Process -FilePath Ping.exe -ArgumentList "192.168.1.1 -t"
	start Ping  192.168.1.1
	*/
}

/*读取用户信息数据库，装载到用户列表控件中*/
void HomePage::slotLoadDBtoUserList()
{
	//对用户列表头进行整理
	ui.treeWidgetUserList->clear();
	m_ProjectRegion.clear();//!在用户列表控件清空后，这个地方是一定要清空的。因为m_ProjectRegion中保存的QTreeWidgetItem顶层控件的指针都被清了。总之就是所有的东西都请了！
	ui.treeWidgetUserList->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
	//创建一个数据库
	g_UserListDB = QSqlDatabase::database("UserList");//注意：只有在第一次打开数据库的时候，才是addDatabase,也就是添加数据库，其他的时候都是“获取指向数据库连接”
	g_UserListDB.setDatabaseName("../conf/UserList.db");//数据库的名字
	bool ok = g_UserListDB.open();//如果不存在就创建，存在就打开
	if (!ok)
	{
		qDebug() << g_UserListDB.lastError().text();//调用上一次出错的原因
		QMessageBox::critical(this, "用户列表错误", "检查数据库文件\n" + g_UserListDB.lastError().text());
		exit(-1);
		g_UserListDB.close();
		QSqlDatabase::removeDatabase("QSQLITE");
	}
	QSqlQuery query(g_UserListDB);

	//添加地区，!相同名字顶层控件无法重复添加
	query.exec("SELECT DISTINCT ProjectRegion FROM UserList");
	while (query.next())// 密云区，延庆区，朝阳区，丰台区，石景山区，海淀区，门头沟区，房山区，通州区，顺义区，昌平区，大兴区，大兴区，怀柔区，平谷区，东城区，西城区。
	{
		if (m_ProjectRegion.find(query.value(0).toString()) == m_ProjectRegion.end())//该地区同样的地区，添加一次，即使不加这个限制也没有问题，因为!相同名字顶层控件无法重复添加。非顶层控件可以添加一样的。
		{
			m_ProjectRegion.insert(query.value(0).toString(), new QTreeWidgetItem(QStringList() << query.value(0).toString()));
			ui.treeWidgetUserList->addTopLevelItem(m_ProjectRegion[query.value(0).toString()]);
		}
	}
	//地区下添加工程
	query.exec("SELECT DISTINCT ProjectRegion,ProjectName FROM UserList");
	while (query.next())
	{
		if (m_ProjectName.find(query.value(1).toString()) == m_ProjectName.end())
		{
			m_ProjectName.insert(query.value(1).toString(), new QTreeWidgetItem(QStringList() << query.value(1).toString()));
			m_ProjectRegion[query.value(0).toString()]->addChild(m_ProjectName[query.value(1).toString()]);
		}
	}
	//工程下添加CJQ
	query.exec("SELECT * FROM UserList");
	while (query.next())
	{
		QString CJQuuid = query.value(0).toString() + query.value(1).toString() + query.value(2).toString() + query.value(3).toString() + query.value(4).toString() + query.value(5).toString() + query.value(6).toString() + query.value(7).toString() + query.value(8).toString() + query.value(9).toString() + query.value(10).toString();
		if (m_ProjectCJQName.find(CJQuuid) == m_ProjectCJQName.end())
		{
			m_ProjectCJQName.insert(CJQuuid,
				new QTreeWidgetItem(QStringList() << "  " << query.value(2).toString() << query.value(3).toString() << query.value(4).toString() << query.value(5).toString() << query.value(6).toString()\
					<< query.value(7).toString() << query.value(8).toString() << query.value(9).toString() << query.value(10).toString()));

			m_ProjectName[query.value(1).toString()]->addChild(m_ProjectCJQName[CJQuuid]);
		}
		CheckDir("../Project/" + query.value(0).toString() + "/" + query.value(1).toString() + "/" + query.value(4).toString());//放在if外的原因是因为，如果不小心把文件夹删掉。重启软件之后，还可以重新建立
	}
	g_UserListDB.close();
	QSqlDatabase::removeDatabase("QSQLITE");
}

/*增加指定的用户条目*/
void HomePage::slotLoadOrderDBtoUserList(QString ProjectRegion, QString ProjectName, QString ElecRoomID, QString DevIndex, QString IP, QString N2NIP, QString Port, QString UserName, QString PassWord, QString DevName, QString Note)
{
	//目前也是有问题，在不同的地区下，添加同一个相同的项目会出问题——项目会添加到老项目下面。但实际情况下不同的地区，项目名字不一样。
	//加上后面的这个是因为，当本身就存在这个条目，但是被删除后，又添加回来的时候，如果没有后面的这个条件就会失败，
	//因为被删除后，m_ProjectCJQName[对应的地区]在map中是由这个数据的，前面的条件就过不去。所以需要后面的条件。
	if (m_ProjectRegion.find(ProjectRegion) == m_ProjectRegion.end() || m_ProjectRegion[ProjectRegion] == nullptr)
	{
		m_ProjectRegion.insert(ProjectRegion, new QTreeWidgetItem(QStringList() << ProjectRegion));
		ui.treeWidgetUserList->addTopLevelItem(m_ProjectRegion[ProjectRegion]);//添加地区，!相同名字顶层控件无法重复添加
	}
	if (m_ProjectName.find(ProjectName) == m_ProjectName.end() || m_ProjectName[ProjectName] == nullptr)
	{
		m_ProjectName.insert(ProjectName, new QTreeWidgetItem(QStringList() << ProjectName));
		m_ProjectRegion[ProjectRegion]->addChild(m_ProjectName[ProjectName]);//地区下添加工程
	}
	QString CJQuuid = ProjectRegion + ProjectName + ElecRoomID + DevIndex + IP + N2NIP + Port + UserName + PassWord + DevName + Note;
	if (m_ProjectCJQName.find(CJQuuid) == m_ProjectCJQName.end() || m_ProjectCJQName[CJQuuid] == nullptr)
	{
		m_ProjectCJQName.insert(CJQuuid, new QTreeWidgetItem(QStringList() << "  " << ElecRoomID << DevIndex << IP << N2NIP << Port << UserName << PassWord << DevName << Note));
		m_ProjectName[ProjectName]->addChild(m_ProjectCJQName[CJQuuid]);//工程下添加CJQ
	}
}

/*修改指定的用户条目*/
void HomePage::slotModfiedOrderDBtoUserList(QString ProjectRegion, QString ProjectName, QString ElecRoomID, QString DevIndex, QString IP, QString N2NIP, QString Port, QString UserName, QString PassWord, QString DevName, QString Note, \
	QString OldProjectRegion, QString OldProjectName, QString OldElecRoomID, QString OldDevIndex, QString OldIP, QString OldN2NIP, QString OldPort, QString OldUserName, QString OldPassWord, QString OldDevName, QString OldNote)
{
	QTreeWidgetItem* item = ui.treeWidgetUserList->currentItem();//当前节点
	if (item->parent() == nullptr)//点击父节点
	{
		item->setText(0, ProjectRegion);
		m_ProjectRegion[ProjectRegion] = m_ProjectRegion[OldProjectRegion];
		m_ProjectRegion[OldProjectRegion] = nullptr;
	}
	else if (item->parent()->parent() != nullptr)//点击子子节点
	{
		item->setText(1, ElecRoomID);
		item->setText(2, DevIndex);
		item->setText(3, IP);
		item->setText(4, N2NIP);
		item->setText(5, Port);
		item->setText(6, UserName);
		item->setText(7, PassWord);
		item->setText(8, DevName);
		item->setText(9, Note);

		m_ProjectRegion[ElecRoomID] = m_ProjectRegion[OldElecRoomID];
		m_ProjectRegion[DevIndex] = m_ProjectRegion[OldDevIndex];
		m_ProjectRegion[IP] = m_ProjectRegion[OldIP];
		m_ProjectRegion[N2NIP] = m_ProjectRegion[OldN2NIP];
		m_ProjectRegion[Port] = m_ProjectRegion[OldPort];
		m_ProjectRegion[UserName] = m_ProjectRegion[OldUserName];
		m_ProjectRegion[PassWord] = m_ProjectRegion[OldPassWord];
		m_ProjectRegion[DevName] = m_ProjectRegion[OldDevName];
		m_ProjectRegion[Note] = m_ProjectRegion[OldNote];

		m_ProjectRegion[OldElecRoomID] = nullptr;
		m_ProjectRegion[OldDevIndex] = nullptr;
		m_ProjectRegion[OldIP] = nullptr;
		m_ProjectRegion[OldN2NIP] = nullptr;
		m_ProjectRegion[OldPort] = nullptr;
		m_ProjectRegion[OldUserName] = nullptr;
		m_ProjectRegion[OldPassWord] = nullptr;
		m_ProjectRegion[OldDevName] = nullptr;
		m_ProjectRegion[OldNote] = nullptr;
	}
	else//单击子节点，在该项目中没有什么要处理的，所以就不做处理
	{
		item->setText(0, ProjectName);
		m_ProjectRegion[ProjectName] = m_ProjectRegion[OldProjectName];
		m_ProjectRegion[OldProjectName] = nullptr;
	}
}

/* 删除指定的用户条目*/
void HomePage::slotDeletOrderDBtoUserList()
{
	QTreeWidgetItem* item = ui.treeWidgetUserList->currentItem();//当前节点
	if (item->parent() == nullptr)//点击父节点
	{
		qDebug() << item->text(0);
		m_ProjectRegion[item->text(0)] = nullptr;
		for (int ChildIndex = 0; ChildIndex < item->childCount(); ChildIndex++)
		{
			QTreeWidgetItem* ChildItem = item->child(ChildIndex);
			qDebug() << ChildItem->text(0);
			m_ProjectName[ChildItem->text(0)] = nullptr;
			for (int GrandChildIndex = 0; GrandChildIndex < ChildItem->childCount(); GrandChildIndex++)
			{
				QTreeWidgetItem* GrandChildItem = ChildItem->child(GrandChildIndex);
				QString CJQuuid = GrandChildItem->parent()->parent()->text(0) + GrandChildItem->parent()->text(0) + GrandChildItem->text(1) + GrandChildItem->text(2) + GrandChildItem->text(3) + GrandChildItem->text(4) + GrandChildItem->text(5)\
					+ GrandChildItem->text(6) + GrandChildItem->text(7) + GrandChildItem->text(8) + GrandChildItem->text(9) + GrandChildItem->text(10);
				m_ProjectCJQName[CJQuuid] = nullptr;
				qDebug() << CJQuuid;
			}
		}
		delete  ui.treeWidgetUserList->takeTopLevelItem(ui.treeWidgetUserList->currentIndex().row());
	}
	else if (item->parent()->parent() != nullptr)//点击子子节点
	{

		QString CJQuuid = item->parent()->parent()->text(0) + item->parent()->text(0) + item->text(1) + item->text(2) + item->text(3) + item->text(4) + item->text(5) + item->text(6) + item->text(7) + item->text(8) + item->text(9) + item->text(10);
		QTreeWidgetItem* parItem = item->parent();//父节点
		parItem->removeChild(item);//移除一个子节点，并不会删除
		delete item;//!!item的值和m_ProjectCJQName[CJQuuid]值是一样的。只是删除这个变量是删除不掉的，得用removeChild函数。
		m_ProjectCJQName[CJQuuid] = nullptr;
	}
	else//单击子节点，在该项目中没有什么要处理的，所以就不做处理
	{
		QTreeWidgetItem* parItem = item->parent();//父节点
		for (int ChildIndex = 0; ChildIndex < item->childCount(); ChildIndex++)
		{
			QTreeWidgetItem* ChildItem = item->child(ChildIndex);
			QString CJQuuid = ChildItem->parent()->parent()->text(0) + ChildItem->parent()->text(0) + ChildItem->text(1) + ChildItem->text(2) + ChildItem->text(3) + ChildItem->text(4) + ChildItem->text(5)\
				+ ChildItem->text(6) + ChildItem->text(7) + ChildItem->text(8) + ChildItem->text(9) + ChildItem->text(10);
			m_ProjectCJQName[CJQuuid] = nullptr;
			qDebug() << CJQuuid;
		}
		m_ProjectCJQName[item->text(0)] = nullptr;
		parItem->removeChild(item);//移除一个子节点，并不会删除
		delete item;//!!item的值和m_ProjectCJQName[CJQuuid]值是一样的。只是删除这个变量是删除不掉的，得用removeChild函数。
	}
}

/*打开对应的工程文件目录*/
void HomePage::slotOpenProjectFile()
{
	QDesktopServices::openUrl(QUrl::fromLocalFile("../Project/" + g_ProjectRegion + "/" + g_ProjectName + "/" + g_DevIP));//！fromLocalFile从文档来看，他就是告诉openUrl我是一个本地文档，并且支持中文路径，
}

/*显示升级进度*/
void HomePage::slotShowUpdateProcess(int Pro, int All, QString Filename)
{
	qmb_UpdateProcess->setText("当前：" + QString::number(Pro) + "   共计：" + QString::number(All) + "\n" + Filename);
	qmb_UpdateProcess->show();
}

/*启动N2N*/
void HomePage::slotStartN2N()
{
	if (!QFile::exists("./../Tools/StartN2N.bat"))
	{
		QMessageBox::critical(this, "n2n启动失败", "StartN2N.bat缺失");
		return;
	}
	if (g_N2NIP == "" || g_N2NIPSec == "")
	{
		QMessageBox::critical(this, "n2n连接错误", "n2n IP为空\n或者\n你的配置文件N2N后缀为空");
		return;
	}
	g_ConnetIP = g_N2NIP;
	QString MyN2nIP = g_N2NIP;
	MyN2nIP.remove(g_N2NIP.lastIndexOf(".") + 1, 3);
	MyN2nIP += g_N2NIPSec;
	//C:/Windows/System32/WindowsPowerShell/v1.0/powershell.exe
	//./../test/UPdateForEXE.bat
	//return;
	QProcess p;
	p.startDetached("powershell", QStringList() << "Start-Process -FilePath ./../Tools/StartN2N.bat -ArgumentList " << "\"`\"" + g_OtherToolsSetting.N2N + "`\"\"" << "," << MyN2nIP);//多个参数用逗号分开
	//p.startDetached("powershell", QStringList() << "Start-Process -FilePath ./../Tools/StartN2N.bat ");//多个参数用逗号分开
	//p.startDetached("powershell", QStringList() << "start-process PowerShell -verb runas");
}

/*停止自动升级的定时器*/
void HomePage::slotStopTimer()
{
	TimeToUpdate->stop();
	delete TimeToUpdate;
}

/*软件介绍*/
void HomePage::slotAboutMySoftware()
{
	AboutMySoftware->resize(400, 200);
	lb_AboutMySoftware = new QLabel("123", AboutMySoftware);
	lb_AboutMySoftware->setText("本软件基于C++11，QT开发。\
	\n在基于SSH，SFTP，FTP三大模块基础上，实现了自定义功能开发和使用。\
	\n部分技术介绍：sqlite3，xml，ini，josn，boost，udp，ssh，ftp，sftp，\
	\n多线程以及线程池，c++11特性，zlib，protobuf，zip文件解压，qtcore，\
	\nhttp通讯，动态窗口布局等等\
	\n\n\n内部使用，请勿外传！");
	AboutMySoftware->show();
}

/*打开modbustype类型说明文档*/
void HomePage::slotOpenModbusTypeDoc()
{
	if (QFile::exists(("./../Tools/Jzag-Modbus数据解析类型说明.docx")))//!!要注意的是通过qt打开后，脚本的执行位置和脚本所在的位置是不一样的。
	{
		QDesktopServices::openUrl(QUrl::fromLocalFile("./../Tools/Jzag-Modbus数据解析类型说明.docx"));
	}
	else
	{
		QMessageBox::critical(this, "Modbus数据解析类型说明.", "打开失败，该文件不存在！检查该文件./../Tools/Jzag-Modbus数据解析类型说明.docx");
	}
}

/*打开log目录*/
void HomePage::slotOpenLogDir()
{
	QDesktopServices::openUrl(QUrl::fromLocalFile("./../log/"));
}

/*通过poweshell打开ssh*/
void HomePage::slotOpenSSH()
{
	QProcess p;
	QString CMD = "Start-Process -FilePath ssh.exe -ArgumentList 'root@" + g_DevIP + "'";
	p.startDetached("powershell", QStringList() << CMD);
}

/*点击用户信息*/
void HomePage::slotitemDoubleClicked(QTreeWidgetItem * item, int column)
{
	g_CurrentItem = ui.treeWidgetUserList->currentItem();
	g_ItemRow = ui.treeWidgetUserList->currentIndex().row();
	if (ui.treeWidgetUserList->currentItem()->parent() == nullptr)//点击父节点
	{
		//qDebug() << "父节点";
		g_NewUser->ui.cb_ProjectRegion->setCurrentText(item->text(0));//给新建用户窗口的地区名字赋值
		g_ProjectRegion = item->text(0);
		lb_StatusBarText->setText("IP未选择");
		g_ProjectName = "";
		g_DevIP = "";
		g_N2NIP = "";
		g_ConnetIP = "";//点击父节点的时候，清空IP地址。要不然点击父节点IP也可以链接
		g_TopLevelItemRow = ui.treeWidgetUserList->indexOfTopLevelItem(g_CurrentItem);//拿到父节点的位置
	}
	else if (item->parent()->parent() != nullptr)//点击子子节点
	{
		g_NewUser->ui.cb_ProjectRegion->setCurrentText(ui.treeWidgetUserList->currentItem()->parent()->parent()->text(0));//给新建用户窗口的地区名字赋值
		g_NewUser->ui.le_ProjectName->setText(ui.treeWidgetUserList->currentItem()->parent()->text(0));//给新建用户窗口的工程名字赋值
		QTreeWidgetItem  *parentItem = item->parent();
		g_ProjectRegion = item->parent()->parent()->text(0);
		g_ProjectName = parentItem->text(0);
		g_ElecRoomID = item->text(1);
		g_DevIndex = item->text(2);
		g_DevIP = item->text(3);
		g_N2NIP = item->text(4);
		g_DevPort = item->text(5);
		g_UserName = item->text(6);
		g_UserWord = item->text(7);
		g_DevName = item->text(8);
		g_UserListNote = item->text(9);
		if (g_DevName == "中嵌")
		{
			g_FTPHead = "ftp://";//ftp后面必须两个斜杠！！
		}
		else if (g_DevName == "集智达" || g_DevName == "华威" || g_DevName == "centos" || g_DevName == "研华")
		{
			g_FTPHead = "sftp://";//sftp斜杠有一个两个无所谓,为了保持统一，多加一个
		}
		//qDebug() << item->text(column) << "该子节点对应父节点" << g_ProjectName << "该子节点对应父节点的位置" << g_ItemRow<< "该项在子节点中的位置" << column;
		//g_MonitorData->m_bFirst = true;//在每次点击ip的时候，都会重新刷新一遍数据。
		if (column == 4)//链接n2n ip的时候，需要单独点击一下n2n ip。 其他点击条目任意位置都是devip
		{
			g_ConnetIP = g_N2NIP;
			lb_StatusBarText->setText("选中的n2nIP为：" + g_ConnetIP);
		}
		else
		{
			g_ConnetIP = g_DevIP;
			lb_StatusBarText->setText("选中的物理IP为：" + g_ConnetIP);
		}
	}
	else//单击子节点，在该项目中没有什么要处理的，所以就不做处理
	{
		g_NewUser->ui.cb_ProjectRegion->setCurrentText(ui.treeWidgetUserList->currentItem()->parent()->text(0));//给新建用户窗口的地区名字赋值
		g_NewUser->ui.le_ProjectName->setText(ui.treeWidgetUserList->currentItem()->text(0));//给新建用户窗口的工程名字赋值
		g_ProjectRegion = ui.treeWidgetUserList->currentItem()->parent()->text(0);
		g_ProjectName = ui.treeWidgetUserList->currentItem()->text(0);
	}
}


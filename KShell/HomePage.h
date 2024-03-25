#pragma once
#include "ui_HomePage.h"
#include"SshClient.h"
#include "SSHWindow.h"
#include <qthread.h>
#include <QMenuBar>
#include <QMenu> //菜单
#include <QAction> //菜单项
#include <QJsonParseError>
#include <qcompleter.h>
#include <qsqldatabase.h>
#include <qsqlerror.h>
#include <qsqlquery.h>
#include <qmap.h>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <qstatusbar.h>
class HomePage : public QMainWindow
{
	Q_OBJECT
		//添加这个宏 是为了有connect函数  和moc有关
public:
	HomePage(QMainWindow *parent = Q_NULLPTR);
	~HomePage();

	QMenuBar *m_MenuBar;
	QMenu *m_MenuNew;
	QMenu *m_MenuSet;
	QMenu *m_MenuTool;
	QMenu *m_About;
	QMenu *m_MenuHelp;
	QAction *m_ActNew;
	QAction *m_ActSet;
	QAction *m_ActHelp;
	QAction *m_ActModbusTypeHelp;
	QAction *m_ActOpenLogDir;
	QAction *m_ActDel;
	QAction *m_ActChange;
	QAction *m_ActPing;
	QAction *m_ActPowerShell;
	QAction *m_ActPowerShellAdmin;
	QAction *m_ActToolForCJQ;
	QAction *m_CheckUpdate;
	QAction *m_AboutMySoftware;
	QThread* m_pProStatusThread;
	QThread* m_pProMonitorDataThread;
	QThread* m_pFTPThread;
	QThread *m_pUPWorkThread;//自动升级，只会执行一会，到时候把他关掉

	//现在想想，这种用户列表的修改方式以及下面的三个map方案，实在是太拉了，1、应该能在控件上直接改，2、去掉这个三个map作为中间件。3、不该有任何的耦合，想怎么重复就怎么重复，想么该就怎么改！！！在设计的时候就没考虑好，不该做一些检测。
	QSqlDatabase g_UserListDB;
	QMap<QString, QTreeWidgetItem*> m_ProjectRegion;//key 区域名 vaule 区域名控件
	QMap<QString, QTreeWidgetItem*> m_ProjectName;//key 项目名 vaule 项目名控件
	QMap<QString, QTreeWidgetItem*> m_ProjectCJQName;//key 项目名 vaule 项目名下的具体的采集
	QGridLayout* qgl_PushButtonLayout;
	QHBoxLayout* qbl_PBandUserListLayout;
	QVBoxLayout* qvl_GuidWindowLayout;
	QMessageBox* qmb_UpdateProcess;
	QStatusBar* sb_StatusBar;
	QLabel* lb_StatusBarText;
	QTimer* TimeToUpdate;

	QWidget* AboutMySoftware;
	QLabel* lb_AboutMySoftware;

	void StartProStatusThread();
	void StartFTPThread();
	void StartUPDateThread();
	void MenuBar();
	void contextMenuEvent(QContextMenuEvent *);//右键
	void InitreeWidge();//初始化treeWidge标题
	void CreatUserListDB();
	void InitLayout();
	void PBInit();
	void MessageInit();
	void InitStatusBar();
	void StartUpdte();
private:
	Ui::HomePage ui;

signals:
	void OpenMainWindow();
	void sigDisconnected();//断开连接信号
	void sigStartProStatusThread();

public slots:
	void solttest();
	void slotOpenPowershell();
	void slotOpenPowershellAdmin();
	void slotOpenoolForCJQ();
	void slotitemDoubleClicked(QTreeWidgetItem *item, int column);
	void slotCmdPing();
	void slotLoadDBtoUserList();
	void slotLoadOrderDBtoUserList(QString ProjectRegion, QString ProjectName, QString ElecRoomID, QString DevIndex, \
		QString IP, QString N2NIP, QString Port, QString UserName, QString PassWord, QString DevName, QString Note);

	void slotModfiedOrderDBtoUserList(QString ProjectRegion, QString ProjectName, QString ElecRoomID, QString DevIndex, QString IP, QString N2NIP, QString Port, QString UserName, QString PassWord, QString DevName, QString Note, \
		QString OldProjectRegion, QString OldProjectName, QString OldElecRoomID, QString OldDevIndex, QString OldIP, QString OldN2NIP, QString OldPort, QString OldUserName, QString OldPassWord, QString OldDevName, QString OldNote);


	void slotDeletOrderDBtoUserList();
	void slotOpenProjectFile();
	void slotShowUpdateProcess(int Pro, int All, QString Filename);
	void slotStartN2N();
	void slotStopTimer();
	void slotAboutMySoftware();
	void slotOpenModbusTypeDoc();
	void slotOpenLogDir();
	void slotOpenSSH();
};

#pragma once
#include <QWidget>
#include "ui_SSHWindow.h"
#include <qtimer.h>
#include "SshClient.h"
#include <qmessagebox.h>
#include "qtabbar.h"
#include <qvector.h>
#include <qgridlayout.h>
#include <qtextedit.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qstandarditemmodel.h>
#include <qsqldatabase.h>
#include <qsqlerror.h>
#include <qsqlquery.h>
#include <qmenu.h>
#include <qscopedpointer.h>
#include <qformlayout.h>
#include <QSplitter>
#include <qmainwindow.h>
#include "DataStruck.h"
#include <qpainter.h>
#include <qitemdelegate.h>
/*与SSH主进程配合使用。*/
class SSHWindow : public QWidget
{
	Q_OBJECT;

public:
	QVector<bool>m_bConnectState;//连接状态 0未连接 1连接  添加新的子窗口，才会插入新的对象
	QVector<CConnectionForSshClient *>m_sshSocket;//通过有参构造 拿到SshClient的对象
	QPixmap ProStatusInitMap;
	QPixmap ProStatusOnlineMap;
	QPixmap proStatusOutlineMap;
	SSHWindow(QWidget *parent = Q_NULLPTR);
	~SSHWindow();
	QVector< QThread *> m_pThread;//SSH线程
	QPushButton* qb_SSHWidgetadd;//tab窗口中的加号
	QVector<QPushButton*>qb_SSHConnet;//连接按钮
	QVector<QPushButton*>qb_SSHSend;//发送按钮
	QVector<QLineEdit*>qe_SSHCmdLine;//命令发送输入框
	QVector<QWidget*> tw_NewTapWidget;//每个子窗口的界面
	QVector<QGridLayout*>qgl_NewTapLay;//每个子窗口的布局
	QVector<QTextEdit*> qe_SSHText;//每个子窗口的SSH输出窗口
	int m_SSHNum = 0;//SSH数量
	int m_CurrentSSHIndex = 0;//当前选中的SSH子窗口
	QSqlDatabase m_CMDListDB;
	//表格
	QStandardItemModel* m_CMDItemModel;
	QList<QStandardItem *> m_itemList;
	QLineEdit* le_CMD;
	QLineEdit* le_CMDText;
	QPushButton* qb_CMDConfirm;
	QFormLayout* qfl_layout;
	QWidget * qw_CMDWindow;
	QString m_CMD;
	QString m_CMDtext;
	QString SSHRecStr = "";
	bool ISCreat = false;
	void CreatDB();
	void OpenDB();
	void CloseDB();
	void contextMenuEvent(QContextMenuEvent *);
	void Init();
	bool eventFilter(QObject *object, QEvent *event);
	void InitTapWidget();
	void InitTable();
	void InitCMDWidget();
	void keyReleaseEvent(QKeyEvent *e);
	void InitForSSHTextRK();
	Ui::SSHWindow ui;



public slots:
	void slotDataArrived(QString strMsg, QString strIp, int nPort, int SSHIndex);//把收到的终端信息打印出来
	void slotConnectStateChanged(bool bState, QString Err, int SSHIndex);//改变连接图标的文字
	void slotSshSendCmd();
	void slotSshConnect();
	void slotSshSendERR(int Err);
	void slotAddTapwindow();
	void slotCloseTab(int SSHIndex);
	void slottest();
	void slottest(int SSHIndex);
	void slottest(QString CMD, int SSHIndex);
	void slotGetSSHTabIndex(int SSHIndex);
	void slotLoadBDtoCMDTable();
	void slotTableClicked(const QModelIndex &index);//左击
	void slotTablePress(const QModelIndex &index);//点击
	void slotCreatCMDToDB();

signals:
	void sigtest();
	void sigSend(QString strMsg);//包含命令内容的信号
	void sigSshSendERR(int Err);
	void sigDisconnected();
	void sigCreateConnection();
};

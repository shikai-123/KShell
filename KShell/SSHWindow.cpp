#include "SSHWindow.h"
#include <qpixmap.h>
#include <Windows.h>
#include <qdatetime.h>
#include"SshClient.h"
#include <QDebug>
#include<qevent.h>
#include "FTPTreeList.h"

#pragma execution_character_set("utf-8")
extern bool g_bConnectState;
//extern ProStatusSsh *g_ProStatusSsh;
extern QString g_ElecRoomID, g_DevIP, g_ConnetIP, g_DevPort, g_UserName, g_UserWord, g_UserListNote, g_DevName, g_DevIndex, g_ProjectName, g_FTPHead;//用于采集登录信息 g_FTPHead:用来区别ftp和sftp
extern QString g_SSHFTPJzhsDefaultPath;
QMap<QString, JzagErr> g_JzagErrCSV;//key错误编号 value错误信息
// mSeconds 毫秒 最大100s
void Delay(int mSeconds)
{
	QTime dieTime = QTime::currentTime().addMSecs(mSeconds);//返回一个当前时间对象之后或之前ms毫秒的时间对象(之前还是之后视ms的符号,如为正则之后，反之之前)
	while (QTime::currentTime() < dieTime)
	{
		QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
	}
}

SSHWindow::SSHWindow(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	Init();
	LoadJzagErrCSV();
	//connect(ui.pb_Connect, SIGNAL(clicked()), g_ProStatusSsh, SLOT(slotRun()));//连接按钮--启动程序检测线程！
	connect(this, SIGNAL(sigSshSendERR(int)), this, SLOT(slotSshSendERR(int)));
	connect(this, &SSHWindow::sigJzagErrmsg, this, &SSHWindow::slotPraseJzagErrMsg);
	connect(this, &SSHWindow::sigStartJzag, this, &SSHWindow::slotClearJzagErrTab);
}

SSHWindow::~SSHWindow()
{
}

//创建一个数据库
void SSHWindow::CreatDB()
{
	m_CMDListDB = QSqlDatabase::addDatabase("QSQLITE", "CMDList");//添加驱动---虽然不是同一个数据库，但是两个数据库的名字都是默认的，这样名字就是一样的，就会出现问题
	m_CMDListDB.setDatabaseName("../conf/CMDList.db");//数据库的名字----这个地方只是说明路径而已，其他就没了
}

/*打开命令列表数据库*/
void SSHWindow::OpenDB()
{
	m_CMDListDB = QSqlDatabase::database("CMDList");//获取指向数据库连接
	m_CMDListDB.setDatabaseName("../conf/CMDList.db");//数据库的名字
	bool ok = m_CMDListDB.open();//如果不存在就创建，存在就打开
	if (!ok)
	{
		qDebug() << m_CMDListDB.lastError().text();//调用上一次出错的原因
		QMessageBox::critical(this, "命令列表错误", "检查数据库文件\n" + m_CMDListDB.lastError().text());
		exit(-1);
		m_CMDListDB.close();
		QSqlDatabase::removeDatabase("QSQLITE");
	}
}

/*关闭数据库*/
void SSHWindow::CloseDB()
{
	m_CMDListDB.close();
	QSqlDatabase::removeDatabase("QSQLITE");
}

/*SSH窗口右键*/
void SSHWindow::contextMenuEvent(QContextMenuEvent *)
{
	if (ui.tv_CMDTable->underMouse())
	{
		QMenu *menu = new QMenu(ui.tv_CMDTable);
		QScopedPointer<QAction> m_ActNew(new QAction("新建", ui.tv_CMDTable));
		QScopedPointer<QAction> m_ActChange(new QAction("修改", ui.tv_CMDTable));
		QScopedPointer<QAction> m_ActDel(new QAction("删除", ui.tv_CMDTable));
		connect(m_ActNew.get(), &QAction::triggered, [&]() {
			ISCreat = true;
			le_CMD->clear();
			le_CMDText->clear();
			qw_CMDWindow->show();
		});
		connect(m_ActChange.get(), &QAction::triggered, [&]() {
			ISCreat = false;
			le_CMD->setText(m_CMD);
			le_CMDText->setText(m_CMDtext);
			qw_CMDWindow->show();
		});
		connect(m_ActDel.get(), &QAction::triggered, [&]() {
			int ret = QMessageBox::question(this, "删除", "是否删除", QMessageBox::Yes | QMessageBox::No);
			if (ret == QMessageBox::No)
			{
				return;
			}
			OpenDB();
			QSqlQuery query(m_CMDListDB);
			query.prepare("delete from CMDList where CMD = :CMD and  CMDtext = :CMDtext"); //名称绑定的方式 注意和一般的sql语句不同的是，在text类型的时候他不需要加 ''--如果在使用exec的纯sql语句的时候 必须加上''
			query.bindValue(":CMD", m_CMD);
			query.bindValue(":CMDtext", m_CMDtext);
			bool ok = query.exec();
			CloseDB();
			slotLoadBDtoCMDTable();
		});
		menu->addAction(m_ActNew.get());
		menu->addAction(m_ActDel.get());
		menu->addAction(m_ActChange.get());
		menu->addSeparator();
		menu->exec(QCursor::pos());

	}
	else if (ui.tv_SSHErrTab->underMouse())
	{
		QMenu *menu = new QMenu(ui.tv_CMDTable);
		QScopedPointer<QAction> m_ClearJzagErr(new QAction("清空错误信息", ui.tv_CMDTable));
		connect(m_ClearJzagErr.get(), &QAction::triggered, this, &SSHWindow::slotClearJzagErrTab);
		menu->addAction(m_ClearJzagErr.get());
		menu->exec(QCursor::pos());
	}
}

/*初始化图标等信息*/
void SSHWindow::Init()
{
	this->setWindowTitle("SSH客户端");
	ProStatusInitMap = QPixmap(":/img/问号.png");//设置的图片的方式 还可以通过这种方式来做
	ProStatusOnlineMap.load(":/img/在线.png");
	proStatusOutlineMap.load(":/img/离线.png");
	CreatDB();//创造数据库
	InitTapWidget();//TapWidget初始化
	InitTable();//命令表格初始化
	InitJzagErrTable();//Jzag错误信息表格初始化
	InitCMDWidget();//CMD操作窗口初始化
	ui.splitter->setStretchFactor(0, 85);//设置分割比例
	ui.splitter->setStretchFactor(1, 15);
}

/* 在状态窗口 捕捉回车键 暂且没有调用 */
bool SSHWindow::eventFilter(QObject * object, QEvent * event)
{
	if (object == qe_SSHText[m_CurrentSSHIndex] && event->type() == QEvent::KeyPress)
	{
		QKeyEvent *e = static_cast<QKeyEvent *>(event);//！！转换无效 加头文件Qevent.h
		if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return)//步骤三
		{
			qDebug() << "捕捉到回车键  " << m_CurrentSSHIndex;
			return true;
		}
	}
	return QWidget::eventFilter(object, event);
}

/*改变连接图标-------这个函数调用 是连接成功和连接错误才会调用  断开连接不调用*/
void SSHWindow::slotConnectStateChanged(bool bState, QString Err, int SSHIndex)
{
	m_bConnectState[SSHIndex] = bState;
	if (m_bConnectState[SSHIndex]) {

		qb_SSHConnet[SSHIndex]->setText("断开");
		ui.tw_SSHTabWidget->setTabIcon(SSHIndex, QIcon(":/img/在线.png"));
	}
	else
	{
		qb_SSHConnet[SSHIndex]->setText("连接");
		ui.tw_SSHTabWidget->setTabIcon(SSHIndex, QIcon(":/img/离线.png"));
		qe_SSHText[SSHIndex]->append(Err);//#文本追加（不管光标位置)
		qe_SSHText[SSHIndex]->moveCursor(QTextCursor::End);//移动光标到结尾
	}
}

/*发送命令*/
void SSHWindow::slotSshSendCmd()
{
	if (QObject::sender() != nullptr)//如果不是 删除按钮调用的删除函数  那么btnName就为"" 如果
	{
		qDebug() << "发送按钮objectName " << QObject::sender()->objectName();
	}

	qDebug() << "发送按钮被调用 " << m_CurrentSSHIndex;
	if (m_bConnectState[m_CurrentSSHIndex]) {
		QString strCmd = qe_SSHCmdLine[m_CurrentSSHIndex]->text();
		if (strCmd == "ls" || strCmd == "ls -l")
		{
			strCmd = qe_SSHCmdLine[m_CurrentSSHIndex]->text() + " --color=never";
		}
		strCmd += "\n"; //添加回车
		/*！放到这个地方是因为 Qt::UniqueConnection不适用于lambda*/
		QMetaObject::invokeMethod(m_sshSocket[m_CurrentSSHIndex], "slotSend", Qt::QueuedConnection, Q_ARG(QString, strCmd), Q_ARG(int, m_CurrentSSHIndex));

	}
	else
	{
		QMessageBox::critical(this, "发送命令错误", "未连接，连接后再试");
	}
	qe_SSHCmdLine[m_CurrentSSHIndex]->clear();
}

/*0.8 点击链接按钮调用 */
void SSHWindow::slotSshConnect()
{
	qDebug() << "连接按钮被调用  " << m_CurrentSSHIndex;
	if (!m_bConnectState[m_CurrentSSHIndex]) {
		if (g_ConnetIP == "")
		{
			QMessageBox::critical(this, "IP未选择", "IP未选择");
			return;
		}
		qb_SSHConnet[m_CurrentSSHIndex]->hide();
		qe_SSHText[m_CurrentSSHIndex]->setText("正在连接请稍候...");
		qe_SSHText[m_CurrentSSHIndex]->show();
		qe_SSHText[m_CurrentSSHIndex]->setReadOnly(true);
		InitForSSHTextRK();
		//	connect(qe_SSHText[m_CurrentSSHIndex], SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotQTextLineRK(QPoint)));
		qe_SSHCmdLine[m_CurrentSSHIndex]->show();
		qb_SSHSend[m_CurrentSSHIndex]->show();
		qgl_NewTapLay[m_CurrentSSHIndex]->addWidget(qe_SSHText[m_CurrentSSHIndex], 0, 0, 3, 5);//坐标00 3行5列
		qgl_NewTapLay[m_CurrentSSHIndex]->addWidget(qe_SSHCmdLine[m_CurrentSSHIndex], 4, 0, 1, 4);
		qgl_NewTapLay[m_CurrentSSHIndex]->addWidget(qb_SSHSend[m_CurrentSSHIndex], 4, 4, 1, 1);
		ui.tw_SSHTabWidget->currentWidget()->setLayout(qgl_NewTapLay[m_CurrentSSHIndex]);
		m_sshSocket[m_CurrentSSHIndex]->m_SSHIndex = m_CurrentSSHIndex;//给SSH链接线程的序号，赋值。
		/*创建 并启动线程 启动连接线程!!在这要注意，即使你创建线程，什么的都没问题，
		但是也是要注意，如果你在主线程中，直接调用子线程中的函数的话，还是让这个函数，运行在主线程中。。 到时候还是有问题。*/
		QMetaObject::invokeMethod(m_sshSocket[m_CurrentSSHIndex], "slotCreateConnection", Qt::QueuedConnection);//把断开操作发送到，SSH线程中，避免跨线程出现调用套接字
		/*根据连接的状态----改变图标文字 -连接 断开*/
		connect(m_sshSocket[m_CurrentSSHIndex], SIGNAL(sigConnectStateChanged(bool, QString, int)), this, SLOT(slotConnectStateChanged(bool, QString, int)));
		/* 把从终端读到的数据放到文档编辑框中 */
		connect(m_sshSocket[m_CurrentSSHIndex], SIGNAL(sigDataArrived(QString, QString, int, int)), this, SLOT(slotDataArrived(QString, QString, int, int)));
	}
	else//！！删掉
	{
		emit sigDisconnected();//断开连接
	}
}

/*SSH发送命令错误处理*/
void SSHWindow::slotSshSendERR(int Err)
{
	if (Err == -1)
	{
		QMessageBox::critical(this, "发送命令错误", "未连接，连接后再试");
	}
}

/*添加tapwidget---都是往最后的位置插*/
void SSHWindow::slotAddTapwindow()
{
	if (g_ConnetIP == "")
	{
		QMessageBox::critical(this, "SSH连接错误", "IP未选择");
		return;
	}
	m_bConnectState.push_back(false);
	m_pThread.push_back(new QThread());
	tw_NewTapWidget.push_back(new QWidget());
	qb_SSHConnet.push_back(new QPushButton("连接", tw_NewTapWidget[m_SSHNum]));//把按钮放在子窗口上
	qb_SSHConnet[m_SSHNum]->setObjectName(QString::number(m_SSHNum));

	ui.tw_SSHTabWidget->insertTab(ui.tw_SSHTabWidget->count() - 1, tw_NewTapWidget[m_SSHNum], g_ConnetIP);//增加子窗口
	m_sshSocket.push_back(new CConnectionForSshClient(g_ConnetIP, g_DevPort.toInt(), g_UserName, g_UserWord));//!在新建tab窗口的时候，就创建ssh套接字
	//connect(m_pThread[m_SSHNum], SIGNAL(finished()), this, SLOT(slottest()));//线程退出信号
	m_sshSocket[m_SSHNum]->moveToThread(m_pThread[m_SSHNum]);
	m_pThread[m_SSHNum]->start();//添加窗口后，就启动线程
	qDebug() << "启动线程 ID：" << m_pThread[m_SSHNum]->currentThreadId();
	connect(qb_SSHConnet[m_SSHNum], SIGNAL(clicked()), this, SLOT(slotSshConnect()));//连接按钮
	//connect(m_pThread[m_SSHNum], SIGNAL(finished()), m_sshSocket[m_SSHNum], SLOT(slotThreadFinished()));//如果线程结束 调用析构回收线程资源以及这个对象的资源
	connect(m_pThread[m_SSHNum], SIGNAL(finished()), m_sshSocket[m_SSHNum], SLOT(slotDisconnected()));//如果线程结束 ,断开连接，然后释放 调用析构回收线程资源以及这个对象的资源
	ui.tw_SSHTabWidget->setTabIcon(m_SSHNum, QIcon(":/img/离线.png"));//子窗口图标
	qgl_NewTapLay.push_back(new QGridLayout());//每个子窗口的布局
	qe_SSHCmdLine.push_back(new QLineEdit("在此输入命令", tw_NewTapWidget[m_SSHNum]));//命令发送输入框
	qe_SSHCmdLine[m_SSHNum]->hide();//命令发送输入框隐藏
	qe_SSHText.push_back(new QTextEdit(tw_NewTapWidget[m_SSHNum]));//每个子窗口的SSH输出窗口
	qe_SSHText[m_SSHNum]->hide();//SSH输出窗口隐藏
	qb_SSHSend.push_back(new QPushButton("发送", tw_NewTapWidget[m_SSHNum]));//发送按钮
	qe_SSHText[m_SSHNum]->document()->setMaximumBlockCount(1000);//内容限制1000行
	qb_SSHSend[m_SSHNum]->setObjectName(QString::number(m_SSHNum));
	qb_SSHSend[m_SSHNum]->hide();//SSH输出窗口隐藏
	connect(qb_SSHSend[m_SSHNum], SIGNAL(clicked()), this, SLOT(slotSshSendCmd()));//发送命令按钮
	qDebug() << "m_SSHNum = " << m_SSHNum;
	m_SSHNum++;
}

/*TapWidget初始化*/
void SSHWindow::InitTapWidget()
{
	ui.tw_SSHTabWidget->tabBar()->setTabsClosable(1);//为每个标签添加close按钮
	connect(ui.tw_SSHTabWidget->tabBar(), SIGNAL(tabCloseRequested(int)), this, SLOT(slotCloseTab(int)));
	connect(ui.tw_SSHTabWidget->tabBar(), SIGNAL(tabBarClicked(int)), this, SLOT(slotGetSSHTabIndex(int)));
	/*创建第一个tap窗口*/
	QWidget *tw_NewTapWidget = new QWidget();
	ui.tw_SSHTabWidget->insertTab(0, tw_NewTapWidget, "");

	/*tab窗口中的加号*/
	qb_SSHWidgetadd = new QPushButton("+");//tapwidget的加号+
	ui.tw_SSHTabWidget->tabBar()->setTabButton(0, QTabBar::RightSide, qb_SSHWidgetadd);//tabwidget的首页的加号
	connect(qb_SSHWidgetadd, SIGNAL(clicked()), this, SLOT(slotAddTapwindow()));
}

/*命令表格初始化*/
void SSHWindow::InitTable()
{
	m_CMDItemModel = new QStandardItemModel();
	ui.tv_CMDTable->setModel(m_CMDItemModel);
	m_CMDItemModel->setHorizontalHeaderLabels(QStringList() << "命令" << "描述");
	ui.tv_CMDTable->setEditTriggers(QAbstractItemView::NoEditTriggers);//不可编辑
	ui.tv_CMDTable->setSelectionBehavior(QAbstractItemView::SelectRows);	//选择行

	slotLoadBDtoCMDTable();
	connect(ui.tv_CMDTable, SIGNAL(clicked(const QModelIndex&)), this, SLOT(slotTableClicked(const QModelIndex &)));//clicked只能左键 pressed左右都行
	connect(ui.tv_CMDTable, SIGNAL(pressed(const QModelIndex&)), this, SLOT(slotTablePress(const QModelIndex &)));//clicked只能左键 pressed左右都行

}

/*CMD操作窗口初始化*/
void SSHWindow::InitCMDWidget()
{
	qw_CMDWindow = new QWidget();
	qfl_layout = new QFormLayout();
	le_CMD = new QLineEdit();
	le_CMDText = new QLineEdit();
	qb_CMDConfirm = new QPushButton("确定");
	connect(qb_CMDConfirm, SIGNAL(clicked()), this, SLOT(slotCreatCMDToDB()));
	qfl_layout->addRow("命令:", le_CMD);
	qfl_layout->addRow("描述:", le_CMDText);
	qfl_layout->addRow(qb_CMDConfirm);
	qfl_layout->setSpacing(10);
	qfl_layout->setLabelAlignment(Qt::AlignLeft);//设置标签的对齐方式
	qw_CMDWindow->setLayout(qfl_layout);
	qw_CMDWindow->setWindowTitle("CMD操作");
}

/*Jzag错误信息表格初始化*/
void SSHWindow::InitJzagErrTable()
{
	m_JzagErrItemModel = new QStandardItemModel();
	ui.tv_SSHErrTab->setModel(m_JzagErrItemModel);
	ui.tv_SSHErrTab->setWordWrap(true);
	m_JzagErrItemModel->setHorizontalHeaderLabels(QStringList() << "错误号" << "错误信息" << "解决办法" << "解释" << "功能");
	ui.tv_SSHErrTab->setEditTriggers(QAbstractItemView::NoEditTriggers);//不可编辑
	ui.tv_SSHErrTab->setSelectionBehavior(QAbstractItemView::SelectRows);	//选择行
	ui.tv_SSHErrTab->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);//自适应列宽...但是太长了之后，内容会省略成省略号
	ui.tv_SSHErrTab->setWordWrap(true); //自动换行--其实就是识别内部的换行符
}

/*捕捉回车键，发送命令*/
void SSHWindow::keyReleaseEvent(QKeyEvent * e)
{
	if (e->key() == Qt::Key_Return)//回车键
	{
		qDebug() << "回车发送被调用 " << m_CurrentSSHIndex;
		if (m_bConnectState[m_CurrentSSHIndex]) {
			QString strCmd = qe_SSHCmdLine[m_CurrentSSHIndex]->text();
			if (strCmd == "ls" || strCmd == "ls -l")
			{
				strCmd = qe_SSHCmdLine[m_CurrentSSHIndex]->text() + " --color=never";
			}
			strCmd += "\n"; //添加回车
			/*！放到这个地方是因为 Qt::UniqueConnection不适用于lambda*/
			QMetaObject::invokeMethod(m_sshSocket[m_CurrentSSHIndex], "slotSend", Qt::QueuedConnection, Q_ARG(QString, strCmd), Q_ARG(int, m_CurrentSSHIndex));
		}
		else
		{
			QMessageBox::critical(this, "发送命令错误", "未连接，连接后再试");
		}
		qe_SSHCmdLine[m_CurrentSSHIndex]->clear();
	}
	if (e->matches(QKeySequence::Copy))//ctrl+c
	{
		QString strCmd = (QString)3 + "\n";
		//strCmd += "\n";
		if (m_bConnectState.size() != 0 && m_bConnectState[m_CurrentSSHIndex])
		{
			QMetaObject::invokeMethod(m_sshSocket[m_CurrentSSHIndex], "slotSend", Qt::QueuedConnection, Q_ARG(QString, strCmd), Q_ARG(int, m_CurrentSSHIndex));
		}
	}
	if (e->key() == Qt::Key_Tab)//tab
	{
		QString strCmd = qe_SSHCmdLine[m_CurrentSSHIndex]->text() + (QString)9 + (QString)9 + " " + "\n";//加一个tab不行，我也不知道为什么？可能这个库也不是很好用
		if (m_bConnectState.size() != 0 && m_bConnectState[m_CurrentSSHIndex])
		{
			QMetaObject::invokeMethod(m_sshSocket[m_CurrentSSHIndex], "slotSend", Qt::QueuedConnection, Q_ARG(QString, strCmd), Q_ARG(int, m_CurrentSSHIndex));
		}
	}
}

/*SSH窗口内容右键初始化*/
void SSHWindow::InitForSSHTextRK()
{
	qe_SSHText[m_CurrentSSHIndex]->setContextMenuPolicy(Qt::CustomContextMenu);
	QMenu* stdMenu = qe_SSHText[m_CurrentSSHIndex]->createStandardContextMenu();
	QAction* clearAction = new QAction("Clear", qe_SSHText[m_CurrentSSHIndex]);
	stdMenu->addAction(clearAction);

	/*显示右键*/
	QObject::connect(qe_SSHText[m_CurrentSSHIndex], &QTextEdit::customContextMenuRequested, [=](QPoint x)
	{
		stdMenu->move(qe_SSHText[m_CurrentSSHIndex]->mapToGlobal(x));
		stdMenu->show();
	});
	/*清空ssh窗口内容*/
	QObject::connect(clearAction, &QAction::triggered, [=]()
	{
		qe_SSHText[m_CurrentSSHIndex]->clear();
	});
}

/*加载Jzag错误csv文件*/
void SSHWindow::LoadJzagErrCSV()
{
	g_JzagErrCSV.clear();
	QString fileName = "./../conf/JzagErr.csv";
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qCritical() << "程序主动退出！读取错误；JzagErr.csv " << file.errorString();
		exit(-1);
	}
	QTextStream in(&file);//Qt 文本流 
	while (!in.atEnd())
	{
		JzagErr jzagerr;
		QString FileLine = in.readLine();//读取一行
		QStringList m_CSVLineData = FileLine.split(",");
		// 从字符串中有","的地方将其分为多个子字符串，QString::SkipEmptyParts表示跳过空的条目 这样即使csv中不按标准来，用了空格也可过滤掉
		jzagerr.Solution = m_CSVLineData[1];
		jzagerr.Note = m_CSVLineData[2];
		jzagerr.FucSection = m_CSVLineData[3];
		jzagerr.IsRepeatInfo = m_CSVLineData[4].toInt();
		g_JzagErrCSV[m_CSVLineData[0]] = jzagerr;
	}
	file.close();
}

/*创建新的命令到数据库*/
void SSHWindow::slotCreatCMDToDB()
{
	if (ISCreat)//创建-新CMD
	{
		if (le_CMD->text() == "" || le_CMDText->text() == "")
		{
			QMessageBox::critical(qw_CMDWindow, "错误", "内容不能为空");
			return;
		}
		OpenDB();
		QSqlQuery query(m_CMDListDB);
		query.prepare("insert into CMDList (CMD, CMDtext) values (?,?)");//位置绑定的方式--插入数据
		query.addBindValue(le_CMD->text());
		query.addBindValue(le_CMDText->text());
		query.exec();
		CloseDB();
		slotLoadBDtoCMDTable();
		qw_CMDWindow->hide();
	}
	else//修改-旧CMD
	{
		if (le_CMD->text() == "" || le_CMDText->text() == "")
		{
			QMessageBox::critical(qw_CMDWindow, "错误", "内容不能为空");
			return;
		}
		OpenDB();
		QSqlQuery query(m_CMDListDB);
		query.prepare("update  CMDList set CMD = ?, CMDtext = ? where CMD = ? and CMDtext = ?");
		query.addBindValue(le_CMD->text());
		query.addBindValue(le_CMDText->text());
		query.addBindValue(m_CMD);
		query.addBindValue(m_CMDtext);
		query.exec();
		CloseDB();
		slotLoadBDtoCMDTable();
		qw_CMDWindow->hide();
	}
}

/*处理Jzag错误信息，填充到错误信息表格中*/
void SSHWindow::slotPraseJzagErrMsg(QString Err)
{
	//qDebug() << Err;//[20220805 16:31:14-ERROR] E10048:3 mbs reg=256 chid=0 link=27
	//return;
	int Pos = Err.indexOf("ERROR", 1);
	//有时候有可能是err和info信息一块过来
	QString ErrNum = Err.mid(Pos + 6, Err.indexOf(":", Pos) - Pos - 6);
	Err = Err.mid(Err.indexOf(":", Pos) + 1);//错误信息，去掉时间和错误号
	ErrNum = ErrNum.simplified();//去掉收尾空格
	if (!g_JzagErrCSV[ErrNum].IsAdd)
	{
		m_itemList.clear();
		if (g_JzagErrCSV[ErrNum].IsRepeatInfo)//如果是重复数据，就得每次去添加
		{
			if (!g_JzagErrCSV[ErrNum].RepeatErrInfo.contains(Err))//如果这条错误已经被记录了，就不会再次添加的表格中
			{
				//qDebug() << "重复错误信息，添加  " << Err;
				m_itemList.append(new QStandardItem(ErrNum));
				m_itemList.append(new QStandardItem(Err));
				m_itemList.append(new QStandardItem(g_JzagErrCSV[ErrNum].Solution));
				m_itemList.append(new QStandardItem(g_JzagErrCSV[ErrNum].Note));
				m_itemList.append(new QStandardItem(g_JzagErrCSV[ErrNum].FucSection));
				m_JzagErrItemModel->appendRow(m_itemList);
				g_JzagErrCSV[ErrNum].RepeatErrInfo << Err;//如果是需要重复的错误信息，把信息添加到重复信息列表中

			}
		}
		else
		{
			m_itemList.append(new QStandardItem(ErrNum));
			m_itemList.append(new QStandardItem(Err));
			m_itemList.append(new QStandardItem(g_JzagErrCSV[ErrNum].Solution));
			m_itemList.append(new QStandardItem(g_JzagErrCSV[ErrNum].Note));
			m_itemList.append(new QStandardItem(g_JzagErrCSV[ErrNum].FucSection));
			m_JzagErrItemModel->appendRow(m_itemList);
			g_JzagErrCSV[ErrNum].IsAdd = true;
		}
	}
}

/*清空jzag错误信息表格*/
void SSHWindow::slotClearJzagErrTab()
{
	m_JzagErrItemModel->clear();
	for (QMap<QString, JzagErr>::iterator it = g_JzagErrCSV.begin(); it != g_JzagErrCSV.end(); it++)
	{
		it->IsAdd = false;
		it->RepeatErrInfo.clear();
	}
	m_JzagErrItemModel->setHorizontalHeaderLabels(QStringList() << "错误号" << "错误信息" << "解决办法" << "解释" << "功能");
}

/*删除Tab窗口*/
void SSHWindow::slotCloseTab(int SSHIndex)
{
	/*断开连接 */
//	delete m_sshSocket[SSHIndex];!!!不能直接在这删除，因为m_sshSocket整个类已经移动到另外一个线程中了，发送信号过去
	if (m_pThread[SSHIndex]->isRunning())
	{
		m_pThread[SSHIndex]->exit(0);
		/*终止线程的执行。 线程可以立即终止，也可以不立即终止，这取决于操作系统的调度策略。 在terminate()之后使用QThread::wait()来确定。
		警告:此函数是危险的，不鼓励使用。 线程可以在其代码路径的任何位置终止。 线程可以在修改数据时终止。
		线程没有机会在自己之后进行清理，解锁任何持有的互斥对象，等等。 简而言之，只有在绝对必要时才使用此函数。		 */
		if (m_pThread[SSHIndex]->wait())//断开连接后面一定要有个wait回收资源，要不然软件崩溃！在wait执行过程之中调用调用析构
		{
			qDebug() << m_pThread[SSHIndex]->currentThreadId() << " 线程退出成功！";
		}
	}
	delete m_pThread[SSHIndex];
	m_pThread.erase(m_pThread.begin() + SSHIndex);//结束线程
	m_sshSocket.erase(m_sshSocket.begin() + SSHIndex);
	for (size_t i = 0; i < m_sshSocket.size(); i++)
	{
		m_sshSocket[i]->m_SSHIndex = i;
	}

	delete qe_SSHText[SSHIndex];
	delete qb_SSHSend[SSHIndex];
	delete qgl_NewTapLay[SSHIndex];
	delete qe_SSHCmdLine[SSHIndex];
	qe_SSHText.erase(qe_SSHText.begin() + SSHIndex);
	qb_SSHSend.erase(qb_SSHSend.begin() + SSHIndex);
	qgl_NewTapLay.erase(qgl_NewTapLay.begin() + SSHIndex);
	qe_SSHCmdLine.erase(qe_SSHCmdLine.begin() + SSHIndex);
	m_bConnectState.erase(m_bConnectState.begin() + SSHIndex);
	ui.tw_SSHTabWidget->removeTab(SSHIndex);
	qb_SSHConnet.erase(qb_SSHConnet.begin() + SSHIndex);
	tw_NewTapWidget.erase(tw_NewTapWidget.begin() + SSHIndex);


	m_SSHNum--;
}

/*接受SHH发来的信息*/
void SSHWindow::slotDataArrived(QString strMsg, QString strIp, int nPort, int SSHIndex)
{
	//qDebug() << strMsg;
	/*下面对于数据处理，看起来是很复杂，并且没有必要，但实际上是有很多必要的
	据我观察，QSSH库给我数据分三种，
	纯\r\n ，
	以#结束的如“root@EC2022BR:/mnt/nandflash/jzhs/bin# ”，还
	有就是最正常的以\r\n 结尾的cd /mnt/nandflash/jzhs/bin\\r\\n\"
	还有上次没有发\r\n这次开头就发
	如果不做处理的话，就会造成数据分散。比如一个句子，QSSH会打散发过来。所以需要处理。
	还有一种情况，不同的句子之前会出现很多的换行，这是因为QSSH发给你的句子中，已经有了换行符（可能是系统给你添加的），然后还给你发送一个换行符。 这样就会造成多个换行
	*/
	//qDebug() << "rec：" << strMsg << "size " << strMsg.size();
	Q_UNUSED(strIp);//Q_UNUSED() 没有实质性的作用，用来避免编译器警告--就是没有用strIp这个参数 正常来说，会警告
	Q_UNUSED(nPort);
	SSHRecStr += strMsg;
	//主要还是防止ssh有时候只发来一个字符，只有一个字符或者零星几个字符的话，是没有\r\n或者# 的，所以退出函数，然后继续累加字符串，等到字符串以\r\n结尾,在对字符串做处理
	if (!(SSHRecStr.endsWith("\r\n") || strMsg.endsWith("# ")))
	{
		return;
	}
	QStringList SHHTextList = SSHRecStr.split("\r\n");
	for (size_t i = 0; i < SHHTextList.size(); i++)
	{
		if (SHHTextList[i].isEmpty())
		{
			continue;
		}
		qe_SSHText[SSHIndex]->append(SHHTextList[i]);//#文本追加（不管光标位置)
		qe_SSHText[SSHIndex]->moveCursor(QTextCursor::End);//移动光标到结尾
		if (SSHRecStr.contains("ERROR"))//!!判断err第一次出现的位置
		{
			emit sigJzagErrmsg(SSHRecStr);
		}
	}
	SSHRecStr.clear();
	SHHTextList.clear();
}

/*测试槽函数---不带参*/
void SSHWindow::slottest()
{
	qDebug() << "SSHWindow::slottest()被调用！";
}

/*测试槽函数---带参*/
void SSHWindow::slottest(int SSHIndex)
{

}

/*测试函数--无实际意义*/
void SSHWindow::slottest(QString CMD, int SSHIndex)
{
	qDebug() << "当前选中的窗口，序号 " << SSHIndex << "cmd " << CMD;
}

/*获取当前的SSH子窗口的序号*/
void SSHWindow::slotGetSSHTabIndex(int SSHIndex)
{
	qDebug() << "当前选中的窗口，序号 " << SSHIndex;
	m_CurrentSSHIndex = SSHIndex;
}

/*加载命令数据库文件到表格控件*/
void SSHWindow::slotLoadBDtoCMDTable()
{
	OpenDB();//打来数据库
	QSqlQuery query(m_CMDListDB);
	m_CMDItemModel->clear();
	m_CMDItemModel->setHorizontalHeaderLabels(QStringList() << "命令" << "描述");
	query.exec("create table CMDList(CMD text, CMDtext text)");//创建表,如果表存在了，就不创建
	query.exec("select * from CMDList");
	while (query.next())
	{
		//qDebug() << query.value(0).toString() << query.value(1).toString();
		m_itemList.append(new QStandardItem(query.value(0).toString()));
		m_itemList.append(new QStandardItem(query.value(1).toString()));
		m_CMDItemModel->appendRow(m_itemList);
		m_itemList.clear();
	}
	ui.tv_CMDTable->setSelectionBehavior(QAbstractItemView::SelectRows);	//选择行
	CloseDB();
}

/*CMD表格被左击*/
void SSHWindow::slotTableClicked(const QModelIndex &index)
{
	QModelIndex indextemp = m_CMDItemModel->index(index.row(), 0);
	m_CMD = m_CMDItemModel->data(indextemp).toString();
	indextemp = m_CMDItemModel->index(index.row(), 1);
	m_CMDtext = m_CMDItemModel->data(indextemp).toString();

	qDebug() << "CMD表格被调用 " << m_CurrentSSHIndex;
	if (m_bConnectState.size() > m_CurrentSSHIndex && m_bConnectState[m_CurrentSSHIndex]) {
		if (m_CMD == "./jzag")
		{
			emit sigStartJzag();
		}
		else if (m_CMD == "ls" || m_CMD == "ls -l")
		{
			m_CMD = m_CMD + " --color=never";
		}
		else if (m_CMD == "reboot")
		{
			int ret = QMessageBox::question(this, "警告", "是否重启", QMessageBox::Yes | QMessageBox::No);
			if (ret == QMessageBox::No)
			{
				return;
			}
		}
		m_CMD += "\n"; //添加回车
		/*！放到这个地方是因为 Qt::UniqueConnection不适用于lambda*/
		QMetaObject::invokeMethod(m_sshSocket[m_CurrentSSHIndex], "slotSend", Qt::QueuedConnection, Q_ARG(QString, m_CMD), Q_ARG(int, m_CurrentSSHIndex));
	}
	else
	{
		QMessageBox::critical(this, "发送命令错误", "未连接，连接后再试");
	}
}

/*CMD表格被点击*/
void SSHWindow::slotTablePress(const QModelIndex & index)
{
	QModelIndex indextemp = m_CMDItemModel->index(index.row(), 0);
	m_CMD = m_CMDItemModel->data(indextemp).toString();
	indextemp = m_CMDItemModel->index(index.row(), 1);
	m_CMDtext = m_CMDItemModel->data(indextemp).toString();
}

#include "SshClient.h"
#include <QDebug>
#include "ssh_global.h"
#include "sftpchannel.h"
#include "sftpdefs.h"
#include "sftpfilesystemmodel.h"
#include "sshconnection.h"
#include "sshconnectionmanager.h"
#include "ssherrors.h"
#include "sshkeygenerator.h"
#include "sshpseudoterminal.h"
#include "sshremoteprocess.h"
#include "sshremoteprocessrunner.h"
#include "sshdirecttcpiptunnel.h"
#include "sshkeycreationdialog.h"
#include "sshhostkeydatabase.h"
#include "Log.h"

#pragma execution_character_set("utf-8")

extern QString DevIP, DevPort, FTPType, DevWord;//用于采集登录信息


CConnectionForSshClient::CConnectionForSshClient()
{
	//m_argParameters.password = "123";
}

/*退出当前线程函数*/
void CConnectionForSshClient::unInit()
{
}

CConnectionForSshClient::CConnectionForSshClient(QString m_strIp, int m_nPort, QString m_strUser, QString m_strPwd)
{
	this->m_strIp = m_strIp;
	m_argParameters.setHost(m_strIp);
	m_argParameters.setUserName(m_strUser);
	m_argParameters.setPassword(m_strPwd);
	m_argParameters.setPort(m_nPort);
	m_argParameters.timeout = 30;
	m_argParameters.authenticationType = QSsh::SshConnectionParameters::AuthenticationTypePassword;

}

/* 1 连接初始化函数  启动线程 */
void CConnectionForSshClient::init()
{
	//m_pThread = new QThread();
	//connect(m_pThread, SIGNAL(finished()), this, SLOT(slotThreadFinished()));//如果线程结束 回收线程资源以及这个对象的资源
	//this->moveToThread(m_pThread);//m_pThread是在主线程中申请的对象，属于主线程，依赖于主线程，但是使用moveToThread，可以让线程依附到次线程中，也就是依付自己！这样的话，槽函数就在次线程中运行
	//m_pThread->start();//启动线程--qt线程的自带函数
	/* 这样做的目的---  假设槽函数很费时间 如果槽函数在主线程，那么就会导致 主线程卡死  有可能会导致界面卡死 */
	//之后的逻辑都得通过信号和槽接通
	//qDebug() << "启动线程 ID：" << m_pThread->currentThreadId();

	slotCreateConnection(); //连接

	//connect(m_pTimer, SIGNAL(timeout()), this, SLOT(soltExitTheThread()));
}

/*析构函数，回收套接字资源*/
CConnectionForSshClient::~CConnectionForSshClient()
{
	qDebug() << "析构函数，回收套接字资源";
	if (nullptr != m_pSshSocket) {
		m_pSshSocket->disconnectFromHost();
		delete m_pSshSocket;
		m_pSshSocket = nullptr;
	}
}

/*ip用来检测是否0 0退出  命令是放在数组中的*/
void CConnectionForSshClient::slotSendByQByteArray(QString strIpPort, QByteArray arrMsg)
{
	if (m_bConnected) {
		m_shell->write(arrMsg);
	}
	else {
		qDebug() << "CConnectionForSshClient::send(QString strMessage) 发送失败 未建立连接:" << getIpPort();
	}
}

/* 3 检查链接 并连接   */
void CConnectionForSshClient::slotCreateConnection()
{
	qDebug() << "CConnectionForSshClient::slotCreateConnection检查连接";
	if (true == m_bConnected)
		return;

	if (nullptr == m_pSshSocket) {
		m_pSshSocket = new QSsh::SshConnection(m_argParameters);//创建ssh连接套接字
		/*连接成功  */
		connect(m_pSshSocket, SIGNAL(connected()), SLOT(slotConnected()));
		/*连接失败 */
		connect(m_pSshSocket, SIGNAL(error(QSsh::SshError)), SLOT(slotSshConnectError(QSsh::SshError)));//连接错误--打印错误代码
	}
	m_pSshSocket->connectToHost();//库函数开始连接 
	qDebug() << "m_sshSocket  " << m_pSshSocket;
	/*出现！！QObject::startTimer: Timers can only be used with threads started with QThread*/
	qDebug() << "CConnectionForSshClient::slotCreateConnection() 以ssh方式 尝试连接:" << getIpPort();
}

/* 4 连接成功之后  拿到关于该设备的套接字 */
void CConnectionForSshClient::slotConnected()
{
	qDebug() << "CConnectionForSshClient::slotConnected ssh已连接到:" << getIpPort();

	m_shell = m_pSshSocket->createRemoteShell();//m_shell 估计是连接到某个设备的套接字
	//信号是库提供的！ 和设备连接后，发送信号；槽函数把能发送信息标志位置1
	connect(m_shell.data(), &QSsh::SshRemoteProcess::started, [&]() {
		this->m_bSendAble = true;
		qInfo() << "CConnectionForSshClient::slotShellStart Shell已连接:" << getIpPort();
	});
	//connect(m_shell.data(), SIGNAL(started()), this, SLOT(slottest()));//链接成功

	/*SSH错误打印*/
	connect(m_shell.data(), &QSsh::SshRemoteProcess::readyReadStandardError, [&]() {
		qCritical() << "CConnectionForSshClient::slotShellError Shell发生错误:" << getIpPort();
	});
	//在textline中打印终端发来的文字
	connect(m_shell.get(), &QSsh::SshRemoteProcess::readyReadStandardOutput, [&]() {
		QByteArray byteRecv = m_shell->readAllStandardOutput();
		QString strRecv = QString::fromUtf8(byteRecv);
		if (!strRecv.isEmpty()) //过滤空行
			emit sigDataArrived(strRecv, m_strIp, m_nPort, m_SSHIndex);//发送到指定的窗口
	});


	m_shell.data()->start();
	m_bConnected = true;
	emit sigConnectStateChanged(m_bConnected, "ok", m_SSHIndex);//发送链接成功状态。
}

/*断开连接 */
void CConnectionForSshClient::slotDisconnected()
{
	qDebug() << "调用槽函数，去断开连接";
	if (m_pSshSocket != nullptr)
	{
		m_pSshSocket->disconnectFromHost();
	}
	slotThreadFinished();//先断开链接，后释放资源。
}

/*线程结束 回收资源*/
void CConnectionForSshClient::slotThreadFinished()
{
	qDebug() << "线程结束发送信号,调用槽函数";
	this->deleteLater();//并没有将对象立即销毁，当控件返回到事件循环时，该对象将被删除。 QCoreApplication::exec()这就是事件循环，我理解为主UI刷新
}

/*SSH错误具体信息---！改成错误提示框*/
void CConnectionForSshClient::slotSshConnectError(QSsh::SshError sshError)
{
	QStringList SSHErr = { "ConnectError SshNoError" ,"ConnectError SshSocketError","ConnectError SshTimeoutError","ConnectError SshProtocolError",
	"ConnectError SshHostKeyError","ConnectError SshKeyFileError","ConnectError SshAuthenticationError","ConnectError SshClosedByServerError",
	"ConnectError SshInternalError" };
	m_bSendAble = false;
	m_bConnected = false;
	emit sigConnectStateChanged(m_bConnected, SSHErr[sshError], m_SSHIndex);//发送链接失败状态；
}

/*退出当前线程函数*/
void CConnectionForSshClient::soltExitTheThread()
{
}

/*测试槽函数*/
void CConnectionForSshClient::slottest()
{
	qInfo() << "调用CConnectionForSshClient::test()";
}

/* 发送命令 */
void CConnectionForSshClient::slotSend(QString CMD, int SSHIndex)
{
	//qDebug() << "CConnectionForSshClient::CMD 被调用   "  << CMD;
	if (m_bConnected && m_bSendAble) {
		m_shell->write(CMD.toLatin1().data());
	}
	else {
		qDebug() << "CConnectionForSshClient::send() ssh未连接 或 shell未连接:" << getIpPort();
	}
}



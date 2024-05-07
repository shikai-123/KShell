#pragma once
#include <sshconnection.h>
#include <sshremoteprocess.h>
#include <sftpchannel.h>
#include <QTimer>
#include <QHostAddress>
#include <QThread>
#include <qwidget.h>
#include <qaction.h>
/*主SSH线程，与SSHWIndow搭配使用*/
class  CConnectionForSshClient : public QObject
{
	Q_OBJECT
public:
	CConnectionForSshClient();//构造函数声明了 必须要实现 
	CConnectionForSshClient(QString m_strIp, int m_nPort, QString m_strUser, QString m_strPwd);
	void init();
	void unInit();

	~CConnectionForSshClient();

	//QThread *m_pThread = nullptr;

	bool m_bConnected = false;//0:未连接 1:已连接
	bool m_bIsFirstConnect = true;//未用！
	bool m_bSendAble = false;//0不能发送 1能发送


	QString m_strIp = "";
	int m_nPort = 22;

	QSsh::SshConnectionParameters m_argParameters;
	QSsh::SshConnection *m_pSshSocket = nullptr;
	QSharedPointer<QSsh::SshRemoteProcess> m_shell;
	QString getIpPort() { return m_strIp + ":" + QString::number(m_nPort); }
	int m_SSHIndex;//这个赋值在点击链接按钮，用当前的子窗口序号赋值，以及在删除子窗口的时候赋值



public slots:
	void slotSend(QString CMD, int SSHIndex);//发送命令
	void slotSendByQByteArray(QString strIpPort, QByteArray arrMsg);//未调用
	void slotDisconnected();
	void slotCreateConnection();
	void slotConnected();
	void slotThreadFinished();
	void slotSshConnectError(QSsh::SshError sshError);
	void soltExitTheThread();
	void slottest();

signals:
	void sigConnectStateChanged(bool bState,QString Err, int SSHindex);//当前的链接状态
	void sigDataArrived(QString strMsg, QString strIp, int nPort, int SSHindex);


};

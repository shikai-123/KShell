#include "CJQConf.h"
#include "FTPTreeList.h"
#include "FTPTreeWork.h"
#pragma execution_character_set("utf-8")
extern QString g_ProjectName, g_ElecRoomID, g_DevIndex, g_DevIP, g_ConnetIP, g_DevPort, g_UserName, g_UserWord, g_UserListNote, g_DevName, g_ProjectName, g_FTPHead;//用于采集登录信息
extern FTPTreeList * g_FTPTreeList;
extern FTPTreeWork * g_FTPTreeWork;
extern  CJQConf *g_CJQConf;
extern QString g_SSHFTPJzhsDefaultPath;
extern QMap<QString, QString> g_ProjectNameCSV;
extern bool g_IsSHHTogetherFTP;
QString g_CJQfileLocalPath;//CJQ文件所在的本地地址
QString g_FTPCurrentPath;//初始目录 随着点击.. 进入不同的目录 而发生改变

/*重洗鼠标滚轮事件-啥也不干，屏蔽鼠标滚动*/
void QComboBox::wheelEvent(QWheelEvent *e)
{
	//啥也不干，屏蔽鼠标滚动
}

//啥也不干，屏蔽鼠标滚动
void QAbstractSpinBox::wheelEvent(QWheelEvent *e)
{

}

/*初始化布局*/
CJQConf::CJQConf()
{
	Init();/*初始化布局*/
}

/*初始化-各个模块*/
void CJQConf::Init()
{
	this->setWindowTitle("CJQ设置窗口");
	this->resize(1000, 800);
	/*开启支持拖拽*/
	//setAcceptDrops(true);




	//QHBoxLayout* hbly_MainWin = new QHBoxLayout();/*!滚动窗口 一定要布局  要不然 缩小窗口的时候  只是缩小显示区域 会导致setWidgetResizable无效*/
	//hbly_MainWin->addWidget(sa_ScrollArea);
	//setLayout(hbly_MainWin);
}



/*下载到本地并且读取*/
void CJQConf::slotDownToLocalAndRead()
{
	qDebug() << g_ConnetIP << g_DevPort << g_UserName << g_UserWord << g_UserListNote << g_DevName << g_ProjectName;
	if (g_ConnetIP == "")
	{
		QMessageBox::critical(this, "下载失败", "未选择IP");
		return;
	}
	int ret = QMessageBox::question(this, "下载到本地", "是否下载", QMessageBox::Yes | QMessageBox::No);

	if (ret == QMessageBox::Yes)
	{
		//改成信号就好了 要不然就会卡在这里 发送信号 在work中接  然后启动这个函数 这里还是接着往下走 然后下载进度发送信号 下载完成发送信号（这种做饭 就和当初qssh库的做法 差不多）\
		而下面的这些关于 弹出框的处理 也是需要放到一个槽函数中去处理 成功和失败  都要发送不同的信号 这样不影响主进程的进行 要不然还是主进程去等待 FtpGetFile\
		的执行还是卡  所以 qt线程和信号是分不开的！！！
		QString CJQfilename = QString("CJQ%1%2%3").arg(g_ProjectNameCSV[g_ProjectName].mid(1).toInt(), 4, 16, QLatin1Char('0')).arg(g_ElecRoomID.toInt(), 2, 16, QLatin1Char('0')).arg(g_DevIndex.toInt(), 2, 16, QLatin1Char('0')).toUpper();
		CJQfilename += ".json";
		//emit sigFTPDownAndOpen(g_FTPHead + g_ConnetIP + "/" + g_SSHFTPJzhsDefaultPath + "conf/" + CJQfilename, g_UserName, g_UserWord, "../Project/" + g_ProjectName + "/" + g_DevIP + "/" + CJQfilename, false, false, true, false);

		return;
	}
	else if (ret == QMessageBox::No)
	{
	}
}

/*保存并且上传到设备*/
void CJQConf::slotSaveAndUp()
{
	if (g_ConnetIP == "")
	{
		QMessageBox::critical(this, "上传失败", "未选择IP");
		return;
	}
	int ret = QMessageBox::question(this, "上传到设备", "是否上传", QMessageBox::Yes | QMessageBox::No);

	if (ret == QMessageBox::Yes)
	{

		if (g_ProjectNameCSV.find(g_ProjectName) != g_ProjectNameCSV.end())//找到了该工程
		{
			bool ok;
			//QString CJQfilename = "CJQ" + QString::number(g_ProjectNameCSV[g_ProjectName].mid(1).toInt(),16) + QString::number(g_ElecRoomID.toInt(),16) + g_DevIndex+".json";
			QString CJQfilename = QString("CJQ%1%2%3").arg(g_ProjectNameCSV[g_ProjectName].mid(1).toInt(), 4, 16, QLatin1Char('0')).arg(g_ElecRoomID.toInt(), 2, 16, QLatin1Char('0')).arg(g_DevIndex.toInt(), 2, 16, QLatin1Char('0')).toUpper();
			CJQfilename += ".json";
			//emit sigStartFTPUP(g_FTPHead + g_ConnetIP + "/" + g_SSHFTPJzhsDefaultPath + "conf/" + CJQfilename, g_UserName, g_UserWord, g_CJQfileLocalPath, true);
		}
		else
		{
			QMessageBox::critical(this, "ERR", "该工程未在 ProjectList.cvs 文件中，请检查文件");
		}
		return;
	}
	else if (ret == QMessageBox::No)
	{
	}
}


/*上传工程*/
void CJQConf::slotUpProject()
{
	g_FTPCurrentPath = g_FTPHead + g_ConnetIP + "/" + "mnt/nandflash/";
	g_FTPTreeList->LocalFileList.clear();
	QString ProjectDir = "../Project/" + g_ProjectName + "/" + g_DevIP + "/" + "jzhs";
	QString Temp_LocalDirName;
	QStringList Temp_LocalFileList;
	g_FTPTreeList->FindFile(ProjectDir);
	Temp_LocalDirName = ProjectDir.section("/", 0, -2) + "/";//在路径中去掉你选择的目录
	Temp_LocalFileList = g_FTPTreeList->LocalFileList;
	for (size_t FileInex = 0; FileInex < g_FTPTreeList->LocalFileList.size(); FileInex++)
	{
		qDebug() << Temp_LocalFileList[FileInex].remove(0, Temp_LocalDirName.size());
		//emit g_CJQConf->sigStartFTPUP(g_FTPCurrentPath + Temp_LocalFileList[FileInex], g_UserName, g_UserWord, g_FTPTreeList->LocalFileList[FileInex], false);
	}
}

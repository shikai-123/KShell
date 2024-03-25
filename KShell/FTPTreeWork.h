#pragma once
#include <qobject.h>
#include "curl/curl.h"
#include<QTextCodec>
#include <qdebug.h>
#include <qmessagebox.h>
#include <qfileinfo.h>
#include <qmetaobject.h>
#include <curl/multi.h>
#include <iostream>
#include <qprocess.h>

class FTPTreeWork :public QObject
{
	Q_OBJECT
public:
	CURL *curl = NULL;
	CURLcode rs;
	bool IsUTF8;//!为了解决中文路径的打开以及显示，在win环境
	FTPTreeWork();
	struct curl_slist *CMDlist = nullptr;//递归删除文件夹 文件队列命令
	QStringList DeletDirList;
	QStringList DownDirList;//文件夹要下载文件夹list !QStringList是vector
	QStringList DownFileList;//文件夹要下载文件list
	QString TempDeletDirPath;//临时要删除的文件夹目录 去掉sftp:/192.168.6.38
	QString TempDownDirPath;//临时要下载的文件夹目录

	QProcess* process;
	void CurlInit();
	int CurlOptClear();
	void AddOtherListItem();//打开文件窗口的时候，加载

public slots:
	void slotOpenFTPlist();
	int slotFtpGetFile(QString FTPUrl, QString User, QString PassWord, QString LocalPath, bool IsOpenFile, bool IsDownDir, bool IsOpenInCJQConf, bool IsAddWatch);
	int slotFtpUpFile(QString FTPUrl, QString User, QString PassWord, QString LocalPath, bool IsFile);
	void slotCloseFTPConnect();
	int slotFTPCMDExec(const char* CMD);
	int slotGetFTPList(QString Path);
	void slotFTPDeletDir(QString Path);
	void slotFTPDownDir(QString RemotePath, QString TargetPath);

signals:
	void sigFTPProcess(float Process);
	void sigFileList(QStringList FileList);
	void sigFTPDownError(int Err, QString FilePath, bool IsAddWatch);
	void sigFTPUpError(int Err);
	void sigFTPListError(int Err);
	void sigFTPCMDError(int Err);
	void sigFTPDeletDirError(QString ErrType, int Err);
	void sigFTPDownDirError(QString ErrType, int Err);
	void sigFTPListRefresh();
	void sigCrulNull();
	void sigFTPStatusAlarm(bool Status);
	void sigFTPDownAndOpenError(bool Err);
	void sigOpenInCJQConf();
	void sigUPorDownProcessMsg(QString ProcessMsg);
	void sigFTPDownAndOpen(QString FTPUrl, QString User, QString PassWord, QString LocalPath, bool IsOpenFile, bool IsDownDir, bool IsOpenInCJQConf, bool IsAddWatch);
	void sigStartFTPUP(QString ftpurl, QString user, QString passwd, QString LocalPath, bool IsFile);

};


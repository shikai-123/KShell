#pragma once
#include <QObject>
#include <qthread.h>
#include <qdebug.h>
#include <qfile.h>
#include <qnetworkaccessmanager.h>
#include <qnetworkreply.h>
#include "qt_windows.h"
#include <qstringlist.h>
#include <qmetaobject.h>
#include <qxml.h>
#include <qdom.h>
#include "DataStruck.h"
#include <qfile.h>
#include "QtGui/private/qzipreader_p.h"
#include "QtGui/private/qzipwriter_p.h"
#include <qtimer.h>

class UPDate  : public QObject
{
	Q_OBJECT

public:
	UPDate();
	~UPDate();

	QNetworkAccessManager *manager;
	QNetworkReply *reply;
	QFile *myfile;

	bool m_bDownFinish;//0未结束 1结束
	static bool m_IsUpdatedEXE;//本次更新中时候是否下载了exe 0未下载 1下载了
	QString m_NetErrStr;
	bool m_HasErr = false;
	QStringList DownFileList;
	QVector<UpdateFile> RemoteUpdateInfo;
	QVector<UpdateFile> LocalUpdateInfo;

	void ReadUpdateList(QString Path, QVector<UpdateFile>& UpdateInfo);
	void MoveFiles();
	bool zipReader(QString zipPath, QString zipDir);

signals:
	void sigUpdateProcess(int Pro,int All,QString Filename);
	void sitStopTimerForUpdate();

public slots:
	int slotUPdateforList(QStringList& DownFileList);
	void slotStartUPDate();
	void slotReadyRead();
	void slotFinished();
	void slotDownloadProgress(qint64 recv_total, qint64 all_total);
	void slotNetErr(QNetworkReply::NetworkError code);
	void slotTimeToUpdate();

};

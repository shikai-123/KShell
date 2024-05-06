#pragma once

#include <QWidget>
#include"ui_NewUser.h"
#include <qcompleter.h>
#include <qsqldatabase.h>
#include <qsqlerror.h>
#include <qsqlquery.h>
#include <qmessagebox.h>
class NewUser : public QWidget
{
	Q_OBJECT

public:
	NewUser(QWidget *parent = Q_NULLPTR);
	~NewUser();
	void Init();
	Ui::NewUser ui;
	QStringList m_CSVLineData;
	QStringList m_ProjectNameList;
	QCompleter *cp_ProjectNmae;
	bool m_HasErr = false;

	void OpenDB();
signals:
	void sigDelted();
	void sigModifed(QString ProjectRegion, QString ProjectName, QString ElecRoomID, QString DevIndex, QString IP, QString N2NIP, QString Port, QString UserName, QString PassWord, QString FTPType, QString Note, \
		QString OldProjectRegion, QString OldProjectName, QString OldElecRoomID, QString OldDevIndex, QString OldIP, QString OldN2NIP, QString OldPort, QString OldUserName, QString OldPassWord, QString OldDevName, QString OldNote);
	void sigAddUserItem(QString ProjectRegion, QString ProjectName, QString ElecRoomID, QString DevIndex, \
		QString IP, QString N2NIP, QString Port, QString UserName, QString PassWord, QString FTPType, QString Note);

public slots:
	void solttest();
	void slotFillDate();
	void slotCreatProjectToDB();
	void soltDeletProjectFromDB();
	void soltModifProjectFromDB();

};

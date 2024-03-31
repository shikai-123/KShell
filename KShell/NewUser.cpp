#include "NewUser.h"
#include "HomePage.h"
#include <QSettings>
#include "DataStruck.h"
#include <qtextcodec.h>
#include <qdir.h>
#include <qmessagebox.h>
#pragma execution_character_set("utf-8")

extern HomePage* g_Home_Page;
extern QTreeWidgetItem *g_CurrentItem;
extern QString g_ProjectRegion, g_ElecRoomID, g_DevIndex, g_DevIP, g_N2NIP, g_DevPort, g_UserName, g_UserWord, g_UserListNote, g_DevName, g_ProjectName;
extern int g_WorM, g_ItemRow, g_TopLevelItemRow;
extern QSqlDatabase g_UserListDB;
extern void CheckDir(QString DirName);
extern NewUser* g_NewUser;

#ifdef _DEBUG
#define __LIBRARIES_SUB_VERSION    Debug
#else
#define __LIBRARIES_SUB_VERSION   
#endif
NewUser::NewUser(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	Init();
}

NewUser::~NewUser()
{
}

/*初始化-构造函数*/
void NewUser::Init()
{
	connect(ui.pb_Confirm, SIGNAL(clicked()), this, SLOT(slotCreatProjectToDB()));
	connect(ui.pb_Confirm, SIGNAL(clicked()), this, SLOT(soltModifProjectFromDB()));
	connect(ui.pb_Confirm, &QPushButton::clicked, [&] {
		if (!m_HasErr)//如果在操作用户信息时，没有错误，就隐藏窗口
		{
			this->hide();
		}
		m_HasErr = false;
	});
	//ReadProjectNameConf();
	QStringList RegionList;
	RegionList << "北京" << "上海";
	ui.cb_ProjectRegion->addItems(RegionList);
}

/*重命名工程文件夹  path为改名文件夹的上层目录  OldDir旧文件夹名字  NewDir新文件夹名字*/
void RenameDir(QString Path, QString OldDir, QString NewDir)
{
	QDir dir;
	dir.setPath(Path);
	//qDebug() << "old " << OldDir << " new " << NewDir;
	if (dir.rename(OldDir, NewDir))
	{
		qDebug() << "重命名成功";
	}
	else
	{
		qDebug() << "重命名失败";
		QMessageBox::critical(g_NewUser, "重命名失败", "路径：" + Path + "\n旧：" + OldDir + "\n新：" + NewDir);
	}
}

/*删除文件夹*/
void RemoveDir(QString DirName)
{
	QDir dir(DirName);
	//qDebug() << "要删除文件夹的路径" << dir.path();
	QFileInfo fi(dir.path());
	if (fi.isRoot()) {
		QMessageBox::warning(NULL, "删除错误", "怎么删到“根”目录了？是不是路径选错了？");
		return;
	}
	else
	{
		dir.removeRecursively();
	}
}



/*打开数据库*/
void NewUser::OpenDB()
{
	g_UserListDB = QSqlDatabase::database("UserList");//获取指向数据库连接
	g_UserListDB.setDatabaseName("../conf/UserList.db");//数据库的名字
	bool ok = g_UserListDB.open();//如果不存在就创建，存在就打开
	if (!ok)
	{
		qDebug() << g_UserListDB.lastError().text();//调用上一次出错的原因
		QMessageBox::critical(this, "用户列表错误", "检查数据库文件\n" + g_UserListDB.lastError().text());
		g_UserListDB.close();
		QSqlDatabase::removeDatabase("QSQLITE");
		exit(-1);
	}
}

/*右键更改数据的时候 负责填充数据到新建用户窗口*/
void NewUser::slotFillDate()
{
	qDebug() << "NewUser::slotFillDate()";
	ui.cb_FTPType->setCurrentText(g_ProjectRegion);
	ui.le_ProjectName->setText(g_ProjectName);
	ui.le_No1ID->setText(g_ElecRoomID);
	ui.le_No2ID->setText(g_DevIndex);
	ui.le_UserName->setText(g_UserName);
	ui.le_PassWord->setText(g_UserWord);
	ui.le_IP->setText(g_DevIP);
	ui.le_IP2->setText(g_N2NIP);
	ui.le_Port->setText(g_DevPort);
	ui.cb_FTPType->setCurrentText(g_DevName);
	ui.le_Note->setText(g_UserListNote);
}

/*创建新的项目到数据库*/
void NewUser::slotCreatProjectToDB()
{
	if (g_WorM == 0)
	{
		OpenDB();
		qDebug() << " NewUser::soltWriteUserListDB()";
		QString ProjectRegion = ui.cb_ProjectRegion->currentText();
		QString ProjectName = ui.le_ProjectName->text();
		QString ElecRoomID = ui.le_No1ID->text();
		QString DevIndex = ui.le_No2ID->text();
		QString UserName = ui.le_UserName->text();
		QString PassWord = ui.le_PassWord->text();
		QString IP = ui.le_IP->text();
		QString N2NIP = ui.le_IP2->text();
		QString Port = ui.le_Port->text();
		QString Note = ui.le_Note->text();
		QString DevName = ui.cb_FTPType->currentText();
		if (ProjectRegion == "" || ProjectName == "" || ElecRoomID == "" || DevIndex == "" || UserName == "" || PassWord == "" || IP == "" || Port == "" || Note == "" || DevName == "")
		{
			QMessageBox::critical(this, "错误", "内容不能为空！");
			m_HasErr = true;
			g_UserListDB.close();
			QSqlDatabase::removeDatabase("QSQLITE");
			return;
		}
		QSqlQuery query(g_UserListDB);
		//query.exec("insert into UserList (ProjectName, ElecRoomID, DevIndex,IP,Port,UserName,PassWord,DevName,Note) values('测试','1','1','192.168.6.38','22','root','nk5730','集智达','我的备注')");
		//ProjectName, ElecRoomID, DevIndex,IP,Port,UserName,PassWord,DevName,Note
		query.exec("select * from UserList");
		while (query.next())
		{
			//如果有数据库中有该工程,就不添加。我去掉了这个规则，如果测试好的话，我就把他去掉——条件中不能有n2n来判断，如果这样ip相同，n2n不相同的话，也会算一个新记录，而实际上ip相同就是一个设备，也不存在两个n2n的情况，这样就能避免输错n2n导致新建一个条目
			if (ProjectRegion == query.value(0) && ProjectName == query.value(1) && ElecRoomID == query.value(2) && DevIndex == query.value(3) && IP == query.value(4) && N2NIP == query.value(5) && Port == query.value(6)\
				&& UserName == query.value(7) && PassWord == query.value(8) && DevName == query.value(9) && Note == query.value(10))
			{
				QMessageBox::critical(this, "新建用户错误", "用户信息，数据库中已有");
				m_HasErr = true;
				g_UserListDB.close();
				QSqlDatabase::removeDatabase("QSQLITE");
				return;
			}
		}
		//query.exec("insert into UserList (ProjectName, ElecRoomID, DevIndex,IP,Port,UserName,PassWord,DevName,Note) values('测试','1','1','192.168.6.38','22','root','nk5730','集智达','我的备注')");
		query.prepare("insert into UserList (ProjectRegion, ProjectName, ElecRoomID, DevIndex,IP,N2NIP,Port,UserName,PassWord,DevName,Note) values(?,?,?,?,?,?,?,?,?,?,?)");//位置绑定的方式--插入数据
		query.addBindValue(ProjectRegion);
		query.addBindValue(ProjectName);
		query.addBindValue(ElecRoomID);
		query.addBindValue(DevIndex);
		query.addBindValue(IP);
		query.addBindValue(N2NIP);
		query.addBindValue(Port);
		query.addBindValue(UserName);
		query.addBindValue(PassWord);
		query.addBindValue(DevName);
		query.addBindValue(Note);
		query.exec();
		g_UserListDB.close();
		QSqlDatabase::removeDatabase("QSQLITE");
		sigAddUserItem(ProjectRegion, ProjectName, ElecRoomID, DevIndex, IP, N2NIP, Port, UserName, PassWord, DevName, Note);
		CheckDir("../Project/" + ProjectRegion + "/" + ProjectName + "/" + IP);
	}
	return;
}

/*从数据库中删除数据*/
void NewUser::soltDeletProjectFromDB()
{
	int ret = QMessageBox::question(this, "删除", "是否删除", QMessageBox::Yes | QMessageBox::No);
	if (ret == QMessageBox::No)
	{
		return;
	}
	OpenDB();
	QSqlQuery query(g_UserListDB);
	if (g_ProjectRegion != "" && g_ProjectName == "")//只选中父节点
	{
		query.prepare("delete from UserList where ProjectRegion = :ProjectRegion "); //名称绑定的方式 注意和一般的sql语句不同的是，在text类型的时候他不需要加 ''--如果在使用exec的纯sql语句的时候 必须加上''
		query.bindValue(":ProjectRegion", g_ProjectRegion);
		if (query.exec())
		{
			RemoveDir("./../Project/" + g_ProjectRegion);
		}
	}
	else if (g_ProjectName != "" && g_DevIP == "")//选中父节点下面的子节点
	{
		bool ExistTheProjectName = false;
		query.exec("select ProjectName from UserList");
		while (query.next())
		{
			if (ui.le_ProjectName->text() == query.value(0))//如果有数据库中有该工程名字--删除掉整个工程
			{
				ExistTheProjectName = true;
				query.clear();
				break;
			}
		}
		if (ExistTheProjectName)
		{
			/*ProjectName	ElecRoomID	DevIndex	IP	Port	UserName	PassWord	DevName	Note*/
			query.prepare("delete from UserList where ProjectName = :ProjectName and ProjectRegion = :ProjectRegion"); //名称绑定的方式 注意和一般的sql语句不同的是，在text类型的时候他不需要加 ''--如果在使用exec的纯sql语句的时候 必须加上''
			query.bindValue(":ProjectName", g_ProjectName);
			query.bindValue(":ProjectRegion", g_ProjectRegion);
			if (query.exec())
			{
				RemoveDir("./../Project/" + g_ProjectRegion + "/" + g_ProjectName);
			}
		}
		else
		{
			m_HasErr = true;
			QMessageBox::critical(this, "删除用户错误", "用户信息，数据库没有该用户信息");
		}
	}
	else if (g_ProjectName != "" && g_DevIP != "")//选中父节点下面的子节点d的子节点
	{
		query.prepare("delete from UserList where ProjectRegion =? and ProjectName=? and  ElecRoomID=? and  DevIndex=? and  IP=? and  N2NIP=? and Port=?	and UserName=?	and PassWord=?	and DevName=?	and Note=?"); //名称绑定的方式 注意和一般的sql语句不同的是，在text类型的时候他不需要加 ''
		query.addBindValue(g_ProjectRegion);
		query.addBindValue(g_ProjectName);
		query.addBindValue(g_ElecRoomID);
		query.addBindValue(g_DevIndex);
		query.addBindValue(g_DevIP);
		query.addBindValue(g_N2NIP);
		query.addBindValue(g_DevPort);
		query.addBindValue(g_UserName);
		query.addBindValue(g_UserWord);
		query.addBindValue(g_DevName);
		query.addBindValue(g_UserListNote);
		if (query.exec())
		{
			RemoveDir("./../Project/" + g_ProjectRegion + "/" + g_ProjectName + "/" + g_DevIP);
		}
	}
	g_UserListDB.close();
	QSqlDatabase::removeDatabase("QSQLITE");
	emit sigDelted();
	return;
}

/*从数据库中修改数据*/
void NewUser::soltModifProjectFromDB()
{
	if (g_WorM == 1)
	{
		OpenDB();
		QSqlQuery query(g_UserListDB);
		if (g_ProjectRegion != "" && g_ProjectName == "")//父节点
		{
			query.prepare("update  UserList set ProjectRegion = ? where ProjectRegion = ?");
			query.addBindValue(ui.cb_ProjectRegion->currentText());
			query.addBindValue(g_ProjectRegion);
			if (query.exec())
			{
				RenameDir("./../Project", g_ProjectRegion, ui.cb_ProjectRegion->currentText());
			}
		}
		if (g_ProjectName != "" && g_DevIP == "")//只选中子节点
		{
			/*ProjectName	ElecRoomID	DevIndex	IP	Port	UserName	PassWord	DevName	Note*/
			query.prepare("update  UserList set ProjectName = ? where ProjectName = ? AND ProjectRegion = ? "); //名称绑定的方式 注意和一般的sql语句不同的是，在text类型的时候他不需要加 ''--如果在使用exec的纯sql语句的时候 必须加上''
			query.addBindValue(ui.le_ProjectName->text());
			query.addBindValue(g_ProjectName);
			query.addBindValue(g_ProjectRegion);
			if (query.exec())
			{
				RenameDir("./../Project/" + g_ProjectRegion, g_ProjectName, ui.le_ProjectName->text());
			}
		}
		else if (g_ProjectName != "" && g_DevIP != "")//子子节点
		{
			query.prepare("update UserList  set ElecRoomID = ?, DevIndex = ?, IP = ?,N2NIP = ?, Port = ?, UserName = ?, PassWord = ?, DevName = ?, Note = ? where  ProjectRegion = ?  and ProjectName=? and  ElecRoomID=? and  DevIndex=? and  IP=?  and  N2NIP=? and Port=?	and UserName=?	and PassWord=?	and DevName=?	and Note=?"); //名称绑定的方式 注意和一般的sql语句不同的是，在text类型的时候他不需要加 ''
			query.addBindValue(ui.le_No1ID->text());
			query.addBindValue(ui.le_No2ID->text());
			query.addBindValue(ui.le_IP->text());
			query.addBindValue(ui.le_IP2->text());
			query.addBindValue(ui.le_Port->text());
			query.addBindValue(ui.le_UserName->text());
			query.addBindValue(ui.le_PassWord->text());
			query.addBindValue(ui.cb_FTPType->currentText());
			query.addBindValue(ui.le_Note->text());

			query.addBindValue(g_ProjectRegion);
			query.addBindValue(g_ProjectName);
			query.addBindValue(g_ElecRoomID);
			query.addBindValue(g_DevIndex);
			query.addBindValue(g_DevIP);
			query.addBindValue(g_N2NIP);
			query.addBindValue(g_DevPort);
			query.addBindValue(g_UserName);
			query.addBindValue(g_UserWord);
			query.addBindValue(g_DevName);
			query.addBindValue(g_UserListNote);
			if (query.exec() && ui.le_IP->text() != g_DevIP)//只有修改前后的IP不想等的时候，才需要改对应文件夹名字
			{
				RenameDir("./../Project/" + g_ProjectRegion + "/" + g_ProjectName, g_DevIP, ui.le_IP->text());
			}
		}
		emit sigModifed(ui.cb_ProjectRegion->currentText(), ui.le_ProjectName->text(), ui.le_No1ID->text(), ui.le_No2ID->text(), ui.le_IP->text(), \
			ui.le_IP2->text(), ui.le_Port->text(), ui.le_UserName->text(), ui.le_PassWord->text(), ui.cb_FTPType->currentText(), ui.le_Note->text(), \
			g_ProjectRegion, g_ProjectName, g_ElecRoomID, g_DevIndex, g_DevIP, g_N2NIP, g_DevPort, g_UserName, g_UserWord, g_DevName, g_UserListNote
		);
	}
	return;
}
/*测试函数 --无实际意义*/
void NewUser::solttest()
{
	qDebug() << "调用NewUser::solttest()";

}
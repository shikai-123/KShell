#pragma once
#include <qstring.h>

#define DATA_H

#define ZQ	 1
#define JZD  2
#define HW	 3


#define USERNUM 256
#define DEVNUM 32
#define MAXCHANEL 128 
#define MAXMETER  128


/*log设置*/
struct LogSetting
{
	bool Enable;
	int Num;
	float Size_MB;
	char LogLevel;
};

/*各种工具设置*/
struct OtherToolsSetting
{
	QString SSH;
	QString FTP;
	QString Editor;
	QString ToolForCJQ;
	QString N2N;
};

/*检测jzag 守护进程 是否在线设置*/
struct CheckProSetting
{
	bool Enable;
	int Time_S;
};

/*数据监控设置*/
struct DataMonitSetting
{
	int Time_S;
};

/*字体设置*/
struct FontSetting
{
	int Fontsize;
	QString FontType;
};

/*FTP文件管理器设置*/
struct FTPSetting
{
	bool KeepOnline;
	QString DefaultPath;
};

/*CID信息*/
struct CidInfo
{
	QString name;
	QString type;
};

/*自动升级文件信息*/
struct UpdateFile
{
	QString FileName;
	QString Version;
	QString RemotePath;
	QString LocalPath;
	bool IsUpdateFile = false;
	bool IsNewFile = false;
	bool IsRometHaveLoaclNot = false;
};

/*Jzag错误信息*/
struct JzagErr
{
	/*这些信息从CSV中加载*/
	//QString ErrInfo;
	QString Solution;
	QString Note;
	QString FucSection;
	/*这些信息是在程序执行的时候，动态修改*/
	bool IsAdd = false;
	bool IsRepeatInfo = false;
	QStringList RepeatErrInfo;
};
#pragma once
#include <qstring.h>

#define DATA_H



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
	QString Editor;
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


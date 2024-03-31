#include <QApplication>
#include "Log.h"
#include <qdebug.h>
#include <QtGui>
#include "HomePage.h"
#include "NewUser.h"
#include "Tray.h"
#include <qdir.h>
#include "FTPTreeWork.h"
#include "UPDate.h"
#include <qtimer.h>
#include "CJQConf.h"
#pragma execution_character_set("utf-8")

NewUser* g_NewUser = NULL;
HomePage *g_Home_Page = NULL;
SSHWindow* g_SSHWindow = NULL;//SSH窗口的全局指针
SetingWindow* g_SetingWindow = nullptr;
HelpWindows* g_HelpWindows = nullptr;
FTPTreeList * g_FTPTreeList = nullptr;
FTPTreeWork * g_FTPTreeWork = nullptr;
UPDate * g_UPDate = nullptr;


LogSetting g_LogSetting;
OtherToolsSetting g_OtherToolsSetting;
CheckProSetting g_CheckProSetting;
DataMonitSetting g_DataMonitSetting;
FontSetting g_FontSetting;
QString g_SSHFTPJzhsDefaultPath;//jzhs默认目录
bool g_IsSHHTogetherFTP;//SHH,FTP窗口是否聚合
QString g_LastUsedTime;//软件最后一次打开时间for自动更新检测
bool g_ISUpDate = false;
QJsonObject g_jsonObject;

extern void Delay(int mSeconds);

/*加载样式表*/
void loadStyle(const QString & qssFile)
{
	QString qss;
	QFile file(":/qss/" + qssFile);
	if (file.open(QFile::ReadOnly)) {
		//用QTextStream读取样式文件不用区分文件编码 带bom也行
		QStringList list;
		QTextStream in(&file);
		//in.setCodec("utf-8");
		while (!in.atEnd()) {
			QString line;
			in >> line;
			list << line;
		}
		file.close();
		qss = list.join("\n");
		QString paletteColor = qss.mid(20, 7);
		qApp->setPalette(QPalette(paletteColor));
		//用时主要在下面这句
		qApp->setStyleSheet(qss);
	}
}

/*Qjson 异常输出*/
void ConfError(QJsonParseError& parseJsonErr)
{
	if (parseJsonErr.error != QJsonParseError::NoError)
	{
		QString Error;
		switch (parseJsonErr.error)
		{
		case 1:
			Error = "对象没有正确地以右花括号结束,逗号少了或者多了！";
			break;
		case 2:
			Error = "缺少分隔不同项目的逗号";
			break;
		case 3:
			Error = "数组未正确以右方括号终止";
			break;
		case 4:
			Error = "缺少将对象内的键与值分开的冒号";
			break;
		case 5:
			Error = "值非法";
			break;
		case 6:
			Error = "输入流在解析数字时结束";
			break;
		case 7:
			Error = "数字格式不正确";
			break;
		case 8:
			Error = "输入中出现非法转义序列";
			break;
		case 9:
			Error = "输入中出现非法 UTF8 序列";
			break;
		case 10:
			Error = "字符串没有以引号结尾";
			break;
		case 11:
			Error = "需要一个对象，但找不到";
			break;
		case 12:
			Error = "JSON 文档嵌套太深，解析器无法解析";
			break;
		case 13:
			Error = "JSON 文档太大，解析器无法解析";
			break;
		case 14:
			Error = "解析后的文档末尾包含额外的垃圾字符";
			break;
		default:
			Error = "你能看到这个错误提示！牛逼！";
			break;
		}
		//QMessageBox::critical(this, "载入CONF.json错误", Error);
		qCritical() << "解析配置文件错误！ 错误代码： " << parseJsonErr.error << Error;
		exit(-1);
	}

}

void LoadConf()
{
	/*解析json文件*/
	QFile file("./../conf/CONF.json");
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qCritical() << "载入CONF.json错误，找不到文件！" << file.errorString();
		QMessageBox::critical(g_Home_Page, "载入CONF.json错误", "找不到文件！");
		exit(-1);
	}
	QString value = file.readAll();
	file.close();

	QJsonParseError parseJsonErr;
	QJsonDocument document = QJsonDocument::fromJson(value.toUtf8(), &parseJsonErr);
	ConfError(parseJsonErr);
	QJsonObject jsonObject = g_jsonObject = document.object();
	//qDebug() << "jsonObject[name]==" << jsonObject["Author"].toString();
	g_SSHFTPJzhsDefaultPath = jsonObject["SSHFTPJzhsDefaultPath"].toString();
	g_IsSHHTogetherFTP = jsonObject["SHHTogetherFTP"].toBool();

	QJsonValue jsonValueList = jsonObject.value(QStringLiteral("LOG"));
	QJsonObject item = jsonValueList.toObject();
	g_LogSetting.Enable = item["Enable"].toInt();
	g_LogSetting.Num = item["Num"].toInt();
	g_LogSetting.Size_MB = item["Size_MB"].toInt();
	g_LogSetting.LogLevel = item["LogLevel"].toInt();

	jsonValueList = jsonObject.value(QStringLiteral("OtherTools"));
	item = jsonValueList.toObject();
	g_OtherToolsSetting.Editor = item["Editor"].toString();

	jsonValueList = jsonObject.value(QStringLiteral("CheckPro"));
	item = jsonValueList.toObject();
	g_CheckProSetting.Enable = item["Enable"].toInt();
	g_CheckProSetting.Time_S = item["Time_S"].toInt();

	jsonValueList = jsonObject.value(QStringLiteral("DataMonit"));
	item = jsonValueList.toObject();
	g_DataMonitSetting.Time_S = item["Time_S"].toInt();

	jsonValueList = jsonObject.value(QStringLiteral("Font"));
	item = jsonValueList.toObject();
	g_FontSetting.Fontsize = item["Fontsize"].toInt();
	g_FontSetting.FontType = item["FontType"].toString();
	g_LastUsedTime = QDate::currentDate().toString("yyyy:MM:dd");
	//qDebug() << jsonObject["LastUsedTime"].toString();
	if (jsonObject["LastUsedTime"].toString() != g_LastUsedTime)
	{
		qDebug() << "今天第一运行，启动更新";
		/*写json*/
		jsonObject["LastUsedTime"] = QDate::currentDate().toString("yyyy:MM:dd");
		QJsonDocument doc;
		doc.setObject(jsonObject);
		QByteArray data = doc.toJson();
		file.open(QIODevice::WriteOnly | QIODevice::Text);
		file.write(data);
		file.close();
		g_ISUpDate = true;
		//StartUpdte();
	}
	loadStyle(jsonObject["SoftSkin"].toString());
}

void CheckDir(QString DirName)
{
	QDir dir;
	if (!dir.exists(DirName)) {
		qInfo() << "创建目录：" << DirName;
		dir.mkpath(DirName);
	}
	else
	{
		//qInfo() << DirName << "目录存在！";
	}
}

int main(int argc, char *argv[])
{
	qDebug() << "QT_VER" << QT_VERSION_STR;
	qInstallMessageHandler(outputMessage);
	CheckDir("./../log");
	CheckDir("./../conf");
	CheckDir("./../Project");
	CheckDir("./../Update");
	CheckDir("./../Temp");
	CheckDir("./../Tools");
	CheckDir("./../Update");
	QApplication a(argc, argv);// 对于用Qt写的任何一个GUI应用，不管这个应用有没有窗口或多少个窗口，有且只有一个QApplication对象。
	//设置中文字体  
	a.setFont(QFont("Microsoft Yahei", 9));
	LoadConf();
	g_NewUser = new NewUser();//创建新用户窗口
	g_SSHWindow = new SSHWindow();//创建状态检测窗口
	g_SetingWindow = new SetingWindow();//设置窗口
	g_HelpWindows = new HelpWindows();//帮助窗口
	g_FTPTreeList = new FTPTreeList();//FTP目录展示
	g_FTPTreeWork = new FTPTreeWork();//FTP下载、上传、展示线程
	g_UPDate = new UPDate();//软件自动升级线程部分
	g_Home_Page = new HomePage;
	g_Home_Page->show();
	if (g_ISUpDate)
	{
		g_Home_Page->StartUpdte();
	}
	Tray tray(*g_Home_Page);
	return a.exec();
}

#include "SetingWindow.h"
#include "DataStruck.h"
#include <qdebug.h>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#pragma execution_character_set("utf-8")

extern LogSetting g_LogSetting;
extern OtherToolsSetting g_OtherToolsSetting;
extern CheckProSetting g_CheckProSetting;
extern DataMonitSetting g_DataMonitSetting;
extern FontSetting g_FontSetting;
extern QString g_SSHFTPJzhsDefaultPath;
extern QString g_LastUsedTime;
extern bool g_IsSHHTogetherFTP;
extern QString g_N2NIPSec;
extern QJsonObject g_jsonObject;

/*本cpp只负责写json，以及把已经读取好的json内容显示到窗口上，读取josn在guidwindow上*/
SetingWindow::SetingWindow(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	this->setWindowTitle("设置");
	QStringList SkinList;
	SkinList << "lightblue.css" << "blacksoft.css" << "flatgray.css" << "无";
	ui.cb_SoftSkin->addItems(SkinList);
	this->ui.cb_SoftSkin->setCurrentText(g_jsonObject["SoftSkin"].toString());

	//LoadData();
	connect(ui.tb_ChoseSSHAddr, SIGNAL(clicked()), this, SLOT(slotChoseSSHAddr()));
	connect(ui.tb_ChoseFTPAddr, SIGNAL(clicked()), this, SLOT(slotChoseFTPAddr()));
	connect(ui.tb_ChoseEditorAddr, SIGNAL(clicked()), this, SLOT(slotChoseEditorAddr()));
	connect(ui.tb_ChoseToolForCJQAddr, SIGNAL(clicked()), this, SLOT(slotChoseToolForCJQAddr()));
	connect(ui.tb_ChoseN2NAddr, SIGNAL(clicked()), this, SLOT(slotN2NAddr()));
	connect(ui.pb_Confirm, SIGNAL(clicked()), this, SLOT(slotWriteJson()));
	connect(ui.pb_OpenConfJson, SIGNAL(clicked()), this, SLOT(slotOpenConfJson()));

}

SetingWindow::~SetingWindow()
{
}

/*默认工具选择 -- 选择SSH路径*/
void SetingWindow::slotChoseSSHAddr()
{
	SSHAddr = QFileDialog::getOpenFileName(this, tr("选择SSH路径"), qApp->applicationDirPath(), tr("Files (*.exe)"));
	ui.lb_SSHAddr->setText(SSHAddr);
}

/*默认工具选择 -- 选择FTP路径*/
void SetingWindow::slotChoseFTPAddr()
{
	FTPAddr = QFileDialog::getOpenFileName(this, tr("选择FTP路径"), qApp->applicationDirPath(), tr("Files (*.exe)"));
	ui.lb_FTPAddr->setText(FTPAddr);
}

/*默认工具选择 -- 选择Editor路径*/
void SetingWindow::slotChoseEditorAddr()
{
	EditorAddr = QFileDialog::getOpenFileName(this, tr("选择Editor路径"), qApp->applicationDirPath(), tr("Files (*.exe)"));
	ui.lb_EditorAddr->setText(EditorAddr);
}

/*默认工具选择 -- 选择ToolForCJQ路径*/
void SetingWindow::slotChoseToolForCJQAddr()
{
	ToolForCJQAddr = QFileDialog::getOpenFileName(this, tr("选择ToolForCJQ路径"), qApp->applicationDirPath(), tr("Files (*.exe)"));
	ui.lb_ToolForCJQAddr->setText(ToolForCJQAddr);
}

/*默认工具选择 -- 选择其他工具路径*/
void SetingWindow::slotN2NAddr()
{
	N2NAddr = QFileDialog::getExistingDirectory(this, "选择n2n路径", "");//绝对路径
	ui.lb_N2NAddr->setText(N2NAddr);
}

/*把从设置窗口读到的内容，写到CONF.json*/
void SetingWindow::slotWriteJson()
{
	/*一些检查*/
	if (ui.le_FTPSSHDeflautPath->text().endsWith("/") == false || ui.le_FTPSSHDeflautPath->text().startsWith("/") == false)
	{
		QMessageBox::critical(this, "ERR", "jzhs路径不是以“/”开头或结束");
		return;
	}

	QJsonObject LOG_obj;
	LOG_obj.insert("Enable", int(ui.cb_OpenLog->isChecked()));
	LOG_obj.insert("Num", ui.sb_LogNum->value());
	LOG_obj.insert("Size_MB", ui.sb_LogSize->value());
	LOG_obj.insert("LogLevel", ui.sb_LogLevel->value());

	QJsonObject OtherTools_obj;
	OtherTools_obj.insert("SSH", ui.lb_SSHAddr->text());
	OtherTools_obj.insert("FTP", ui.lb_FTPAddr->text());
	OtherTools_obj.insert("Editor", ui.lb_EditorAddr->text());
	OtherTools_obj.insert("ToolForCJQ", ui.lb_ToolForCJQAddr->text());
	OtherTools_obj.insert("N2N", ui.lb_N2NAddr->text());

	QJsonObject CheckPro_obj;
	CheckPro_obj.insert("Enable", int(ui.cb_ProCheckOpen->isChecked()));
	CheckPro_obj.insert("Time_S", ui.sb_PeoCheckTime->value());

	QJsonObject DataMonit_obj;
	DataMonit_obj.insert("Time_S", ui.sb_DataMonitTime->value());

	QJsonObject Font_obj;
	Font_obj.insert("Fontsize", ui.sb_FontSize->text().toInt());
	Font_obj.insert("FontType", ui.cb_FontType->currentText());

	QJsonObject Json;
	Json.insert("SSHFTPJzhsDefaultPath", ui.le_FTPSSHDeflautPath->text());
	Json.insert("LastUsedTime", g_LastUsedTime);
	Json.insert("SHHTogetherFTP", bool(ui.cb_IsSHHTogetherFTP->isChecked()));//!!因为是bool量，所以只能转到bool，在json中为false或者ture。 0或者1是读不到的
	Json.insert("N2NIPSec", ui.le_N2NIP->text());
	Json.insert("LOG", LOG_obj);
	Json.insert("OtherTools", OtherTools_obj);
	Json.insert("CheckPro", CheckPro_obj);
	Json.insert("DataMonit", DataMonit_obj);
	Json.insert("Font", Font_obj);
	Json.insert("SoftSkin", ui.cb_SoftSkin->currentText());

	//从josn转成文本
	QJsonDocument doc;
	doc.setObject(Json);
	QByteArray data = doc.toJson();

	//json写入文件
	QFile file("./../conf/CONF.json");
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		qCritical() << "打开写CONF.json错误！" << file.errorString();
		QMessageBox::critical(this, "写CONF.json错误", "保存失败！" + file.errorString());
		return;
	}
	if (file.write(data) < 0)
	{
		qCritical() << "打开写CONF.json错误！" << file.errorString();
		QMessageBox::critical(this, "写CONF.json错误", "保存失败！" + file.errorString());
		return;
	}
	else
	{
		qInfo() << "打开写CONF.json OK！" << file.errorString();
		QMessageBox::information(this, "保存设置", "保存成功，重启生效！");
		file.close();
		Reboot();
	}
	file.close();
}

/*打开CONF.json*/
void SetingWindow::slotOpenConfJson()
{
	QDesktopServices::openUrl(QUrl::fromLocalFile("./../conf/CONF.json"));//!!!必须要加::fromLocalFile
}

/*把从CONF.json读取到的数据，显示在窗口上*/
void SetingWindow::LoadData()
{
	ui.le_FTPSSHDeflautPath->setText(g_SSHFTPJzhsDefaultPath);
	ui.cb_IsSHHTogetherFTP->setChecked(g_IsSHHTogetherFTP);
	ui.le_N2NIP->setText(g_N2NIPSec);

	ui.cb_OpenLog->setChecked(g_LogSetting.Enable);
	ui.sb_LogNum->setValue(g_LogSetting.Num);
	ui.sb_LogSize->setValue(g_LogSetting.Size_MB);
	ui.sb_LogLevel->setValue(g_LogSetting.LogLevel);

	ui.lb_SSHAddr->setText(g_OtherToolsSetting.SSH);
	ui.lb_FTPAddr->setText(g_OtherToolsSetting.FTP);
	ui.lb_EditorAddr->setText(g_OtherToolsSetting.Editor);
	ui.lb_ToolForCJQAddr->setText(g_OtherToolsSetting.ToolForCJQ);
	ui.lb_N2NAddr->setText(g_OtherToolsSetting.N2N);

	ui.cb_ProCheckOpen->setChecked(g_CheckProSetting.Enable);
	ui.sb_PeoCheckTime->setValue(g_CheckProSetting.Time_S);

	ui.sb_DataMonitTime->setValue(g_DataMonitSetting.Time_S);

	ui.sb_FontSize->setValue(g_FontSetting.Fontsize);
}

/*重启软件*/
void SetingWindow::Reboot()
{
	QString program = QApplication::applicationFilePath();
	QStringList arguments = QApplication::arguments();
	QString workingDirectory = QDir::currentPath();
	QProcess::startDetached(program, arguments, workingDirectory);
	QApplication::exit();
}

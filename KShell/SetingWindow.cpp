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
extern FontSetting g_FontSetting;
extern QString g_SSHFTPDefaultPath;
extern QString g_LastUsedTime;
extern bool g_IsSHHTogetherFTP;
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

	connect(ui.tb_ChoseEditorAddr, SIGNAL(clicked()), this, SLOT(slotChoseEditorAddr()));
	connect(ui.pb_Confirm, SIGNAL(clicked()), this, SLOT(slotWriteJson()));
	connect(ui.pb_OpenConfJson, SIGNAL(clicked()), this, SLOT(slotOpenConfJson()));

}

SetingWindow::~SetingWindow()
{
}



/*默认工具选择 -- 选择Editor路径*/
void SetingWindow::slotChoseEditorAddr()
{
	EditorAddr = QFileDialog::getOpenFileName(this, tr("选择Editor路径"), qApp->applicationDirPath(), tr("Files (*.exe)"));
	ui.lb_EditorAddr->setText(EditorAddr);
}





/*把从设置窗口读到的内容，写到CONF.json*/
void SetingWindow::slotWriteJson()
{
	/*一些检查*/
	if (ui.le_FTPSSHDeflautPath->text().endsWith("/") == false || ui.le_FTPSSHDeflautPath->text().startsWith("/") == false)
	{
		QMessageBox::critical(this, "ERR", "默认路径不是以“/”开头或结束");
		return;
	}

	QJsonObject LOG_obj;
	LOG_obj.insert("Enable", int(ui.cb_OpenLog->isChecked()));
	LOG_obj.insert("Num", ui.sb_LogNum->value());
	LOG_obj.insert("Size_MB", ui.sb_LogSize->value());
	LOG_obj.insert("LogLevel", ui.sb_LogLevel->value());

	QJsonObject OtherTools_obj;
	OtherTools_obj.insert("Editor", ui.lb_EditorAddr->text());


	QJsonObject Font_obj;
	Font_obj.insert("Fontsize", ui.sb_FontSize->text().toInt());
	Font_obj.insert("FontType", ui.cb_FontType->currentText());

	QJsonObject Json;
	Json.insert("SSHFTPDefaultPath", ui.le_FTPSSHDeflautPath->text());
	Json.insert("LastUsedTime", g_LastUsedTime);
	Json.insert("SHHTogetherFTP", bool(ui.cb_IsSHHTogetherFTP->isChecked()));//!!因为是bool量，所以只能转到bool，在json中为false或者ture。 0或者1是读不到的
	Json.insert("LOG", LOG_obj);
	Json.insert("OtherTools", OtherTools_obj);
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
	ui.le_FTPSSHDeflautPath->setText(g_SSHFTPDefaultPath);
	ui.cb_IsSHHTogetherFTP->setChecked(g_IsSHHTogetherFTP);

	ui.cb_OpenLog->setChecked(g_LogSetting.Enable);
	ui.sb_LogNum->setValue(g_LogSetting.Num);
	ui.sb_LogSize->setValue(g_LogSetting.Size_MB);
	ui.sb_LogLevel->setValue(g_LogSetting.LogLevel);

	ui.lb_EditorAddr->setText(g_OtherToolsSetting.Editor);
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

#pragma once

#include <QWidget>
#include "ui_SetingWindow.h"
#include <qstring.h>
#include <qfiledialog.h>
#include <qprocess.h>
#include <qdesktopservices.h>
class SetingWindow : public QWidget
{
	Q_OBJECT

public:
	SetingWindow(QWidget *parent = Q_NULLPTR);
	~SetingWindow();
	QString SSHAddr;
	QString FTPAddr;
	QString EditorAddr;
	QString ToolForCJQAddr;
	QString N2NAddr;
	void LoadData();
	void Reboot();
	Ui::SetingWindow ui;

private:
signals:
	void sigRefreshVersion();
public slots:
	void slotChoseSSHAddr();
	void slotChoseFTPAddr();
	void slotChoseEditorAddr();
	void slotChoseToolForCJQAddr();
	void slotN2NAddr();
	void slotWriteJson();
	void slotOpenConfJson();
};

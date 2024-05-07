#include "HelpWindows.h"
#include "qstring.h"
#include "qdebug.h"
#pragma execution_character_set("utf-8")
HelpWindows::HelpWindows(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	this->setWindowTitle("帮助");

	QString LinkAddr = "<a style='color: green;' href = \"https://gitee.com/shikai1995/KShellUpdate\" >更新源</a>";
	ui.lb_UpdateSoouce->setOpenExternalLinks(true);
	ui.lb_UpdateSoouce->setText(LinkAddr);
	ui.lb_UpdateSoouce->show();
}

HelpWindows::~HelpWindows()
{
}
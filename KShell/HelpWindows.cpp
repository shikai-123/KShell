#include "HelpWindows.h"
#include "qstring.h"
#include "qdebug.h"
#pragma execution_character_set("utf-8")
HelpWindows::HelpWindows(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	this->setWindowTitle("帮助");
	QString LinkAddr = "<a style='color: green;' href = \"http://note.youdao.com/noteshare?id=b27d23700090009ece91b61d42963923\" >JZAG资料</a>";
	ui.lb_youdaoyun->setOpenExternalLinks(true);
	ui.lb_youdaoyun->setText(LinkAddr);
	ui.lb_youdaoyun->show();
	
	LinkAddr = "<a style='color: green;' href = \"https://gitee.com/shikai1995/cjqtool_conf\" >更新源</a>";
	ui.lb_UpdateSoouce->setOpenExternalLinks(true);
	ui.lb_UpdateSoouce->setText(LinkAddr);
	ui.lb_UpdateSoouce->show();

	LinkAddr = "<a style='color: green;' href = \"https://note.youdao.com/ynoteshare/index.html?id=3b91e1814277a87368f3a226ad53b8d0&type=note&_time=1658826076940\" >使用说明</a>";
	ui.lb_UseNote->setOpenExternalLinks(true);
	ui.lb_UseNote->setText(LinkAddr);
	ui.lb_UseNote->show();
}

HelpWindows::~HelpWindows()
{
}
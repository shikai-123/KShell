#pragma once

#include <QWidget>
#include "ui_HelpWindows.h"

class HelpWindows : public QWidget
{
	Q_OBJECT

public:
	HelpWindows(QWidget *parent = Q_NULLPTR);
	~HelpWindows();

private:
	Ui::HelpWindows ui;
};

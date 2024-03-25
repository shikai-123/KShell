#include <QMutex>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <string>
#include <qdebug.h>
#include <iostream>
#include <qfileinfo.h>
#include "DataStruck.h"
#pragma execution_character_set("utf-8")
extern LogSetting g_LogSetting;

void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	if (g_LogSetting.Enable != true)//作为日志的开关
	{
		return;
	}
	if (type < g_LogSetting.LogLevel)//作为日志的打印等级 0任何等级都打印
	{
		return;
	}
	qDebug() << msg;
	static QMutex mutex;
	mutex.lock();

	QString text;
	switch (type)
	{
	case QtDebugMsg:
		text = QString("[Debug]");
		break;
	case QtWarningMsg:
		text = QString("[Warning]");
		break;
	case QtCriticalMsg:
		text = QString("[Critical]");
		break;
	case QtInfoMsg:
		text = QString("[Info]");
		break;
	case QtFatalMsg:
		text = QString("[Fatal]");
	}

	QString context_info = QString("%1:%2").arg((QString(context.file).split("\\")).last()).arg(context.line);
	QString current_date_time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss ddd");
	QString current_date = QString("[%1]").arg(current_date_time);
	QString message = QString("%1 %2 %3").arg(current_date).arg(text).arg(msg);


	QString FileName = "../log/log";
	for (size_t i = 0; i < g_LogSetting.Num; i++)
	{
		QString CurrentFileName = FileName + QString::number(i) + ".log";
		QFile file(CurrentFileName);
		//qDebug() << file.fileTime(file.FileBirthTime).toLocalTime();
		//qDebug() << file.fileTime(file.FileBirthTime).toString("yyyyMMddhhmmss").toLongLong();

		if (file.size() >= g_LogSetting.Size_MB * 1024 * 1024)//log文件大小
		{
			file.close();
			if (i == g_LogSetting.Num - 1)//标志着所有的文件都已经写满了
			{
				qDebug() << "所有的日志文件都已经写满了,";
				for (size_t i = 0; i < 4; i++)
				{
					QString TempFileName = FileName + QString::number(i) + ".log";
					QFile file(TempFileName);
					if (i == 0)//删除最旧的log文件 log0
					{
						file.remove();
					}
					else
					{
						file.rename(FileName + QString::number(i - 1) + ".log");//log文件名字更改 ex:log1->log0
					}
					file.close();
				}
			}
			continue;
		}
		file.open(QIODevice::WriteOnly | QIODevice::Append);
		QTextStream text_stream(&file);
		text_stream << message << "\r\n";
		file.flush();
		file.close();
		break;
	}


	mutex.unlock();
}
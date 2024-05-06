#pragma execution_character_set("utf-8")
#include "FTPTreeWork.h"
#include "FTPTreeList.h"
#include "DataStruck.h"
#include "CJQConf.h"
extern FTPTreeWork * g_FTPTreeWork;
extern FTPTreeList * g_FTPTreeList;
extern QString g_DevIP, g_DevPort, g_ConnetIP, g_UserName, g_UserWord, g_UserListNote, g_FTPType, g_ProjectName, g_FTPHead;//用于采集登录信息
extern OtherToolsSetting g_OtherToolsSetting;
QString g_FTPCurrentPath;

FTPTreeWork::FTPTreeWork()
{
	CurlInit();//libcurl初始化
	connect(this, SIGNAL(sigFTPDownAndOpen(QString, QString, QString, QString, bool, bool, bool, bool)), this, SLOT(slotFtpGetFile(QString, QString, QString, QString, bool, bool, bool, bool)));
	connect(this, SIGNAL(sigStartFTPUP(QString, QString, QString, QString, bool)), this, SLOT(slotFtpUpFile(QString, QString, QString, QString, bool)));
	connect(g_FTPTreeList, SIGNAL(sigOpenFTPlist()), this, SLOT(slotOpenFTPlist()));

	connect(this, SIGNAL(sigFileList(QStringList)), g_FTPTreeList, SLOT(slotViewFileList(QStringList)));//文件列表具体内容
	connect(this, SIGNAL(sigFTPProcess(float)), g_FTPTreeList, SLOT(slotDownProcess(float)));
	connect(this, SIGNAL(sigFTPDownError(int, QString, bool)), g_FTPTreeList, SLOT(slotFTPDownError(int, QString, bool)));
	connect(this, SIGNAL(sigFTPUpError(int)), g_FTPTreeList, SLOT(slotFTPUpError(int)));
	connect(this, SIGNAL(sigFTPListError(int)), g_FTPTreeList, SLOT(slotFTPListError(int)));
	connect(this, SIGNAL(sigFTPCMDError(int)), g_FTPTreeList, SLOT(slotFTPCMDError(int)));
	connect(this, SIGNAL(sigFTPDeletDirError(QString, int)), g_FTPTreeList, SLOT(slotFTPDeletDirError(QString, int)));
	connect(this, SIGNAL(sigFTPDownDirError(QString, int)), g_FTPTreeList, SLOT(slotFTPDownDirError(QString, int)));
	connect(this, SIGNAL(sigCrulNull()), g_FTPTreeList, SLOT(slotCurlNullError()));
	connect(this, SIGNAL(sigFTPListRefresh()), g_FTPTreeList, SLOT(slotRK_FTPFreshList()));
	connect(this, SIGNAL(sigFTPStatusAlarm(bool)), g_FTPTreeList, SLOT(slotFTPStatusAlarm(bool)));
	connect(this, SIGNAL(sigFTPDownAndOpenError(bool)), g_FTPTreeList, SLOT(slotFTPDownAndOpenError(bool)));
	//BlockingQueuedConnection这个一定要加，因为在上传本地文件夹的时候，如果不保持同步，就会导致
	connect(g_FTPTreeList, SIGNAL(sigFTPCMD(const char*)), this, SLOT(slotFTPCMDExec(const char*)));
	connect(g_FTPTreeList, SIGNAL(sigFTPCMDDeletDir(QString)), this, SLOT(slotFTPDeletDir(QString)));
	//connect(g_FTPTreeList, SIGNAL(sigFTPCMDDeletDir(QString)), this, SLOT(slotFTPDeletDir(QString)));
	connect(g_FTPTreeList, SIGNAL(sigFTPCMDDownDir(QString, QString)), this, SLOT(slotFTPDownDir(QString, QString)));
	connect(g_FTPTreeList, SIGNAL(sigGetFTPList(QString)), this, SLOT(slotGetFTPList(QString)));//, Qt::BlockingQueuedConnection
	connect(this, &FTPTreeWork::sigUPorDownProcessMsg, g_FTPTreeList, &FTPTreeList::slotUPorDownProcessMsg);
}

/*libcurl初始化*/
void FTPTreeWork::CurlInit()
{
	//初始化libcurl
	rs = curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();/*curl_easy_init用来初始化一个CURL的指针(有些像返回FILE类型的指针一样). 相应的在调用结束时要用curl_easy_cleanup函数清理.一般curl_easy_init意味着一个会话的开始. 它会返回一个easy_handle(CURL*对象), 一般都用在easy系列的函数中.*/
	if (curl == nullptr)
	{
		qCritical() << "! curl 指针为空 !";
		emit sigCrulNull();
	}
	qDebug() << "LibCurl_VER： " << curl_version();//!!不打印为啥
}

/*在配置窗口或首页点击文件管理器，--打开FTP文件列表*/
void FTPTreeWork::slotOpenFTPlist()
{
	slotGetFTPList(g_FTPCurrentPath);
}

/*复位libcurl curl_easy_setopt 设置 0ok !0 有问题*/
int FTPTreeWork::CurlOptClear()
{
	int ret = 0;
	ret += curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, nullptr);//关闭文件列表回调函数
	ret += curl_easy_setopt(curl, CURLOPT_NOPROGRESS, true);//关闭进度条
	ret += curl_easy_setopt(curl, CURLOPT_POSTQUOTE, nullptr);//清空命令设置 用完之后 必须清空  要不 再次执行 拿到文件列表的时候  库会崩溃！！！！
	ret += curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);//取消打印Ftp的过程信息
	ret += curl_easy_setopt(curl, CURLOPT_READDATA, nullptr);//设置上传文件的文件指针为空
	ret += curl_easy_setopt(curl, CURLOPT_UPLOAD, 0);    //取消上传模式
	ret += curl_easy_setopt(curl, CURLOPT_INFILESIZE, 0);//清空阶段，设置上传文件的大小为0，有需要在单独的模块中设置
	ret += curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 0);  //远程服务器如果没有这个目录，会自动创建然后上传文件到这个目录下面--取消
	ret += curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, nullptr);//清空进度调进度回调函数

	if (ret != 0)
	{
		qDebug() << "复位设置有问题";
	}
	if (CMDlist != nullptr)
	{
		curl_slist_free_all(CMDlist);//清空命令列表 - 此函数没有返回值
		CMDlist = nullptr;//清空后 一定要手动赋值NULL
	}
	return ret;
}

/*在FTP中增加当前条目和上一级条目，仅对FTP协议而言*/
void FTPTreeWork::AddOtherListItem()
{
	//"-rw-r--r--    1 root     root         6061 Mar 22 11:10 123.json\\n\""
	QStringList CurrentDir;
	CurrentDir << "drw-r--r-- " << "1" << "root" << "root" << "1" << "1" << "1" << "1" << ".";
	QStringList DownDir;
	DownDir << "drw-r--r-- " << "1" << "root" << "root" << "1" << "1" << "1" << "1" << "..";
	emit g_FTPTreeWork->sigFileList(CurrentDir);
	emit g_FTPTreeWork->sigFileList(DownDir);
}

/*FTP下载进度回调函数*/
int FTPDownProgress(void *buffer, double dltotal, double dlnow, double ultotal, double ulnow)
{
	if (dlnow == 0.0)
	{
		return 0;
	}
	emit g_FTPTreeWork->sigFTPProcess((dlnow / dltotal) * 100);
	return 0;
}

/*FTP上传进度回调函数*/
int FTPUpProgress(void *buffer, double dltotal, double dlnow, double ultotal, double ulnow)
{
	if (ulnow == 0.0)
	{
		return 0;
	}
	emit g_FTPTreeWork->sigFTPProcess((ulnow / ultotal) * 100);
	return 0;
}

/*ftp下载 ftpurl：要拿的文件的Ftp地址； filename：存放的地址 0 ok -1打开文件失败 -2创建文件夹失败 其他curl错误码*/
int FTPTreeWork::slotFtpGetFile(QString FTPUrl, QString User, QString PassWord, QString LocalPath, bool IsOpenFile, bool IsDownDir, bool IsOpenInCJQConf, bool IsAddWatch)
{
	if (LocalPath.endsWith("/"))
	{
		qDebug() << "下载文件夹方式--本地创建--" << LocalPath;
		QDir Dir;//!QDir 实例化带参数和不带参数的区别，带参活默认路径为你参数的路径，不带参默认路径为你的app路径
		if (!Dir.exists(LocalPath))
		{
			if (!Dir.mkpath(LocalPath))// 创建返回true，否则失败
			{
				qDebug() << "创建失败";
				return -2;
			}
		}
		return 0;
	}
	int rs = 0;
	FILE* fp = NULL;
	qDebug() << "ftp url= " << FTPUrl;
	qDebug() << "ftp down= " << LocalPath;
	QTextCodec *code = QTextCodec::codecForName("GB2312");//Qt 中默认的编码为UTF-8，故在windows下需要先转码才能打正确打开。
	std::string name = code->fromUnicode(LocalPath).data();
	fp = fopen(name.c_str(), "wb");
	if (fp)
	{
		//rs = curl_global_init(CURL_GLOBAL_ALL);//初始化libcurl
		//curl = curl_easy_init();/*curl_easy_init用来初始化一个CURL的指针(有些像返回FILE类型的指针一样). 相应的在调用结束时要用curl_easy_cleanup函数清理.一般curl_easy_init意味着一个会话的开始. 它会返回一个easy_handle(CURL*对象), 一般都用在easy系列的函数中.*/
		QString loginstr = User + ":" + PassWord;
		rs = curl_easy_setopt(curl, CURLOPT_URL, FTPUrl.toStdString().c_str());//设置传输选项；CURLOPT_URL设置访问URl，ftpurl.data()url的指针；
		if (User.length() > 0)
		{
			rs = curl_easy_setopt(curl, CURLOPT_USERPWD, loginstr.toStdString().c_str());/*返回0意味一切ok*/
			rs = curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);/*将CURLOPT_VERBOSE属性设置为1，libcurl会输出通信过程中的一些细节。*/
			rs = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, nullptr);
			/*一般不用设置 CURLOPT_WRITEFUNCTION CURLOPT_WRITEDATA 是相互关联的一个是文件指针 一个是函数指针 同一个信息会输出到这两个
			在获取文件列表的时候，CURLOPT_WRITEFUNCTION 设置了回调函数，所以这里我设置CURLOPT_WRITEDATA 会导致下面的回调函数会调用
			所以我这里设置一个空指针 而且curl_easy_setopt设置不分顺序 因为是在 curl_easy_perform生效*/
			rs = curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);/*此函数多做数据保存的功能，如处理下载文件。可以通过 CURLOPT_WRITEDATA属性给默认回调函数传递一个已经打开的文件指针，用于将数据输出到文件里。 */
			rs = curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, FTPDownProgress);
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, FALSE);//设置为false才能开启进度条
			/*
			rs = curl_easy_setopt(curl, CURLOPT_PORT, 21);//指定端口
			LogInfo((boost::format("ftp rs6= %d") % rs).str());
			*/

			rs = curl_easy_perform(curl);/*该函数是完成curl_easy_setopt指定的所有选项，返回0意味一切ok*/
			fclose(fp);
			CurlOptClear();
			if (IsOpenFile == 1 && rs == 0)//下载并打开文件
			{
				/*if (QDesktopServices::openUrl(QUrl::fromLocalFile(LocalPath)) == false)
				{
					qDebug() << "打开文件失败！";
					emit sigFTPDownAndOpen(false);
				}*/
				process = new QProcess();
				if (process->startDetached(g_OtherToolsSetting.Editor, QStringList() << LocalPath) == false)
				{
					qDebug() << "打开文件失败！";
					emit sigFTPDownAndOpenError(false);
				}
				delete process;//关闭指针不会影响已经打开的软件
			}
			if (IsDownDir == false)//如果下载的是文件夹，就没必要每次都提示
			{
				emit sigFTPDownError(rs, LocalPath, IsAddWatch);
			}
			return rs;
		}
	}
	else
	{
		emit sigFTPDownError(-1, LocalPath, IsAddWatch);
		return -1;
	}
	return 0;
}

/*Ftp上传 ftpurl：Ftp地址；user：Ftp用户名；passwd：密码； filename：本地文件存放的地址 0ok -1打开文件错误 -2 文件大小错误 其他curl错误码*/
int FTPTreeWork::slotFtpUpFile(QString FTPUrl, QString User, QString PassWord, QString LocalPath, bool IsFile)
{
	/*！上传文件，如果这个文件是多层结构，远程目录没有这个文件夹，没关系，他问自己创建*/
	//qDebug() << "上传文件" << LocalPath;
	FILE* sendFile = NULL;
	int rs = 0;
	QString loginstr = User + ":" + PassWord;
	curl_easy_setopt(curl, CURLOPT_URL, FTPUrl.toStdString().data());

	QTextCodec *code = QTextCodec::codecForName("GB2312");//Qt 中默认的编码为UTF-8，故在windows下需要先转码才能打正确打开。
	std::string UpFilename = code->fromUnicode(LocalPath).data();
	QFileInfo info(LocalPath);  //文件大小(字节)
	//打开ftp上传的源文件   
	if (info.size() < 0)
	{
		CurlOptClear();
		emit sigFTPUpError(-2);
		return -2;
	}
	if (NULL == (sendFile = fopen(UpFilename.c_str(), "rb")))//一定是二进制打开，要不然他的大小和info.size()不一致
	{
		CurlOptClear();
		emit sigFTPUpError(-1);
		return -1;
	}
	curl_easy_setopt(curl, CURLOPT_USERPWD, loginstr.toStdString().c_str());
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);//打印Ftp的信息会详细
	curl_easy_setopt(curl, CURLOPT_READDATA, sendFile);//设置上传文件的文件指针
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1);    //设置上传
	curl_easy_setopt(curl, CURLOPT_INFILESIZE, info.size());//设置上传文件的大小
	curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1);  /*远程服务器如果没有这个目录，会自动创建然后上传文件到这个目录下面*/
	curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, FTPUpProgress);
	//curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, FTPUpProgress);//如果可以， 我们鼓励用户改用较新的CURLOPT_XFERINFOFUNCTION 。
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, FALSE);//设置为false才能开启进度条
	rs = curl_easy_perform(curl);
	curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, nullptr);
	fclose(sendFile);
	emit sigUPorDownProcessMsg(LocalPath);
	if (IsFile == true)//只有上传文件，上传成功后刷新；
	{
		emit sigFTPUpError(rs);//发送上传成功或失败信号
		if (g_FTPCurrentPath != "")//加这个if是为了--在不打开文件管理器的情况下，有上传文件的需求，此时g_FTPCurrentPath为空，如果要刷新文件列表，就会报错。通过这个判断为空，来识别他不一通过文件管理器上传文件。 从而不刷新
		{
			emit sigFTPListRefresh();
		}
	}
	else if (g_FTPTreeList->LocalFileList[g_FTPTreeList->LocalFileList.size() - 1] == LocalPath)//上传本地文件夹--到最后一个文件的时候-执行这个
	{
		emit sigFTPStatusAlarm(0);
		emit sigFTPListRefresh();
	}
	else if (g_FTPTreeList->LocalFileList[0] == LocalPath)//上传本地文件夹-第一个文件的时候，执行这个.
	{
		emit sigFTPStatusAlarm(1);
	}
	CurlOptClear();
	return rs;
}

/*输出文件列表回调函数1--用于显示文件列表*/
size_t FileList1(void *ptr, size_t size, size_t nmemb, void *data)
{
	static QString AllFtpListStr = "";
	QString str = QObject::tr((char*)ptr);
	//qDebug() << "输出文件列表1 " << QObject::tr((char*)ptr);//\"-rw-r--r--    1 root     root         6061 Mar 22 11:10 123.json\\n\""
	if (g_FTPHead == "ftp://")
	{
		if (str.endsWith("\r\n"))
		{
			AllFtpListStr += str;
			QStringList FileItem = AllFtpListStr.split("\r\n");
			for (size_t i = 0; i < FileItem.size() - 1; i++)
			{
				emit g_FTPTreeWork->sigFileList(FileItem[i].split(" ", QString::SkipEmptyParts));
			}
			AllFtpListStr.clear();
		}
		else
		{
			//qDebug() << "分段了";
			AllFtpListStr += str;
		}
	}
	else if (g_FTPHead == "sftp://")
	{
		str.remove(-1, 1);//移除最后的\n
		QStringList FileItem = str.split(" ", QString::SkipEmptyParts);//注意 如果本身 他就是空格的话 他的大小就为1 
		emit g_FTPTreeWork->sigFileList(FileItem);
	}
	//	qDebug() << "size " << size << "nmemb " << nmemb << "size*nmemb" << size * nmemb;
	return nmemb;
}

/*输出文件列表回调函数2--用于递归删除服务器文件夹*/
size_t FileList2(void *ptr, size_t size, size_t nmemb, void *data)
{
	static QString AllFtpListStr = "";
	QString str = QObject::tr((char*)ptr);
	//	qDebug() << "输出文件列表2 " << str;//\"-rw-r--r--    1 root     root         6061 Mar 22 11:10 123.json\\n\""
	if (g_FTPHead == "ftp://")
	{
		if (str.endsWith("\r\n"))
		{
			AllFtpListStr += str;
			QStringList AllFileItem = AllFtpListStr.split("\r\n");
			for (size_t i = 0; i < AllFileItem.size() - 1; i++)
			{
				QStringList FileItem = AllFileItem[i].split(" ", QString::SkipEmptyParts);//注意 如果本身 他就是空格的话 他的大小就为1 
				if (FileItem[0].at(0) == "-")
				{
					qDebug() << "删除文件：" + FileItem[8];
					emit g_FTPTreeWork->sigUPorDownProcessMsg(FileItem[8]);
					QString CMD = "DELE " + g_FTPTreeWork->TempDeletDirPath + FileItem[8];
					g_FTPTreeWork->CMDlist = curl_slist_append(g_FTPTreeWork->CMDlist, CMD.toStdString().c_str());

				}
				else if (FileItem[0].at(0) == "d" && FileItem[8] != "." && FileItem[8] != "..")
				{
					qDebug() << "记录目录：" + FileItem[8];
					g_FTPTreeWork->DeletDirList << g_FTPHead + g_ConnetIP + "/" + g_FTPTreeWork->TempDeletDirPath + FileItem[8] + "/";
				}
			}
			AllFtpListStr.clear();
		}
		else
		{
			//qDebug() << "分段了";
			AllFtpListStr += str;
		}
	}
	else if (g_FTPHead == "sftp://")
	{
		QStringList FileItem = str.split(" ", QString::SkipEmptyParts);//注意 如果本身 他就是空格的话 他的大小就为1 
		FileItem[8].chop(1);//!!文件名最后的\n 去不去没有区别(对删除文件而言，但是比较的时候 后面的\n会有影响)
		if (FileItem[0].at(0) == "-")
		{
			qDebug() << "删除文件：" + FileItem[8];
			emit g_FTPTreeWork->sigUPorDownProcessMsg(FileItem[8]);
			QString CMD = "rm " + g_FTPTreeWork->TempDeletDirPath + FileItem[8];
			g_FTPTreeWork->CMDlist = curl_slist_append(g_FTPTreeWork->CMDlist, CMD.toStdString().c_str());

		}
		else if (FileItem[0].at(0) == "d" && FileItem[8] != "." && FileItem[8] != "..")
		{
			qDebug() << "记录目录：" + FileItem[8];
			//记录他的路径 放到QStringList中 这个回调函数指定完毕后  再去遍历 估计还要递归！
			g_FTPTreeWork->DeletDirList << g_FTPHead + g_ConnetIP + "/" + g_FTPTreeWork->TempDeletDirPath + FileItem[8] + "/";
		}
	}
	return nmemb;
}

/*输出文件列表回调函数3--用于递归下载服务器文件夹*/
size_t FileList3(void *ptr, size_t size, size_t nmemb, void *data)
{
	QString str = QObject::tr((char*)ptr);
	static QString AllFtpListStr = "";
	//qDebug() << "输出文件列表3 " << str;//\"-rw-r--r--    1 root     root         6061 Mar 22 11:10 123.json\\n\""
	if (g_FTPHead == "ftp://")
	{
		if (str.endsWith("\r\n"))
		{
			AllFtpListStr += str;
			QStringList AllFileItem = AllFtpListStr.split("\r\n");
			for (size_t i = 0; i < AllFileItem.size() - 1; i++)
			{
				qDebug() << "输出文件列表3-1 " << AllFileItem[i];
				QStringList FileItem = AllFileItem[i].split(" ", QString::SkipEmptyParts);//注意 如果本身 他就是空格的话 他的大小就为1 
				if (FileItem[0].at(0) == "-")
				{
					//qDebug() << "记录下载文件：" + FileItem[8];
					g_FTPTreeWork->DownFileList << g_FTPHead + g_ConnetIP + "/" + g_FTPTreeWork->TempDownDirPath + FileItem[8];
				}
				else if (FileItem[0].at(0) == "d" && FileItem[8] != "." && FileItem[8] != "..")
				{
					//qDebug() << "记录下载文件夹：" + FileItem[8];
					g_FTPTreeWork->DownDirList << g_FTPHead + g_ConnetIP + "/" + g_FTPTreeWork->TempDownDirPath + FileItem[8] + "/";
				}
			}
			AllFtpListStr.clear();
		}
		else
		{
			//qDebug() << "FileList3-分段了";
			AllFtpListStr += str;
		}
	}
	else if (g_FTPHead == "sftp://")
	{
		QStringList FileItem = str.split(" ", QString::SkipEmptyParts);//注意 如果本身 他就是空格的话 他的大小就为1 
		FileItem[8].chop(1);//!!文件名最后的\n 去不去没有区别(对删除文件而言，但是比较的时候 后面的\n会有影响)
		if (FileItem[0].at(0) == "-")
		{
			//qDebug() << "记录下载文件：" + FileItem[8];
			g_FTPTreeWork->DownFileList << g_FTPHead + g_ConnetIP + "/" + g_FTPTreeWork->TempDownDirPath + FileItem[8];
		}
		else if (FileItem[0].at(0) == "d" && FileItem[8] != "." && FileItem[8] != "..")
		{
			//qDebug() << "记录下载文件夹：" + FileItem[8];
			g_FTPTreeWork->DownDirList << g_FTPHead + g_ConnetIP + "/" + g_FTPTreeWork->TempDownDirPath + FileItem[8] + "/";
		}
	}
	return nmemb;
}

/*获取FTP服务器文件列表 0 ok 其他curl错误码 */
int FTPTreeWork::slotGetFTPList(QString Path)
{
	curl_easy_setopt(curl, CURLOPT_NOBODY, 0);//0 文件列表能输出 反之不输出
	//!!!获取文件列表的代码和下载文件的代码几乎一致 但是用来区分是获取列表还是下载文件 就看你的路径是个文件路径还是目录路径
	curl_easy_setopt(curl, CURLOPT_URL, Path.toStdString().c_str());//sftp://192.168.100.249/mnt/nandflash/jzhs/  
	//curl_easy_setopt(curl, CURLOPT_URL, "ftp://127.0.0.1//C:\\bin\\");//sftp://192.168.100.249/mnt/nandflash/jzhs/  
	//curl_easy_setopt(curl, CURLOPT_URL,"ftp://192.168.100.249//mnt/nandflash/jzhs/bin/");//ftp://192.168.100.249//mnt/nandflash/jzhs/!!ip前后都得加个斜杠
	//curl_easy_setopt(curl, CURLOPT_URL,"ftp://192.168.100.249//mnt/nandflash/jzhs/sdata/Release/qmake/123/");
	//curl_easy_setopt(curl, CURLOPT_URL,"scp://192.168.100.249//mnt/");//scp://<my_host>:<my_port>
	QString UsrPass = g_UserName + ":" + g_UserWord;
	/*user & pwd*/
	curl_easy_setopt(curl, CURLOPT_USERPWD, UsrPass.toStdString().c_str());
	/*此函数多做数据保存的功能，如处理下载文件。
	可以通过 CURLOPT_WRITEDATA属性给默认回调函数传递一个已经打开的文件指针，用于将数据输出到文件里。 */
	//rs = curl_easy_setopt(curl, CURLOPT_WRITEDATA, nullptr);//输出文件列表 到文件指针
	//curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "NLST");//ftp默认就是这个 可以不用加
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, FileList1);//输出文件列表 
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, true);//设置为false才能开启进度条  这里关闭的意思是因为 我打开进度条 他也有反应 所以我要关闭
	//curl_easy_setopt(curl, CURLOPT_PORT, 22);//指定端口

	/*这个选项要给一个回调函数作为参数，回调函数相当于一个触发器，
	CUrl 会把每一条 response 的 header 指令传给这个函数，由函数来决定如何执行后面的步骤。这个选项要给一个回调函数作为参数，
	回调函数相当于一个触发器，CUrl 会把每一条 response 的 header 指令传给这个函数，由函数来决定如何执行后面的步骤。
	!这个函数 可有可无 但是必须有下面“CURLOPT_HEADERDATA” 而这个函数本身的指针是来自CURLOPT_HEADERDATA，只不过“CURLOPT_HEADERFUNCTION”
	操作这个文件指针的方式 是通过回调函数 所以即使没有“CURLOPT_HEADERFUNCTION”，“CURLOPT_HEADERDATA”也是一样直接写入文件
	小提示，给 CURLOPT_HEADERFUNCTION 设置回调函数的时候，文档上说要一个字符串形式的函数名作为参数。*/
	//rs = curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, write_response);//也是输出ftp服务器对客户端回应 也就是 header回应
	//rs = curl_easy_setopt(curl, CURLOPT_HEADERDATA, respfile);//输出ftp服务器对客户端的回应
	//qDebug() << "ftp 5=" << rs;

	rs = curl_easy_perform(curl);//第一次连接的时候----在这需要消耗慢点 这个就相当于把前面的参数 一次性发送到SFTP服务器；
	if (rs == 0 && g_FTPHead == "ftp://")
	{
		AddOtherListItem();
	}
	emit sigFTPListError(rs);
	CurlOptClear();

	return rs;
}

/*递归-删除文件夹--目前来说支持空，非空，多级文件夹删除---支持SFTP和FTP，对于ftp而言，这个库不能够显示隐藏文件，所以导致删除有很大的问题*/
void FTPTreeWork::slotFTPDeletDir(QString Path)
{
	//"sftp://192.168.6.38//mnt/nandflash/jzhs/confy/"
	QString DeletCmdHead;
	if (g_FTPHead == "sftp://")
	{
		DeletCmdHead = "rmdir /";
	}
	else if (g_FTPHead == "ftp://")
	{
		DeletCmdHead = "RMD /";
	}
	emit sigFTPStatusAlarm(1);//打开“勿操作”对话框
	DeletDirList.clear();
	if (CMDlist != nullptr)
	{
		curl_slist_free_all(CMDlist);
		CMDlist = nullptr;
	}
	int ret = 0;
	TempDeletDirPath.clear();
	TempDeletDirPath = "/" + Path.section("/", 4);// /mnt/nandflash/jzhs/confy/
	qDebug() << "文件夹 - 递归删除 - 开始";
	curl_easy_setopt(curl, CURLOPT_URL, Path.toStdString().c_str());
	curl_easy_setopt(curl, CURLOPT_NOBODY, 0);//0 文件列表能输出 反之不输出
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, FileList2);//输出文件列表 
	ret = curl_easy_perform(curl);
	if (ret != CURLE_OK)
	{
		emit sigFTPStatusAlarm(0);
		CurlOptClear();
		qCritical() << "文件夹-递归删除0-错误 " << ret;
		emit sigFTPDeletDirError("递归删除错误0--展开当前目录失败\n", ret);
		return;
	}
	if (CMDlist != nullptr)//！！！CMDlist为NULL去执行 也不报错 所以把这个还有另外一个去掉
	{
		curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
		curl_easy_setopt(curl, CURLOPT_POSTQUOTE, CMDlist);// CURLOPT_POSTQUOTE CURLOPT_QUOTE 我的理解出了执行的优先级不一样 其他没区别
		ret = curl_easy_perform(curl);//执行完这个之后 就一定会打印文件列表 ！！ 即使你把文件列表回调函数指针赋值为null 也不行！！
		if (ret != CURLE_OK)
		{
			emit sigFTPStatusAlarm(0);
			CurlOptClear();
			qCritical() << "文件夹-递归删除1-错误 " << curl_easy_strerror((CURLcode)ret);
			emit sigFTPDeletDirError("递归删除错误1--删除当前目录文件失败\n", ret);
			return;
		}
	}
	if (CMDlist != nullptr)
	{
		curl_slist_free_all(CMDlist);
		CMDlist = nullptr;//！！！清空后 一定要手动赋值NULL
		//!!清空命令链表之后，把请求设为NULL
	}
	curl_easy_setopt(curl, CURLOPT_POSTQUOTE, nullptr);

	curl_easy_setopt(curl, CURLOPT_NOBODY, 0);
	for (size_t i = 0; i < DeletDirList.size(); i++)//递归的核心就在这 DirList在不断的变化，遍历出文件夹就会增加内容，知道把所有的文件夹目录放进去，然后遍历结束
	{
		TempDeletDirPath = "/" + DeletDirList[i].section("/", 4);
		qDebug() << " 文件列表 " << DeletDirList[i];
		curl_easy_setopt(curl, CURLOPT_URL, DeletDirList[i].toStdString().c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, FileList2);//输出文件列表 
		int ret = curl_easy_perform(curl);
		if (ret != CURLE_OK)
		{
			emit sigFTPStatusAlarm(0);
			CurlOptClear();
			qCritical() << "文件夹-递删除2-错误 " << ret;
			emit sigFTPDeletDirError("递归删除错误2--展开此目录失败\n" + DeletDirList[i] + "\n", ret);
			return;
		}
	}
	if (CMDlist != nullptr)
	{
		curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
		curl_easy_setopt(curl, CURLOPT_POSTQUOTE, CMDlist);
		ret = curl_easy_perform(curl);
		if (ret != CURLE_OK)
		{
			emit sigFTPStatusAlarm(0);
			CurlOptClear();
			qCritical() << "文件夹-递删除3-错误 " << ret;
			emit sigFTPDeletDirError("递归删除错误3--删除内层文件失败\n", ret);
			return;
		}
	}

	//删除清空完的空文件夹 以及本来就是空的文件夹
	if (CMDlist != nullptr)
	{
		curl_slist_free_all(CMDlist);
		CMDlist = nullptr;//！！！清空后 一定要手动赋值NULL
	}
	int DirIndex = DeletDirList.size() - 1;
	for (int i = DirIndex; i >= 0; i--)
	{
		QString CMD = DeletCmdHead + DeletDirList[i].section("/", 4);
		CMDlist = curl_slist_append(CMDlist, CMD.toStdString().c_str());
	}
	CMDlist = curl_slist_append(CMDlist, QString(DeletCmdHead + Path.section("/", 4)).toStdString().c_str()); //删除最外面的文件夹
	curl_easy_setopt(curl, CURLOPT_POSTQUOTE, CMDlist);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);//0 ftp信息就简略
	ret = curl_easy_perform(curl);
	if (ret != CURLE_OK)
	{
		emit sigFTPStatusAlarm(0);
		CurlOptClear();
		qCritical() << "文件夹-递删除4-错误 " << ret;
		emit sigFTPDeletDirError("递归删除错误4--删除所有文件夹失败\n", ret);
		return;
	}
	DeletDirList.clear();
	if (CMDlist != nullptr)
	{
		curl_slist_free_all(CMDlist);
		CMDlist = nullptr;
	}
	curl_easy_setopt(curl, CURLOPT_POSTQUOTE, nullptr);
	qDebug() << "文件夹-递归删除-结束";
	emit sigFTPStatusAlarm(0);
	emit sigFTPListRefresh();
}

/*递归-下载文件夹--目前来说支持空，非空，多级文件夹下载*/
void FTPTreeWork::slotFTPDownDir(QString RemotePath, QString TargetPath)//sftp:/192.168.6.38/mnt/nandflash/jzhs/Release/
{
	int ret = 0;
	emit sigFTPStatusAlarm(1);//打开“勿操作”对话框
	QString CurrentDirName = RemotePath.section("/", -2);//Release/
	qDebug() << "递归-下载文件夹 " << RemotePath;
	//在本地创建文件夹
	//if (slotFtpGetFile("", "", "", "../Project/" + g_ProjectName + "/" + g_DevIP + "/" + CurrentDirName, false, true, false) == -2)
	if (slotFtpGetFile("", "", "", TargetPath + "/" + CurrentDirName, false, true, false, false) == -2)
	{
		emit sigFTPDownDirError("递归下载错误0--创建 " + CurrentDirName + " 目录失败\n", ret);
		return;//创建文件夹失败，结束下载
	}
	DownDirList.clear();
	DownFileList.clear();
	TempDownDirPath = "/" + RemotePath.section("/", 4);// /mnt/nandflash/jzhs/Release/
	qDebug() << "文件夹 - 递归下载 - 开始";
	curl_easy_setopt(curl, CURLOPT_URL, RemotePath.toStdString().c_str());
	curl_easy_setopt(curl, CURLOPT_NOBODY, 0);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, FileList3);//输出文件列表 
	ret = curl_easy_perform(curl);
	if (ret != CURLE_OK)
	{
		emit sigFTPStatusAlarm(0);
		CurlOptClear();
		qCritical() << "文件夹-递归下载1-错误 " << ret;
		emit sigFTPDownDirError("递归下载错误1--展开当前目录失败\n", ret);
		return;
	}
	for (size_t i = 0; i < DownDirList.size(); i++)//递归的核心就在这 DirList在不断的变化，遍历出文件夹就会增加内容，知道把所有的文件夹目录放进去，然后遍历结束
	{
		TempDownDirPath = "/" + DownDirList[i].section("/", 4);
		qDebug() << " 文件夹列表 " << DownDirList[i];
		curl_easy_setopt(curl, CURLOPT_URL, DownDirList[i].toStdString().c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, FileList3);//输出文件列表
		int ret = curl_easy_perform(curl);
		if (ret != CURLE_OK)
		{
			emit sigFTPStatusAlarm(0);
			CurlOptClear();
			qCritical() << "文件夹-递下载2-错误 " << ret;
			emit sigFTPDownDirError("递归下载错误2--展开此目录失败\n" + DownDirList[i] + "\n", ret);
			return;
		}
	}
	//以上对所有目录遍历完毕，下面开始下载
	for (size_t DirIndex = 0; DirIndex < DownDirList.size(); DirIndex++)//在本地创建文件夹
	{
		qDebug() << "创建文件夹目录 " << TargetPath + "/" + DownDirList[DirIndex].mid(DownDirList[DirIndex].indexOf(CurrentDirName));
		if (slotFtpGetFile("", "", "", TargetPath + "/" + DownDirList[DirIndex].mid(DownDirList[DirIndex].indexOf(CurrentDirName)), false, true, false, false) == -2)
		{
			//发信号，提示是创建哪个文件夹失败
			emit sigFTPDownDirError("递归下载错误3--创建 " + CurrentDirName + " 目录失败\n", ret);
			return;//创建文件夹失败，结束下载
		}
	}
	for (size_t FileIndex = 0; FileIndex < DownFileList.size(); FileIndex++)//下载文件
	{
		//qDebug() << "下载文件 " << "../Download/" + DownFileList[FileIndex].mid(DownFileList[FileIndex].indexOf(CurrentDirName));//sftp://192.168.6.38//mnt/nandflash/jzhs/Release/1/11/a.c
		emit sigUPorDownProcessMsg(DownFileList[FileIndex].mid(DownFileList[FileIndex].indexOf(CurrentDirName)));
		if (ret = slotFtpGetFile(DownFileList[FileIndex], g_UserName, g_UserWord, TargetPath + "/" + DownFileList[FileIndex].mid(DownFileList[FileIndex].indexOf(CurrentDirName)), false, true, false, false) != 0)
		{
			//发信号，提示是创建哪个文件夹失败
			emit sigFTPDownDirError("递归下载错误4--下载 " + CurrentDirName + " 文件失败\n", ret);
			emit sigFTPStatusAlarm(0);
			return;
		}
	}
	if (ret == 0)
	{
		emit sigFTPDownDirError("递归下载成功--下载\n", ret);
	}

	DownDirList.clear();
	DownFileList.clear();
	CurlOptClear();
	qDebug() << "文件夹-递归下载--结束";
	emit sigFTPStatusAlarm(0);
	emit sigFTPListRefresh();
}

/*执行FTP命令  0 ok 其他curl错误码 --有重命名,删除文件，新建文件夹 */
int FTPTreeWork::slotFTPCMDExec(const char* CMD)
{
	//重命名一旦失败了 整个库内部命令就会变乱 不知道是不是这个库的问题
	qDebug() << CMD;
	if (g_FTPHead == "ftp://")//在这加这个的原因，是因为在ftp的情况下，出现下载文件后，执行命令，就会崩溃的现象，所以重新清空了一下，而sftp就不会出现这些问题
	{
		curl_easy_reset(curl);
		curl_easy_setopt(curl, CURLOPT_URL, g_FTPCurrentPath.toStdString().c_str());
		QString UsrPass = g_UserName + ":" + g_UserWord;
		curl_easy_setopt(curl, CURLOPT_USERPWD, UsrPass.toStdString().c_str());
	}
	if (CMDlist != nullptr)
	{
		curl_slist_free_all(CMDlist);//清空命令列表 - 此函数没有返回值
		CMDlist = nullptr;//清空后 一定要手动赋值NULL
	}
	QString CMD2 = QString(QLatin1String(CMD));
	//启用时将不对HTML中的BODY部分进行输出。
	//！ 默认为0 改成0之后  删除文件后回调函数会自动拿到一次最新的文件列表 但是如果是1 删除之后  刷新文件列表回调函数没有反应 ！！！！
	curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
	//curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, FileList1);//命令操作后，他会自己输出文件列表，我这个文件列表放到我的窗口中
	if (g_FTPHead == "sftp://")
	{
		CMDlist = curl_slist_append(CMDlist, CMD);//rm mkdir rename rmdir chmod atime pwd touch
	}
	else if (g_FTPHead == "ftp://" && CMD2.contains("RNTO"))//ftp的重命名
	{
		CMDlist = curl_slist_append(CMDlist, CMD2.mid(0, CMD2.indexOf("RNTO")).toStdString().c_str());//!!!!在FTP的时候，这个一定只能隔一个空格 所有命令都是这样 而且不用加那么长的路径
		CMDlist = curl_slist_append(CMDlist, CMD2.mid(CMD2.indexOf("RNTO")).toStdString().c_str());
	}
	else if ((g_FTPHead == "ftp://") && (CMD2.contains("DELE") || (CMD2.contains("MKD")) || (CMD2.contains("SITE"))))//ftp的其他命令
	{
		CMDlist = curl_slist_append(CMDlist, CMD);
	}
	curl_easy_setopt(curl, CURLOPT_POSTQUOTE, CMDlist);// CURLOPT_POSTQUOTE CURLOPT_QUOTE 我的理解出了执行的优先级不一样 其他没区别
	//curl_easy_setopt(curl, CURLOPT_QUOTE, CMDlist); //传递指向 FTP 或 SFTP 命令链接列表的指针，以在您的请求之前传递给服务器。这将在发出任何其他命令之前完成（甚至在 FTP 的 CWD 命令之前）
	//rs = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);//输出文件列表 
	rs = curl_easy_perform(curl);//执行完这个之后 就一定会打印文件列表 ！！ 即使你把文件列表回调函数指针赋值为null 也不行！！
		//qDebug() << "命令执行成功";
		//g_FTPTreeList->qtw_FTPListWidget->clear();//！！在非ui线程中 调用ui线程函数 操作ui  发送崩溃  即使想这样 非在子线程中申请部件，而是调用主线程部件 也不行\
		"QObject::connect: Cannot queue arguments of type 'QItemSelection'\n(Make sure 'QItemSelection' is registered using qRegisterMetaType().)"

	/* Qt::BlockingQueuedConnection会死锁 因为slotRK_FTPFreshList这这句话在一个线程*/
	//QMetaObject::invokeMethod(g_FTPTreeList, "slotRK_FTPFreshList", Qt::QueuedConnection);//把对UI的操作再发送到主线程去 

	//emit sigFTPListRefresh();
	CurlOptClear();
	emit sigFTPCMDError(rs);
	return rs;
}

/*关闭Curl的链接*/
void FTPTreeWork::slotCloseFTPConnect()
{
	curl_easy_cleanup(curl);
	curl_global_cleanup();
}




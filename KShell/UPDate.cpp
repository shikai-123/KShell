#include "UPDate.h"
#pragma execution_character_set("utf-8")
extern void Delay(int mSeconds);
bool UPDate::m_IsUpdatedEXE = false;
UPDate::UPDate()
{
	manager = new QNetworkAccessManager(this);
	myfile = new QFile(this);
}

UPDate::~UPDate()
{
	OleUninitialize();
}

/*下载完毕后，把内容写到文件中*/
void UPDate::slotReadyRead()
{
	while (!reply->atEnd())
	{
		QByteArray ba = reply->readAll();
		myfile->write(ba);
	}
}

/*下载结束*/
void UPDate::slotFinished()
{
	myfile->close();
	m_bDownFinish = true;
	/*
	Http的各种标头
	qDebug() << reply->hasRawHeader("Content-Encoding ");
	qDebug() << reply->hasRawHeader("Content-Language");
	qDebug() << reply->hasRawHeader("Content-Length");
	qDebug() << reply->hasRawHeader("Content-Type");
	qDebug() << reply->hasRawHeader("Last-Modified");
	qDebug() << reply->hasRawHeader("Expires");
	*/
}

/*下载进度*/
void UPDate::slotDownloadProgress(qint64 recv_total, qint64 all_total)
{
	//qDebug() << recv_total << "  " << all_total;
}

/*下载错误*/
void UPDate::slotNetErr(QNetworkReply::NetworkError code)
{
	qDebug() << "err " << code;
	m_HasErr = true;
	m_NetErrStr = QMetaEnum::fromType<QNetworkReply::NetworkError>().valueToKey(code);
}

/*定时器到时候，启动升级*/
void UPDate::slotTimeToUpdate()
{
	emit sitStopTimerForUpdate();
	slotStartUPDate();
}

/*根据形参来下载文件 0ok -1下载列表为空 -2ole初始化失败 -3打开文件失败 -4下载失败*/
int UPDate::slotUPdateforList(QStringList &DownFileList)
{
	if (DownFileList.isEmpty())
	{
		DownFileList.clear();
		return -1;
	}
	HRESULT r = OleInitialize(0);//这是是windows函数，不加就会在运行的时候报错。
	if (r != S_OK && r != S_FALSE)
	{
		qWarning("Qt:初始化Ole失败（error %x）", (unsigned int)r);
		return -2;
	}
	int FileSize = DownFileList.size();//节约性能
	for (size_t FileIndex = 0; FileIndex < FileSize; FileIndex++)
	{
		m_bDownFinish = false;
		QNetworkRequest request;
		QString url = DownFileList[FileIndex];
		request.setUrl(url);
		reply = manager->get(request);															//发送请求
		connect(reply, &QNetworkReply::readyRead, this, &UPDate::slotReadyRead);                //可读
		connect(reply, &QNetworkReply::finished, this, &UPDate::slotFinished);                  //结束
		connect(reply, &QNetworkReply::downloadProgress, this, &UPDate::slotDownloadProgress);  //大小
		connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), this, &UPDate::slotNetErr);//异常
		QStringList list = url.split("/");
		QString filename = list.at(list.length() - 1);
		QString file = "./../Update/" + filename;
		myfile->setFileName(file);
		emit sigUpdateProcess(FileIndex + 1, FileSize, filename);//发送下载进程信号
		bool ret = myfile->open(QIODevice::WriteOnly | QIODevice::Truncate);					//创建文件
		if (!ret)
		{
			emit sigUpdateProcess(FileIndex + 1, FileSize, filename + "\n打开失败：" + m_NetErrStr + "\n请重试");
			myfile->remove();
			return -3;
		}
		while (!m_bDownFinish)
		{
			Delay(100);
			//qDebug() << "下载中 " << file;
			if (m_HasErr)
			{
				emit sigUpdateProcess(FileIndex + 1, FileSize, filename + "\n下载错误：" + m_NetErrStr + "\n请重试");
				myfile->remove();
				m_HasErr = false;
				return -4;
			}
		}
		if (myfile->size() == 0)//下载失败尤其是exe。
		{
			emit sigUpdateProcess(-1, -1, myfile->fileName() + "下载失败,大小为0,gitee又抽风了，手动下载就行了");
			return -5;
		}
		if (filename == "CJQTool.zip")
		{
			m_IsUpdatedEXE = true;
			zipReader("./../Update/CJQTool.zip", "./../Update/");
		}
	}
	return 0;
}

/*读取升级列表xml文件*/
void UPDate::ReadUpdateList(QString Path, QVector<UpdateFile>& UpdateInfo)
{
	QDomDocument xDoc;//创建QDomDocument对象
	QFile file(Path);//指定读取的xml文件路径
	//xml文件以只读方式打卡
	if (!file.open(QIODevice::ReadOnly))
	{
		qDebug() << "打开升级列表失败：" + Path;
		emit sigUpdateProcess(-1, -1, "打开升级列表失败");
		return;
	}
	//调用setContent函数设置数据源
	if (!xDoc.setContent(&file)) {
		file.close();
		qDebug() << "解析升级列表失败：" + Path;
		emit sigUpdateProcess(-1, -1, "解析升级列表失败");
		return;
	}
	file.close();
	QDomElement docElem = xDoc.documentElement();//获取xDoc中的QDomElement对象
	QDomNode node = docElem.firstChild();//获取docElem的根节点
	while (!node.isNull())
	{
		UpdateFile TmpInfo;
		QDomElement  childNode1 = node.toElement();//子节点的全数据---属性和值
		QString attri1 = TmpInfo.FileName = childNode1.attribute("name");//子节点-属性
		QDomNode cChildNode = childNode1.firstChild();//子节点-值--子子节点
		QDomElement e = cChildNode.toElement();//子节点的全数据---属性和值--!!对于子子节点的遍历，我去掉了循环，这样得把xml固定下来
		TmpInfo.Version = e.firstChild().nodeValue(); //子子节点-值
		TmpInfo.RemotePath = cChildNode.nextSibling().toElement().firstChild().nodeValue();//获取“子节点”下一节“子子节点”
		TmpInfo.LocalPath = cChildNode.nextSibling().nextSibling().toElement().firstChild().nodeValue();
		UpdateInfo.push_back(TmpInfo);
		node = node.nextSibling();//获取“根节点”下一节“子节点”
	}
}

/*解压zip文件 zipPath压缩包路径  zipDir解压到路径  1ok 0no */
bool UPDate::zipReader(QString zipPath, QString zipDir)
{
	QFile tempDir;
	if (!tempDir.exists(zipPath))
	{
		emit sigUpdateProcess(-1, -1, "CJQTool.zip不存在");
		return false;
	}
	QZipReader reader(zipPath);
	if (reader.extractAll(zipDir))
	{
		reader.close();//!如果不关闭的话。下面的删除就会失败，或者等到reader析构后再删除
		qDebug() << "移除压缩包" << zipPath << QFile::remove(zipPath);//解压成功后，移除压缩包
		return true;
	}
	else
	{
		reader.close();
		emit sigUpdateProcess(-1, -1, "CJQTool.zip解压失败");
		return false;
	}
}

/*移动文件*/
void UPDate::MoveFiles()
{
	QString MoveErr = "升级移动失败:\n";
	foreach(UpdateFile FileList, RemoteUpdateInfo)//注意这是遍历的云升级列表--升级是按照云的目录去升级
	{
		if (FileList.FileName == "CJQTool.exe" || FileList.FileName == "README.md")
		{
			continue;//如果是CJQTool.exe就不移动他，他有单独的移动脚本去负责 README.md就在update目录
		}
		if (FileList.IsUpdateFile)
		{
			//！删除的文件是根据本地conf的目录来的
			QFile::remove(FileList.LocalPath + "/" + FileList.FileName);//加“/”的原因，是怕在xml中没有添加最后的“/”，导致目录失败
			if (QFile::rename("./../Update/" + FileList.FileName, FileList.LocalPath + "/" + FileList.FileName))//新路径上如果已经有了文件，就会移动失败
			{
			}
			else//移动失败
			{
				MoveErr += FileList.FileName + "\n";
			}
		}
		else if (FileList.IsNewFile)
		{
			QFile::remove(FileList.LocalPath + "/" + FileList.FileName);//!对于新增文件而言，一般来说本地都没有，但是为了处理异常情况，也是要删除一下
			if (QFile::rename("./../Update/" + FileList.FileName, FileList.LocalPath + "/" + FileList.FileName))//新路径上如果已经有了文件，就会移动失败
			{
			}
			else//移动失败
			{
				MoveErr += FileList.FileName + "\n";
			}
		}
	}
	if (MoveErr.size() != 8)
	{
		qDebug() << MoveErr;
		emit sigUpdateProcess(-1, -1, MoveErr);
	}
}

/*点击升级后，执行升级函数---升级程序主入口*/
void UPDate::slotStartUPDate()
{
	QString DeletFaultFileList;//删除失败文件列表
	QFile::remove("./../Update/UpdateList.xml");//保证每次下载都是最新的
	DownFileList.clear();
	DownFileList << "https://gitee.com/shikai1995/KShellUpdate/raw/master/UpdateList.xml";
	if (slotUPdateforList(DownFileList) != 0)//下载失败
	{
		emit sigUpdateProcess(-1, -1, "升级列表获取失败");
		return;
	}
	DownFileList.clear();
	RemoteUpdateInfo.clear();
	LocalUpdateInfo.clear();
	ReadUpdateList("./../Update/UpdateList.xml", RemoteUpdateInfo);
	ReadUpdateList("./../conf/UpdateList.xml", LocalUpdateInfo);
#if 0

	if (RemoteUpdateInfo.size() == LocalUpdateInfo.size())//远程升级列表和本地升级列表中升级数量一致----那就比较其中版本的差异
	{
		int XmlSize = RemoteUpdateInfo.size();
		for (size_t FileIndex = 0; FileIndex < XmlSize; FileIndex++)
		{
			if (RemoteUpdateInfo[FileIndex].Version.toFloat() > LocalUpdateInfo[FileIndex].Version.toFloat())//低于云端版本
			{
				RemoteUpdateInfo[FileIndex].IsUpdateFile = true;
				DownFileList << RemoteUpdateInfo[FileIndex].RemotePath;
				qDebug() << "加入升级列表：" << RemoteUpdateInfo[FileIndex].FileName;
			}
			else if (RemoteUpdateInfo[FileIndex].Version.toFloat() < LocalUpdateInfo[FileIndex].Version.toFloat())//高于云端版本
			{
				qDebug() << "该文件本地版本高于云版本！本地版本：" << LocalUpdateInfo[FileIndex].Version << "  云版本：" << RemoteUpdateInfo[FileIndex].Version;
			}
			else//版本一致
			{
				qDebug() << "版本一致：" << RemoteUpdateInfo[FileIndex].FileName;
				emit sigUpdateProcess(0, 0, "已是最新版本");
			}
		}
		if (DownFileList.isEmpty())
		{
			emit sigUpdateProcess(0, 0, "已是最新版本");
		}
		else
		{
			if (slotUPdateforList(DownFileList) != 0)//如果有多个文件需要升级，只要有一个下载失败---就不进行文件移动，这样虽然武断一些，但是代码清晰度好很多
			{
				return;
			}
			MoveFiles();
		}
	}
	else if (RemoteUpdateInfo.size() != LocalUpdateInfo.size())
	{
		qDebug() << "配置数量不一致";
		size_t Rsize = RemoteUpdateInfo.size();
		size_t Lsize = LocalUpdateInfo.size();
		if (Rsize > Lsize)//云升级列表文件数量比本地多--大部分都是这种情况
		{
			for (size_t RemoteFileIndex = 0; RemoteFileIndex < Rsize; RemoteFileIndex++)
			{
				for (size_t LocalFileIndex = 0; LocalFileIndex < Lsize; LocalFileIndex++)
				{
					if (RemoteUpdateInfo[RemoteFileIndex].FileName == LocalUpdateInfo[LocalFileIndex].FileName)
					{
						goto NextLoop;
					}
				}
				qDebug() << "远程新增文件：" << RemoteUpdateInfo[RemoteFileIndex].FileName;
				DownFileList << RemoteUpdateInfo[RemoteFileIndex].RemotePath;
				RemoteUpdateInfo[RemoteFileIndex].IsNewFile = true;
			NextLoop:
				continue;
			}
			if (slotUPdateforList(DownFileList) != 0)
			{
				return;
			}
			MoveFiles();
		}
		else//
		{

		}
	}
#endif // 0

	size_t Rsize = RemoteUpdateInfo.size();
	size_t Lsize = LocalUpdateInfo.size();
	/*进行比较，不在通过大小比较，然后再判断。
	云有的，本地有的，比较版本，本地版本低下载，本地版本高报错结束升级。
	云有的，本地没有的加入下载列表，
	云没有，本地有的删掉xml对应的，删掉本地文件*/
	QStringList RemoteFileList;
	QStringList LocalFileList;
	foreach(UpdateFile RemoteFile, RemoteUpdateInfo)
	{
		RemoteFileList << RemoteFile.FileName;
	}
	foreach(UpdateFile LocalFile, LocalUpdateInfo)
	{
		LocalFileList << LocalFile.FileName;
	}
	for (size_t RFileIndex = 0; RFileIndex < Rsize; RFileIndex++)
	{
		if (LocalFileList.contains(RemoteUpdateInfo[RFileIndex].FileName))//云有的，本地有的---进一步比较两者的版本
		{
			foreach(UpdateFile LocalFile, LocalUpdateInfo)
			{
				if (RemoteUpdateInfo[RFileIndex].FileName == LocalFile.FileName)
				{
					if (RemoteUpdateInfo[RFileIndex].Version.toFloat() > LocalFile.Version.toFloat())//云端版本高--加入升级列表
					{
						RemoteUpdateInfo[RFileIndex].IsUpdateFile = true;
						DownFileList << RemoteUpdateInfo[RFileIndex].RemotePath;
						qDebug() << "云端版本高---加入升级列表：" << RemoteUpdateInfo[RFileIndex].FileName;
					}
					else if (RemoteUpdateInfo[RFileIndex].Version.toFloat() < LocalFile.Version.toFloat())//云端版本低---报错
					{
						emit sigUpdateProcess(-1, -1, "本地版本怎么比云端的版本还高？\n本地版本：" + LocalFile.Version + "\n云端版本：" + RemoteUpdateInfo[RFileIndex].Version + "\n" + RemoteUpdateInfo[RFileIndex].FileName);
						return;
					}
					//版本相同，就不做任何操作
				}
			}
		}
		else//云有的，本地没有----添加到下载列表中
		{
			RemoteUpdateInfo[RFileIndex].IsNewFile = true;
			DownFileList << RemoteUpdateInfo[RFileIndex].RemotePath;
			qDebug() << "本地没有该文件---加入升级列表：" << RemoteUpdateInfo[RFileIndex].FileName;
		}
	}
	if (!DownFileList.isEmpty())
	{
		if (slotUPdateforList(DownFileList) != 0)//如果有多个文件需要升级，只要有一个下载失败---就不进行文件移动，这样虽然武断一些，但是代码清晰度好很多
		{
			return;
		}
		MoveFiles();
	}
	foreach(UpdateFile LocalFile, LocalUpdateInfo)
	{
		if (RemoteFileList.contains(LocalFile.FileName))
		{
		}
		else//云没有，本地有----删掉本地文件,而删掉xml对应的节点，则在最后的移动xml文件中得到体现
		{
			if (!QFile::remove(LocalFile.LocalPath + LocalFile.FileName))//保证更新都能得到最新的升级列表
			{
				DeletFaultFileList += LocalFile.FileName;
			}
		}
	}
	if (!DeletFaultFileList.isEmpty())
	{
		emit sigUpdateProcess(-1, -1, "自动更新--删除废弃文件失败:\n" + DeletFaultFileList);
		return;
	}
	QFile::remove("./../conf/UpdateList.xml");//保证更新都能得到最新的升级列表
	QFile::rename("./../Update/UpdateList.xml", "./../conf/UpdateList.xml");
	if (DownFileList.isEmpty())
	{
		emit sigUpdateProcess(0, 0, "已是最新版本");
		return;
	}
	else
	{
		emit sigUpdateProcess(0, 0, "更新成功，重启后生效");
	}
}

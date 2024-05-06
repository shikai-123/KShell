#include "FTPTreeList.h"
#include "FTPTreeWork.h"
#include "SSHWindow.h" 
#include "CJQConf.h"
#pragma execution_character_set("utf-8")
extern QString g_FTPCurrentPath;
extern FTPTreeWork * g_FTPTreeWork;
extern QString g_ProjectRegion, g_DevIP, g_ConnetIP, g_DevPort, g_UserName, g_UserWord, g_UserListNote, g_FTPType, g_ProjectName, g_FTPHead;//用于采集登录信息
extern QString g_SSHFTPJzhsDefaultPath;
extern SSHWindow* g_SSHWindow;//状态窗口的全局指针
extern bool g_IsSHHTogetherFTP;//ssh ftp窗口是否聚合
extern void Delay(int mSeconds);
extern OtherToolsSetting g_OtherToolsSetting;
bool FTPTreeList::FTPOnce = false;
bool FTPTreeList::SFTPOnce = false;
FTPTreeList::FTPTreeList()
{
	Init();
}
using namespace Qt;
/*文件管理初始化*/
void FTPTreeList::Init()
{
	this->setWindowTitle("FTP文件管理器");
	this->resize(1000, 400);
	vbl_RemoteFileLayout = new QVBoxLayout();
	vbl_LoaclFileLayout = new QVBoxLayout();
	vbl_AllFtpLayout = new QVBoxLayout();

	/*初始化左右子窗口*/
	qw_RemotFileWidget = new QWidget(this);
	qw_LocalFileWidget = new QWidget(this);

	/*本地文件窗口设置*/
	qtv_LocalFileTree = new QTreeView(qw_LocalFileWidget);
	vbl_LoaclFileLayout->addWidget(qtv_LocalFileTree);
	qtv_LocalFileTree->header()->setStretchLastSection(true);//排序..暂不好用
	qtv_LocalFileTree->header()->setSortIndicator(0, Qt::AscendingOrder);
	qtv_LocalFileTree->header()->setSortIndicatorShown(true);
	qtv_LocalFileTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

#if QT_VERSION >= 0x050000
	qtv_LocalFileTree->header()->setSectionsClickable(true);
#else
	qtv_LocalFileTree->header()->setClickable(true);
#endif

	/*开启支持拖拽*/
	setAcceptDrops(true);//启用放下操作

	/*远程FTP文件列表*/
	qtw_FTPListWidget = new QTreeWidget(qw_RemotFileWidget);
	QStringList TitleList;
	TitleList << "名字" << "大小" << "文件夹数" << "最后修改时间" << "权限";
	qtw_FTPListWidget->setHeaderLabels(TitleList);//添加表头
	/*按照第1和4列排序*/
	qtw_FTPListWidget->setSortingEnabled(1);
	qtw_FTPListWidget->header()->setSortIndicatorShown(true);
	qtw_FTPListWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

	connect(qtw_FTPListWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(slotCheckFile(QTreeWidgetItem *, int)));//双击选中的文件
	vbl_RemoteFileLayout->addWidget(qtw_FTPListWidget);

	/*进度条*/
	qpb_DownPro = new QProgressBar(this);
	qpb_DownPro->setRange(0, 100);
	qpb_DownPro->setOrientation(Qt::Horizontal);  //水平方向
	qpb_DownPro->setMaximumHeight(30);
	qpb_DownPro->setValue(0);//设置进度条的进度
	//qpb_DownPro->setFormat(tr("Current progress : %1%").arg(QString::number(10.0, 'f', 1)));//设置进度条的文字描述的值
	qpb_DownPro->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);//左对齐 居中对齐

	/*水平分割器*/
	qs_SplitterForL_R = new QSplitter(Qt::Horizontal, this);
	qs_SplitterForL_R->addWidget(qw_LocalFileWidget);
	qs_SplitterForL_R->addWidget(qw_RemotFileWidget);

	/*FTP所有窗口布局*/
	qw_LocalFileWidget->setLayout(vbl_LoaclFileLayout);
	qw_RemotFileWidget->setLayout(vbl_RemoteFileLayout);
	vbl_AllFtpLayout->addWidget(qs_SplitterForL_R);
	vbl_AllFtpLayout->addWidget(qpb_DownPro);
	setLayout(vbl_AllFtpLayout);

	/*“过程窗口”初始化*/
	MessageInit();

	/*新建文件、文件夹窗口  自定义 QMessageBox */
	qe_NewFile = new QLineEdit(this);
	qmb_NewFileWidget = new QMessageBox(QMessageBox::Information, "新建", "", QMessageBox::Ok | QMessageBox::Cancel, this);
	dynamic_cast<QGridLayout *>(qmb_NewFileWidget->layout())->addWidget(qe_NewFile, 0, 1, 1, 2);//!!!!学习

	/*重命名文件 自定义QMessageBox*/
	qe_RenameFile = new QLineEdit(this);
	qmb_RenameWidget = new QMessageBox(QMessageBox::Information, "重命名", "", QMessageBox::Ok | QMessageBox::Cancel, this);
	dynamic_cast<QGridLayout *>(qmb_RenameWidget->layout())->addWidget(qe_RenameFile, 0, 1, 1, 2);
	connect(this, SIGNAL(sigClearFileList()), this, SLOT(slotClearFileList()));//每次打开文件列表 都会清空一次

	/*文件监视*/
	qfs_FileWatcher = new QFileSystemWatcher(this);
	connect(qfs_FileWatcher, SIGNAL(fileChanged(QString)), this, SLOT(slotMonitorFile(QString)), Qt::UniqueConnection);
	/*在这把FTP窗口放到SHH的原因，是因为FTPList对象的创建，是在SSHWindow后*/
	if (g_IsSHHTogetherFTP)//!!!这种用法非常的不好--几乎不是这么用的
	{
		qs_litter = new QSplitter(Qt::Vertical, g_SSHWindow);//1、创建SSH窗口的竖直分裂器
		qs_litter->addWidget(g_SSHWindow->ui.splitter_2);//2、新的分裂器添加原先的分裂器--这个分裂器中有原先所有的部件
		qs_litter->addWidget(this);//3、然后，新的分裂器添加这个窗口
		g_SSHWindow->ui.gridLayout->addWidget(qs_litter);// 4、把这个新的分裂器放到SSH窗口的布局中。 !!这个地方是一定要有的，要不然界面没有布局！！


		g_SSHWindow->ui.splitter_2->setStretchFactor(0, 1);//设置分割比例
		g_SSHWindow->ui.splitter_2->setStretchFactor(1, 1);
		qs_litter->setStretchFactor(0, 1);
		qs_litter->setStretchFactor(1, 1);

	}
	LoadLocalFileTree();//加载本地文件树
}

/*右键菜单*/
void FTPTreeList::contextMenuEvent(QContextMenuEvent *)
{
	if (qw_RemotFileWidget->underMouse())
	{
		QMenu *menu = new QMenu(this);
		QScopedPointer<QAction> qa_RootDir(new QAction("根目录", this));
		qa_RootDir->setIcon(QIcon(":/img/首页.png"));
		connect(qa_RootDir.get(), SIGNAL(triggered()), this, SLOT(slotRK_FTPGetRootDir()), Qt::UniqueConnection);//！！其实不加 也行 因为每次打开都会建立新的对象qa_RefreshList

		QScopedPointer<QAction> qa_HomeOrConnect(new QAction("首页//连接", this));
		qa_HomeOrConnect->setIcon(QIcon(":/img/首页.png"));
		connect(qa_HomeOrConnect.get(), SIGNAL(triggered()), this, SLOT(slotRK_FTPHome_Connect()), Qt::UniqueConnection);//！！其实不加 也行 因为每次打开都会建立新的对象qa_RefreshList

		QScopedPointer<QAction> qa_RefreshList(new QAction("刷新", this));
		qa_RefreshList->setIcon(QIcon(":/img/还原.png"));
		connect(qa_RefreshList.get(), SIGNAL(triggered()), this, SLOT(slotRK_FTPFreshList()), Qt::UniqueConnection);//！！其实不加 也行 因为每次打开都会建立新的对象qa_RefreshList

		QScopedPointer<QAction> qa_DownToProjectDir(new QAction("下载工程目录", this));
		qa_DownToProjectDir->setIcon(QIcon(":/img/下载.png"));
		connect(qa_DownToProjectDir.get(), SIGNAL(triggered()), this, SLOT(slotRK_FTPDown()), Qt::UniqueConnection);

		QScopedPointer<QAction> qa_DownToLeftDir(new QAction("下载左侧目录", this));
		qa_DownToLeftDir->setIcon(QIcon(":/img/下载.png"));
		connect(qa_DownToLeftDir.get(), SIGNAL(triggered()), this, SLOT(slotRK_FTPDownToLeftDir()), Qt::UniqueConnection);

		QScopedPointer<QAction> qa_DownAndOpen(new QAction("查看", this));
		qa_DownAndOpen->setIcon(QIcon(":/img/查看.png"));
		connect(qa_DownAndOpen.get(), SIGNAL(triggered()), this, SLOT(slotRK_FTPDownAndOpen()), Qt::UniqueConnection);

		QScopedPointer<QAction> qa_DownAndEidt(new QAction("编辑", this));
		qa_DownAndEidt->setIcon(QIcon(":/img/编写.png"));
		connect(qa_DownAndEidt.get(), SIGNAL(triggered()), this, SLOT(slotRK_FTPDownAndEdit()), Qt::UniqueConnection);

		QScopedPointer<QAction> qa_UpFile(new QAction("上传文件", this));
		qa_UpFile->setIcon(QIcon(":/img/上传文件.png"));
		connect(qa_UpFile.get(), SIGNAL(triggered()), this, SLOT(slotRK_FTPUPFile()), Qt::UniqueConnection);

		QScopedPointer<QAction> qa_UpDir(new QAction("上传文件夹", this));
		qa_UpDir->setIcon(QIcon(":/img/上传文件夹.png"));
		connect(qa_UpDir.get(), SIGNAL(triggered()), this, SLOT(slotRK_FTPUPDir()), Qt::UniqueConnection);

		QScopedPointer<QAction> qa_Delet(new QAction("删除", this));
		qa_Delet->setIcon(QIcon(":/img/垃圾桶1.png"));
		connect(qa_Delet.get(), SIGNAL(triggered()), this, SLOT(slotRK_FTPDelet()), Qt::UniqueConnection);

		QScopedPointer<QAction> qa_Rename(new QAction("重命名", this));
		qa_Rename->setIcon(QIcon(":/img/重命名.png"));
		connect(qa_Rename.get(), SIGNAL(triggered()), this, SLOT(slotRK_FTPReName()), Qt::UniqueConnection);

		QScopedPointer<QAction> qa_NewDir(new QAction("新建目录", this));
		qa_NewDir->setIcon(QIcon(":/img/文件夹.png"));
		connect(qa_NewDir.get(), SIGNAL(triggered()), this, SLOT(slotRK_FTPNewDir()), Qt::UniqueConnection);

		QScopedPointer<QAction> qa_AddPermission(new QAction("增加执行权限", this));
		qa_AddPermission->setIcon(QIcon(":/img/点赞.png"));
		connect(qa_AddPermission.get(), SIGNAL(triggered()), this, SLOT(slotRK_FTPAddPermission()), Qt::UniqueConnection);

		QScopedPointer<QAction> qa_ViewDataFile_Zlib(new QAction("查看数据文件(压缩)", this));
		qa_ViewDataFile_Zlib->setIcon(QIcon(":/img/查看.png"));
		connect(qa_ViewDataFile_Zlib.get(), SIGNAL(triggered()), this, SLOT(slotRK_ReadDataFileZlib()), Qt::UniqueConnection);

		QScopedPointer<QAction> qa_ViewDataFile_NoZlib(new QAction("查看数据文件(非压缩)", this));
		qa_ViewDataFile_NoZlib->setIcon(QIcon(":/img/查看.png"));
		connect(qa_ViewDataFile_NoZlib.get(), SIGNAL(triggered()), this, SLOT(slotRK_ReadDataFileNoZlib()), Qt::UniqueConnection);

		//menu->addAction(qa_RootDir.get());
		menu->addAction(qa_HomeOrConnect.get());
		menu->addAction(qa_RefreshList.get());
		menu->addSeparator();
		menu->addAction(qa_DownAndEidt.get());
		menu->addAction(qa_DownAndOpen.get());
		menu->addAction(qa_DownToProjectDir.get());
		menu->addAction(qa_DownToLeftDir.get());
		menu->addAction(qa_UpFile.get());
		menu->addAction(qa_UpDir.get());
		menu->addSeparator();
		menu->addAction(qa_Delet.get());
		menu->addSeparator();
		menu->addAction(qa_Rename.get());
		menu->addAction(qa_NewDir.get());
		menu->addSeparator();
		menu->addAction(qa_AddPermission.get());
		menu->addSeparator();
		menu->addAction(qa_ViewDataFile_Zlib.get());
		menu->addAction(qa_ViewDataFile_NoZlib.get());
		menu->exec(QCursor::pos());//！！！！connect函数必须放在 这句话的上面，至于为啥，暂不清楚！

		delete menu;//这个删除 估计是 右键界面结束后 才会执行到这 
	}
	else if (qw_LocalFileWidget->underMouse())
	{
		QMenu *menu = new QMenu(this);

		QScopedPointer<QAction> qa_ExpandDesktop(new QAction("展开桌面目录", this));
		qa_ExpandDesktop->setIcon(QIcon(":/img/桌面.png"));
		connect(qa_ExpandDesktop.get(), SIGNAL(triggered()), this, SLOT(slotRK_LocalExpandDesktop()), Qt::UniqueConnection);

		QScopedPointer<QAction> qa_ExpandProjectDir(new QAction("展开工程目录", this));
		qa_ExpandProjectDir->setIcon(QIcon(":/img/工程.png"));
		connect(qa_ExpandProjectDir.get(), SIGNAL(triggered()), this, SLOT(slotRK_LocalExpandProjectDir()), Qt::UniqueConnection);

		QScopedPointer<QAction> qa_LocalCollapseAl(new QAction("全部折叠", this));
		qa_LocalCollapseAl->setIcon(QIcon(":/img/收回.png"));
		connect(qa_LocalCollapseAl.get(), SIGNAL(triggered()), this, SLOT(slotRK_LocalCollapseAll()), Qt::UniqueConnection);

		QScopedPointer<QAction> qa_DownEdit(new QAction("编辑", this));
		qa_DownEdit->setIcon(QIcon(":/img/编写.png"));
		connect(qa_DownEdit.get(), SIGNAL(triggered()), this, SLOT(slotRK_LocalEdit()), Qt::UniqueConnection);

		QScopedPointer<QAction> qa_OpenInWinwos(new QAction("在资源管理器中打开", this));
		qa_OpenInWinwos->setIcon(QIcon(":/img/打开.png"));
		connect(qa_OpenInWinwos.get(), SIGNAL(triggered()), this, SLOT(slotRK_LoaclOpenDir()), Qt::UniqueConnection);

		QScopedPointer<QAction> qa_UpFile(new QAction("上传", this));
		qa_UpFile->setIcon(QIcon(":/img/上传文件.png"));
		connect(qa_UpFile.get(), SIGNAL(triggered()), this, SLOT(slotRK_LocalUp()), Qt::UniqueConnection);

		QScopedPointer<QAction> qa_Delet(new QAction("删除", this));
		qa_Delet->setIcon(QIcon(":/img/垃圾桶1.png"));
		connect(qa_Delet.get(), SIGNAL(triggered()), this, SLOT(slotRK_LocalDelet()), Qt::UniqueConnection);

		QScopedPointer<QAction> qa_Rename(new QAction("重命名", this));
		qa_Rename->setIcon(QIcon(":/img/重命名.png"));
		connect(qa_Rename.get(), SIGNAL(triggered()), this, SLOT(slotRK_LocalReName()), Qt::UniqueConnection);

		QScopedPointer<QAction> qa_NewDir(new QAction("新建目录", this));
		qa_NewDir->setIcon(QIcon(":/img/文件夹.png"));
		connect(qa_NewDir.get(), SIGNAL(triggered()), this, SLOT(slotRK_LocalNewDir()), Qt::UniqueConnection);

		QScopedPointer<QAction> qa_NewFile(new QAction("新建文件", this));
		qa_NewFile->setIcon(QIcon(":/img/文本.png"));
		connect(qa_NewFile.get(), SIGNAL(triggered()), this, SLOT(slotRK_LocalNewFile()), Qt::UniqueConnection);


		QScopedPointer<QAction> qa_ViewDataFile_Zlib(new QAction("查看数据文件(压缩)", this));
		qa_ViewDataFile_Zlib->setIcon(QIcon(":/img/查看.png"));
		connect(qa_ViewDataFile_Zlib.get(), SIGNAL(triggered()), this, SLOT(slotRK_LocalOpenDatFileZlib()), Qt::UniqueConnection);

		QScopedPointer<QAction> qa_ViewDataFile_NoZlib(new QAction("查看数据文件(非压缩)", this));
		qa_ViewDataFile_NoZlib->setIcon(QIcon(":/img/查看.png"));
		connect(qa_ViewDataFile_NoZlib.get(), SIGNAL(triggered()), this, SLOT(slotRK_LocalOpenDatFileNoZlib()), Qt::UniqueConnection);

		menu->addAction(qa_LocalCollapseAl.get());
		menu->addAction(qa_ExpandDesktop.get());
		menu->addAction(qa_ExpandProjectDir.get());
		menu->addSeparator();
		menu->addAction(qa_DownEdit.get());
		menu->addAction(qa_OpenInWinwos.get());
		menu->addSeparator();
		menu->addAction(qa_Delet.get());
		menu->addAction(qa_UpFile.get());
		menu->addSeparator();
		menu->addAction(qa_Rename.get());
		menu->addAction(qa_NewFile.get());
		menu->addAction(qa_NewDir.get());
		menu->addSeparator();
		menu->addSeparator();
		menu->addAction(qa_ViewDataFile_Zlib.get());
		menu->addAction(qa_ViewDataFile_NoZlib.get());
		menu->exec(QCursor::pos());//！！！！connect函数必须放在 这句话的上面，至于为啥，暂不清楚！

		delete menu;//这个删除 估计是 右键界面结束后 才会执行到这 
	}
}

/*FTP文件列表-添加具体文件条目*/
void FTPTreeList::slotViewFileList(QStringList FileList)
{
	QString FileName = FileList[8];
	//"-rw-r--r--    1 root     root         6061 Mar 22 11:10 123.json\\n\""
	/*把.dat文件的文件修改时间，改成由.dat文件的时间签生成*/
	if (FileName.contains(".dat") && (FileName.startsWith("1_") || FileName.startsWith("2_") || FileName.startsWith("3_") || FileName.startsWith("4_") || FileName.startsWith("5_")))
	{
		QDateTime time = QDateTime::fromMSecsSinceEpoch(((FileName.section('_', 1, 1) + FileName.section('_', 2, 2)).section(".", 0, 0)).toULongLong());//转换为毫秒级别
		item = new QTreeWidgetItem(QStringList() << FileList[8] << QString::number((FileList[4].toFloat() / 1024.0), 'f', 2) + " KB" << FileList[1] << time.toString("yyyy-MM-dd hh:mm:ss.zzz") << FileList[0]);
	}
	else
	{
		item = new QTreeWidgetItem(QStringList() << FileList[8] << QString::number((FileList[4].toFloat() / 1024.0), 'f', 2) + " KB" << FileList[1] << Month[FileList[5]] + "/" + FileList[6] + "/" + FileList[7] << FileList[0]);
	}
	//!!这个地方会导致 内存泄露
	item->setTextAlignment(1, Qt::AlignRight);
	item->setTextAlignment(2, Qt::AlignCenter);
	if (FileList[8] != ".."&& FileList[8] != ".")
	{
		qic_FileIcon = FindFileIcon(FileList[8].section(".", 1));
		item->setIcon(0, qic_FileIcon);
		if (FileList[0].at(0) == "d")
		{
			QFileIconProvider provider;
			qic_FileIcon = provider.icon((QFileIconProvider::IconType)5);
			item->setIcon(0, qic_FileIcon);
		}
	}
	qtw_FTPListWidget->addTopLevelItem(item);

	if (g_FTPHead == "sftp://" &&SFTPOnce == false)
	{
		qtw_FTPListWidget->header()->setSortIndicator(4, Qt::AscendingOrder);
		qtw_FTPListWidget->header()->setSortIndicator(0, Qt::AscendingOrder);
		SFTPOnce = true;
	}
	else if (g_FTPHead == "ftp://"&&FTPOnce == false)
	{
		qtw_FTPListWidget->header()->setSortIndicator(0, Qt::AscendingOrder);
		qtw_FTPListWidget->header()->setSortIndicator(4, Qt::AscendingOrder);
		FTPOnce = true;
	}
}
using namespace Qt;
/*当用户拖动文件到窗口部件上时候，就会触发dragEnterEvent事件*/
void FTPTreeList::dragEnterEvent(QDragEnterEvent * e)
{
	if (e->mimeData()->hasFormat("text/uri-list")) //只能打开文本文件
	{
		e->acceptProposedAction();
	}//可以在这个窗口部件上拖放对象
	else
	{
		QMessageBox::critical(this, "错误", "只能打开文本文件");//！！但实际上 我
	}
}

/*重写QMessageBox-“过程窗口”初始化--删除文件夹、上传文件架*/
void FTPTreeList::MessageInit()
{
	/*FTP删除过程，对话框 自定义 QMessageBox*/
	qmb_FTPStatusAlarm = new QMessageBox(this);
	qmb_FTPStatusAlarm->setText("正在执行，请勿操作！");
	qmb_FTPStatusAlarm->addButton(QMessageBox::Ok);
	qmb_FTPStatusAlarm->button(QMessageBox::Ok)->hide();
}

/*本地-展开指定目录*/
void FTPTreeList::ExpanPath(QString Path)
{
	//这里不能直接使用ui->treeView->setExpanded(m_FileMode->index(目标文件路径),1);
	//这样只能打开第一层文件夹，因此需要使用循环一层一层的打开。
	//循环一层一层的打开文件夹直到目标文件夹被打开
	QStringList list = Path.split("/");
	//每次循环需要打开的文件路径
	QString findpath;
	foreach(QString addstr, list)
	{
		if (findpath.size() > 0)
		{
			addstr = '/' + addstr;
		}
		findpath += addstr;
		qtv_LocalFileTree->setExpanded(m_FileMode->index(findpath), 1);
	}
}

/*本地-加载本机文件管理器*/
void FTPTreeList::LoadLocalFileTree()
{
	m_FileMode = new QFileSystemModel;
	m_FileMode->setRootPath("C:/Users/45428/Desktop");
	qtv_LocalFileTree->setModel(m_FileMode);
	//qtv_LocalFileTree->setRootIndex(m_FileMode->index("C:/Users/45428/Desktop"));//必须搭配这个才能显示根目录
	//需要默认展开的文件位置
	QString filepath = QCoreApplication::applicationDirPath().mid(0, QCoreApplication::applicationDirPath().lastIndexOf("/")) + "/Project/" + g_ProjectName + "/" + g_DevIP;
	ExpanPath(filepath);
	qtv_LocalFileTree->setCurrentIndex((m_FileMode->index(filepath)));//把指定的路径转化为index，然后设为当前路径
}

/*打开数据文件*/
void FTPTreeList::OpenDatFile(QString Path)
{

}

/*用指定程序打开文件*/
void FTPTreeList::OpenWithOrderEXE(QString EXEPath, QString FilePath)
{
	process = new QProcess();
	if (process->startDetached(EXEPath, QStringList() << FilePath) == false)
	{
		QMessageBox::critical(this, "打开错误", "打开失败");
	}
	delete process;//关闭指针不会影响已经打开的软件
}

/*双击条目--下载、进入不同目录*/
void FTPTreeList::slotCheckFile(QTreeWidgetItem * item, int column)
{
	//qDebug() << item->text(0);//文件名
	if (item->text(0) == ".")//刷新
	{
		qtw_FTPListWidget->clear();
		emit sigGetFTPList(g_FTPCurrentPath);
		this->setWindowTitle(g_FTPCurrentPath);
		return;
	}
	else if (item->text(0) == "..")//返回上层目录
	{
		if (g_FTPCurrentPath.count("/", Qt::CaseInsensitive) == 4)
		{
			QMessageBox::critical(this, "错误", "别往上了，到根目录了！");
			return;
		}
		g_FTPCurrentPath = g_FTPCurrentPath.remove(g_FTPCurrentPath.lastIndexOf('/'), 1);//ftp://192.168.6.38//mnt//
		g_FTPCurrentPath = g_FTPCurrentPath.left(g_FTPCurrentPath.lastIndexOf('/')) + "/";

		qtw_FTPListWidget->clear();
		emit sigGetFTPList(g_FTPCurrentPath);

		this->setWindowTitle(g_FTPCurrentPath);
		return;
	}
	else if (item->text(4).at(0) == "d")//是正常文件夹 而非. ..特殊文件夹
	{
		g_FTPCurrentPath = g_FTPCurrentPath + item->text(0) + "/";
		qtw_FTPListWidget->clear();//！！清除之后item的指针就为空了
		//g_FTPTreeWork->slotGetFTPList(g_FTPCurrentPath.toStdString().c_str());
		emit sigGetFTPList(g_FTPCurrentPath);
		this->setWindowTitle(g_FTPCurrentPath);
		return;
	}
	else if (item->text(4).at(0) == "-")//一般文件
	{
		m_ShowFTPDownInfo = false;//因为下载成功了，自会有窗口显示出来。所以就不提示了
		qfs_FileWatcher->removePath("../Temp/" + item->text(0));
		emit g_FTPTreeWork->sigFTPDownAndOpen(g_FTPCurrentPath + item->text(0), g_UserName, g_UserWord, "../Temp/" + item->text(0), true, false, false, true);
	}
	else
	{
		QMessageBox::critical(this, "错误", "暂不支持该文件类型操作，如有需要联系我");
		return;
	}

}

/*清空文件列表--！用这个的时候，你要想一下你是不要刷新列表。*/
void FTPTreeList::slotClearFileList()
{
	qtw_FTPListWidget->clear();
}

/*FTP下载、上传进度条*/
void FTPTreeList::slotDownProcess(float DownProcess)
{
	qpb_DownPro->setValue(DownProcess);//int类型决定了他是没有小数点的 除非我们用float
}

/*FTP下载文件后操作，用来提示错误或者正确后再进一步操作*/
void FTPTreeList::slotFTPDownError(int Err, QString FilePath, bool IsAddWatch)
{
	//0 ok -1打开文件失败 其他curl错误码
	if (Err == 0)
	{
		if (m_ShowFTPDownInfo)
		{
			QMessageBox::information(this, "下载成功", "下载成功");
		}
		else
		{
			m_ShowFTPDownInfo = true;
		}
		if (IsAddWatch)
		{
			qfs_FileWatcher->removePath(FilePath);
			qfs_FileWatcher->addPath(FilePath);
			qDebug() << FilePath << " 加入到监视";

		}
		//OpenDatFile("../Project/" + g_ProjectName + "/" + g_DevIP + "/" + qtw_FTPListWidget->currentItem()->text(0));
		OpenDatFile(FilePath);
	}
	else if (Err == -1)
	{
		QMessageBox::critical(this, "下载失败", "创建文件失败");
	}
	else
	{
		QMessageBox::critical(this, "下载失败", curl_easy_strerror((CURLcode)Err));
	}
}

/*FTP过程状态显示 1显示 0隐藏 */
void FTPTreeList::slotFTPStatusAlarm(bool Status)
{
	if (Status == true)
	{
		qmb_FTPStatusAlarm->exec();
	}
	else
	{
		qmb_FTPStatusAlarm->hide();
	}
}

/*处理FTP上传错误*/
void FTPTreeList::slotFTPUpError(int Err)
{	//0ok -1打开文件错误 -2 文件大小错误 其他curl错误码
	if (Err == 0)
	{
		QMessageBox::information(this, "上传成功", "上传成功");
	}
	else if (Err == -1)
	{
		QMessageBox::critical(this, "上传失败", "打开文件错误");
	}
	else if (Err == -2)
	{
		QMessageBox::critical(this, "上传失败", "文件大小错误");
	}
	else
	{
		QMessageBox::critical(this, "上传失败", curl_easy_strerror((CURLcode)Err));
	}
}

/*FTP文件列表错误*/
void FTPTreeList::slotFTPListError(int Err)
{
	//0 ok 其他curl错误码if (Err == 0)
	if (Err != 0)
	{
		QMessageBox::critical(this, "文件列表失败", curl_easy_strerror((CURLcode)Err));
	}
}

/*FTP执行命令错误*/
void FTPTreeList::slotFTPCMDError(int Err)
{
	//0 ok 其他curl错误码if (Err == 0)
	if (Err != 0)
	{
		QMessageBox::critical(this, "FTP操作失败", curl_easy_strerror((CURLcode)Err));
	}
	else//命令执行成功，刷新文件列表
	{
		qDebug() << "命令执行成功";
		slotRK_FTPFreshList();
	}
}

/*FTP下载并打开错误*/
void FTPTreeList::slotFTPDownAndOpenError(bool Err)
{
	if (Err == false)
	{
		QMessageBox::critical(this, "下载并打开错误", "打开失败");
	}
}

/*FTP删除文件夹错误*/
void FTPTreeList::slotFTPDeletDirError(QString ErrType, int Err)
{
	//0 ok 其他curl错误码if (Err == 0)
	if (Err != 0)
	{
		QMessageBox::critical(this, "FTP删除文件夹失败", ErrType + curl_easy_strerror((CURLcode)Err));
	}
}

/*FTP下载文件夹错误*/
void FTPTreeList::slotFTPDownDirError(QString ErrType, int Err)
{
	if (Err != 0)
	{
		QMessageBox::critical(this, "FTP下载文件夹失败", ErrType + curl_easy_strerror((CURLcode)Err));
	}
	else
	{
		QMessageBox::information(this, "下载成功", "下载成功");
	}
}

/*FTP右键进入首页或链接*/
void FTPTreeList::slotRK_FTPHome_Connect()
{
	if (g_ConnetIP == "")
	{
		QMessageBox::critical(this, "FTPError", "IP未选择");
		return;
	}
	qtw_FTPListWidget->clear();
	g_FTPCurrentPath = g_FTPHead + g_ConnetIP + "/" + g_SSHFTPJzhsDefaultPath;
	emit sigGetFTPList(g_FTPCurrentPath);
	this->setWindowTitle(g_FTPCurrentPath);
	FTPTreeList::FTPOnce = false;
	FTPTreeList::SFTPOnce = false;
}

/*FTP右键获取根目录*/
void FTPTreeList::slotRK_FTPGetRootDir()
{
	if (g_ConnetIP == "")
	{
		QMessageBox::critical(this, "FTPError", "IP未选择");
		return;
	}
	qtw_FTPListWidget->clear();
	g_FTPCurrentPath = g_FTPHead + g_ConnetIP + "/";
	emit sigGetFTPList(g_FTPCurrentPath);
	this->setWindowTitle(g_FTPCurrentPath);
	FTPTreeList::FTPOnce = false;
	FTPTreeList::SFTPOnce = false;
}

/*FTP右键刷新列表--！刷新列表优先调用这个*/
void FTPTreeList::slotRK_FTPFreshList()
{
	qtw_FTPListWidget->clear();
	//g_FTPTreeWork->slotGetFTPList(g_FTPCurrentPath.toStdString().c_str());//!!!这个有可能会导致卡的！！！
	emit sigGetFTPList(g_FTPCurrentPath);
	qDebug() << "列表获取完毕";
	this->setWindowTitle(g_FTPCurrentPath);
}

/*FTP右键下载到工程目录*/
void FTPTreeList::slotRK_FTPDown()
{
	qDebug() << qtw_FTPListWidget->currentItem()->text(0);
	QString DownPath = "/" + g_FTPCurrentPath.section("/", 4); // /mnt/nandflash/jzhs/conf/

	if (qtw_FTPListWidget->currentItem()->text(0) == "." || qtw_FTPListWidget->currentItem()->text(0) == "..")
	{
		QMessageBox::critical(this, "下载错误", "该类型文件不可下载！");
		return;
	}
	else if (qtw_FTPListWidget->currentItem()->text(4).at(0) == "d")//是正常文件夹 而非. ..特殊文件夹
	{
		emit sigFTPCMDDownDir(g_FTPHead + g_ConnetIP + "/" + DownPath + qtw_FTPListWidget->currentItem()->text(0) + "/", "../Project/" + g_ProjectRegion + "/" + g_ProjectName + "/" + g_DevIP);

		return;
	}
	else if (qtw_FTPListWidget->currentItem()->text(4).at(0) == "-")//一般文件
	{
		//!! lincurl去判断你的路径是文件还是文件夹的方式 非常的粗暴，只检查url结尾字符 是否为/ 是就是目录 不是就是文件 如果把目录当成文件下载 \
		是可以下载一个文件的，内容为这个文件夹中的文件目录 我下面做文件夹的下载，只能用右键的方式，每次拿到一个文件名 我就发一次信号 \
		看看能不能多次接受信号 每次下载的时候 都会阻塞在那个函数 知道下载完成 
		emit g_FTPTreeWork->sigFTPDownAndOpen(g_FTPCurrentPath + qtw_FTPListWidget->currentItem()->text(0), g_UserName, g_UserWord, "../Project/" + g_ProjectRegion + "/" + g_ProjectName + "/" + g_DevIP + "/" + qtw_FTPListWidget->currentItem()->text(0), false, false, false, false);
	}
	else
	{
		QMessageBox::critical(this, "错误", "暂不支持该文件类型操作，如有需要联系我");
		return;
	}
}

/*FTP右键下载到左侧目录*/
void FTPTreeList::slotRK_FTPDownToLeftDir()
{
	qDebug() << qtw_FTPListWidget->currentItem()->text(0);
	QString DownPath = "/" + g_FTPCurrentPath.section("/", 4); // /mnt/nandflash/jzhs/conf/
	QModelIndex index = qtv_LocalFileTree->currentIndex();
	QString DirFath;
	if (m_FileMode->isDir(index))
	{
		DirFath = m_FileMode->filePath(index);
	}
	else
	{
		DirFath = m_FileMode->filePath(index).mid(0, m_FileMode->filePath(index).lastIndexOf("/"));
	}
	if (qtw_FTPListWidget->currentItem()->text(0) == "." || qtw_FTPListWidget->currentItem()->text(0) == "..")
	{
		QMessageBox::critical(this, "下载错误", "该类型文件不可下载！");
		return;
	}
	else if (qtw_FTPListWidget->currentItem()->text(4).at(0) == "d")//是正常文件夹 而非. ..特殊文件夹
	{
		emit sigFTPCMDDownDir(g_FTPHead + g_ConnetIP + "/" + DownPath + qtw_FTPListWidget->currentItem()->text(0) + "/", DirFath);
		return;
	}
	else if (qtw_FTPListWidget->currentItem()->text(4).at(0) == "-")//一般文件
	{
		emit g_FTPTreeWork->sigFTPDownAndOpen(g_FTPCurrentPath + qtw_FTPListWidget->currentItem()->text(0), g_UserName, g_UserWord, DirFath + "/" + qtw_FTPListWidget->currentItem()->text(0), false, false, false, false);
	}
	else
	{
		QMessageBox::critical(this, "错误", "暂不支持该文件类型操作，如有需要联系我");
		return;
	}
}

/*FTP右键下载并打开*/
void FTPTreeList::slotRK_FTPDownAndOpen()
{
	if (qtw_FTPListWidget->currentItem()->text(4).at(0) == "-")//一般文件
	{
		emit g_FTPTreeWork->sigFTPDownAndOpen(g_FTPCurrentPath + qtw_FTPListWidget->currentItem()->text(0), g_UserName, g_UserWord, "../Project/" + g_ProjectRegion + "/" + g_ProjectName + "/" + g_DevIP + "/" + qtw_FTPListWidget->currentItem()->text(0), true, false, false, false);
	}
	else
	{
		QMessageBox::critical(this, "编辑错误", "不支持该文件类型编辑");
		return;
	}
}

/*FTP右键下载并编辑*/
void FTPTreeList::slotRK_FTPDownAndEdit()
{
	if (qtw_FTPListWidget->currentItem()->text(4).at(0) == "-")//一般文件
	{
		QString TempFileName = qApp->applicationDirPath() + "./../Temp/" + qtw_FTPListWidget->currentItem()->text(0);
		m_ShowFTPDownInfo = false;//因为下载成功了，自会有窗口显示出来。所以就不提示了
		qDebug() << " 移除 " << qfs_FileWatcher->removePath(TempFileName);
		emit g_FTPTreeWork->sigFTPDownAndOpen(g_FTPCurrentPath + qtw_FTPListWidget->currentItem()->text(0), g_UserName, g_UserWord, TempFileName, true, false, false, true);
	}
	else
	{
		QMessageBox::critical(this, "编辑错误", "不支持该文件类型编辑");
		return;
	}
}

/*FTP右键上传文件*/
void FTPTreeList::slotRK_FTPUPFile()
{
	QString LocalFilePath = QFileDialog::getOpenFileName(this, "上传文件", "", "Files (*.*)");//绝对路径--这种方式会增加50M内存使用
	//QString LocalDirName = QFileDialog::getOpenFileName(this, "上传文件", "", "Files (*.*)", nullptr, QFileDialog::DontUseNativeDialog);//返回绝对路径 --- 用qt自己的文件对话框--避免了内存泄露
	if (LocalFilePath == "")
	{
		return;
	}
	qDebug() << LocalFilePath;
	QString FilenName = LocalFilePath.section("/", -1, -1);
	qDebug() << FilenName;
	emit g_FTPTreeWork->sigStartFTPUP(g_FTPCurrentPath + FilenName, g_UserName, g_UserWord, LocalFilePath, true);
}

/*FTP递归遍历指定目录  上传文件夹的时候，要在遍历之前，把最外层的路径，提前加进去，要不然没有自己的路径*/
int FTPTreeList::FindFile(const QString& FilePath)
{
	QDir dir(FilePath);   //QDir的路径一定要是全路径，相对路径会有错误
	if (!dir.exists())
	{
		QMessageBox::critical(this, "上传", "该目录不存在");
		return -1;
	}
	//取到所有的文件和文件名，去掉.和..文件夹
	dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
	dir.setSorting(QDir::DirsFirst);
	//将其转化为一个list
	QFileInfoList list = dir.entryInfoList();
	if (list.size() < 1)
		return -1;

	int i = 0;
	//采用递归算法
	do {
		QFileInfo fileInfo = list.at(i);
		bool bisDir = fileInfo.isDir();
		if (bisDir) {
			LocalDirList << fileInfo.filePath();
			FindFile(fileInfo.filePath());
		}
		else {
			//qDebug() << fileInfo.filePath() << ":" << fileInfo.fileName();
			LocalFileList << fileInfo.filePath();
		}

		++i;
	} while (i < list.size());

	return 0;
}

/*FTP文件管理器匹配图标*/
QIcon FTPTreeList::FindFileIcon(const QString & extension) const
{
	QFileIconProvider provider;
	QIcon icon;
	QString strTemplateName = QDir::tempPath() + QDir::separator() + QCoreApplication::applicationName() + "_XXXXXX." + extension;//XXXXX是随机部分
	QTemporaryFile tmpFile(strTemplateName);
	if (tmpFile.open())//调用open来创建独一无二的临时文件 析构后自动删除
	{
		tmpFile.close();
		icon = provider.icon(QFileInfo(tmpFile.fileName()));
	}
	else
	{
		qCritical() << QString("failed to write temporary file %1").arg(tmpFile.fileName());
	}
	return icon;
}

/*FTP拖拽文件并上传*/
void FTPTreeList::dropEvent(QDropEvent * e)
{
	QList<QUrl> urls = e->mimeData()->urls();
	if (urls.isEmpty())
		return;
	QString DropFileName = urls.first().toLocalFile();
	QFileInfo FileInfo(DropFileName);
	if (FileInfo.isDir())
	{
		LocalFileList.clear();
		QString Temp_LocalDirName;
		QStringList Temp_LocalFileList;//保存精简完整路径FindFile(DropFileName);//D:/c++demo/QtProject/CJQTool/CJQTool/Release/moc/moc_CJQTool.cpp
		FindFile(DropFileName);//D:/c++demo/QtProject/CJQTool/CJQTool/Release/moc/moc_CJQTool.cpp
		Temp_LocalDirName = DropFileName.section("/", 0, -2) + "/";//在路径中去掉你选择的目录
		Temp_LocalFileList = LocalFileList;
		for (size_t FileInex = 0; FileInex < LocalFileList.size(); FileInex++)
		{
			qDebug() << Temp_LocalFileList[FileInex].remove(0, Temp_LocalDirName.size());
			emit g_FTPTreeWork->sigStartFTPUP(g_FTPCurrentPath + Temp_LocalFileList[FileInex], g_UserName, g_UserWord, LocalFileList[FileInex], false);
		}
	}
	else if (FileInfo.isFile())
	{
		QString FilenName = DropFileName.section("/", -1, -1);
		qDebug() << FilenName;
		emit g_FTPTreeWork->sigStartFTPUP(g_FTPCurrentPath + FilenName, g_UserName, g_UserWord, DropFileName, true);
	}
	else
	{
		QMessageBox::critical(this, "上传", "你拖拽的是啥？！");
		return;
	}
}

/*FTP右键上传文件夹*/
void FTPTreeList::slotRK_FTPUPDir()
{
	LocalFileList.clear();
	QString Temp_LocalDirName;
	QStringList Temp_LocalFileList;//保存精简完整路径
	QString LocalDirName = QFileDialog::getExistingDirectory(this, "上传文件夹", "");//绝对路径
	if (LocalDirName == "")
	{
		return;
	}
	qDebug() << LocalDirName;//D:/c++demo/QtProject/CJQTool/CJQTool/Release
	FindFile(LocalDirName);//D:/c++demo/QtProject/CJQTool/CJQTool/Release/moc/moc_CJQTool.cpp
	Temp_LocalDirName = LocalDirName.section("/", 0, -2) + "/";//在路径中去掉你选择的目录
	Temp_LocalFileList = LocalFileList;
	for (size_t FileInex = 0; FileInex < LocalFileList.size(); FileInex++)
	{
		qDebug() << Temp_LocalFileList[FileInex].remove(0, Temp_LocalDirName.size());
		emit g_FTPTreeWork->sigStartFTPUP(g_FTPCurrentPath + Temp_LocalFileList[FileInex], g_UserName, g_UserWord, LocalFileList[FileInex], false);
	}
}

/*FTP右键重命名文件、文件夹*/
void FTPTreeList::slotRK_FTPReName()
{
	qe_RenameFile->setText(qtw_FTPListWidget->currentItem()->text(0));
	if (qmb_RenameWidget->exec() == QMessageBox::Ok) {
		qDebug() << "重命名";
		static std::string SCMD = "";
		QString RenamePath = "/" + g_FTPCurrentPath.section("/", 4); // /mnt/nandflash/jzhs/conf/
		QString QCMD = "";
		if (g_FTPHead == "sftp://")
		{
			QCMD = "rename " + RenamePath + qtw_FTPListWidget->currentItem()->text(0) + " " + RenamePath + qe_RenameFile->text();
		}
		else if (g_FTPHead == "ftp://")
		{
			QCMD = "RNFR " + qtw_FTPListWidget->currentItem()->text(0) + "RNTO " + qe_RenameFile->text();//ftp的重命名不需要太多的路径
		}
		//qDebug() << QCMD;
		SCMD = QCMD.toStdString();
		emit sigFTPCMD(SCMD.c_str());
	}
}

/*FTP右键删除文件、文件夹*/
void FTPTreeList::slotRK_FTPDelet()
{
	int ret = QMessageBox::question(this, "删除", "是否删除", QMessageBox::Yes | QMessageBox::No);
	if (ret == QMessageBox::No)
	{
		return;
	}
	static std::string SCMD = "";
	//sftp:/192.168.6.38/mnt/nandflash/jzhs/conf/
	QString DeletPath = "/" + g_FTPCurrentPath.section("/", 4); // /mnt/nandflash/jzhs/conf/
	QString QCMD = "";
	if (qtw_FTPListWidget->currentItem()->text(4).at(0) == "-")
	{
		qDebug() << "开始删除 -文件";
		if (g_FTPHead == "sftp://")
		{
			QCMD = "rm  " + DeletPath + qtw_FTPListWidget->currentItem()->text(0);//sftp的命令，这个是由库提供的
		}
		else if (g_FTPHead == "ftp://")
		{
			QCMD = "DELE " + DeletPath + qtw_FTPListWidget->currentItem()->text(0);//ftp的命令，这个是ftp本身提供的
		}
		qDebug() << QCMD;
		SCMD = QCMD.toStdString();
		emit sigFTPCMD(SCMD.c_str());

	}
	else if (qtw_FTPListWidget->currentItem()->text(4).at(0) == "d" && qtw_FTPListWidget->currentItem()->text(0) != "." &&qtw_FTPListWidget->currentItem()->text(0) != "..")//删除文件夹，走单独的体系
	{
		qDebug() << "开始删除 d 文件夹";
		if (g_FTPHead == "sftp://")
		{
			emit sigFTPCMDDeletDir(g_FTPHead + g_ConnetIP + "/" + DeletPath + qtw_FTPListWidget->currentItem()->text(0) + "/");
		}
		else if (g_FTPHead == "ftp://")
		{
			//RMD 
			emit sigFTPCMDDeletDir(g_FTPHead + g_ConnetIP + "/" + DeletPath + qtw_FTPListWidget->currentItem()->text(0) + "/");
		}
	}
	else
	{
		QMessageBox::critical(this, "Error", QString("出于保护系统目的，暂不支持该类型文件删除!\n文件类型为： ") + qtw_FTPListWidget->currentItem()->text(4).at(0));
		return;
	}
	//!!!只要在传递参数的时候  直接用 " emit sigFTPCMD(QCMD.toStdString().c_str())"就会出问题  \
	或者static const char* a = QCMD.toStdString().c_str();也会出问题
	//!!注意 这个信号的接受者字另外一个线程 并且传递的是个指针 所以如果不用static的话 在发送信号之后函数结束 对应变量释放，\
	那么该指针对应的内存立马重置，所以导致后面去访问这个内存中数据 全部为？？？？
}

/*FTP右键新建文件夹*/
void FTPTreeList::slotRK_FTPNewDir()
{
	qe_NewFile->setText("");
	if (qmb_NewFileWidget->exec() == QMessageBox::Ok) {
		static std::string SCMD = "";// 和上面的 static std::string SCMD  两者储存在不同的内存
		qDebug() << "新建文件夹";
		QString QCMD = "";
		if (g_FTPHead == "sftp://")
		{
			QString DirPath = "/" + g_FTPCurrentPath.section("/", 3);
			QCMD = "mkdir " + DirPath + qe_NewFile->text();//!!!mkdir不支持参数 他创建的文件夹drwxr-xr-x 没有x
			//QCMD = "mkdir " + (QString)"/mnt/" + qe_NewFile->text();
		}
		else if (g_FTPHead == "ftp://")
		{
			QCMD = "MKD " + qe_NewFile->text();
		}

		qDebug() << QCMD;
		SCMD = QCMD.toStdString();
		emit sigFTPCMD(SCMD.c_str());
	}
}

/*FTP右键添加执行权限*/
void FTPTreeList::slotRK_FTPAddPermission()
{
	qDebug() << "增加执行权限";
	static std::string SCMD = "";
	QString ChmodPath = "/" + g_FTPCurrentPath.section("/", 4); // /mnt/nandflash/jzhs/
	//QString QCMD ="chmod 777 /mnt/nandflash/jzhs/logo.png";
	QString QCMD;
	if (g_FTPHead == "sftp://")
	{
		QCMD = "chmod 777 " + ChmodPath + qtw_FTPListWidget->currentItem()->text(0);
	}
	else if (g_FTPHead == "ftp://")
	{
		//SITE chmod 646 mylifle.ziP
		QCMD = "SITE chmod 777 " + qtw_FTPListWidget->currentItem()->text(0);
	}

	qDebug() << QCMD;
	SCMD = QCMD.toStdString();
	emit sigFTPCMD(SCMD.c_str());
}

/*FTP右键一键读取数据文件-压缩*/
void FTPTreeList::slotRK_ReadDataFileZlib()
{
	QString FileName = QString(qtw_FTPListWidget->currentItem()->text(0));
	if (FileName.contains(".dat") && (FileName.startsWith("1_") || FileName.startsWith("2_") || FileName.startsWith("3_") || FileName.startsWith("4_") || FileName.startsWith("5_")))
	{
		m_IsZlib = true;
		m_IsRKReadDataFile = true;
		emit g_FTPTreeWork->sigFTPDownAndOpen(g_FTPCurrentPath + qtw_FTPListWidget->currentItem()->text(0), g_UserName, g_UserWord, "../Project/" + g_ProjectRegion + "/" + g_ProjectName + "/" + g_DevIP + "/" + qtw_FTPListWidget->currentItem()->text(0), false, false, false, false);
	}
	else
	{
		m_IsRKReadDataFile = false;
		QMessageBox::critical(this, "错误", "你选择的不是数据文件");
	}
}

/*FTP右键一键读取数据文件-非压缩*/
void FTPTreeList::slotRK_ReadDataFileNoZlib()
{
	QString FileName = QString(qtw_FTPListWidget->currentItem()->text(0));
	if (FileName.contains(".dat") && (FileName.startsWith("1_") || FileName.startsWith("2_") || FileName.startsWith("3_") || FileName.startsWith("4_") || FileName.startsWith("5_")))
	{
		m_IsZlib = false;
		m_IsRKReadDataFile = true;
		emit g_FTPTreeWork->sigFTPDownAndOpen(g_FTPCurrentPath + qtw_FTPListWidget->currentItem()->text(0), g_UserName, g_UserWord, "../Project/" + g_ProjectRegion + "/" + g_ProjectName + "/" + g_DevIP + "/" + qtw_FTPListWidget->currentItem()->text(0), false, false, false, false);
	}
	else
	{
		m_IsRKReadDataFile = false;
		QMessageBox::critical(this, "错误", "你选择的不是数据文件");
	}
}

/*FTP清空文件列表*/
void FTPTreeList::slotTTPPListClear()
{
	qtw_FTPListWidget->clear();
}

/*Curl申请指针为空*/
void FTPTreeList::slotCurlNullError()
{
	QMessageBox::critical(this, "LIbcurl 错误", "指针为空，请重启，若经常出现，联系我");
}

/*更新“过程窗口”的信息*/
void FTPTreeList::slotUPorDownProcessMsg(QString ProcessMsg)
{
	qmb_FTPStatusAlarm->setText("正在执行，请勿操作！\n" + ProcessMsg);
}

/*本地-右键-删除本地文件或者文件夹*/
void FTPTreeList::slotRK_LocalDelet()
{
	int ret = QMessageBox::question(this, "删除", "是否删除", QMessageBox::Yes | QMessageBox::No);
	if (ret == QMessageBox::No)
	{
		return;
	}
	QModelIndex index = qtv_LocalFileTree->currentIndex();
	if (!index.isValid()) {
		return;
	}
	bool ok;
	if (m_FileMode->fileInfo(index).isDir()) {
		//ok = m_FileMode->rmdir(index);//不支持删除非空
		QDir dir(m_FileMode->filePath(index));
		ok = dir.removeRecursively();
	}
	else if (m_FileMode->fileInfo(index).isFile()) {
		ok = m_FileMode->remove(index);
	}
	if (!ok) {
		QMessageBox::information(this, tr("Remove"), tr("Failed to remove %1").arg(m_FileMode->fileInfo(index).fileName()));
	}
}

/*本地-右键-新建本地文件夹*/
void FTPTreeList::slotRK_LocalNewDir()
{
	QModelIndex index = qtv_LocalFileTree->currentIndex();
	if (!index.isValid()) {
		return;
	}

	QString dirName = QInputDialog::getText(this, "新建目录", "请输入文件夹名字");
	if (dirName == "")
	{
		return;
	}
	if (!m_FileMode->mkdir(index, dirName).isValid()) {
		QMessageBox::information(this, "新建目录", "创建失败");
	}
}

/*本地-新建文件*/
void FTPTreeList::slotRK_LocalNewFile()
{
	QString NewFileName = QInputDialog::getText(this, "新建文件", "文件名");
	QModelIndex index = qtv_LocalFileTree->currentIndex();
	QFile NewFile;
	QString NewFileFath;
	if (m_FileMode->isDir(index))
	{
		NewFileFath = m_FileMode->filePath(index) + "/" + NewFileName;
		NewFile.setFileName(NewFileFath);
		if (!NewFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
		{
			QMessageBox::critical(this, "新建文件", "新建文件失败");
		}
	}
	else
	{
		QString DirFath = m_FileMode->filePath(index).mid(0, m_FileMode->filePath(index).lastIndexOf("/"));
		NewFileFath = DirFath + "/" + NewFileName;
		NewFile.setFileName(NewFileFath);
		if (!NewFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
		{
			QMessageBox::critical(this, "新建文件", "新建文件失败");
		}
	}
	NewFile.close();
}

/*本地-右键-编辑文件*/
void FTPTreeList::slotRK_LocalEdit()
{
	QModelIndex index = qtv_LocalFileTree->currentIndex();
	if (!index.isValid()) {
		return;
	}
	if (m_FileMode->fileInfo(index).isFile()) {
		OpenWithOrderEXE(g_OtherToolsSetting.Editor, m_FileMode->filePath(index));
	}
}

/*本地-收起所有展开目录*/
void FTPTreeList::slotRK_LocalCollapseAll()
{
	qtv_LocalFileTree->collapseAll();
}

/*本地-上传文件、文件夹*/
void FTPTreeList::slotRK_LocalUp()
{
	QModelIndex index = qtv_LocalFileTree->currentIndex();
	//qDebug() << m_FileMode->filePath(index);
	//qDebug() << m_FileMode->fileInfo(index).filePath();
	//qDebug() << m_FileMode->fileName(index);//!!显示的不光是文件名，他会根据你的箭头的位置，打印对应的名字，创建日期啥的
	//qDebug() << m_FileMode->fileInfo(index).fileName();//
	//return;
	if (!index.isValid()) {
		return;
	}
	if (m_FileMode->fileInfo(index).isFile()) {
		emit g_FTPTreeWork->sigStartFTPUP(g_FTPCurrentPath + m_FileMode->fileInfo(index).fileName(), g_UserName, g_UserWord, m_FileMode->filePath(index), true);
	}
	else if (m_FileMode->fileInfo(index).isDir())
	{
		LocalFileList.clear();
		LocalDirList.clear();
		QString Temp_LocalDirName;
		QStringList Temp_LocalFileList;//保存精简完整路径
		QStringList Temp_LocalDirList;//保存精简完整路径
		QString LocalDirName = m_FileMode->filePath(index);//绝对路径
		//qDebug() << LocalDirName;//D:/c++demo/QtProject/CJQTool/CJQTool/Release
		LocalDirList << LocalDirName;//！！上传文件夹的时候，要在遍历之前，把最外层的路径，提前加进去。
		FindFile(LocalDirName);//D:/c++demo/QtProject/CJQTool/CJQTool/Release/moc/moc_CJQTool.cpp
		Temp_LocalDirName = LocalDirName.section("/", 0, -2) + "/";//在绝对路径中去掉你选择的目录D:/c++demo/QtProject/CJQTool/CJQTool/
		Temp_LocalFileList = LocalFileList;
		Temp_LocalDirList = LocalDirList;
		for (size_t DirInex = 0; DirInex < LocalDirList.size(); DirInex++)//！！这个先手动创建文件夹，不需要，因为libcurl可以自己创建
		{
			Temp_LocalDirList[DirInex].remove(0, Temp_LocalDirName.size());
			static std::string SCMD = "";
			QString QCMD = "";
			if (g_FTPHead == "sftp://")
			{
				QString DirPath = "/" + g_FTPCurrentPath.section("/", 3);
				QCMD = "mkdir " + DirPath + Temp_LocalDirList[DirInex];
			}
			else if (g_FTPHead == "ftp://")
			{
				QCMD = "MKD " + Temp_LocalDirList[DirInex];
			}
			//qDebug() << QCMD;
			SCMD = QCMD.toStdString();
			//emit sigFTPCMD(SCMD.c_str());
		}
		for (size_t FileInex = 0; FileInex < LocalFileList.size(); FileInex++)
		{
			Temp_LocalFileList[FileInex].remove(0, Temp_LocalDirName.size());
			emit g_FTPTreeWork->sigStartFTPUP(g_FTPCurrentPath + Temp_LocalFileList[FileInex], g_UserName, g_UserWord, LocalFileList[FileInex], false);
		}
		if (LocalFileList.isEmpty())//加这个的目的是，在执行
		{
			slotRK_FTPFreshList();
		}
	}
}

/*本地-重命名*/
void FTPTreeList::slotRK_LocalReName()
{
	QModelIndex index = qtv_LocalFileTree->currentIndex();
	QString ParentPath = m_FileMode->filePath(index);
	ParentPath = ParentPath.mid(0, ParentPath.lastIndexOf("/"));
	QString NewName = QInputDialog::getText(this, "重命名", "新名字");
	if (!index.isValid()) {
		return;
	}
	if (m_FileMode->fileInfo(index).isFile()) {

		if (QFile::rename(m_FileMode->filePath(index), ParentPath + "/" + NewName))
		{
			qDebug() << "文件重命名成功";
		}
		else
		{
			qDebug() << "文件重命名失败";
			QMessageBox::critical(this, "文件重命名失败", m_FileMode->fileInfo(index).fileName());
		}
	}
	else if (m_FileMode->fileInfo(index).isDir())
	{
		QDir dir;
		dir.setPath(ParentPath);
		if (dir.rename(m_FileMode->fileInfo(index).fileName(), NewName))
		{
			qDebug() << "文件夹重命名成功";
		}
		else
		{
			qDebug() << "文件夹重命名失败";
			QMessageBox::critical(this, "文件夹重命名失败", m_FileMode->fileInfo(index).fileName());
		}
	}
}

/*本地-打开压缩数据文件*/
void FTPTreeList::slotRK_LocalOpenDatFileZlib()
{
	QModelIndex index = qtv_LocalFileTree->currentIndex();
	QString FileName = m_FileMode->fileInfo(index).fileName();
	if (m_FileMode->fileInfo(index).isFile() && FileName.contains(".dat") && (FileName.startsWith("1_") || FileName.startsWith("2_") || FileName.startsWith("3_") || FileName.startsWith("4_") || FileName.startsWith("5_")))
	{
		m_IsZlib = true;
		m_IsRKReadDataFile = true;
		OpenDatFile(m_FileMode->filePath(index));
	}
	else
	{
		m_IsRKReadDataFile = false;
		QMessageBox::critical(this, "错误", "你选择的不是数据文件");
	}
}

/*本地-打开非压缩数据文件*/
void FTPTreeList::slotRK_LocalOpenDatFileNoZlib()
{
	QModelIndex index = qtv_LocalFileTree->currentIndex();
	QString FileName = m_FileMode->fileInfo(index).fileName();
	if (m_FileMode->fileInfo(index).isFile() && FileName.contains(".dat") && (FileName.startsWith("1_") || FileName.startsWith("2_") || FileName.startsWith("3_") || FileName.startsWith("4_") || FileName.startsWith("5_")))
	{
		m_IsZlib = false;
		m_IsRKReadDataFile = true;
		OpenDatFile(m_FileMode->filePath(index));
	}
	else
	{
		m_IsRKReadDataFile = false;
		QMessageBox::critical(this, "错误", "你选择的不是数据文件");
	}
}

/*本地-打开文件夹*/
void FTPTreeList::slotRK_LoaclOpenDir()
{
	QModelIndex index = qtv_LocalFileTree->currentIndex();
	if (m_FileMode->isDir(index))
	{
		qDebug() << m_FileMode->filePath(index);
		qDebug() << QUrl::fromLocalFile(m_FileMode->filePath(index));
		QDesktopServices::openUrl(QUrl::fromLocalFile(m_FileMode->filePath(index)));
	}
	else
	{
		QString DirFath = m_FileMode->filePath(index).mid(0, m_FileMode->filePath(index).lastIndexOf("/"));
		QDesktopServices::openUrl(QUrl::fromLocalFile(DirFath));
	}
}

/*FTP监视文件改动，并上传*/
void FTPTreeList::slotMonitorFile(QString path)
{
	qfs_FileWatcher->disconnect();
	if (!QFile::exists(path))
	{
		qCritical() << path << "文件不存在，此时文件被修改后不上传";
		connect(qfs_FileWatcher, SIGNAL(fileChanged(QString)), this, SLOT(slotMonitorFile(QString)), Qt::UniqueConnection);
		return;
	}
	qDebug() << path << " 被修改";
	QString FilenName = path.section("/", -1, -1);
	Delay(500);//这个延迟必须有
	emit g_FTPTreeWork->sigStartFTPUP(g_FTPCurrentPath + FilenName, g_UserName, g_UserWord, path, true);
	connect(qfs_FileWatcher, SIGNAL(fileChanged(QString)), this, SLOT(slotMonitorFile(QString)), Qt::UniqueConnection);
}

/*测试函数--无实际意义*/
void FTPTreeList::slottest()
{
	process = new QProcess();
	process->startDetached(g_OtherToolsSetting.Editor, QStringList() << "D:/c++demo/QtProject/CJQTool/Tools/test.bat");
	delete process;//关闭指针不会影响已经打开的软件
}

void FTPTreeList::slotOpenFTPWindows()
{
	if (g_IsSHHTogetherFTP)
	{
		QMessageBox::critical(this, "警告", "SSH和FTP窗口已经聚合，不能从此处打开FTP \n应从SSH窗口内，点击右键，发起链接");
		return;
	}
	if (g_ConnetIP == "")
	{
		QMessageBox::critical(this, "FTP链接失败", "未选择IP");
		return;
	}
	if (g_FTPHead == "ftp://")
	{
		//QMessageBox::critical(this, "错误", "FTP未作适配");
		//return;
	}

	g_FTPCurrentPath = g_FTPHead + g_ConnetIP + "/" + g_SSHFTPJzhsDefaultPath;//!!在ip后面加/，是因为ftp不加，库报错，但是sftp加不加无所谓
	emit sigClearFileList();//每次打开服务区文件列表之前 首先清空一下
	this->showNormal();//最小化也能显示出来
	this->raise();//显示到最前面
	emit sigOpenFTPlist();//打开文件列表
	this->setWindowTitle(g_FTPCurrentPath);
	FTPTreeList::FTPOnce = false;
	FTPTreeList::SFTPOnce = false;
	//g_FTPTreeList->LoadLocalFileTree();//加载本地文件目录，并展开工程目录
}

/*本地-打开工程文件夹*/
void FTPTreeList::slotRK_LocalExpandProjectDir()
{
	QString filepath = QDir::currentPath();//filepath = D:/c++demo/QtProject/CJQTool/CJQTool
	filepath.remove(filepath.lastIndexOf("/") + 1, 7);
	filepath += "Project/" + g_ProjectRegion + "/" + g_ProjectName + "/" + g_DevIP;
	ExpanPath(filepath);
}

/*本地-展开桌面文件夹*/
void FTPTreeList::slotRK_LocalExpandDesktop()
{
	ExpanPath(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
}
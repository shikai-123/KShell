#pragma once
#include <qdebug.h>
#include <qstring.h>
#include <qlayout.h>
#include <qtreewidget.h>
#include <qstringlist.h>
#include <qheaderview.h>
#include <qpushbutton.h>
#include <qmenu.h>
#include <qaction.h>
#include <qmessagebox.h>
#include <qprogressbar.h>
#include <qmap.h>
#include <qdialogbuttonbox.h>
#include <qlabel.h>
#include <qfiledialog.h>
#include <QFileIconProvider>
#include <qtemporaryfile.h>
#include <QSharedPointer>//QT的只能指针
#include <qsplitter.h>
#include <qpushbutton.h>
#include <qtreeview.h>
#include <qfilesystemmodel.h>
#include <qinputdialog.h>
#include <qfilesystemwatcher.h>
#include <qprocess.h>
#include <qdatetime.h>
#include <qdesktopservices.h>
#include <qevent.h>
#include <QApplication> // 或者其他包含 QMimeData 定义的 Qt 头文件
#include <qmimedata.h>
class FTPTreeList : public QWidget
{
	Q_OBJECT
public:
	QVBoxLayout* vbl_RemoteFileLayout;//远程文件窗口布局
	QWidget* qw_RemotFileWidget;
	QVBoxLayout* vbl_LoaclFileLayout;//本地文件窗口布局
	QWidget* qw_LocalFileWidget;
	QVBoxLayout* vbl_AllFtpLayout;//总FTP窗口布局
	QTreeWidget* qtw_FTPListWidget;
	QProgressBar* qpb_DownPro;
	QLineEdit* qe_NewFile;
	QLineEdit* qe_RenameFile;
	QMessageBox* qmb_NewFileWidget;
	QMessageBox* qmb_FTPStatusAlarm;
	QMessageBox* qmb_RenameWidget;
	QStringList LocalFileList;
	QStringList LocalDirList;
	QIcon qic_FileIcon;
	QTreeWidgetItem * item = nullptr;
	static bool FTPOnce;//为false的时候，要进行对文件管理器条目排序。
	static bool SFTPOnce;
	bool m_IsRKReadDataFile = false;//是否为右键“读取数据文件” 0no 1yes
	bool m_IsZlib = false;
	QMap<QString, QString> Month = { {"Jan","1"}, {"Feb","2"}, {"Mar","3"} , {"Apr","4"} , {"May","5"} , {"Jun","6"} ,\
	{"Jul","7"} , {"Aug","8"} , {"Sep","9"} , {"Oct","10"} , {"Nov","11"} , {"Dec","12"} };
	QIcon FindFileIcon(const QString &extension) const;
	/*添加该窗口到SSH中*/
	QSplitter* qs_litter;
	QSplitter* qs_SplitterForL_R;
	QTreeView* qtv_LocalFileTree;
	QFileSystemModel *m_FileMode;

	/*文件监视改动后上传*/
	QFileSystemWatcher* qfs_FileWatcher;
	bool m_ShowFTPDownInfo = true;//0不显示“纯”FTP下载成功提示 1显示下载成功提示
	QProcess* process;

	FTPTreeList();
	void Init();
	void contextMenuEvent(QContextMenuEvent *);
	int FindFile(const QString& FilePath);
	void dropEvent(QDropEvent * e);
	void dragEnterEvent(QDragEnterEvent *e);
	void MessageInit();
	void LoadLocalFileTree();
	void ExpanPath(QString Path);
	void OpenDatFile(QString Path);
	void OpenWithOrderEXE(QString EXEPath, QString FilePath);

public slots:
	void slotViewFileList(QStringList FileList);
	void slotCheckFile(QTreeWidgetItem *item, int column);
	void slotClearFileList();
	void slotDownProcess(float DownProcess);
	void slotFTPDownError(int Err, QString FilePath, bool IsAddWatch);
	void slotFTPStatusAlarm(bool Status);
	void slotFTPUpError(int Err);
	void slotFTPListError(int Err);
	void slotFTPCMDError(int Err);
	void slotFTPDownAndOpenError(bool Err);
	void slotFTPDeletDirError(QString ErrType, int Err);
	void slotFTPDownDirError(QString ErrType, int Err);
	void slotRK_FTPHome_Connect();
	void slotRK_FTPGetRootDir();
	void slotRK_FTPFreshList();
	void slotRK_FTPDown();
	void slotRK_FTPDownToLeftDir();
	void slotRK_FTPDownAndOpen();
	void slotRK_FTPDownAndEdit();
	void slotRK_FTPUPFile();
	void slotRK_FTPUPDir();
	void slotRK_FTPReName();
	void slotRK_FTPDelet();
	void slotRK_FTPNewDir();
	void slotRK_FTPAddPermission();
	void slotRK_ReadDataFileZlib();
	void slotRK_ReadDataFileNoZlib();
	void slotTTPPListClear();
	void slotCurlNullError();
	void slotUPorDownProcessMsg(QString ProcessMsg);
	void slotRK_LocalDelet();
	void slotRK_LocalNewDir();
	void slotRK_LocalNewFile();
	void slotRK_LocalEdit();
	void slotRK_LocalExpandDesktop();
	void slotRK_LocalExpandProjectDir();
	void slotRK_LocalCollapseAll();
	void slotRK_LocalUp();
	void slotRK_LocalReName();
	void slotRK_LocalOpenDatFileZlib();
	void slotRK_LocalOpenDatFileNoZlib();
	void slotRK_LoaclOpenDir();
	void slotMonitorFile(QString path);
	void slottest();
	void slotOpenFTPWindows();


signals:
	void sigFTPCMD(const char* CMD);//!!每次执行完命令后，要手动刷新一下文件列表
	void sigFTPCMDDeletDir(QString Path);
	void sigFTPCMDDownDir(QString RemotePath, QString TargetPath);
	void sigGetFTPList(QString FTPCurrentPath);
	void sigClearFileList();
	void sigOpenFTPlist();
};


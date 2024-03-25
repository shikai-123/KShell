#pragma once
#include <QWidget>
#include <qdebug.h>
#include <qlabel.h>
#include <qscrollarea.h>
#include <qgroupbox.h>
#include <qboxlayout.h>
#include <qcheckbox.h>
#include <qlineedit.h>
#include <qspinbox.h>
#include <qfiledialog.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qformlayout.h>
#include <qtoolbutton.h>
#include <qvector.h>
#include "DataStruck.h"
#include <QApplication>
#include <qradiobutton.h>
#include <QButtonGroup>
#include <qmessagebox.h>
#include <iostream>
#include <Qfile>
#include <fstream>
#include "curl/curl.h"
#include <QTextCodec>
#include <qdesktopservices.h>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QUrl>
#include <qmimedata.h>
struct SMeterDate
{
	QString MeterlName;
	int MeterID;
	int MeterLink;
	QString MeterFile;
};

struct SChannelData
{
	QString ChannelName;
	int MeterNum;
	int ChannelMode;
	int ComIndex;
	SMeterDate MeterInfo[MAXMETER];
};


class CJQConf : public QWidget
{
	Q_OBJECT
public:
	CJQConf();

	QLabel *lb_Title;
	QScrollArea *sa_ScrollArea;
	QWidget* sa_WindowContent;
	QGroupBox* gb_BaseSetting;
	QLineEdit* le_Author;
	QLineEdit* le_Note;
	QCheckBox* cb_AlarmReport;
	QCheckBox* cb_SerialText;
	QCheckBox* cb_Mode;
	QCheckBox* cb_EnableZib;
	QCheckBox* cb_reserve3;
	QCheckBox* cb_ExtremeReport;
	QCheckBox* cb_MQTTRemoteControl;
	QCheckBox* cb_ModbusOptimal;

	QVector <QVector<QHBoxLayout*>> MeterLayout;
	//= QVector <QVector<QHBoxLayout*>>(MAXCHANEL, QVector<QHBoxLayout*>());/*每个表计的布局*/
	//QGridLayout* layout[MAXCHANEL];/*每个通道的布局  也就是每个通道的QGroupBox中的垂直布局*/
	QVector<QVBoxLayout*>ChannelLayout;/*每个通道的布局  也就是每个通道的QGroupBox中的垂直布局*/
	QGroupBox* gb_BaseSetting1;
	QLineEdit* le_SnapshotInterval;
	QLineEdit* le_Msg_sleep;
	QLineEdit* le_rsp_timeout_sec;
	QLineEdit* le_rsp_timeout_usec;
	QLineEdit* le_byte_timeout_sec;
	QLineEdit* le_byte_timeout_usec;
	QLineEdit* le_SendInterval;
	QLineEdit* le_CJQPort;
	QLineEdit* le_MQ2_SendInterval;
	QLineEdit* le_MQ3_SendInterval;
	QLineEdit* le_Codec;
	QVBoxLayout* vbl_ChannelsSetting;/*总通道设置-垂直布局*/
	QGroupBox* gb_AllChannelsSetting;
	QSpinBox* sb_ChannelNum;
	QPushButton* qb_AddChannel;

	SChannelData ChannelInfo;

	QVector<QLabel *> lb_ChannelTitle;
	QVector<SChannelData> VChannelInfo;

	QVector<QGroupBox*> gb_ChannelsSetting;/*每个小通道的QGroupBox*/
	QVector<QSpinBox*> sb_Channel_MeterNum;
	QVector<QComboBox*> cb_Channel_Mode;
	QVector<QSpinBox*> sb_Channel_Index;
	QVector <QVector<QLabel*>> lb_MeterTitle = QVector <QVector<QLabel*>>(MAXCHANEL, QVector<QLabel*>(MAXMETER));//初始化中的每个元素为NULL
	QVector <QVector<QSpinBox*>> sb_ID = QVector <QVector<QSpinBox*>>(MAXCHANEL, QVector<QSpinBox*>(MAXMETER));
	QVector <QVector<QSpinBox*>> sb_LinkID = QVector <QVector<QSpinBox*>>(MAXCHANEL, QVector<QSpinBox*>(MAXMETER));
	QVector <QVector<QLabel*>> qb_MeterFileNameTitle = QVector <QVector<QLabel*>>(MAXCHANEL, QVector<QLabel*>(MAXMETER));
	QVector<QPushButton*> qb_AddMeter;
	QVector<QPushButton*> qb_ChannelDelet;
	QVector <QVector<QPushButton*>> qb_MeterDelet = QVector <QVector<QPushButton*>>(MAXCHANEL, QVector<QPushButton*>(MAXMETER));
	QVector <QVector<QToolButton*>> tb_ChoseMeterFile = QVector <QVector<QToolButton*>>(MAXCHANEL, QVector<QToolButton*>(MAXMETER));
	//= QVector <QVector<QToolButton*>>(MAXCHANEL, QVector<QToolButton*>());
	//QVector <QVector<QToolButton*>> tb_ChoseMeterFile(10, QVector<QToolButton*>()); //这种不能放在类的声明中   但可以放在cpp中

	QVector<QVector<QString>> MeterFileName = QVector<QVector<QString>>(MAXCHANEL, QVector<QString>(MAXMETER));
	//MeterFileName[i] 第I个vec  MeterFileName[i][l] 第I个vec中的第l个元素


	QPushButton* tb_text[10];/*测试完删除*/


	/*MQTT组设置*/
	QGroupBox* gb_MQTTSetting;/*MQTT组-总设置*/
	QSpinBox* sb_MQTTNum;
	QPushButton* qb_MQTTNumConfirm;
	QVector<QGroupBox*> gb_SingleMQTTSetting;/*单个mqtt设置*/
	QVector<QLineEdit*> MQTTIP;
	QVector<QLineEdit*> MQTTPort;
	QVector<QLineEdit*> MQTTUser;
	QVector<QLineEdit*> MQTTPassWord;
	QVector<QLineEdit*> MQTTClientID;
	QVector<QLineEdit*> MQTTMFName;
	QVector<QLineEdit*> MQTTIsSSl;
	QVector<QLineEdit*> MQTTIsMEW;
	QVector<QPushButton*> qb_DeletMQTT;

	QVBoxLayout* vbl_MQTTLayout;/*MQTT 总垂直布局*/
	QHBoxLayout* hbl_MQTTLayout1;/*上方MQTT 水平布局1*/
	QHBoxLayout* hbl_MQTTLayout2;/*下方MQTT 水平布局2*/

	QVector<QFormLayout*> qfl_SingleMQTTlayout;/*单个MQTT中表单布局*/

	/*串口设置*/
	QGroupBox* gb_RSSetting;/*RS组-总设置*/
	QSpinBox* sb_RSNum;
	QPushButton* qb_RSNumConfirm;
	QVector<QGroupBox*> gb_SingleRSSetting;/*单个RS设置*/
	QVector<QComboBox*> cb_BaudRate;//波特率
	QVector<QComboBox*> cb_ParityBit;//校验位
	QVector<QComboBox*> cb_DatayBit;//数据位
	QVector<QComboBox*> cb_StopBit;//停止位
	QVector<QPushButton*> qb_DeletRS;

	QVBoxLayout* vbl_RSayout;/*RS 总垂直布局*/
	QHBoxLayout* hbl_RSLayout1;/*上方RS 水平布局1*/
	QVBoxLayout* hbl_RSLayout2;/*下方RS 水平布局2*/

	QVector<QHBoxLayout*> hbl_SingleRSlayout;/*单个rs中的水平布局*/

	/*网口设置*/
	QGroupBox* gb_TCPSetting;/*TCP组-总设置*/
	QSpinBox* sb_TCPNum;
	QPushButton* qb_TCPNumConfirm;
	QVector<QGroupBox*> gb_SingleTCPSetting;/*单个TCP设置*/
	QVector<QLineEdit*> le_TCPHost;
	QVector<QLineEdit*> le_TCPPort;
	QVector<QLineEdit*> le_TCPUserName;
	QVector<QLineEdit*> le_TCPPassWord;
	QVector<QPushButton*> qb_DeletTCP;

	QVBoxLayout* vbl_TCPayout;/*TCP 总垂直布局*/
	QHBoxLayout* hbl_TCPLayout1;/*上方TCP 水平布局1*/
	QVBoxLayout* vbl_TCPLayout2;/*下方TCP 垂直布局2*/

	QVector<QHBoxLayout*> hbl_SingleTCPlayout;/*单个TCP中的水平布局*/


	/*FTP设置
	我准备重写
	*/


	/*zk设置*/
	QGroupBox* gb_ZKSetting;/*ZK组-总设置*/
	QCheckBox* cb_OpenZK;
	QLineEdit* le_ZKHost;
	QLineEdit* le_ZKPort;
	QLineEdit* le_ZKUserName;
	QLineEdit* le_ZKPassWord;
	QVBoxLayout* vbl_ZKayout;/*ZK 总垂直布局*/
	QHBoxLayout* hbl_ZKLayout1;/*上方ZK 水平布局1*/
	QHBoxLayout* hbl_ZKLayout2;/*上方ZK 水平布局1*/

	/*eventbuilder设置*/
	QGroupBox* gb_EBSetting;/*eventbuilder组-总设置*/
	QCheckBox* cb_OpenEB;
	QLineEdit* le_EBHost;
	QLineEdit* le_EBPort;
	QVBoxLayout* vbl_EBLayout;/*ZK 总垂直布局*/
	QHBoxLayout* hbl_EBLayout1;/*上方ZK 水平布局1*/
	QHBoxLayout* hbl_EBLayout2;/*下方ZK 水平布局2*/

	/*多用户设置*/
	QGroupBox* gb_MUSetting;/*multiuser组-总设置*/
	QLabel* lb_MUUserIDTitle;
	QCheckBox* cb_OpenMU;
	QLineEdit* le_MUUserID;
	QHBoxLayout* hbl_MULayout;/*multi-user 总水平布局*/


	/*表计通讯质量*/
	QGroupBox* gb_CSSetting;/*Commstatistics组-总设置*/
	QLabel* lb_CSSendIntervalTitle;
	QCheckBox* cb_OpenCS;
	QLineEdit* le_CSSendInterval;
	QHBoxLayout* hbl_CSLayout;/*Commstatistics总水平布局*/

	/*数据保存设置*/
	QGroupBox* gb_RDSetting;/*RetainData组-总设置*/
	QLabel* lb_RDNumlTitle;
	QLineEdit* le_RDNum;
	QLabel* lb_RDDays;
	QLineEdit* le_RDDays;
	QHBoxLayout* hbl_RDLayout;/*RetainData总水平布局*/

	/*热机双备设置*/
	QGroupBox* gb_DMSetting;/*DoubelMachine组-总设置*/
	QCheckBox* cb_OpenDM;
	QButtonGroup* bg_DMIsMorS;
	QRadioButton* rb_DMMaster;
	QRadioButton* rb_DMSlave;
	QLineEdit* le_DMOverTime;
	QLineEdit* le_DMDateTime;
	QLineEdit* le_DMOtherIP;
	QLineEdit* le_DMSerialServerIP;
	QLineEdit* le_DMNetCardName;


	/*打印指定表计报文显示设置*/
	QGroupBox* gb_CMSetting;/*CommuMsg组-总设置*/
	QLabel* lb_CommuMsgIDTitle;
	QCheckBox* cb_OpenCM;
	QLineEdit* le_CommuMsgID;
	QHBoxLayout* hbl_CMLayout;/*CommuMsg 总水平布局*/

	QHBoxLayout* hbl_DMLayout;/*DoubelMachine主从选择水平布局*/
	QFormLayout* qfl_DMLayout;/*DoubelMachine总表单布局*/


	/*操作按钮*/
	QGroupBox* gb_OpSetting;/*操作operation 组-总设置*/
	QPushButton* qb_OpDownAndOpen;
	QPushButton* qb_OpFileMannege;
	QPushButton* qb_OpOpenProjectDir;
	QToolButton* qb_OpOpenOrderCJQ;
	QToolButton* qb_OpSaveToOther;
	QPushButton* qb_OpUpToDevice;
	QPushButton* qb_OpSaveToLocal;
	QPushButton* qb_UpProjecrt;
	QPushButton* qb_TEST;

	QGridLayout* bl_QBLayout;//按钮布局




	/*Json*/
	std::string JsonFilenName;//读取后的json地址 下载后josn的地址 总之是当前操作json文件的唯一地址
	std::string OtherJsonFath;

	/*FTP、SFTP*/
	void CurlInit();
	int FtpGetFile(QString ftpurl, QString user, QString passwd, QString filename);
	int FtpUpFile(QString ftpurl, QString user, QString passwd, QString filename);
	bool IsUTF8;//!为了解决中文路径的打开以及显示，在win环境
	CURL *curl = NULL;
	CURLcode rs;


	void Init();

public slots:

	void slotDownToLocalAndRead();
	void slotSaveAndUp();
	void slotUpProject();

signals:
};
#pragma once
#include <iostream>

using  namespace std;

/*
@brief 从Ftp服务器拿指定的文件，放到指定的文件地址，Ftp服务器的账户和密码
@param ftpurl：要拿的文件的Ftp地址； filename：存放的地址
@return 返回0正常，返回其他的是错误代码
@note  !SFTP的修改  估计是在这里面
*/
int ftpget(string ftpurl, string user, string passwd, string filename);

/*
@brief 往Ftp服务器上传文件
@param ftpurl：Ftp地址；user：Ftp用户名；passwd：密码； filename：本地文件存放的地址
@return -1打开文件失败，-2文件大小有问题；-3Ftp参数设置有问题；
@note
*/
int ftpput(string ftpurl, string user, string passwd, string filename);
#pragma once
#include <iostream>

using  namespace std;

/*
@brief ��Ftp��������ָ�����ļ����ŵ�ָ�����ļ���ַ��Ftp���������˻�������
@param ftpurl��Ҫ�õ��ļ���Ftp��ַ�� filename����ŵĵ�ַ
@return ����0�����������������Ǵ������
@note  !SFTP���޸�  ��������������
*/
int ftpget(string ftpurl, string user, string passwd, string filename);

/*
@brief ��Ftp�������ϴ��ļ�
@param ftpurl��Ftp��ַ��user��Ftp�û�����passwd�����룻 filename�������ļ���ŵĵ�ַ
@return -1���ļ�ʧ�ܣ�-2�ļ���С�����⣻-3Ftp�������������⣻
@note
*/
int ftpput(string ftpurl, string user, string passwd, string filename);
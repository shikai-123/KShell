@echo off
echo  ��ʼ����CJQTool.exe�����Ժ�
ping -n 3 127.0.0.1 > nul
::TASKKILL /IM CJQTool.exe
move ./../Update/CJQTool.exe ./../CJQTool/
if %errorlevel% == 0 ( echo *****************�����ɹ�******************** ) else ( echo *****************����ʧ�ܣ�������********************)
pause
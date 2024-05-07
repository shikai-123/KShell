@echo off
echo  开始升级CJQTool.exe，请稍候
ping -n 3 127.0.0.1 > nul
::TASKKILL /IM CJQTool.exe
move ./../Update/CJQTool.exe ./../CJQTool/
if %errorlevel% == 0 ( echo *****************升级成功******************** ) else ( echo *****************升级失败，请重试********************)
pause
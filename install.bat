@echo off

rem 判断是否已经是管理员权限
copy %windir%\win.ini %windir%\~6B3349A.txt  1> nul 2> nul
if '%errorlevel%' NEQ '0' (goto UACPrompt) else (goto UACAdmin)

:UACPrompt
%1 start "" mshta vbscript:createobject("shell.application").shellexecute("""%~0""","::",,"runas",1)(window.close)
exit /B

:UACAdmin
del %windir%\~6B3349A.txt > nul

taskkill /im werfault.exe /f > nul
copy /Y %windir%\system32\WerFault.exe %windir%\system32\WerFault.exe.bak
copy /Y "%~dp0bin\werfault.exe" %windir%\system32\WerFault.exe
copy /Y %windir%\syswow64\WerFault.exe %windir%\syswow64\WerFault.exe.bak
copy /Y "%~dp0bin\32-bit\werfault.exe" %windir%\syswow64\WerFault.exe
echo Note: If this fails, you may use special tools to delete the original werfault.exe.
pause
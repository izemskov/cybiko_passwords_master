@echo off
"C:\Program Files (x86)\Cybiko\Cybiko_SDK\bin\vcc" -R0 src/*.c res/*.help res/*.inf -o passwords_master.app
if "%1" == "-p1" goto end
pause
:end
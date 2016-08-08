@echo off
setlocal enabledelayedexpansion

net file 1>nul 2>nul
if errorlevel 1 goto :runadmin

set i=0
set a=x32\

pushd %~p0

for %%d in (Debug, Release, x64\Debug, x64\Release) do (

if !i!==2 (
	set a=
)

if not exist %%d md %%d
if not exist "Common\CNLIB\!a!%%d" md "Common\CNLIB\!a!%%d"

for %%f in (%%d\*.dll, %%d\*.lib) do (
	if exist "Common\CNLIB\!a!%%f" del "Common\CNLIB\!a!%%f"
	mklink "Common\CNLIB\!a!%%f" "..\..\..\..\%%f"
)

set /a i+=1
)

popd

endlocal
pause
goto :eof

:runadmin
wscript RunAdmin.vbs %*

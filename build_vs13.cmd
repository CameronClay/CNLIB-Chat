@echo off

if not defined VS120COMNTOOLS (
	echo no build tools
	goto :eof
)

call "%VS120COMNTOOLS%..\..\vc\vcvarsall.bat"

:Win32
msbuild /p:Configuration=Debug;Platform=Win32;PlatformToolset=v120
msbuild /p:Configuration=Release;Platform=Win32;PlatformToolset=v120

:Win64
msbuild /p:Configuration=Debug;Platform=x64;PlatformToolset=v120
msbuild /p:Configuration=Release;Platform=x64;PlatformToolset=v120

:Win32XP
msbuild /p:Configuration=DebugXP;Platform=Win32;PlatformToolset=v120_xp
msbuild /p:Configuration=ReleaseXP;Platform=Win32;PlatformToolset=v120_xp

:Win64XP
msbuild /p:Configuration=DebugXP;Platform=x64;PlatformToolset=v120_xp
msbuild /p:Configuration=ReleaseXP;Platform=x64;PlatformToolset=v120_xp

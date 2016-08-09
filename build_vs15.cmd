@echo off

if not defined VS140COMNTOOLS (
	echo no build tools
	goto :eof
)

call "%VS140COMNTOOLS%..\..\vc\vcvarsall.bat"

:Win32
msbuild /p:Configuration=Debug;Platform=Win32;PlatformToolset=v140
msbuild /p:Configuration=Release;Platform=Win32;PlatformToolset=v140

:Win64
msbuild /p:Configuration=Debug;Platform=x64;PlatformToolset=v140
msbuild /p:Configuration=Release;Platform=x64;PlatformToolset=v140

:Win32XP
msbuild /p:Configuration=DebugXP;Platform=Win32;PlatformToolset=v140_xp
msbuild /p:Configuration=ReleaseXP;Platform=Win32;PlatformToolset=v140_xp

:Win64XP
msbuild /p:Configuration=DebugXP;Platform=x64;PlatformToolset=v140_xp
msbuild /p:Configuration=ReleaseXP;Platform=x64;PlatformToolset=v140_xp

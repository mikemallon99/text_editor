@echo off

set opts=-DENABLE_assert=1
set libs=User32.lib
set CommonCompilerFlags=-MTd -nologo -Gm- -GR- -EHa- -Oi -WX -W4 -wd4013 -wd4201 -wd4100 -wd4189 -wd4505 -wd4310 -FC -Z7 -TC %opts%
set CommonLinkerFlags= -incremental:no -opt:ref 
set cl_opts=-FC -GR- -EHa- -nologo -Zi %opts%
set code=%cd%
set exe_name=test.exe
set dll_name=test_dll.dll

IF NOT EXIST %~dp0..\_build mkdir %~dp0..\_build
pushd %~dp0..\_build

REM 32 bit build
REM cl %CommonCompilerFlags% ..\code\win32_handmade.cpp -Fmwin32_handmade.map /link -subsystem:windows,5.1 %CommonLinkerFlags%

REM 64 bit build
@REM cl %CommonCompilerFlags% ..\code\win32_handmade.c -Fmwin32_handmade.map /link /SUBSYSTEM:CONSOLE %CommonLinkerFlags% 
cl -Fe%exe_name% %cl_opts% %libs% %~dp0\win32_platform.c

popd

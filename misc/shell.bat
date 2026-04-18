@echo off
cmd.exe /k "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

:: Evan's cl path
:: cmd.exe /k "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvarsall.bat" x64
pwd | clip
REM set path=C:\Users\Owner\dev\drgui;%path%

:: NOTE: Currently assuming ONLY msvc
:: Copied from raddebugger and edited
@echo off
setlocal enabledelayedexpansion
cd /D "%~dp0"

:: --- Unpack Arguments ---
for %%a in (%*) do set "%%~a=1"
if not "%release%"=="1" set debug=1
if "%debug%"=="1"   set release=0 && echo [debug mode]
if "%release%"=="1" set debug=0 && echo [release mode]
if "%~1"==""                     echo [default mode, assuming 'drgui' build] && set drgui=1
if "%~1"=="release" if "%~2"=="" echo [default mode, assuming 'drgui' build] && set drgui=1

:: --- Compile/Link Line Definitions ---
set cl_dis_warn=    /wd4201 /wd4100 /wd4189 /wd4505
set cl_common=      /nologo /EHa /Oi /WX /W4 %cl_dis_warn% /FC /Z7
set cl_debug=       call cl /Od /Ob1 /DDR_INTERNAL=1 /Zo  %cl_common%
set cl_release=     call cl /O2 /DDR_INTERNAL=0 %cl_common%
set cl_link=        /link -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib
set cl_out=         /out:
set cl_linker=

:: --- Per-build settings ---
set linker=%cl_linker%
set only_compile=/c
:: Exception handling flag
set EHsc=/EHsc
set no_aslr=/DYNAMICBASE:NO
set rc=call rc
set link_dll=/link /DLL

:: --- Choose Compile/Link Lines ---
set compile_debug=%cl_debug%
set compile_release=%cl_release%
set compile_link=%cl_link%
set out=%cl_out%
if "%debug%"=="1"   set compile=%compile_debug%
if "%release%"=="1" set compile=%compile_release% 

:: --- Prep Directories ---
if not exist .\build mkdir .\build

:: --- Build Everything ---
pushd .\build
if "%drgui%"=="1"       set didbuild=1 && %compile% ..\src\win32_drgui.cpp %compile_link% %out%drgui.exe || exit /b 1
popd

:: --- Warn on No Builds ---
if "%didbuild%"=="" (
    echo [WARNING] no valid build target specified; run .\build.bat release or .\build.bat debug
    exit /b 1
)

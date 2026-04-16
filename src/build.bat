@echo off

set CommonCompilerFlags=-MTd -nologo -Gm- -GR- -EHa- -Zo -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -FC -Z7 -DDR_INTERNAL=1
set CommonLinkerFlags= -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib

IF NOT EXIST .\build mkdir .\build
pushd .\build
REM 64-bit build
cl %CommonCompilerFlags% ..\src\win32_drgui.cpp /link %CommonLinkerFlags%
popd

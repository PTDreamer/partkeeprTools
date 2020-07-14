set PATH=%PATH%;C:\Qt\5.12.2\msvc2017\bin
CALL "C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" amd64
qmake
nmake
windeployqt release\partkeeprTools.exe

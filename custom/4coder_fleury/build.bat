@echo off
ctime -begin 4ed.ctm
call ..\bin\buildsuper_x64-win.bat .\4coder_fleury.cpp %1
copy .\custom_4coder.dll ..\..\custom_4coder.dll
copy .\custom_4coder.pdb ..\..\custom_4coder.pdb
ctime -end 4ed.ctm
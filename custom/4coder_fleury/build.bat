@echo off
ctime -begin 4ed.ctm
call ..\bin\buildsuper_x64-win.bat .\4coder_long.cpp %1
move .\custom_4coder.dll ..\..\custom_4coder.dll
move .\custom_4coder.pdb ..\..\custom_4coder.pdb
move .\vc140.pdb         ..\..\vc140.pdb
ctime -end 4ed.ctm
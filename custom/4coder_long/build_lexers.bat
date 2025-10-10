@echo off
pushd %~dp0

REM The lexer build steps are:
REM 1. Use build_one_time to build the lexer_gen file into one_time.exe
REM 2. Run the one_time.exe to generate the lexer files
REM The one_time.exe can easily failed when the lexer_gen system has bugs
REM The build process only runs successful when the console prints <file_path>:1:

call ..\bin\build_one_time .\4coder_long_cs_lexer_gen.cpp ..\..\
del ..\..\4coder_long_cs_lexer_gen.obj
..\..\one_time.exe

call ..\bin\build_one_time ..\languages\4coder_cpp_lexer_gen.cpp ..\..\
del ..\..\4coder_cpp_lexer_gen.obj
..\..\one_time.exe
popd
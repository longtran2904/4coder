@echo off

REM The lexer build steps are:
REM 1. Use build_one_time to build the lexer_gen file into one_time.exe
REM 2. Run the one_time.exe to generate the lexer files
REM The one_time.exe can easily failed when the lexer_gen system has bugs
REM The build process only runs successful when the console prints <file_path>:1:

call ..\bin\build_one_time .\4coder_long_cs_lexer_gen.cpp ..\..\
..\..\one_time.exe
REM copy ..\generated\lexer_cs.h .\generated\4coder_long_lexer_cs.h
REM copy ..\generated\lexer_cs.cpp .\generated\4coder_long_lexer_cs.cpp

REM call ..\bin\build_one_time .\4coder_fleury_jai_lexer_gen.cpp ..\..\
REM ..\..\one_time.exe
REM copy ..\generated\lexer_jai.h .\generated\4coder_fleury_lexer_jai.h
REM copy ..\generated\lexer_jai.cpp .\generated\4coder_fleury_lexer_jai.cpp

REM call ..\bin\build_one_time ..\languages\4coder_cpp_lexer_gen.cpp ..\..\
REM ..\..\one_time.exe
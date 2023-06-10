@echo off
call ..\bin\build_one_time .\4coder_fleury_jai_lexer_gen.cpp ..\..\
..\..\one_time.exe
copy ..\generated\lexer_jai.h .\generated\4coder_fleury_lexer_jai.h
copy ..\generated\lexer_jai.cpp .\generated\4coder_fleury_lexer_jai.cpp

call ..\bin\build_one_time .\4coder_long_cs_lexer_gen.cpp ..\..\
..\..\one_time.exe
copy ..\generated\lexer_cs.h .\generated\4coder_long_lexer_cs.h
copy ..\generated\lexer_cs.cpp .\generated\4coder_long_lexer_cs.cpp

call ..\bin\build_one_time ..\languages\4coder_cpp_lexer_gen.cpp ..\..\
..\..\one_time.exe
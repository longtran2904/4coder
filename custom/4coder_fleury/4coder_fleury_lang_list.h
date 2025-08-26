/* date = January 29th 2021 9:37 pm */

#ifndef FCODER_FLEURY_LANG_LIST_H
#define FCODER_FLEURY_LANG_LIST_H

// NOTE(rjf): Include language files here.
#include "4coder_fleury_lang_cpp.cpp"
#include "4coder_fleury_lang_metadesk.cpp"
#include "4coder_fleury_lang_jai.cpp"
#include "4coder_long_lang_cs.cpp"
#include "4coder_long_lang_cpp.cpp"

// NOTE(rjf): @f4_register_languages Register languages.
function void F4_RegisterLanguages(void)
{
    // NOTE(rjf): C/C++
    String8 cpp_exts[] = { S8Lit("cpp"), S8Lit("cc"), S8Lit("c"), S8Lit("cxx"), S8Lit("h"), S8Lit("hpp"), };
    for (i32 i = 0; i < ArrayCount(cpp_exts); ++i)
    {
        F4_RegisterLanguage(cpp_exts[i], F4_CPP_IndexFile, lex_full_input_cpp_init, lex_full_input_cpp_breaks,
                            F4_CPP_PosContext, F4_CPP_Highlight, Lex_State_Cpp);
    }
    
    F4_RegisterLanguage(S8Lit("lcpp"), Long_CPP_IndexFile, lex_full_input_cpp_init, lex_full_input_cpp_breaks,
                        Long_CPP_PosContext, 0, Lex_State_Cpp);
    
    // NOTE(long): CS
    F4_RegisterLanguage(S8Lit("cs"), Long_CS_IndexFile, lex_full_input_cs_init, lex_full_input_cs_breaks,
                        Long_CS_PosContext, Long_CS_Highlight, Lex_State_Cs);
    
    // NOTE(rjf): Jai
    F4_RegisterLanguage(S8Lit("jai"), F4_Jai_IndexFile, lex_full_input_jai_init, lex_full_input_jai_breaks,
                        F4_Jai_PosContext, F4_Jai_Highlight, Lex_State_Jai);
    
    // NOTE(rjf): Metadesk
    String8 md_exts[] =
    {
        // TODO(rjf): Maybe find a config-driven way to specify these? "mc" was sort of introduced ad-hoc...
        S8Lit("md"), S8Lit("mc"),
        S8Lit("metacode"), S8Lit("metadesk"),
        S8Lit("meta"), S8Lit("mdesk"),
    };
    
    for (i32 i = 0; i < ArrayCount(md_exts); ++i)
        F4_RegisterLanguage(md_exts[i], F4_MD_IndexFile, F4_MD_LexInit, F4_MD_LexFullInput,
                            F4_MD_PosContext, F4_MD_Highlight, Lex_State_Cpp);
}

#endif //FCODER_FLEURY_LANG_LIST_H

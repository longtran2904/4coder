/* date = November 21st 2024 3:03 pm */

#ifndef FCODER_LONG_BINDINGS_H
#define FCODER_LONG_BINDINGS_H

function void Long_Binding_SetupEssential(Mapping* mapping, i64 global_id, i64 file_id, i64 code_id);
function void   Long_Binding_SetupDefault(Mapping* mapping, i64 global_id, i64 file_id, i64 code_id);
function void    Long_Binding_MultiCursor(Mapping* mapping, i64 global_id, i64 file_id, i64 code_id);

function b32 Long_Binding_LoadData(Application_Links* app, Mapping* mapping, String8 filename, String8 data);

#endif //4CODER_LONG_BINDINGS_H

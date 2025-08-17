/* date = November 21st 2024 3:03 pm */

#ifndef FCODER_LONG_BINDINGS_H
#define FCODER_LONG_BINDINGS_H

//~ NOTE(long): Bindings

function void Long_Binding_SetupEssential(Mapping* mapping);
function void   Long_Binding_SetupDefault(Mapping* mapping);
function void    Long_Binding_MultiCursor();

function b32 Long_Binding_LoadData(Application_Links* app, Mapping* mapping, String8 filename, String8 data);

//~ NOTE(long): Mapping

function Command_Map* Long_Mapping_GetMap(Application_Links* app, View_ID view, Mapping** mapping);
function Command_Metadata* Long_Mapping_GetMetadata(Mapping* mapping, Command_Map* map, User_Input* in);
function b32 Long_Mapping_HandleCommand(Application_Links* app, View_ID view, Mapping** mapping, Command_Map** map, User_Input* in);

#endif //4CODER_LONG_BINDINGS_H

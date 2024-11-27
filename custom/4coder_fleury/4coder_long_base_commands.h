/* date = October 30th 2023 4:22 pm */

#ifndef FCODER_LONG_BASE_COMMANDS_H
#define FCODER_LONG_BASE_COMMANDS_H

//~ NOTE(long): Globals/Macros

global View_ID long_global_active_view;

global b32 global_code_peek_open = 0;

// NOTE(long): first bit is the move_next side, second bit is the move_prev side
global u32 long_global_move_side = 1;

global Range_i64 long_cursor_select_range;

global Character_Predicate long_predicate_alpha_numeric_underscore_dot = { {
        0,   0,   0,   0,   0,   64,  255, 3, 
        254, 255, 255, 135, 254, 255, 255, 7, 
        0,   0,   0,   0,   0,   0,   0,   0, 
        0,   0,   0,   0,   0,   0,   0,   0, 
    } };

//~ NOTE(long): Helper Functions

#define clamp_loop(x, size) ((((x) % (size)) + (size)) % (size))

function b32 Long_F32_Invalid(f32 f);
function b32 Long_Rf32_Invalid(Rect_f32 r);
function i32 Long_Abs(i32 num);
function i64 Long_Abs(i64 num);

function b32        Long_IsPosValid(Application_Links* app, View_ID view, Buffer_ID buffer, i64 pos, i64 current_pos);
function void         Long_SnapView(Application_Links* app, View_ID view = 0);
function void Long_SnapMarkToCursor(Application_Links* app);

function void Long_Jump_ToLocation(Application_Links* app, View_ID view, Buffer_ID buffer, i64 pos);
function void   Long_Jump_ToBuffer(Application_Links* app, View_ID view, Buffer_ID buffer, b32 push_src = 1, b32 push_dst = 1);

function void Long_Print_Messagef(Application_Links* app, char* fmt, ...);
function void   Long_Print_Errorf(Application_Links* app, char* fmt, ...);
#define Long_Print_Error(app, err) Long_Print_Errorf((app), "%.*s", string_expand(err))

function void     Long_Print_AppendVars(Application_Links* app, Arena* arena, String8List* list, Variable_Handle var, i32 indent);
function String8 Long_Print_StrFromVars(Application_Links* app, Arena* arena, Variable_Handle var, i32 indent= 0);

//~ NOTE(long): Query Functions

#define LONG_QUERY_STRING_SIZE KB(1)

function Query_Bar*   Long_Query_StartBar(Application_Links* app, Arena* arena, char* prompt, String8 string);
function Query_Bar* Long_Query_ReserveBar(Application_Links* app, Arena* arena, char* prompt, String8 string);
function void        Long_Query_AppendBar(Query_Bar* bar, String8 string);

function b32 Long_Query_DefaultInput(Application_Links* app, Query_Bar* bar, View_ID view, User_Input* in);
function b32    Long_Query_TextInput(Application_Links* app, Query_Bar* bar, String_Const_u8 init);
function String8  Long_Query_FillBar(Application_Links* app, Arena* arena, char* prompt, String8 space, String8 init);
function String8   Long_Query_String(Application_Links* app, Arena* arena, char* prompt, String8 init = {});

//~ NOTE(long): Project Functions

function String8 Long_Prj_RelBufferName(Application_Links* app, Arena* arena, Buffer_ID buffer);
function void Long_Config_ApplyFromData(Application_Links* app, String8 data, i32 override_font_size, b32 override_hinting);

//~ NOTE(long): Point Stack Functions

struct Long_Point_Stack
{
    Managed_Object markers[POINT_STACK_DEPTH];
    Buffer_ID buffers[POINT_STACK_DEPTH];
    i32 bot;
    
    // NOTE(long): The current and top is one past the actual point index
    i32 current;
    i32 top;
};

function void Long_PointStack_Push(Application_Links* app, Buffer_ID buffer, i64 pos, View_ID view = 0);
function void Long_PointStack_SetCurrent(Long_Point_Stack* stack, i32 index);
function void Long_PointStack_Append(Application_Links* app, Long_Point_Stack* stack, Buffer_ID buffer, i64 pos, View_ID view = 0);

function void Long_PointStack_Jump(Application_Links* app, View_ID view, Buffer_ID target_buffer, i64 target_pos,
                                   Buffer_ID current_buffer = 0, i64 current_pos = 0);
function void Long_PointStack_JumpNext(Application_Links* app, View_ID view, i32 advance);

//~ NOTE(long): History Functions

struct Long_Buffer_History
{
    History_Record_Index index;
    Record_Info record;
};

CUSTOM_ID(attachment, long_buffer_history);

function Long_Buffer_History* Long_Buffer_GetAttachedHistory(Application_Links* app, Buffer_ID buffer);
function Long_Buffer_History   Long_Buffer_GetCurrentHistory(Application_Links* app, Buffer_ID buffer, i32 offset = 0);
function b32                   Long_Buffer_CheckDirtyHistory(Application_Links* app, Buffer_ID buffer = 0, i32 offset = 0);

#endif //4CODER_LONG_BASE_COMMANDS_H

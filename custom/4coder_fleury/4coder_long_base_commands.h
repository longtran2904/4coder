/* date = October 30th 2023 4:22 pm */

#ifndef FCODER_LONG_BASE_COMMANDS_H
#define FCODER_LONG_BASE_COMMANDS_H

//~ NOTE(long): Globals/Macros

global View_ID long_global_active_view;

global b32 global_code_peek_open = 0;

// NOTE(long): first bit is the move_next side, second bit is the move_prev side
global u32 long_global_move_side = 1;

global Range_i64 long_cursor_select_range;

global String8 current_theme_name = {};
#define DEFAULT_THEME_NAME S8Lit("4coder")

global Character_Predicate long_predicate_alpha_numeric_underscore_dot = { {
        0,   0,   0,   0,   0,   64,  255, 3, 
        254, 255, 255, 135, 254, 255, 255, 7, 
        0,   0,   0,   0,   0,   0,   0,   0, 
        0,   0,   0,   0,   0,   0,   0,   0, 
    } };

//~ NOTE(long): Helper Functions

#define clamp_loop(x, size) ((((x) % (size)) + (size)) % (size))

function b32  Long_F32_Invalid(f32 f);
function b32 Long_Rf32_Invalid(Rect_f32 r);
function i32 Long_Abs(i32 num);
function i64 Long_Abs(i64 num);

function b32 Long_IsPosValid(Application_Links* app, View_ID view, Buffer_ID buffer, i64 pos, i64 current_pos);
function void  Long_SnapView(Application_Links* app, View_ID view = 0);

function void Long_Jump_ToLocation(Application_Links* app, View_ID view, Buffer_ID buffer, i64 pos);
function void   Long_Jump_ToBuffer(Application_Links* app, View_ID view, Buffer_ID buffer, b32 push_src = 1, b32 push_dst = 1);

//~ NOTE(long): Project Functions

function String8 Long_Prj_RelBufferName(Application_Links* app, Arena* arena, Buffer_ID buffer);

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

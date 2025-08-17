/* date = October 30th 2023 4:22 pm */

#ifndef FCODER_LONG_BASE_COMMANDS_H
#define FCODER_LONG_BASE_COMMANDS_H

//~ NOTE(long): Globals/Macros

global View_ID long_global_active_view = 0;

// NOTE(long): first bit is the move_next side, second bit is the move_prev side
global u32 long_global_move_side = 1;
global b32 long_global_code_peek_open = 0;
global b32 long_global_pos_context_open    = 1;
global i32 long_global_child_tooltip_count = 20;
global i32 long_active_pos_context_option  = 0;

global Character_Predicate long_predicate_alpha_numeric_underscore_dot = { {
        0,   0,   0,   0,   0,   64,  255, 3, 
        254, 255, 255, 135, 254, 255, 255, 7, 
        0,   0,   0,   0,   0,   0,   0,   0, 
        0,   0,   0,   0,   0,   0,   0,   0, 
    } };

#define Long_Rewrite_ReplaceRange (Rewrite_WordComplete+1)

//~ NOTE(long): Helper Functions

#define WrapIndex(x, size) ((((x) % (size)) + (size)) % (size))
#define Long_View_ForEach(view, app, flags) \
    for (View_ID view = get_view_next(app, 0, flags); view; view = get_view_next(app, view, flags))

#define StrStartsWith(str, needle, ...) string_match(string_prefix(str, needle.size), needle, __VA_ARGS__)

function b32 Long_F32_Invalid(f32 f);
function b32 Long_Rf32_Invalid(Rect_f32 r);
function Rect_f32 Long_Rf32_Round(Rect_f32 r);
function i32 Long_Abs(i32 num);
function i64 Long_Abs(i64 num);

function View_ID Long_View_NextPassive(Application_Links* app, View_ID view, Access_Flag access);
function View_ID Long_View_Open(Application_Links* app, View_ID view, Buffer_ID buffer, View_Split_Position position, b32 passive);
function void    Long_View_Snap(Application_Links* app, View_ID view);

function void       Long_Mouse_ScrollWheel(Application_Links* app, View_ID view);
function void        Long_SnapMarkToCursor(Application_Links* app);
function void Long_ConfirmMarkSnapToCursor(Application_Links* app);

function b32       Long_IsPosValid(Application_Links* app, View_ID view, Buffer_ID buffer, i64 pos, i64 current_pos);
function void Long_Jump_ToLocation(Application_Links* app, View_ID view, Buffer_ID buffer, i64 pos);
function void   Long_Jump_ToBuffer(Application_Links* app, View_ID view, Buffer_ID buffer, b32 push_src = 1, b32 push_dst = 1);

function String8 Long_Buffer_PushLine(Application_Links* app, Arena* arena, Buffer_ID buffer, i64 pos);
function Buffer_ID Long_Buffer_GetSearchBuffer(Application_Links* app);
function b32 Long_Buffer_IsSearch(Application_Links* app, Buffer_ID buffer);
function b32 Long_Buffer_IsSpecial(Application_Links* app, String8 name);

function void Long_Print_Messagef(Application_Links* app, char* fmt, ...);
function void   Long_Print_Errorf(Application_Links* app, char* fmt, ...);
#define Long_Print_Error(app, err) Long_Print_Errorf((app), "%.*s", string_expand(err))

//~ NOTE(long): Vars/Config Functions

#define vars_read_key_lit(var, key) vars_read_key((var), vars_save_string_lit(key))
#define vars_read_root_key_lit(key) vars_read_key(vars_get_root(), vars_save_string_lit(key))

#define vars_str_from_key(arena, var, key) vars_string_from_var(arena, vars_read_key(var, key))
#define vars_str_from_key_lit(arena, var, key) vars_string_from_var(arena, vars_read_key_lit(var, key))
#define vars_b32_from_key(var, key) vars_b32_from_var(vars_read_key(var, key))
#define vars_b32_from_key_lit(var, key) vars_b32_from_var(vars_read_key_lit(var, key))

#define def_get_config_b32_lit(key) def_get_config_b32(vars_save_string_lit(key))
#define def_toggle_config_b32_lit(key) Stmnt(String_ID __id__ = vars_save_string_lit(key); \
                                             def_set_config_b32(__id__, !def_get_config_b32(__id__));)
#define def_get_config_u64_lit(app, key) def_get_config_u64(app, vars_save_string_lit(key))
#define def_get_config_str_lit(arena, key) def_get_config_string(arena, vars_save_string_lit(key))
#define def_get_config_f32_lit(app, key) ((f32)def_get_config_u64((app), vars_save_string_lit(key)))

function void    Long_Vars_Append(Application_Links* app, Arena* arena, String8List*  list, Variable_Handle var, i32 indent);
function String8 Long_Vars_StrFromArray(Application_Links* app, Arena* arena, Variable_Handle var, i32 indent, String8List ignore = {});
function String8 Long_Vars_SingleStrArray(Application_Links* app, Arena* arena, Variable_Handle var);

//~ NOTE(long): Search Buffer Functions

function Marker* Long_SearchBuffer_GetMarkers(Application_Links* app, Arena* arena, Managed_Scope search_scope,
                                              Buffer_ID buffer, i32* out_count);
function void Long_SearchBuffer_Jump(Application_Links* app, Locked_Jump_State state, Sticky_Jump jump);
function Sticky_Jump Long_SearchBuffer_NearestJump(Application_Links* app, Buffer_ID buffer, i64 pos,
                                                   Marker_List* list, Managed_Scope scope, Sticky_Jump_Stored* out_stored);

//~ NOTE(long): List Functions

function Buffer_ID Long_CreateOrSwitchBuffer(Application_Links* app, String8 name_string, View_ID default_target_view);
function String_Match_List Long_Matches_Find(Application_Links* app, Arena* arena, String8Array match_patterns,
                                             String_Match_Flag must_have, String_Match_Flag must_not_have, Buffer_ID current);
function void Long_Matches_Print(Application_Links* app, String8Array match_patterns, String_Match_Flag must_have_flags,
                                 String_Match_Flag must_not_have_flags, Buffer_ID out_buffer_id, Buffer_ID current_buffer);

//~ NOTE(long): Query Functions

#define LONG_QUERY_STRING_SIZE KB(1)

function Query_Bar*   Long_Query_StartBar(Application_Links* app, Arena* arena, char* prompt, String8 string);
function Query_Bar* Long_Query_ReserveBar(Application_Links* app, Arena* arena, char* prompt, String8 string);
function void        Long_Query_AppendBar(Query_Bar* bar, String8 string);

function b32 Long_Query_DefaultInput(Application_Links* app, Query_Bar* bar, View_ID view, User_Input* in);
function b32      Long_Query_General(Application_Links* app, Query_Bar* bar, String8 init, u32 force_radix = 0);
function String8 Long_Query_StringEx(Application_Links* app, Arena* arena, char* prompt, u8* string_space,
                                     u64 space_size, String8 init);
function String8   Long_Query_String(Application_Links* app, Arena* arena, char* prompt, String8 init = {});

//~ NOTE(long): Search Functions

function void Long_ISearch(Application_Links* app, Scan_Direction start_scan, i64 first_pos, String8 init, b32 insensitive);
function void  Long_Search(Application_Links* app, Scan_Direction dir, b32 identifier, b32 insensitive);

//~ NOTE(long): Replace Functions

function void Long_Query_Replace(Application_Links* app, String8 replace_str, i64 start_pos);
function void Long_Query_Replace(Application_Links* app, Buffer_ID buffer, Range_i64 range);
function void Long_Query_ReplaceRange(Application_Links* app, View_ID view, Range_i64 range);

//~ NOTE(long): Project Functions

function String8 Long_Prj_RelBufferName(Application_Links* app, Arena* arena, Buffer_ID buffer);
function void Long_Prj_Print(Application_Links* app, Variable_Handle prj_var);
function Variable_Handle Long_Prj_Parse(Application_Links* app, String8 filename, String8 data);
function void Long_Prj_Load(Application_Links* app, Variable_Handle prj_var);

function void Long_Config_Print(Application_Links* app);
function void Long_Config_ApplyFromData(Application_Links* app, String8 data, i32 override_font_size, b32 override_hinting);

//~ NOTE(long): Point Stack Functions

// @IMPORTANT(long): This must be a power of two
#define LONG_POINT_STACK_DEPTH 64
#define PS_WrapIndex(idx) ((idx) & (LONG_POINT_STACK_DEPTH - 1))

struct Long_Point_Stack
{
    Managed_Object markers[LONG_POINT_STACK_DEPTH];
    Buffer_ID      buffers[LONG_POINT_STACK_DEPTH];
    u32 bot;
    u32 curr;
    u32 top; // opl
};

CUSTOM_ID(attachment, long_point_stacks);

function void Long_PointStack_Push(Application_Links* app, Buffer_ID buffer, i64 pos, View_ID view = 0);
function void Long_PointStack_PushCurrent(Application_Links* app, View_ID view);
function void Long_PointStack_SetCurrent(Long_Point_Stack* stack, i32 index);
function void Long_PointStack_Append(Application_Links* app, Long_Point_Stack* stack, Buffer_ID buffer, i64 pos, View_ID view = 0);

function void Long_PointStack_Jump(Application_Links* app, View_ID view, Buffer_ID target_buffer, i64 target_pos);
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

/* date = December 13th 2022 11:45 am */

#ifndef FCODER_LONG_INDEX_H
#define FCODER_LONG_INDEX_H

//~ NOTE(long): Global IDs

CUSTOM_ID(colors, long_color_index_field);
CUSTOM_ID(colors, long_color_index_local);
CUSTOM_ID(colors, long_color_index_param);
// TODO(long): color for globals

//~ NOTE(long): Typedefs

#define LONG_INDEX_FILTER(name) b32 name(F4_Index_Note* note)
typedef LONG_INDEX_FILTER(NoteFilter);
#define LONG_INDEX_FILTER_NOTE Long_Filter_Note

function LONG_INDEX_FILTER(Long_Filter_Note);

//~ NOTE(long): Indexing Functions

//- NOTE(long): Predicate Functions
function b32 Long_Index_IsMatch(F4_Index_ParseCtx* ctx, Range_i64 range, String8* array, u64 count);
function b32 Long_Index_IsMatch(F4_Index_ParseCtx* ctx, Token* token, String8* array, u64 count);
function b32 Long_Index_IsMatch(String8 string, String8* array, u64 count);
function b32 Long_Index_MatchNote(Application_Links* app, F4_Index_Note* note, Range_i64 range, String8 match);

function b32 Long_Index_IsComment(F4_Index_NoteKind* note);
function b32 Long_Index_IsGenericArgument(F4_Index_Note* note);
function b32 Long_Index_IsLambda(F4_Index_Note* note);
function b32 Long_Index_IsNamespace(F4_Index_Note* note);

//- NOTE(long): Parsing Functions
function b32 Long_Index_SkipExpression(F4_Index_ParseCtx* ctx, i16 seperator, i16 terminator);
function b32 Long_Index_SkipBody(F4_Index_ParseCtx* ctx, b32 dec = 0, b32 exit_early = 0);
function b32 Long_Index_SkipBody(Application_Links* app, Token_Iterator_Array* it, Buffer_ID buffer, b32 dec = 0, b32 exit_early = 0);
function b32 Long_Index_ParsePattern(F4_Index_ParseCtx* ctx, char* fmt, ...);
function b32 Long_Index_PeekPattern(F4_Index_ParseCtx* ctx, char* fmt, ...);

//- NOTE(long): Inline version of Parse/PeekPattern
function b32 Long_Index_ParseStr (F4_Index_ParseCtx* ctx, String8 str);
function b32 Long_Index_PeekStr  (F4_Index_ParseCtx* ctx, String8 str);
function b32 Long_Index_ParseKind(F4_Index_ParseCtx* ctx, Token_Base_Kind kind, Range_i64* out_range);
function b32 Long_Index_ParseSubKind(F4_Index_ParseCtx* ctx, i64 sub_kind, Range_i64* out_range);

//- NOTE(long): Init Functions
function void Long_Index_UpdateTick(Application_Links* app);
function F4_Index_Note* Long_Index_MakeNote(F4_Index_ParseCtx* ctx, Range_i64 base_range, Range_i64 range, F4_Index_NoteKind kind,
                                            b32 push_parent = true);
function F4_Index_Note* Long_Index_MakeNamespace(F4_Index_ParseCtx* ctx, Range_i64 base, Range_i64 name);
function F4_Index_Note* Long_Index_PushNamespaceScope(F4_Index_ParseCtx* ctx);
function F4_Index_Note* Long_Index_PopNamespaceScope(F4_Index_ParseCtx* ctx);

//-  NOTE(long): Lookup Functions
function F4_Index_Note* Long_Index_LookupNote(String_Const_u8 string);
function F4_Index_Note* Long_Index_LookupChild(F4_Index_Note* parent, i32 index);
function F4_Index_Note* Long_Index_LookupChild(String_Const_u8 string, F4_Index_Note* parent = 0);
function F4_Index_Note* Long_Index_LookupBestNote(Application_Links* app, Buffer_ID buffer, Token_Array* array, Token* token,
                                                  F4_Index_Note* filter_note = 0, b32 continue_after_comma = 0);

//- NOTE(long): Render Functions
function Vec2_f32 Long_Index_DrawTooltip(Application_Links* app, Rect_f32 screen_rect, Vec2_f32 tooltip_pos,
                                         Face_ID face, f32 padding, f32 line_height, ARGB_Color color, ARGB_Color highlight_color,
                                         F4_Index_Note* note, i32 index, Range_i64 range, Range_i64 highlight_range);
function void Long_Index_DrawPosContext(Application_Links* app, View_ID view, F4_Language_PosContextData* first_ctx);
function void Long_Index_DrawCodePeek(Application_Links* app, View_ID view);

//- NOTE(long): Buffer Functions
function void Long_Index_IndentBuffer(Application_Links* app, Buffer_ID buffer, Range_i64 range, b32 merge_history = false);
function i32 Long_SaveFile(Application_Links *app, Buffer_ID buffer_id);
function i32 Long_EndBuffer(Application_Links* app, Buffer_ID buffer_id);

//~ NOTE(long): Macros

#define Long_Index_HasScopeRange(note) ((note)->scope_range.min > (note)->range.max)
#define Long_Index_IsArgument(note) ((note)->range.max < (note)->parent->scope_range.min)
#define Long_Index_ArgumentRange(note) (Range_i64{ (note)->range.min, Max((note)->range.max, (note)->scope_range.min) })

#define Long_Index_IterateValidNoteInFile(child, note) \
    for (F4_Index_Note* child = (note)->first_child; \
         child != ((note)->last_child ? (note)->last_child->next_sibling : 0); \
         child = child->next_sibling) if (!Long_Index_IsNamespace(child))

#define Long_Index_IterateValidNoteInFile_Dir(child, note, forward) \
    F4_Index_Note* start = (forward) ? (note)->first_child : (note)->last_child; \
    F4_Index_Note* end   = (forward) ? (note)->last_child : (note)->first_child; \
    if (end) end = (forward) ? end->next_sibling : end->prev_sibling;\
    for (F4_Index_Note* child = start; child != end; child = (forward) ? child->next_sibling : child->prev_sibling) \
    if (!Long_Index_IsNamespace(child))

#define Long_Index_Token(ctx) token_it_read(&(ctx)->it)
#define Long_Index_CtxScope(ctx) (ctx)->active_parent->scope_range
#define Long_Index_CtxCompare(ctx, compare) ((ctx)->active_parent && ((ctx)->active_parent->compare))
#define Long_Index_PopParent(ctx) F4_Index_PopParent((ctx), (ctx)->active_parent->parent)
#define Long_Index_IsParentInitialized(ctx) !((ctx)->active_parent && (Long_Index_CtxScope(ctx).start == 0))

#define Long_Index_StartCtxScope(ctx) Long_Index_CtxScope(ctx).start = (ctx)->it.ptr->pos
#define Long_Index_EndCtxScope(ctx)   Long_Index_CtxScope(ctx).end   = (ctx)->it.ptr->pos
#define Long_Index_BeginCtxChange(ctx) F4_Index_ParseCtx __currentCtx__ = *(ctx)
#define Long_Index_EndCtxChange(ctx) *(ctx) = __currentCtx__

#define Long_Index_BlockCtxScope(ctx, stm) do { \
        Long_Index_StartCtxScope(ctx); stm; Long_Index_EndCtxScope(ctx); \
    } while (0)
#define Long_Index_BlockCtxChange(ctx, stm) do { \
        Long_Index_BeginCtxChange(ctx); stm; Long_Index_EndCtxChange(ctx); \
    } while (0)
#define Long_Index_PeekPrevious(ctx, stm) do { \
        if (token_it_dec(&(ctx)->it)) { stm; token_it_inc(&(ctx)->it); } \
    } while (0)

#endif //4CODER_LONG_INDEX_H

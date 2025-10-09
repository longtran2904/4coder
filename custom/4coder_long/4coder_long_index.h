/* date = December 13th 2022 11:45 am */

#ifndef FCODER_LONG_INDEX_H
#define FCODER_LONG_INDEX_H

//~ NOTE(long): Global IDs

CUSTOM_ID(colors, long_color_index_field);
CUSTOM_ID(colors, long_color_index_local);
CUSTOM_ID(colors, long_color_index_param);
// @CONSIDER(long): color for globals

//~ NOTE(long): Typedefs

#define LONG_INDEX_FILTER(name) b32 name(F4_Index_Note* note)
typedef LONG_INDEX_FILTER(NoteFilter);
#define LONG_INDEX_FILTER_NOTE Long_Filter_Note

function LONG_INDEX_FILTER(Long_Filter_Note);

//~ NOTE(long): Indexing Functions

//- NOTE(long): Predicate Functions
function b32 Long_Index_IsMatch(F4_Index_ParseCtx* ctx, Range_i64 range, String8* array, u64 count);
function b32 Long_Index_IsMatch(F4_Index_ParseCtx* ctx, Token* token, String8* array, u64 count);
function b32 Long_Index_IsMatch(F4_Index_ParseCtx* ctx, Range_i64 range, String8 str);
function b32 Long_Index_IsMatch(String8 string, String8* array, u64 count);
function b32 Long_Index_MatchNote(Application_Links* app, F4_Index_Note* note, Range_i64 range, String8 match);

function b32 Long_Index_IsComment(F4_Index_NoteKind* note);
function b32 Long_Index_IsGenericArgument(F4_Index_Note* note);
function b32 Long_Index_IsLambda(F4_Index_Note* note);
function b32 Long_Index_IsNamespace(F4_Index_Note* note);

//- NOTE(long): Parsing Functions
function b32 Long_ParseCtx_Inc(F4_Index_ParseCtx* ctx);
function b32 Long_Index_ParseAngleBody(Application_Links* app, Buffer_ID buffer, Token* token, i32* out_nest);
function b32 Long_Index_SkipExpression(F4_Index_ParseCtx* ctx, i16 seperator, i16 terminator);
function b32 Long_Index_SkipBody(F4_Index_ParseCtx* ctx, b32 dec = 0, b32 exit_early = 0);
function b32 Long_Index_SkipBody(Application_Links* app, Token_Iterator_Array* it, Buffer_ID buffer, b32 dec = 0, b32 exit_early = 0);
function b32 Long_Index_ParsePattern(F4_Index_ParseCtx* ctx, char* fmt, ...);
function b32 Long_Index_PeekPattern(F4_Index_ParseCtx* ctx, char* fmt, ...);

//- NOTE(long): Inline version of Parse/PeekPattern
function b32      Long_Index_PeekStr(F4_Index_ParseCtx* ctx, String8 str);
function b32     Long_Index_ParseStr(F4_Index_ParseCtx* ctx, String8 str);
function b32     Long_Index_PeekKind(F4_Index_ParseCtx* ctx, Token_Base_Kind kind, Range_i64* out_range);
function b32    Long_Index_ParseKind(F4_Index_ParseCtx* ctx, Token_Base_Kind kind, Range_i64* out_range);
function b32  Long_Index_PeekSubKind(F4_Index_ParseCtx* ctx, i64 sub_kind, Range_i64* out_range);
function b32 Long_Index_ParseSubKind(F4_Index_ParseCtx* ctx, i64 sub_kind, Range_i64* out_range);
function void Long_Index_SkipPreproc(F4_Index_ParseCtx* ctx);

//- NOTE(long): Init Functions
function void Long_Index_Tick(Application_Links* app);
function void Long_Index_ClearFile(F4_Index_File* file);

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
function Range_i64 Long_Index_PosContextRange(Application_Links* app, Buffer_ID buffer, i64 pos);
function void Long_Index_DrawPosContext(Long_Render_Context* ctx, F4_Language_PosContextData* first_pos_ctx);
function void Long_Index_DrawCodePeek(Long_Render_Context* ctx);

//- NOTE(long): Indent Functions
function void Long_Index_IndentBuffer(Application_Links* app, Buffer_ID buffer, Range_i64 range, b32 merge_history = false);

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

#define Long_Parse_Scope(ctx) (ctx)->active_parent->scope_range
#define Long_Parse_Comp(ctx, compare) ((ctx)->active_parent && ((ctx)->active_parent->compare))
#define Long_Index_PopParent(ctx) F4_Index_PopParent((ctx), (ctx)->active_parent->parent)
#define Long_Index_IsParentInitialized(ctx) !((ctx)->active_parent && (Long_Parse_Scope(ctx).start == 0))

#define Long_Index_StartCtxScope(ctx) Long_Parse_Scope(ctx).start = (ctx)->it.ptr->pos
#define Long_Index_EndCtxScope(ctx)   Long_Parse_Scope(ctx).end   = (ctx)->it.ptr->pos
#define Long_Index_ScopeBlock(ctx) \
    for (i64 _temp_ = ((Long_Index_StartCtxScope(ctx)), 1); _temp_; Long_Index_EndCtxScope(ctx), _temp_ = 0)

#define Long_Index_IterBlock(ctx) \
    for (Token* _ptr_ = (ctx)->it.ptr; _ptr_; (ctx)->it.ptr = _ptr_, _ptr_ = 0)
//#define Long_Index_PeekPrevious(ctx, stm) do { \
//if (token_it_dec(&(ctx)->it)) { stm; token_it_inc(&(ctx)->it); } \
//} while (0)
#define Long_Index_PeekPrevious(ctx) \
    for (Token* _ptr_ = (ctx)->it.ptr; _ptr_ && token_it_dec(&(ctx)->it); (ctx)->it.ptr = _ptr_, _ptr_ = 0)

#endif //4CODER_LONG_INDEX_H

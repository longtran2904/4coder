/* date = December 13th 2022 11:45 am */

#ifndef FCODER_LONG_INDEX_H
#define FCODER_LONG_INDEX_H

function b32 Long_Index_IsMatch(F4_Index_ParseCtx* ctx, Range_i64 range, String8* array, u64 count);
function b32 Long_Index_IsMatch(F4_Index_ParseCtx* ctx, Token* token, String8* array, u64 count);
function b32 Long_Index_SkipExpression(F4_Index_ParseCtx* ctx, i16 seperator, i16 terminator);
function b32 Long_Index_SkipBody(F4_Index_ParseCtx* ctx, b32 dec = 0, b32 exit_early = 0);
function b32 Long_Index_SkipBody(Application_Links* app, Token_Iterator_Array* it, Buffer_ID buffer, b32 dec = 0, b32 exit_early = 0);
function b32 Long_Index_ParsePattern(F4_Index_ParseCtx* ctx, char* fmt, ...);
function b32 Long_Index_PeekPattern(F4_Index_ParseCtx* ctx, char* fmt, ...);

function F4_Index_Note* Long_Index_MakeNote(F4_Index_ParseCtx* ctx, Range_i64 base_range, Range_i64 range, F4_Index_NoteKind kind,
                                            b32 push_parent = true);
function F4_Index_Note* Long_Index_LookupChild(F4_Index_Note* parent, String8 name);
function F4_Index_Note* Long_Index_LookupChild(F4_Index_Note* parent, i32 index);
function F4_Index_Note* Long_Index_LookupRef(Application_Links* app, Token_Array* array, F4_Index_Note* note);
function F4_Index_Note* Long_Index_LookupNote(String_Const_u8 string);
function F4_Index_Note* Long_Index_LookupBestNote(Application_Links* app, Buffer_ID buffer, Token_Array* array, Token* token,
                                                  b32 preferNewNote = 0);

#define Long_Index_CurrentToken(ctx) token_it_read(&(ctx)->it)
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

function void Long_Index_DrawTooltip(Application_Links* app, View_ID view, Token_Array* array, F4_Index_Note* note, b32 display_ref, Vec2_f32* tooltip_offset);

#endif //4CODER_LONG_INDEX_H

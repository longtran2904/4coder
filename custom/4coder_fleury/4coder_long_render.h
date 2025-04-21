/* date = October 3rd 2024 1:43 pm */

#ifndef FCODER_LONG_RENDER_H
#define FCODER_LONG_RENDER_H

//~ NOTE(long): Cursor Rendering

enum Cursor_Type
{
    cursor_none,
    cursor_open_range,
    cursor_close_range,
};

function void Long_Render_DrawBlock(Application_Links* app, Text_Layout_ID layout, Range_i64 range, f32 roundness, FColor color);
function Rect_f32 Long_Render_CursorRect(Application_Links *app, View_ID view, Buffer_ID buffer, Text_Layout_ID layout,
                                         i64 cursor, i64 mark, f32 roundness, f32 thickness);
function void Long_Render_NotepadCursor(Application_Links *app, View_ID view_id, Buffer_ID buffer, Text_Layout_ID text_layout_id,
                                        Frame_Info frame_info, f32 roundness, f32 outline_thickness, b32 is_active_view);
function void Long_Render_EmacsCursor(Application_Links* app, View_ID view_id, Buffer_ID buffer, Text_Layout_ID text_layout_id,
                                      Frame_Info frame_info, f32 roundness, f32 outline_thickness, b32 is_active_view);
function void Long_HighlightCursorMarkRange(Application_Links* app, View_ID view_id);

//~ NOTE(long): Tooltip Rendering

function Fancy_Block Long_Render_LayoutString(Application_Links* app, Arena* arena, String8 string,
                                              Face_ID face, f32 max_width, b32 newline);
function void Long_Render_TooltipRect(Application_Links *app, Rect_f32 rect, f32 roundness = 4.f);
function void Long_Render_DrawPeek(Application_Links* app, Rect_f32 tooltip_rect, Rect_f32 peek_rect, Rect_f32 region,
                                   Buffer_ID buffer, Range_i64 range, i64 line_offset, f32 roundness = 4.f);
function void Long_Render_DrawPeek(Application_Links* app, Rect_f32 rect, Buffer_ID buffer,
                                   Range_i64 range, i64 line_offset, f32 roundness = 4.f);
function Rect_f32 Long_Render_DrawString(Application_Links* app, String8 string, Vec2_f32 tooltip_position, Rect_f32 region,
                                         Face_ID face, f32 line_height, f32 padding, ARGB_Color color, f32 roundness = 4.f);

//~ NOTE(long): Misc

function void     Long_Render_CalcComments(Application_Links* app, View_ID view, Buffer_ID buffer,
                                           Text_Layout_ID layout, Frame_Info frame);
function void  Long_Render_DividerComments(Application_Links* app, Buffer_ID buffer, Text_Layout_ID layout);
function void Long_Render_LineOffsetNumber(Application_Links* app, View_ID view, Buffer_ID buffer,
                                           Face_ID face, Text_Layout_ID layout, Rect_f32 margin);

//~ NOTE(long): Highlight Rendering

struct Long_Highlight_Node
{
    Long_Highlight_Node* next;
    Range_i64 range;
};

struct Long_Highlight_List
{
    Arena arena;
    Buffer_ID buffer;
    Long_Highlight_Node* first;
};

CUSTOM_ID(attachment, long_highlight_list);

function Long_Highlight_List* Long_Highlight_GetList(Application_Links* app, View_ID view);
function void Long_Highlight_Push(Application_Links* app, View_ID view, Range_i64 range);
function void Long_Highlight_Clear(Application_Links* app, View_ID view);
function void Long_Highlight_ClearList(Long_Highlight_List* list);
function void Long_Highlight_DrawRangeList(Application_Links* app, View_ID view, Buffer_ID buffer,
                                           Text_Layout_ID layout, f32 roundness);

function void Long_Highlight_DrawErrors(Application_Links* app, Buffer_ID buffer, Text_Layout_ID layout, Buffer_ID jump_buffer);
function void Long_Highlight_DrawList  (Application_Links* app, Buffer_ID buffer, Text_Layout_ID layout, f32 roundness, f32 thickness);
function void Long_MC_DrawHighlights   (Application_Links* app, View_ID view, Buffer_ID buffer,
                                        Text_Layout_ID layout, f32 roundness, f32 thickness, b32 is_active_view);
function b32  Long_Highlight_DrawRange (Application_Links* app, View_ID view, Buffer_ID buffer, Text_Layout_ID layout, f32 roundness);
function b32  Long_Highlight_HasRange  (Application_Links* app, View_ID view);

//~ NOTE(long): Colors

//- NOTE(long): F4 Forward Declaration
struct ColorCtx;
function ColorCtx ColorCtx_Token(Token token, Buffer_ID buffer);
function b32 F4_ARGBIsValid(ARGB_Color color);

//- NOTE(long): Actual Functions
function ARGB_Color Long_ARGBFromID(Managed_ID id, i32 subindex = 0);
function ARGB_Color Long_Color_Alpha(ARGB_Color color, f32 alpha);
function ARGB_Color Long_Color_Cursor(Application_Links* app, b32 is_macro, keybinding_mode mode);

function void Long_SyntaxHighlight(Application_Links* app, Text_Layout_ID text_layout_id, Token_Array* array);
function void Long_Render_HexColor(Application_Links* app, View_ID view, Buffer_ID buffer, Text_Layout_ID layout);
function void Long_Render_CommentHighlight(Application_Links* app, Buffer_ID buffer, Text_Layout_ID layout,
                                           Token_Array* array, Comment_Highlight_Pair* pairs, i32 pair_count);

function void     Long_Render_FadeError(Application_Links* app, Buffer_ID buffer, Range_i64 range);
function void Long_Render_FadeHighlight(Application_Links* app, Buffer_ID buffer, Range_i64 range);


#endif //FCODER_LONG_RENDER_H

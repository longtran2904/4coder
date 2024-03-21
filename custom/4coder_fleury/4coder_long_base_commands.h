/* date = October 30th 2023 4:22 pm */

#ifndef FCODER_LONG_BASE_COMMANDS_H
#define FCODER_LONG_BASE_COMMANDS_H

#define clamp_loop(x, size) ((((x) % (size)) + (size)) % (size))

//- NOTE(long): Render functions
function Fancy_Block Long_Render_LayoutString(Application_Links* app, Arena* arena, String8 string,
                                              Face_ID face, f32 max_width, b32 newline);
function void Long_Render_DrawBlock(Application_Links* app, Text_Layout_ID layout, Range_i64 range, f32 roundness, FColor color);
function void Long_Render_NotepadCursor(Application_Links *app, View_ID view, Buffer_ID buffer, Text_Layout_ID layout,
                                        i64 cursor, i64 mark, f32 roundness, f32 thickness);
function void Long_Render_TooltipRect(Application_Links *app, Rect_f32 rect, f32 roundness = 4.f);
function void Long_Render_DrawPeek(Application_Links* app, Rect_f32 tooltip_rect, Rect_f32 peek_rect, Rect_f32 region,
                                   Buffer_ID buffer, Range_i64 range, i64 line_offset, f32 roundness = 4.f);
function void Long_Render_DrawPeek(Application_Links* app, Rect_f32 rect, Buffer_ID buffer,
                                   Range_i64 range, i64 line_offset, f32 roundness = 4.f);
function Rect_f32 Long_Render_DrawString(Application_Links* app, String8 string, Vec2_f32 tooltip_position, Rect_f32 region,
                                         Face_ID face, f32 line_height, f32 padding, ARGB_Color color, f32 roundness = 4.f);
function void Long_Render_LineOffsetNumber(Application_Links *app, View_ID view_id, Buffer_ID buffer,
                                           Face_ID face_id, Text_Layout_ID text_layout_id, Rect_f32 margin);

//- NOTE(long): Project functions
function String8 Long_Buffer_NameNoProjPath(Application_Links* app, Arena* arena, Buffer_ID buffer);

//- NOTE(long): Highlight functions
function void Long_Highlight_DrawList(Application_Links *app, Buffer_ID buffer, Text_Layout_ID layout, f32 roundness, f32 thickness);
function b32  Long_Highlight_DrawRange(Application_Links *app, View_ID view, Buffer_ID buffer, Text_Layout_ID layout, f32 roundness);

// NOTE(long): Fleury's functions
function b32 F4_ARGBIsValid(ARGB_Color color);

#endif //4CODER_LONG_BASE_COMMANDS_H

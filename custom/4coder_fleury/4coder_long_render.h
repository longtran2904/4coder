/* date = October 3rd 2024 1:43 pm */

#ifndef FCODER_LONG_RENDER_H
#define FCODER_LONG_RENDER_H

//~ NOTE(long): Render Context

struct Long_Render_Context
{
    Application_Links* app;
    Text_Layout_ID layout;
    Token_Array array;
    Buffer_ID buffer;
    Vec2_f32 mouse;
    Range_i64 visible_range;
    
    Frame_Info frame;
    Rect_f32 rect;
    View_ID view;
    b32 is_active_view;
    
    Face_ID face;
    Face_Metrics metrics;
};

#define Long_Render_CtxBlock(ctx, ...) \
    for (Long_Render_Context _rst_ = *(ctx), *_i_ = (__VA_ARGS__, (ctx)); _i_; _i_ = 0, *(ctx) = _rst_)

// NOTE(long): This doesn't update the Text_Layout_ID
#define Long_Render_PushCtxBuffer(ctx, buf) \
    Long_Render_CtxBlock(ctx, (ctx)->buffer = (buf), (ctx)->array = get_token_array_from_buffer((ctx)->app, (buf)))

#define Long_Render_TooltipOffset(ctx, value) (def_get_config_f32_lit((ctx)->app, "tooltip_thickness") + \
                                               ((ctx)->metrics.value))

function Long_Render_Context Long_Render_CreateCtx(Application_Links* app, Frame_Info frame, View_ID view);
function void Long_Render_InitCtxLayout(Long_Render_Context* ctx, Text_Layout_ID layout, Token_Array array);
function void Long_Render_InitCtxFace(Long_Render_Context* ctx, Face_ID face);

//~ NOTE(long): Render Helper

function Rect_f32 Long_Render_SetClip(Application_Links* app, Rect_f32 new_clip);
function void Long_Render_Outline(Application_Links* app, Rect_f32 rect, f32 roundness, f32 thickness, ARGB_Color color);
function void Long_Render_Rect(Application_Links* app, Rect_f32 rect, f32 roundness, ARGB_Color color);
function void Long_Render_BorderRect(Application_Links* app, Rect_f32 rect, f32 thickness, f32 roundness,
                                     ARGB_Color background_color, ARGB_Color border_color);

//~ NOTE(long): Cursor Rendering

function f32 Long_Render_CursorThickness(Long_Render_Context* ctx);
function void  Long_Render_NotepadCursor(Long_Render_Context* ctx);
function void    Long_Render_EmacsCursor(Long_Render_Context* ctx);

//~ NOTE(long): Tooltip Rendering

global Fancy_Block long_fancy_tooltips;
function Fancy_Line* Long_Render_PushTooltip(Fancy_Line* line, String8 string, FColor color);
function void Long_Render_TooltipRect(Application_Links* app, Rect_f32 rect);

function Fancy_Block Long_Render_LayoutString(Long_Render_Context* ctx, Arena* arena, String8 string, f32 max_width);
function Fancy_Block Long_Render_LayoutLine(Long_Render_Context* ctx, Arena* arena, Fancy_Line* line);
function Vec2_f32 Long_Render_FancyBlock(Long_Render_Context* ctx, Fancy_Block* block, Vec2_f32 tooltip_pos);
function Vec2_f32 Long_Render_DrawString(Long_Render_Context* ctx, String8 string, Vec2_f32 tooltip_pos, ARGB_Color color);
function void Long_Render_DrawPeek(Application_Links* app, Rect_f32 rect, Buffer_ID buffer, Range_i64 range, i64 line_offset);

//~ NOTE(long): Brace Rendering

function i64 Long_Nest_DelimPos(Application_Links* app, Token_Array array, Find_Nest_Flag flags, i64 pos);
#define Long_Render_DrawEnclosures(ctx, pos, flags, colors) \
    draw_enclosures((ctx)->app, (ctx)->layout, (ctx)->buffer, (pos), (flags), \
                    RangeHighlightKind_CharacterHighlight, 0, 0, (colors).vals, (colors).count);

function void Long_Render_BraceAnnotation(Long_Render_Context* ctx, i64 pos, Color_Array colors);
function void Long_Render_BraceLines     (Long_Render_Context* ctx, i64 pos, Color_Array colors);

//~ NOTE(long): Render Hook

function void Long_Render_CalcComments(Long_Render_Context* ctx);
function void Long_Render_DivComments(Long_Render_Context* ctx);
function Rect_f32 Long_Render_LineNumber(Long_Render_Context* ctx);
function void Long_Render_HexColor(Long_Render_Context* ctx);
function void Long_Render_FPS(Long_Render_Context* ctx);

function void Long_Render_FileBar(Long_Render_Context* ctx);
function void Long_Render_Buffer(Long_Render_Context* ctx);

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
CUSTOM_ID(attachment, long_buffer_face);

function Long_Highlight_List* Long_Highlight_GetList(Application_Links* app, View_ID view);
function void      Long_Highlight_Push(Application_Links* app, View_ID view, Range_i64 range);
function void     Long_Highlight_Clear(Application_Links* app, View_ID view);
function void Long_Highlight_ClearList(Long_Highlight_List* list);

function void Long_Highlight_CursorMark(Application_Links* app, f32 x_pos, f32 width);
function void Long_Highlight_DrawErrors(Long_Render_Context* ctx, Buffer_ID jump_buffer);
function void   Long_Highlight_DrawList(Long_Render_Context* ctx);
function void    Long_MC_DrawHighlights(Long_Render_Context* ctx);
function b32   Long_Highlight_DrawRange(Long_Render_Context* ctx);
function b32    Long_Highlight_HasRange(Application_Links* app, View_ID view);

function void     Long_Render_FadeError(Application_Links* app, Buffer_ID buffer, Range_i64 range);
function void Long_Render_FadeHighlight(Application_Links* app, Buffer_ID buffer, Range_i64 range);
function void    Long_Highlight_Comment(Long_Render_Context* ctx, Comment_Highlight_Pair* pairs, i32 pair_count);
function void Long_Highlight_Whitespace(Long_Render_Context* ctx);

//~ NOTE(long): Colors

//- NOTE(long): F4 Forward Declaration
struct ColorCtx;
function ColorCtx ColorCtx_Token(Token token, Buffer_ID buffer);
function b32 F4_ARGBIsValid(ARGB_Color color);

//- NOTE(long): Actual Functions
function ARGB_Color Long_ARGBFromID(Managed_ID id, i32 subindex = 0);
function ARGB_Color Long_Color_Alpha(ARGB_Color color, f32 alpha);
function ARGB_Color Long_Color_Cursor();

#define Long_Token_ColorTable(X) \
    X(EOF, defcolor_text_default) \
    X(Whitespace, defcolor_text_default) \
    X(LexError, defcolor_text_default) \
    X(Comment, defcolor_comment) \
    X(Keyword, defcolor_keyword) \
    X(Preprocessor, defcolor_preproc) \
    X(Identifier, defcolor_text_default) \
    X(Operator, fleury_color_operators) \
    X(LiteralInteger, defcolor_int_constant) \
    X(LiteralFloat, defcolor_float_constant) \
    X(LiteralString, defcolor_str_constant) \
    X(ScopeOpen, fleury_color_syntax_crap) \
    X(ScopeClose, fleury_color_syntax_crap) \
    X(ParentheticalOpen, fleury_color_syntax_crap) \
    X(ParentheticalClose, fleury_color_syntax_crap) \
    X(StatementClose, fleury_color_syntax_crap) \

function void Long_Syntax_Highlight(Long_Render_Context* ctx);

#endif //FCODER_LONG_RENDER_H

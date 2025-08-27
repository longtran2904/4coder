
//~ NOTE(long): Render Context

function Long_Render_Context Long_Render_CreateCtx(Application_Links* app, Frame_Info frame, View_ID view, b32 global_face)
{
    Long_Render_Context ctx = {};
    ctx.app = app;
    ctx.frame = frame;
    ctx.mouse = V2f32(get_mouse_state(app).p);
    ctx.buffer = view_get_buffer(app, view, 0);
    
    ctx.rect = view_get_screen_rect(app, view);
    ctx.view = view;
    ctx.is_active_view = get_active_view(app, 0) == view;
    
    Long_Render_SetCtxFace(&ctx, get_face_id(app, !global_face * ctx.buffer));
    return ctx;
}

function Text_Layout_ID Long_Render_InitCtxBuffer(Long_Render_Context* ctx, Buffer_ID buffer,
                                                  Rect_f32 layout_rect, Buffer_Point point)
{
    Application_Links* app = ctx->app;
    ctx->buffer = buffer;
    ctx->array = get_token_array_from_buffer(app, buffer);
    ctx->layout = text_layout_create(app, buffer, layout_rect, point);
    ctx->visible_range = text_layout_get_visible_range(app, ctx->layout);
    return ctx->layout;
}

function void Long_Render_SetCtxFace(Long_Render_Context* ctx, Face_ID face)
{
    ctx->face = face;
    ctx->metrics = get_face_metrics(ctx->app, face);
}

//~ NOTE(long): Render Helpers

function Rect_f32 Long_Render_SetClip(Application_Links* app, Rect_f32 new_clip)
{
    new_clip.x0 = f32_floor32(new_clip.x0);
    new_clip.y0 = f32_floor32(new_clip.y0);
    new_clip.x1 =  f32_ceil32(new_clip.x1);
    new_clip.y1 =  f32_ceil32(new_clip.y1);
    return draw_set_clip(app, new_clip);
}

function void Long_Render_Outline(Application_Links* app, Rect_f32 rect, f32 roundness, f32 thickness, ARGB_Color color)
{
    f32 real_roundness = roundness * .01f * thickness * 2;
    f32 real_thickness = thickness + 1; // NOTE(long): There seems to be a bug in draw_rectangle_outline
    draw_rectangle_outline(app, rect, real_roundness, thickness + 1, color);
}

function void Long_Render_Rect(Application_Links* app, Rect_f32 rect, f32 roundness, ARGB_Color color)
{
    f32 rect_roundness = roundness * .01f;
    rect_roundness *= Min(rect.x1 - rect.x0, rect.y1 - rect.y0);
    draw_rectangle(app, rect, rect_roundness, color);
}

function void Long_Render_CursorRect(Application_Links* app, Rect_f32 rect, ARGB_Color color)
{
    f32 roundness = def_get_config_f32_lit(app, "cursor_roundness");
    Long_Render_Rect(app, rect, roundness, color);
}

function void Long_Render_BorderRect(Application_Links* app, Rect_f32 rect, f32 thickness, f32 roundness,
                                     ARGB_Color background_color, ARGB_Color border_color)
{
    f32 rect_roundness = roundness * .01f * thickness * 2; // @COPYPASTA(long): Long_Render_Outline
    draw_rectangle(app, rect, rect_roundness, background_color);
    Long_Render_Outline(app, rect, roundness, thickness, border_color);
}

function void Long_Render_RoundedRect(Application_Links* app, Rect_f32 rect, f32 thickness, f32 roundness,
                                      ARGB_Color background_color, ARGB_Color border_color)
{
    // @COPYPASTA(long): Long_Render_Rect
    f32 outline_roundness = roundness * .01f;
    outline_roundness *= Min(rect.x1 - rect.x0, rect.y1 - rect.y0);
    
    Long_Render_Rect(app, rect, roundness, background_color);
    draw_rectangle_outline(app, rect, outline_roundness, thickness + 1, border_color);
}

function void Long_Render_DrawBlock(Long_Render_Context* ctx, Range_i64 range, ARGB_Color color)
{
    f32 block_height = 0;
    for (i64 i = range.first; i < range.one_past_last; ++i, ++range.first)
    {
        Rect_f32 rect = text_layout_character_on_screen(ctx->app, ctx->layout, i);
        if (!Long_Rf32_Invalid(rect))
        {
            block_height = rect_height(rect);
            break;
        }
    }
    
    f32 roundness = def_get_config_f32_lit(ctx->app, "cursor_roundness")/100.f;
    roundness *= block_height;
    draw_character_block(ctx->app, ctx->layout, range, roundness, color);
}

function void Long_Render_HighlightBlock(Long_Render_Context* ctx, Range_i64 range, f32 alpha = 0.f)
{
    ARGB_Color back_color = Long_ARGBFromID(defcolor_highlight);
    ARGB_Color text_color = Long_ARGBFromID(defcolor_at_highlight);
    if (alpha > 0)
    {
        back_color = Long_Color_Alpha(back_color, alpha);
        text_color = Long_Color_Alpha(text_color, alpha);
    }
    
    Long_Render_DrawBlock(ctx, range, back_color);
    paint_text_color(ctx->app, ctx->layout, range, text_color);
}

//~ NOTE(long): Cursor Rendering

// @COPYPASTA(long): DoTheCursorInterpolation
function void Long_Render_CursorInterpolation(Long_Render_Context* ctx, Rect_f32* rect, Rect_f32* last_rect, Rect_f32 target)
{
    Rect_f32 _rect;
    if (!last_rect)
        last_rect = &_rect;
    *last_rect = *rect;
    
    f32 x_change = target.x0 - rect->x0;
    f32 y_change = target.y0 - rect->y0;
    
    f32 cursor_size_x = (target.x1 - target.x0);
    f32 cursor_size_y = (target.y1 - target.y0) * (1 + fabsf(y_change) / 60.f);
    
    b32 should_animate_cursor = !def_get_config_b32_lit("f4_disable_cursor_trails");
    if (should_animate_cursor)
    {
        if (fabs(x_change) > 1.f || fabs(y_change) > 1.f)
            animate_in_n_milliseconds(ctx->app, 0);
    }
    else
    {
        *rect = *last_rect = target;
        cursor_size_y = target.y1 - target.y0;
    }
    
    if (should_animate_cursor)
    {
        rect->x0 += (x_change) * ctx->frame.animation_dt * 30.f;
        rect->y0 += (y_change) * ctx->frame.animation_dt * 30.f;
        rect->x1 = rect->x0 + cursor_size_x;
        rect->y1 = rect->y0 + cursor_size_y;
    }
    
    if (target.y0 > last_rect->y0)
    {
        if (rect->y0 < last_rect->y0)
            rect->y0 = last_rect->y0;
    }
    else
    {
        if (rect->y1 > last_rect->y1)
            rect->y1 = last_rect->y1;
    }
}

function f32 Long_Render_CursorThickness(Long_Render_Context* ctx)
{
    f32 bracket_width = 0.5f * ctx->metrics.text_height;
    f32 thickness = def_get_config_f32_lit(ctx->app, "mark_thickness") * .01f;
    thickness *= bracket_width;
    return f32_round32(thickness);
}

// @COPYPASTA(long): F4_Cursor_RenderNotepadStyle
function Rect_f32 Long_Render_NotepadSymbol(Long_Render_Context* ctx, i64 cursor, i64 mark, ARGB_Color color)
{
    Application_Links* app = ctx->app;
    if (cursor != mark)
        Long_Render_HighlightBlock(ctx, Ii64(cursor, mark));
    
    // NOTE(rjf): Draw cursor
    Rect_f32 rect = text_layout_character_on_screen(app, ctx->layout, cursor);
    f32 width = Long_Render_CursorThickness(ctx);
    rect.x1 = rect.x0 + width;
    
    Rect_f32 view_rect = ctx->rect;
    if (rect.x0 < view_rect.x0)
    {
        rect.x0 = view_rect.x0;
        rect.x1 = view_rect.x0 + width;
    }
    
    Long_Render_CursorRect(app, rect, color);
    return rect;
}

function void Long_Render_NotepadCursor(Long_Render_Context* ctx)
{
    Application_Links* app = ctx->app;
    Text_Layout_ID layout = ctx->layout;
    Buffer_ID buffer = ctx->buffer;
    View_ID view = ctx->view;
    
    b32 has_highlight_range = Long_Highlight_HasRange(app, view);
    if (!has_highlight_range)
    {
        ARGB_Color cursor_color = Long_Color_Cursor();
        ARGB_Color  ghost_color = Long_Color_Alpha(cursor_color, 0.5f);
        
        i64 cursor_pos = view_get_cursor_pos(app, view);
        i64 mark_pos = view_get_mark_pos(app, view);
        Rect_f32 rect = Long_Render_NotepadSymbol(ctx, cursor_pos, mark_pos, cursor_color);
        
        if (ctx->is_active_view)
            Long_Render_CursorInterpolation(ctx, &global_cursor_rect, &global_last_cursor_rect, rect);
        Long_Render_CursorRect(app, global_cursor_rect, ghost_color);
        
        if (view == mc_context.view)
        {
            for_mc(node, mc_context.cursors)
            {
                if (range_contains(ctx->visible_range, node->cursor_pos) || range_contains(ctx->visible_range, node->mark_pos))
                {
                    Rect_f32 target_rect = text_layout_character_on_screen(app, layout, node->cursor_pos);
                    node->nxt_position = target_rect.p0;
                    
                    // TODO(long): cursor interpolation
                    Long_Render_NotepadSymbol(ctx, node->cursor_pos, node->mark_pos, cursor_color);
                }
            }
        }
    }
}

// @COYPASTA(long): C4_RenderCursorSymbolThingy
function void Long_Render_EmacsSymbol(Long_Render_Context* ctx, Rect_f32 rect, ARGB_Color color, b32 cursor_open)
{
    // NOTE(long): We don't use height/bracket_width because interpolation changes their size
    f32 width = Long_Render_CursorThickness(ctx);
    f32 height = rect.y1 - rect.y0;
    f32 bracket_width = 0.5f * height;
    
    Vec2_f32 pos = rect.p0;
    Rect_f32 vert_rect = Rf32_xy_wh(pos.x, pos.y, width, height);
    Rect_f32 horz_rect = {};
    if (cursor_open)
        horz_rect = Rf32_xy_wh(pos.x, pos.y, bracket_width, width);
    else
    {
        horz_rect.p1 = pos + V2f32(width, height);
        horz_rect.p0 = horz_rect.p1 - V2f32(bracket_width, width);
    }
    
    Long_Render_CursorRect(ctx->app, vert_rect, color);
    Long_Render_CursorRect(ctx->app, horz_rect, color);
}

// @COPYPASTA(long): F4_Cursor_RenderEmacsStyle
function void Long_Render_EmacsCursor(Long_Render_Context* ctx)
{
    Application_Links* app = ctx->app;
    Text_Layout_ID layout = ctx->layout;
    View_ID view = ctx->view;
    
    b32 has_highlight_range = Long_Highlight_HasRange(app, view);
    if (!has_highlight_range)
    {
        ARGB_Color cursor_color = Long_Color_Cursor();
        if (!ctx->is_active_view)
        {
            ARGB_Color inactive_cursor_color = Long_ARGBFromID(fleury_color_cursor_inactive, 0);
            if (F4_ARGBIsValid(inactive_cursor_color))
                cursor_color = inactive_cursor_color;
        }
        ARGB_Color mark_color = cursor_color;
        
        i64 cursor_pos = view_get_cursor_pos(app, view);
        i64 mark_pos = view_get_mark_pos(app, view);
        b32 cursor_is_open = cursor_pos <= mark_pos;
        Rect_f32  target_cursor = text_layout_character_on_screen(app, layout, cursor_pos);
        Rect_f32    target_mark = text_layout_character_on_screen(app, layout, mark_pos);
        Range_i64 visible_range = ctx->visible_range;
        
        // NOTE(rjf): Draw cursor.
        if (ctx->is_active_view)
        {
            Rect_f32 view_rect = view_get_screen_rect(app, view);
            
            if (cursor_pos < visible_range.start || cursor_pos > visible_range.end)
            {
                f32 width = target_cursor.x1 - target_cursor.x0;
                target_cursor.x0 = view_rect.x0;
                target_cursor.x1 = target_cursor.x0 + width;
            }
            
            Long_Render_CursorInterpolation(ctx, &global_cursor_rect, &global_last_cursor_rect, target_cursor);
            
            if (mark_pos > visible_range.end)
            {
                target_mark.x0 = 0;
                target_mark.y0 = view_rect.y1;
                target_mark.y1 = view_rect.y1;
            }
            
            if (mark_pos < visible_range.start || mark_pos > visible_range.end)
            {
                f32 width = target_mark.x1 - target_mark.x0;
                target_mark.x0 = view_rect.x0;
                target_mark.x1 = target_mark.x0 + width;
            }
            
            Long_Render_CursorInterpolation(ctx, &global_mark_rect, &global_last_mark_rect, target_mark);
        }
        
        // NOTE(rjf): Draw main cursor.
        Long_Render_EmacsSymbol(ctx, global_cursor_rect, cursor_color, cursor_is_open);
        Long_Render_EmacsSymbol(ctx,      target_cursor, cursor_color, cursor_is_open);
        
        // NOTE(rjf): GLOW IT UP
        for (i32 glow = 0; glow < 20; ++glow)
        {
            f32 alpha = .1f - glow*.015f;
            ARGB_Color glow_color = Long_Color_Alpha(cursor_color, alpha);
            if (alpha > 0)
            {
                Rect_f32 glow_rect = rect_inner(target_cursor, -(f32)glow);
                // TODO(long): roundness + glow*.7f
                Long_Render_EmacsSymbol(ctx, glow_rect, glow_color, cursor_is_open);
            }
            else break;
        }
        
        Long_Render_EmacsSymbol(ctx, global_mark_rect, Long_Color_Alpha(mark_color, 0.50f), !cursor_is_open);
        Long_Render_EmacsSymbol(ctx,      target_mark, Long_Color_Alpha(mark_color, 0.75f), !cursor_is_open);
        
        if (view == mc_context.view)
        {
            ARGB_Color        mc_color = Long_ARGBFromID(defcolor_cursor, 1);;
            ARGB_Color mc_cursor_color = Long_Color_Alpha(mc_color, .75f);
            ARGB_Color   mc_mark_color = Long_Color_Alpha(mc_color, .50f);
            
            for_mc(node, mc_context.cursors)
            {
                if (range_contains(visible_range, node->cursor_pos))
                {
                    Rect_f32 target_rect = text_layout_character_on_screen(app, layout, node->cursor_pos);
                    node->nxt_position = target_rect.p0;
                    // TODO(long): cursor interpolation
                    Long_Render_EmacsSymbol(ctx, target_rect, mc_cursor_color, cursor_is_open);
                }
                
                if (range_contains(visible_range, node->mark_pos))
                {
                    Rect_f32 target_rect = text_layout_character_on_screen(app, layout, node->mark_pos);
                    Long_Render_EmacsSymbol(ctx, target_rect, mc_mark_color, !cursor_is_open);
                }
            }
        }
    }
}

// @COPYPASTA(long): F4_HighlightCursorMarkRange
function void Long_Highlight_CursorMark(Application_Links* app, f32 x_pos, f32 width)
{
    Rect_f32 draw_rect = {};
    draw_rect.x0 = x_pos;
    draw_rect.x1 = draw_rect.x0 + width;
    draw_rect.y0 = Min(global_last_cursor_rect.y1, global_last_mark_rect.y1);
    draw_rect.y1 = Max(global_last_cursor_rect.y0, global_last_mark_rect.y0);
    
    //if (abs_f32(global_last_cursor_rect.y0 - global_last_mark_rect.y0) > 1)
    if (draw_rect.y1 - draw_rect.y0 >= rect_height(global_last_cursor_rect))
    {
        ARGB_Color color = Long_Color_Alpha(Long_ARGBFromID(defcolor_comment), 0.5f);
        Long_Render_CursorRect(app, draw_rect, color);
    }
}

//~ NOTE(long): Tooltip Rendering

function Fancy_Line* Long_Render_PushTooltip(Fancy_Line* line, String8 string, FColor color)
{
    String8 string_copy = push_string_copy(&global_frame_arena, string);
    if (!line)
        line = push_fancy_line(&global_frame_arena, &long_fancy_tooltips);
    push_fancy_string(&global_frame_arena, line, color, string_copy);
    return line;
}

// @COPYPASTA(long): F4_DrawTooltipRect
function void Long_Render_TooltipRect(Application_Links* app, Rect_f32 rect)
{
    ARGB_Color   back_color = Long_Color_Alpha(Long_ARGBFromID(defcolor_back), .8125f);
    ARGB_Color border_color = Long_Color_Alpha(Long_ARGBFromID(defcolor_margin_active), .8125f);
    f32 thickness = def_get_config_f32_lit(app, "tooltip_thickness");
    f32 roundness = def_get_config_f32_lit(app, "tooltip_roundness");
    Long_Render_BorderRect(app, rect, thickness, roundness, back_color, border_color);
}

function void Long_Render_DrawPeek(Application_Links* app, Rect_f32 rect, Buffer_ID buffer, Range_i64 range, i64 line_offset)
{
    if (!buffer_exists(app, buffer))
        return;
    
    i64 line = get_line_number_from_pos(app, buffer, range.min);
    i64 draw_line = line - Min(line_offset, line - 1);
    while (draw_line < line && line_is_blank(app, buffer, draw_line))
        draw_line++;
    
    i64 end_pos = get_line_end_pos(app, buffer, line);
    range.min = get_line_start_pos(app, buffer, draw_line);
    range.max = clamp_bot(range.max, end_pos);
    
    Face_Metrics metrics = get_face_metrics(app, get_face_id(app, buffer));
    rect = rect_inner(rect, metrics.line_height); 
    rect = Long_Rf32_Round(rect);
    
    Long_Render_Context ctx = {app};
    Text_Layout_ID layout = Long_Render_InitCtxBuffer(&ctx, buffer, rect, {draw_line});
    if (ctx.array.tokens)
        Long_Syntax_Highlight(&ctx);
    else
        paint_text_color(app, layout, range, Long_ARGBFromID(defcolor_text_default));
    
    draw_line_highlight(app, layout, line, Long_ARGBFromID(defcolor_highlight_white));
    draw_text_layout_default(app, layout);
    text_layout_free(app, layout);
}

function Vec2_f32 Long_Render_FancyBlock(Long_Render_Context* ctx, Fancy_Block* block, Vec2_f32 tooltip_pos)
{
    Application_Links* app = ctx->app;
    Scratch_Block scratch(app);
    Rect_f32 region = ctx->rect;
    
    Vec2_f32 padded_size = Long_V2f32(Long_Render_TooltipOffset(ctx, normal_advance/2.f));
    Vec2_f32 needed_size = get_fancy_block_dim(app, ctx->face, block);
    Rect_f32 tooltip_rect = Rf32_xy_wh(tooltip_pos, needed_size + 2*padded_size);
    
#if 1
    f32 offset = clamp_bot(tooltip_rect.x1 - region.x1, 0);
    tooltip_rect.x0 -= offset;
    tooltip_rect.x1 -= offset;
#else
    Vec2_f32 offset = { clamp_bot(tooltip_rect.x1 - region.x1, 0), clamp_bot(tooltip_rect.y1 - region.y1, 0) };
    tooltip_rect.p0 -= offset;
    tooltip_rect.p1 -= offset;
#endif
    
    Long_Render_TooltipRect(app, tooltip_rect);
    draw_fancy_block(app, ctx->face, fcolor_zero(), block, tooltip_rect.p0 + padded_size);
    return tooltip_rect.p1;
}

function Fancy_Block Long_Render_LayoutString(Long_Render_Context* ctx, Arena* arena, String8 string, f32 max_width)
{
    Application_Links* app = ctx->app;
    Scratch_Block scratch(app, arena);
    Fancy_Block result = {};
    
    String8 ws = S8Lit(" \n\r\t\f\v");
    String8List list = string_split(scratch, string, ws.str, (i32)ws.size);
    Fancy_Line* line = push_fancy_line(arena, &result);
    
    for (String8Node* node = list.first; node; node = node->next)
    {
        String8 str = node->string;
        b32 was_newline = node == list.first || str.str[-1] == '\n';
        
        b32 overflow = 0;
        if (!was_newline)
        {
            f32 advance = get_string_advance(app, ctx->face, str);
            f32 width = get_fancy_line_width(app, ctx->face, line);
            Assert(width);
            overflow = (advance + width + ctx->metrics.space_advance) > max_width;
        }
        
        if (overflow)
            line = push_fancy_line(arena, &result);
        else if (!was_newline)
            push_fancy_string(arena, line, S8Lit(" "));
        push_fancy_string(arena, line, str);
    }
    return result;
}

function Fancy_Block Long_Render_LayoutLine(Long_Render_Context* ctx, Arena* arena, Fancy_Line* line)
{
    Fancy_Block result = {};
    Application_Links* app = ctx->app;
    f32 max_width = rect_width(ctx->rect);
    
    Scratch_Block scratch(app, arena);
    String8 ws = S8Lit(" \n\r\t\f\v");
    Fancy_Line* block_line = push_fancy_line(arena, &result);
    
    for (Fancy_String* fancy_str = line->first,* prev_str = 0; fancy_str; prev_str = fancy_str, fancy_str = fancy_str->next)
    {
        // @COPYPASTA(long): Long_Render_LayoutString
        String8List list = string_split(scratch, fancy_str->value, ws.str, (i32)ws.size);
        for (String8Node* node = list.first; node; node = node->next)
        {
            String8 str = node->string;
            b32 push_space = node != list.first || fancy_str != line->first;
            b32 was_newline = push_space && str.str[-1] == '\n';
            
            f32 advance = get_string_advance(app, ctx->face, str);
            f32 width = get_fancy_line_width(app, ctx->face, block_line);
            b32 overflow = (advance + width + ctx->metrics.space_advance) > max_width;
            
            if (was_newline || overflow)
                block_line = push_fancy_line(arena, &result);
            else if (push_space)
                push_fancy_string(arena, block_line, fancy_str->fore, S8Lit(" "));
            push_fancy_string(arena, block_line, fancy_str->fore, str);
        }
    }
    
    return result;
}

function Vec2_f32 Long_Render_DrawString(Long_Render_Context* ctx, String8 string, Vec2_f32 tooltip_pos, ARGB_Color color)
{
    Application_Links* app = ctx->app;
    Scratch_Block scratch(app);
    Fancy_Block block = Long_Render_LayoutString(ctx, scratch, string, rect_width(ctx->rect));
    for (Fancy_Line* line = block.first; line; line = line->next)
        line->fore = fcolor_argb(color);
    return Long_Render_FancyBlock(ctx, &block, tooltip_pos);
}

//~ NOTE(long): Render Hook

#ifdef FCODER_FLEURY_CALC_H
// @COPYPASTA(long): F4_CLC_RenderComments
function void Long_Render_CalcComments(Long_Render_Context* ctx)
{
    Application_Links* app = ctx->app;
    ProfileScope(app, "[Long] Calc Comments");
    if (ctx->array.tokens)
    {
        Token_Iterator_Array it = token_iterator_pos(0, &ctx->array, ctx->visible_range.start);
        
        for(;;)
        {
            Scratch_Block scratch(app);
            Token* token = token_it_read(&it);
            if (!token || token->pos >= ctx->visible_range.end || !token_it_inc_non_whitespace(&it))
                break;
            
            if (token->kind == TokenBaseKind_Comment && token->size >= 5)
            {
                i64 token_size = token->size > 1024 ? 1024 : token->size;
                u8* token_buffer = push_array(scratch, u8, token_size + 1);
                buffer_read_range(app, ctx->buffer, Ii64_size(token->pos, token_size), token_buffer);
                token_buffer[token_size] = 0;
                
                // NOTE(long): This is language-specific. It expects comments to be delimited by 2 characters.
                // Multi-line comments are ended by flipping the start delimiter.
                if (token_buffer[2] == 'c' && character_is_whitespace(token_buffer[3]))
                {
                    if (token_buffer[1] == token_buffer[token_size-2] && token_buffer[0] == token_buffer[token_size-1])
                        token_buffer[token_size-2] = 0;
                    
                    F4_CLC_RenderCode(app, ctx->buffer, ctx->view, ctx->layout, ctx->frame, scratch, (char*)token_buffer + 3, token->pos + 3);
                }
            }
        }
    }
}
#endif

// @COPYPASTA(long): F4_RenderDividerComments
function void Long_Render_DivComments(Long_Render_Context* ctx)
{
    Application_Links* app = ctx->app;
    Text_Layout_ID layout = ctx->layout;
    
    ProfileScope(app, "[Long] Divider Comments");
    Token_Iterator_Array it = token_iterator_pos(0, &ctx->array, ctx->visible_range.first);
    Rect_f32 view_rect = text_layout_region(app, layout);
    
    for(;;)
    {
        Scratch_Block scratch(app);
        Token* token = token_it_read(&it);
        
        if (!token || token->pos >= ctx->visible_range.one_past_last || !token_it_inc_non_whitespace(&it))
            break;
        
        if (token->kind == TokenBaseKind_Comment)
        {
            Rect_f32 comment_first_char_rect = text_layout_character_on_screen(app, layout, token->pos);
            Rect_f32 comment_last_char_rect = text_layout_character_on_screen(app, layout, token->pos+token->size-1);
            String8 token_string = push_buffer_range(app, scratch, ctx->buffer, Ii64(token));
            String8 signifier_substring = string_substring(token_string, Ii64(0, 3));
            f32 roundness = 4.f;
            
            // NOTE(rjf): Strong dividers.
            if (string_match(signifier_substring, S8Lit("//~")))
            {
                Rect_f32 rect =
                {
                    comment_first_char_rect.x0, comment_first_char_rect.y0-2,
                    10000                     , comment_first_char_rect.y0,
                };
                draw_rectangle(app, rect, roundness, Long_ARGBFromID(defcolor_comment));
            }
            
            // NOTE(rjf): Weak dividers.
            else if (string_match(signifier_substring, S8Lit("//-")))
            {
                f32 dash_size = 8;
                Rect_f32 rect =
                {
                    comment_last_char_rect.x1 +         0, (comment_last_char_rect.y0 + comment_last_char_rect.y1)/2 - 1,
                    comment_last_char_rect.x1 + dash_size, (comment_last_char_rect.y0 + comment_last_char_rect.y1)/2 + 1,
                };
                
                for (i32 i = 0; i < 1000 && rect.x1 <= view_rect.x1; i += 1)
                {
                    draw_rectangle(app, rect, roundness, Long_ARGBFromID(defcolor_comment));
                    rect.x0 += dash_size*1.5f;
                    rect.x1 += dash_size*1.5f;
                }
            }
        }
    }
}

function void Long_Render_Underline(Long_Render_Context* ctx, Range_i64 range, ARGB_Color color)
{
    Application_Links* app = ctx->app;
    Rect_f32 range_start_rect = text_layout_character_on_screen(app, ctx->layout, range.start);
    Rect_f32   range_end_rect = text_layout_character_on_screen(app, ctx->layout, range.end-1);
    Rect_f32 total_range_rect = rect_union(range_start_rect, range_end_rect);
    
    f32 thickness = Long_Render_CursorThickness(ctx);
    total_range_rect.y0 = total_range_rect.y1 + 0.f;
    total_range_rect.y1 = total_range_rect.y0 + thickness;
    Long_Render_CursorRect(app, total_range_rect, color);
}

function Rect_f32 Long_Render_LineNumber(Long_Render_Context* ctx)
{
    Application_Links* app = ctx->app;
    Buffer_ID buffer = ctx->buffer;
    View_ID view = ctx->view;
    FColor line_color = fcolor_id(defcolor_line_numbers_text, 0);
    FColor curr_color = fcolor_id(defcolor_line_numbers_text, 1);
    b32 draw_line_offset = def_get_config_b32_lit("long_show_line_number_offset");
    
    ProfileScope(app, "[Long] Render Line Number Margin");
    Rect_f32 layout_rect, margin;
    u64 digit_count = 0;
    {
        i64 line_count = buffer_get_line_count(app, buffer);
        if (draw_line_offset)
        {
            i64 visible_line_count = i32_ceil32(rect_height(ctx->rect)/ctx->metrics.line_height) + 1;
            visible_line_count = clamp_top(visible_line_count, line_count-1);
            digit_count = digit_count_from_integer(visible_line_count, 10);
            digit_count++; // For the +/- unary prefix
        }
        else
            digit_count = digit_count_from_integer(line_count, 10);
        
        f32 digit_advance = ctx->metrics.decimal_digit_advance;
        f32 width = (f32)digit_count*digit_advance + digit_advance*2.f;
        Rect_f32_Pair pair = rect_split_left_right(ctx->rect, width);
        margin = pair.min;
        layout_rect = pair.max;
    }
    
    // NOTE(long): This layout_rect is slightly bigger than the one in Long_Render along the x-axis
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
    Text_Layout_ID layout = text_layout_create(app, buffer, layout_rect, scroll.position);
    Range_i64 visible_range = text_layout_get_visible_range(app, layout);
    draw_rectangle(app, margin, 0.f, Long_ARGBFromID(defcolor_line_numbers_back));
    
    Range_i64 line_range = get_line_range_from_pos_range(app, buffer, visible_range);
    line_range.max++;
    
    i64 current_line = get_line_number_from_pos(app, buffer, view_get_cursor_pos(app, view));
    for (i64 line_number = line_range.min; line_number < line_range.max; ++line_number)
    {
        Scratch_Block scratch(app);
        b32 is_current = line_number == current_line;
        
        Fancy_String fstring = {};
        if (!draw_line_offset)
            fstring.value = push_stringf(scratch,  " %*lld", digit_count, line_number);
        else if (is_current)
            fstring.value = push_stringf(scratch,  " %*lld", digit_count, 0);
        else
            fstring.value = push_stringf(scratch, " %+*lld", digit_count, line_number - current_line);
        
        Range_f32 line_y = text_layout_line_on_screen(app, layout, line_number);
        Vec2_f32 p = V2f32(margin.x0, line_y.min);
        draw_fancy_string(app, ctx->face, is_current ? curr_color : line_color, &fstring, p);
    }
    
    text_layout_free(app, layout);
    return layout_rect;
}

CUSTOM_COMMAND_SIG(long_toggle_line_offset)
CUSTOM_DOC("Toggles between line numbers and offsets.")
{
    def_toggle_config_b32_lit("long_show_line_number_offset");
}

// @COPYPASTA(long): qol_draw_hex_color
function void Long_Render_HexColor(Long_Render_Context* ctx)
{
    Application_Links* app = ctx->app;
    Text_Layout_ID layout = ctx->layout;
    Range_i64 visible_range = ctx->visible_range;
    
    Scratch_Block scratch(app);
    String8 string = push_buffer_range(app, scratch, ctx->buffer, visible_range);
    i64 digits = 8, prefix = 2;
    i64 width = digits + prefix;
    i64 size = range_size(visible_range) - width + 1;
    
    for (i64 i = 0; i < size; ++i)
    {
        HEX_SCAN:
        u8* str = string.str+i;
        
        b32 b0 = i > 0      ? character_is_alpha_numeric(str[-1]) : 0;
        b32 b1 = i < size-1 ? character_is_alpha_numeric(str[prefix+digits]) : 0;
        b32 b2 = str[0] != '0';
        b32 b3 = str[1] != 'x';
        if (b0 || b1 || b2 || b3)
            continue;
        
        for (i64 j = 0; j < digits; ++j)
        {
            u8 c = str[j+prefix];
            if (!(('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F')))
            {
                i += j+1;
                goto HEX_SCAN;
            }
        }
        
        // @CONSTANT
        f32 threshold = 110.f;
        f32 thickness = .1f;
        f32 roundness = def_get_config_f32_lit(app, "cursor_roundness");
        
        ARGB_Color color = (ARGB_Color)string_to_integer(SCu8(str+prefix, digits), 16);
        f32 avg = (((color >> 16) & 0xFF) + ((color >> 8) & 0xFF) + (color & 0xFF))/3.f;
        ARGB_Color contrast = 0xFF000000 | (i32(avg > threshold)-1);
        
        Range_i64 range = Ii64_size(visible_range.min+i, width);
        Rect_f32  left = text_layout_character_on_screen(app, layout, range.min);
        Rect_f32 right = text_layout_character_on_screen(app, layout, range.max-1);
        Rect_f32  rect = rect_union(left, right);
        
        thickness *= rect_height(rect);
        rect = rect_inner(rect, -thickness);
        paint_text_color(app, layout, range, contrast);
        Long_Render_RoundedRect(app, rect, thickness, roundness, color, contrast);
        
        // The loop will automatically increase by an additional one, making us skip an extra byte
        // That is good because we shouldn't render 2 hex numbers right next to each other
        i += width;
    }
}

// @COPYPASTA(long): draw_fps_hud
function void Long_Render_FPS(Long_Render_Context* ctx)
{
    Application_Links* app = ctx->app;
    Frame_Info frame = ctx->frame;
    
    f32 line_height = ctx->metrics.line_height;
    f32 height = line_height * fps_history_depth; // layout_fps_hud_on_bottom
    
    Vec2_f32 pos;
    {
        f32 padding = Long_Render_TooltipOffset(ctx, normal_advance);
        f32 width = get_string_advance(app, ctx->face, S8Lit("FPS: [12345]: ---------- | ---------- | 123.45"));
        
        Rect_f32 rect = global_get_screen_rectangle(app);
        f32 offset = line_height * 2;
        rect.x1 = rect.x1 - offset;
        rect.x0 = rect.x1 - width - padding * 2;
        rect.y0 = rect.y0 + offset;
        rect.y1 = rect.y0 + height + padding * 2;
        
        Long_Render_TooltipRect(app, rect);
        pos = rect.p0 + Long_V2f32(padding);
    }
    
    local_persist f32 history_literal_dt[fps_history_depth] = {};
    local_persist f32 history_animation_dt[fps_history_depth] = {};
    local_persist i32 history_frame_index[fps_history_depth] = {};
    
    i32 wrapped_index = frame.index%fps_history_depth;
    history_literal_dt[wrapped_index]   = frame.literal_dt;
    history_animation_dt[wrapped_index] = frame.animation_dt;
    history_frame_index[wrapped_index]  = frame.index;
    
    Range_i32 ranges[2] = {};
    ranges[0].first = wrapped_index;
    ranges[0].one_past_last = -1;
    ranges[1].first = fps_history_depth - 1;
    ranges[1].one_past_last = wrapped_index;
    
    for (i32 i = 0; i < ArrayCount(ranges); ++i)
    {
        Range_i32 r = ranges[i];
        for (i32 j = r.first; j > r.one_past_last; --j, pos.y += line_height)
        {
            Scratch_Block scratch(app);
            f32 dts[2] = { history_literal_dt[j], history_animation_dt[j] };
            
            Fancy_Line list = {};
            push_fancy_stringf(scratch, &list, f_pink , "FPS: ");
            push_fancy_stringf(scratch, &list, f_green, "[");
            push_fancy_stringf(scratch, &list, f_white, "%5d", history_frame_index[j]);
            push_fancy_stringf(scratch, &list, f_green, "]: ");
            
            for (i32 k = 0; k < ArrayCount(dts); k += 1)
            {
                f32 dt = dts[k];
                if (dt == 0.f)
                    push_fancy_stringf(scratch, &list, f_white, "----------");
                else
                    push_fancy_stringf(scratch, &list, f_white, "%10.6f", dt, 1.f / dt);
                push_fancy_stringf(scratch, &list, f_green, " | ");
            }
            
            push_fancy_stringf(scratch, &list, f_gray, "%6.2f", 1.f/dts[0]);
            draw_fancy_line(app, ctx->face, fcolor_zero(), &list, pos);
        }
    }
}

// @COPYPASTA(long): F4_RenderBuffer
function void Long_Render_Buffer(Long_Render_Context* ctx)
{
    Application_Links* app = ctx->app;
    Buffer_ID buffer = ctx->buffer;
    Rect_f32 rect = ctx->rect;
    View_ID view = ctx->view;
    
    Range_i64 visible_range = ctx->visible_range;
    Text_Layout_ID layout = ctx->layout;
    Token_Array array = ctx->array;
    b32 has_token_array = !!array.tokens;
    
    Rect_f32 layout_rect = text_layout_region(app, layout);
    Rect_f32 buffer_rect = view_get_buffer_region(app, view);
    Rect_f32 screen_rect = view_get_screen_rect(app, view);
    
    ProfileScope(app, "[Long] Render Buffer");
    Scratch_Block scratch(app);
    String8 buffer_name = push_buffer_base_name(app, scratch, buffer);
    b32 is_special = Long_Buffer_IsSpecial(app, buffer_name);
    
    // NOTE(long): When fading out an undo, the BufferEditRange hook will run before the Tick hook, making the note tree outdated.
    // Later, the RenderCaller hook will try to look up the color based on the old tree and crash the program.
    // 1. F4_Language_LexFullInput_NoBreaks (BufferEditRange) will modify the Token list and the buffer (buffer_mark_as_modified)
    // 2. F4_Render (HookdID_RenderCaller) uses the outdated note tree and crashes the program, so step 3 never runs
    // 3. F4_Index_Tick (F4_Tick/HookID_Tick) updates the note tree correctly and clears the global_buffer_modified_set
    // (F4_Tick -> default_tick -> code_index_update_tick -> buffer_modified_set_clear)
    // Calling the Tick function here will fix the problem.
    
    // This bug only happens when you undo/paste some text, not when you redo/delete/type the text.
    // The render hook always runs after the tick hook and global_buffer_modified_set is always empty in the latter case
    for (Buffer_Modified_Node* node = global_buffer_modified_set.first; node; node = node->next)
    {
        if (node->buffer == buffer)
        {
            Long_Index_Tick(app);
            code_index_update_tick(app);
            break;
        }
    }
    
    i64 cursor_pos = view_correct_cursor(app, view);
    view_correct_mark(app, view);
    
    //- NOTE(long): Order Of Execution
    // Scope Highlight: (Token_Array)
    //  Line Highlight: Jump <-> Error -> Current Line
    // Range Highlight: List -> Range -> MC
    // Brace Rendering: (Token_Array) Line -> Annotation
    // Token Rendering: (Token_Array) Dividier Comment -> Occurrence Highlight
    // Token  Coloring: (Token_Array) Syntax Highlight -> Comment <-> Hex <-> Brace <-> Paren -> Fade
    
    // Scope -> Line -> Range -> Brace -> Draw Text Layout (Token Coloring) -> Divider Comment -> Occurrence Highlight
    
    //- NOTE(long): Token Colorizing
    if (has_token_array)
    {
        Long_Syntax_Highlight(ctx);
        
        // NOTE(long): Comment Rendering
        if (def_get_config_b32_lit("use_comment_keywords"))
        {
            Comment_Highlight_Pair pairs[] =
            {
                { S8Lit("NOTE"), finalize_color(defcolor_comment_pop, 0) },
                { S8Lit("TODO"), finalize_color(defcolor_comment_pop, 1) },
                { def_get_config_str_lit(scratch, "user_name"), finalize_color(fleury_color_comment_user_name, 0) },
            };
            Long_Highlight_Comment(ctx, pairs, ArrayCount(pairs));
        }
    }
    else paint_text_color_fcolor(app, layout, visible_range, fcolor_id(defcolor_text_default));
    
    //- NOTE(long): Line Highlight
    // TODO(long): These functions should ignore the line number margin's rect
    {
        // NOTE(long): Scope Highlight
        if (has_token_array && def_get_config_b32_lit("use_scope_highlight"))
        {
            Color_Array scope_colors = finalize_color_array(defcolor_back_cycle);
            if (scope_colors.count >= 1 && F4_ARGBIsValid(scope_colors.vals[0]))
                draw_scope_highlight(app, buffer, layout, cursor_pos, scope_colors.vals, scope_colors.count);
        }
        
        Buffer_ID compilation_buffer = get_buffer_by_name(app, S8Lit("*compilation*"), Access_Always);
        
        // NOTE(allen): Search highlight
        if (def_get_config_b32_lit("use_jump_highlight"))
        {
            Buffer_ID jump_buffer = get_locked_jump_buffer(app);
            if (jump_buffer != compilation_buffer)
                draw_jump_highlights(app, buffer, layout, jump_buffer, fcolor_id(defcolor_highlight_white));
        }
        
        // NOTE(allen): Error highlight
        if (def_get_config_b32_lit("use_error_highlight"))
            Long_Highlight_DrawErrors(ctx, compilation_buffer);
        
        // NOTE(long): Current line highlight
        if (def_get_config_b32_lit("highlight_line_at_cursor") && (ctx->is_active_view || is_special))
        {
            // @COPYPASTA(long): draw_line_highlight
            i64 line = get_line_number_from_pos(app, buffer, cursor_pos);
            Range_f32 y = text_layout_line_on_screen(app, layout, line);
            
            if (range_size(y) > 0.f)
            {
                // TODO(long): Bypass fleury_color_inactive_pane_overlay when highlight inactive (special) buffer
                Rect_f32 highlight_rect = Rf32(rect_range_x(rect), y);
                draw_rectangle(app, highlight_rect, 0.f, Long_ARGBFromID(defcolor_highlight_cursor_line));
            }
        }
    }
    
    //- NOTE(long): Range Highlight
    {
        // NOTE(long): This function calls both paint_text_color and draw_rectangle
        // We want the former to be called after Long_Syntax_Highlight
        // While the latter must be called before Long_Highlight_XXX functions
        // Maybe part of this should be moved into Long_Syntax_Highlight
        Long_Render_HexColor(ctx);
        
        // TODO(long): Comprese these
        Long_Highlight_DrawList(ctx);
        Long_Highlight_DrawRange(ctx);
        Long_MC_DrawHighlights(ctx);
        
        b64 show_whitespace = 0;
        view_get_setting(app, view, ViewSetting_ShowWhitespace, &show_whitespace);
        if (show_whitespace || (!is_special && def_get_config_b32_lit("long_global_show_whitespace")))
            Long_Highlight_Whitespace(ctx);
    }
    
    //- NOTE(long): Brace Rendering
    if (has_token_array)
    {
        ARGB_Color comment_color = finalize_color(defcolor_comment, 0);
        i64 brace_pos = Long_Nest_DelimPos(app, array, FindNest_Scope, cursor_pos);
        
        if (!def_get_config_b32_lit("f4_disable_brace_lines"))
        {
            Color_Array line_colors = finalize_color_array(fleury_color_brace_line);
            if (!(line_colors.count >= 1 && F4_ARGBIsValid(line_colors.vals[0])))
                line_colors = { &comment_color, 1 };
            Long_Render_BraceLines(ctx, brace_pos, line_colors);
        }
        
        if (!def_get_config_b32_lit("f4_disable_close_brace_annotation"))
        {
            Color_Array annotation_colors = finalize_color_array(fleury_color_brace_annotation);
            if (!(annotation_colors.count >= 1 && F4_ARGBIsValid(annotation_colors.vals[0])))
                annotation_colors = { &comment_color, 1 };
            Long_Render_BraceAnnotation(ctx, brace_pos, annotation_colors);
        }
        
        if (!def_get_config_b32_lit("f4_disable_brace_highlight"))
        {
            Color_Array brace_color = finalize_color_array(fleury_color_brace_highlight);
            if (brace_color.count >= 1 && F4_ARGBIsValid(brace_color.vals[0]))
                Long_Render_DrawEnclosures(ctx, brace_pos, FindNest_Scope, brace_color);
        }
        
        if (def_get_config_b32_lit("use_paren_helper"))
        {
            Color_Array paren_colors = finalize_color_array(defcolor_text_cycle);
            if (paren_colors.count >= 1 && F4_ARGBIsValid(paren_colors.vals[0]))
            {
                i64 paren_pos = Long_Nest_DelimPos(app, array, FindNest_Paren, cursor_pos);
                Long_Render_DrawEnclosures(ctx, paren_pos, FindNest_Paren, paren_colors);
            }
        }
    }
    
    //- NOTE(long): Put the actual text on the screen (all the paint_text_color calls)
    {
        ProfileBlock(app, "[Long] Render Text Layout");
        // NOTE(long): The current fade's color will override all previous colors
        paint_fade_ranges(app, layout, buffer);
        draw_text_layout_default(app, layout);
    }
    
    // NOTE(long): Dividier Comments Rendering
    if (has_token_array && !def_get_config_b32_lit("f4_disable_divider_comments"))
        Long_Render_DivComments(ctx);
    
    // NOTE(long): Token Occurrence Highlight
    if (has_token_array)
    {
        ARGB_Color token_highlight = Long_ARGBFromID(fleury_color_token_highlight);
        
        // TODO(long): Rather than using tokens, this should works with normal text
        if (def_get_config_b32_lit("long_disable_token_occurrence")) 
        {
            Token* token = token_from_pos(&array, cursor_pos);
            if (token && token->kind == TokenBaseKind_Identifier)
                Long_Render_Underline(ctx, Ii64(token), token_highlight);
        }
        
        else
        {
            View_ID active_view = get_active_view(app, Access_Always);
            Buffer_ID active_buffer = view_get_buffer(app, active_view, Access_Always);
            i64 active_cursor_pos = view_get_cursor_pos(app, active_view);
            Token* cursor_token = get_token_from_pos(app, active_buffer, active_cursor_pos);
            
            if (cursor_token)
            {
                // NOTE(long): Loop the visible tokens
                ARGB_Color minor_highlight = Long_ARGBFromID(fleury_color_token_minor_highlight);
                String8 cursor_lexeme = push_token_lexeme(app, scratch, active_buffer, cursor_token);
                Token_Iterator_Array it = token_iterator_pos(0, &array, visible_range.first);
                
                for (;;)
                {
                    Token* token = token_it_read(&it);
                    if (token->pos >= visible_range.one_past_last)
                        break;
                    
                    if (token->kind == TokenBaseKind_Identifier)
                    {
                        Range_i64 range = Ii64(token);
                        String8 lexeme = push_buffer_range(app, scratch, buffer, range);
                        
                        if (range_contains(range, cursor_pos))
                            Long_Render_Underline(ctx, range, token_highlight);
                        else if (string_match(lexeme, cursor_lexeme))
                            Long_Render_Underline(ctx, range, minor_highlight);
                    }
                    
                    if (!token_it_inc(&it))
                        break;
                }
            }
        }
    }
    
    //- NOTE(long): Cursor Rendering
    Rect_f32 prev_clip = draw_set_clip(app, rect);
    switch (fcoder_mode)
    {
        case FCoderMode_Original:      Long_Render_EmacsCursor(ctx); break;
        case FCoderMode_NotepadLike: Long_Render_NotepadCursor(ctx); break;
    }
    draw_set_clip(app, prev_clip);
    
#ifdef FCODER_FLEURY_CALC_H
    // NOTE(rjf): Interpret buffer as calc code, if it's the calc buffer.
    if (string_match(buffer_name, S8Lit("*calc*")))
        F4_CLC_RenderBuffer(app, buffer, view, layout, ctx->frame);
    
    // NOTE(long): Calc Comments Rendering
    if (!def_get_config_b32_lit("f4_disable_calc_comments"))
        Long_Render_CalcComments(ctx);
#endif
}

// @COPYPASTA(long): F4_DrawFileBar
function void Long_Render_FileBar(Long_Render_Context* ctx)
{
    Application_Links* app = ctx->app;
    Buffer_ID buffer = ctx->buffer;
    View_ID view = ctx->view;
    Face_ID face = get_face_id(app, 0);
    Face_Metrics metrics = get_face_metrics(app, face);
    
    ProfileScope(app, "[Long] File Bar");
    Scratch_Block scratch(app);
    
    FColor base_color = fcolor_id(defcolor_base);
    FColor pop2_color = fcolor_id(defcolor_pop2);
    i64 line_count = buffer_get_line_count(app, buffer);
    Buffer_Cursor cursor = view_compute_cursor(app, view, seek_pos(view_get_cursor_pos(app, view)));
    
    Vec2_f32 padding = V2f32(metrics.normal_advance, f32_round32(metrics.line_height/8.f));
    Rect_f32 bar;
    {
        Rect_f32_Pair pair = rect_split_top_bottom(ctx->rect, metrics.line_height + padding.y * 2);
        ctx->rect = pair.max;
        bar = pair.min;
    }
    draw_rectangle_fcolor(app, bar, 0.f, fcolor_id(defcolor_bar));
    
    if (!def_get_config_b32_lit("f4_disable_progress_bar"))
    {
        f32 progress = (f32)(cursor.line-1) / (f32)(line_count-1);
        Rect_f32 progress_bar_rect = bar;
        progress_bar_rect.x1 = bar.x0 + (bar.x1 - bar.x0) * progress;
        
        ARGB_Color progress_bar_color = Long_ARGBFromID(fleury_color_file_progress_bar);
        if (F4_ARGBIsValid(progress_bar_color))
            draw_rectangle(app, progress_bar_rect, 0, progress_bar_color);
    }
    
    Vec2_f32 start_pos = bar.p0 + padding;
    Vec2_f32 end_pos = V2f32(bar.x1 - padding.x, bar.y0 + padding.y);
    String8 pos_str = push_stringf(scratch, "P%lld", cursor.pos);
    String8 end_str = push_stringf(scratch, "L%lld:C%-4lld %6s", cursor.line, cursor.col, pos_str.str);
    end_pos.x -= get_string_advance(app, face, end_str);
    
    Fancy_Line list = {};
    String8 unique_name = push_buffer_unique_name(app, scratch, buffer);
    push_fancy_string(scratch, &list, base_color, unique_name);
    
    if (buffer == get_comp_buffer(app))
    {
        ProfileBlock(app, "[Long] Comp File Bar");
        Character_Predicate* predicate = &character_predicate_alpha_numeric_underscore_utf8;
        Range_i64 range = buffer_range(app, buffer);
        
        String_Match_List errors = buffer_find_all_matches(app, scratch, buffer, 0, range, S8Lit(": error"  ), predicate, Scan_Forward);
        String_Match_List  warns = buffer_find_all_matches(app, scratch, buffer, 0, range, S8Lit(": warning"), predicate, Scan_Forward);
        push_fancy_stringf(scratch, &list, base_color, " - %3lld Line(s) %3d Error(s) %3d Warning(s)",
                           line_count, errors.count, warns.count);
    }
    
    else if (buffer == Long_Buffer_GetSearchBuffer(app))
    {
        Locked_Jump_State jump_state = get_locked_jump_state(app, &global_heap);
        i32 match_count = 0;
        if (jump_state.list)
            match_count = jump_state.list->jump_count;
        push_fancy_stringf(scratch, &list, base_color, " - %d Match%s", match_count, match_count != 1 ? "es" : "");
    }
    
    else if (buffer == get_buffer_by_name(app, S8Lit("*messages*"), 0))
    {
        b32 is_virtual = def_get_config_b32_lit("enable_virtual_whitespace");
        push_fancy_stringf(scratch, &list, base_color, " - Virtual Whitespace: %s", is_virtual ? "On" : "Off");
        push_fancy_stringf(scratch, &list, base_color, " - Word: Prev-%s/Next-%s",
                           (long_global_move_side>>1) ? "Max" : "Min", (long_global_move_side&1) ? "Max" : "Min");
    }
    
    else
    {
        push_fancy_stringf(scratch, &list, base_color, ": %d - ", buffer);
        
        Managed_Scope scope = buffer_get_managed_scope(app, buffer);
        Line_Ending_Kind* eol_setting = scope_attachment(app, scope, buffer_eol_setting, Line_Ending_Kind);
        String8 eol_string = {};
        switch (*eol_setting)
        {
            case LineEndingKind_Binary: eol_string = S8Lit( "bin"); break;
            case LineEndingKind_LF:     eol_string = S8Lit(  "lf"); break;
            case LineEndingKind_CRLF:   eol_string = S8Lit("crlf"); break;
        }
        push_fancy_string(scratch, &list, base_color, eol_string);
        
        Face_Description desc = get_buffer_face_description(app, buffer);
        push_fancy_stringf(scratch, &list, base_color, " - %upt ", desc.parameters.pt_size);
        
        u8 spaces[3] = {};
        {
            Dirty_State dirty = buffer_get_dirty_state(app, buffer);
            String_u8 str = Su8(spaces, 0, 3);
            
            if (dirty)
            {
                if (HasFlag(dirty, DirtyState_UnsavedChanges))
                    string_append(&str, S8Lit("*"));
                if (HasFlag(dirty, DirtyState_UnloadedChanges))
                    string_append(&str, S8Lit("!"));
            }
            
            push_fancy_string(scratch, &list, pop2_color, str.string);
        }
        
        f32 total_width = get_fancy_line_width(app, face, &list);
        if (start_pos.x + total_width >= end_pos.x)
        {
            list.last = list.first;
            list.first->next = 0;
        }
    }
    
    start_pos = draw_fancy_line(app, face, fcolor_zero(), &list, start_pos);
    if (start_pos.x < end_pos.x)
        draw_string(app, face, end_str, end_pos, base_color);
}

//~ NOTE(long): Highlight Rendering

function Long_Highlight_List* Long_Highlight_GetList(Application_Links* app, View_ID view)
{
    Managed_Scope scope = view_get_managed_scope(app, view);
    Long_Highlight_List* list = scope_attachment(app, scope, long_highlight_list, Long_Highlight_List);
    if (!list->arena.base_allocator)
        list->arena = make_arena_system();
    return list;
}

function void Long_Highlight_ClearList(Long_Highlight_List* list)
{
    linalloc_clear(&list->arena);
    list->first = 0;
}

function void Long_Highlight_Push(Application_Links* app, View_ID view, Range_i64 range)
{
    Long_Highlight_List* list = Long_Highlight_GetList(app, view);
    
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    if (buffer != list->buffer)
    {
        Long_Highlight_ClearList(list);
        list->buffer = buffer;
    }
    
    b32 overlapped = 0;
    for (Long_Highlight_Node* node = list->first; node; node = node->next)
        if (range_overlap(node->range, range))
            overlapped = 1;
    
    if (!overlapped)
    {
        Long_Highlight_Node* node = push_array_zero(&list->arena, Long_Highlight_Node, 1);
        node->range = range;
        sll_stack_push(list->first, node);
    }
}

function void Long_Highlight_Clear(Application_Links* app, View_ID view)
{
    Long_Highlight_List* list = Long_Highlight_GetList(app, view);
    Long_Highlight_ClearList(list);
}

function void Long_Highlight_DrawList(Long_Render_Context* ctx)
{
    Long_Highlight_List* list = Long_Highlight_GetList(ctx->app, ctx->view);
    if (list->buffer != ctx->buffer)
        Long_Highlight_ClearList(list);
    for (Long_Highlight_Node* node = list->first; node; node = node->next)
        Long_Render_HighlightBlock(ctx, node->range);
}

// @COPYPASTA(long): qol_draw_compile_errors and F4_RenderErrorAnnotations
function void Long_Highlight_DrawErrors(Long_Render_Context* ctx, Buffer_ID jump_buffer)
{
    Application_Links* app = ctx->app;
    Text_Layout_ID layout = ctx->layout;
    Buffer_ID buffer = ctx->buffer;
    
    ProfileScope(app, "[Long] Highlight Errors");
    Marker_List* list = jump_buffer ? get_or_make_list_for_buffer(app, &global_heap, jump_buffer) : 0;
    if (!list)
        return;
    
    Managed_Scope scopes[2];
    scopes[0] = buffer_get_managed_scope(app, jump_buffer);
    scopes[1] = buffer_get_managed_scope(app, buffer);
    Managed_Scope comp_scope = get_managed_scope_with_multiple_dependencies(app, scopes, ArrayCount(scopes));
    Managed_Object* markers_object = scope_attachment(app, comp_scope, sticky_jump_marker_handle, Managed_Object);
    
    Scratch_Block scratch(app);
    i32 count = managed_object_get_item_count(app, *markers_object);
    Marker* markers = push_array(scratch, Marker, count);
    managed_object_load_data(app, *markers_object, 0, count, markers);
    
    Sticky_Jump_Stored* storage = get_all_stored_jumps_from_list(app, scratch, list);
    i64* unique_lines = push_array_zero(scratch, i64, count);
    i32 line_count = 0;
    
    ARGB_Color annotation_color = Long_ARGBFromID(fleury_color_error_annotation);
    ARGB_Color  highlight_color = Long_ARGBFromID(defcolor_highlight_junk);
    
    for (i32 i = 0; i < count; ++i)
    {
        i64 code_line_number = get_line_number_from_pos(app, buffer, markers[i].pos);
        Range_i64 line_range = get_line_pos_range(app, buffer, code_line_number);
        if (!range_overlap(ctx->visible_range, line_range))
            continue;
        
        // NOTE(long): Get the line number inside the comp buffer
        String8 error_string = {};
        for (i32 list_index = 0; list_index < list->jump_count; ++list_index)
        {
            if (storage[list_index].jump_buffer_id == buffer && storage[list_index].index_into_marker_array == i)
            {
                String8 comp_line = push_buffer_line(app, scratch, jump_buffer, storage[list_index].list_line);
                Parsed_Jump jump = parse_jump_location(comp_line, JumpFlag_SkipSubs);
                
                if (jump.success)
                    error_string = string_skip(comp_line, jump.colon_position + 2);
                break;
            }
        }
        
        if (!error_string.size)
            continue;
        
        b32 overlapped = 0;
        for (i32 j = 0; j < line_count && !overlapped; ++j)
            if (unique_lines[j] == code_line_number)
                overlapped = 1;
        
        // NOTE(long): Draw line highlights
        if (!overlapped)
        {
            draw_line_highlight(app, layout, code_line_number, highlight_color);
            unique_lines[line_count++] = code_line_number;
        }
        
        Range_f32 y = text_layout_line_on_screen(app, layout, code_line_number);
        Rect_f32 region = text_layout_region(app, layout);
        
        // NOTE(long): Draw error annotations
        if (!overlapped)
        {
            i64 end_pos = get_line_end_pos(app, buffer, code_line_number) - 1;
            Rect_f32 end_rect = text_layout_character_on_screen(app, layout, end_pos);
            Face_ID face = global_small_code_face;
            
#if 0
            Vec2_f32 draw_position = V2f32(end_rect.x1 + 40.f, end_rect.y0);
#else
            Face_Metrics metrics = get_face_metrics(app, face);
            Vec2_f32 draw_position =
            {
                region.x1 - metrics.max_advance*error_string.size - (y.max-y.min)/2 - ctx->metrics.line_height/2,
                y.min + (y.max-y.min)/2 - ctx->metrics.line_height/2,
            };
            draw_position.x = clamp_bot(draw_position.x, end_rect.x1 + 30);
#endif
            draw_string(app, face, error_string, draw_position, annotation_color);
        }
        
        // NOTE(long): Push error messages when hovering over the highlighted line
        Vec2_f32 mouse = ctx->mouse;
        if (mouse.x >= region.x0 && mouse.x < region.x1 && mouse.y >= y.min && mouse.y < y.max)
            Long_Render_PushTooltip(0, error_string, fcolor_argb(annotation_color));
    }
}

function void Long_MC_DrawHighlights(Long_Render_Context* ctx)
{
    if (def_get_config_b32_lit("use_jump_highlight"))
        return;
    
    // TODO(long): Technically speaking only mc cursors and the main cursor must be in the active view
    // The minor highlighted matches don't need to and can still be drawn on inactive views
    // But there's currently no way for this function to know the size of each highlight (the search string's length)
    if (!ctx->is_active_view)
        return;
    
    Application_Links* app = ctx->app;
    Text_Layout_ID layout = ctx->layout;
    View_ID view = ctx->view;
    Buffer_ID search_buffer = Long_Buffer_GetSearchBuffer(app);
    
    if (search_buffer && string_match(locked_buffer, search_name))
    {
        Scratch_Block scratch(app);
        Managed_Scope scope = buffer_get_managed_scope(app, search_buffer);
        i32 count;
        Marker* markers = Long_SearchBuffer_GetMarkers(app, scratch, scope, ctx->buffer, &count);
        
        if (count)
        {
            i64 cursor = view_get_cursor_pos(app, view);
            i64 mark   =   view_get_mark_pos(app, view);
            i64 size   = mark - cursor;
            Assert(size >= 0);
            
            ARGB_Color token_color = Long_ARGBFromID(fleury_color_token_minor_highlight);
            for (i32 i = 0; i < count; i += 1)
                if (range_contains(ctx->visible_range, markers[i].pos))
                    Long_Render_DrawBlock(ctx, Ii64_size(markers[i].pos, size), token_color);
            
            for_mc(node, mc_context.cursors)
                Long_Render_HighlightBlock(ctx, Ii64(node->cursor_pos, node->mark_pos), .75f);
            Long_Render_HighlightBlock(ctx, Ii64(cursor, mark));
        }
    }
}

// @COPYPASTA(long): draw_highlight_range
function b32 Long_Highlight_DrawRange(Long_Render_Context* ctx)
{
    Application_Links* app = ctx->app;
    Managed_Scope scope = view_get_managed_scope(app, ctx->view);
    Buffer_ID* highlight_buffer = scope_attachment(app, scope, view_highlight_buffer, Buffer_ID);
    
    b32 has_highlight_range = 0;
    if (*highlight_buffer != 0)
    {
        if (*highlight_buffer != ctx->buffer)
            view_disable_highlight_range(app, ctx->view);
        
        else
        {
            has_highlight_range = 1;
            Managed_Object* highlight = scope_attachment(app, scope, view_highlight_range, Managed_Object);
            Marker marker_range[2];
            if (managed_object_load_data(app, *highlight, 0, 2, marker_range))
                Long_Render_HighlightBlock(ctx, Ii64(marker_range[0].pos, marker_range[1].pos));
        }
    }
    
    return has_highlight_range;
}

function b32 Long_Highlight_HasRange(Application_Links* app, View_ID view)
{
    Managed_Scope scope = view_get_managed_scope(app, view);
    Managed_Object* highlight = scope_attachment(app, scope, view_highlight_range, Managed_Object);
    return *highlight != 0;
}

function void Long_Render_FadeError(Application_Links* app, Buffer_ID buffer, Range_i64 range)
{
    buffer_post_fade(app, buffer, .5f, range, Long_ARGBFromID(defcolor_pop2));
}

function void Long_Render_FadeHighlight(Application_Links* app, Buffer_ID buffer, Range_i64 range)
{
    buffer_post_fade(app, buffer, .5f, range, Long_ARGBFromID(defcolor_highlight));
}

// @COPYPASTA(lonng): draw_comment_highlights
function void Long_Highlight_Comment(Long_Render_Context* ctx, Comment_Highlight_Pair* pairs, i32 pair_count)
{
    Application_Links* app = ctx->app;
    Token_Iterator_Array it = token_iterator_pos(ctx->buffer, &ctx->array, ctx->visible_range.first);
    
    while (1)
    {
        Scratch_Block scratch(app);
        Token* token = token_it_read(&it);
        if (token->pos >= ctx->visible_range.one_past_last)
            break;
        
        String8 tail = {};
        if (token_it_check_and_get_lexeme(app, scratch, &it, TokenBaseKind_Comment, &tail))
        {
            for (i64 index = token->pos; tail.size > 0; tail = string_skip(tail, 1), index += 1)
            {
                if (index > token->pos && character_is_alpha_numeric(tail.str[-1]))
                    continue;
                
                for (Comment_Highlight_Pair* pair = pairs; pair != pairs+pair_count; pair++)
                {
                    u64 needle_size = pair->needle.size;
                    if (needle_size == 0)
                        continue;
                    
                    String8 prefix = string_prefix(tail, needle_size);
                    if (!string_match(prefix, pair->needle))
                        continue;
                    
                    if (tail.size > prefix.size && character_is_alpha_numeric(prefix.str[prefix.size]))
                        continue;
                    
                    paint_text_color(app, ctx->layout, Ii64_size(index, needle_size), pair->color);
                    tail = string_skip(tail, needle_size);
                    index += needle_size;
                    break;
                }
            }
        }
        
        if (!token_it_inc_non_whitespace(&it))
            break;
    }
}

function b32 Long_Buffer_Match(Application_Links* app, Buffer_ID buffer, i64 pos, Character_Predicate predicate)
{
    u8 c = buffer_get_char(app, buffer, pos);
    b32 result = character_predicate_check_character(predicate, c);
    return result;
}

// @COPYPASTA(long): draw_whitespace_highlight
function void Long_Highlight_Whitespace(Long_Render_Context* ctx)
{
    Application_Links* app = ctx->app;
    Text_Layout_ID layout = ctx->layout;
    Buffer_ID buffer = ctx->buffer;
    ProfileScope(app, "[Long] Highlight Whitespace");
    
    local_const Character_Predicate character_space = { {
            0,  26,   0,   0,   1,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,
        } };
    
    //- NOTE(long): Look up or create the font
    Face_ID face;
    {
        Managed_Scope scope = buffer_get_managed_scope(app, buffer);
        Face_ID* buf_face = scope_attachment(app, scope, long_buffer_face, Face_ID);
        face = *buf_face;
        
        Face_Description buf_desc = get_face_description(app, face);
        Face_Description ctx_desc = get_face_description(app, ctx->face);
        u32 face_size = u32(ctx_desc.parameters.pt_size*1.5f);
        
        if (!font_load_location_match(&buf_desc.font, &ctx_desc.font) || buf_desc.parameters.pt_size != face_size)
        {
            ProfileBlock(app, "[Long] Create Face");
            // NOTE(long): The bottom panels have a different font than the normal panels
            // We probably don't want to highlight whitespace in those anyway, so we can just use ctx_desc
            // If you prefer to highlight those with the same global font, you can use get_global_face_description instead
            Face_Description def_desc = ctx_desc;
            def_desc.parameters.pt_size = face_size;
            
            Face_ID new_face = try_create_new_face(app, &def_desc);
            if (face)
                try_release_face(app, face, new_face);
            *buf_face = face = new_face;
        }
    }
    
    //- NOTE(long): Calculate the circle's offset
    Vec2_f32 offset;
    {
        Face_Metrics normal_metrics = ctx->metrics;
        Face_Metrics custom_metrics = get_face_metrics(app, face);
        Vec2_f32 normal_pos = V2f32(normal_metrics.space_advance/2.f, normal_metrics.ascent);
        Vec2_f32 custom_pos = V2f32(custom_metrics.space_advance/2.f, custom_metrics.ascent);
        
        offset = custom_pos-normal_pos;
        offset.y += normal_pos.y/4;
    }
    
    //- NOTE(long): Checking for whitespace
    ARGB_Color color = Long_ARGBFromID(defcolor_line_numbers_text);
    for (i64 i = ctx->visible_range.min; i < ctx->visible_range.max; ++i)
    {
        if (Long_Buffer_Match(app, buffer, i, character_space))
        {
            ProfileBlock(app, "[Long] Draw Circle");
            i64 j = i+1;
            for (; j < ctx->visible_range.max && Long_Buffer_Match(app, buffer, j, character_space); ++j);
            
            //- NOTE(long): Draw all cells inside the block
            for (i64 pos = i; pos < j; ++pos)
            {
                Rect_f32 rect = text_layout_character_on_screen(app, layout, pos);
                if (rect.x0 < rect.x1 && rect.y0 < rect.y1)
                    // TODO(long): Use defcolor_at_highlight when the current character is highlighted
                    draw_string(app, face, S8Lit("."), rect.p0 - offset, color);
            }
            
            i = j-1;
        }
    }
}

CUSTOM_COMMAND_SIG(long_toggle_show_whitespace_all_buffers)
CUSTOM_DOC("Toggles whitespace visibility status for all buffers.")
{
    def_toggle_config_b32_lit("long_global_show_whitespace");
}

//~ NOTE(long): Brace Rendering

function i64 Long_Nest_DelimPos(Application_Links* app, Token_Array array, Find_Nest_Flag flags, i64 pos)
{
    if (array.tokens)
    {
        Token_Iterator_Array it = token_iterator_pos(0, &array, pos);
        Token* token = it.ptr;
        Nest_Delimiter_Kind delim = get_nest_delimiter_kind(token->kind, flags);
        if (delim == NestDelim_Open)
            pos = token->pos + token->size;
        
        else if (token_it_dec_all(&it))
        {
            token = it.ptr;
            delim = get_nest_delimiter_kind(token->kind, flags);
            if (delim == NestDelim_Close && pos == token->pos + token->size)
                pos = token->pos;
        }
    }
    
    return pos;
}

// @COPYPASTA(long): F4_Brace_RenderCloseBraceAnnotation
function void Long_Render_BraceAnnotation(Long_Render_Context* ctx, i64 pos, Color_Array colors)
{
    Application_Links* app = ctx->app;
    Buffer_ID buffer = ctx->buffer;
    Scratch_Block scratch(app);
    
    ProfileScope(app, "[F4] Brace Annotation");
    Range_i64_Array ranges = get_enclosure_ranges(app, scratch, buffer, pos, FindNest_Scope);
    
    for (i32 i = ranges.count - 1; i >= 0; --i)
    {
        Range_i64 range = ranges.ranges[i];
        
        // NOTE(rjf): Turn this on to only display end scope annotations where the top is off-screen.
#if 0
        if (range.start >= ctx->visible_range.start)
            continue;
#endif
        
        // NOTE(jack): Prevent brace annotations from printing on single line scopes.
        Range_i64 line_range = get_line_range_from_pos_range(app, buffer, range);
        if (line_range.min == line_range.max)
            continue;
        
        if (i > 0)
        {
            Range_i64 prev_range = get_line_range_from_pos_range(app, buffer, ranges.ranges[i-1]);
            if (prev_range.min == line_range.min || prev_range.max == line_range.max)
                continue;
        }
        
        // NOTE(rjf): Find token set before this scope begins.
        Token* start_token = 0;
        Token_Iterator_Array it = token_iterator_pos(0, &ctx->array, range.start-1);
        i32 paren_nest = 0;
        
        // TODO(long): Replace this with Code_Index_Nest
        do {
            Token* token = token_it_read(&it);
            if (!token)
                break;
            else if (token->kind == TokenBaseKind_ParentheticalClose)
                ++paren_nest;
            else if (token->kind == TokenBaseKind_ParentheticalOpen)
                --paren_nest;
            
            else if (paren_nest == 0)
            {
                // NOTE(long): This sub_kind is C-specific
                b32 is_c_label = token->kind == TokenBaseKind_StatementClose && token->sub_kind != TokenCppKind_Colon;
                if (is_c_label || token->kind == TokenBaseKind_ScopeOpen || token->kind == TokenBaseKind_ScopeClose)
                    break;
                
                b32 is_atom = (token->kind == TokenBaseKind_LiteralInteger || token->kind == TokenBaseKind_LiteralFloat ||
                               token->kind == TokenBaseKind_LiteralString  || token->kind == TokenBaseKind_Identifier);
                if (is_atom || token->kind == TokenBaseKind_Keyword        || token->kind == TokenBaseKind_Comment)
                {
                    if (start_token)
                        if (get_line_number_from_pos(app, buffer, token->pos) != get_line_number_from_pos(app, buffer, start_token->pos))
                            break;
                    start_token = token;
                }
            }
        } while (token_it_dec_non_whitespace(&it));
        
        // NOTE(rjf): Draw.
        if (start_token)
        {
            // NOTE(long): Rather than drawing the entire line (Long_Buffer_PushLine), we only draw from the start_token to the end
            String8 line;
            {
                Range_i64 annotation_range = Ii64(start_token->pos);
                annotation_range.max = get_line_end_pos_from_pos(app, buffer, start_token->pos);
                line = push_buffer_range(app, scratch, buffer, annotation_range);
                line = string_skip_chop_whitespace(line);
            }
            
            i64 last_char = get_line_end_pos_from_pos(app, buffer, range.end) - 1;
            Rect_f32 close_scope_rect = text_layout_character_on_screen(app, ctx->layout, last_char);
            if (rect_width(close_scope_rect) > 0)
            {
                f32 draw_offset = ctx->metrics.space_advance;
                Vec2_f32 close_scope_pos = V2f32(close_scope_rect.x1 + draw_offset, close_scope_rect.y0);
                ARGB_Color color = colors.vals[(ranges.count-i-1) % colors.count];
                draw_string(app, global_small_code_face, line, close_scope_pos, color);
            }
        }
    }
}

// @COPYPASTA(long): F4_Brace_RenderHighlight
function void Long_Render_BraceHighlight(Long_Render_Context* ctx, i64 pos, Color_Array colors)
{
    draw_enclosures(ctx->app, ctx->layout, ctx->buffer, pos, FindNest_Scope,
                    RangeHighlightKind_CharacterHighlight, 0, 0, colors.vals, colors.count);
}

// @COPYPASTA(long): F4_Brace_RenderLines
function void Long_Render_BraceLines(Long_Render_Context* ctx, i64 pos, Color_Array colors)
{
    Application_Links* app = ctx->app;
    Text_Layout_ID layout = ctx->layout;
    Face_Metrics metrics = ctx->metrics;
    Buffer_ID buffer = ctx->buffer;
    View_ID view = ctx->view;
    Rect_f32 rect = text_layout_region(app, ctx->layout);
    
    ProfileScope(app, "[F4] Brace Lines");
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
    f32 off_x = rect.x0 - scroll.position.pixel_shift.x + f32_floor32(metrics.normal_advance/2.f);
    u64 vw_indent = def_get_config_u64_lit(app, "virtual_whitespace_regular_indent");
    
    Scratch_Block scratch(app);
    Range_i64_Array ranges = get_enclosure_ranges(app, scratch, buffer, pos, FindNest_Scope);
    
    for (i32 i = ranges.count - 1; i >= 0; i -= 1)
    {
        Range_i64 range = ranges.ranges[i];
        f32 pos_x;
        if (def_get_config_b32_lit("enable_virtual_whitespace"))
            pos_x = (ranges.count-i-1) * metrics.space_advance * vw_indent;
        
        else
        {
            Range_i64 line_range = get_line_range_from_pos_range(app, buffer, range);
            b32 use_brace_pos = 0;
            if (i < ranges.count - 1)
            {
                Range_i64 prev_range = get_line_range_from_pos_range(app, buffer, ranges.ranges[i+1]);
                use_brace_pos = prev_range.min == line_range.min || prev_range.max == line_range.max;
            }
            
            u64 beg_idx, end_idx;
            if (use_brace_pos)
            {
                i64 beg_pos = get_line_start_pos(app, buffer, line_range.min);
                i64 end_pos = get_line_start_pos(app, buffer, line_range.max);
                beg_idx = range.min - beg_pos;
                end_idx = range.max - end_pos - 1;
            }
            
            else
            {
                String8 line_beg = push_buffer_line(app, scratch, buffer, line_range.min);
                String8 line_end = push_buffer_line(app, scratch, buffer, line_range.max);
                beg_idx = string_find_first_non_whitespace(line_beg);
                end_idx = string_find_first_non_whitespace(line_end);
            }
            
            pos_x = metrics.space_advance * Min(beg_idx, end_idx);
        }
        
        Range_f32 y = rect_y(rect);
        {
            Rect_f32 beg_rect = text_layout_character_on_screen(app, layout, range.min);
            Rect_f32 end_rect = text_layout_character_on_screen(app, layout, range.max);
            if (range.min >= ctx->visible_range.min)
                y.min = beg_rect.y0 + metrics.line_height;
            if (range.max <= ctx->visible_range.max)
                y.max = end_rect.y0 - 4.f;
        }
        
        // NOTE(long): We can't use the open/close braces rect directly because they may not be visible
        pos_x += off_x;
        f32 thickness = metrics.normal_advance/8.f;
        Rect_f32 line_rect = Rf32(pos_x, y.min, pos_x + thickness, y.max);
        draw_rectangle(app, line_rect, 0.5f, colors.vals[(ranges.count-i-1) % colors.count]);
    }
}

//~ NOTE(long): Colors

function ARGB_Color Long_ARGBFromID(Managed_ID id, i32 subindex)
{
    return F4_ARGBFromID(active_color_table, id, subindex);
}

function ARGB_Color Long_Color_Alpha(ARGB_Color color, f32 alpha)
{
    Vec4_f32 rgba = unpack_color(color);
    rgba.a = alpha;
    return pack_color(rgba);
}

function ARGB_Color Long_Color_Cursor()
{
    ARGB_Color color;
    if (global_keyboard_macro_is_recording)
        color = Long_ARGBFromID(fleury_color_cursor_macro);
    else
        color = Long_ARGBFromID(defcolor_cursor, GlobalKeybindingMode);
    return color;
}

// @COPYPASTA(long): F4_SyntaxHighlight
function void Long_Syntax_Highlight(Long_Render_Context* ctx)
{
    Application_Links* app = ctx->app;
    Text_Layout_ID layout = ctx->layout;
    Token_Array* array = &ctx->array;
    Buffer_ID buffer = ctx->buffer;
    
    Color_Table table = active_color_table;
    ARGB_Color default_color = F4_ARGBFromID(table, defcolor_text_default);
    ARGB_Color comment_tag_color = F4_ARGBFromID(table, fleury_color_index_comment_tag);
    b32 valid_comment_color = F4_ARGBIsValid(comment_tag_color);
    
    local_const ARGB_Color color_table[TokenBaseKind_COUNT] = {
#define X(kind, color) F4_ARGBFromID(table, color),
        Long_Token_ColorTable(X)
#undef X
    };
    
    local_const F4_SyntaxFlags flag_table[TokenBaseKind_COUNT] = {
        0, 0, 0, // EOF, Whitespace, LexError
        F4_SyntaxFlag_Preprocessor,
        F4_SyntaxFlag_Keywords,
        F4_SyntaxFlag_Preprocessor,
        0,
        F4_SyntaxFlag_Operators,
        F4_SyntaxFlag_Literals,
        F4_SyntaxFlag_Literals,
        F4_SyntaxFlag_Literals,
        F4_SyntaxFlag_Operators,
        F4_SyntaxFlag_Operators,
        F4_SyntaxFlag_Operators,
        F4_SyntaxFlag_Operators,
        F4_SyntaxFlag_Operators,
    };
    
    ARGB_Color alt_int_color = default_color;
    {
        ARGB_Color int_color = color_table[TokenBaseKind_LiteralInteger];
        Vec4_f32 rgba = unpack_color(int_color);
        
        f32 coeff = .7f;
        if (rgba.r < .5f && rgba.g < .5f && rgba.b < .5f)
            coeff = 1.3f;
        
        rgba.r *= coeff;
        rgba.g *= coeff;
        rgba.b *= coeff;
        alt_int_color = pack_color(rgba);
    }
    
    Long_Index_ProfileScope(app, "[Long] Syntax Highlight");
    Token_Iterator_Array it = token_iterator_pos(0, array, ctx->visible_range.first);
    F4_Language* lang = F4_LanguageFromBuffer(app, buffer);
    
    for (;;)
    {
        Token* token = token_it_read(&it);
        if (!token || token->pos >= ctx->visible_range.one_past_last)
            break;
        
        Scratch_Block scratch(app);
        String8 lexeme = push_token_lexeme(app, scratch, buffer, token);
        
        ARGB_Color argb = color_table[token->kind];
        F4_SyntaxFlags syntax_flag = flag_table[token->kind];
        
#define Long_SetColor(flag, id) do { syntax_flag = (F4_SyntaxFlag_##flag); argb = F4_ARGBFromID(table, (id)); } while (0)
        // NOTE(long): Setup Color and Syntax Flags
        if (token->kind == TokenBaseKind_Identifier)
        {
            F4_Index_Note* note = Long_Index_LookupBestNote(app, buffer, array, token);
            if (note)
            {
                switch (note->kind)
                {
                    case F4_Index_NoteKind_Type:     Long_SetColor(Types,     fleury_color_index_product_type); break;
                    case F4_Index_NoteKind_Macro:    Long_SetColor(Macros,    fleury_color_index_macro);    break;
                    case F4_Index_NoteKind_Function: Long_SetColor(Functions, fleury_color_index_function); break;
                    case F4_Index_NoteKind_Constant: Long_SetColor(Constants, fleury_color_index_constant); break;
                    case F4_Index_NoteKind_Decl:     Long_SetColor(Constants, fleury_color_index_decl);     break;
                }
                
                switch (note->kind)
                {
                    case F4_Index_NoteKind_Decl:
                    {
                        if (note->parent)
                        {
                            if (Long_Index_IsArgument(note))
                                Long_SetColor(Constants, long_color_index_param);
                            else if (note->parent->kind == F4_Index_NoteKind_Type)
                                Long_SetColor(Constants, long_color_index_field);
                            else
                                Long_SetColor(Constants, long_color_index_local);
                        }
                    } break;
                    
                    case F4_Index_NoteKind_Type:
                    {
                        if (Long_Index_IsNamespace(note)) break;
                        
                        NOTE_TYPE_CASE:
                        if (F4_ARGBIsValid(F4_ARGBFromID(table, fleury_color_index_sum_type)))
                        {
                            String8 base_type = push_buffer_range(app, scratch, note->file->buffer, note->base_range);
                            String8 types[] = { S8Lit("struct"), S8Lit("enum") };
                            if (Long_Index_IsMatch(base_type, ExpandArray(types) || (note->flags & F4_Index_NoteFlag_SumType)))
                                Long_SetColor(Types, fleury_color_index_sum_type);
                        }
                    } break;
                    
                    case F4_Index_NoteKind_Function:
                    {
                        if (note->parent && note->parent->kind == F4_Index_NoteKind_Type && string_match(note->string, note->parent->string))
                        {
                            note = note->parent;
                            goto NOTE_TYPE_CASE;
                        }
                    } break;
                }
            }
        }
        
        else if (lang && lang->LexFullInput == (F4_Language_LexFullInput*)lex_full_input_cpp_breaks)
        {
            // TODO(long): Abstract this to user-defined language
            switch (token->sub_kind)
            {
                case TokenCppKind_LiteralTrue:
                case TokenCppKind_LiteralFalse:  Long_SetColor(Literals, defcolor_bool_constant); break;
                case TokenCppKind_PPIncludeFile: Long_SetColor(Literals, defcolor_include);       break;
                
                case TokenCppKind_LiteralCharacter:
                case TokenCppKind_LiteralCharacterWide:
                case TokenCppKind_LiteralCharacterUTF8:
                case TokenCppKind_LiteralCharacterUTF16:
                case TokenCppKind_LiteralCharacterUTF32: Long_SetColor(Literals, defcolor_char_constant); break;
            }
        }
#undef Long_SetColor
        
        // NOTE(long): Color Blend/Transitions
        f32 t = f4_syntax_flag_transitions[BitOffset(syntax_flag)];
        if (F4_ARGBIsValid(argb))
            argb = color_blend(default_color, t, argb);
        else
            argb = default_color;
        paint_text_color(app, layout, Ii64(token), argb);
        
        // NOTE(rjf): Substring from comments
        if (valid_comment_color && token->kind == TokenBaseKind_Comment)
        {
            ARGB_Color color = color_blend(default_color, t, comment_tag_color);
            for (u64 i = 0; i < lexeme.size; i += 1)
            {
                if (lexeme.str[i] == '@')
                {
                    u64 j = i+1;
                    for (; j < lexeme.size && character_is_alpha_numeric(lexeme.str[j]); j += 1);
                    paint_text_color(app, layout, Ii64(token->pos+i, token->pos+j), color);
                }
            }
        }
        
        // NOTE(long): RADDBG-style alternative digit group
        else if (token->kind == TokenBaseKind_LiteralInteger && token->size >= 3 && character_is_base10(lexeme.str[0]))
        {
            // @CONSIDER(long): C-like octal
            b32 has_prefix = lexeme.str[0] == '0' && character_is_alpha(lexeme.str[1]);
            u64 suffix_pos = 0;
            for (u64 i = has_prefix ? 2 : 1; i < lexeme.size && !suffix_pos; ++i)
                if (character_to_lower(lexeme.str[i]) > 'f')
                    suffix_pos = i;
            
            i64 chunk_size = has_prefix ? 4 : 3;
            ARGB_Color colors[] = { argb, color_blend(default_color, t, alt_int_color) };
            
            Range_i64 token_range = Ii64(token);
            token_range.min += 2*has_prefix;
            
            if (suffix_pos)
            {
                Range_i64 suffix_range = Ii64(token->pos + suffix_pos, token_range.max);
                token_range.max = suffix_range.min;
                paint_text_color(app, layout, suffix_range, colors[1]);
            }
            
            i64 size = range_size(token_range);
            if (has_prefix)
                paint_text_color(app, layout, Ii64_size(token->pos, 2), colors[!((size-1)/chunk_size % 2)]);
            
            for (i64 i = 0; i < size; i += chunk_size)
            {
                Range_i64 range = Ii64_size(token_range.max-i, -chunk_size);
                range.min = clamp_bot(range.min, token_range.min);
                paint_text_color(app, layout, range, colors[(i/chunk_size) % 2]);
            }
        }
        
        if (!token_it_inc_all(&it))
            break;
    }
    
    // NOTE(long): Language Highlight
    if (lang != 0 && lang->Highlight != 0)
    {
        Long_Index_ProfileBlock(app, "[Long] Language Highlight");
        lang->Highlight(app, layout, array, table);
    }
}

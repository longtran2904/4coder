
//~ NOTE(long): Cursor Rendering

// @COYPASTA(long): C4_RenderCursorSymbolThingy
function void Long_Render_CursorSymbol(Application_Links* app, Rect_f32 rect, f32 roundness, f32 thickness,
                                       ARGB_Color color, Cursor_Type type)
{
    f32 line_height = rect.y1 - rect.y0;
    f32 bracket_width = 0.5f * line_height;
    
    if (type == cursor_open_range)
    {
        Rect_f32 start_top, start_side, start_bottom;
        Vec2_f32 start_p = {rect.x0, rect.y0};
        
        start_top.x0 = start_p.x + thickness;
        start_top.x1 = start_p.x + bracket_width;
        start_top.y0 = start_p.y;
        start_top.y1 = start_p.y + thickness;
        
        start_bottom.x0 = start_top.x0;
        start_bottom.x1 = start_top.x1;
        start_bottom.y1 = start_p.y + line_height;
        start_bottom.y0 = start_bottom.y1 - thickness;
        
        start_side.x0 = start_p.x;
        start_side.x1 = start_p.x + thickness;
        start_side.y0 = start_top.y0;
        start_side.y1 = start_bottom.y1;
        
        draw_rectangle(app, start_top, roundness, color);
        draw_rectangle(app, start_side, roundness, color);
    }
    else if(type == cursor_close_range)
    {
        Rect_f32 end_top, end_side, end_bottom;
        Vec2_f32 end_p = {rect.x0, rect.y0};
        
        end_top.x0 = end_p.x;
        end_top.x1 = end_p.x - bracket_width;
        end_top.y0 = end_p.y;
        end_top.y1 = end_p.y + thickness;
        
        end_side.x1 = end_p.x;
        end_side.x0 = end_p.x + thickness;
        end_side.y0 = end_p.y;
        end_side.y1 = end_p.y + line_height;
        
        end_bottom.x0 = end_top.x0;
        end_bottom.x1 = end_top.x1;
        end_bottom.y1 = end_p.y + line_height;
        end_bottom.y0 = end_bottom.y1 - thickness;
        
        draw_rectangle(app, end_bottom, roundness, color);
        draw_rectangle(app, end_side, roundness, color);
    }
}

// @COPYPASTA(long): DoTheCursorInterpolation
function void Long_Render_CursorInterpolation(Application_Links* app, Frame_Info frame_info,
                                              Rect_f32* rect, Rect_f32* last_rect, Rect_f32 target)
{
    Rect_f32 _rect_;
    if (!last_rect)
        last_rect = &_rect_;
    *last_rect = *rect;
    
    float x_change = target.x0 - rect->x0;
    float y_change = target.y0 - rect->y0;
    
    float cursor_size_x = (target.x1 - target.x0);
    float cursor_size_y = (target.y1 - target.y0) * (1 + fabsf(y_change) / 60.f);
    
    b32 should_animate_cursor = !def_get_config_b32(vars_save_string_lit("f4_disable_cursor_trails"));
    if (should_animate_cursor)
    {
        if (fabs(x_change) > 1.f || fabs(y_change) > 1.f)
            animate_in_n_milliseconds(app, 0);
    }
    else
    {
        *rect = *last_rect = target;
        cursor_size_y = target.y1 - target.y0;
    }
    
    if (should_animate_cursor)
    {
        rect->x0 += (x_change) * frame_info.animation_dt * 30.f;
        rect->y0 += (y_change) * frame_info.animation_dt * 30.f;
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

function void Long_Render_DrawBlock(Application_Links* app, Text_Layout_ID layout, Range_i64 range, f32 roundness, FColor color)
{
    for (i64 i = range.first; i < range.one_past_last; ++i)
    {
        Rect_f32 rect = text_layout_character_on_screen(app, layout, i);
        if (Long_Rf32_Invalid(rect))
            range.first++;
        else
            break;
    }
    draw_character_block(app, layout, range, roundness, color);
}

// @COPYPASTA(long): F4_Cursor_RenderNotepadStyle
function Rect_f32 Long_Render_CursorRect(Application_Links* app, View_ID view_id, Buffer_ID buffer, Text_Layout_ID layout,
                                         i64 cursor, i64 mark, f32 roundness, f32 thickness)
{
    Rect_f32 view_rect = view_get_screen_rect(app, view_id);
    
    if (cursor != mark)
    {
        Range_i64 range = Ii64(cursor, mark);
        Long_Render_DrawBlock(app, layout, range, roundness, fcolor_id(defcolor_highlight));
        paint_text_color_fcolor(app, layout, range, fcolor_id(defcolor_at_highlight));
    }
    
    // NOTE(rjf): Draw cursor
    Rect_f32 rect = text_layout_character_on_screen(app, layout, cursor);
    {
        ARGB_Color cursor_color = Long_GetColor(app, ColorCtx_Cursor(0, GlobalKeybindingMode));
        rect.x1 = rect.x0 + thickness;
        if(rect.x0 < view_rect.x0)
        {
            rect.x0 = view_rect.x0;
            rect.x1 = view_rect.x0 + thickness;
        }
        
        draw_rectangle(app, rect, roundness, cursor_color);
    }
    
    return rect;
}

function void Long_Render_NotepadCursor(Application_Links* app, View_ID view, b32 is_active_view,
                                        Buffer_ID buffer, Text_Layout_ID layout,
                                        f32 roundness, f32 thickness, Frame_Info frame)
{
    //b32 has_highlight_range = Long_Highlight_DrawRange(app, view, buffer, layout, roundness);
    b32 has_highlight_range = Long_Highlight_HasRange(app, view);
    if (!has_highlight_range)
    {
        i64 cursor_pos = view_get_cursor_pos(app, view);
        i64 mark_pos = view_get_mark_pos(app, view);
        Rect_f32 rect = Long_Render_CursorRect(app, view, buffer, layout,
                                               cursor_pos, mark_pos, roundness, thickness);
        
        if (is_active_view)
            Long_Render_CursorInterpolation(app, frame, &global_cursor_rect, &global_last_cursor_rect, rect);
        ARGB_Color cursor_color = Long_GetColor(app, ColorCtx_Cursor(0, GlobalKeybindingMode));
        ARGB_Color  ghost_color = Long_Color_Alpha(cursor_color, 0.5f);
        draw_rectangle(app, global_cursor_rect, roundness, ghost_color);
        
        if (view == mc_context.view)
        {
            Range_i64 visible_range = text_layout_get_visible_range(app, layout);
            for_mc(node, mc_context.cursors)
            {
                ARGB_Color mc_color = F4_ARGBFromID(active_color_table, defcolor_cursor, 1);
                
                if (range_contains(visible_range, node->cursor_pos) || range_contains(visible_range, node->mark_pos))
                {
                    Rect_f32 target_rect = text_layout_character_on_screen(app, layout, node->cursor_pos);
                    node->nxt_position = target_rect.p0;
                    // TODO(long): cursor interpolation
                    Long_Render_CursorRect(app, view, buffer, layout, node->cursor_pos, node->mark_pos, roundness, thickness);
                }
            }
        }
    }
}

// @COPYPASTA(long): F4_Cursor_RenderEmacsStyle
function void Long_Render_EmacsCursor(Application_Links* app, View_ID view_id, b32 is_active_view,
                                      Buffer_ID buffer, Text_Layout_ID text_layout_id,
                                      f32 roundness, f32 outline_thickness, Frame_Info frame_info)
{
    Rect_f32 view_rect = view_get_screen_rect(app, view_id);
    Rect_f32 clip = draw_set_clip(app, view_rect);
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    
    //b32 has_highlight_range = Long_Highlight_DrawRange(app, view_id, buffer, text_layout_id, roundness);
    b32 has_highlight_range = Long_Highlight_HasRange(app, view_id);
    if (!has_highlight_range)
    {
        ColorFlags flags = 0;
        flags |= !!global_keyboard_macro_is_recording * ColorFlag_Macro;
        ARGB_Color cursor_color = Long_GetColor(app, ColorCtx_Cursor(flags, GlobalKeybindingMode));
        ARGB_Color mark_color = cursor_color;
        ARGB_Color inactive_cursor_color = F4_ARGBFromID(active_color_table, fleury_color_cursor_inactive, 0);
        
        if (!F4_ARGBIsValid(inactive_cursor_color))
            inactive_cursor_color = cursor_color;
        
        if (is_active_view == 0)
        {
            cursor_color = inactive_cursor_color;
            mark_color = inactive_cursor_color;
        }
        
        i64 cursor_pos = view_get_cursor_pos(app, view_id);
        i64 mark_pos = view_get_mark_pos(app, view_id);
        
        Cursor_Type cursor_type = cursor_none;
        Cursor_Type mark_type = cursor_none;
        
        if (cursor_pos <= mark_pos)
        {
            cursor_type = cursor_open_range;
            mark_type = cursor_close_range;
        }
        else
        {
            cursor_type = cursor_close_range;
            mark_type = cursor_open_range;
        }
        
        Rect_f32 target_cursor = text_layout_character_on_screen(app, text_layout_id, cursor_pos);
        Rect_f32 target_mark = text_layout_character_on_screen(app, text_layout_id, mark_pos);
        
        // NOTE(rjf): Draw cursor.
        if (is_active_view)
        {
            if (cursor_pos < visible_range.start || cursor_pos > visible_range.end)
            {
                f32 width = target_cursor.x1 - target_cursor.x0;
                target_cursor.x0 = view_rect.x0;
                target_cursor.x1 = target_cursor.x0 + width;
            }
            
            Long_Render_CursorInterpolation(app, frame_info, &global_cursor_rect, &global_last_cursor_rect, target_cursor);
            
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
            
            Long_Render_CursorInterpolation(app, frame_info, &global_mark_rect, &global_last_mark_rect, target_mark);
        }
        
        // NOTE(rjf): Draw main cursor.
        Long_Render_CursorSymbol(app, global_cursor_rect, roundness, outline_thickness, cursor_color, cursor_type);
        Long_Render_CursorSymbol(app, target_cursor     , roundness, outline_thickness, cursor_color, cursor_type);
        
        // NOTE(rjf): GLOW IT UP
        for (i32 glow = 0; glow < 20; ++glow)
        {
            Vec4_f32 rgba = unpack_color(cursor_color);
            rgba.a = 0.1f - glow*0.015f;
            if (rgba.a > 0)
            {
                Rect_f32 glow_rect = target_cursor;
                glow_rect.x0 -= glow;
                glow_rect.y0 -= glow;
                glow_rect.x1 += glow;
                glow_rect.y1 += glow;
                Long_Render_CursorSymbol(app, glow_rect, roundness + glow*0.7f, 2.f, pack_color(rgba), cursor_type);
            }
            else break;
        }
        
        Long_Render_CursorSymbol(app, global_mark_rect, roundness, 2.0f, Long_Color_Alpha(mark_color, 0.50f), mark_type);
        Long_Render_CursorSymbol(app,      target_mark, roundness, 2.0f, Long_Color_Alpha(mark_color, 0.75f), mark_type);
        
        if (view_id == mc_context.view)
        {
            for_mc(node, mc_context.cursors)
            {
                ARGB_Color mc_color = F4_ARGBFromID(active_color_table, defcolor_cursor, 1);
                
                if (range_contains(visible_range, node->cursor_pos))
                {
                    Rect_f32 target_rect = text_layout_character_on_screen(app, text_layout_id, node->cursor_pos);
                    node->nxt_position = target_rect.p0;
                    // TODO(long): cursor interpolation
                    Long_Render_CursorSymbol(app, target_rect, roundness, outline_thickness, Long_Color_Alpha(mc_color, .75f), cursor_type);
                }
                
                if (range_contains(visible_range, node->mark_pos))
                {
                    Rect_f32 target_rect = text_layout_character_on_screen(app, text_layout_id, node->mark_pos);
                    Long_Render_CursorSymbol(app, target_rect, roundness, outline_thickness, Long_Color_Alpha(mc_color, 0.50f), mark_type);
                }
            }
        }
    }
    
    draw_set_clip(app, clip);
}

// @COPYPASTA(long): F4_HighlightCursorMarkRange
function void Long_HighlightCursorMarkRange(Application_Links* app, View_ID view_id)
{
    if (abs_f32(global_last_cursor_rect.y0 - global_last_mark_rect.y0) > 1)
    {
        Rect_f32 view_rect = view_get_screen_rect(app, view_id);
        Rect_f32 clip = draw_set_clip(app, view_rect);
        
        f32 x = view_rect.x0 + 2;
        f32 width = 3;
        f32 lower_bound_y = Min(global_last_cursor_rect.y1, global_last_mark_rect.y1);
        f32 upper_bound_y = Max(global_last_cursor_rect.y0, global_last_mark_rect.y0);
        
        draw_rectangle(app, Rf32(x, lower_bound_y, x + width, upper_bound_y), 3.f,
                       Long_Color_Alpha(F4_ARGBFromID(active_color_table, defcolor_comment), 0.5f));
        draw_set_clip(app, clip);
    }
}

//~ NOTE(long): Tooltip Rendering

function Fancy_Block Long_Render_LayoutString(Application_Links* app, Arena* arena, String8 string,
                                              Face_ID face, f32 max_width, b32 newline)
{
    Fancy_Block result = {};
    Scratch_Block scratch(app, arena);
    
    String8 ws = S8Lit(" \n\r\t\f\v");
    String8List list = string_split(scratch, string, ws.str, (i32)ws.size);
    Fancy_Line* line = push_fancy_line(arena, &result);
    f32 space_size = get_string_advance(app, face, S8Lit(" "));
    
    for (String8Node* node = list.first; node; node = node->next)
    {
        b32 not_first = node != list.first;
        b32 was_newline = not_first && node->string.str[-1] == '\n';
        was_newline &= newline;
        
        f32 advance = get_string_advance(app, face, node->string);
        f32 line_width = get_fancy_line_width(app, face, line);
        
        if (was_newline || (advance + line_width + space_size > max_width && line_width))
            line = push_fancy_line(arena, &result);
        else if (not_first)
            push_fancy_string(arena, line, S8Lit(" "));
        
        push_fancy_string(arena, line, node->string);
    }
    
    return result;
}

// @COPYPASTA(long): F4_DrawTooltipRect
function void Long_Render_TooltipRect(Application_Links* app, Rect_f32 rect, f32 roundness)
{
    ARGB_Color background_color = fcolor_resolve(fcolor_id(defcolor_back));
    ARGB_Color border_color = fcolor_resolve(fcolor_id(defcolor_margin_active));
    
    background_color &= 0x00ffffff;
    background_color |= 0xd0000000;
    
    border_color &= 0x00ffffff;
    border_color |= 0xd0000000;
    
    draw_rectangle(app, rect, roundness, background_color);
    draw_rectangle_outline(app, rect, roundness, 3.f, border_color);
}

function void Long_Render_DrawPeek(Application_Links* app, Rect_f32 tooltip_rect, Rect_f32 peek_rect, Rect_f32 region,
                                   Buffer_ID buffer, Range_i64 range, i64 line_offset, f32 roundness)
{
    if (!buffer_exists(app, buffer))
        return;
    
    Rect_f32 prev_prev_clip = draw_set_clip(app, region);
    Long_Render_TooltipRect(app, tooltip_rect, roundness);
    
    i64 line = get_line_number_from_pos(app, buffer, range.min);
    i64 draw_line = line - Min(line_offset, line - 1);
    while (draw_line < line && line_is_blank(app, buffer, draw_line))
        draw_line++;
    
    range.min = get_line_start_pos(app, buffer, draw_line);
    i64 end_pos = get_line_end_pos(app, buffer, line);
    range.max = clamp_bot(range.max, end_pos);
    
    Text_Layout_ID layout = text_layout_create(app, buffer, peek_rect, { draw_line });
    
    Token_Array array = get_token_array_from_buffer(app, buffer);
    if(array.tokens != 0)
        Long_SyntaxHighlight(app, layout, &array);
    else
        paint_text_color_fcolor(app, layout, range, fcolor_id(defcolor_text_default));
    
    draw_line_highlight(app, layout, line, fcolor_id(defcolor_highlight_white));
    draw_text_layout_default(app, layout);
    
    draw_set_clip(app, prev_prev_clip);
    text_layout_free(app, layout);
}

function void Long_Render_DrawPeek(Application_Links* app, Rect_f32 rect, Buffer_ID buffer,
                                   Range_i64 range, i64 line_offset, f32 roundness)
{
    Long_Render_DrawPeek(app, rect, rect_inner(rect, 20), rect, buffer, range, line_offset, roundness);
}

function Rect_f32 Long_Render_DrawString(Application_Links* app, String8 string, Vec2_f32 tooltip_position, Rect_f32 region,
                                         Face_ID face, f32 line_height, f32 padding, ARGB_Color color, f32 roundness)
{
    Scratch_Block scratch(app);
    Fancy_Block block = Long_Render_LayoutString(app, scratch, string, face, rect_width(region) - 2*padding, 0);
    Vec2_f32 needed_size = get_fancy_block_dim(app, face, &block);
    
    Rect_f32 draw_rect =
    {
        tooltip_position.x,
        tooltip_position.y,
        tooltip_position.x + needed_size.x + 2*padding,
        tooltip_position.y + needed_size.y + 2*padding,
    };
    
#if 1
    f32 offset = clamp_bot(draw_rect.x1 - region.x1, 0);
    draw_rect.x0 -= offset;
    draw_rect.x1 -= offset;
#else
    Vec2_f32 offset = { clamp_bot(draw_rect.x1 - region.x1, 0), clamp_bot(draw_rect.y1 - region.y1, 0) };
    draw_rect.p0 -= offset;
    draw_rect.p1 -= offset;
#endif
    
    Long_Render_TooltipRect(app, draw_rect, roundness);
    draw_fancy_block(app, face, fcolor_argb(color), &block, draw_rect.p0 + Vec2_f32{ padding, padding });
    return draw_rect;
}

//~ NOTE(long): Render Hook

// @COPYPASTA(long): F4_RenderDividerComments
function void Long_Render_DividerComments(Application_Links* app, Buffer_ID buffer, Text_Layout_ID layout)
{
    if (!def_get_config_b32(vars_save_string_lit("f4_disable_divider_comments")))
    {
        ProfileScope(app, "[Long] Divider Comments");
        
        Token_Array array = get_token_array_from_buffer(app, buffer);
        Range_i64 visible_range = text_layout_get_visible_range(app, layout);
        Scratch_Block scratch(app);
        
        if (array.tokens)
        {
            i64 first_index = token_index_from_pos(&array, visible_range.first);
            Token_Iterator_Array it = token_iterator_index(0, &array, first_index);
            
            for(;;)
            {
                Token* token = token_it_read(&it);
                
                if (!token || token->pos >= visible_range.one_past_last || !token_it_inc_non_whitespace(&it))
                    break;
                
                if (token->kind == TokenBaseKind_Comment)
                {
                    Rect_f32 comment_first_char_rect = text_layout_character_on_screen(app, layout, token->pos);
                    Rect_f32 comment_last_char_rect = text_layout_character_on_screen(app, layout, token->pos+token->size-1);
                    String_Const_u8 token_string = push_buffer_range(app, scratch, buffer, Ii64(token));
                    String_Const_u8 signifier_substring = string_substring(token_string, Ii64(0, 3));
                    f32 roundness = 4.f;
                    
                    // NOTE(rjf): Strong dividers.
                    if (string_match(signifier_substring, S8Lit("//~")))
                    {
                        Rect_f32 rect =
                        {
                            comment_first_char_rect.x0, comment_first_char_rect.y0-2,
                            10000                     , comment_first_char_rect.y0,
                        };
                        draw_rectangle(app, rect, roundness, fcolor_resolve(fcolor_id(defcolor_comment)));
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
                        
                        for (i32 i = 0; i < 1000; i += 1)
                        {
                            draw_rectangle(app, rect, roundness, fcolor_resolve(fcolor_id(defcolor_comment)));
                            rect.x0 += dash_size*1.5f;
                            rect.x1 += dash_size*1.5f;
                        }
                    }
                }
            }
        }
    }
}

function void Long_Render_LineOffsetNumber(Application_Links* app, View_ID view, Buffer_ID buffer,
                                           Face_ID face, Text_Layout_ID layout, Rect_f32 margin)
{
    FColor line_color = fcolor_id(defcolor_line_numbers_text);
    Range_i64 visible_range = text_layout_get_visible_range(app, layout);
    
    Rect_f32 prev_clip = draw_set_clip(app, margin);
    draw_rectangle_fcolor(app, margin, 0.f, fcolor_id(defcolor_line_numbers_back));
    
    Buffer_Cursor cursor = view_compute_cursor(app, view, seek_pos(visible_range.first));
    Buffer_Cursor cursor_opl = view_compute_cursor(app, view, seek_pos(visible_range.one_past_last));
    i64 one_past_last_line_number = cursor_opl.line + 1;
    
    i64 current_line = get_line_number_from_pos(app, buffer, view_get_cursor_pos(app, view));
    i64 line_count = buffer_get_line_count(app, buffer);
    i64 digit_count = digit_count_from_integer(line_count, 10);
    
    for (i64 line_number = cursor.line; line_number < one_past_last_line_number && line_number < line_count; ++line_number)
    {
        Scratch_Block scratch(app);
        
        Range_f32 line_y = text_layout_line_on_screen(app, layout, line_number);
        Vec2_f32 p = V2f32(margin.x0, line_y.min);
        
        Fancy_String fstring = {};
        if (line_number == current_line)
            fstring.value = push_stringf(scratch, "%*lld", digit_count, 0);
        else
            fstring.value = push_stringf(scratch, "%+*lld", digit_count, line_number - current_line);
        draw_fancy_string(app, face, line_color, &fstring, p);
    }
    
    draw_set_clip(app, prev_clip);
}

CUSTOM_COMMAND_SIG(long_toggle_line_offset)
CUSTOM_DOC("Toggles between line numbers and offsets.")
{
    String_ID key = vars_save_string_lit("long_show_line_number_offset");
    b32 val = def_get_config_b32(key);
    def_set_config_b32(key, !val);
}

//~ NOTE(long): Highlight Rendering

// @COPYPASTA(long): qol_draw_compile_errors and F4_RenderErrorAnnotations
function void Long_Highlight_DrawErrors(Application_Links* app, Buffer_ID buffer, Text_Layout_ID layout, Buffer_ID jump_buffer)
{
    Marker_List* list = get_or_make_list_for_buffer(app, &global_heap, jump_buffer);
    if (!jump_buffer || !list)
        return;
    
    Scratch_Block scratch(app);
    Range_i64 visible_range = text_layout_get_visible_range(app, layout);
    
    Managed_Scope scopes[2];
    scopes[0] = buffer_get_managed_scope(app, jump_buffer);
    scopes[1] = buffer_get_managed_scope(app, buffer);
    Managed_Scope comp_scope = get_managed_scope_with_multiple_dependencies(app, scopes, ArrayCount(scopes));
    Managed_Object* markers_object = scope_attachment(app, comp_scope, sticky_jump_marker_handle, Managed_Object);
    
    i32 count = managed_object_get_item_count(app, *markers_object);
    Marker* markers = push_array(scratch, Marker, count);
    managed_object_load_data(app, *markers_object, 0, count, markers);
    
    for (i32 i = 0; i < count; i += 1)
    {
        i64 jump_line_number = 0, code_line_number = 0;
        Sticky_Jump_Stored stored = {};
        if (get_stored_jump_from_list(app, list, i, &stored))
        {
            jump_line_number = stored.list_line;
            code_line_number = get_line_number_from_pos(app, buffer, markers[stored.index_into_marker_array].pos);
        }
        
        Range_i64 line_range = get_line_pos_range(app, buffer, code_line_number);
        if (!range_overlap(visible_range, line_range)){ continue; }
        
        String_Const_u8 comp_line = push_buffer_line(app, scratch, jump_buffer, jump_line_number);
        Parsed_Jump jump = parse_jump_location(comp_line);
        if (!jump.success)
            continue;
        
        draw_line_highlight(app, layout, code_line_number, fcolor_id(defcolor_highlight_junk));
        
        String_Const_u8 error_string = string_skip(comp_line, jump.colon_position + 2);
        Rect_f32 end_rect = text_layout_character_on_screen(app, layout,
                                                            get_line_end_pos(app, buffer, code_line_number)-1);
        Face_ID face = global_small_code_face;
        Range_f32 y = text_layout_line_on_screen(app, layout, code_line_number);
        
        if (range_size(y) > 0.f)
        {
            Rect_f32 region = text_layout_region(app, layout);
            
#if 1
            Vec2_f32 draw_position = V2f32(end_rect.x1 + 40.f, end_rect.y0);
#else
            Face_Metrics metrics = get_face_metrics(app, face);
            Vec2_f32 draw_position =
            {
                region.x1 - metrics.max_advance*error_string.size - (y.max-y.min)/2 - metrics.line_height/2,
                y.min + (y.max-y.min)/2 - metrics.line_height/2,
            };
            draw_position.x = clamp_bot(draw_position.x, end_rect.x1 + 30);
#endif
            
            draw_string(app, face, error_string, draw_position, fcolor_id(fleury_color_error_annotation));
            
            Mouse_State mouse = get_mouse_state(app);
            if (mouse.x >= region.x0 && mouse.x <= region.x1 && mouse.y >= y.min && mouse.y <= y.max)
                F4_PushTooltip(error_string, 0xffff0000);
        }
    }
}

function b32 Long_Highlight_HasRange(Application_Links* app, View_ID view)
{
    Managed_Scope scope = view_get_managed_scope(app, view);
    Managed_Object* highlight = scope_attachment(app, scope, view_highlight_range, Managed_Object);
    return *highlight != 0;
}

// @COPYPASTA(long): draw_highlight_range
function b32 Long_Highlight_DrawRange(Application_Links* app, View_ID view_id, Buffer_ID buffer,
                                      Text_Layout_ID text_layout_id, f32 roundness)
{
    b32 has_highlight_range = 0;
    Managed_Scope scope = view_get_managed_scope(app, view_id);
    Buffer_ID* highlight_buffer = scope_attachment(app, scope, view_highlight_buffer, Buffer_ID);
    if (*highlight_buffer != 0)
    {
        if (*highlight_buffer != buffer)
            view_disable_highlight_range(app, view_id);
        else
        {
            has_highlight_range = 1;
            Managed_Object* highlight = scope_attachment(app, scope, view_highlight_range, Managed_Object);
            Marker marker_range[2];
            if (managed_object_load_data(app, *highlight, 0, 2, marker_range))
            {
                Range_i64 range = Ii64(marker_range[0].pos, marker_range[1].pos);
                Long_Render_DrawBlock(app, text_layout_id, range, roundness, fcolor_id(defcolor_highlight));
                paint_text_color_fcolor(app, text_layout_id, range, fcolor_id(defcolor_at_highlight));
            }
        }
    }
    return(has_highlight_range);
}

function void Long_MC_DrawHighlights(Application_Links* app, View_ID view, Buffer_ID buffer,
                                     Text_Layout_ID layout, f32 roundness, f32 thickness)
{
    if (def_get_config_b32(vars_save_string_lit("use_jump_highlight")))
        return;
    
    Buffer_ID search_buffer = Long_Buffer_GetSearchBuffer(app);
    if (search_buffer && string_match(locked_buffer, search_name))
    {
        Scratch_Block scratch(app);
        Managed_Scope scope = buffer_get_managed_scope(app, search_buffer);
        i32 count;
        Marker* markers = Long_SearchBuffer_GetMarkers(app, scratch, scope, buffer, &count);
        
        i64 cursor = view_get_cursor_pos(app, view);
        i64 mark = view_get_mark_pos(app, view);
        i64 size = cursor - mark;
        Assert(size >= 0);
        
        Range_i64 visible_range = text_layout_get_visible_range(app, layout);
        for (i32 i = 0; i < count; i += 1)
            if (range_contains(visible_range, markers[i].pos))
                Long_Render_DrawBlock(app, layout, Ii64_size(markers[i].pos, size), 0.f, fcolor_id(fleury_color_token_minor_highlight));
        
        if (view == mc_context.view)
        {
            for_mc(node, mc_context.cursors)
            {
                Range_i64 range = Ii64(node->cursor_pos, node->mark_pos);
                Long_Render_DrawBlock(app, layout, range, roundness, fcolor_change_alpha(fcolor_id(defcolor_highlight), .75f));
                paint_text_color_fcolor(app, layout, range, fcolor_change_alpha(fcolor_id(defcolor_at_highlight), .75f));
            }
        }
        
        if (buffer != search_buffer)
            Long_Render_CursorRect(app, view, buffer, layout, cursor, mark, roundness, thickness);
    }
}

// @COPYPASTA(long): draw_jump_highlights
function void Long_Highlight_DrawList(Application_Links* app, Buffer_ID buffer, Text_Layout_ID layout, f32 roundness, f32 thickness)
{
    if (def_get_config_b32(vars_save_string_lit("use_jump_highlight")))
        return;
    
    Buffer_ID search_buffer = Long_Buffer_GetSearchBuffer(app);
    if (search_buffer && string_match(locked_buffer, search_name))
    {
        Scratch_Block scratch(app);
        View_ID view = get_active_view(app, Access_Always);
        
        Managed_Scope scope = buffer_get_managed_scope(app, search_buffer);
        i64 size = *scope_attachment(app, scope, long_search_string_size, i64);
        i32 selection_offset = *scope_attachment(app, scope, long_start_selection_offset, i32);
        i64 pos_offset = *scope_attachment(app, scope, long_selection_pos_offset, i64);
        
        i32 count;
        Marker* markers = Long_SearchBuffer_GetMarkers(app, scratch, scope, buffer, &count);
        
        Range_i32 select_range;
        {
            Locked_Jump_State jump_state = get_locked_jump_state(app, &global_heap);
            select_range = Long_Highlight_GetRange(jump_state, selection_offset);
            
            Sticky_Jump_Stored current_jump = {};
            get_stored_jump_from_list(app, jump_state.list, jump_state.list_index, &current_jump);
            
            i32 offset = jump_state.list_index - current_jump.index_into_marker_array;
            select_range.min -= offset;
            select_range.max -= offset;
        }
        
        for (i32 i = 0; i < count; i += 1)
        {
            i64 marker_pos = markers[i].pos + pos_offset;
            Range_i64 range = Ii64_size(marker_pos, size);
            // NOTE(long): If the user only has one selection, the MultiSelect function already handles it
            if (range_contains_inclusive(select_range, i))
                Long_Render_CursorRect(app, view, buffer, layout, marker_pos + size, marker_pos, roundness, thickness);
            else
                Long_Render_DrawBlock(app, layout, range, 0.f, fcolor_id(fleury_color_token_minor_highlight));
        }
    }
}

//~ NOTE(long): Colors

// @COPYPASTA(long): qol_draw_hex_color
function void Long_Render_HexColor(Application_Links* app, View_ID view, Buffer_ID buffer, Text_Layout_ID layout)
{
    Scratch_Block scratch(app);
    Range_i64 visible_range = text_layout_get_visible_range(app, layout);
    String_Const_u8 buffer_string = push_buffer_range(app, scratch, buffer, visible_range);
    
    for (i64 i = 0; i+9 < range_size(visible_range); ++i)
    {
        u8* str = buffer_string.str+i;
        b32 s0 = str[0] != '0';
        b32 s1 = str[1] != 'x';
        if (s0 || s1)
            continue;
        
        b32 all_hex = 1;
        for (i64 j = 0; j < 8 && all_hex; ++j)
        {
            u8 c = str[j+2];
            b32 is_digit = '0' <= c && c <= '9';
            b32 is_lower = 'a' <= c && c <= 'f';
            b32 is_upper = 'A' <= c && c <= 'F';
            all_hex = is_digit || is_lower || is_upper;
        }
        if (!all_hex) continue;
        
        i64 pos = visible_range.min + i;
        Rect_f32 r0 = text_layout_character_on_screen(app, layout, pos+0);
        Rect_f32 r1 = text_layout_character_on_screen(app, layout, pos+9);
        Rect_f32 rect = rect_inner(rect_union(r0, r1), -1.f);
        
        ARGB_Color color = ARGB_Color(string_to_integer(SCu8(str+2, 8), 16));
        u32 sum = ((color >> 16) & 0xFF) + ((color >> 8) & 0xFF) + (color & 0xFF);
        ARGB_Color contrast = ARGB_Color(0xFF000000 | (i32(sum > 330)-1));
        
        draw_rectangle_outline(app, rect_inner(rect, -2.f), 10.f, 4.f, contrast);
        draw_rectangle(app, rect, 8.f, color);
        paint_text_color(app, layout, Ii64_size(pos, 10), contrast);
        i += 9;
    }
}

function ARGB_Color Long_Color_Alpha(ARGB_Color color, f32 alpha)
{
    Vec4_f32 rgba = unpack_color(color);
    rgba.a = alpha;
    return pack_color(rgba);
}

// @COPYPASTA(long): F4_GetColor
function ARGB_Color Long_GetColor(Application_Links* app, ColorCtx ctx)
{
    Color_Table table = active_color_table;
    ARGB_Color default_color = F4_ARGBFromID(table, defcolor_text_default);
    ARGB_Color color = default_color;
    f32 t = 1;
    
#define Long_FillAndSetColor(flag, id) do { \
        t = f4_syntax_flag_transitions[BitOffset(flag)]; \
        color = F4_ARGBFromID(table, id); \
    } while(0)
    
    // NOTE(rjf): Token Color
    if(ctx.token.size != 0)
    {
        Scratch_Block scratch(app);
        
        switch(ctx.token.kind)
        {
            case TokenBaseKind_Identifier:
            {
                Token_Array array = get_token_array_from_buffer(app, ctx.buffer);
                F4_Index_Note* note = Long_Index_LookupBestNote(app, ctx.buffer, &array, &ctx.token);
                
                if (note)
                {
                    color = 0xffff0000;
                    switch (note->kind)
                    {
                        case F4_Index_NoteKind_Type:     Long_FillAndSetColor(F4_SyntaxFlag_Types, (note->flags & F4_Index_NoteFlag_SumType) ?
                                                                              fleury_color_index_sum_type : fleury_color_index_product_type); break;
                        case F4_Index_NoteKind_Macro:    Long_FillAndSetColor(F4_SyntaxFlag_Macros,  fleury_color_index_macro);       break;
                        case F4_Index_NoteKind_Function: Long_FillAndSetColor(F4_SyntaxFlag_Functions,  fleury_color_index_function); break;
                        case F4_Index_NoteKind_Constant: Long_FillAndSetColor(F4_SyntaxFlag_Constants,  fleury_color_index_constant); break;
                        case F4_Index_NoteKind_Decl:     Long_FillAndSetColor(F4_SyntaxFlag_Constants,  fleury_color_index_decl);     break;
                        
                        default: color = 0xffff00ff; break;
                    }
                    
                    if (!F4_ARGBIsValid(color))
                        color = default_color;
                }
            } break;
            
            case TokenBaseKind_Preprocessor:   Long_FillAndSetColor(F4_SyntaxFlag_Preprocessor, defcolor_preproc);    break;
            case TokenBaseKind_Keyword:        Long_FillAndSetColor(F4_SyntaxFlag_Keywords, defcolor_keyword);        break;
            case TokenBaseKind_Comment:        color = F4_ARGBFromID(table, defcolor_comment);                        break;
            case TokenBaseKind_LiteralString:  Long_FillAndSetColor(F4_SyntaxFlag_Literals, defcolor_str_constant);   break;
            case TokenBaseKind_LiteralInteger: Long_FillAndSetColor(F4_SyntaxFlag_Literals, defcolor_int_constant);   break;
            case TokenBaseKind_LiteralFloat:   Long_FillAndSetColor(F4_SyntaxFlag_Literals, defcolor_float_constant); break;
            case TokenBaseKind_Operator:
            {
                Long_FillAndSetColor(F4_SyntaxFlag_Operators, fleury_color_operators);
                if (!F4_ARGBIsValid(color))
                    color = default_color;
            } break;
            
            case TokenBaseKind_ScopeOpen:
            case TokenBaseKind_ScopeClose:
            case TokenBaseKind_ParentheticalOpen:
            case TokenBaseKind_ParentheticalClose:
            case TokenBaseKind_StatementClose:
            {
                color = F4_ARGBFromID(table, fleury_color_syntax_crap);
                if (!F4_ARGBIsValid(color))
                    color = default_color;
            } break;
            
            default:
            {
                switch (ctx.token.sub_kind)
                {
                    case TokenCppKind_LiteralTrue:
                    case TokenCppKind_LiteralFalse: Long_FillAndSetColor(F4_SyntaxFlag_Literals, defcolor_bool_constant); break;
                    
                    case TokenCppKind_LiteralCharacter:
                    case TokenCppKind_LiteralCharacterWide:
                    case TokenCppKind_LiteralCharacterUTF8:
                    case TokenCppKind_LiteralCharacterUTF16:
                    case TokenCppKind_LiteralCharacterUTF32: Long_FillAndSetColor(F4_SyntaxFlag_Literals, defcolor_char_constant); break;
                    
                    case TokenCppKind_PPIncludeFile: Long_FillAndSetColor(F4_SyntaxFlag_Literals, defcolor_include); break;
                }
            } break;
        }
    }
    
    // NOTE(rjf): Cursor Color
    else
    {
        if (ctx.flags & ColorFlag_Macro)
            color = F4_ARGBFromID(table, fleury_color_cursor_macro);
        else
            color = F4_ARGBFromID(table, defcolor_cursor, ctx.mode);
    }
    
    return color_blend(default_color, t, color);
}

// @COPYPASTA(long): F4_SyntaxHighlight
function void Long_SyntaxHighlight(Application_Links* app, Text_Layout_ID text_layout_id, Token_Array* array)
{
    Color_Table table = active_color_table;
    Buffer_ID buffer = text_layout_get_buffer(app, text_layout_id);
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    i64 first_index = token_index_from_pos(array, visible_range.first);
    Token_Iterator_Array it = token_iterator_index(0, array, first_index);
    ARGB_Color comment_tag_color = F4_ARGBFromID(table, fleury_color_index_comment_tag, 0);
    
    for (;;)
    {
        Token* token = token_it_read(&it);
        if (!token || token->pos >= visible_range.one_past_last)
            break;
        
        ARGB_Color argb = Long_GetColor(app, ColorCtx_Token(*token, buffer));
        paint_text_color(app, text_layout_id, Ii64_size(token->pos, token->size), argb);
        
        // NOTE(rjf): Substrings from comments
        if (F4_ARGBIsValid(comment_tag_color))
        {
            if (token->kind == TokenBaseKind_Comment)
            {
                Scratch_Block scratch(app);
                String_Const_u8 string = push_buffer_range(app, scratch, buffer, Ii64(token->pos, token->pos + token->size));
                
                for (u64 i = 0; i < string.size; i += 1)
                {
                    if (string.str[i] == '@')
                    {
                        u64 j = i+1;
                        for (; j < string.size; j += 1)
                            if (character_is_whitespace(string.str[j]))
                                break;
                        paint_text_color(app, text_layout_id, Ii64(token->pos + (i64)i, token->pos + (i64)j), comment_tag_color);
                    }
                }
            }
        }
        
        if (!token_it_inc_all(&it))
            break;
    }
    
    F4_Language* lang = F4_LanguageFromBuffer(app, buffer);
    if (lang != 0 && lang->Highlight != 0)
        lang->Highlight(app, text_layout_id, array, table);
}

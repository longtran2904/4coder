
//~ NOTE(long): Helper Functions

//- NOTE(long): Misc
function b32 Long_F32_Invalid(f32 f)
{
    return f == max_f32 || f == min_f32 || f == -max_f32 || f == -min_f32;
}

function b32 Long_Rf32_Invalid(Rect_f32 r)
{
    return (Long_F32_Invalid(r.x0) || Long_F32_Invalid(r.x1) || Long_F32_Invalid(r.y0) || Long_F32_Invalid(r.y1));
}

function b32 Long_IsPosValid(Application_Links* app, View_ID view, Buffer_ID buffer, i64 pos, i64 current_pos)
{
    b32 result = pos != current_pos;
    if (current_pos > pos)
    {
        Scratch_Block scratch(app);
        Buffer_Cursor cursor = view_compute_cursor(app, view, seek_pos(pos));
        String8 string = push_buffer_range(app, scratch, buffer, Ii64(pos, current_pos));
        u64 non_whitespace = string_find_first_non_whitespace(string);
        if (non_whitespace == string.size)
            result = false;
    }
    return result;
}

// @COPYPASTA(long): center_view. This will snap the view to the target position instantly.
// If you want to snap the current view to the center, call center_view first, then call this function.
function void Long_SnapView(Application_Links* app)
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
    scroll.position = scroll.target;
    view_set_buffer_scroll(app, view, scroll, SetBufferScroll_SnapCursorIntoView);
    no_mark_snap_to_cursor(app, view);
}

function i32 Long_Abs(i32 num)
{
    i32 mask = num >> 31;
    return (num ^ mask) - mask;
}

function i64 Long_Abs(i64 num)
{
    i32 mask = num >> 63;
    return (num ^ mask) - mask;
}

//- NOTE(long): Jumping
function b32 Long_Jump_ParseLocation(Application_Links* app, View_ID view, Buffer_ID buffer, Buffer_ID* out_buf, i64* out_pos)
{
    b32 result = 0;
    Scratch_Block scratch(app);
    
    Buffer_ID _buf;
    i64 _pos;
    if (!out_buf) out_buf = &_buf;
    if (!out_pos) out_pos = &_pos;
    
    i64 pos = view_get_cursor_pos(app, view);
    String8 line = push_buffer_line(app, scratch, buffer, get_line_number_from_pos(app, buffer, pos));
    String8 lexeme = {};
    {
        Token* token = get_token_from_pos(app, buffer, pos);
        if (token)
        {
            lexeme = push_token_lexeme(app, scratch, buffer, token);
            if (token->kind == TokenBaseKind_Comment)
                lexeme = string_skip(lexeme, 2);
            else if (token->kind == TokenBaseKind_LiteralString)
                lexeme = string_skip(lexeme, 1);
        }
    }
    
    Parsed_Jump jump = parse_jump_location(line, 0);
    if (!jump.success || !get_jump_buffer(app, 0, &jump.location))
        jump = parse_jump_location(lexeme, 0);
    
    if (jump.success && get_jump_buffer(app, out_buf, &jump.location))
    {
        *out_pos = view_compute_cursor(app, view, seek_line_col(jump.location.line, jump.location.column)).pos;
        result = 1;
    }
    
    return result;
}

function void Long_PointStack_Push(Application_Links* app, Buffer_ID buffer, i64 pos, View_ID view);

function void Long_Jump_ToLocation(Application_Links* app, View_ID view, Buffer_ID target_buffer, i64 target_pos,
                                   Buffer_ID current_buffer = 0, i64 current_pos = 0)
{
    if (!current_buffer)
        current_buffer = view_get_buffer(app, view, Access_Always);
    if (!current_pos)
        current_pos = view_get_cursor_pos(app, view);
    
    Long_PointStack_Push(app, current_buffer, current_pos, view);
    F4_JumpToLocation(app, view, target_buffer, target_pos);
    Long_PointStack_Push(app, target_buffer, target_pos, view);
}

function void Long_Jump_ToBuffer(Application_Links* app, View_ID view, Buffer_ID buffer, b32 push_src = 1, b32 push_dst = 1)
{
    if (push_src)
        Long_PointStack_Push(app, view_get_buffer(app, view, Access_Always), view_get_cursor_pos(app, view), view);
    view_set_buffer(app, view, buffer, 0);
    if (push_dst)
        Long_PointStack_Push(app, buffer, view_get_cursor_pos(app, view), view);
}

//- NOTE(long): Buffer
function Buffer_ID Long_Buffer_GetSearchBuffer(Application_Links* app)
{
    return get_buffer_by_name(app, search_name, Access_Always);
}

function b32 Long_IsSearchBuffer(Application_Links* app, Buffer_ID buffer)
{
    Scratch_Block scratch(app);
    String8 name = push_buffer_base_name(app, scratch, buffer);
    return string_match(name, search_name);
}

function String8 Long_Buffer_PushLine(Application_Links* app, Arena* arena, Buffer_ID buffer, i64 pos)
{
    Buffer_Cursor cursor = buffer_compute_cursor(app, buffer, seek_pos(pos));
    String_Const_u8 line = push_buffer_line(app, arena, buffer, cursor.line);
    return line;
}

function String8 Long_Buffer_NameNoProjPath(Application_Links* app, Arena* arena, Buffer_ID buffer)
{
    Scratch_Block scratch(app, arena);
    String8 filepath = push_buffer_file_name(app, scratch, buffer);
    {
        String8 buffer_name = push_buffer_base_name(app, scratch, buffer);
        filepath = string_chop(filepath, buffer_name.size);
    }
    
    Variable_Handle prj_var = vars_read_key(vars_get_root(), vars_save_string_lit("prj_config"));
    b32 has_prj_path = false;
    if (filepath.size)
    {
        String8 prj_dir = prj_path_from_project(scratch, prj_var);
        if (prj_dir.size)
        {
            // NOTE(long): prj_dir always has a slash at the end
            prj_dir = string_mod_replace_character(prj_dir, '/', '\\');
            if (string_match(string_prefix(filepath, prj_dir.size), prj_dir))
            {
                filepath = string_skip(filepath, prj_dir.size);
                has_prj_path = true;
            }
        }
    }
    
    if (filepath.size && !has_prj_path)
    {
        Variable_Handle reference_path_var = vars_read_key(prj_var, vars_save_string_lit("reference_paths"));
        i32 i = 0;
        b32 multi_paths = !vars_is_nil(vars_next_sibling(vars_first_child(reference_path_var)));
        for (Vars_Children(path_var, reference_path_var), ++i)
        {
            String8 ref_dir = vars_string_from_var(scratch, path_var);
            if (ref_dir.size)
            {
                ref_dir = string_mod_replace_character(ref_dir, '/', '\\');
                if (ref_dir.str[ref_dir.size - 1] == '\\')
                    ref_dir.size--;
                if (string_match(string_prefix(filepath, ref_dir.size), ref_dir))
                {
                    filepath = string_skip(filepath, ref_dir.size);
                    String8 path_index = multi_paths ? push_stringf(scratch, ":%d", i) : String8{};
                    filepath = push_stringf(scratch, "REFPATH%.*s%.*s", string_expand(path_index), string_expand(filepath));
                    break;
                }
            }
        }
    }
    
    if (filepath.size)
    {
        if (filepath.str[filepath.size - 1] == '\\')
            filepath.size--;
    }
    
    String8 result = {};
    if (filepath.size)
        result = push_string_copy(arena, filepath);
    return result;
}

//- NOTE(long): Render
function void Long_Render_DrawBlock(Application_Links* app, Text_Layout_ID layout, Range_i64 range, f32 roundness, FColor color)
{
    for (i64 i = range.first; i < range.one_past_last; ++i)
        if (Long_Rf32_Invalid(text_layout_character_on_screen(app, layout, i)))
            return;
    draw_character_block(app, layout, range, roundness, color);
}

// @COPYPASTA(long): F4_Cursor_RenderNotepadStyle
function void Long_Render_NotepadCursor(Application_Links *app, View_ID view_id, Buffer_ID buffer, Text_Layout_ID text_layout_id,
                                        i64 cursor_pos, i64 mark_pos, f32 roundness, f32 outline_thickness)
{
    Rect_f32 view_rect = view_get_screen_rect(app, view_id);
    
    if (cursor_pos != mark_pos)
    {
        Range_i64 range = Ii64(cursor_pos, mark_pos);
        Long_Render_DrawBlock(app, text_layout_id, range, roundness, fcolor_id(defcolor_highlight));
        paint_text_color_fcolor(app, text_layout_id, range, fcolor_id(defcolor_at_highlight));
    }
    
    // NOTE(rjf): Draw cursor
    {
        ARGB_Color cursor_color = F4_GetColor(app, ColorCtx_Cursor(0, GlobalKeybindingMode));
        ARGB_Color ghost_color = fcolor_resolve(fcolor_change_alpha(fcolor_argb(cursor_color), 0.5f));
        Rect_f32 rect = text_layout_character_on_screen(app, text_layout_id, cursor_pos);
        rect.x1 = rect.x0 + outline_thickness;
        if(rect.x0 < view_rect.x0)
        {
            rect.x0 = view_rect.x0;
            rect.x1 = view_rect.x0 + outline_thickness;
        }
        
        draw_rectangle(app, rect, roundness, cursor_color);
    }
}

// @COPYPASTA(long): F4_DrawTooltipRect
function void Long_Render_TooltipRect(Application_Links *app, Rect_f32 rect, f32 roundness)
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
        F4_SyntaxHighlight(app, layout, &array);
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

function void Long_Render_LineOffsetNumber(Application_Links *app, View_ID view_id, Buffer_ID buffer,
                                           Face_ID face_id, Text_Layout_ID text_layout_id, Rect_f32 margin)
{
    FColor line_color = fcolor_id(defcolor_line_numbers_text);
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    
    Rect_f32 prev_clip = draw_set_clip(app, margin);
    draw_rectangle_fcolor(app, margin, 0.f, fcolor_id(defcolor_line_numbers_back));
    
    Buffer_Cursor cursor = view_compute_cursor(app, view_id, seek_pos(visible_range.first));
    Buffer_Cursor cursor_opl = view_compute_cursor(app, view_id, seek_pos(visible_range.one_past_last));
    i64 one_past_last_line_number = cursor_opl.line + 1;
    
    i64 current_line = get_line_number_from_pos(app, buffer, view_get_cursor_pos(app, view_id));
    i64 line_count = buffer_get_line_count(app, buffer);
    i64 digit_count = digit_count_from_integer(line_count, 10);
    
    for (i64 line_number = cursor.line; line_number < one_past_last_line_number && line_number < line_count; ++line_number)
    {
        Scratch_Block scratch(app);
        
        Range_f32 line_y = text_layout_line_on_screen(app, text_layout_id, line_number);
        Vec2_f32 p = V2f32(margin.x0, line_y.min);
        
        Fancy_String fstring = {};
        if (line_number == current_line)
            fstring.value = push_stringf(scratch, "%*lld", digit_count, 0);
        else
            fstring.value = push_stringf(scratch, "%+*lld", digit_count, line_number - current_line);
        draw_fancy_string(app, face_id, line_color, &fstring, p);
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

//~ NOTE(long): Point Stack Commands

struct Long_Point_Stack
{
    Managed_Object markers[POINT_STACK_DEPTH];
    Buffer_ID buffers[POINT_STACK_DEPTH];
    i32 bot;
    
    // NOTE(long): The current and top is one past the actual point index
    i32 current;
    i32 top;
};

CUSTOM_ID(attachment, long_point_stacks);

function Long_Point_Stack* Long_GetPointStack(Application_Links* app, View_ID view = 0)
{
    if (view == 0)
        view = get_active_view(app, Access_Always);
    return scope_attachment(app, view_get_managed_scope(app, view), long_point_stacks, Long_Point_Stack);
}

function void Long_PointStack_Read(Application_Links* app, Long_Point_Stack* stack, i32 index, Buffer_ID* out_buffer, i64* out_pos)
{
    Marker* marker = (Marker*)managed_object_get_pointer(app, stack->markers[index]);
    if (marker)
    {
        *out_buffer = stack->buffers[index];
        *out_pos = marker->pos;
    }
}

function void Long_PointStack_Push(Application_Links* app, Buffer_ID buffer, i64 pos, View_ID view = 0)
{
    Long_Point_Stack* stack = Long_GetPointStack(app, view);
    i32 size = ArrayCount(stack->markers);
    stack->top = stack->current;
    
    // NOTE(long): Only push a point if it isn't the same as the current one
    if (stack->bot != stack->top)
    {
        Buffer_ID top_buffer = 0; i64 top_pos = 0;
        Long_PointStack_Read(app, stack, clamp_loop(stack->top - 1, size), &top_buffer, &top_pos);
        if (buffer == top_buffer && pos == top_pos)
            return;
    }
    
    // NOTE(long): Free and assign the new item to the top (current) position
    {
        i32 index = stack->top;
        stack->buffers[index] = buffer;
        
        Managed_Object object = alloc_buffer_markers_on_buffer(app, buffer, 1, 0);
        Marker* marker = (Marker*)managed_object_get_pointer(app, object);
        marker->pos = pos;
        marker->lean_right = false;
        
        managed_object_free(app, stack->markers[index]);
        stack->markers[index] = object;
    }
    
    // NOTE(long): Increase the top (and bot if need to)
    i32 next_top = clamp_loop(stack->top + 1, size);
    stack->top = next_top;
    stack->current = next_top;
    if (next_top == stack->bot)
        stack->bot = clamp_loop(stack->bot + 1, size);
}

function void Long_PointStack_SetCurrent(Long_Point_Stack* stack, i32 index)
{
    stack->current = clamp_loop(index + 1, ArrayCount(stack->markers));
}

function void Long_PointStack_Append(Application_Links* app, Long_Point_Stack* stack, Buffer_ID buffer, i64 pos, View_ID view = 0)
{
    Long_PointStack_SetCurrent(stack, stack->top - 1);
    Long_PointStack_Push(app, buffer, pos, view);
}

#define Long_IteratePointStack(app, stack, start, end, advance, size, func) \
    for (i32 i = start; i != end; i = clamp_loop(i + (advance), size)) \
    { \
        Buffer_ID buffer = 0; \
        i64 pos = 0; \
        Long_PointStack_Read(app, stack, i, &buffer, &pos); \
        if (!buffer && !pos) continue; \
        func; \
    }

CUSTOM_UI_COMMAND_SIG(long_point_lister)
CUSTOM_DOC("List jump history")
{
    Scratch_Block scratch(app);
    Lister_Block lister(app, scratch);
    lister_set_default_handlers(lister);
    
    Long_Point_Stack* stack = Long_GetPointStack(app);
    i32 size = ArrayCount(stack->markers);
    
#define MAX_LISTER_NAME_SIZE 80
    Long_IteratePointStack(app, stack, stack->bot, stack->top, 1, size,
                           {
                               String8 line = Long_Buffer_PushLine(app, scratch, buffer, pos);
                               line = string_condense_whitespace(scratch, line);
                               line = string_skip_chop_whitespace(line);
                               String8 tag = (i == stack->current - 1) ? S8Lit("current") : String8{};
                               Long_Lister_AddItem(app, lister, line, tag, buffer, pos, i);
                           });
    
    lister_set_query(lister, push_u8_stringf(scratch, "Top: %d, Bot: %d, Current: %d", stack->top, stack->bot, stack->current));
    Lister_Result l_result = run_lister(app, lister);
    
    Long_Lister_Data result = {};
    if (Long_Lister_IsResultValid(l_result))
        block_copy_struct(&result, (Long_Lister_Data*)l_result.user_data);
    
    if (result.buffer != 0)
    {
        F4_JumpToLocation(app, get_this_ctx_view(app, Access_Always), result.buffer, result.pos);
        Long_PointStack_SetCurrent(stack, (i32)result.user_index);
    }
}

function void Long_PointStack_JumpNext(Application_Links* app, View_ID view, i32 advance, b32 exit_if_current = 0)
{
    if (!view)
        view = get_active_view(app, Access_Always);
    Long_Point_Stack* stack = Long_GetPointStack(app, view);
    if (stack->top == stack->bot)
        return;
    
    i32 size = ArrayCount(stack->markers);
    Buffer_ID current_buffer = view_get_buffer(app, view, Access_Always);
    i64 current_pos = view_get_cursor_pos(app, view);
    
    i32 end = 0;
    if (advance == 0)
    {
        end = stack->bot - 1;
        advance = -1;
    }
    else if (advance > 0) end = stack->top;
    else if (advance < 0) end = stack->bot - 1;
    
    Long_IteratePointStack(app, stack, clamp_loop(stack->current - 1, size), clamp_loop(end, size), advance, size,
                           // NOTE(long): If virtual whitespace is enabled, we can't jump to the start of the line
                           // Long_IsPosValid will check for that and make sure pos is different from the current pos
                           if (buffer == current_buffer && pos == current_pos && exit_if_current) break;
                           
                           if (buffer != current_buffer || Long_IsPosValid(app, view, buffer, pos, current_pos))
                           {
                               if (clamp_loop(i + 1, size) == stack->top && advance < 0)
                               {
                                   i64 current_line = get_line_number_from_pos(app, current_buffer, current_pos);
                                   i64 line = get_line_number_from_pos(app, buffer, pos);
                                   if (current_buffer != buffer || Long_Abs((i32)(line - current_line)) >= 16)
                                       Long_PointStack_Append(app, stack, current_buffer, current_pos, view);
                               }
                               
                               F4_JumpToLocation(app, view, buffer, pos);
                               Long_PointStack_SetCurrent(stack, i);
                               break;
                           });
}

CUSTOM_COMMAND_SIG(long_undo_jump)
CUSTOM_DOC("Read from the current point stack and jump there; if already there go to the previous option")
{
    Long_PointStack_JumpNext(app, 0, -1);
}

CUSTOM_COMMAND_SIG(long_redo_jump)
CUSTOM_DOC("Read from the current point stack and jump there; if already there go to the next option")
{
    Long_PointStack_JumpNext(app, 0, +1);
}

CUSTOM_COMMAND_SIG(long_push_new_jump)
CUSTOM_DOC("Push the current position to the point stack; if the stack's current is the position then ignore it")
{
    View_ID view = get_active_view(app, Access_Always);
    Long_PointStack_Push(app, view_get_buffer(app, view, Access_Always), view_get_cursor_pos(app, view), view);
}

//~ NOTE(long): Buffer Commands

//- NOTE(long): Lister
#define OUTPUT_BUFFER_HEADER 0

function void Long_Buffer_OutputBuffer(Application_Links *app, Lister *lister, Buffer_ID buffer)
{
    Dirty_State dirty = buffer_get_dirty_state(app, buffer);
    String8 status = {};
    switch (dirty)
    {
        case DirtyState_UnsavedChanges:                   status = string_u8_litexpr("*");  break;
        case DirtyState_UnloadedChanges:                  status = string_u8_litexpr("!");  break;
        case DirtyState_UnsavedChangesAndUnloadedChanges: status = string_u8_litexpr("*!"); break;
    }
    Scratch_Block scratch(app, lister->arena);
    String8 buffer_name = push_buffer_base_name(app, scratch, buffer);
#if OUTPUT_BUFFER_HEADER
    Long_Lister_AddBuffer(app, lister, buffer_name, status, buffer);
#else
    String8 filepath = Long_Buffer_NameNoProjPath(app, scratch, buffer);
    if (filepath.size)
        status = push_stringf(scratch, "<%.*s>%s%.*s", string_expand(filepath), dirty ? " " : "", string_expand(status));
    lister_add_item(lister, buffer_name, status, IntAsPtr(buffer), 0);
#endif
}

// @COPYPASTA(long): generate_all_buffers_list
function void Long_Buffer_GenerateLists(Application_Links* app, Lister* lister)
{
    lister_begin_new_item_set(app, lister);
    
    Buffer_ID viewed_buffers[16];
    i32 viewed_buffer_count = 0;
    
    // List currently viewed buffers
    for (View_ID view = get_view_next(app, 0, Access_Always); view != 0; view = get_view_next(app, view, Access_Always))
    {
        Buffer_ID new_buffer_id = view_get_buffer(app, view, Access_Always);
        for (i32 i = 0; i < viewed_buffer_count; i += 1)
            if (new_buffer_id == viewed_buffers[i])
                goto skip0;
        viewed_buffers[viewed_buffer_count++] = new_buffer_id;
        skip0:;
    }
    
    // Regular Buffers
    for (Buffer_ID buffer = get_buffer_next(app, 0, Access_Always); buffer != 0; buffer = get_buffer_next(app, buffer, Access_Always))
    {
        for (i32 i = 0; i < viewed_buffer_count; i += 1)
            if (buffer == viewed_buffers[i])
                goto skip1;
        
        if (!buffer_has_name_with_star(app, buffer))
            Long_Buffer_OutputBuffer(app, lister, buffer);
        skip1:;
    }
    
    // Buffers Starting with *
    for (Buffer_ID buffer = get_buffer_next(app, 0, Access_Always); buffer != 0; buffer = get_buffer_next(app, buffer, Access_Always))
    {
        for (i32 i = 0; i < viewed_buffer_count; i += 1)
            if (buffer == viewed_buffers[i])
                goto skip2;
        
        if (buffer_has_name_with_star(app, buffer))
            Long_Buffer_OutputBuffer(app, lister, buffer);
        skip2:;
    }
    
    // Buffers That Are Open in Views
    for (i32 i = 0; i < viewed_buffer_count; i += 1)
        Long_Buffer_OutputBuffer(app, lister, viewed_buffers[i]);
}

function Buffer_ID Long_Buffer_RunLister(Application_Links *app, char* query)
{
    Lister_Handlers handlers = lister_get_default_handlers();
    handlers.refresh = Long_Buffer_GenerateLists;
    Lister_Result l_result = run_lister_with_refresh_handler(app, SCu8(query), handlers);
    Buffer_ID result = 0;
    if (Long_Lister_IsResultValid(l_result))
#if OUTPUT_BUFFER_HEADER
	result = ((Long_Lister_Data*)l_result.user_data)->buffer;
#else
	result = (Buffer_ID)(PtrAsInt(l_result.user_data));
#endif
    return result;
}

CUSTOM_UI_COMMAND_SIG(long_jump_lister)
CUSTOM_DOC("When executed on a buffer with jumps, creates a persistent lister for all the jumps")
{
    View_ID view = get_active_view(app, Access_Always);
    Buffer_ID jump_buffer = Long_Buffer_GetSearchBuffer(app);
    Marker_List* list = get_or_make_list_for_buffer(app, &global_heap, jump_buffer);
    
    if (list != 0)
    {
        Jump_Lister_Result jump = {};
        
        Scratch_Block scratch(app);
        Lister_Block lister(app, scratch);
        lister_set_query(lister, "Jump:");
        lister_set_default_handlers(lister);
        
        for (i32 i = 0; i < list->jump_count; i += 1)
        {
            ID_Pos_Jump_Location location = {};
            if (get_jump_from_list(app, list, i, &location))
            {
                Buffer_ID buffer = location.buffer_id;
                i64 pos = location.pos;
                String_Const_u8 line = Long_Buffer_PushLine(app, scratch, buffer, pos);
                line = string_condense_whitespace(scratch, line);
                line = string_skip_chop_whitespace(line);
                Long_Lister_AddItem(app, lister, line, {}, buffer, pos, i);
            }
        }
        
        Lister_Result l_result = run_lister(app, lister);
        if (Long_Lister_IsResultValid(l_result))
        {
            jump.success = true;
            jump.index = (i32)((Long_Lister_Data*)l_result.user_data)->user_index;
        }
        
        jump_to_jump_lister_result(app, view, list, &jump);
    }
}

// @COPYPASTA(long): f4_recent_files_menu
CUSTOM_UI_COMMAND_SIG(long_recent_files_menu)
CUSTOM_DOC("Lists the recent files used in the current panel.")
{
    View_ID view = get_active_view(app, Access_Read);
    Managed_Scope scope = view_get_managed_scope(app, view);
    F4_RecentFiles_ViewState *state = scope_attachment(app, scope, f4_recentfiles_viewstate, F4_RecentFiles_ViewState);
    
    if (state != 0)
    {
        Scratch_Block scratch(app);
        Lister_Block lister(app, scratch);
        lister_set_query(lister, "Recent Buffers:");
        lister_set_default_handlers(lister);
        
        for (int i = 1; i < state->recent_buffer_count; ++i)
        {
            Buffer_ID buffer = state->recent_buffers[i];
            if (buffer_exists(app, buffer))
            {
                String_Const_u8 buffer_name = push_buffer_unique_name(app, scratch, buffer);
                lister_add_item(lister, buffer_name, S8Lit(""), IntAsPtr(buffer), 0);
            }
        }
        
        Lister_Result l_result = run_lister(app, lister);
        if (Long_Lister_IsResultValid(l_result))
            view_set_buffer(app, view, (Buffer_ID)PtrAsInt(l_result.user_data), 0);
    }
}

CUSTOM_UI_COMMAND_SIG(long_interactive_switch_buffer)
CUSTOM_DOC("Interactively switch to an open buffer.")
{
    // @COPYPASTA(long): interactive_switch_buffer
    Buffer_ID buffer = Long_Buffer_RunLister(app, "Switch:");
    if (buffer != 0)
    {
        View_ID view = get_this_ctx_view(app, Access_Always);
        Long_Jump_ToBuffer(app, view, buffer);
    }
}

function void Long_KillBuffer(Application_Links* app, Buffer_ID buffer, View_ID view)
{
    // `buffer_view` is the view that has the buffer while `view` is the view that shows the kill options
    View_ID buffer_view = get_first_view_with_buffer(app, buffer);
    // must do this check before killing the buffer
    b32 is_search_buffer = Long_IsSearchBuffer(app, buffer);
    // killed always equals to false because the search buffer is never dirty and always killable
    b32 killed = try_buffer_kill(app, buffer, view, 0) == BufferKillResult_Killed;
    
    if (is_search_buffer && killed && buffer_view)
    {
        Long_PointStack_JumpNext(app, buffer_view, 0, 1);
        Long_SnapView(app);
        view_set_active(app, view);
    }
}

CUSTOM_UI_COMMAND_SIG(long_interactive_kill_buffer)
CUSTOM_DOC("Interactively kill an open buffer.")
{
    Long_KillBuffer(app, Long_Buffer_RunLister(app, "Kill:"), get_this_ctx_view(app, Access_Always));
}

//- NOTE(long): History
struct Long_Buffer_History
{
    History_Record_Index index;
    Record_Info record;
};

CUSTOM_ID(attachment, long_buffer_history);

function Long_Buffer_History* Long_Buffer_GetAttachedHistory(Application_Links* app, Buffer_ID buffer)
{
    return scope_attachment(app, buffer_get_managed_scope(app, buffer), long_buffer_history, Long_Buffer_History);
}

function Long_Buffer_History Long_Buffer_GetCurrentHistory(Application_Links* app, Buffer_ID buffer, i32 offset = 0)
{
    History_Record_Index current = buffer_history_get_current_state_index(app, buffer);
    Record_Info record = buffer_history_get_record_info(app, buffer, current + offset);
    return { current, record };
}

function b32 Long_Buffer_CompareCurrentHistory(Application_Links* app, Buffer_ID buffer, i32 offset = 0)
{
    Long_Buffer_History current = Long_Buffer_GetCurrentHistory(app, buffer, offset);
    Long_Buffer_History history = *Long_Buffer_GetAttachedHistory(app, buffer);
    b32 result = current.index == history.index && current.record.edit_number == history.record.edit_number;
    return result;
}

function b32 Long_Buffer_CheckHistoryAndSetDirty(Application_Links* app, Buffer_ID buffer = 0, i32 offset = 0)
{
    b32 result = 0;
    if (!buffer)
        buffer = view_get_buffer(app, get_active_view(app, Access_ReadWriteVisible), Access_ReadWriteVisible);
    
    if (buffer)
    {
        Dirty_State dirty = buffer_get_dirty_state(app, buffer);
        result = Long_Buffer_CompareCurrentHistory(app, buffer, offset);
        if (result)
            buffer_set_dirty_state(app, buffer, RemFlag(dirty, DirtyState_UnsavedChanges));
    }
    
    return result;
}

CUSTOM_UI_COMMAND_SIG(long_history_lister)
CUSTOM_DOC("Opens an interactive list of the current buffer history.")
{
    Scratch_Block scratch(app);
    Lister_Block lister(app, scratch);
    lister_set_default_handlers(lister);
    
    View_ID view = get_active_view(app, Access_Always);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    
    History_Record_Index current = buffer_history_get_current_state_index(app, buffer);
    History_Record_Index max     = buffer_history_get_max_record_index(app, buffer);
    Long_Buffer_History  history = *Long_Buffer_GetAttachedHistory(app, buffer);
    
    for (History_Record_Index i = 0; i <= max; ++i)
    {
        Record_Info record = buffer_history_get_record_info(app, buffer, i);
        String8 line = push_stringf(scratch, "[%d, %d]: \"%.*s\" \"%.*s\"", i, record.edit_number,
                                    string_expand(record.single_string_backward), string_expand(record.single_string_forward));
        
        String8 tag = {};
        {
            b32 isCurrent = i == current;
            b32 isSaved = record.edit_number == history.record.edit_number && i == history.index;
            if (isCurrent && isSaved)
                tag = S8Lit("current saved");
            else if (isCurrent)
                tag = S8Lit("current");
            else if (isSaved)
                tag = S8Lit("saved");
        }
        
        Long_Lister_AddItem(app, lister, line, tag, buffer, record.pos_before_edit, i);
    }
    
    lister_set_query(lister, push_u8_stringf(scratch, "Max: %d, Current: %d, Saved: (%d, %d)",
                                             max, current, history.index, history.record.edit_number));
    
    Lister_Result l_result = run_lister(app, lister);
    if (Long_Lister_IsResultValid(l_result))
    {
        History_Record_Index index = (History_Record_Index)((Long_Lister_Data*)l_result.user_data)->user_index;
        Assert(index >= 0 && index <= max);
        if (index != current)
        {
            buffer_history_set_current_state_index(app, buffer, index);
            i64 new_pos;
            if (index < current)
                new_pos = record_get_new_cursor_position_undo(app, buffer, index);
            else
                new_pos = record_get_new_cursor_position_redo(app, buffer, index);
            view_set_cursor_and_preferred_x(app, view, seek_pos(new_pos));
            
            Long_Buffer_CheckHistoryAndSetDirty(app, buffer);
        }
    }
}

//- @COPYPASTA(long): undo__fade_finish, undo, redo, undo_all_buffers, redo_all_buffers
// Because an undo can be deferred later, I can't call CheckHistory after doing an undo.
// I must call it inside the fade_finish callback, or in the do_immedite_undo check.
// But a redo, on the other hand, always executes immediately, so I can safely call it right afterward.
// I also fixed the "getting the wrong Record_Info" bug in the redo command

function void Long_Undo_FinishFade(Application_Links *app, Fade_Range* range)
{
    History_Record_Index current = buffer_history_get_current_state_index(app, range->buffer_id);
    if (current > 0)
    {
        buffer_history_set_current_state_index(app, range->buffer_id, current - 1);
        Long_Buffer_CheckHistoryAndSetDirty(app, range->buffer_id);
    }
}

CUSTOM_COMMAND_SIG(long_undo)
CUSTOM_DOC("Advances backwards through the undo history of the current buffer.")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    undo__flush_fades(app, buffer);
    
    History_Record_Index current = buffer_history_get_current_state_index(app, buffer);
    if (current > 0){
        Record_Info record = buffer_history_get_record_info(app, buffer, current);
        i64 new_position = record_get_new_cursor_position_undo(app, buffer, current, record);
        
        b32 do_immedite_undo = true;
        f32 undo_fade_time = 0.33f;
        b32 enable_undo_fade_out = def_get_config_b32(vars_save_string_lit("enable_undo_fade_out"));
        if (enable_undo_fade_out &&
            undo_fade_time > 0.f &&
            record.kind == RecordKind_Single &&
            record.single_string_backward.size == 0){
            b32 has_hard_character = false;
            for (u64 i = 0; i < record.single_string_forward.size; i += 1){
                if (!character_is_whitespace(record.single_string_forward.str[i])){
                    has_hard_character = true;
                    break;
                }
            }
            if (has_hard_character){
                Range_i64 range = Ii64_size(record.single_first, record.single_string_forward.size);
                ARGB_Color color = fcolor_resolve(fcolor_id(defcolor_undo)) & 0xFFFFFF;
                Fade_Range *fade = buffer_post_fade(app, buffer, undo_fade_time, range, color);
                fade->negate_fade_direction = true;
                fade->finish_call = Long_Undo_FinishFade;
                do_immedite_undo = false;
                if (new_position > range.max){
                    new_position -= range_size(range);
                }
            }
        }
        
        if (do_immedite_undo){
            buffer_history_set_current_state_index(app, buffer, current - 1);
            Long_Buffer_CheckHistoryAndSetDirty(app, buffer);
            if (record.single_string_backward.size > 0){
                Range_i64 range = Ii64_size(record.single_first, record.single_string_backward.size);
                ARGB_Color color = fcolor_resolve(fcolor_id(defcolor_undo));
                buffer_post_fade(app, buffer, undo_fade_time, range, color);
            }
        }
        
        view_set_cursor_and_preferred_x(app, view, seek_pos(new_position));
    }
}

CUSTOM_COMMAND_SIG(long_redo)
CUSTOM_DOC("Advances forwards through the undo history of the current buffer.")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    undo__flush_fades(app, buffer);
    
    History_Record_Index current = buffer_history_get_current_state_index(app, buffer);
    History_Record_Index max_index = buffer_history_get_max_record_index(app, buffer);
    if (current < max_index){
        Record_Info record = buffer_history_get_record_info(app, buffer, current + 1);
        i64 new_position = record_get_new_cursor_position_redo(app, buffer, current + 1, record);
        
        buffer_history_set_current_state_index(app, buffer, current + 1);
        
        if (record.single_string_forward.size > 0){
            Range_i64 range = Ii64_size(record.single_first, record.single_string_forward.size);
            ARGB_Color color = fcolor_resolve(fcolor_id(defcolor_undo));
            f32 undo_fade_time = 0.33f;
            buffer_post_fade(app, buffer, undo_fade_time, range, color);
        }
        
        view_set_cursor_and_preferred_x(app, view, seek_pos(new_position));
    }
    
    Long_Buffer_CheckHistoryAndSetDirty(app);
}

CUSTOM_COMMAND_SIG(long_undo_all_buffers)
CUSTOM_DOC("Advances backward through the undo history in the buffer containing the most recent regular edit.")
{
    undo_all_buffers(app);
    for (Buffer_ID buffer = get_buffer_next(app, 0, Access_Always); buffer; buffer = get_buffer_next(app, buffer, Access_Always))
        Long_Buffer_CheckHistoryAndSetDirty(app, buffer);
}

CUSTOM_COMMAND_SIG(long_redo_all_buffers)
CUSTOM_DOC("Advances forward through the undo history in the buffer containing the most recent regular edit.")
{
    redo_all_buffers(app);
    for (Buffer_ID buffer = get_buffer_next(app, 0, Access_Always); buffer; buffer = get_buffer_next(app, buffer, Access_Always))
        Long_Buffer_CheckHistoryAndSetDirty(app, buffer);
}

function void Long_Buffer_AdvanceHistorySamePos(Application_Links* app, b32 undo)
{
    View_ID view = get_active_view(app, Access_Always);
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
    
    Managed_Object object = alloc_buffer_markers_on_buffer(app, view_get_buffer(app, view, Access_Always), 1, 0);
    Marker* marker = (Marker*)managed_object_get_pointer(app, object);
    marker->pos = view_get_cursor_pos(app, view);
    marker->lean_right = 0;
    
    if (undo)
        long_undo(app);
    else
        long_redo(app);
    
    view_set_cursor_and_preferred_x(app, view, seek_pos(marker->pos));
    managed_object_free(app, object);
    view_set_buffer_scroll(app, view, scroll, SetBufferScroll_SnapCursorIntoView);
}

CUSTOM_COMMAND_SIG(long_undo_same_pos)
CUSTOM_DOC("Advances backward through the undo history of the current buffer but doesn't move the cursor.")
{
    Long_Buffer_AdvanceHistorySamePos(app, 1);
}

CUSTOM_COMMAND_SIG(long_redo_same_pos)
CUSTOM_DOC("Advances forwards through the undo history of the current buffer but doesn't move the cursor.")
{
    Long_Buffer_AdvanceHistorySamePos(app, 0);
}

//- NOTE(long): Indentation
function i32 Long_EndBuffer(Application_Links* app, Buffer_ID buffer_id)
{
    F4_Index_Lock();
    F4_Index_File* file = F4_Index_LookupFile(app, buffer_id);
    if (file)
    {
        Long_Index_ClearFile(file);
        F4_Index_EraseFile(app, buffer_id);
    }
    F4_Index_Unlock();
    return end_buffer_close_jump_list(app, buffer_id);
}

// @COPYPASTA(long): default_file_save
function i32 Long_SaveFile(Application_Links *app, Buffer_ID buffer_id)
{
    ProfileScope(app, "[Long] Save File");
    
    b32 auto_indent = def_get_config_b32(vars_save_string_lit("automatically_indent_text_on_save"));
    b32 is_virtual = def_get_config_b32(vars_save_string_lit("enable_virtual_whitespace"));
    History_Record_Index index = buffer_history_get_current_state_index(app, buffer_id);
    if (auto_indent && is_virtual && index)
        Long_Index_IndentBuffer(app, buffer_id, buffer_range(app, buffer_id), true);
    
    Long_Buffer_History  current = Long_Buffer_GetCurrentHistory (app, buffer_id);
    Long_Buffer_History* history = Long_Buffer_GetAttachedHistory(app, buffer_id);
    
    Scratch_Block scratch(app);
    String8 name = push_buffer_base_name(app, scratch, buffer_id);
    print_message(app, push_stringf(scratch, "Saving Buffer: \"%.*s\", Current Undo: %.3d, Previous Saved: (%.3d, %.3d)\n",
                                    string_expand(name), current.index, history->index, history->record.edit_number));
    
    *history = current;
    
    Managed_Scope scope = buffer_get_managed_scope(app, buffer_id);
    Line_Ending_Kind *eol = scope_attachment(app, scope, buffer_eol_setting, Line_Ending_Kind);
    
    switch (*eol)
    {
        case LineEndingKind_LF:   rewrite_lines_to_lf  (app, buffer_id); break;
        case LineEndingKind_CRLF: rewrite_lines_to_crlf(app, buffer_id); break;
    }
    
    // no meaning for return
    return 0;
}

CUSTOM_COMMAND_SIG(long_indent_whole_file)
CUSTOM_DOC("Audo-indents the entire current buffer.")
{
    Buffer_ID buffer = view_get_buffer(app, get_active_view(app, Access_ReadWriteVisible), Access_ReadWriteVisible);
    Long_Index_IndentBuffer(app, buffer, buffer_range(app, buffer));
}

function void Long_Indent_CursorRange(Application_Links* app, b32 merge_history, b32 force_update = 0)
{
    // NOTE(long): Call the update tick function here to force the index system to reparse our buffer
    // so that it's up-to-date when we indent it later
    if (force_update)
        Long_Index_Tick(app);
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    Range_i64 range = get_view_range(app, view);
    Long_Index_IndentBuffer(app, buffer, range, merge_history);
    move_past_lead_whitespace(app, view, buffer);
}

CUSTOM_COMMAND_SIG(long_indent_range)
CUSTOM_DOC("Auto-indents the range between the cursor and the mark.")
{
    Long_Indent_CursorRange(app, 0);
}

// @COPYPASTA(long): write_text_and_auto_indent
CUSTOM_COMMAND_SIG(long_write_text_and_auto_indent)
CUSTOM_DOC("Inserts text and auto-indents the line on which the cursor sits if any of the text contains 'layout punctuation' such as ;:{}()[]# and new lines.")
{
    ProfileScope(app, "write and auto indent");
    User_Input in = get_current_input(app);
    String_Const_u8 insert = to_writable(&in);
    if (insert.str != 0 && insert.size > 0){
        b32 do_auto_indent = false;
        for (u64 i = 0; !do_auto_indent && i < insert.size; i += 1){
            switch (insert.str[i]){
                case ';': case ':':
                case '{': case '}':
                case '(': case ')':
                case '[': case ']':
                case '#':
                case '\n': case '\t':
                {
                    do_auto_indent = true;
                }break;
            }
        }
        if (do_auto_indent){
            View_ID view = get_active_view(app, Access_ReadWriteVisible);
            Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
            
            Range_i64 pos = {};
            if (view_has_highlighted_range(app, view)){
                pos = get_view_range(app, view);
            }
            else{
                pos.min = pos.max = view_get_cursor_pos(app, view);
            }
            
            write_text_input(app);
            
            i64 end_pos = view_get_cursor_pos(app, view);
            pos.min = Min(pos.min, end_pos);
            pos.max = Max(pos.max, end_pos);
            
            Long_Index_IndentBuffer(app, buffer, pos, true);
            move_past_lead_whitespace(app, view, buffer);
        }
        else{
            write_text_input(app);
        }
    }
}

//- NOTE(long): Special Buffers
CUSTOM_COMMAND_SIG(long_kill_buffer)
CUSTOM_DOC("Kills the current buffer.")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    Long_KillBuffer(app, buffer, view);
}

CUSTOM_COMMAND_SIG(long_switch_to_search_buffer)
CUSTOM_DOC("Switch to the search buffer.")
{
    Long_Jump_ToBuffer(app, get_this_ctx_view(app, Access_Always), Long_Buffer_GetSearchBuffer(app), 1, 0);
}

CUSTOM_COMMAND_SIG(long_kill_search_buffer)
CUSTOM_DOC("Kills the current search jump buffer.")
{
    Long_KillBuffer(app, Long_Buffer_GetSearchBuffer(app), get_active_view(app, Access_ReadVisible));
}

CUSTOM_COMMAND_SIG(long_open_matching_file_same_panel)
CUSTOM_DOC("If the current file is a *.cpp or *.h, attempts to open the corresponding *.h or *.cpp file in the same view.")
{
    View_ID view = get_active_view(app, Access_Always);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    Buffer_ID new_buffer = 0;
    if (get_cpp_matching_file(app, buffer, &new_buffer))
        Long_Jump_ToBuffer(app, view, new_buffer);
}

CUSTOM_COMMAND_SIG(long_toggle_compilation_expand)
CUSTOM_DOC("Expand the compilation window.")
{
    f32 line_height = get_face_metrics(app, get_face_id(app, view_get_buffer(app, global_compilation_view, Access_Always))).line_height;
    i32 line_count = (global_compilation_view_expanded ^= 1) ? 31 : 3;
    f32 bar_height = get_face_metrics(app, get_face_id(app, 0)).line_height + 2.f;
    f32 margin_size = (f32)def_get_config_u64(app, vars_save_string_lit("f4_margin_size"));
    f32 padding = 3.f;
    
    view_set_split_pixel_size(app, global_compilation_view, (i32)(line_height * line_count + bar_height + margin_size * 2.f + padding));
}

CUSTOM_COMMAND_SIG(long_toggle_panel_expand)
CUSTOM_DOC("Expand the compilation window.")
{
    View_ID view = get_active_view(app, 0);
    Rect_f32 old_rect = view_get_screen_rect(app, view);
    view_set_split_proportion(app, view, .5f);
    Rect_f32 new_rect = view_get_screen_rect(app, view);
    if (rect_width(new_rect) == rect_width(old_rect) && rect_height(new_rect) == rect_height(old_rect))
        view_set_split_proportion(app, view, .625f);
}

//~ NOTE(long): Search/Jump Commands

#define LONG_QUERY_STRING_SIZE KB(1)
CUSTOM_ID(attachment, long_search_string_size);
CUSTOM_ID(attachment, long_start_selection_offset);
CUSTOM_ID(attachment, long_selection_pos_offset);

// @COPYPASTA(long): create_or_switch_to_buffer_and_clear_by_name
function Buffer_ID Long_CreateOrSwitchBuffer(Application_Links *app, String_Const_u8 name_string, View_ID default_target_view)
{
    Buffer_ID search_buffer = get_buffer_by_name(app, name_string, Access_Always);
    b32 jump_to_buffer = 1;
    if (search_buffer)
    {
        View_ID target_view = get_first_view_with_buffer(app, search_buffer);
        if (target_view)
        {
            default_target_view = target_view;
            jump_to_buffer = 0;
        }
        buffer_set_setting(app, search_buffer, BufferSetting_ReadOnly, true);
        clear_buffer(app, search_buffer);
        buffer_send_end_signal(app, search_buffer);
    }
    else
    {
        search_buffer = create_buffer(app, name_string, BufferCreate_AlwaysNew);
        buffer_set_setting(app, search_buffer, BufferSetting_Unimportant, true);
        buffer_set_setting(app, search_buffer, BufferSetting_ReadOnly, true);
    }
    
    Long_Jump_ToBuffer(app, default_target_view, search_buffer, jump_to_buffer, 0);
    view_set_active(app, default_target_view);
    return(search_buffer);
}

function String_Match_List Long_FindAllMatches(Application_Links *app, Arena *arena, String_Const_u8_Array match_patterns,
                                               String_Match_Flag must_have, String_Match_Flag must_not_have, Buffer_ID current)
{
    String_Match_List all_matches = {};
    for (Buffer_ID buffer = current ? current : get_buffer_next(app, 0, 0); buffer; buffer = get_buffer_next(app, buffer, 0))
    {
        String_Match_List buffer_matches = {};
        
        for (i32 i = 0; i < match_patterns.count; i += 1)
        {
            Range_i64 range = buffer_range(app, buffer);
            String_Match_List matches = buffer_find_all_matches(app, arena, buffer, i, range, match_patterns.vals[i],
                                                                &character_predicate_alpha_numeric_underscore_utf8, Scan_Forward);
            
            string_match_list_filter_flags(&matches, must_have, must_not_have);
            if (matches.count > 0)
                buffer_matches = buffer_matches.count ? string_match_list_merge_front_to_back(&buffer_matches, &matches) : matches;
        }
        
        all_matches = string_match_list_join(&all_matches, &buffer_matches);
        if (current)
            break;
    }
    return all_matches;
}

function void Long_PrintAllMatches(Application_Links *app, String_Const_u8_Array match_patterns, String_Match_Flag must_have_flags,
                                   String_Match_Flag must_not_have_flags, Buffer_ID out_buffer_id, Buffer_ID current_buffer)
{
    Scratch_Block scratch(app);
    String_Match_List matches = Long_FindAllMatches(app, scratch, match_patterns, must_have_flags, must_not_have_flags, current_buffer);
    string_match_list_filter_remove_buffer(&matches, out_buffer_id);
    string_match_list_filter_remove_buffer_predicate(app, &matches, buffer_has_name_with_star);
    print_string_match_list_to_buffer(app, out_buffer_id, matches);
}

typedef i64 Line_Col_Predicate(Application_Links *app, Buffer_ID buffer, i64 line);

function void Long_PrintMatchedLines(Application_Links *app, Buffer_ID out_buffer, Buffer_ID buffer,
                                     Range_i64 lines, Line_Col_Predicate* predicate = 0)
{
    Scratch_Block scratch(app);
    clear_buffer(app, out_buffer);
    buffer_set_setting(app, out_buffer, BufferSetting_ReadOnly, true);
    buffer_set_setting(app, out_buffer, BufferSetting_RecordsHistory, false);
    
    Buffer_Insertion out = begin_buffer_insertion_at_buffered(app, out_buffer, 0, scratch, KB(64));
    {
        String8 filename = push_buffer_file_name(app, scratch, buffer);
        if (!filename.size)
            filename = push_buffer_unique_name(app, scratch, buffer);
        
        for (i64 line = lines.min; line <= lines.max; ++line)
        {
            i64 col = predicate ? predicate(app, buffer, line) : 1;
            if (col < 1)
                continue;
            
            Temp_Memory line_temp = begin_temp(scratch);
            String_Const_u8 line_str = string_skip_chop_whitespace(push_buffer_line(app, scratch, buffer, line));
            insertf(&out, "%.*s:%d:%d: %.*s\n", string_expand(filename), line, col, string_expand(line_str));
            end_temp(line_temp);
        }
    }
    end_buffer_insertion(&out);
}

// @COPYPASTA(long): query_user_general
function b32 Long_Query_User_String(Application_Links *app, Query_Bar *bar, String_Const_u8 init_string){
    if (start_query_bar(app, bar, 0) == 0){
        return(false);
    }
    
    if (init_string.size > 0){
        String_u8 string = Su8(bar->string.str, bar->string.size, bar->string_capacity);
        string_append(&string, init_string);
        bar->string.size = string.string.size;
    }
    
    b32 success = true;
    for (;;){
        User_Input in = get_next_input(app, EventPropertyGroup_Any,
                                       EventProperty_Escape|EventProperty_MouseButton);
        if (in.abort){
            success = false;
            break;
        }
        
        Scratch_Block scratch(app);
        b32 good_insert = false;
        String_Const_u8 insert_string = to_writable(&in);
        if (insert_string.str != 0 && insert_string.size > 0){
            insert_string = string_replace(scratch, insert_string,
                                           string_u8_litexpr("\n"),
                                           string_u8_litexpr(""));
            insert_string = string_replace(scratch, insert_string,
                                           string_u8_litexpr("\t"),
                                           string_u8_litexpr(""));
            good_insert = true;
        }
        
        if (in.event.kind == InputEventKind_KeyStroke && in.event.key.code == KeyCode_Return){
            break;
        }
        else if (in.event.kind == InputEventKind_KeyStroke &&
                 in.event.key.code == KeyCode_Backspace){
            if (has_modifier(&in, KeyCode_Control))
                bar->string.size = 0;
            else
                bar->string = backspace_utf8(bar->string);
        }
        
        else if (match_key_code(&in, KeyCode_Tab))
        {
            View_ID view = get_active_view(app, 0);
            Range_i64 range = get_view_range(app, view);
            String8 string = push_buffer_range(app, scratch, view_get_buffer(app, view, 0), range);
            String_u8 bar_string = Su8(bar->string.str, bar->string.size, bar->string_capacity);
            string_append(&bar_string, string);
            bar->string.size = bar_string.size;
        }
        else if (match_key_code(&in, KeyCode_V) && has_modifier(&in, KeyCode_Control))
        {
            Scratch_Block scratch(app);
            String8 string = push_clipboard_index(scratch, 0, 0);
            String_u8 bar_string = Su8(bar->string.str, bar->string.size, bar->string_capacity);
            string_append(&bar_string, string);
            bar->string.size = bar_string.size;
        }
        
        else if (good_insert){
            String_u8 string = Su8(bar->string.str, bar->string.size, bar->string_capacity);
            string_append(&string, insert_string);
            bar->string.size = string.string.size;
        }
        else{
            // NOTE(allen): is the user trying to execute another command?
            View_ID view = get_this_ctx_view(app, Access_Always);
            View_Context ctx = view_current_context(app, view);
            Mapping *mapping = ctx.mapping;
            Command_Map *map = mapping_get_map(mapping, ctx.map_id);
            Command_Binding binding = map_get_binding_recursive(mapping, map, &in.event);
            if (binding.custom != 0){
                Command_Metadata *metadata = get_command_metadata(binding.custom);
                if (metadata != 0){
                    if (metadata->is_ui){
                        view_enqueue_command_function(app, view, binding.custom);
                        break;
                    }
                }
                binding.custom(app);
            }
            else{
                leave_current_input_unhandled(app);
            }
        }
    }
    
    return(success);
}

function Range_i32 Long_Highlight_GetRange(Locked_Jump_State state, i32 selection_offset)
{
    Range_i32 result;
    if (selection_offset == state.list->jump_count)
        result = Ii32_size(0, state.list->jump_count - 1);
    else
        result = Ii32_size(state.list_index, selection_offset);
    return result;
}

function Marker* Long_SearchBuffer_GetMarkers(Application_Links* app, Arena* arena, Managed_Scope search_scope,
                                              Buffer_ID buffer, i32* out_count)
{
    Managed_Scope scopes[2];
    scopes[0] = search_scope;
    scopes[1] = buffer_get_managed_scope(app, buffer);
    Managed_Scope comp_scope = get_managed_scope_with_multiple_dependencies(app, scopes, ArrayCount(scopes));
    Managed_Object* markers_object = scope_attachment(app, comp_scope, sticky_jump_marker_handle, Managed_Object);
    
    i32 count = managed_object_get_item_count(app, *markers_object);
    *out_count = count;
    
    Marker* markers = push_array(arena, Marker, count);
    managed_object_load_data(app, *markers_object, 0, count, markers);
    return markers;
}

function void Long_SearchBuffer_Jump(Application_Links* app, Locked_Jump_State state, Sticky_Jump jump)
{
    goto_jump_in_order(app, state.list, state.view, { jump.jump_buffer_id, jump.jump_pos });
    view_set_cursor_and_preferred_x(app, state.view, seek_line_col(jump.list_line, 1));
}

function Sticky_Jump Long_SearchBuffer_NearestJump(Application_Links* app, View_ID view, Buffer_ID buffer, i64 pos,
                                                   Locked_Jump_State state, Managed_Scope scope)
{
    Scratch_Block scratch(app);
    
    i32 list_index = -1;
    Sticky_Jump_Stored* storage = 0;
    
    i32 count;
    Marker* markers = Long_SearchBuffer_GetMarkers(app, scratch, scope, buffer, &count);
    if (markers)
    {
        i32 marker_index = binary_search(&markers->pos, sizeof(*markers), count, pos);
        storage = get_all_stored_jumps_from_list(app, scratch, state.list);
        for (i32 i = 0; i < state.list->jump_count; ++i)
        {
            if (storage[i].jump_buffer_id == buffer && storage[i].index_into_marker_array == marker_index)
            {
                list_index = i;
                break;
            }
        }
    }
    
    Sticky_Jump result = {};
    if (list_index >= 0)
    {
        result.list_line        = storage[list_index].list_line;
        result.list_colon_index = storage[list_index].list_colon_index;
        result.is_sub_error     = storage[list_index].is_sub_error;
        result.jump_buffer_id   = storage[list_index].jump_buffer_id;
        result.jump_pos = markers[storage[list_index].index_into_marker_array].pos;
    }
    return result;
}

function void Long_SearchBuffer_MultiSelect(Application_Links* app, View_ID view, Buffer_ID search_buffer,
                                            String8 title, i64 init_size, b32 push_jump = 0)
{
    if (get_active_view(app, Access_ReadVisible) != view)
        view_set_active(app, view); 
    
    Query_Bar_Group group(app);
    Query_Bar bar = {};
    bar.string = title;
    if (!start_query_bar(app, &bar, 0))
        return;
    
    Vec2_f32 old_margin = {};
    Vec2_f32 old_push_in = {};
    view_get_camera_bounds(app, view, &old_margin, &old_push_in);
    view_set_camera_bounds(app, view, Vec2_f32{ old_margin.x, clamp_bot(200.f, old_margin.y) }, old_push_in);
    
    auto_center_after_jumps = false;
    Managed_Scope scope = buffer_get_managed_scope(app, search_buffer);
    
#define LONG_SELECT_DEFER_PUSH_JUMP
    
    i64 cursor_pos = view_get_cursor_pos(app, view), mark_pos = 0;
    i32 jump_count;
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    {
        lock_jump_buffer(app, search_buffer);
        Locked_Jump_State jump_state = get_locked_jump_state(app, &global_heap);
        
        jump_count = jump_state.list->jump_count;
        mark_pos = view_get_mark_pos(app, view);
        
#ifndef LONG_SELECT_DEFER_PUSH_JUMP
        if (push_jump)
            Long_PointStack_Push(app, buffer, cursor_pos, view);
#endif
        
        Sticky_Jump jump = Long_SearchBuffer_NearestJump(app, view, buffer, cursor_pos, jump_state, scope);
        if (jump.jump_buffer_id)
            Long_SearchBuffer_Jump(app, jump_state, jump);
        else
            goto_first_jump(app);
    }
    
    i32* selection_offset = scope_attachment(app, scope, long_start_selection_offset, i32);
    *selection_offset = init_size == 0 ? jump_count : 0;
    
    i64* size = scope_attachment(app, scope, long_search_string_size, i64);
    *size = init_size;
    
    i64* offset = scope_attachment(app, scope, long_selection_pos_offset, i64);
    *offset = 0;
    
    b32 abort = false, exit_to_jump_highlight = false, has_modified_string = false;
    def_set_config_b32(vars_save_string_lit("use_jump_highlight"), 0);
    
    Long_Buffer_History start_history = {};
    global_history_edit_group_begin(app);
    
    for (;;)
    {
        Scratch_Block scratch(app);
        
        User_Input in = get_next_input(app, EventPropertyGroup_Any, EventProperty_Escape);
        if (in.abort)
        {
            exit_to_jump_highlight = has_modifier(&in, KeyCode_Shift);
            abort = !exit_to_jump_highlight;
            break;
        }
        
        Locked_Jump_State jump_state = get_locked_jump_state(app, &global_heap);
        if (!jump_state.view)
        {
            abort = exit_to_jump_highlight = true;
            break;
        }
        
        i32 selection_count = clamp_top(Long_Abs(*selection_offset) + 1, jump_count);
        bar.prompt = push_stringf(scratch, "Selection Count: %d, For: ", selection_count);
        
        ID_Pos_Jump_Location current_location;
        get_jump_from_list(app, jump_state.list, jump_state.list_index, &current_location);
        
        // NOTE(long): Rather than disable the highlight, we set it to an empty range so that F4_RenderBuffer doesn't render any cursor
        view_set_highlight_range(app, view, {});
        
        String8 string = to_writable(&in);
        i32 advance = 0;
        b32 change_string = false;
        
        if (match_key_code(&in, KeyCode_Return) || match_key_code(&in, KeyCode_Tab))
        {
            if (push_jump)
            {
#ifdef LONG_SELECT_DEFER_PUSH_JUMP
                Long_PointStack_Push(app, buffer, cursor_pos, view);
#endif
                Long_PointStack_Push(app, current_location.buffer_id, current_location.pos, view);
            }
            break;
        }
        
        else if (!has_modified_string && (match_key_code(&in, KeyCode_PageUp) || match_key_code(&in, KeyCode_Up)))
        {
            goto_prev_jump(app);
            advance = -1;
            
            CHANGE_JUMP:
            i64 new_pos = view_get_cursor_pos(app, view);
            Buffer_ID new_buffer = view_get_buffer(app, view, 0);
            if (current_location.pos != new_pos || current_location.buffer_id != new_buffer)
            {
                if (has_modifier(&in, KeyCode_Shift) && *selection_offset != jump_count)
                    *selection_offset -= advance;
                else
                    *selection_offset  = 0;
            }
        }
        else if (!has_modified_string && (match_key_code(&in, KeyCode_PageDown) || match_key_code(&in, KeyCode_Down)))
        {
            goto_next_jump(app);
            advance = +1;
            goto CHANGE_JUMP;
        }
        
        else if (match_key_code(&in, KeyCode_Left ))
        {
            advance = -1;
            
            CHANGE_SIZE:
            if (has_modified_string)
                *offset += advance;
            else if (!has_modifier(&in, KeyCode_Control))
                *size += advance;
            else
            {
                has_modified_string = true;
                if ((advance > 0 && *size > 0) || (advance < 0 && *size < 0))
                    *offset = *size;
                *size = 0;
            }
        }
        else if (match_key_code(&in, KeyCode_Right))
        {
            advance = +1;
            goto CHANGE_SIZE;
        }
        
        else if (match_key_code(&in, KeyCode_A) && has_modifier(&in, KeyCode_Control))
            *selection_offset = (*selection_offset == jump_count) ? 0 : jump_count;
        
        else if (match_key_code(&in, KeyCode_E) && has_modifier(&in, KeyCode_Control))
            center_view(app);
        
        else if (match_key_code(&in, KeyCode_V) && has_modifier(&in, KeyCode_Control))
        {
            string = push_clipboard_index(scratch, 0, 0);
            change_string = true;
        }
        
        else if (match_key_code(&in, KeyCode_Backspace))
        {
            advance = -1;
            
            DELETION:
            change_string = true;
            if (has_modified_string)
                *size = advance;
        }
        
        else if (match_key_code(&in, KeyCode_Delete))
        {
            advance = +1;
            goto DELETION;
        }
        
        else if (string.str != 0 && string.size > 0)
            change_string = true;
        
        else
        {
            // NOTE(allen): is the user trying to execute another command?
            View_Context ctx = view_current_context(app, view);
            Command_Binding binding = map_get_binding_recursive(ctx.mapping, mapping_get_map(ctx.mapping, ctx.map_id), &in.event);
            
            if (binding.custom)
            {
                Command_Metadata* metadata = get_command_metadata(binding.custom);
                if (metadata && metadata->is_ui)
                {
                    view_enqueue_command_function(app, view, binding.custom);
                    break;
                }
                binding.custom(app);
            }
            else leave_current_input_unhandled(app);
        }
        
        if (change_string)
        {
            Range_i32 select_range = Long_Highlight_GetRange(jump_state, *selection_offset);
            
            Batch_Edit* batch_head = 0;
            Batch_Edit* batch_tail = 0;
            Buffer_ID buffer = 0;
            
            for (i32 i = select_range.min; i <= select_range.max; ++i)
            {
                ID_Pos_Jump_Location location;
                if (get_jump_from_list(app, jump_state.list, i, &location))
                {
                    Batch_Edit* batch = push_array(scratch, Batch_Edit, 1);
                    batch->edit.text = string;
                    batch->edit.range = Ii64_size(location.pos + *offset, *size);
                    
                    if (location.buffer_id != buffer)
                    {
                        if (batch_head)
                        {
                            buffer_batch_edit(app, buffer, batch_head);
                            if (!has_modified_string)
                                start_history = Long_Buffer_GetCurrentHistory(app, buffer);
                        }
                        batch_head = batch_tail = 0;
                        buffer = location.buffer_id;
                    }
                    
                    sll_queue_push(batch_head, batch_tail, batch);
                    
                    if (i == select_range.max && batch_head)
                    {
                        buffer_batch_edit(app, buffer, batch_head);
                        if (!start_history.index)
                            start_history = Long_Buffer_GetCurrentHistory(app, buffer);
                    }
                }
            }
            
            if (*offset >= 0)
                *offset += string.size;
            
            if (has_modified_string)
            {
                if ((*offset > 0 && *size < 0) || // Backspace
                    (*offset < 0 && *size > 0))   // Delete
                    *offset += *size;
            }
            
            *size = 0;
            has_modified_string = true;
        }
    }
    
    global_history_edit_group_end(app);
    if (start_history.index)
    {
        for (Buffer_ID buffer = get_buffer_next(app, 0, Access_Always); buffer; buffer = get_buffer_next(app, buffer, Access_Always))
        {
            History_Record_Index last_index = buffer_history_get_current_state_index(app, buffer);
            for (History_Record_Index first_index = 1; first_index <= last_index; ++first_index)
            {
                Record_Info record = buffer_history_get_record_info(app, buffer, first_index);
                
                if (record.edit_number == start_history.record.edit_number)
                {
                    if (abort)
                    {
                        buffer_history_set_current_state_index(app, buffer, first_index - 1);
                        buffer_history_clear_after_current_state(app, buffer);
                        Long_Buffer_CheckHistoryAndSetDirty(app, buffer);
                    }
                    else if (first_index < last_index)
                        buffer_history_merge_record_range(app, buffer, first_index, last_index, 0);
                    
                    break;
                }
            }
        }
    }
    
    if (exit_to_jump_highlight)
        def_set_config_b32(vars_save_string_lit("use_jump_highlight"), 1);
    else
        long_kill_search_buffer(app);
    
    if (abort)
    {
#ifdef LONG_SELECT_DEFER_PUSH_JUMP
        push_jump = 0;
#endif
        
        if (push_jump)
            Long_PointStack_JumpNext(app, view, 0, 1);
        else
            F4_JumpToLocation(app, view, buffer, cursor_pos);
        view_set_mark(app, view, seek_pos(mark_pos));
    }
    
    auto_center_after_jumps = true;
    view_disable_highlight_range(app, view); // NOTE(long): Disable the highlight will make F4_RenderBuffer draw the cursor again
    view_set_camera_bounds(app, view, old_margin, old_push_in);
}

function void Long_ListAllLocations(Application_Links *app, String_Const_u8 needle, List_All_Locations_Flag flags, b32 all_buffer)
{
    if (!needle.size)
        return;
    String_Match_Flag must_have_flags = 0;
    if (HasFlag(flags, ListAllLocationsFlag_CaseSensitive))
        AddFlag(must_have_flags, StringMatch_CaseSensitive);
    
    String_Match_Flag must_not_have_flags = 0;
    if (!HasFlag(flags, ListAllLocationsFlag_MatchSubstring))
        AddFlag(must_not_have_flags, StringMatch_LeftSideSloppy|StringMatch_RightSideSloppy);
    
    View_ID view = get_active_view(app, Access_Always);
    Buffer_ID current_buffer = view_get_buffer(app, view, Access_Always);
    Buffer_ID search_buffer = Long_CreateOrSwitchBuffer(app, search_name, get_next_view_after_active(app, Access_Always));
    Long_PrintAllMatches(app, { &needle, 1 }, must_have_flags, must_not_have_flags, search_buffer, all_buffer ? 0 : current_buffer);
    
    Scratch_Block scratch(app);
    String8 no_match = S8Lit("no matches");
    String8 search_result = push_buffer_range(app, scratch, search_buffer, Ii64_size(0, no_match.size));
    if (!string_match(search_result, no_match))
        Long_SearchBuffer_MultiSelect(app, view, search_buffer, needle, needle.size, 1);
}

// @COPYPASTA(long): get_query_string
function String8 Long_Get_Query_String(Application_Links *app, char *query_str, u8 *string_space, i32 space_size, u64 init_size = 0)
{
    Query_Bar_Group group(app);
    Query_Bar bar = {};
    bar.prompt = SCu8((u8*)query_str);
    bar.string = SCu8(string_space, init_size);
    bar.string_capacity = space_size;
    if (!Long_Query_User_String(app, &bar, string_u8_empty)){
        bar.string.size = 0;
    }
    return(bar.string);
}

function void Long_ListAllLocations_Query(Application_Links *app, char* query, List_All_Locations_Flag flags, b32 all_buffer)
{
    Scratch_Block scratch(app);
    u8* space = push_array(scratch, u8, LONG_QUERY_STRING_SIZE);
    String8 needle = fcoder_mode == FCoderMode_NotepadLike ? push_view_range_string(app, scratch) : String8{};
    i64 size = Min(needle.size, LONG_QUERY_STRING_SIZE);
    block_copy(space, needle.str, size);
    needle = Long_Get_Query_String(app, "List Locations For: ", space, LONG_QUERY_STRING_SIZE, size);
    
    Long_ListAllLocations(app, needle, flags, all_buffer);
}

function void Long_ListAllLocations_Identifier(Application_Links *app, List_All_Locations_Flag flags, b32 all_buffer)
{
    Scratch_Block scratch(app);
    String_Const_u8 needle = push_token_or_word_under_active_cursor(app, scratch);
    Long_ListAllLocations(app, needle, flags, all_buffer);
}

function void Long_ListAllLines_InRange(Application_Links *app, Range_i64 range, Line_Col_Predicate* predicate = 0, i64 size = 0)
{
    View_ID view = get_active_view(app, Access_Always);
    Buffer_ID current_buffer = view_get_buffer(app, view, Access_Always);
    
    Range_i64 lines = get_line_range_from_pos_range(app, current_buffer, range);
    if (lines.first == lines.max)
        return;
    
    Buffer_ID search_buffer = Long_CreateOrSwitchBuffer(app, search_name, get_next_view_after_active(app, Access_Always));
    Long_PrintMatchedLines(app, search_buffer, current_buffer, lines, predicate);
    
    if (buffer_get_size(app, search_buffer))
    {
        Scratch_Block scratch(app);
        String8 title = push_stringf(scratch, "Lines: %d -> %d", lines.min, lines.max);
        Long_SearchBuffer_MultiSelect(app, view, search_buffer, title, size);
    }
}

CUSTOM_COMMAND_SIG(long_list_all_locations)
CUSTOM_DOC("Queries the user for a string and lists all exact case-sensitive matches found in all open buffers.")
{
    Long_ListAllLocations_Query(app, "List Case-Sensitive Locations In All Buffer For: ", ListAllLocationsFlag_CaseSensitive, 1);
}

CUSTOM_COMMAND_SIG(long_list_all_substring_locations_case_insensitive)
CUSTOM_DOC("Queries the user for a string and lists all case-insensitive substring matches found in all open buffers.")
{
    Long_ListAllLocations_Query(app, "List Substring Locations In All Buffer For: ", ListAllLocationsFlag_MatchSubstring, 1);
}

CUSTOM_COMMAND_SIG(long_list_all_locations_current_buffer)
CUSTOM_DOC("Queries the user for a string and lists all exact case-sensitive matches found in the current buffer.")
{
    Long_ListAllLocations_Query(app, "List Case-Sensitive Locations For: ", ListAllLocationsFlag_CaseSensitive, 0);
}

CUSTOM_COMMAND_SIG(long_list_all_substring_locations_case_insensitive_current_buffer)
CUSTOM_DOC("Queries the user for a string and lists all case-insensitive substring matches found in the current buffer.")
{
    Long_ListAllLocations_Query(app, "List Substring Locations For: ", ListAllLocationsFlag_MatchSubstring, 0);
}

CUSTOM_COMMAND_SIG(long_list_all_locations_of_identifier)
CUSTOM_DOC("Reads a token or word under the cursor and lists all exact case-sensitive mathces in all open buffers.")
{
    Long_ListAllLocations_Identifier(app, ListAllLocationsFlag_CaseSensitive, 1);
}

CUSTOM_COMMAND_SIG(long_list_all_substring_locations_of_identifier_case_insensitive)
CUSTOM_DOC("Reads a token or word under the cursor and lists all exact case-insensitive mathces in all open buffers.")
{
    Long_ListAllLocations_Identifier(app, ListAllLocationsFlag_MatchSubstring, 1);
}

CUSTOM_COMMAND_SIG(long_list_all_locations_of_identifier_current_buffer)
CUSTOM_DOC("Reads a token or word under the cursor and lists all exact case-sensitive mathces in the current buffer.")
{
    Long_ListAllLocations_Identifier(app, ListAllLocationsFlag_CaseSensitive, 0);
}

CUSTOM_COMMAND_SIG(long_list_all_substring_locations_of_identifier_case_insensitive_current_buffer)
CUSTOM_DOC("Reads a token or word under the cursor and lists all exact case-insensitive mathces in the current buffer.")
{
    Long_ListAllLocations_Identifier(app, ListAllLocationsFlag_CaseSensitive|ListAllLocationsFlag_MatchSubstring, 0);
}

function i64 Long_Line_NonWhitespace(Application_Links* app, Buffer_ID buffer, i64 line)
{
    Range_i64 range = get_line_pos_range(app, buffer, line);
    Indent_Info info = get_indent_info_range(app, buffer, range, 0);
    if (info.first_char_pos == range.end) return -1; // NOTE(long): Maybe just return normally here?
    return info.first_char_pos - range.start + 1;
}

function i64 Long_Line_SeekEnd(Application_Links* app, Buffer_ID buffer, i64 line)
{
    Range_i64 range = get_line_pos_range(app, buffer, line);
    return range.end - range.start + 1;
}

function i64 Long_Line_SeekEnd_NonWhitespace(Application_Links* app, Buffer_ID buffer, i64 line)
{
    Range_i64 range = get_line_pos_range(app, buffer, line);
    Indent_Info info = get_indent_info_range(app, buffer, range, 0);
    if (info.first_char_pos == range.end) return -1;
    return range.end - range.start + 1;
}

global Range_i64 long_cursor_select_range;

function i64 Long_Line_CompareRange(Application_Links* app, Buffer_ID buffer, i64 line, b32 compare_first_char = 0)
{
    Range_i64 range = get_line_pos_range(app, buffer, line);
    Indent_Info info = get_indent_info_range(app, buffer, range, 0);
    i64 start_pos = info.first_char_pos;
    if (range_size(long_cursor_select_range) > range.end - start_pos)
        return -1;
    if (compare_first_char && start_pos - range.start != long_cursor_select_range.min)
        return -1;
    return info.first_char_pos - range.start + 1;
}

function i64 Long_Line_HasEnoughSize(Application_Links* app, Buffer_ID buffer, i64 line)
{
    return Long_Line_CompareRange(app, buffer, line, 0);
}

function i64 Long_Line_HasEnoughSizeAndOffset(Application_Links* app, Buffer_ID buffer, i64 line)
{
    return Long_Line_CompareRange(app, buffer, line, 1);
}

CUSTOM_COMMAND_SIG(long_list_all_lines_in_range)
CUSTOM_DOC("Lists all lines in between the mark and the cursor that are not blank.")
{
    Long_ListAllLines_InRange(app, get_view_range(app, get_active_view(app, Access_Always)));
}

CUSTOM_COMMAND_SIG(long_list_all_lines_in_range_non_white)
CUSTOM_DOC("Lists all lines in between the mark and the cursor that are not blank.")
{
    Long_ListAllLines_InRange(app, get_view_range(app, get_active_view(app, Access_Always)), Long_Line_NonWhitespace);
}

CUSTOM_COMMAND_SIG(long_list_all_lines_in_range_seek_end)
CUSTOM_DOC("Lists all the end position of all lines in between the mark and the cursor.")
{
    Long_ListAllLines_InRange(app, get_view_range(app, get_active_view(app, Access_Always)), Long_Line_SeekEnd);
}

CUSTOM_COMMAND_SIG(long_list_all_lines_in_range_seek_end_non_white)
CUSTOM_DOC("Lists all the end position of all lines in between the mark and the cursor.")
{
    Long_ListAllLines_InRange(app, get_view_range(app, get_active_view(app, Access_Always)), Long_Line_SeekEnd_NonWhitespace);
}

function void Long_ListAllLines_SizeAndOffset(Application_Links* app, b32 check_offset)
{
    View_ID view = get_active_view(app, Access_Always);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    i64 pos = view_get_cursor_pos(app, view);
    
    Range_i64 line_range = get_line_pos_range(app, buffer, get_line_number_from_pos(app, buffer, pos));
    Indent_Info info = get_indent_info_range(app, buffer, line_range, 0);
    long_cursor_select_range = Ii64(info.first_char_pos - line_range.start, pos - line_range.start);
    Long_ListAllLines_InRange(app, buffer_range(app, buffer), check_offset ? Long_Line_HasEnoughSizeAndOffset : Long_Line_HasEnoughSize,
                              range_size(long_cursor_select_range));
}

CUSTOM_COMMAND_SIG(long_list_all_lines_from_start_to_cursor_relative)
CUSTOM_DOC("Lists all the start-to-relative-cursor range of all lines in the current buffer.")
{
    Long_ListAllLines_SizeAndOffset(app, 0);
}

CUSTOM_COMMAND_SIG(long_list_all_lines_from_start_to_cursor_absolute)
CUSTOM_DOC("Lists all the start-to-absolute-cursor range of all lines in the current buffer.")
{
    Long_ListAllLines_SizeAndOffset(app, 1);
}

// @COPYPASTA(long): draw_highlight_range
function b32 Long_Highlight_DrawRange(Application_Links *app, View_ID view_id,
                                      Buffer_ID buffer, Text_Layout_ID text_layout_id,
                                      f32 roundness){
    b32 has_highlight_range = false;
    Managed_Scope scope = view_get_managed_scope(app, view_id);
    Buffer_ID *highlight_buffer = scope_attachment(app, scope, view_highlight_buffer, Buffer_ID);
    if (*highlight_buffer != 0){
        if (*highlight_buffer != buffer){
            view_disable_highlight_range(app, view_id);
        }
        else{
            has_highlight_range = true;
            Managed_Object *highlight = scope_attachment(app, scope, view_highlight_range, Managed_Object);
            Marker marker_range[2];
            if (managed_object_load_data(app, *highlight, 0, 2, marker_range)){
                Range_i64 range = Ii64(marker_range[0].pos, marker_range[1].pos);
                Long_Render_DrawBlock(app, text_layout_id, range, roundness,
                                      fcolor_id(defcolor_highlight));
                paint_text_color_fcolor(app, text_layout_id, range,
                                        fcolor_id(defcolor_at_highlight));
            }
        }
    }
    return(has_highlight_range);
}

// @COPYPASTA(long): draw_jump_highlights
function void Long_Highlight_DrawList(Application_Links *app, Buffer_ID buffer, Text_Layout_ID layout, f32 roundness, f32 thickness)
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
                Long_Render_NotepadCursor(app, view, buffer, layout, marker_pos + size, marker_pos, roundness, thickness);
            else
                Long_Render_DrawBlock(app, layout, range, 0.f, fcolor_id(fleury_color_token_minor_highlight));
        }
    }
}

// @COPYPASTA(long): isearch (Add the ability to handle case-sensitive string and paste string using Ctrl+V)
function void Long_ISearch(Application_Links* app, Scan_Direction start_scan, i64 first_pos, String_Const_u8 query_init, b32 insensitive)
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    if (!buffer_exists(app, buffer)){
        return;
    }
    
    i64 buffer_size = buffer_get_size(app, buffer);
    
    Query_Bar_Group group(app);
    Query_Bar bar = {};
    if (start_query_bar(app, &bar, 0) == 0){
        return;
    }
    
    Vec2_f32 old_margin = {};
    Vec2_f32 old_push_in = {};
    view_get_camera_bounds(app, view, &old_margin, &old_push_in);
    
    Vec2_f32 margin = old_margin;
    margin.y = clamp_bot(200.f, margin.y);
    view_set_camera_bounds(app, view, margin, old_push_in);
    
    Scan_Direction scan = start_scan;
    i64 pos = first_pos;
    
    u8 bar_string_space[LONG_QUERY_STRING_SIZE];
    bar.string = SCu8(bar_string_space, query_init.size);
    block_copy(bar.string.str, query_init.str, query_init.size);
    
    String_Const_u8 isearch_str = insensitive ? S8Lit("I-Search: ") : S8Lit("I-Search-Sensitive: ");
    String_Const_u8 rsearch_str = insensitive ? S8Lit("Reverse-I-Search: ") : S8Lit("Reverse-I-Search-Sensitve: ");
    
    u64 match_size = bar.string.size;
    
    User_Input in = {};
    for (;;)
    {
        switch (scan)
        {
            case Scan_Forward:  bar.prompt = isearch_str; break;
            case Scan_Backward: bar.prompt = rsearch_str; break;
        }
        isearch__update_highlight(app, view, Ii64_size(pos, match_size));
        
        in = get_next_input(app, EventPropertyGroup_Any, EventProperty_Escape);
        if (in.abort)
            break;
        
        String_Const_u8 string = to_writable(&in);
        b32 string_change = false;
        b32 trigger_command = true;
        
        if (match_key_code(&in, KeyCode_Return))
        {
            Input_Modifier_Set* mods = &in.event.key.modifiers;
            if (has_modifier(mods, KeyCode_Control))
            {
                bar.string.size = cstring_length(previous_isearch_query);
                block_copy(bar.string.str, previous_isearch_query, bar.string.size);
            }
            else
            {
                u64 size = bar.string.size;
                size = clamp_top(size, sizeof(previous_isearch_query) - 1);
                block_copy(previous_isearch_query, bar.string.str, size);
                previous_isearch_query[size] = 0;
                break;
            }
        }
        
        else if (match_key_code(&in, KeyCode_Tab))
        {
            Scratch_Block scratch(app);
            Range_i64 range = get_view_range(app, view);
            String8 string = push_buffer_range(app, scratch, buffer, range);
            if (string.size)
            {
                //String_u8 bar_string = Su8(bar.string.str, bar.string.size, bar.string_capacity);
                String_u8 bar_string = Su8(bar.string, sizeof(bar_string_space));
                string_append(&bar_string, string);
                bar.string = bar_string.string;
                string_change = true;
            }
        }
        
        else if (match_key_code(&in, KeyCode_E) && has_modifier(&in, KeyCode_Control))
        {
            center_view(app);
        }
        
        else if (match_key_code(&in, KeyCode_V) && has_modifier(&in, KeyCode_Control))
        {
            Scratch_Block scratch(app);
            String8 string = push_clipboard_index(scratch, 0, 0);
            if (string.size)
            {
                String_u8 bar_string = Su8(bar.string, sizeof(bar_string_space));
                string_append(&bar_string, string);
                bar.string = bar_string.string;
                string_change = true;
            }
        }
        
        else if (match_key_code(&in, KeyCode_Backspace))
        {
            if (is_unmodified_key(&in.event))
            {
                u64 old_bar_string_size = bar.string.size;
                bar.string = backspace_utf8(bar.string);
                string_change = (bar.string.size < old_bar_string_size);
            }
            else if (has_modifier(&in, KeyCode_Control))
            {
                if (bar.string.size > 0)
                {
                    string_change = true;
                    bar.string.size = 0;
                }
            }
        }
        
        else
        {
            trigger_command = false;
            if (string.str != 0 && string.size > 0)
            {
                String_u8 bar_string = Su8(bar.string, sizeof(bar_string_space));
                string_append(&bar_string, string);
                bar.string = bar_string.string;
                string_change = true;
            }
        }
        
        b32 do_scan_action = false;
        b32 do_scroll_wheel = false;
        Scan_Direction change_scan = scan;
        if (!string_change)
        {
            if (match_key_code(&in, KeyCode_PageDown) || match_key_code(&in, KeyCode_Down))
            {
                change_scan = Scan_Forward;
                do_scan_action = true;
            }
            else if (match_key_code(&in, KeyCode_PageUp) || match_key_code(&in, KeyCode_Up))
            {
                change_scan = Scan_Backward;
                do_scan_action = true;
            }
            else if (!trigger_command)
                leave_current_input_unhandled(app);
        }
        
        if (string_change || do_scan_action)
        {
            if (do_scan_action)
                scan = change_scan;
            b32 backward = scan == Scan_Backward;
            
            i64 offset = 0;
            if (string_change)
                offset = (backward ? +1 : -1);
            
            Buffer_Seek_String_Flags flags = 0;
            if (backward)    flags |= BufferSeekString_Backward;
            if (insensitive) flags |= BufferSeekString_CaseInsensitive;
            
            i64 new_pos = 0;
            seek_string(app, buffer, pos + offset, 0, 0, bar.string, &new_pos, flags);
            if (new_pos >= 0 && new_pos < buffer_size)
            {
                pos = new_pos;
                match_size = bar.string.size;
            }
        }
        else if (do_scroll_wheel)
            mouse_wheel_scroll(app);
    }
    
    view_disable_highlight_range(app, view);
    
    if (in.abort)
    {
        u64 size = bar.string.size;
        size = clamp_top(size, sizeof(previous_isearch_query) - 1);
        block_copy(previous_isearch_query, bar.string.str, size);
        previous_isearch_query[size] = 0;
        view_set_cursor_and_preferred_x(app, view, seek_pos(first_pos));
    }
    
    view_set_camera_bounds(app, view, old_margin, old_push_in);
}

// @COPYPASTA(long): F4_Search
function void Long_Search(Application_Links *app, Scan_Direction dir, b32 insensitive)
{
    Scratch_Block scratch(app);
    View_ID view = get_active_view(app, Access_Read);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Read);
    if(view && buffer)
    {
        i64 cursor = view_get_cursor_pos(app, view);
        i64 mark = view_get_mark_pos(app, view);
        i64 cursor_line = get_line_number_from_pos(app, buffer, cursor);
        i64 mark_line = get_line_number_from_pos(app, buffer, mark);
        String_Const_u8 query_init = (fcoder_mode != FCoderMode_NotepadLike || cursor == mark || cursor_line != mark_line) ?
            SCu8() : push_buffer_range(app, scratch, buffer, Ii64(cursor, mark));
        Long_ISearch(app, dir, cursor, query_init, insensitive);
    }
}

// @COPYPASTA(long): isearch_identifier
function void Long_Search_Identifier(Application_Links *app, Scan_Direction scan, b32 insensitive)
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer_id = view_get_buffer(app, view, Access_ReadVisible);
    i64 pos = view_get_cursor_pos(app, view);
    Scratch_Block scratch(app);
    Range_i64 range = enclose_pos_alpha_numeric_underscore(app, buffer_id, pos);
    String_Const_u8 query = push_buffer_range(app, scratch, buffer_id, range);
    Long_ISearch(app, scan, range.first, query, insensitive);
}

CUSTOM_COMMAND_SIG(long_search)
CUSTOM_DOC("Begins an incremental search down through the current buffer for a user specified string.")
{
    Long_Search(app, Scan_Forward, 1);
}

CUSTOM_COMMAND_SIG(long_reverse_search)
CUSTOM_DOC("Begins an incremental search up through the current buffer for a user specified string.")
{
    Long_Search(app, Scan_Backward, 1);
}

CUSTOM_COMMAND_SIG(long_search_identifier)
CUSTOM_DOC("Begins an incremental search down through the current buffer for the word or token under the cursor.")
{
    Long_Search_Identifier(app, Scan_Forward, 1);
}

CUSTOM_COMMAND_SIG(long_reverse_search_identifier)
CUSTOM_DOC("Begins an incremental search up through the current buffer for the word or token under the cursor.")
{
    Long_Search_Identifier(app, Scan_Backward, 1);
}

CUSTOM_COMMAND_SIG(long_search_case_sensitive)
CUSTOM_DOC("Searches the current buffer forward for the exact string. If something is highlighted, will fill search query with it.")
{
    Long_Search(app, Scan_Forward, 0);
}

CUSTOM_COMMAND_SIG(long_reverse_search_case_sensitive)
CUSTOM_DOC("Searches the current buffer backwards for the exact string. If something is highlighted, will fill search query with it.")
{
    Long_Search(app, Scan_Backward, 0);
}

CUSTOM_COMMAND_SIG(long_search_identifier_case_sensitive)
CUSTOM_DOC("Begins an incremental search down through the current buffer for the exact word or token under the cursor.")
{
    Long_Search_Identifier(app, Scan_Forward, 0);
}

function void Long_Query_Replace(Application_Links *app, String_Const_u8 replace_str, i64 start_pos, b32 add_replace_query_bar)
{
    Query_Bar_Group group(app);
    Query_Bar replace = {};
    replace.prompt = S8Lit("Replace: ");
    replace.string = replace_str;
    
    if (add_replace_query_bar)
        start_query_bar(app, &replace, 0);
    
    u8 with_space[LONG_QUERY_STRING_SIZE];
    Query_Bar with = {};
    with.prompt = S8Lit("With: ");
    with.string = SCu8(with_space, (u64)0);
    with.string_capacity = sizeof(with_space);
    if (!Long_Query_User_String(app, &with, string_u8_empty)) return;
    
    String8 w = with.string;
    String_Const_u8 r = replace.string;
    
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    
    Query_Bar bar = {};
    bar.prompt = string_u8_litexpr("Replace? (enter), (Page)Up, (Page)Down, (esc)\n");
    start_query_bar(app, &bar, 0);
    
    Vec2_f32 old_margin = {};
    Vec2_f32 old_push_in = {};
    view_get_camera_bounds(app, view, &old_margin, &old_push_in);
    
    Vec2_f32 margin = old_margin;
    margin.y = clamp_bot(200.f, margin.y);
    view_set_camera_bounds(app, view, margin, old_push_in);
    
    i64 pos = start_pos;
    seek_string_forward(app, buffer, pos - 1, 0, r, &pos);
    if (pos == buffer_get_size(app, buffer))
        pos = start_pos;
    
    i64 size = r.size;
    Scan_Direction scan = Scan_Forward;
    
    User_Input in = {};
    for (;;)
    {
        Range_i64 match = Ii64_size(pos, size);
        isearch__update_highlight(app, view, match);
        
        in = get_next_input(app, EventProperty_AnyKey, EventProperty_Escape);
        if (in.abort)
        {
            if (!has_modifier(&in, KeyCode_Shift))
                pos = start_pos;
            break;
        }
        
        b32 replaced = false;
        if (match_key_code(&in, KeyCode_Return) || match_key_code(&in, KeyCode_Tab))
        {
            if (range_size(match) > 0)
            {
                buffer_replace_range(app, buffer, match, w);
                pos = match.start + w.size;
                replaced = true;
                
                if (scan == Scan_Forward) goto FORWARD;
                else                      goto BACKWARD;
            }
            else break;
        }
        
        else if (match_key_code(&in, KeyCode_PageDown) || match_key_code(&in, KeyCode_Down))
        {
            FORWARD:
            scan = Scan_Forward;
            i64 new_pos;
            seek_string_forward(app, buffer, pos, 0, r, &new_pos);
            if (new_pos < buffer_get_size(app, buffer))
            {
                pos = new_pos;
                size = r.size;
            }
            else if (replaced)
            {
                FAILED:
                size = 0;
            }
        }
        
        else if (match_key_code(&in, KeyCode_PageUp  ) || match_key_code(&in, KeyCode_Up  ))
        {
            BACKWARD:
            scan = Scan_Backward;
            i64 new_pos;
            seek_string_backward(app, buffer, pos, 0, r, &new_pos);
            if (new_pos >= 0)
            {
                pos = new_pos;
                size = r.size;
            }
            else if (replaced) goto FAILED;
        }
        
        else if (match_key_code(&in, KeyCode_E) && has_modifier(&in, KeyCode_Control))
            center_view(app);
    }
    
    view_disable_highlight_range(app, view);
    
    view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
    view_set_camera_bounds(app, view, old_margin, old_push_in);
}

CUSTOM_COMMAND_SIG(long_query_replace)
CUSTOM_DOC("Queries the user for two strings, and incrementally replaces every occurence of the first string with the second string.")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    if (buffer != 0)
    {
        u8 replace_space[LONG_QUERY_STRING_SIZE];
        Query_Bar_Group group(app);
        Query_Bar bar = {};
        bar.prompt = S8Lit("Replace: ");
        bar.string = SCu8(replace_space, (u64)0);
        bar.string_capacity = sizeof(replace_space);
        if (!Long_Query_User_String(app, &bar, string_u8_empty))
            bar.string.size = 0;
        String8 replace = bar.string;
        if (replace.size > 0)
            Long_Query_Replace(app, replace, view_get_cursor_pos(app, view), false);
    }
}

CUSTOM_COMMAND_SIG(long_query_replace_identifier)
CUSTOM_DOC("Queries the user for a string, and incrementally replace every occurence of the word or token found at the cursor with the specified string.")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    if (buffer != 0){
        Scratch_Block scratch(app);
        i64 pos = view_get_cursor_pos(app, view);
        Range_i64 range = enclose_pos_alpha_numeric_underscore(app, buffer, pos);
        String_Const_u8 replace = push_buffer_range(app, scratch, buffer, range);
        if (replace.size != 0){
            Long_Query_Replace(app, replace, range.min, true);
        }
    }
}

CUSTOM_COMMAND_SIG(long_query_replace_selection)
CUSTOM_DOC("Queries the user for a string, and incrementally replace every occurence of the string found in the selected range with the specified string.")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    if (buffer != 0){
        Scratch_Block scratch(app);
        Range_i64 range = get_view_range(app, view);
        String_Const_u8 replace = push_buffer_range(app, scratch, buffer, range);
        if (replace.size != 0){
            Long_Query_Replace(app, replace, range.min, true);
        }
    }
}

//~ NOTE(long): Index Commands

//- NOTE(long): PosContext
CUSTOM_COMMAND_SIG(long_toggle_pos_context)
CUSTOM_DOC("Toggles position context window.")
{
    long_global_pos_context_open ^= 1;
}

CUSTOM_COMMAND_SIG(long_switch_pos_context_option)
CUSTOM_DOC("Switches the position context mode.")
{
    long_active_pos_context_option = (long_active_pos_context_option + 1) % ArrayCount(long_global_context_opts);
}

CUSTOM_COMMAND_SIG(long_switch_pos_context_draw_position)
CUSTOM_DOC("Switches between drawing the position context at cursor position or at bottom of buffer.")
{
    String_ID id = vars_save_string_lit("f4_poscontext_draw_at_bottom_of_buffer");
    def_set_config_b32(id, !def_get_config_b32(id));
}

//- NOTE(long): Jumping
function void Long_GoToDefinition(Application_Links* app, b32 handle_jump_location, b32 same_panel)
{
    View_ID view = get_active_view(app, Access_Always);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    
    View_ID goto_view = view;
    if(!same_panel)
        goto_view = get_next_view_looped_primary_panels(app, view, Access_Always);
    
    if (handle_jump_location)
    {
        Buffer_ID jump_buffer;
        i64 jump_pos;
        if (Long_Jump_ParseLocation(app, view, buffer, &jump_buffer, &jump_pos))
        {
            Long_Jump_ToLocation(app, goto_view, jump_buffer, jump_pos);
            return;
        }
    }
    
    F4_Index_Note* note = 0;
    {
        Token* token = get_token_from_pos(app, buffer, view_get_cursor_pos(app, view));
        
        if (token != 0 && token->size > 0 && token->kind != TokenBaseKind_Whitespace)
        {
            Token_Array array = get_token_array_from_buffer(app, buffer);
            note = Long_Index_LookupBestNote(app, buffer, &array, token);
            
            if (note && note->file)
            {
                if (note->range.min == token->pos)
                {
                    if (LONG_INDEX_FILTER_NOTE(note))
                    {
                        F4_Index_Note* next_note = note;
                        do
                        {
                            next_note = next_note->next;
                            if (!next_note)
                                next_note = Long_Index_LookupNote(note->string);
                        } while (!LONG_INDEX_FILTER_NOTE(next_note));
                        
                        if (next_note)
                            note = next_note;
                    }
                }
                
                else if (note->flags & F4_Index_NoteFlag_Prototype)
                    note = Long_Index_GetDefNote(note);
                
                // NOTE(long): note->file is valid because LONG_INDEX_FILTER_NOTE will filter out namespaces
                // and GetDefNote always returns a valid note->file
            }
            else note = 0;
        }
    }
    
    // @COPYPASTA(long): F4_GoToDefinition
    if (note)
    {
        // NOTE(long): The difference between F4_JumpToLocation and F4_GoToDefinition is:
        // 1. The first one scroll to the target while the second one snap to the target instantly (scroll.position = scroll.target)
        // 2. The first one call view_set_preferred_x (I don't really know what this function does)
        // For simplicity, I just use Long_Jump_ToLocation (which calls F4_JumpToLocation) for all jumping behaviors
        // And if I want to snap the view directly, I just call Long_SnapView
        Long_Jump_ToLocation(app, goto_view, note->file->buffer, note->range.min);
        if (!same_panel)
            Long_SnapView(app);
    }
}

CUSTOM_COMMAND_SIG(long_go_to_definition)
CUSTOM_DOC("Goes to the jump location at the cursor or the definition of the identifier under the cursor.")
{
    Long_GoToDefinition(app, 1, 0);
}

CUSTOM_COMMAND_SIG(long_go_to_definition_same_panel)
CUSTOM_DOC("Goes to the jump location at the cursor or the definition of the identifier under the cursor in the same panel.")
{
    Long_GoToDefinition(app, 1, 1);
}

//- NOTE(long): Lister
function String8 Long_Note_PushTag(Application_Links* app, Arena* arena, F4_Index_Note* note)
{
    String8 result = S8Lit("");
    switch(note->kind)
    {
        case F4_Index_NoteKind_Type:
        {
            b32 is_forward = note->flags & F4_Index_NoteFlag_Prototype;
            b32 is_union   = note->flags & F4_Index_NoteFlag_SumType;
            b32 is_generic = Long_Index_IsGenericArgument(note);
            result = push_stringf(arena, "type%s%s%s",
                                  is_forward ? " [forward]" : "",
                                  is_union ? " [union]" : "",
                                  is_generic ? " [generic]" : "");
        } break;
        
        case F4_Index_NoteKind_Macro:       result = S8Lit("macro");       break;
        case F4_Index_NoteKind_Constant:    result = S8Lit("constant");    break;
        case F4_Index_NoteKind_CommentTag:  result = S8Lit("comment tag"); break;
        case F4_Index_NoteKind_CommentToDo: result = S8Lit("TODO");        break;
        
        case F4_Index_NoteKind_Function:
        {
            String8 type = S8Lit("function");
            if (note->base_range.min)
            {
                if (Long_Index_MatchNote(app, note, note->base_range, S8Lit("operator")))
                    type = S8Lit("operator");
            }
            else if (!range_size(note->range)) type = S8Lit("lambda");
            else if (note->parent)
            {
                switch (note->parent->kind)
                {
                    case F4_Index_NoteKind_Type:
                    {
                        if (string_match(note->string, note->parent->string))
                            type = S8Lit("constructor");
                    } break;
                    
                    case F4_Index_NoteKind_Decl:
                    {
                        if (string_match(note->string, S8Lit("get")))
                            type = S8Lit("getter");
                        if (string_match(note->string, S8Lit("set")))
                            type = S8Lit("setter");
                    } break;
                }
            }
            
            result = push_stringf(arena, "%s%s", type.str, note->flags & F4_Index_NoteFlag_Prototype ? " [forward]" : "");
        } break;
        
        case F4_Index_NoteKind_Decl:
        {
            String8 string = {};
            if (note->parent)
            {
                if (Long_Index_IsArgument(note))
                    string = S8Lit("argument");
                else if (note->parent->kind == F4_Index_NoteKind_Function || note->parent->kind == F4_Index_NoteKind_Scope)
                    string = S8Lit("local");
                else if (note->parent->kind == F4_Index_NoteKind_Type)
                {
                    string = S8Lit("field");
                    if (range_size(note->scope_range) >= 2)
                    {
                        if (Long_Index_MatchNote(app, note, Ii64_size(note->scope_range.min, 2), S8Lit("=>")) ||
                            Long_Index_MatchNote(app, note, Ii64_size(note->scope_range.min, 1), S8Lit("{")))
                            string = S8Lit("property");
                    }
                }
            }
            else string = S8Lit("global");
            
            result = push_stringf(arena, "declaration [%s]", string.str);
        } break;
        
        default: break;
    }
    
    if (note->parent)
        result = push_stringf(arena, "<%.*s> %.*s", string_expand(note->parent->string), string_expand(result));
    
    // NOTE(long): Long_Lister_PushIndexNote will never iterate over a namespace note
    // This case only runs in long_write_to_file_all_identifiers
    if (Long_Index_IsNamespace(note))
        result = push_stringf(arena, "%.*s%s[namespace]", string_expand(result), result.size ? " " : "");
    
    return result;
}

function LONG_INDEX_FILTER(Long_Filter_Empty_Scopes)
{
    return note->kind == F4_Index_NoteKind_Scope && note->first_child == 0;
}

function LONG_INDEX_FILTER(Long_Filter_Declarations)
{
    return note->kind == F4_Index_NoteKind_Decl;
}

function LONG_INDEX_FILTER(Long_Filter_FunctionAndType)
{
    return Long_Filter_Func(note) || Long_Filter_Type(note);
}

function LONG_INDEX_FILTER(Long_Filter_Note)
{
    b32 result = true;
    F4_Index_Note* parent = note->parent;
    
    if (note->kind == F4_Index_NoteKind_Scope)
        result = false;
    else if (parent)
    {
        if (parent->kind == F4_Index_NoteKind_Function || (parent->kind == F4_Index_NoteKind_Scope && !Long_Index_IsNamespace(parent)))
            result = false; // Local
        else if (note->kind == F4_Index_NoteKind_Decl)
        {
            if (Long_Index_IsArgument(note))
                result = false; // Argument
            else if (parent->kind == F4_Index_NoteKind_Type)
                result = false; // Field
        }
        else if (Long_Index_IsGenericArgument(note))
            result = false; // Generic Argument
    }
    
    return result;
}

function void Long_Lister_PushIndexNote(Application_Links* app, Arena* arena, Lister* lister, F4_Index_Note* note, NoteFilter* filter)
{
    if (!filter || filter(note))
    {
        Buffer_ID buffer = note->file->buffer;
        Scratch_Block scratch(app, arena);
        
        String8List list = {};
        if (note->base_range.max)
            string_list_push(scratch, &list, push_buffer_range(app, scratch, buffer, note->base_range));
        string_list_push(scratch, &list, push_buffer_range(app, scratch, buffer, Long_Index_ArgumentRange(note)));
        String8 string = string_list_flatten(arena, list, S8Lit(" "), 0, 0);
        string = string_condense_whitespace(scratch, string);
        
        Long_Lister_AddItem(app, lister, string, Long_Note_PushTag(app, arena, note), buffer, note->range.first);
    }
    
    Long_Index_IterateValidNoteInFile(child, note)
        Long_Lister_PushIndexNote(app, arena, lister, child, filter);
}

function void Long_SearchDefinition(Application_Links* app, NoteFilter* filter, b32 project_wide)
{
    View_ID view = get_active_view(app, Access_Always);
    
    Scratch_Block scratch(app);
    Lister_Block lister(app, scratch);
    lister_set_default_handlers(lister);
    
    F4_Index_Lock();
    // TODO(long): Split iterating through all notes in project vs file into two paths
    for (Buffer_ID buffer = project_wide ? get_buffer_next(app, 0, Access_Always) : view_get_buffer(app, view, Access_Always);
         buffer; buffer = get_buffer_next(app, buffer, Access_Always))
    {
        Long_Index_ProfileBlock(app, "[Long] Search Definition");
        F4_Index_File* file = F4_Index_LookupFile(app, buffer);
        String8 filename = push_buffer_base_name(app, scratch, buffer);
        for (F4_Index_Note* note = file ? file->first_note : 0; note; note = note->next_sibling)
            Long_Lister_PushIndexNote(app, scratch, lister, note, filter);
        if (!project_wide)
            break;
    }
    F4_Index_Unlock();
    
    lister_set_query(lister, push_u8_stringf(scratch, "Index (%s):", project_wide ? "Project" : "File"));
    Lister_Result l_result = run_lister(app, lister);
    Long_Lister_Data result = {};
    if (Long_Lister_IsResultValid(l_result))
        block_copy_struct(&result, (Long_Lister_Data*)l_result.user_data);
    if (result.buffer != 0)
    {
        Long_Jump_ToLocation(app, view, result.buffer, result.pos);
        Long_SnapView(app);
    }
}

CUSTOM_UI_COMMAND_SIG(long_search_for_definition__project_wide)
CUSTOM_DOC("List all definitions in the index and jump to the one selected by the user.")
{
    Long_SearchDefinition(app, LONG_INDEX_FILTER_NOTE, 1);
}

CUSTOM_UI_COMMAND_SIG(long_search_for_definition__current_file)
CUSTOM_DOC("List all definitions in the current file and jump to the one selected by the user.")
{
    Long_SearchDefinition(app, LONG_INDEX_FILTER_NOTE, 0);
}

CUSTOM_UI_COMMAND_SIG(long_search_for_definition_no_filter__project_file)
CUSTOM_DOC("List all definitions in the current file and jump to the one selected by the user.")
{
    Long_SearchDefinition(app, 0, 1);
}

//- NOTE(long): Write
function void Long_Note_PushString(Application_Links* app, Arena* arena, String8List* list, int* noteCount,
                                   F4_Index_Note* note, NoteFilter* filter)
{
    if (*noteCount > 3000)
        system_error_box("PushNoteString is overflowed");
    if (!((note->scope_range.min == note->scope_range.max) || (note->scope_range.min && note->scope_range.max)))
    {
        String8 string = push_stringf(arena, "Note: %.*s:%d (%d, %d) isn't initialized correctly!",
                                      string_expand(note->string), note->range.min, note->scope_range.min, note->scope_range.max);
        system_error_box((char*)string.str);
    }
    
    if (!filter || filter(note))
    {
        *noteCount += 1;
        String8 tag = Long_Note_PushTag(app, arena, note);
        u64 size = note->string.size;
        u8* str = note->string.str;
        if (size && (str[size-1] == '\r' || str[size-1] == '\n'))
            --size;
        String8 parentName = note->parent ? note->parent->string : S8Lit("NULL");
        String8 string = push_stringf(arena, "[%.*s] %.*s { %.*s }", string_expand(parentName), size, str, string_expand(tag));
        string_list_push(arena, list, string);
    }
    
    Long_Index_IterateValidNoteInFile(child, note)
        Long_Note_PushString(app, arena, list, noteCount, child, filter);
}

function void Long_Buffer_Write(Application_Links* app, Arena* arena, String8 name, String8 string)
{
    View_ID view = get_next_view_after_active(app, Access_Always);
    Buffer_ID outBuffer = Long_CreateOrSwitchBuffer(app, name, view);
    Buffer_Insertion out = begin_buffer_insertion_at_buffered(app, outBuffer, 0, arena, KB(64));
    buffer_set_setting(app, outBuffer, BufferSetting_ReadOnly      , true);
    buffer_set_setting(app, outBuffer, BufferSetting_RecordsHistory, false);
    insert_string(&out, string);
    end_buffer_insertion(&out);
}

function void Long_Note_DumpTree(Application_Links* app, String8 name, NoteFilter* filter)
{
    Buffer_ID buffer = view_get_buffer(app, get_active_view(app, Access_Always), Access_Always);
    Scratch_Block scratch(app);
    String8List list = {};
    
    F4_Index_Lock();
    {
        int noteCount = 0;
        F4_Index_File* file = F4_Index_LookupFile(app, buffer);
        for (F4_Index_Note *note = file ? file->first_note : 0; note; note = note->next_sibling)
            Long_Note_PushString(app, scratch, &list, &noteCount, note, filter);
        string_list_push(scratch, &list, push_stringf(scratch, "Note count: %d", noteCount));
    }
    F4_Index_Unlock();
    
    String8 string = string_list_flatten(scratch, list, 0, S8Lit("\n"), 0, 0);
    Long_Buffer_Write(app, scratch, name, string);
}

CUSTOM_COMMAND_SIG(long_write_to_file_all_definitions)
CUSTOM_DOC("Save all definitions in the hash table.")
{
    Long_Note_DumpTree(app, S8Lit("definitions.txt"), 0);
}

CUSTOM_COMMAND_SIG(long_write_to_file_all_empty_scopes)
CUSTOM_DOC("Save all definitions in the hash table.")
{
    Long_Note_DumpTree(app, S8Lit("scopes.txt"), Long_Filter_Empty_Scopes);
}

CUSTOM_COMMAND_SIG(long_write_to_file_all_declarations)
CUSTOM_DOC("Save all definitions in the hash table.")
{
    Long_Note_DumpTree(app, S8Lit("declarations.txt"), Long_Filter_Declarations);
}

CUSTOM_COMMAND_SIG(long_write_to_file_all_identifiers)
CUSTOM_DOC("Save all identifiers in the hash table.")
{
    Buffer_ID buffer = view_get_buffer(app, get_active_view(app, Access_Always), Access_Always);
    Scratch_Block scratch(app);
    String8List list = {};
    Token_Array array = get_token_array_from_buffer(app, buffer);
    Token_Iterator_Array it = token_iterator(0, &array);
    
    while (true)
    {
        Token* token = token_it_read(&it);
        
        if (token->kind == TokenBaseKind_Identifier)
        {
            F4_Index_Note* note = Long_Index_LookupBestNote(app, buffer, &array, token);
            if (note)
            {
                String8 tag = Long_Note_PushTag(app, scratch, note);
                u64 size = note->string.size;
                u8* str = note->string.str;
                if (size && (str[size-1] == '\r' || str[size-1] == '\n'))
                    --size;
                Range_i64 range = Ii64(token);
                String8 parentName = note->parent ? note->parent->string : S8Lit("NULL");
                String8 string = push_stringf(scratch, "[%.*s] %.*s { %.*s }", string_expand(parentName), size, str, string_expand(tag));
                string_list_push(scratch, &list, string);
            }
            else
            {
                String8 string = push_token_lexeme(app, scratch, buffer, token);
                string = push_stringf(scratch, "NULL: %.*s", string_expand(string));
                string_list_push(scratch, &list, string);
            }
        }
        
        if (!token_it_inc_non_whitespace(&it))
            break;
    }
    
    string_list_push(scratch, &list, push_stringf(scratch, "Note count: %d", list.node_count));
    String8 string = string_list_flatten(scratch, list, 0, S8Lit("\n"), 0, 0);
    Long_Buffer_Write(app, scratch, S8Lit("identifiers.txt"), string);
}

//~ NOTE(long): Clipboard Commands

CUSTOM_COMMAND_SIG(long_paste_and_replace_range)
CUSTOM_DOC("replace the text between the mark and the cursor with the text from the top of the clipboard.")
{
    clipboard_update_history_from_system(app, 0);
    i32 count = clipboard_count(0);
    if (count > 0)
    {
        View_ID view = get_active_view(app, Access_ReadWriteVisible);
        if_view_has_highlighted_range_delete_range(app, view);
        
        set_next_rewrite(app, view, Rewrite_Paste);
        
        Managed_Scope scope = view_get_managed_scope(app, view);
        i32* paste_index = scope_attachment(app, scope, view_paste_index_loc, i32);
        if (paste_index != 0)
        {
            *paste_index = 0;
            
            Scratch_Block scratch(app);
            
            String_Const_u8 string = push_clipboard_index(scratch, 0, *paste_index);
            if (string.size > 0){
                Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
                
                Range_i64 range = get_view_range(app, view);
                buffer_replace_range(app, buffer, range, string);
                range = Ii64_size(range.min, string.size);
                view_set_cursor_and_preferred_x(app, view, seek_pos(range.max));
                
                ARGB_Color argb = fcolor_resolve(fcolor_id(defcolor_paste));
                buffer_post_fade(app, buffer, 0.667f, range, argb);
            }
        }
    }
    
    Long_Indent_CursorRange(app, 1, 1);
}

CUSTOM_COMMAND_SIG(long_paste_and_indent)
CUSTOM_DOC("Paste from the top of clipboard and run auto-indent on the newly pasted text.")
{
    paste(app);
    Long_Indent_CursorRange(app, 1, 1);
}

function void Long_SelectCurrentLine(Application_Links* app, b32 toggle)
{
    View_ID view = get_active_view(app, Access_Read);
    i64 cursor_pos = view_get_cursor_pos(app, view);
    i64 line = get_line_number_from_pos(app, view_get_buffer(app, view, Access_Always), cursor_pos);
    
    if (line > 1)
    {
        view_set_cursor(app, view, seek_line_col(line - 1, 1));
        seek_end_of_line(app);
        i64 old_mark = view_get_mark_pos(app, view);
        i64 new_mark = view_get_cursor_pos(app, view);
        view_set_mark(app, view, seek_pos(new_mark));
        
        view_set_cursor(app, view, seek_pos(cursor_pos));
        seek_end_of_line(app);
        no_mark_snap_to_cursor(app, view);
        if (!toggle || old_mark != new_mark || cursor_pos != view_get_cursor_pos(app, view))
            return;
    }
    
    f4_home_first_non_whitespace(app);
    if (view_get_mark_pos(app, view) == view_get_cursor_pos(app, view))
        f4_home_first_non_whitespace(app);
    view_set_mark(app, view, seek_pos(view_get_cursor_pos(app, view)));
    seek_end_of_line(app);
    no_mark_snap_to_cursor(app, view);
}

CUSTOM_COMMAND_SIG(long_copy_line)
CUSTOM_DOC("Copy the text in the current line onto the clipboard.")
{
    Long_SelectCurrentLine(app, 0);
    copy(app);
}

CUSTOM_COMMAND_SIG(long_cut_line)
CUSTOM_DOC("Cut the text in the current line onto the clipboard.")
{
    Long_SelectCurrentLine(app, 0);
    cut(app);
}

CUSTOM_COMMAND_SIG(long_copy_token)
CUSTOM_DOC("Copy the token that the cursor is currently on onto the clipboard.")
{
    long_select_current_token(app);
    copy(app);
}

CUSTOM_COMMAND_SIG(long_cut_token)
CUSTOM_DOC("Cut the token that the cursor is currently on onto the clipboard.")
{
    long_select_current_token(app);
    cut(app);
}

//~ NOTE(long): Move Commands

//- NOTE(long): Select
CUSTOM_COMMAND_SIG(long_select_current_line)
CUSTOM_DOC("Set the mark to the end of the previous line and cursor to the end of the current line")
{
    Long_SelectCurrentLine(app, 1);
}

function void Long_Scan_Move(Application_Links *app, Scan_Direction direction, Boundary_Function_List funcs, b32 snap_mark = 0)
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    i64 cursor_pos = view_get_cursor_pos(app, view);
    i64 pos = scan(app, funcs, buffer, direction, cursor_pos);
    view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
    if (!snap_mark)
        no_mark_snap_to_cursor_if_shift(app, view);
}

CUSTOM_COMMAND_SIG(long_select_current_token)
CUSTOM_DOC("Set the mark and cursor to the current token's boundary.")
{
    Scratch_Block scratch(app);
    View_ID view = get_active_view(app, Access_ReadVisible);
    i64 pos = view_get_cursor_pos(app, view);
    
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    Token_Array array = get_token_array_from_buffer(app, buffer);
    Token* token = get_token_from_pos(app, &array, pos);
    if (token)
    {
        REPEAT:
        if (token->kind == TokenBaseKind_Whitespace ||
            token->kind == TokenBaseKind_ParentheticalOpen ||
            token->kind == TokenBaseKind_ParentheticalClose)
        {
            i64 line = get_line_number_from_pos(app, buffer, token->pos);
            i64 prev_line = get_line_number_from_pos(app, buffer, token->pos - 1);
            if (token != array.tokens && (token == &array.tokens[array.count - 2] || line == prev_line))
                token = get_token_from_pos(app, buffer, token->pos - 1);
            else
                token = get_token_from_pos(app, buffer, token->pos + token->size);
        }
        else if (token->kind == TokenBaseKind_EOF)
        {
            if (token != array.tokens)
                token--;
            Assert(token->kind != TokenBaseKind_EOF);
            goto REPEAT;
        }
        
        view_set_cursor_and_preferred_x(app, view, seek_pos(token->pos));
        view_set_mark(app, view, seek_pos(token->pos + token->size));
        no_mark_snap_to_cursor(app, view);
    }
}

CUSTOM_COMMAND_SIG(long_clean_whitespace_at_cursor)
CUSTOM_DOC("Removes all trailing and leading whitespace at the cursor")
{
    View_ID view = get_active_view(app, 0);
    i64 pos = view_get_cursor_pos(app, view);
    Buffer_ID buffer = view_get_buffer(app, view, 0);
    Range_i64 range = {};
    
    String_Match left  = buffer_seek_character_class(app, buffer, &character_predicate_non_whitespace, Scan_Backward, pos);
    String_Match right = buffer_seek_character_class(app, buffer, &character_predicate_non_whitespace, Scan_Forward , pos - 1);
    if (left.buffer = right.buffer)
        range = { left.range.max, right.range.min };
    Range_i64 line_range = get_line_range_from_pos_range(app, buffer, range);
    
    if (range_size(range) > 1 && range_size(line_range) == 0)
        buffer_replace_range(app, buffer, range, S8Lit(" "));
}

//- NOTE(long): Index
function i64 Long_Boundary(Application_Links *app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos)
{
    i64 result = boundary_non_whitespace(app, buffer, side, direction, pos);
    Token_Array tokens = get_token_array_from_buffer(app, buffer);
    if (tokens.tokens)
    {
        switch (direction)
        {
            case Scan_Forward:
            {
                i64 buffer_size = buffer_get_size(app, buffer);
                result = buffer_size;
                if (tokens.count > 0)
                {
                    Token_Iterator_Array it = token_iterator_pos(0, &tokens, pos);
                    result = it.ptr->pos + it.ptr->size;
                }
            } break;
            
            case Scan_Backward:
            {
                result = 0;
                if (tokens.count > 0)
                {
                    Token_Iterator_Array it = token_iterator_pos(0, &tokens, pos);
                    if (it.ptr->pos == pos)
                        token_it_dec_all(&it);
                    result = it.ptr->pos;
                }
            } break;
        }
    }
    return(result);
}

CUSTOM_COMMAND_SIG(long_move_right_boundary)
CUSTOM_DOC("Seek right for the next end of a token.")
{
    Scratch_Block scratch(app);
    Long_Scan_Move(app, Scan_Forward, push_boundary_list(scratch, Long_Boundary));
}

CUSTOM_COMMAND_SIG(long_move_left_boundary)
CUSTOM_DOC("Seek left for the next beginning of a token.")
{
    Scratch_Block scratch(app);
    Long_Scan_Move(app, Scan_Backward, push_boundary_list(scratch, Long_Boundary));
}

function i64 Long_Boundary_AlphaNumericCamel(Application_Links *app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos)
{
    i64 an_pos = boundary_alpha_numeric(app, buffer, side, direction, pos);
    String_Match m = buffer_seek_character_class(app, buffer, &character_predicate_uppercase, direction, pos);
    i64 cap_pos = m.range.min;
    
    {
        i64 an_next_pos = boundary_alpha_numeric(app, buffer, flip_side(side), flip_direction(direction), an_pos);
        if (direction == Scan_Backward && an_next_pos < pos)
            an_pos = Max(an_pos, an_next_pos);
        else if (direction == Scan_Forward && an_next_pos > pos)
            an_pos = Min(an_pos, an_next_pos);
    }
    
    i64 result = 0;
    if (direction == Scan_Backward)
        result = Max(an_pos, cap_pos);
    else
        result = Min(an_pos, cap_pos);
    return result;
}

CUSTOM_COMMAND_SIG(long_move_right_alpha_numeric_or_camel_boundary)
CUSTOM_DOC("Seek right for boundary between alphanumeric characters or camel case word and non-alphanumeric characters.")
{
    Scratch_Block scratch(app);
    Long_Scan_Move(app, Scan_Forward, push_boundary_list(scratch, Long_Boundary_AlphaNumericCamel));
}

CUSTOM_COMMAND_SIG(long_move_left_alpha_numeric_or_camel_boundary)
CUSTOM_DOC("Seek left for boundary between alphanumeric characters or camel case word and non-alphanumeric characters.")
{
    Scratch_Block scratch(app);
    Long_Scan_Move(app, Scan_Backward, push_boundary_list(scratch, Long_Boundary_AlphaNumericCamel));
}

function F4_Index_Note* Long_Scan_Note(Scan_Direction dir, F4_Index_Note* note, NoteFilter* filter, i64 pos)
{
    F4_Index_Note* result = 0;
    
    if (!filter || filter(note))
    {
        switch (dir)
        {
            case Scan_Forward:  { if (note->range.min > pos) result = note; } break;
            case Scan_Backward: { if (note->range.min < pos) result = note; } break;
        }
    }
    
    Long_Index_IterateValidNoteInFile_Dir(child, note, dir == Scan_Forward)
    {
        F4_Index_Note* scan_note = Long_Scan_Note(dir, child, filter, pos);
        if (scan_note)
        {
            if (dir == Scan_Backward || !result)
                result = scan_note;
            break;
        }
    }
    
    return result;
}

function i64 Long_Boundary_FunctionAndType(Application_Links *app, Buffer_ID buffer, Side side, Scan_Direction dir, i64 pos)
{
    F4_Index_File* file = F4_Index_LookupFile(app, buffer);
    if (file)
    {
        for (F4_Index_Note* note = dir == Scan_Forward ? file->first_note : file->last_note; note;
             note = dir == Scan_Forward ? note->next_sibling : note->prev_sibling)
        {
            F4_Index_Note* scan_note = Long_Scan_Note(dir, note, Long_Filter_FunctionAndType, pos);
            if (scan_note)
            {
                pos = scan_note->range.min;
                break;
            }
        }
    }
    return pos;
}

CUSTOM_COMMAND_SIG(long_move_to_next_function_and_type)
CUSTOM_DOC("Seek right for the next function or type in the buffer.")
{
    Scratch_Block scratch(app);
    Long_Scan_Move(app, Scan_Forward, push_boundary_list(scratch, Long_Boundary_FunctionAndType), 1);
}

CUSTOM_COMMAND_SIG(long_move_to_prev_function_and_type)
CUSTOM_DOC("Seek left for the previous function or type in the buffer.")
{
    Scratch_Block scratch(app);
    Long_Scan_Move(app, Scan_Backward, push_boundary_list(scratch, Long_Boundary_FunctionAndType), 1);
}

// @COPYPASTA(long):
// https://github.com/Jack-Punter/4coder_punter/blob/0b43bad07998132e76d7094ed7ee385151a52ab7/4coder_fleury_base_commands.cpp#L651
function i64 F4_Boundary_CursorToken(Application_Links *app, Buffer_ID buffer, 
                                     Side side, Scan_Direction direction, i64 pos)
{
    Scratch_Block scratch(app);
    
    //-
    // NOTE(jack): Iterate the code index file for the buffer to find the search range for tokens
    // The search range_bounds should contain the function body, and its parameter list
    Code_Index_File *file = code_index_get_file(buffer);
    Range_i64 search_bounds = {};
    if (file) 
    {
        Code_Index_Nest_Ptr_Array file_nests = file->nest_array;
        Code_Index_Nest *prev_important_nest = 0;
        
        for (Code_Index_Nest *nest = file->nest_list.first; 
             nest != 0; 
             nest = nest->next)
        {
            if (range_contains(Range_i64{nest->open.start, nest->close.end}, pos))
            {
                switch (nest->kind)
                {
                    case CodeIndexNest_Paren:
                    {
                        // NOTE(jack): Iterate to the next scope nest. We occasionally see CodeIndexNest_Preprocessor or 
                        // CodeIndexNest_Statement types between function parameter lists and the fucntion body
                        Code_Index_Nest *funciton_body_nest = nest->next;
                        while(funciton_body_nest && funciton_body_nest ->kind != CodeIndexNest_Scope) {
                            funciton_body_nest = funciton_body_nest->next;
                        }
                        
                        search_bounds.start = nest->open.start;
                        search_bounds.end = (funciton_body_nest ?
                                             funciton_body_nest->close.end : 
                                             nest->close.end);
                    } break; 
                    
                    case CodeIndexNest_Scope:
                    {
                        search_bounds.start = (prev_important_nest ? 
                                               prev_important_nest->open.start :
                                               nest->open.start);
                        search_bounds.end = nest->close.end;
                    } break;
                }
            }
            
            // NOTE(jack): Keep track of the most recent Paren scope (parameter list) so that we can use it directly
            // if the cursor is within a function body.
            if (nest->kind == CodeIndexNest_Paren) 
            {
                prev_important_nest = nest;
            }
        }
    }
    
    //-
    View_ID view = get_active_view(app, Access_Always);
    i64 active_cursor_pos = view_get_cursor_pos(app, view);
    Token_Array tokens = get_token_array_from_buffer(app, buffer);
    Token_Iterator_Array active_cursor_it = token_iterator_pos(0, &tokens, active_cursor_pos);
    Token *active_cursor_token = token_it_read(&active_cursor_it);
    String_Const_u8 cursor_string = push_buffer_range(app, scratch, buffer, Ii64(active_cursor_token));
    i64 cursor_offset = pos - active_cursor_token->pos;
    
    //-
    // NOTE(jack): Loop to find the next cursor token occurance in the search_bounds. 
    // (only if we are on an identifier and have valid search bounds.
    i64 result = pos;
    if (tokens.tokens != 0 &&
        active_cursor_token->kind == TokenBaseKind_Identifier &&
        range_contains(search_bounds, pos))
    {
        Token_Iterator_Array it = token_iterator_pos(0, &tokens, pos);
        for (;;)
        {
            b32 out_of_file = false;
            switch (direction)
            {
                case Scan_Forward:
                {
                    out_of_file = !token_it_inc_non_whitespace(&it);
                } break;
                
                case Scan_Backward:
                {
                    out_of_file = !token_it_dec_non_whitespace(&it);
                } break;
            }
            
            if (out_of_file || !range_contains(search_bounds, token_it_read(&it)->pos))
            {
                break;
            }
            else
            {
                Token *token = token_it_read(&it);
                // NOTE(jack): Only push the token string and compare if the token is an identifier.
                if (token->kind == TokenBaseKind_Identifier) 
                {
                    String_Const_u8 token_string = push_buffer_range(app, scratch, buffer, Ii64(token));
                    if (string_match(cursor_string, token_string))
                    {
                        result = token->pos + cursor_offset;
                        break;
                    }
                }
            }
        }
    }
    return result ;
}

CUSTOM_COMMAND_SIG(long_move_up_token_occurrence)
CUSTOM_DOC("Moves the cursor to the previous occurrence of the token that the cursor is over.")
{
    Scratch_Block scratch(app);
    Long_Scan_Move(app, Scan_Backward, push_boundary_list(scratch, F4_Boundary_CursorToken));
}

CUSTOM_COMMAND_SIG(long_move_down_token_occurrence)
CUSTOM_DOC("Moves the cursor to the next occurrence of the token that the cursor is over.")
{
    Scratch_Block scratch(app);
    Long_Scan_Move(app, Scan_Forward, push_boundary_list(scratch, F4_Boundary_CursorToken));
}

//- NOTE(long): Scope
// @COPYPASTA(long): find_nest_side
function b32 Long_FindNestSide(Application_Links *app, Buffer_ID buffer, i64 pos,
                               Find_Nest_Flag flags, Scan_Direction scan, Nest_Delimiter_Kind delim, Range_i64 *out)
{
    b32 result = false;
    
    b32 balanced = HasFlag(flags, FindNest_Balanced);
    if (balanced &&
        (delim == NestDelim_Open  && scan == Scan_Forward) ||
        (delim == NestDelim_Close && scan == Scan_Backward))
        balanced = false;
    
    b32 (*next_token)(Token_Iterator_Array* it) = token_it_inc;
    if (scan == Scan_Backward) next_token = token_it_dec;
    
    Token_Array* tokens = scope_attachment(app, buffer_get_managed_scope(app, buffer), attachment_tokens, Token_Array);
    if (tokens != 0 && tokens->count > 0)
    {
        Token_Iterator_Array it = token_iterator_pos(0, tokens, pos);
        i32 level = 0;
        for (;;)
        {
            Token* token = token_it_read(&it);
            Nest_Delimiter_Kind token_delim = get_nest_delimiter_kind(token->kind, flags);
            
            if (level == 0 && token_delim == delim)
            {
                SUCCESS:
                *out = Ii64(token);
                result = true;
                break;
            }
            
            if (balanced && token_delim != NestDelim_None)
            {
                level += (token_delim == delim)?-1:1;
                if (level == 0 && token_delim == delim)
                    goto SUCCESS;
            }
            
            if (!next_token(&it))
                break;
        }
    }
    
    return result;
}

// @COPYPASTA(long): select_next_scope_after_pos
function Range_i64 Long_GetNextScopeAfterPos(Application_Links* app, View_ID view, Buffer_ID buffer, i64 pos)
{
    Range_i64 range;
    Find_Nest_Flag flags = FindNest_Scope;
    if (find_nest_side(app, buffer, pos + 1, flags, Scan_Forward, NestDelim_Open, &range) &&
        find_nest_side(app, buffer, range.end, flags|FindNest_Balanced|FindNest_EndOfToken, Scan_Forward, NestDelim_Close, &range.end))
        return range;
    return {};
}

function Range_i64 Long_GetScopeFromPos(Application_Links* app, View_ID view, Buffer_ID buffer, i64 pos, Scan_Direction dir)
{
    Range_i64 result = {}, range;
    Find_Nest_Flag flags = FindNest_Scope|FindNest_Balanced;
    
    // NOTE(long): This will either get the prev/next scope in the same level or the parent scope
    if (dir == Scan_Backward)
    {
        if (Long_FindNestSide(app, buffer, pos - 1, flags, Scan_Backward, NestDelim_Open, &range) &&
            find_nest_side(app, buffer, range.max, flags|FindNest_EndOfToken, Scan_Forward, NestDelim_Close, &range.max))
            result = range;
    }
    else
    {
        i64 search_pos = pos;
        FIND_NEXT:
        if (Long_FindNestSide(app, buffer, search_pos, flags, Scan_Forward, NestDelim_Close, &range) &&
            find_nest_side(app, buffer, range.min - 1, flags, Scan_Backward, NestDelim_Open, &range.min))
        {
            result = range;
            // NOTE(long): In case the cursor has already selected this scope, find the next scope.
            // We should use range_is_scope_selection for correctness, but that requires passing in the mark pos.
            if (pos == range.min)
            {
                search_pos = range.max;
                goto FIND_NEXT;
            }
        }
    }
    
    return result;
}

function void Long_SelectScopeSameLevel(Application_Links* app, View_ID view, Buffer_ID buffer, i64 cursor, i64 mark, Scan_Direction dir)
{
    Range_i64 range = Long_GetScopeFromPos(app, view, buffer, cursor, dir);
    if (range == Range_i64{}) return;
    if (range_is_scope_selection(app, buffer, Ii64(cursor, mark)) && range_contains( { range.min + 1, range.max }, cursor)) return;
    select_scope(app, view, range);
}

CUSTOM_COMMAND_SIG(long_select_prev_scope_current_level)
CUSTOM_DOC("If a scope is selected, find the prev scope that starts before the selected scope on the same level. Otherwise, find the first scope that starts before the cursor.")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    i64 pos = view_get_cursor_pos(app, view);
    Long_SelectScopeSameLevel(app, view, buffer, pos, view_get_mark_pos(app, view), Scan_Backward);
}

CUSTOM_COMMAND_SIG(long_select_next_scope_current_level)
CUSTOM_DOC("If a scope is selected, find the next scope that starts after the selected scope on the same level. Otherwise, find the first scope that starts before the cursor.")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    i64 pos = view_get_cursor_pos(app, view);
    Long_SelectScopeSameLevel(app, view, buffer, pos, view_get_mark_pos(app, view), Scan_Forward);
}

CUSTOM_COMMAND_SIG(long_select_upper_scope)
CUSTOM_DOC("Select the surrounding scope.")
{
    select_surrounding_scope(app);
}

CUSTOM_COMMAND_SIG(long_select_lower_scope)
CUSTOM_DOC("Find the first child scope that starts inside the current selected scope or surrounding scope.")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    i64 pos = view_get_cursor_pos(app, view);
    
    Range_i64 surrounding_nest;
    if (range_is_scope_selection(app, buffer, surrounding_nest = Ii64(pos, view_get_mark_pos(app, view))));
    else if (find_surrounding_nest(app, buffer, pos, FindNest_Scope, &surrounding_nest));
    else surrounding_nest = {};
    
    Range_i64 range = Long_GetNextScopeAfterPos(app, view, buffer, pos);
    if (!surrounding_nest.max || range.min < surrounding_nest.max)
        select_scope(app, view, range);
}

CUSTOM_COMMAND_SIG(long_select_surrounding_scope)
CUSTOM_DOC("Select the surrounding scope.")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    
    i64 cursor = view_get_cursor_pos(app, view), mark = view_get_mark_pos(app, view);
    i64 advance = cursor < mark ? 1 : -1;
    Range_i64 range = Ii64(cursor, mark);
    
    if (range_is_scope_selection(app, buffer, range))
    {
        view_set_cursor(app, view, seek_pos(cursor + advance));
        view_set_mark  (app, view, seek_pos(mark   - advance));
    }
    else if (range_is_scope_selection(app, buffer, { range.min - 1, range.max + 1 }))
    {
        view_set_cursor(app, view, seek_pos(cursor - advance));
        view_set_mark  (app, view, seek_pos(mark   + advance));
    }
    else
        select_surrounding_scope(app);
}

//~ NOTE(long): Misc Commands

//- NOTE(long): Command
CUSTOM_COMMAND_SIG(long_test_lister_render)
CUSTOM_DOC("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec placerat tellus vitae feugiat tincidunt. Suspendisse sagittis velit porttitor justo commodo sagittis. Etiam erat metus, elementum eu aliquam et, dictum at eros. Vivamus nulla ex, gravida malesuada iaculis id, maximus at quam. Fusce sodales, velit id varius rhoncus, odio ipsum placerat eros, nec euismod dui nunc in mauris. Donec quis commodo enim. Etiam sed efficitur elit, in interdum lacus. Vivamus sollicitudin hendrerit lacinia. Suspendisse aliquet bibendum nunc, eget fermentum quam feugiat ac. Sed at fringilla neque, eu aliquam risus. Donec ut ante eu erat cursus semper eget et velit. Quisque ut aliquam nibh. Curabitur et justo hendrerit, finibus sapien quis, fringilla ante. Nullam vehicula, nisi in facilisis egestas, tellus nunc faucibus lacus, tempor aliquet nulla felis sit amet felis. Nam non vulputate elit.\n\nVestibulum volutpat est vel felis tincidunt, sed imperdiet neque feugiat. Integer placerat dignissim eros, in sollicitudin lacus venenatis varius. Maecenas in feugiat ex. Nunc elementum sem est, sodales facilisis ligula hendrerit interdum. Integer pulvinar orci eget ipsum porta dapibus. Pellentesque sapien eros, semper sit amet placerat a, viverra malesuada nulla. Donec cursus turpis ut metus auctor pellentesque. Etiam dolor dui, maximus vitae malesuada ac, tincidunt eu tortor. Nullam felis ante, varius elementum mattis nec, pretium in diam.\n\nUt ut malesuada justo. Donec consequat magna sed diam feugiat pellentesque. Duis quis tempus tortor. Donec vulputate ullamcorper massa, eget porta metus ultrices nec. Cras dignissim dictum blandit. Nullam pellentesque volutpat purus vitae bibendum. Aenean quis neque eget orci imperdiet lacinia id finibus lorem. Interdum et malesuada fames ac ante ipsum primis in faucibus. Interdum et malesuada fames ac ante ipsum primis in faucibus. Praesent dictum lectus a ligula venenatis, nec ultricies turpis placerat. Proin euismod ut odio eu luctus. Vivamus eleifend eros sit amet felis dapibus, ac tempus est feugiat. Vivamus sit amet quam id lorem commodo volutpat. Maecenas ac nulla non turpis euismod vestibulum eget vitae tortor.\n\nMauris venenatis nunc ac enim fringilla, vitae varius neque imperdiet. Duis odio purus, commodo in dolor in, mollis malesuada tellus. Duis pharetra vulputate mauris ut cursus. Cras non eros feugiat, lacinia augue ut, tincidunt arcu. Donec pulvinar pulvinar lorem, vel sollicitudin arcu commodo ac. Sed facilisis lorem elit, sit amet dapibus urna varius elementum. Cras at viverra urna, eu vehicula ligula. Etiam ut convallis magna. Suspendisse feugiat quam sit amet accumsan aliquet. Pellentesque vestibulum sapien ut urna sollicitudin consequat. Duis non ullamcorper nibh.\n\nNullam hendrerit, sem et dictum faucibus, neque purus tristique ligula, eu sollicitudin arcu orci in magna. Vivamus auctor, enim varius ornare mattis, dolor magna condimentum enim, nec convallis sem velit nec augue. Pellentesque rutrum mauris ut nulla consectetur condimentum. Aliquam nec massa eu metus sollicitudin tincidunt. Donec lobortis ultricies sem id pretium. Donec quis felis vel ante fermentum pretium. Nulla at mi sit amet ex molestie imperdiet a eget lacus. Nullam rutrum aliquet tellus interdum bibendum.")
{
}

function Custom_Command_Function* Long_Query_User_Command(Application_Links* app, String8 query, Command_Lister_Status_Rule* rule)
{
    Scratch_Block scratch(app);
    Lister_Block lister(app, scratch);
    lister_set_query(lister, query);
    lister_set_default_handlers(lister);
    
    for (i32 i = 0; i < command_one_past_last_id; i += 1)
    {
        i32 j = clamp(0, i, command_one_past_last_id);
        
        Custom_Command_Function* proc = fcoder_metacmd_table[j].proc;
        
        Command_Trigger_List triggers = map_get_triggers_recursive(scratch, rule->mapping, rule->map_id, proc);
        String8List list = {};
        
        for (Command_Trigger* node = triggers.first; node != 0; node = node->next)
        {
            command_trigger_stringize(scratch, &list, node);
            if (node->next != 0)
                string_list_push(scratch, &list, S8Lit(" "));
        }
        
        String8 name = SCu8(fcoder_metacmd_table[j].name, fcoder_metacmd_table[j].name_len);
        String8 status = string_list_flatten(scratch, list);
        // NOTE(long): fcoder_metacmd_table[j].description_len isn't escaped correctly
        // (e.g "\n" is 2 bytes rather than 1) but is probably null-terminated.
        String8 tooltip = SCu8(fcoder_metacmd_table[j].description);
        Long_Lister_AddItem(app, lister, name, status, 0, 0, PtrAsInt(proc), tooltip);
    }
    
    Lister_Result l_result = run_lister(app, lister);
    Custom_Command_Function* result = 0;
    if (Long_Lister_IsResultValid(l_result))
        result = (Custom_Command_Function*)IntAsPtr(((Long_Lister_Data*)l_result.user_data)->user_index);
    return result;
}

CUSTOM_UI_COMMAND_SIG(long_command_lister)
CUSTOM_DOC("Opens an interactive list of all registered commands.")
{
    View_ID view = get_this_ctx_view(app, Access_Always);
    if (view)
    {
        Command_Lister_Status_Rule rule = {};
        Buffer_ID buffer = view_get_buffer(app, view, Access_Visible);
        Managed_Scope buffer_scope = buffer_get_managed_scope(app, buffer);
        
        Command_Map_ID* map_id_ptr = scope_attachment(app, buffer_scope, buffer_map_id, Command_Map_ID);
        if (map_id_ptr)
            rule = command_lister_status_bindings(&framework_mapping, *map_id_ptr);
        else
            rule = command_lister_status_descriptions();
        
        Custom_Command_Function* func = Long_Query_User_Command(app, S8Lit("Command:"), &rule);
        if (func)
            view_enqueue_command_function(app, view, func);
    }
}

//- NOTE(long): Theme
String8 current_theme_name = {};
#define DEFAULT_THEME_NAME S8Lit("4coder")

function void Long_UpdateCurrentTheme(Color_Table* table)
{
    for (Color_Table_Node* node = global_theme_list.first; node != 0; node = node->next)
    {
        if (&node->table == table)
        {
            current_theme_name = node->name;
            return;
        }
    }
    current_theme_name = DEFAULT_THEME_NAME;
}

CUSTOM_COMMAND_SIG(long_reload_all_themes_default_folder)
CUSTOM_DOC("Clears and reloads all the theme files in the default theme folder.")
{
    Scratch_Block scratch(app);
    String8 current_name = push_string_copy(scratch, current_theme_name);
    clear_all_themes(app);
    load_themes_default_folder(app);
    
    current_theme_name = DEFAULT_THEME_NAME;
    for (Color_Table_Node* node = global_theme_list.first; node != 0; node = node->next)
    {
        if (string_match(node->name, current_name))
        {
            current_theme_name = node->name;
            active_color_table = node->table;
        }
    }
}

CUSTOM_UI_COMMAND_SIG(long_theme_lister)
CUSTOM_DOC("Opens an interactive list of all registered themes.")
{
    Color_Table* color_table = get_color_table_from_user(app);
    if (color_table != 0)
    {
        active_color_table = *color_table;
        Long_UpdateCurrentTheme(color_table);
    }
}

//- NOTE(long): Macro
CUSTOM_COMMAND_SIG(long_macro_toggle_recording)
CUSTOM_DOC("Toggle macro recording")
{
    if (global_keyboard_macro_is_recording)
        keyboard_macro_finish_recording(app);
    else
        keyboard_macro_start_recording(app);
}

CUSTOM_COMMAND_SIG(long_macro_repeat)
CUSTOM_DOC("Repeat most recently recorded keyboard macro n times.")
{
    Query_Bar_Group group(app);
    Query_Bar number_bar = {};
    number_bar.prompt = SCu8("Repeat Count: ");
    u8 number_buffer[4];
    number_bar.string.str = number_buffer;
    number_bar.string_capacity = sizeof(number_buffer);
    
    if (query_user_number(app, &number_bar))
    {
        if (number_bar.string.size > 0)
        {
            i32 repeats = (i32)string_to_integer(number_bar.string, 10);
            repeats = clamp_top(repeats, 1000);
            for (i32 i = 0; i < repeats; ++i)
                keyboard_macro_replay(app);
        }
    }
}

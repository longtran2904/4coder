
//~ NOTE(long): Helper Functions

//- NOTE(long): Math
function b32 Long_F32_Invalid(f32 f)
{
    return f == max_f32 || f == min_f32 || f == -max_f32 || f == -min_f32;
}

function b32 Long_Rf32_Invalid(Rect_f32 r)
{
    return (Long_F32_Invalid(r.x0) || Long_F32_Invalid(r.x1) || Long_F32_Invalid(r.y0) || Long_F32_Invalid(r.y1));
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
function void Long_SnapView(Application_Links* app, View_ID view)
{
    if (!view)
        view = get_active_view(app, Access_ReadVisible);
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
    scroll.position = scroll.target;
    view_set_buffer_scroll(app, view, scroll, SetBufferScroll_SnapCursorIntoView);
    no_mark_snap_to_cursor(app, view);
}

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
        *out_pos = buffer_compute_cursor(app, *out_buf, seek_line_col(jump.location.line, jump.location.column)).pos;
        result = 1;
    }
    
    return result;
}

// @COPYPASTA(long): F4_JumpToLocation
function void Long_Jump_ToLocation(Application_Links* app, View_ID view, Buffer_ID buffer, i64 pos)
{
    view_set_active(app, view);
    Buffer_Seek seek = seek_pos(pos);
    set_view_to_location(app, view, buffer, seek);
    
    if (auto_center_after_jumps)
        center_view(app);
    
    view_set_cursor(app, view, seek);
    view_set_mark(app, view, seek);
}

function void Long_Jump_ToBuffer(Application_Links* app, View_ID view, Buffer_ID buffer, b32 push_src, b32 push_dst)
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

function b32 Long_Buffer_NoSearch(Application_Links* app, Buffer_ID buffer)
{
    Scratch_Block scratch(app);
    String8 str = push_buffer_unique_name(app, scratch, buffer);
    if (str.size > 0 && str.str[0] == '*')
    {
        if (string_match(str, S8Lit("*scratch*"))) return 0;
        if (string_match(str, S8Lit("*calc*"   ))) return 0;
        return 1;
    }
    return 0;
}

//~ NOTE(long): Point Stack Commands

CUSTOM_ID(attachment, long_point_stacks);

function Long_Point_Stack* Long_GetPointStack(Application_Links* app, View_ID view = 0)
{
    if (view == 0)
        view = get_active_view(app, Access_Always);
    return scope_attachment(app, view_get_managed_scope(app, view), long_point_stacks, Long_Point_Stack);
}

function b32 Long_PointStack_Read(Application_Links* app, Long_Point_Stack* stack, i32 index, Buffer_ID* out_buffer, i64* out_pos)
{
    Marker* marker = (Marker*)managed_object_get_pointer(app, stack->markers[index]);
    if (marker)
    {
        *out_buffer = stack->buffers[index];
        *out_pos = marker->pos;
    }
    return !!marker;
}

function void Long_PointStack_Push(Application_Links* app, Buffer_ID buffer, i64 pos, View_ID view)
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

function void Long_PointStack_Append(Application_Links* app, Long_Point_Stack* stack, Buffer_ID buffer, i64 pos, View_ID view)
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

#define Long_PointStack_Iter(app, stack, start, end, advance) \
    Buffer_ID buffer = 0; i64 pos = 0;\
    for (i32 i = (start), _size_ = ArrayCount((stack)->markers); i != (end); i = clamp_loop(i + (advance), _size_)) \
    if (Long_PointStack_Read(app, (stack), i, &buffer, &pos))

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
        Long_Jump_ToLocation(app, get_this_ctx_view(app, Access_Always), result.buffer, result.pos);
        Long_PointStack_SetCurrent(stack, (i32)result.user_index);
    }
}

function void Long_PointStack_Jump(Application_Links* app, View_ID view, Buffer_ID target_buffer, i64 target_pos,
                                   Buffer_ID current_buffer, i64 current_pos)
{
    if (!current_buffer)
        current_buffer = view_get_buffer(app, view, Access_Always);
    if (!current_pos)
        current_pos = view_get_cursor_pos(app, view);
    
    Long_PointStack_Push(app, current_buffer, current_pos, view);
    Long_Jump_ToLocation(app, view, target_buffer, target_pos);
    Long_PointStack_Push(app, target_buffer, target_pos, view);
}

function void Long_PointStack_JumpNext(Application_Links* app, View_ID view, i32 advance)
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
        //advance = -1;
    }
    else if (advance > 0) end = stack->top;
    else if (advance < 0) end = stack->bot - 1;
    
    Long_PointStack_Iter(app, stack, clamp_loop(stack->current - 1, size), clamp_loop(end, size), advance)
    {
        if (buffer == current_buffer && pos == current_pos && advance == 0) break;
        
        // NOTE(long): If virtual whitespace is enabled, we can't jump to the start of the line
        // Long_IsPosValid will check for that and make sure pos is different from the current pos
        if (buffer != current_buffer || Long_IsPosValid(app, view, buffer, pos, current_pos) || advance == 0)
        {
            if (clamp_loop(i + 1, size) == stack->top && advance < 0)
            {
                i64 current_line = get_line_number_from_pos(app, current_buffer, current_pos);
                i64 line = get_line_number_from_pos(app, buffer, pos);
                if (current_buffer != buffer || Long_Abs((i32)(line - current_line)) >= 35)
                    Long_PointStack_Append(app, stack, current_buffer, current_pos, view);
            }
            
            Long_Jump_ToLocation(app, view, buffer, pos);
            Long_PointStack_SetCurrent(stack, i);
            break;
        }
    }
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

function void Long_Buffer_OutputBuffer(Application_Links* app, Lister* lister, Buffer_ID buffer)
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
    String8 filepath = Long_Prj_RelBufferName(app, scratch, buffer);
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

function Buffer_ID Long_Buffer_RunLister(Application_Links* app, char* query)
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

#ifdef FCODER_FLEURY_RECENT_FILES_H
// @COPYPASTA(long): f4_recent_files_menu
CUSTOM_UI_COMMAND_SIG(long_recent_files_menu)
CUSTOM_DOC("Lists the recent files used in the current panel.")
{
    View_ID view = get_active_view(app, Access_Read);
    Managed_Scope scope = view_get_managed_scope(app, view);
    F4_RecentFiles_ViewState* state = scope_attachment(app, scope, f4_recentfiles_viewstate, F4_RecentFiles_ViewState);
    
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
#endif

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
    // killing the search buffer is always possible because it's never dirty and always killable
    b32 killed = try_buffer_kill(app, buffer, view, 0) == BufferKillResult_Killed;
    
    if (is_search_buffer && killed && buffer_view)
    {
        Long_PointStack_JumpNext(app, buffer_view, 0);
        Long_SnapView(app, view);
        view_set_active(app, view);
    }
}

CUSTOM_UI_COMMAND_SIG(long_interactive_kill_buffer)
CUSTOM_DOC("Interactively kill an open buffer.")
{
    Long_KillBuffer(app, Long_Buffer_RunLister(app, "Kill:"), get_this_ctx_view(app, Access_Always));
}

//- NOTE(long): History
function Long_Buffer_History* Long_Buffer_GetAttachedHistory(Application_Links* app, Buffer_ID buffer)
{
    return scope_attachment(app, buffer_get_managed_scope(app, buffer), long_buffer_history, Long_Buffer_History);
}

function Long_Buffer_History Long_Buffer_GetCurrentHistory(Application_Links* app, Buffer_ID buffer, i32 offset)
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

function b32 Long_Buffer_CheckDirtyHistory(Application_Links* app, Buffer_ID buffer, i32 offset)
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
            b32 is_current = i == current;
            b32 is_saved = record.edit_number == history.record.edit_number && i == history.index;
            if (is_current && is_saved)
                tag = S8Lit("current saved");
            else if (is_current)
                tag = S8Lit("current");
            else if (is_saved)
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
            
            Long_Buffer_CheckDirtyHistory(app, buffer);
        }
    }
}

//- @COPYPASTA(long): undo__fade_finish, undo, redo, undo_all_buffers, redo_all_buffers
// Because an undo can be deferred later, I can't call CheckHistory after doing an undo.
// I must call it inside the fade_finish callback, or in the do_immedite_undo check.
// But a redo, on the other hand, always executes immediately, so I can safely call it right afterward.
// I also fixed the "getting the wrong Record_Info" bug in the redo command

function void Long_Undo_FinishFade(Application_Links* app, Fade_Range* range)
{
    History_Record_Index current = buffer_history_get_current_state_index(app, range->buffer_id);
    if (current > 0)
    {
        buffer_history_set_current_state_index(app, range->buffer_id, current - 1);
        Long_Buffer_CheckDirtyHistory(app, range->buffer_id);
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
                Fade_Range* fade = buffer_post_fade(app, buffer, undo_fade_time, range, color);
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
            Long_Buffer_CheckDirtyHistory(app, buffer);
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
    
    Long_Buffer_CheckDirtyHistory(app);
}

CUSTOM_COMMAND_SIG(long_undo_all_buffers)
CUSTOM_DOC("Advances backward through the undo history in the buffer containing the most recent regular edit.")
{
    undo_all_buffers(app);
    for (Buffer_ID buffer = get_buffer_next(app, 0, Access_Always); buffer; buffer = get_buffer_next(app, buffer, Access_Always))
        Long_Buffer_CheckDirtyHistory(app, buffer);
}

CUSTOM_COMMAND_SIG(long_redo_all_buffers)
CUSTOM_DOC("Advances forward through the undo history in the buffer containing the most recent regular edit.")
{
    redo_all_buffers(app);
    for (Buffer_ID buffer = get_buffer_next(app, 0, Access_Always); buffer; buffer = get_buffer_next(app, buffer, Access_Always))
        Long_Buffer_CheckDirtyHistory(app, buffer);
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

//- NOTE(long): Special Buffers/Panels
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
    i32 line_count = (global_compilation_view_expanded ^= 1) ? 31 : 8;
    f32 bar_height = get_face_metrics(app, get_face_id(app, 0)).line_height + 2.f;
    f32 margin_size = (f32)def_get_config_u64(app, vars_save_string_lit("f4_margin_size"));
    f32 padding = 3.f;
    
    view_set_split_pixel_size(app, global_compilation_view, (i32)(line_height * line_count + bar_height + margin_size * 2.f + padding));
    
    //if (global_compilation_view_expanded)
    {
        // @COPYPASTA(long): center_view
        Rect_f32 region = view_get_buffer_region(app, global_compilation_view);
        i64 pos = view_get_cursor_pos(app, global_compilation_view);
        Buffer_Cursor cursor = view_compute_cursor(app, global_compilation_view, seek_pos(pos));
        f32 view_height = rect_height(region);
        Buffer_Scroll scroll = view_get_buffer_scroll(app, global_compilation_view);
        scroll.target.line_number = cursor.line;
        scroll.target.pixel_shift.y = -view_height*0.5f;
        scroll.position = scroll.target;
        view_set_buffer_scroll(app, global_compilation_view, scroll, SetBufferScroll_SnapCursorIntoView);
        no_mark_snap_to_cursor(app, global_compilation_view);
    }
}

CUSTOM_COMMAND_SIG(long_toggle_panel_expand)
CUSTOM_DOC("Expand the current window.")
{
    View_ID view = get_active_view(app, 0);
    Rect_f32 old_rect = view_get_screen_rect(app, view);
    view_set_split_proportion(app, view, .5f);
    Rect_f32 new_rect = view_get_screen_rect(app, view);
    if (rect_width(new_rect) == rect_width(old_rect) && rect_height(new_rect) == rect_height(old_rect))
        view_set_split_proportion(app, view, .625f);
}

CUSTOM_COMMAND_SIG(long_toggle_panel_expand_big)
CUSTOM_DOC("Expand the current window.")
{
    View_ID view = get_active_view(app, 0);
    Rect_f32 old_rect = view_get_screen_rect(app, view);
    view_set_split_proportion(app, view, .75f);
    Rect_f32 new_rect = view_get_screen_rect(app, view);
    if (rect_width(new_rect) == rect_width(old_rect) && rect_height(new_rect) == rect_height(old_rect))
        view_set_split_proportion(app, view, .625f);
}

CUSTOM_COMMAND_SIG(long_change_active_panel)
CUSTOM_DOC("Change the currently active panel, moving to the panel with the next highest view_id.")
{
    View_ID view = get_active_view(app, Access_Always);
    if (view == global_compilation_view)
        view = long_global_active_view;
    
    view = get_next_view_looped_primary_panels(app, view, Access_Always);
    if (view)
    {
        view_set_active(app, view);
        long_global_active_view = view;
    }
}

CUSTOM_COMMAND_SIG(long_change_active_panel_backwards)
CUSTOM_DOC("Change the currently active panel, moving to the panel with the next lowest view_id.")
{
    View_ID view = get_active_view(app, Access_Always);
    if (view == global_compilation_view)
        view = long_global_active_view;
    
    view = get_prev_view_looped_primary_panels(app, view, Access_Always);
    if (view)
    {
        view_set_active(app, view);
        long_global_active_view = view;
    }
}

CUSTOM_COMMAND_SIG(long_change_to_build_panel)
CUSTOM_DOC("If the special build panel is open, makes the build panel the active panel.")
{
    View_ID view = get_or_open_build_panel(app);
    View_ID active_view = get_active_view(app, 0);
    if (view == active_view)
        view = long_global_active_view;
    view_set_active(app, view);
}

//~ NOTE(long): Search/Jump Commands

#define LONG_QUERY_STRING_SIZE KB(1)
CUSTOM_ID(attachment, long_search_string_size);
CUSTOM_ID(attachment, long_start_selection_offset);
CUSTOM_ID(attachment, long_selection_pos_offset);

// @COPYPASTA(long): create_or_switch_to_buffer_and_clear_by_name
function Buffer_ID Long_CreateOrSwitchBuffer(Application_Links* app, String_Const_u8 name_string, View_ID default_target_view)
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
    
    // TODO(long): This will overwrite the buffer's history from this point forward
    Long_Jump_ToBuffer(app, default_target_view, search_buffer, jump_to_buffer, 0);
    view_set_active(app, default_target_view);
    return(search_buffer);
}

function String_Match_List Long_FindAllMatches(Application_Links* app, Arena* arena, String_Const_u8_Array match_patterns,
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

function void Long_PrintAllMatches(Application_Links* app, String_Const_u8_Array match_patterns, String_Match_Flag must_have_flags,
                                   String_Match_Flag must_not_have_flags, Buffer_ID out_buffer_id, Buffer_ID current_buffer)
{
    Scratch_Block scratch(app);
    String_Match_List matches = Long_FindAllMatches(app, scratch, match_patterns, must_have_flags, must_not_have_flags, current_buffer);
    string_match_list_filter_remove_buffer(&matches, out_buffer_id);
    string_match_list_filter_remove_buffer_predicate(app, &matches, Long_Buffer_NoSearch);
    print_string_match_list_to_buffer(app, out_buffer_id, matches);
}

typedef i64 Line_Col_Predicate(Application_Links* app, Buffer_ID buffer, i64 line);

function void Long_PrintMatchedLines(Application_Links* app, Buffer_ID out_buffer, Buffer_ID buffer,
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
function b32 Long_Query_User_String(Application_Links* app, Query_Bar* bar, String_Const_u8 init_string){
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
            Buffer_ID buffer = view_get_buffer(app, view, 0);
            Range_i64 range;
            if (has_modifier(&in, KeyCode_Control))
                range = get_view_range(app, view);
            else
            {
                i64 pos = view_get_cursor_pos(app, view);
                range = enclose_pos_alpha_numeric_underscore_utf8(app, buffer, pos);
                
                // NOTE(long): The cursor can still be right after
                if (range.max == range.min)
                    range = enclose_pos_alpha_numeric_underscore_utf8(app, buffer, pos - 1);
            }
            
            String8 string = push_buffer_range(app, scratch, buffer, range);
            String_u8 bar_string = Su8(bar->string.str, bar->string.size, bar->string_capacity);
            string_append(&bar_string, string);
            bar->string.size = bar_string.size;
        }
        
        else if (match_key_code(&in, KeyCode_V) && has_modifier(&in, KeyCode_Control))
        {
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
            Mapping* mapping = ctx.mapping;
            Command_Map* map = mapping_get_map(mapping, ctx.map_id);
            Command_Binding binding = map_get_binding_recursive(mapping, map, &in.event);
            if (binding.custom != 0){
                Command_Metadata* metadata = get_command_metadata(binding.custom);
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

function void Long_SnapMarkToCursor(Application_Links* app)
{
    if (fcoder_mode == FCoderMode_NotepadLike)
    {
        for (View_ID view = get_view_next(app, 0, 0); view; view = get_view_next(app, view, 0))
        {
            i64 pos = view_get_cursor_pos(app, view);
            view_set_mark(app, view, seek_pos(pos));
            
            Managed_Scope scope = view_get_managed_scope(app, view);
            b32* snap_mark_to_cursor = scope_attachment(app, scope, view_snap_mark_to_cursor, b32);
            *snap_mark_to_cursor = true;
        }
    }
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
        Long_SnapMarkToCursor(app);
        
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
            Custom_Command_Function* custom = binding.custom;
            
            if (custom)
            {
                Command_Metadata* metadata = get_command_metadata(custom);
                if (metadata && metadata->is_ui)
                {
                    view_enqueue_command_function(app, view, custom);
                    break;
                }
                custom(app);
            }
            else leave_current_input_unhandled(app);
        }
        
        if (change_string)
        {
            Range_i32 select_range = Long_Highlight_GetRange(jump_state, *selection_offset);
            
            Batch_Edit* batch_head = 0;
            Batch_Edit* batch_tail = 0;
            Buffer_ID current_buffer = 0;
            
            for (i32 i = select_range.min; i <= select_range.max; ++i)
            {
                ID_Pos_Jump_Location location;
                if (get_jump_from_list(app, jump_state.list, i, &location))
                {
                    Batch_Edit* batch = push_array(scratch, Batch_Edit, 1);
                    batch->edit.text = string;
                    batch->edit.range = Ii64_size(location.pos + *offset, *size);
                    
                    if (location.buffer_id != current_buffer)
                    {
                        if (batch_head)
                        {
                            buffer_batch_edit(app, current_buffer, batch_head);
                            if (!has_modified_string)
                                start_history = Long_Buffer_GetCurrentHistory(app, current_buffer);
                        }
                        batch_head = batch_tail = 0;
                        current_buffer = location.buffer_id;
                    }
                    
                    sll_queue_push(batch_head, batch_tail, batch);
                    
                    if (i == select_range.max && batch_head)
                    {
                        buffer_batch_edit(app, current_buffer, batch_head);
                        if (!start_history.index)
                            start_history = Long_Buffer_GetCurrentHistory(app, current_buffer);
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
        for (Buffer_ID buff = get_buffer_next(app, 0, Access_Always); buff; buff = get_buffer_next(app, buff, Access_Always))
        {
            History_Record_Index last_index = buffer_history_get_current_state_index(app, buff);
            for (History_Record_Index first_index = 1; first_index <= last_index; ++first_index)
            {
                Record_Info record = buffer_history_get_record_info(app, buff, first_index);
                
                if (record.edit_number == start_history.record.edit_number)
                {
                    if (abort)
                    {
                        buffer_history_set_current_state_index(app, buff, first_index - 1);
                        buffer_history_clear_after_current_state(app, buff);
                        Long_Buffer_CheckDirtyHistory(app, buff);
                    }
                    else if (first_index < last_index)
                        buffer_history_merge_record_range(app, buff, first_index, last_index, 0);
                    
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
            Long_PointStack_JumpNext(app, view, 0);
        else
            Long_Jump_ToLocation(app, view, buffer, cursor_pos);
        view_set_mark(app, view, seek_pos(mark_pos));
    }
    
    auto_center_after_jumps = true;
    view_disable_highlight_range(app, view); // NOTE(long): Disable the highlight will make F4_RenderBuffer draw the cursor again
    view_set_camera_bounds(app, view, old_margin, old_push_in);
}

function void Long_ListAllLocations(Application_Links* app, String_Const_u8 needle, List_All_Locations_Flag flags, b32 all_buffer)
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
function String8 Long_Get_Query_String(Application_Links* app, char* query_str, u8* string_space, i32 space_size, u64 init_size = 0)
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

function void Long_ListAllLocations_Query(Application_Links* app, char* query, List_All_Locations_Flag flags, b32 all_buffer)
{
    Scratch_Block scratch(app);
    u8* space = push_array(scratch, u8, LONG_QUERY_STRING_SIZE);
    String8 needle = fcoder_mode == FCoderMode_NotepadLike ? push_view_range_string(app, scratch) : String8{};
    i64 size = Min(needle.size, LONG_QUERY_STRING_SIZE);
    block_copy(space, needle.str, size);
    needle = Long_Get_Query_String(app, "List Locations For: ", space, LONG_QUERY_STRING_SIZE, size);
    
    Long_ListAllLocations(app, needle, flags, all_buffer);
}

function void Long_ListAllLocations_Identifier(Application_Links* app, List_All_Locations_Flag flags, b32 all_buffer)
{
    Scratch_Block scratch(app);
    //String_Const_u8 needle = push_token_or_word_under_active_cursor(app, scratch);
    View_ID view = get_active_view(app, 0);
    Buffer_ID buffer = view_get_buffer(app, view, 0);
    i64 pos = view_get_cursor_pos(app, view);
    String_Const_u8 needle = push_enclose_range_at_pos(app, scratch, buffer, pos, enclose_alpha_numeric_underscore);
    Long_ListAllLocations(app, needle, flags, all_buffer);
}

function void Long_ListAllLines_InRange(Application_Links* app, Range_i64 range, Line_Col_Predicate* predicate = 0, i64 size = 0)
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
            String8 select_string = push_buffer_range(app, scratch, buffer, range);
            if (select_string.size)
            {
                String_u8 bar_string = Su8(bar.string, sizeof(bar_string_space));
                string_append(&bar_string, select_string);
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
            String8 clipboard = push_clipboard_index(scratch, 0, 0);
            if (clipboard.size)
            {
                String_u8 bar_string = Su8(bar.string, sizeof(bar_string_space));
                string_append(&bar_string, clipboard);
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
function void Long_Search(Application_Links* app, Scan_Direction dir, b32 insensitive)
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
        String_Const_u8 query_init = string_u8_empty;
        if (fcoder_mode == FCoderMode_NotepadLike && cursor != mark /*&& cursor_line == mark_line*/)
        {
            if (cursor > mark)
                Swap(i64, cursor, mark);
            query_init = push_buffer_range(app, scratch, buffer, Ii64(cursor, mark));
        }
        Long_ISearch(app, dir, cursor, query_init, insensitive);
    }
}

// @COPYPASTA(long): isearch_identifier
function void Long_Search_Identifier(Application_Links* app, Scan_Direction scan, b32 insensitive)
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

function void Long_Query_Replace(Application_Links* app, String_Const_u8 replace_str, i64 start_pos, b32 add_replace_query_bar)
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

//- NOTE(long): Code Peeking
CUSTOM_COMMAND_SIG(long_code_peek)
CUSTOM_DOC("Toggles code peek.")
{
    // @COPYPASTA(long): f4_code_peek
    global_code_peek_open ^= 1;
}

// TODO(long): f4_code_peek_yank/f4_code_peek_clear

//- NOTE(long): Positional Context
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
            Long_PointStack_Jump(app, goto_view, jump_buffer, jump_pos);
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
        // For simplicity, I just use Long_PointStack_Jump (which calls F4_JumpToLocation) for all jumping behaviors
        // And if I want to snap the view directly, I just call Long_SnapView
        Long_PointStack_Jump(app, goto_view, note->file->buffer, note->range.min);
        if (!same_panel)
            Long_SnapView(app, view);
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
        Long_PointStack_Jump(app, view, result.buffer, result.pos);
        Long_SnapView(app, view);
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
        for (F4_Index_Note* note = file ? file->first_note : 0; note; note = note->next_sibling)
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

//~ NOTE(long): Navigate Commands

//- NOTE(long): Cursor/Mark
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

// @COPYPASTA(long): delete_range except not setting the buffer to dirty when the range is empty
CUSTOM_COMMAND_SIG(long_delete_range)
CUSTOM_DOC("Deletes the text in the range between the cursor and the mark.")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    Range_i64 range = get_view_range(app, view);
    if (range_size(range))
        buffer_replace_range(app, buffer, range, string_u8_empty);
}

CUSTOM_COMMAND_SIG(long_paste_and_indent)
CUSTOM_DOC("Paste from the top of clipboard and run auto-indent on the newly pasted text.")
{
    paste(app);
    Long_Indent_CursorRange(app, 1, 1);
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

//- NOTE(long): Line
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

CUSTOM_COMMAND_SIG(long_select_current_line)
CUSTOM_DOC("Set the mark to the end of the previous line and cursor to the end of the current line")
{
    Long_SelectCurrentLine(app, 1);
}

//- NOTE(long): Token
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

function Side Long_Scan_GetSide(Scan_Direction direction)
{
    Side  forward_side = long_global_move_side  & 1;
    Side backward_side = long_global_move_side >> 1;
    return direction == Scan_Forward ? forward_side : backward_side;
}

// @COPYPASTA(long): current_view_scan_move
function void Long_Scan_Move(Application_Links* app, Side side, Scan_Direction direction, Boundary_Function func)
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    i64 cursor_pos = view_get_cursor_pos(app, view);
    i64 pos = func(app, buffer, side, direction, cursor_pos);
    view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
    no_mark_snap_to_cursor_if_shift(app, view);
}

function void Long_Scan_Move(Application_Links* app, Scan_Direction direction, Boundary_Function func)
{
    Long_Scan_Move(app, Long_Scan_GetSide(direction), direction, func);
}

// @COPYPASTA(long): current_view_boundary_delete
function void Long_Scan_Delete(Application_Links* app, Side side, Scan_Direction direction, Boundary_Function func)
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    Range_i64 range = {};
    range.first = view_get_cursor_pos(app, view);
    range.one_past_last = func(app, buffer, side, direction, range.first);
    range = rectify(range);
    buffer_replace_range(app, buffer, range, string_u8_empty);
}

function void Long_Scan_Delete(Application_Links* app, Scan_Direction direction, Boundary_Function func)
{
    Long_Scan_Delete(app, Long_Scan_GetSide(direction), direction, func);
}

function i64 Long_Boundary_TokenAndWhitespace(Application_Links* app, Buffer_ID buffer, 
                                              Side side, Scan_Direction direction, i64 pos)
{
    /*
    **  f4_move_left_token_boundary: |void|* |alloc|(|intptr_t |size|) |{  move-to-prev-word-start
    ** f4_move_right_token_boundary:  void|* |alloc|(|intptr_t |size|) |{| move-to-next-word-start
    
    **  move_left_whitespace_or_token_boundary: |void|* |alloc|(|intptr_t |size|) |{  move-to-prev-word-start
    ** move_right_whitespace_or_token_boundary:  void|*| alloc|(|intptr_t| size|)| {| move-to-next-word-end
    
    ** You can use this function as your test code.
    */
    
    i64 result = direction == Scan_Forward ? buffer_get_size(app, buffer) : 0;
    Token_Array tokens = get_token_array_from_buffer(app, buffer);
    if (!tokens.tokens)
        result = boundary_non_whitespace(app, buffer, side, direction, pos);
    
    else if (tokens.count > 1)
    {
        Token_Iterator_Array it = token_iterator_pos(0, &tokens, pos);
        Token* token = token_it_read(&it);
        i64 line = get_line_number_from_pos(app, buffer, pos);
        
        switch (direction)
        {
            case Scan_Forward:
            {
                if (token->kind == TokenBaseKind_Whitespace)
                {
                    token_it_inc_non_whitespace(&it);
                    token = token_it_read(&it);
                }
                
                // TODO(long): DSL Comments/Strings
                if (token->kind == TokenBaseKind_Comment || token->kind == TokenBaseKind_LiteralString)
                {
                    result = boundary_non_whitespace(app, buffer, side, direction, pos);
                    result = clamp_top(result, token->pos + token->size);
                }
                
                // NOTE(long): All other cases
                else
                {
                    if (side == Side_Max)
                    {
                        result = token->pos + token->size;
                        if (line != get_line_number_from_pos(app, buffer, result))
                            result = token->pos;
                    }
                    
                    else
                    {
                        Token* old = token;
                        if (token->pos <= pos)
                            if (token_it_inc_non_whitespace(&it))
                                token = token_it_read(&it);
                        
                        result = token->pos;
                        if (line != get_line_number_from_pos(app, buffer, result))
                            result = old->pos + old->size;
                    }
                }
            } break;
            
            case Scan_Backward:
            {
                if (token->kind == TokenBaseKind_Whitespace)
                {
                    token_it_dec_non_whitespace(&it);
                    token = token_it_read(&it);
                }
                else
                {
                    Token_Iterator_Array it2 = token_iterator_pos(0, &tokens, pos);
                    token_it_dec_non_whitespace(&it2);
                    Token* token2 = token_it_read(&it2);
                    if (token2->kind == TokenBaseKind_Comment || token2->kind == TokenBaseKind_LiteralString)
                        token = token2;
                }
                
                // TODO(long): DSL Comments/Strings
                if ((token->kind == TokenBaseKind_Comment || token->kind == TokenBaseKind_LiteralString) && pos > token->pos)
                {
                    result = boundary_non_whitespace(app, buffer, side, direction, pos);
                    result = clamp_bot(result, token->pos);
                }
                
                // NOTE(long): All other cases
                else
                {
                    if (side == Side_Min)
                    {
                        Assert(token->pos <= pos);
                        if (token->pos == pos)
                        {
                            token_it_dec_non_whitespace(&it);
                            token = token_it_read(&it);
                        }
                        
                        result = token->pos;
                        if (line != get_line_number_from_pos(app, buffer, result))
                            result = token->pos + token->size;
                    }
                    
                    else
                    {
                        i64 same_line_pos = token->pos + token->size; // pos is inside whitespace
                        if (token->pos + token->size >= pos)
                        {
                            // pos is inside non-whitespace or the end boundary of non-whitespace
                            same_line_pos = token->pos;
                            token_it_dec_non_whitespace(&it);
                            token = token_it_read(&it);
                            
                            if (token->pos + token->size == pos)
                            {
                                // pos is the intersection of two non-whitespace tokens
                                // e.g. token_it_dec|(...
                                same_line_pos = token->pos;
                                token_it_dec_non_whitespace(&it);
                                token = token_it_read(&it);
                            }
                        }
                        
                        result = token->pos + token->size;
                        if (line != get_line_number_from_pos(app, buffer, result) && same_line_pos < pos)
                            result = same_line_pos;
                    }
                }
            } break;
        }
    }
    
    return result;
}

CUSTOM_COMMAND_SIG(long_switch_move_side_mode)
CUSTOM_DOC("Change which side move_left/right/next/prev moves to.")
{
    long_global_move_side = (long_global_move_side + 1) % 4;
}

CUSTOM_COMMAND_SIG(long_move_next_word)
CUSTOM_DOC("Seek left for the next token.")
{
    Long_Scan_Move(app, Scan_Forward, Long_Boundary_TokenAndWhitespace);
}

CUSTOM_COMMAND_SIG(long_move_prev_word)
CUSTOM_DOC("Seek left for the prev token.")
{
    Long_Scan_Move(app, Scan_Backward, Long_Boundary_TokenAndWhitespace);
}

CUSTOM_COMMAND_SIG(long_backspace_token_boundary)
CUSTOM_DOC("Deletes left to a token boundary.")
{
    Scratch_Block scratch(app);
    Long_Scan_Delete(app, Scan_Backward, Long_Boundary_TokenAndWhitespace);
}

CUSTOM_COMMAND_SIG(long_delete_token_boundary)
CUSTOM_DOC("Deletes right to a token boundary.")
{
    Scratch_Block scratch(app);
    Long_Scan_Delete(app, Scan_Forward, Long_Boundary_TokenAndWhitespace);
}

//- NOTE(long): Alpha-Numeric/Camel
function i64 Long_Boundary_UpperLower(Application_Links* app, Buffer_ID buffer, Scan_Direction direction, i64 pos)
{
    i64 min = boundary_predicate(app, buffer, Side_Min, direction, pos, &character_predicate_uppercase);
    i64 max = boundary_predicate(app, buffer, Side_Max, direction, pos, &character_predicate_uppercase);
    i64 max_start = boundary_predicate(app, buffer, Side_Min, Scan_Backward, max, &character_predicate_uppercase);
    
    i64 result = min;
    if (max > max_start + 1) // Camel vs UPPERlower
        result = direction == Scan_Forward ? Min(min, max) : Max(min, max);
    return result;
}

function i64 Long_Boundary_AlphaNumericCamel(Application_Links* app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos)
{
    // Test Code: AAA_BBB_CCC AaaBbbCcc Aaa_Bbb_Ccc
    // AAAaaaAAAbbbBcdAbcd AAA_aaaAAA_AAAaaaAaaa_aaa
    
    i64 an_pos = boundary_alpha_numeric(app, buffer, side, direction, pos);
    i64 cm_pos = Long_Boundary_UpperLower(app, buffer, direction, pos);
    
    i64 an_opposite_pos = boundary_alpha_numeric(app, buffer, flip_side(side), flip_direction(direction), an_pos);
    if (an_opposite_pos == cm_pos)
        cm_pos = Long_Boundary_UpperLower(app, buffer, direction, cm_pos);
    
    i64 result = 0;
    if (direction == Scan_Backward)
        result = Max(an_pos, cm_pos);
    else
        result = Min(an_pos, cm_pos);
    return result;
}

CUSTOM_COMMAND_SIG(long_move_next_alpha_numeric_or_camel_boundary)
CUSTOM_DOC("Seek right for boundary between alphanumeric characters or camel case word and non-alphanumeric characters.")
{
    Scratch_Block scratch(app);
    Long_Scan_Move(app, Scan_Forward, Long_Boundary_AlphaNumericCamel);
}

CUSTOM_COMMAND_SIG(long_move_prev_alpha_numeric_or_camel_boundary)
CUSTOM_DOC("Seek left for boundary between alphanumeric characters or camel case word and non-alphanumeric characters.")
{
    Scratch_Block scratch(app);
    Long_Scan_Move(app, Scan_Backward, Long_Boundary_AlphaNumericCamel);
}

CUSTOM_COMMAND_SIG(long_backspace_alpha_numeric_or_camel_boundary)
CUSTOM_DOC("Deletes left to a alphanumeric or camel boundary.")
{
    Scratch_Block scratch(app);
    Long_Scan_Delete(app, Scan_Backward, Long_Boundary_AlphaNumericCamel);
}

CUSTOM_COMMAND_SIG(long_delete_alpha_numeric_or_camel_boundary)
CUSTOM_DOC("Deletes right to an alphanumeric or camel boundary.")
{
    Scratch_Block scratch(app);
    Long_Scan_Delete(app, Scan_Forward, Long_Boundary_AlphaNumericCamel);
}

//- NOTE(long): Index
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

function i64 Long_Boundary_FunctionAndType(Application_Links* app, Buffer_ID buffer, Side side, Scan_Direction dir, i64 pos)
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
    Long_Scan_Move(app, Scan_Forward, Long_Boundary_FunctionAndType);
    Long_SnapMarkToCursor(app);
}

CUSTOM_COMMAND_SIG(long_move_to_prev_function_and_type)
CUSTOM_DOC("Seek left for the previous function or type in the buffer.")
{
    Scratch_Block scratch(app);
    Long_Scan_Move(app, Scan_Backward, Long_Boundary_FunctionAndType);
    Long_SnapMarkToCursor(app);
}

// @COPYPASTA(long):
// https://github.com/Jack-Punter/4coder_punter/blob/0b43bad07998132e76d7094ed7ee385151a52ab7/4coder_fleury_base_commands.cpp#L651
function i64 Long_Boundary_CursorToken(Application_Links* app, Buffer_ID buffer, 
                                       Side side, Scan_Direction direction, i64 pos)
{
    Scratch_Block scratch(app);
    
    // NOTE(jack): Iterate the code index file for the buffer to find the search range for tokens
    // The search range_bounds should contain the function body, and its parameter list
    Code_Index_File* file = code_index_get_file(buffer);
    Range_i64 search_bounds = {};
    if (file) 
    {
        Code_Index_Nest_Ptr_Array file_nests = file->nest_array;
        Code_Index_Nest* prev_important_nest = 0;
        
        for (Code_Index_Nest* nest = file->nest_list.first; 
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
                        Code_Index_Nest* funciton_body_nest = nest->next;
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
    
    View_ID view = get_active_view(app, Access_Always);
    i64 active_cursor_pos = view_get_cursor_pos(app, view);
    Token_Array tokens = get_token_array_from_buffer(app, buffer);
    Token_Iterator_Array active_cursor_it = token_iterator_pos(0, &tokens, active_cursor_pos);
    Token* active_cursor_token = token_it_read(&active_cursor_it);
    String_Const_u8 cursor_string = push_buffer_range(app, scratch, buffer, Ii64(active_cursor_token));
    i64 cursor_offset = pos - active_cursor_token->pos;
    
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
                Token* token = token_it_read(&it);
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
    Long_Scan_Move(app, Scan_Backward, Long_Boundary_CursorToken);
}

CUSTOM_COMMAND_SIG(long_move_down_token_occurrence)
CUSTOM_DOC("Moves the cursor to the next occurrence of the token that the cursor is over.")
{
    Scratch_Block scratch(app);
    Long_Scan_Move(app, Scan_Forward, Long_Boundary_CursorToken);
}

//- NOTE(long): Upper/Lower Case
function void Long_UpperLowerRange(Application_Links* app, b32 upper, b32 lower)
{
    Scratch_Block scratch(app);
    View_ID view = get_active_view(app, 0);
    Range_i64 range = get_view_range(app, view);
    Buffer_ID buffer = view_get_buffer(app, view, 0);
    
    String8 str = push_buffer_range(app, scratch, buffer, range);
    if (str.size)
    {
        for (u64 i = 0; i < str.size; ++i)
        {
            char c = str.str[i];
            if (c < 'A' || c > 'z')
                continue;
            else if (upper && c >= 'a')
                c -= 'a' - 'A';
            else if (lower && c <= 'Z')
                c += 'a' - 'A';
            str.str[i] = c;
        }
        
        i64 c = view_get_cursor_pos(app, view);
        i64 m =   view_get_mark_pos(app, view);
        buffer_replace_range(app, buffer, range, str);
        if (c < m)
            view_set_mark(app, view, seek_pos(range.max));
        else
            view_set_cursor_and_preferred_x(app, view, seek_pos(range.max));
    }
}

CUSTOM_COMMAND_SIG(long_inverse_case)
CUSTOM_DOC("Turn all the upper-case characaters to lower-case and vice versa in the range between the cursor and the mark.")
{
    Long_UpperLowerRange(app, 1, 1);
}

CUSTOM_COMMAND_SIG(long_lower_case)
CUSTOM_DOC("Turn all the upper-case characaters to lower-case in the range between the cursor and the mark.")
{
    Long_UpperLowerRange(app, 0, 1);
}

CUSTOM_COMMAND_SIG(long_upper_case)
CUSTOM_DOC("Turn all the lower-case characaters to upper-case in the range between the cursor and the mark.")
{
    Long_UpperLowerRange(app, 1, 0);
}

//- NOTE(long): Scope
// @COPYPASTA(long): find_nest_side
function b32 Long_FindNestSide(Application_Links* app, Buffer_ID buffer, i64 pos,
                               Find_Nest_Flag flags, Scan_Direction scan, Nest_Delimiter_Kind delim, Range_i64* out)
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

//~ NOTE(long): Project Commands

function String8 Long_Prj_RelBufferName(Application_Links* app, Arena* arena, Buffer_ID buffer)
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

// @COPYPASTA(long): f4_setup_new_project
CUSTOM_COMMAND_SIG(long_setup_new_project)
CUSTOM_DOC("Queries the user for several configuration options and initializes a new 4coder project with build scripts for every OS.")
{
    prj_setup_scripts(app, PrjSetupScriptFlag_Project|PrjSetupScriptFlag_Bat|PrjSetupScriptFlag_Sh);
    
    {
        Scratch_Block scratch(app);
        String8 project_path = push_hot_directory(app, scratch);
        String8 filename = push_file_search_up_path(app, scratch, project_path, S8Lit("project.4coder"));
        Buffer_ID buffer = create_buffer(app, filename, 0);
        
        i64 pos = 0;
        seek_string_forward(app, buffer, 0, 0, S8Lit("F1"), &pos);
        if (pos)
            buffer_replace_range(app, buffer, Ii64_size(pos, sizeof("F1 = \"run\";\n.F2 = \"run\";") - 1),
                                 S8Lit("F1 = \"build\",\n.F2 = \"run\","));
    }
    
    load_project(app);
}

function void Long_Prj_OpenFiles_Recursive_Readonly(Application_Links* app, String8 path,
                                                    Prj_Pattern_List whitelist, Prj_Pattern_List blacklist)
{
    Scratch_Block scratch(app);
    
    ProfileScopeNamed(app, "get file list", profile_get_file_list);
    File_List list = system_get_file_list(scratch, path);
    ProfileCloseNow(profile_get_file_list);
    
    File_Info** info = list.infos;
    for (u32 i = 0; i < list.count; ++i, ++info)
    {
        String8 file_name = (**info).file_name;
        if (HasFlag((**info).attributes.flags, FileAttribute_IsDirectory))
        {
            if (prj_match_in_pattern_list(file_name, blacklist))
                continue;
            
            String8 new_path = push_u8_stringf(scratch, "%.*s%.*s/", string_expand(path), string_expand(file_name));
            Long_Prj_OpenFiles_Recursive_Readonly(app, new_path, whitelist, blacklist);
        }
        
        else
        {
            if (!prj_match_in_pattern_list(file_name, whitelist))
                continue;
            if (prj_match_in_pattern_list(file_name, blacklist))
                continue;
            
            String8 fullpath = push_u8_stringf(scratch, "%.*s%.*s", string_expand(path), string_expand(file_name));
            Buffer_ID buffer = create_buffer(app, fullpath, 0);
            buffer_set_setting(app, buffer, BufferSetting_ReadOnly, true);
            buffer_set_setting(app, buffer, BufferSetting_Unimportant, true);
        }
    }
}

CUSTOM_COMMAND_SIG(long_load_project)
CUSTOM_DOC("Looks for a project.4coder file in the current directory and tries to load it.  Looks in parent directories until a project file is found or there are no more parents.")
{
    load_project(app);
    Scratch_Block scratch(app);
    
    // NOTE(long): This must use the v2 project file format
    Variable_Handle prj_var = vars_read_key(vars_get_root(), vars_save_string_lit("prj_config"));
    Variable_Handle reference_path_var = vars_read_key(prj_var, vars_save_string_lit("reference_paths"));
    
    Variable_Handle whitelist_var = vars_read_key(prj_var, vars_save_string_lit("patterns"));
    Variable_Handle blacklist_var = vars_read_key(prj_var, vars_save_string_lit("blacklist_patterns"));
    
    Prj_Pattern_List whitelist = prj_pattern_list_from_var(scratch, whitelist_var);
    Prj_Pattern_List blacklist = prj_pattern_list_from_var(scratch, blacklist_var);
    
    for (Vars_Children(path_var, reference_path_var))
    {
        String8 path = vars_string_from_var(scratch, path_var);
        if (file_exists_and_is_folder(app, path))
        {
            if (!character_is_slash(string_get_character(path, path.size - 1)))
                path = push_u8_stringf(scratch, "%.*s/", string_expand(path));
            Long_Prj_OpenFiles_Recursive_Readonly(app, path, whitelist, blacklist);
        }
        
        else if (file_exists_and_is_file(app, path))
        {
            Buffer_ID buffer = create_buffer(app, path, 0);
            push_whole_buffer(app, scratch, buffer);
            buffer_set_setting(app, buffer, BufferSetting_ReadOnly, true);
            buffer_set_setting(app, buffer, BufferSetting_Unimportant, true);
        }
        
        else
        {
            String8 message = push_stringf(scratch, "Error: Path \"%.*s\" doesn't exists\n", string_expand(path));
            print_message(app, message);
        }
    }
}

//~ NOTE(long): Command-related Commands

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

// @COPYPASTA(long): execute_any_cli
CUSTOM_COMMAND_SIG(long_execute_any_cli)
CUSTOM_DOC("Queries for an output buffer name and system command, runs the system command as a CLI and prints the output to the specified buffer."){
    Scratch_Block scratch(app);
    Query_Bar_Group group(app);
    
    Query_Bar bar_out = {};
    bar_out.prompt = string_u8_litexpr("Output Buffer: ");
    bar_out.string = SCu8(out_buffer_space, (u64)0);
    bar_out.string_capacity = sizeof(out_buffer_space);
    if (!Long_Query_User_String(app, &bar_out, S8Lit("*run*"))) return;
    bar_out.string.size = clamp_top(bar_out.string.size, sizeof(out_buffer_space) - 1);
    out_buffer_space[bar_out.string.size] = 0;
    
    Query_Bar bar_cmd = {};
    bar_cmd.prompt = string_u8_litexpr("Command: ");
    bar_cmd.string = SCu8(command_space, (u64)0);
    bar_cmd.string_capacity = sizeof(command_space);
    if (!Long_Query_User_String(app, &bar_cmd, string_u8_empty)) return;
    bar_cmd.string.size = clamp_top(bar_cmd.string.size, sizeof(command_space) - 1);
    command_space[bar_cmd.string.size] = 0;
    
    String_Const_u8 hot = push_hot_directory(app, scratch);
    {
        u64 size = clamp_top(hot.size, sizeof(hot_directory_space));
        block_copy(hot_directory_space, hot.str, size);
        hot_directory_space[hot.size] = 0;
    }
    
    execute_previous_cli(app);
}

//~ NOTE(long): Theme Commands

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

CUSTOM_COMMAND_SIG(long_explorer)
CUSTOM_DOC("Opens file explorer in cmd")
{
    Scratch_Block scratch(app);
    String8 hot = push_hot_directory(app, scratch);
    exec_system_command(app, 0, buffer_identifier(0), hot, S8Lit("explorer ."), 0);
}

//~ NOTE(long): Macro Commands

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

//~ NOTE(long): @f4_base_commands
// These are some useful f4 commands that I personally use.
// Most of these are just straight-up copied from the original 4coder_fleury_base_commands file.
// Except for some small or isolated functions that were inlined directly at their call site.

CUSTOM_UI_COMMAND_SIG(f4_open_project)
CUSTOM_DOC("Open a project by navigating to the project file.")
{
    for(;;)
    {
        Scratch_Block scratch(app);
        View_ID view = get_this_ctx_view(app, Access_Always);
        File_Name_Result result = get_file_name_from_user(app, scratch, "Open Project:", view);
        if (result.canceled) break;
        
        String_Const_u8 file_name = result.file_name_activated;
        if (file_name.size == 0)
        {
            file_name = result.file_name_in_text_field;
        }
        if (file_name.size == 0) break;
        
        String_Const_u8 path = result.path_in_text_field;
        String_Const_u8 full_file_name = push_u8_stringf(scratch, "%.*s/%.*s",
                                                         string_expand(path), string_expand(file_name));
        
        if (result.is_folder)
        {
            set_hot_directory(app, full_file_name);
            continue;
        }
        
        if(character_is_slash(file_name.str[file_name.size - 1]))
        {
            File_Attributes attribs = system_quick_file_attributes(scratch, full_file_name);
            if (HasFlag(attribs.flags, FileAttribute_IsDirectory)){
                set_hot_directory(app, full_file_name);
                continue;
            }
            if (string_looks_like_drive_letter(file_name)){
                set_hot_directory(app, file_name);
                continue;
            }
            if (query_create_folder(app, file_name)){
                set_hot_directory(app, full_file_name);
                continue;
            }
            break;
        }
        
        set_hot_directory(app, full_file_name);
        load_project(app);
        break;
    }
}

CUSTOM_COMMAND_SIG(f4_home_first_non_whitespace)
CUSTOM_DOC("Goes to the beginning of the line.")
{
    View_ID view = get_active_view(app, Access_Read);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Read);
    if(view && buffer)
    {
        i64 start_pos = view_get_cursor_pos(app, view);
        seek_pos_of_visual_line(app, Side_Min);
        i64 end_pos = view_get_cursor_pos(app, view);
        i64 line = get_line_number_from_pos(app, buffer, start_pos);
        
        // NOTE(rjf): If we are on the first column, go to the first non-whitespace
        // in the line.
        if(start_pos == end_pos && start_pos == get_line_start_pos(app, buffer, line))
        {
            Scratch_Block scratch(app);
            String_Const_u8 string = push_buffer_line(app, scratch, buffer, line);
            for(u64 i = 0; i < string.size; i += 1)
            {
                if(!character_is_whitespace(string.str[i]))
                {
                    view_set_cursor_by_character_delta(app, view, (i64)i);
                    break;
                }
            }
        }
        
        // NOTE(rjf): If we hit any non-whitespace, move to the first possible
        // non-whitespace instead of the front of the line entirely.
        else 
        {
            Scratch_Block scratch(app);
            String_Const_u8 string = push_buffer_range(app, scratch, buffer, Ii64(start_pos, end_pos));
            
            b32 skipped_non_whitespace = 0;
            {
                for(i64 i = string.size-1; i >= 0; i -= 1)
                {
                    if(!character_is_whitespace(string.str[i]))
                    {
                        skipped_non_whitespace = 1;
                        break;
                    }
                }
            }
            
            if(skipped_non_whitespace)
            {
                for(i64 i = 0; i < (i64)string.size; i += 1)
                {
                    if(!character_is_whitespace(string.str[i]))
                    {
                        view_set_cursor_by_character_delta(app, view, i);
                        break;
                    }
                }
            }
        }
        
        // NOTE(rjf): Scroll all the way left.
        {
            Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
            scroll.target.pixel_shift.x = 0;
            view_set_buffer_scroll(app, view, scroll, SetBufferScroll_NoCursorChange);
        }
    }
}

function void
F4_ReIndentLine(Application_Links *app, Buffer_ID buffer, i64 line, i64 indent_delta)
{
    Scratch_Block scratch(app);
    String_Const_u8 line_string = push_buffer_line(app, scratch, buffer, line);
    i64 line_start_pos = get_line_start_pos(app, buffer, line);
    
    Range_i64 line_indent_range = Ii64(0, 0);
    i64 tabs_at_beginning = 0;
    i64 spaces_at_beginning = 0;
    for(u64 i = 0; i < line_string.size; i += 1)
    {
        if(line_string.str[i] == '\t')
        {
            tabs_at_beginning += 1;
        }
        else if(character_is_whitespace(line_string.str[i]))
        {
            spaces_at_beginning += 1;
        }
        else if(!character_is_whitespace(line_string.str[i]))
        {
            line_indent_range.max = (i64)i;
            break;
        }
    }
    
    // NOTE(rjf): Indent lines.
    {
        Range_i64 indent_range =
        {
            line_indent_range.min + line_start_pos,
            line_indent_range.max + line_start_pos,
        };
        
        i64 indent_width = (i64)def_get_config_u64(app, vars_save_string_lit("indent_width"));
        b32 indent_with_tabs = def_get_config_b32(vars_save_string_lit("indent_with_tabs"));
        i64 spaces_per_indent_level = indent_width;
        i64 indent_level = spaces_at_beginning / spaces_per_indent_level + tabs_at_beginning;
        i64 new_indent_level = indent_level + indent_delta;
        
        String_Const_u8 indent_string = indent_with_tabs ? S8Lit("\t") : push_stringf(scratch, "%.*s", Min(indent_width, 16),
                                                                                      "                ");
        buffer_replace_range(app, buffer, indent_range, S8Lit(""));
        for(i64 i = 0; i < new_indent_level; i += 1)
        {
            buffer_replace_range(app, buffer, Ii64(line_start_pos), indent_string);
        }
    }
    
}

internal Range_i64
F4_LineRangeFromPosRange(Application_Links *app, Buffer_ID buffer, Range_i64 pos_range)
{
    Range_i64 lines_range =
        Ii64(get_line_number_from_pos(app, buffer, pos_range.min),
             get_line_number_from_pos(app, buffer, pos_range.max));
    return lines_range;
}

internal void
F4_ReIndentPosRange(Application_Links *app, Buffer_ID buffer, Range_i64 range, i64 indent_delta)
{
    range = F4_LineRangeFromPosRange(app, buffer, range);
    for (i64 i = range.min; i <= range.max; i += 1)
        F4_ReIndentLine(app, buffer, i, indent_delta);
}

internal void
F4_AdjustCursorAndMarkForIndentation(Application_Links *app, View_ID view, i64 original_cursor, i64 original_mark, Range_i64 original_line_range)
{
    Buffer_ID buffer = view_get_buffer(app, view, Access_Read);
    Scratch_Block scratch(app);
    if(original_cursor == original_mark)
    {
        i64 start_pos = get_line_start_pos(app, buffer, original_line_range.min);
        i64 new_pos = start_pos;
        String_Const_u8 line = push_buffer_line(app, scratch, buffer, original_line_range.min);
        for(u64 i = 0; i < line.size; i += 1)
        {
            if(!character_is_whitespace(line.str[i]))
            {
                new_pos = start_pos + (i64)i;
                break;
            }
        }
        
        view_set_cursor(app, view, seek_pos(new_pos));
        view_set_mark(app, view, seek_pos(new_pos));
    }
    else
    {
        Range_i64 range;
        {
            Range_i64 line_range = original_line_range;
            if (line_range.min > line_range.max)
            {
                i64 swap = line_range.max;
                line_range.max = line_range.min;
                line_range.min = swap;
            }
            range = Ii64(get_line_start_pos(app, buffer, line_range.min), get_line_end_pos(app, buffer, line_range.max));
        }
        
        view_set_cursor(app, view, seek_pos(original_cursor > original_mark ? range.max : range.min));
        view_set_mark(app, view, seek_pos(original_cursor > original_mark ? range.min : range.max));
    }
}

CUSTOM_COMMAND_SIG(f4_autocomplete_or_indent)
CUSTOM_DOC("Tries to autocomplete the word currently being typed, and inserts indentation if such a word is not found.")
{
    ProfileScope(app, "[F4] Word Complete");
    
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    
    if(buffer != 0)
    {
        Managed_Scope scope = view_get_managed_scope(app, view);
        
        b32 first_completion = false;
        Rewrite_Type *rewrite = scope_attachment(app, scope, view_rewrite_loc, Rewrite_Type);
        if (*rewrite != Rewrite_WordComplete){
            first_completion = true;
        }
        
        set_next_rewrite(app, view, Rewrite_WordComplete);
        
        Word_Complete_Iterator *it = word_complete_get_shared_iter(app);
        local_persist b32 initialized = false;
        local_persist Range_i64 range = {};
        
        if(first_completion || !initialized)
        {
            ProfileScope(app, "[F4] Word Complete State Init");
            initialized = false;
            i64 pos = view_get_cursor_pos(app, view);
            Range_i64 needle_range = get_word_complete_needle_range(app, buffer, pos);
            if(range_size(needle_range) > 0)
            {
                initialized = true;
                range = needle_range;
                word_complete_iter_init(buffer, needle_range, it);
            }
        }
        
        // NOTE(rjf): Word-Complete
        if(initialized)
        {
            ProfileScope(app, "[F4] Word Complete Apply");
            
            word_complete_iter_next(it);
            String_Const_u8 str = word_complete_iter_read(it);
            
            buffer_replace_range(app, buffer, range, str);
            
            range.max = range.min + str.size;
            view_set_cursor_and_preferred_x(app, view, seek_pos(range.max));
        }
        
        // NOTE(rjf): Insert indentation if autocomplete failed
        else if(initialized == 0)
        {
            i64 pos = view_get_cursor_pos(app, view);
            i64 mark = view_get_mark_pos(app, view);
            Range_i64 pos_range = Ii64(pos, mark);
            Range_i64 line_range = F4_LineRangeFromPosRange(app, buffer, pos_range);
            
            History_Group group = history_group_begin(app, buffer);
            F4_ReIndentPosRange(app, buffer, Ii64(pos, mark), +1);
            F4_AdjustCursorAndMarkForIndentation(app, view, pos, mark, line_range);
            history_group_end(group);
            no_mark_snap_to_cursor(app, view);
        }
    }
}

CUSTOM_COMMAND_SIG(f4_unindent)
CUSTOM_DOC("Unindent the selected range.")
{
    Scratch_Block scratch(app);
    
    View_ID view = get_active_view(app, Access_ReadWrite);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWrite);
    i64 pos = view_get_cursor_pos(app, view);
    i64 mark = view_get_mark_pos(app, view);
    Range_i64 pos_range = Ii64(pos, mark);
    Range_i64 line_range = F4_LineRangeFromPosRange(app, buffer, pos_range);
    History_Group group = history_group_begin(app, buffer);
    F4_ReIndentPosRange(app, buffer, Ii64(pos, mark), -1);
    F4_AdjustCursorAndMarkForIndentation(app, view, pos, mark, line_range);
    history_group_end(group);
    no_mark_snap_to_cursor(app, view);
}

function File_Name_Result
F4_GetFileNameFromUser_Project(Application_Links *app, Arena *arena, String_Const_u8 query, View_ID view)
{
    Lister_Handlers handlers = lister_get_default_handlers();
    handlers.write_character = lister__write_character__file_path;
    handlers.backspace = lister__backspace_text_field__file_path;
    
    Lister_Result l_result = run_lister_with_refresh_handler(app, arena, query, handlers);
    
    File_Name_Result result = {};
    result.canceled = l_result.canceled;
    if (!l_result.canceled){
        result.clicked = l_result.activated_by_click;
        if (l_result.user_data != 0){
            String_Const_u8 name = SCu8((u8*)l_result.user_data);
            result.file_name_activated = name;
            result.is_folder = character_is_slash(string_get_character(name, name.size - 1));
        }
        result.file_name_in_text_field = string_front_of_path(l_result.text_field);
        
        String_Const_u8 path = {};
        if (l_result.user_data == 0 && result.file_name_in_text_field.size == 0 && l_result.text_field.size > 0){
            result.file_name_in_text_field = string_front_folder_of_path(l_result.text_field);
            path = string_remove_front_folder_of_path(l_result.text_field);
        }
        else{
            path = string_remove_front_of_path(l_result.text_field);
        }
        if (character_is_slash(string_get_character(path, path.size - 1))){
            path = string_chop(path, 1);
        }
        result.path_in_text_field = path;
    }
    
    return(result);
}

CUSTOM_UI_COMMAND_SIG(f4_interactive_open_or_new_in_project)
CUSTOM_DOC("Interactively open a file out of the file system, filtered to files only in the project.")
{
    for(;;)
    {
        Scratch_Block scratch(app);
        View_ID view = get_this_ctx_view(app, Access_Always);
        File_Name_Result result = F4_GetFileNameFromUser_Project(app, scratch, S8Lit("Open (File In Project):"), view);
        if (result.canceled) break;
        
        String_Const_u8 file_name = result.file_name_activated;
        if (file_name.size == 0){
            file_name = result.file_name_in_text_field;
        }
        if (file_name.size == 0) break;
        
        String_Const_u8 path = result.path_in_text_field;
        String_Const_u8 full_file_name = push_u8_stringf(scratch, "%.*s/%.*s",
                                                         string_expand(path), string_expand(file_name));
        
        if(result.is_folder)
        {
            set_hot_directory(app, full_file_name);
            continue;
        }
        
        if(character_is_slash(file_name.str[file_name.size - 1]))
        {
            File_Attributes attribs = system_quick_file_attributes(scratch, full_file_name);
            if(HasFlag(attribs.flags, FileAttribute_IsDirectory))
            {
                set_hot_directory(app, full_file_name);
                continue;
            }
            if(string_looks_like_drive_letter(file_name))
            {
                set_hot_directory(app, file_name);
                continue;
            }
            if(query_create_folder(app, file_name))
            {
                set_hot_directory(app, full_file_name);
                continue;
            }
            break;
        }
        
        Buffer_ID buffer = create_buffer(app, full_file_name, 0);
        if (buffer != 0){
            view_set_buffer(app, view, buffer, 0);
        }
        break;
    }
}

internal void
F4_SetLineCommentedOnLine(Application_Links *app, Buffer_ID buffer, i64 *cursor_p, i64 *mark_p, b32 commented)
{
    i64 cursor = *cursor_p;
    i64 mark = *mark_p;
    i64 cursor_line = get_line_number_from_pos(app, buffer, cursor);
    i64 mark_line = get_line_number_from_pos(app, buffer, mark);
    
    if(cursor_line == mark_line)
    {
        i64 line = cursor_line;
        i64 line_start = get_pos_past_lead_whitespace_from_line_number(app, buffer, line);
        b32 already_has_comment = c_line_comment_starts_at_position(app, buffer, line_start);
        
        if(commented)
        {
            if(!already_has_comment)
            {
                buffer_replace_range(app, buffer, Ii64(line_start), string_u8_litexpr("//"));
                cursor = mark += 2;
            }
        }
        else
        {
            if(already_has_comment)
            {
                buffer_replace_range(app, buffer, Ii64(line_start, line_start + 2), string_u8_empty);
                cursor = mark -= 2;
            }
        }
    }
    
    *cursor_p = cursor;
    *mark_p = mark;
}

internal void
F4_SetBlockCommentedOnRange(Application_Links *app, Buffer_ID buffer, i64 *cursor_p, i64 *mark_p, b32 commented)
{
    Scratch_Block scratch(app);
    
    i64 cursor = *cursor_p;
    i64 mark = *mark_p;
    Range_i64 range = Ii64(cursor, mark);
    
    if(commented)
    {
        buffer_replace_range(app, buffer, Ii64(range.max, range.max), S8Lit("*/"));
        buffer_replace_range(app, buffer, Ii64(range.min, range.min), S8Lit("/*"));
        if(cursor > mark) { cursor += 4; }
        else              { mark   += 4; }
    }
    else if(range.max - range.min >= 2)
    {
        String_Const_u8 opener = push_buffer_range(app, scratch, buffer, Ii64(range.min, range.min+2));
        String_Const_u8 closer = push_buffer_range(app, scratch, buffer, Ii64(range.max-2, range.max));
        if(string_match(opener, S8Lit("/*")) &&
           string_match(closer, S8Lit("*/")))
        {
            buffer_replace_range(app, buffer, Ii64(range.max-2, range.max), S8Lit(""));
            buffer_replace_range(app, buffer, Ii64(range.min, range.min+2), S8Lit(""));
            if(cursor > mark) { cursor -= 4; }
            if(mark > cursor) { mark -= 4; }
        }
    }
    
    *cursor_p = cursor;
    *mark_p = mark;
}

function b32
F4_CBlockCommentStartsAtPosition(Application_Links *app, Buffer_ID buffer, i64 pos)
{
    b32 alread_has_comment = false;
    u8 check_buffer[2];
    if(buffer_read_range(app, buffer, Ii64(pos, pos + 2), check_buffer))
    {
        if(check_buffer[0] == '/' && check_buffer[1] == '*')
        {
            alread_has_comment = true;
        }
    }
    return(alread_has_comment);
}

internal void
F4_SetCommentedOnRange(Application_Links *app, Buffer_ID buffer, i64 *cursor_p, i64 *mark_p, b32 commented)
{
    Scratch_Block scratch(app);
    
    i64 cursor = *cursor_p;
    i64 mark = *mark_p;
    Range_i64 range = Ii64(cursor, mark);
    Range_i64 line_range = F4_LineRangeFromPosRange(app, buffer, range);
    Token_Array tokens = get_token_array_from_buffer(app, buffer);
    
    // NOTE(rjf): No selection
    if(range.min == range.max)
    {
        F4_SetLineCommentedOnLine(app, buffer, &cursor, &mark, commented);
    }
    
    // NOTE(rjf): Single-Line Selection
    else if(line_range.min == line_range.max)
    {
        Token *min_token = get_token_from_pos(app, &tokens, (u64)range.min);
        Token *max_token = get_token_from_pos(app, &tokens, (u64)range.max);
        
        // NOTE(rjf): Selection is inside comment
        if(min_token == max_token && min_token && min_token->kind == TokenBaseKind_Comment)
        {
            (commented ? comment_line : uncomment_line)(app);
        }
        
        // NOTE(rjf): Selection is not inside comment
        else 
        {
            F4_SetBlockCommentedOnRange(app, buffer, &cursor, &mark, commented);
        }
    }
    
    // NOTE(rjf): Multi-Line Selection
    else if(line_range.min != line_range.max)
    {
        if(commented)
        {
            i64 min_pos = Min(cursor, mark);
            i64 line = get_line_number_from_pos(app, buffer, min_pos);
            i64 start_of_line = get_line_start_pos(app, buffer, line);
            
            // NOTE(rjf): Selection starts on first column.
            if(min_pos == start_of_line)
            {
                for(i64 i = line_range.min; i <= line_range.max; i += 1)
                {
                    i64 cursor2 = get_line_start_pos(app, buffer, i);
                    i64 mark2 = get_line_end_pos(app, buffer, i);
                    F4_SetLineCommentedOnLine(app, buffer, &cursor2, &mark2, commented);
                }
                if(cursor < mark)
                {
                    cursor = get_line_start_pos(app, buffer, line_range.min);
                    mark = get_line_end_pos(app, buffer, line_range.max);
                }
                else
                {
                    mark = get_line_start_pos(app, buffer, line_range.min);
                    cursor = get_line_end_pos(app, buffer, line_range.max);
                }
            }
            
            // NOTE(rjf): Selection does not start on first column.
            else
            {
                F4_SetBlockCommentedOnRange(app, buffer, &cursor, &mark, 1);
            }
        }
        else
        {
            b32 starts_with_block_comment = F4_CBlockCommentStartsAtPosition(app, buffer, range.min);
            if(starts_with_block_comment)
            {
                F4_SetBlockCommentedOnRange(app, buffer, &cursor, &mark, 0);
            }
            else
            {
                for(i64 i = line_range.min; i <= line_range.max; i += 1)
                {
                    i64 cursor2 = get_line_start_pos(app, buffer, i);
                    i64 mark2 = get_line_start_pos(app, buffer, i);
                    F4_SetLineCommentedOnLine(app, buffer, &cursor2, &mark2, 0);
                }
                if(cursor < mark)
                {
                    cursor = get_line_start_pos(app, buffer, line_range.min);
                    mark = get_line_end_pos(app, buffer, line_range.max);
                }
                else
                {
                    mark = get_line_start_pos(app, buffer, line_range.min);
                    cursor = get_line_end_pos(app, buffer, line_range.max);
                }
            }
        }
    }
    
    *cursor_p = cursor;
    *mark_p = mark;
}

CUSTOM_COMMAND_SIG(f4_comment_selection)
CUSTOM_DOC("Performs VS-style commenting on the selected range.")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    i64 cursor = view_get_cursor_pos(app, view);
    i64 mark = view_get_mark_pos(app, view);
    
    History_Group group = history_group_begin(app, buffer);
    F4_SetCommentedOnRange(app, buffer, &cursor, &mark, 1);
    view_set_cursor(app, view, seek_pos(cursor));
    view_set_mark(app, view, seek_pos(mark));
    history_group_end(group);
    no_mark_snap_to_cursor(app, view);
}

CUSTOM_COMMAND_SIG(f4_uncomment_selection)
CUSTOM_DOC("Performs VS-style uncommenting on the selected range.")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    i64 cursor = view_get_cursor_pos(app, view);
    i64 mark = view_get_mark_pos(app, view);
    History_Group group = history_group_begin(app, buffer);
    F4_SetCommentedOnRange(app, buffer, &cursor, &mark, 0);
    view_set_cursor(app, view, seek_pos(cursor));
    view_set_mark(app, view, seek_pos(mark));
    history_group_end(group);
    no_mark_snap_to_cursor(app, view);
}

struct F4_LOCInfo
{
    F4_LOCInfo *next;
    String_Const_u8 name;
    i64 lines;
    i64 whitespace_only_lines;
    i64 open_brace_only_lines;
};

function F4_LOCInfo *
F4_LOCInfoFromBuffer(Application_Links *app, Arena *arena, Buffer_ID buffer)
{
    F4_LOCInfo *first = 0;
    F4_LOCInfo *last = 0;
    
    F4_LOCInfo *file_info = push_array_zero(arena, F4_LOCInfo, 1);
    sll_queue_push(first, last, file_info);
    file_info->name = str8_lit("all");
    F4_LOCInfo *active_info = 0;
    
    i64 line_count = buffer_get_line_count(app, buffer);
    for(i64 line_idx = 0; line_idx < line_count; line_idx += 1)
    {
        Scratch_Block scratch(app, arena);
        String_Const_u8 line = push_buffer_line(app, scratch, buffer, line_idx);
        if(line.size != 0 && line.str[line.size-1] == '\r')
        {
            line.size -= 1;
        }
        
        //- rjf: begin a section if we find a root divider comment here
        if(line.size >= 3 && line.str[0] == '/' && line.str[1] == '/' && line.str[2] == '~')
        {
            active_info = push_array_zero(arena, F4_LOCInfo, 1);
            active_info->name = push_string_copy(arena, string_substring(line, Ii64(3, line.size)));
            sll_queue_push(first, last, active_info);
        }
        
        //- rjf: find out if this is a line with only whitespace
        b32 is_only_whitespace = true;
        {
            for(u64 i = 0; i < line.size; i += 1)
            {
                if(!character_is_whitespace(line.str[i]))
                {
                    is_only_whitespace = false;
                    break;
                }
            }
        }
        
        //- rjf: find out if this is a line with only whitespace and an open brace
        b32 is_only_open_brace = false;
        if(is_only_whitespace == false)
        {
            for(u64 i = 0; i < line.size; i += 1)
            {
                if(!character_is_whitespace(line.str[i]))
                {
                    is_only_open_brace = line.str[i] == '{';
                    if(!is_only_open_brace)
                    {
                        break;
                    }
                }
            }
        }
        
        //- rjf: increment line counts
        {
            file_info->lines += 1;
            if(active_info != 0)
            {
                active_info->lines += 1;
            }
            if(is_only_whitespace)
            {
                file_info->whitespace_only_lines += 1;
                if(active_info != 0)
                {
                    active_info->whitespace_only_lines += 1;
                }
            }
            if(is_only_open_brace)
            {
                file_info->open_brace_only_lines += 1;
                if(active_info != 0)
                {
                    active_info->open_brace_only_lines += 1;
                }
            }
        }
    }
    
    return first;
}

function int
F4_LOCInfoCompare(const void *a_void_fuck_cplusplus, const void *b_void_fuck_cplusplus)
{
    F4_LOCInfo *a = (F4_LOCInfo *)a_void_fuck_cplusplus;
    F4_LOCInfo *b = (F4_LOCInfo *)b_void_fuck_cplusplus;
    return ((a->lines < b->lines) ? +1 :
            (a->lines > b->lines) ? -1 :
            0);
}

CUSTOM_COMMAND_SIG(f4_loc)
CUSTOM_DOC("Counts the lines of code in the current buffer, breaks it down by section, and outputs to the *loc* buffer.")
{
    Scratch_Block scratch(app);
    View_ID view = get_active_view(app, Access_Read);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Read);
    
    //- rjf: get all sections and counts from buffer
    F4_LOCInfo *infos_list = F4_LOCInfoFromBuffer(app, scratch, buffer);
    
    //- rjf: build unsorted info in array form
    int info_count = 0;
    F4_LOCInfo *info_array = 0;
    {
        for(F4_LOCInfo *info = infos_list; info; info = info->next, info_count += 1);
        info_array = push_array_zero(scratch, F4_LOCInfo, info_count);
        int i = 0;
        for(F4_LOCInfo *info = infos_list; info; info = info->next, i += 1)
        {
            info_array[i] = *info;
        }
    }
    
    //- rjf: sort array
    {
        qsort(info_array, info_count, sizeof(F4_LOCInfo), F4_LOCInfoCompare);
    }
    
    //- rjf: print loc info
    Buffer_ID loc_buffer = get_buffer_by_name(app, str8_lit("*loc*"), AccessFlag_Write);
    if(loc_buffer != 0)
    {
        clear_buffer(app, loc_buffer);
        
        for(int i = 0; i < info_count; i += 1)
        {
            F4_LOCInfo *info = info_array + i;
            
            Scratch_Block scratch2(app, scratch);
            int padding = 25;
            int chrs = (int)info->name.size;
            int spaces = 0;
            if(chrs > padding)
            {
                chrs = padding;
                spaces = 0;
            }
            else
            {
                spaces = padding - chrs;
            }
            
            if(spaces < 0)
            {
                spaces = 0;
            }
            
            String_Const_u8 string = push_stringf(scratch2,
                                                  ">>> %.*s%.*s: %6i lines; %6i whitespace; %6i open braces; %6i significant\n",
                                                  chrs, info->name.str,
                                                  spaces, "                                            ",
                                                  (int)info->lines,
                                                  (int)info->whitespace_only_lines,
                                                  (int)info->open_brace_only_lines,
                                                  (int)(info->lines - (info->whitespace_only_lines+info->open_brace_only_lines)));
            b32 write_successful = buffer_replace_range(app, loc_buffer, Ii64(buffer_get_size(app, loc_buffer)), string);
            write_successful = write_successful;
        }
    }
}

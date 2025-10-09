
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

function Rect_f32 Long_Rf32_Round(Rect_f32 r)
{
    r.x0 = f32_round32(r.x0);
    r.x1 = f32_round32(r.x1);
    r.y0 = f32_round32(r.y0);
    r.y1 = f32_round32(r.y1);
    return r;
}

function Vec2_f32 Long_V2f32(f32 f)
{
    return V2f32(f, f);
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

//- NOTE(long): View
function View_ID Long_View_NextPassive(Application_Links* app, View_ID view, Access_Flag access)
{
    View_ID start_view = view;
    do
    {
        view = get_next_view_looped_all_panels(app, view, 0);
        if (view_get_is_passive(app, view))
            break;
    } while (view != start_view);
    return view;
}

function View_ID Long_View_Open(Application_Links* app, View_ID view, Buffer_ID buffer, View_Split_Position position, b32 passive)
{
    View_ID result = open_view(app, view, position);
    new_view_settings(app, result);
    view_set_buffer(app, result, buffer, 0);
    
    if (passive)
        view_set_passive(app, result, true);
    return result;
}

function void Long_View_Snap(Application_Links* app, View_ID view)
{
    // @COPYPASTA(long): center_view. This will snap the view to the target position instantly.
    // If you want to snap the current view to the center, call center_view first, then call this function.
    
    if (!view)
        view = get_active_view(app, Access_ReadVisible);
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
    scroll.position = scroll.target;
    view_set_buffer_scroll(app, view, scroll, SetBufferScroll_SnapCursorIntoView);
    no_mark_snap_to_cursor(app, view);
}

//- NOTE(long): Misc
function Rect_f32 Long_Camera_PushBounds(Application_Links* app, View_ID view)
{
    Rect_f32 result = {0};
    view_get_camera_bounds(app, view, &result.p0, &result.p1);
    view_set_camera_bounds(app, view, V2f32(result.x0, clamp_bot(200.f, result.y0)), result.p1);
    return result;
}

function void Long_Camera_PopBounds(Application_Links* app, View_ID view, Rect_f32 bounds)
{
    view_disable_highlight_range(app, view);
    view_set_camera_bounds(app, view, bounds.p0, bounds.p1);
}

function void Long_Block_CopyStr(char* array, u64 size, String8 string)
{
    u64 copy_size = clamp_top(string.size, size - 1);
    block_copy(array, string.str, copy_size);
    array[copy_size] = 0;
}

//- NOTE(long): Mark/Cursor
function void Long_SnapMarkToCursor(Application_Links* app)
{
    if (fcoder_mode == FCoderMode_NotepadLike)
    {
        Long_View_ForEach(view, app, 0)
        {
            i64 pos = view_get_cursor_pos(app, view);
            view_set_mark(app, view, seek_pos(pos));
            
            Managed_Scope scope = view_get_managed_scope(app, view);
            b32* snap_mark_to_cursor = scope_attachment(app, scope, view_snap_mark_to_cursor, b32);
            *snap_mark_to_cursor = true;
        }
    }
}

function void Long_ConfirmMarkSnapToCursor(Application_Links* app)
{
    if (fcoder_mode == FCoderMode_NotepadLike)
    {
        Long_View_ForEach(view, app, 0)
        {
            Managed_Scope scope = view_get_managed_scope(app, view);
            b32* snap_mark_to_cursor = scope_attachment(app, scope, view_snap_mark_to_cursor, b32);
            if (*snap_mark_to_cursor)
            {
                i64 pos = view_get_cursor_pos(app, view);
                view_set_mark(app, view, seek_pos(pos));
            }
        }
    }
}

// @COPYPASTA(long): mouse_wheel_scroll
function void Long_Mouse_ScrollWheel(Application_Links* app, View_ID view)
{
    Mouse_State mouse = get_mouse_state(app);
    if (mouse.wheel)
    {
        Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
        scroll.target = view_move_buffer_point(app, view, scroll.target, V2f32(0.f, (f32)mouse.wheel));
        
        i64 cursor = view_get_cursor_pos(app, view);
        view_set_buffer_scroll(app, view, scroll, SetBufferScroll_SnapCursorIntoView);
        view_set_cursor(app, view, seek_pos(cursor));
    }
    
    if (mouse.l)
        no_mark_snap_to_cursor(app, view);
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
    
    // TODO(long): This works: my/jump/location.cpp(2904): abc
    // But not this: my/jump/location.cpp(2904):
    // Or this: my/jump/location.cpp(2904):abc
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
    Assert(view && buffer);
    if (push_src)
        Long_PointStack_PushCurrent(app, view);
    view_set_buffer(app, view, buffer, 0);
    if (push_dst)
        Long_PointStack_PushCurrent(app, view);
}

//- NOTE(long): Buffer
function Buffer_ID Long_Buffer_GetSearchBuffer(Application_Links* app)
{
    return get_buffer_by_name(app, search_name, Access_Always);
}

function b32 Long_Buffer_IsSearch(Application_Links* app, Buffer_ID buffer)
{
    Scratch_Block scratch(app);
    String8 name = push_buffer_base_name(app, scratch, buffer);
    return string_match(name, search_name);
}

function String8 Long_Buffer_PushLine(Application_Links* app, Arena* arena, Buffer_ID buffer, i64 pos)
{
    Buffer_Cursor cursor = buffer_compute_cursor(app, buffer, seek_pos(pos));
    String8 line = push_buffer_line(app, arena, buffer, cursor.line);
    return line;
}

function b32 Long_Buffer_NoSearch(Application_Links* app, Buffer_ID buffer)
{
    b32 result = 0;
    Scratch_Block scratch(app);
    String8 str = push_buffer_unique_name(app, scratch, buffer);
    if (str.size > 0 && str.str[0] == '*')
        result = !(string_match(str, S8Lit("*scratch*")) || string_match(str, S8Lit("*calc*")));
    return result;
}

function b32 Long_Buffer_IsSpecial(Application_Links* app, String8 name)
{
    b32 special_buffer = (string_match(name, S8Lit("*compilation*")) ||
                          string_match(name, S8Lit("*messages*")) ||
                          string_match(name, S8Lit("*search*")));
    return special_buffer;
}

//- NOTE(long): String
function String8 StrListJoin(Arena* arena, String8List list, String8 separator, String_Separator_Flag flags)
{
    b32 before_first = HasFlag(flags, StringSeparator_BeforeFirst);
    b32 after_last = HasFlag(flags, StringSeparator_AfterLast);
    u64 separator_size = separator.size * clamp_bot(list.node_count + before_first + after_last - 1, 0);
    String_u8 string = string_u8_push(arena, list.total_size + separator_size + 1);
    
    if (before_first)
        string_append(&string, separator);
    
    for (String8Node* node = list.first; node; node = node->next)
    {
        string_append(&string, node->string);
        if (node->next)
            string_append(&string, separator);
    }
    
    if (after_last)
        string_append(&string, separator);
    
    string.str[string.size] = 0;
    return string.string;
}

function void Long_Print_Messagef(Application_Links* app, char* fmt, ...)
{
    Scratch_Block scratch(app);
    va_list args;
    va_start(args, fmt);
    String8 message = push_stringfv(scratch, fmt, args);
    print_message(app, message);
    va_end(args);
}

function void Long_Print_Errorf(Application_Links* app, char* fmt, ...)
{
    Scratch_Block scratch(app);
    va_list args;
    va_start(args, fmt);
    String8 error_str = push_stringfv(scratch, fmt, args);
    va_end(args);
    
    Buffer_ID buffer_comp = get_comp_buffer(app);
    buffer_replace_range(app, buffer_comp, buffer_range(app, buffer_comp), error_str);
    block_zero_struct(&prev_location);
    lock_jump_buffer(app, buffer_comp);
    get_or_make_list_for_buffer(app, &global_heap, buffer_comp);
    print_message(app, error_str);
}

//~ NOTE(long): Vars/Config Functions

// @COPYPASTA(long): vars_print_indented
function void Long_Vars_Append(Application_Links* app, Arena* arena, String8List* list, String8List* ignore,
                               Variable_Handle var, i32 indent)
{
    Scratch_Block scratch(app, arena);
    local_persist char spaces[] =
        "                                                                "
        "                                                                "
        "                                                                "
        "                                                                ";
    
    String8 var_key = vars_key_from_var(scratch, var);
    for (String8Node* node = ignore->first; node; node = node->next)
        if (string_match(node->string, var_key))
            return;
    
    if (string_is_integer(var_key, 10)) // Each element of an array has its index as its name
        var_key = {};
    
    String8 var_val = vars_string_from_var(scratch, var);
    string_list_pushf(arena, list, "%.*s%.*s%s%.*s", clamp_top(indent, sizeof(spaces)), spaces,
                      string_expand(var_key), var_key.size ? ": " : "", string_expand(var_val));
    
    for (Vars_Children(sub, var))
        Long_Vars_Append(app, arena, list, ignore, sub, indent + 1);
}

function String8 Long_Vars_StrFromArray(Application_Links* app, Arena* arena, Variable_Handle var, i32 indent, String8List ignore)
{
    Scratch_Block scratch(app, arena);
    String8List list = {};
    for (Vars_Children(sub, var))
        Long_Vars_Append(app, scratch, &list, &ignore, sub, indent);
    String8 result = StrListJoin(arena, list, S8Lit("\n"), 0);
    return result;
}

function String8 Long_Vars_SingleStrArray(Application_Links* app, Arena* arena, Variable_Handle var)
{
    Scratch_Block scratch(app, arena);
    String8List list = {};
    for (Vars_Children(sub, var))
    {
        String8 val = vars_string_from_var(scratch, sub);
        string_list_pushf(scratch, &list, "\"%.*s\"", string_expand(val));
    }
    String8 result = StrListJoin(arena, list, S8Lit(", "), 0);
    return result;
}

//~ NOTE(long): Point Stack Commands

function b32 Long_PointStack_Read(Application_Links* app, Long_Point_Stack* stack, u32 index, Buffer_ID* out_buffer, i64* out_pos)
{
    index = PS_WrapIndex(index);
    Marker* marker = (Marker*)managed_object_get_pointer(app, stack->markers[index]);
    if (marker)
    {
        if (out_buffer) *out_buffer = stack->buffers[index];
        if (out_pos) *out_pos = marker->pos;
    }
    return !!marker;
}

function Long_Point_Stack* Long_PointStack_FromView(Application_Links* app, View_ID view = 0)
{
    if (view == 0)
        view = get_active_view(app, Access_Always);
    Managed_Scope scope = view_get_managed_scope(app, view);
    Long_Point_Stack* stack = scope_attachment(app, scope, long_point_stacks, Long_Point_Stack);
    
    for (i32 i = (i32)stack->top - 1; i >= (i32)stack->bot; --i)
    {
        if (!Long_PointStack_Read(app, stack, i, 0, 0))
        {
            for (u32 j = i; j < stack->top; ++j)
            {
                stack->markers[j] = stack->markers[j+1];
                stack->buffers[j] = stack->buffers[j+1];
            }
            stack->top--;
        }
    }
    
    return stack;
}

function void Long_PointStack_Push(Application_Links* app, Buffer_ID buffer, i64 pos, View_ID view)
{
    Long_Point_Stack* stack = Long_PointStack_FromView(app, view);
    
    // NOTE(long): Only push a point if it isn't the same as the current one
    if (stack->bot != stack->top)
    {
        Buffer_ID top_buffer = 0; i64 top_pos = 0;
        Long_PointStack_Read(app, stack, stack->curr, &top_buffer, &top_pos);
        if (buffer == top_buffer && pos == top_pos)
        {
            stack->top = stack->curr + 1;
            return;
        }
    }
    
    if (stack->bot != stack->top)
        stack->curr++;
    
    // NOTE(long): Free and assign the new item to the top (current) position
    {
        Managed_Object object = alloc_buffer_markers_on_buffer(app, buffer, 1, 0);
        Marker* marker = (Marker*)managed_object_get_pointer(app, object);
        marker->pos = pos;
        marker->lean_right = 0;
        
        u32 index = PS_WrapIndex(stack->curr);
        managed_object_free(app, stack->markers[index]);
        stack->markers[index] = object;
        stack->buffers[index] = buffer;
    }
    
    // NOTE(long): Increase the top (and bot if need to)
    stack->top = stack->curr + 1;
    if (stack->top > stack->bot + LONG_POINT_STACK_DEPTH)
        stack->bot++;
}

function void Long_PointStack_PushCurrent(Application_Links* app, View_ID view)
{
    Buffer_ID buffer = view_get_buffer(app, view, 0);
    i64 pos = view_get_cursor_pos(app, view);
    Long_PointStack_Push(app, buffer, pos, view);
}

function void Long_PointStack_SetCurrent(Long_Point_Stack* stack, u32 index)
{
    stack->curr = clamp(stack->bot, index, stack->top - 1);
}

function void Long_PointStack_Append(Application_Links* app, Long_Point_Stack* stack, Buffer_ID buffer, i64 pos, View_ID view)
{
    Long_PointStack_SetCurrent(stack, stack->top - 1);
    Long_PointStack_Push(app, buffer, pos, view);
}

CUSTOM_UI_COMMAND_SIG(long_point_lister)
CUSTOM_DOC("List jump history")
{
    Scratch_Block scratch(app);
    Lister_Block lister(app, scratch);
    lister_set_default_handlers(lister);
    
    Long_Point_Stack* stack = Long_PointStack_FromView(app);
    for (u32 i = stack->bot; i < stack->top; ++i)
    {
        Buffer_ID buffer = 0; i64 pos = 0;
        if (!Long_PointStack_Read(app, stack, i, &buffer, &pos))
            continue;
        
        String8 line = Long_Buffer_PushLine(app, scratch, buffer, pos);
        line = string_condense_whitespace(scratch, line);
        line = string_skip_chop_whitespace(line);
        
        String8 tag = (i == stack->curr) ? S8Lit("current") : String8{};
        Long_Lister_AddItem(app, lister, line, tag, buffer, pos, i);
    }
    
    lister_set_query(lister, push_u8_stringf(scratch, "Top: %u, Bot: %u, Current: %u", stack->top, stack->bot, stack->curr));
    Lister_Result l_result = Long_Lister_Run(app, lister);
    
    Long_Lister_Data result = {};
    if (Long_Lister_IsResultValid(l_result))
        block_copy_struct(&result, (Long_Lister_Data*)l_result.user_data);
    
    if (result.buffer != 0)
    {
        Long_Jump_ToLocation(app, get_this_ctx_view(app, Access_Always), result.buffer, result.pos);
        Long_PointStack_SetCurrent(stack, (u32)result.user_index);
    }
}

function void Long_PointStack_Jump(Application_Links* app, View_ID view, Buffer_ID target_buffer, i64 target_pos)
{
    Long_PointStack_PushCurrent(app, view);
    Long_Jump_ToLocation(app, view, target_buffer, target_pos);
    Long_PointStack_Push(app, target_buffer, target_pos, view);
}

function void Long_PointStack_JumpNext(Application_Links* app, View_ID view, i32 advance)
{
    Long_Point_Stack* stack = Long_PointStack_FromView(app, view);
    if (!stack || stack->top == stack->bot)
        return;
    
    Buffer_ID current_buffer = view_get_buffer(app, view, Access_Always);
    i64 current_pos = view_get_cursor_pos(app, view);
    u32 end = advance > 0 ? stack->top : (stack->bot - 1);
    b32 exit_early = advance == 0;
    if (exit_early)
        advance = -1;
    
    for (u32 i = stack->curr; i != end; i += advance)
    {
        Buffer_ID buffer = 0; i64 pos = 0;
        if (Long_PointStack_Read(app, stack, i, &buffer, &pos))
        {
            if (exit_early)
                goto EXIT;
            
            // NOTE(long): If virtual whitespace is enabled, we can't jump to the start of the line
            // Long_IsPosValid will check for that and make sure pos is different from the current pos
            b32 pos_is_valid = Long_IsPosValid(app, view, buffer, pos, current_pos);
            
            if (buffer != current_buffer || pos_is_valid)
            {
                if (i + 1 == stack->top && advance < 0)
                {
                    i64 current_line = get_line_number_from_pos(app, current_buffer, current_pos);
                    i64 line = get_line_number_from_pos(app, buffer, pos);
                    if (current_buffer != buffer || Long_Abs((i32)(line - current_line)) >= 35)
                        Long_PointStack_Append(app, stack, current_buffer, current_pos, view);
                }
                
                EXIT:
                Long_Jump_ToLocation(app, view, buffer, pos);
                Long_PointStack_SetCurrent(stack, i);
                break;
            }
        }
    }
}

CUSTOM_COMMAND_SIG(long_undo_jump)
CUSTOM_DOC("Read from the current point stack and jump there; if already there go to the prev option")
{
    Long_PointStack_JumpNext(app, get_active_view(app, Access_Always), -1);
}

CUSTOM_COMMAND_SIG(long_redo_jump)
CUSTOM_DOC("Read from the current point stack and jump there; if already there go to the next option")
{
    Long_PointStack_JumpNext(app, get_active_view(app, Access_Always), +1);
}

CUSTOM_COMMAND_SIG(long_push_new_jump)
CUSTOM_DOC("Push the current position to the point stack; if the stack's current is the position then ignore it")
{
    View_ID view = get_active_view(app, Access_Always);
    Long_PointStack_PushCurrent(app, view);
}

//~ NOTE(long): Buffer Commands

//- NOTE(long): Lister
function void Long_Buffer_OutputBuffer(Application_Links* app, Lister* lister, Buffer_ID buffer)
{
    Dirty_State dirty = buffer_get_dirty_state(app, buffer);
    String8 status = {};
    switch (dirty)
    {
        case DirtyState_UnsavedChanges:                   status = S8Lit("*");  break;
        case DirtyState_UnloadedChanges:                  status = S8Lit("!");  break;
        case DirtyState_UnsavedChangesAndUnloadedChanges: status = S8Lit("*!"); break;
    }
    
    Scratch_Block scratch(app, lister->arena);
    String8 buffer_name = push_buffer_base_name(app, scratch, buffer);
    String8 filepath = Long_Prj_RelBufferName(app, scratch, buffer);
    if (filepath.size)
        status = push_stringf(scratch, "<%.*s>%s%.*s", string_expand(filepath), dirty ? " " : "", string_expand(status));
    //lister_add_item(lister, buffer_name, status, IntAsPtr(buffer), 0);
    Long_Lister_AddBuffer(app, lister, buffer_name, status, buffer);
}

// @COPYPASTA(long): generate_all_buffers_list
function void Long_Buffer_GenerateLists(Application_Links* app, Lister* lister)
{
    lister_begin_new_item_set(app, lister);
    
    Buffer_ID viewed_buffers[16];
    i32 viewed_buffer_count = 0;
    
    // List currently viewed buffers
    Long_View_ForEach(view, app, 0)
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

function Buffer_ID Long_Buffer_RunLister(Application_Links* app, String8 query)
{
    Scratch_Block scratch(app);
    Lister_Handlers handlers = lister_get_default_handlers();
    handlers.refresh = Long_Buffer_GenerateLists;
    Lister_Result l_result = run_lister_with_refresh_handler(app, scratch, query, handlers);
    Buffer_ID result = 0;
    if (Long_Lister_IsResultValid(l_result))
        result = ((Long_Lister_Data*)l_result.user_data)->buffer;
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
                String8 line = Long_Buffer_PushLine(app, scratch, buffer, pos);
                line = string_condense_whitespace(scratch, line);
                line = string_skip_chop_whitespace(line);
                Long_Lister_AddItem(app, lister, line, {}, buffer, pos, i);
            }
        }
        
        Lister_Result l_result = Long_Lister_Run(app, lister);
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
        
        for (i32 i = 1; i < state->recent_buffer_count; ++i)
        {
            Buffer_ID buffer = state->recent_buffers[i];
            if (buffer_exists(app, buffer))
            {
                String8 buffer_name = push_buffer_unique_name(app, scratch, buffer);
                lister_add_item(lister, buffer_name, S8Lit(""), IntAsPtr(buffer), 0);
            }
        }
        
        Lister_Result l_result = Long_Lister_Run(app, lister);
        if (Long_Lister_IsResultValid(l_result))
            view_set_buffer(app, view, (Buffer_ID)PtrAsInt(l_result.user_data), 0);
    }
}
#endif

CUSTOM_UI_COMMAND_SIG(long_interactive_switch_buffer)
CUSTOM_DOC("Interactively switch to an open buffer.")
{
    // @COPYPASTA(long): interactive_switch_buffer
    Buffer_ID buffer = Long_Buffer_RunLister(app, S8Lit("Switch:"));
    if (buffer)
    {
        View_ID view = get_this_ctx_view(app, Access_Always);
        Long_Jump_ToBuffer(app, view, buffer);
    }
}

function void Long_Buffer_Kill(Application_Links* app, Buffer_ID buffer, View_ID view)
{
    View_ID views_with_buffer[16] = {}; // 16 is the max number of views
    i64 count = 0;
    Long_View_ForEach(view_with_buffer, app, 0)
    {
        Buffer_ID view_buffer = view_get_buffer(app, view_with_buffer, 0);
        if (view_buffer == buffer)
            views_with_buffer[count++] = view_with_buffer;
    }
    
    b32 killed = try_buffer_kill(app, buffer, view, 0) == BufferKillResult_Killed;
    if (killed)
    {
        for (i32 i = 0; i < count; ++i)
        {
            Long_PointStack_JumpNext(app, views_with_buffer[i], 0);
            view_set_active(app, view); // JumpNext will change the active panel, so we need to reset it back
        }
    }
}

CUSTOM_UI_COMMAND_SIG(long_interactive_kill_buffer)
CUSTOM_DOC("Interactively kill an open buffer.")
{
    Long_Buffer_Kill(app, Long_Buffer_RunLister(app, S8Lit("Kill:")), get_this_ctx_view(app, Access_Always));
}

CUSTOM_UI_COMMAND_SIG(long_interactive_open_or_new)
CUSTOM_DOC("Interactively open a file out of the file system.")
{
    Scratch_Block scratch(app);
    View_ID view = get_this_ctx_view(app, Access_Always);
    
    REPEAT:
    File_Name_Result result = get_file_name_from_user(app, scratch, "Open:", view);
    if (result.canceled)
        return;
    
    String8 file_name = result.file_name_activated;
    if (!file_name.size)
        file_name = result.file_name_in_text_field;
    if (!file_name.size)
        return;
    
    String8 path = result.path_in_text_field;
    String8 full_file_name = push_u8_stringf(scratch, "%.*s/%.*s", string_expand(path), string_expand(file_name));
    String8 new_hot_dir = {};
    if (result.is_folder)
        new_hot_dir = full_file_name;
    
    else if (character_is_slash(file_name.str[file_name.size - 1]))
    {
        File_Attributes attribs = system_quick_file_attributes(scratch, full_file_name);
        if (HasFlag(attribs.flags, FileAttribute_IsDirectory))
            new_hot_dir = full_file_name;
        else if (string_looks_like_drive_letter(file_name))
            new_hot_dir = file_name;
        else if (query_create_folder(app, file_name))
            new_hot_dir = full_file_name;
        else return;
    }
    
    if (new_hot_dir.size)
    {
        set_hot_directory(app, new_hot_dir);
        goto REPEAT;
    }
    
    Buffer_ID buffer = create_buffer(app, full_file_name, 0);
    if (buffer)
        Long_Jump_ToBuffer(app, view, buffer);
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
    
    Lister_Result l_result = Long_Lister_Run(app, lister);
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
        b32 enable_undo_fade_out = def_get_config_b32_lit("enable_undo_fade_out");
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
                ARGB_Color color = Long_ARGBFromID(defcolor_undo) & 0xFFFFFF;
                Fade_Range* fade = buffer_post_fade(app, buffer, undo_fade_time, range, color);
                fade->negate_fade_direction = true;
                fade->finish_call = Long_Undo_FinishFade;
                do_immedite_undo = false;
                if (new_position > range.max){
                    new_position -= range_size(range);
                }
            }
        }
        
        if (do_immedite_undo)
        {
            buffer_history_set_current_state_index(app, buffer, current - 1);
            Long_Buffer_CheckDirtyHistory(app, buffer);
            if (record.single_string_backward.size > 0)
            {
                Range_i64 range = Ii64_size(record.single_first, record.single_string_backward.size);
                buffer_post_fade(app, buffer, undo_fade_time, range, Long_ARGBFromID(defcolor_undo));
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
            buffer_post_fade(app, buffer, .33f, range, Long_ARGBFromID(defcolor_undo));
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

// @COPYPASTA(long): write_text
function void Long_Write_Text(Application_Links* app, View_ID view, Buffer_ID buffer, String8 insert)
{
    ProfileScope(app, "[Long] Write Character");
    if (insert.size && buffer)
    {
        i64 pos = view_get_cursor_pos(app, view);
        pos = view_get_character_legal_pos_from_pos(app, view, pos);
        
        // NOTE(allen): consecutive inserts merge logic
        History_Record_Index first_index = buffer_history_get_current_state_index(app, buffer);
        b32 do_merge = 0;
        if (insert.str[0] != '\n')
        {
            Record_Info record = get_single_record(app, buffer, first_index);
            if (record.error == RecordError_NoError && record.kind == RecordKind_Single)
            {
                String8 str = record.single_string_forward;
                if (record.single_first + (i64)str.size == pos && str.size > 0)
                {
                    char c = str.str[str.size - 1];
                    char c0 = insert.str[0];
                    if (c != '\n')
                        if ((character_is_whitespace   (c0) && character_is_whitespace   (c)) ||
                            (character_is_alpha_numeric(c0) && character_is_alpha_numeric(c)))
                            do_merge = 1;
                }
            }
        }
        
        // NOTE(allen): perform the edit
        Range_i64 range = Ii64(pos);
        {
            Managed_Scope scope = view_get_managed_scope(app, view);
            Rewrite_Type* rewrite = scope_attachment(app, scope, view_rewrite_loc, Rewrite_Type);
            if (*rewrite == Long_Rewrite_ReplaceRange || view_has_highlighted_range(app, view))
                range = get_view_range(app, view);
        }
        b32 edit_success = buffer_replace_range(app, buffer, range, insert);
        
        // NOTE(allen): finish merging records if necessary
        if (do_merge)
        {
            History_Record_Index last_index = buffer_history_get_current_state_index(app, buffer);
            buffer_history_merge_record_range(app, buffer, first_index, last_index, RecordMergeFlag_StateInRange_MoveStateForward);
        }
        
        // NOTE(allen): finish updating the cursor
        if (edit_success)
            view_set_cursor_and_preferred_x(app, view, seek_pos(range.min + insert.size));
    }
}

// @COPYPASTA(long): write_text_and_auto_indent
CUSTOM_COMMAND_SIG(long_write_text_and_auto_indent)
CUSTOM_DOC("Inserts text and auto-indents the line on which the cursor sits if any of the text contains 'layout punctuation' such as ;:{}()[]# and new lines.")
{
    ProfileScope(app, "[Long] write and auto indent");
    
    User_Input in = get_current_input(app);
    String8 insert = to_writable(&in);
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    
    if (insert.size && buffer)
    {
        b32 do_auto_indent = false;
        for (u64 i = 0; !do_auto_indent && i < insert.size; i += 1)
        {
            switch (insert.str[i])
            {
                case ';': case ':':
                case '{': case '}':
                case '(': case ')':
                case '[': case ']':
                case '#':
                case '\n': case '\t': do_auto_indent = true; break;
            }
        }
        
        if (do_auto_indent)
        {
            Range_i64 pos = {};
            if (view_has_highlighted_range(app, view))
                pos = get_view_range(app, view);
            else
                pos = Ii64(view_get_cursor_pos(app, view));
            
            Long_Write_Text(app, view, buffer, insert);
            
            i64 end_pos = view_get_cursor_pos(app, view);
            pos.min = Min(pos.min, end_pos);
            pos.max = Max(pos.max, end_pos);
            Long_Index_IndentBuffer(app, buffer, pos, true);
            move_past_lead_whitespace(app, view, buffer);
        }
        else Long_Write_Text(app, view, buffer, insert);
    }
}

//- NOTE(long): Special Buffers/Panels
CUSTOM_COMMAND_SIG(long_kill_buffer)
CUSTOM_DOC("Kills the current buffer.")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    Long_Buffer_Kill(app, buffer, view);
}

CUSTOM_COMMAND_SIG(long_switch_to_search_buffer)
CUSTOM_DOC("Switch to the search buffer.")
{
    Buffer_ID search_buffer = Long_Buffer_GetSearchBuffer(app);
    if (search_buffer)
        Long_Jump_ToBuffer(app, get_this_ctx_view(app, Access_Always), search_buffer, 1, 0);
}

CUSTOM_COMMAND_SIG(long_kill_search_buffer)
CUSTOM_DOC("Kills the current search jump buffer.")
{
    Long_Buffer_Kill(app, Long_Buffer_GetSearchBuffer(app), get_active_view(app, Access_ReadVisible));
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
    if (!global_compilation_view)
        return;
    
    u32 line_count = (global_compilation_view_expanded ^= 1) ? 31 : 8;
    f32 padding = 3.f;
    f32 split_height;
    {
        Buffer_ID buffer = view_get_buffer(app, global_compilation_view, Access_Always);
        f32 line_height = get_face_metrics(app, get_face_id(app, buffer)).line_height;
        f32 bar_height = get_face_metrics(app, get_face_id(app, 0)).line_height + 2.f; // layout_file_bar_on_top
        f32 margin_size = def_get_config_f32_lit(app, "long_margin_size");
        split_height = line_height * line_count + bar_height + margin_size*2 + padding;
    }
    
    // @COPYPASTA(long): view_set_split
    {
        Panel_ID comp_panel = view_get_panel(app, global_compilation_view);;
        Panel_ID footer_panel = panel_get_parent(app, comp_panel);
        Panel_ID parent_panel = panel_get_parent(app, footer_panel);
        Panel_ID min_panel = panel_get_child(app, parent_panel, Side_Min);
        if (min_panel)
        {
            Panel_Split_Kind panel_kind = (min_panel == footer_panel ?
                                           PanelSplitKind_FixedPixels_Min : PanelSplitKind_FixedPixels_Max);
            panel_set_split(app, parent_panel, panel_kind, split_height);
        }
    }
    
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
    if (view_get_is_passive(app, view))
        view = long_global_active_view;
    else
        view = get_next_view_looped_primary_panels(app, view, Access_Always);
    view_set_active(app, view);
}

CUSTOM_COMMAND_SIG(long_change_passive_panel)
CUSTOM_DOC("Change the currently active panel, moving to the panel with the next highest view_id.")
{
    View_ID view = get_active_view(app, Access_Always);
    
    if (!view_get_is_passive(app, view))
        view = global_compilation_view;
    else
    {
        View_ID start_view = view;
        do
        {
            view = get_next_view_looped_all_panels(app, view, 0);
            if (view_get_is_passive(app, view))
                break;
        } while (view != start_view);
    }
    
    view_set_active(app, view);
}

//~ NOTE(long): ListAllLocations

// @COPYPASTA(long): create_or_switch_to_buffer_and_clear_by_name
function Buffer_ID Long_CreateOrSwitchBuffer(Application_Links* app, String8 name_string, View_ID default_target_view)
{
    Buffer_ID buffer = get_buffer_by_name(app, name_string, Access_Always);
    b32 jump_to_buffer = 1;
    
    if (buffer)
    {
        View_ID target_view = get_first_view_with_buffer(app, buffer);
        if (target_view)
        {
            default_target_view = target_view;
            jump_to_buffer = 0;
        }
        
        buffer_set_setting(app, buffer, BufferSetting_ReadOnly, true);
        clear_buffer(app, buffer);
        buffer_send_end_signal(app, buffer);
    }
    
    else
    {
        buffer = create_buffer(app, name_string, BufferCreate_AlwaysNew);
        buffer_set_setting(app, buffer, BufferSetting_Unimportant, true);
        buffer_set_setting(app, buffer, BufferSetting_ReadOnly, true);
    }
    
    // NOTE(long): This will overwrite the buffer's history from this point forward
    Long_Jump_ToBuffer(app, default_target_view, buffer, 1, 0);
    return buffer;
}

// @COPYPASTA(long): find_all_matches_all_buffers
function String_Match_List Long_Matches_Find(Application_Links* app, Arena* arena, String8Array match_patterns,
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

function void Long_Matches_Print(Application_Links* app, String8Array match_patterns, String_Match_Flag must_have_flags,
                                 String_Match_Flag must_not_have_flags, Buffer_ID out_buffer_id, Buffer_ID current_buffer)
{
    Scratch_Block scratch(app);
    String_Match_List matches = Long_Matches_Find(app, scratch, match_patterns,
                                                  must_have_flags, must_not_have_flags, current_buffer);
    string_match_list_filter_remove_buffer(&matches, out_buffer_id);
    string_match_list_filter_remove_buffer_predicate(app, &matches, Long_Buffer_NoSearch);
    print_string_match_list_to_buffer(app, out_buffer_id, matches);
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

function Sticky_Jump Long_SearchBuffer_NearestJump(Application_Links* app, Buffer_ID buffer, i64 pos,
                                                   Marker_List* list, Managed_Scope scope, Sticky_Jump_Stored* out_stored)
{
    Scratch_Block scratch(app);
    
    i32 list_index = -1;
    Sticky_Jump_Stored* storage = 0;
    
    i32 count;
    Marker* markers = Long_SearchBuffer_GetMarkers(app, scratch, scope, buffer, &count);
    if (markers)
    {
        i32 marker_index = binary_search(&markers->pos, sizeof(*markers), count, pos);
        storage = get_all_stored_jumps_from_list(app, scratch, list);
        for (i32 i = 0; i < list->jump_count; ++i)
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
        
        if (out_stored)
            *out_stored = storage[list_index];
    }
    return result;
}

function void Long_MC_ListAllLocations(Application_Links* app, View_ID view, Buffer_ID search_buffer,
                                       String8 title, i64 init_size)
{
    if (get_active_view(app, Access_ReadVisible) != view)
        view_set_active(app, view); 
    
    Scratch_Block scratch(app);
    Query_Bar_Group group(app);
    Query_Bar* bar = Long_Query_StartBar(app, scratch, 0, title);
    if (!bar)
        return;
    
    Rect_f32 bounds = Long_Camera_PushBounds(app, view);
    
    auto_center_after_jumps = false;
    Managed_Scope scope = buffer_get_managed_scope(app, search_buffer);
    
    i64 curr_pos = 0;
    i64 cursor_pos = view_get_cursor_pos(app, view), mark_pos = view_get_mark_pos(app, view);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    {
        lock_jump_buffer(app, search_buffer);
        Locked_Jump_State jump_state = get_locked_jump_state(app, &global_heap);
        
        Sticky_Jump jump = Long_SearchBuffer_NearestJump(app, buffer, cursor_pos, jump_state.list, scope, 0);
        if (jump.jump_buffer_id)
            Long_SearchBuffer_Jump(app, jump_state, jump);
        else
            goto_first_jump(app);
        
        curr_pos = view_get_cursor_pos(app, view);
        view_set_mark  (app, view, seek_pos(curr_pos + title.size));
        view_set_cursor(app, view, seek_pos(curr_pos));
    }
    
    i32 count;
    Marker* markers = Long_SearchBuffer_GetMarkers(app, scratch, scope, buffer, &count);
    
    Mapping* mapping = 0;
    Command_Map* map = Long_Mapping_GetMap(app, view, &mapping);
    
    b32 abort = false, exit_to_jump_highlight = false;
    String_ID jump_highlight_id = vars_save_string_lit("use_jump_highlight");
    def_set_config_b32(jump_highlight_id, 0);
    
    b32 select_all = 0;
    for (Temp_Memory temp = begin_temp(scratch); ; end_temp(temp))
    {
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
        
        i64 selection_count = 1;
        for_mc(node, mc_context.cursors) ++selection_count;
        bar->prompt = push_stringf(scratch, "Selection Count: %d, For: ", selection_count);
        
        ID_Pos_Jump_Location current_location;
        get_jump_from_list(app, jump_state.list, jump_state.list_index, &current_location);
        curr_pos = current_location.pos;
        
        // NOTE(long): Rather than disable the highlight, we set it to an empty range so that F4_RenderBuffer doesn't render any cursor
        view_set_highlight_range(app, view, {});
        
        if (match_key_code(&in, KeyCode_Return) || match_key_code(&in, KeyCode_Tab))
            break;
        
        else if (match_key_code(&in, KeyCode_PageUp) || match_key_code(&in, KeyCode_Up))
        {
            goto_prev_jump(app);
            
            CHANGE_JUMP:
            i64 new_pos = view_get_cursor_pos(app, view);
            Buffer_ID new_buffer = view_get_buffer(app, view, 0);
            if (curr_pos != new_pos || current_location.buffer_id != new_buffer)
            {
                i64 cursor = curr_pos;
                i64 mark = curr_pos + title.size;
                
                if (select_all)
                {
                    for (i32 i = 0; i < count; ++i)
                        MC_remove(app, cursor);
                    select_all = 0;
                }
                
                if (has_modifier(&in, KeyCode_Shift))
                {
                    b32 duplicate = 0;
                    for_mc(node, mc_context.cursors)
                    {
                        if (node->cursor_pos == cursor)
                        {
                            duplicate = 1;
                            break;
                        }
                    }
                    
                    if (duplicate)
                        MC_remove(app, cursor);
                    else
                        MC_insert(app, cursor, mark);
                }
            }
        }
        
        else if (match_key_code(&in, KeyCode_PageDown) || match_key_code(&in, KeyCode_Down))
        {
            goto_next_jump(app);
            goto CHANGE_JUMP;
        }
        
        else if (match_key_code(&in, KeyCode_A) && has_modifier(&in, KeyCode_Control))
        {
            for (i32 i = 0; i < count; ++i)
            {
                i64 cursor = markers[i].pos;
                i64 mark = markers[i].pos + title.size;
                if (select_all)
                    MC_remove(app, cursor);
                else
                    MC_insert(app, cursor, mark);
            }
            select_all = !select_all;
        }
        
        else if (match_key_code(&in, KeyCode_E) && has_modifier(&in, KeyCode_Control))
            center_view(app);
        
        else
        {
            String8 string = to_writable(&in);
            if (string.size)
            {
                enqueue_virtual_event(app, &in.event);
                set_next_rewrite(app, view, Long_Rewrite_ReplaceRange);
                break;
            }
            
            else if (in.event.kind == InputEventKind_KeyStroke || in.event.kind == InputEventKind_KeyRelease)
            {
                Command_Metadata* metadata = Long_Mapping_GetMetadata(mapping, map, &in);
                Custom_Command_Function* custom = metadata ? metadata->proc : 0;
                
                if (custom == long_toggle_compilation_expand || custom == mouse_wheel_scroll)
                {
                    custom(app);
                    custom = 0;
                }
                else if (custom == delete_char || custom == backspace_char)
                    custom = long_delete_range;
                
                if (custom)
                {
                    view_enqueue_command_function(app, view, custom);
                    break;
                }
                leave_current_input_unhandled(app);
            }
        }
        
        view_set_mark  (app, view, seek_pos(curr_pos + title.size));
        view_set_cursor(app, view, seek_pos(curr_pos));
    }
    
    if (exit_to_jump_highlight)
        def_set_config_b32(jump_highlight_id, 1);
    else
        long_kill_search_buffer(app);
    
    if (abort)
    {
        Long_Jump_ToLocation(app, view, buffer, cursor_pos);
        view_set_mark(app, view, seek_pos(mark_pos));
    }
    
    if (abort || exit_to_jump_highlight)
        MC_end(app);
    else
    {
        Long_PointStack_Push(app, buffer, cursor_pos, view);
        Long_PointStack_PushCurrent(app, view);
        MC_begin(app);
        // MC_begin always reset the mark back to the cursor
        view_set_mark(app, view, seek_pos(curr_pos + title.size));
    }
    
    auto_center_after_jumps = true;
    // NOTE(long): This will also disable the highlight and the cursor will be drawn again
    Long_Camera_PopBounds(app, view, bounds);
}

function void Long_ListAllLocations(Application_Links* app, String8 needle, List_All_Locations_Flag flags, b32 all_buffer)
{
    if (!needle.size)
        return;
    
    String_Match_Flag must_have_flags = 0;
    if (HasFlag(flags, ListAllLocationsFlag_CaseSensitive))
        AddFlag(must_have_flags, StringMatch_CaseSensitive);
    
    String_Match_Flag must_not_have_flags = 0;
    if (!HasFlag(flags, ListAllLocationsFlag_MatchSubstring))
        AddFlag(must_not_have_flags, StringMatch_LeftSideSloppy|StringMatch_RightSideSloppy);
    
    View_ID search_view = Long_View_NextPassive(app, global_compilation_view, 0);
    if (search_view == global_compilation_view)
        search_view = get_next_view_after_active(app, Access_Always);
    
    i64 before_search_cursor = view_get_cursor_pos(app, search_view);
    i64 before_search_mark = view_get_mark_pos(app, search_view);
    Buffer_ID before_search_buffer = view_get_buffer(app, search_view, 0);
    
    Buffer_ID search_buffer = Long_CreateOrSwitchBuffer(app, search_name, search_view);
    Face_ID search_face = get_face_id(app, 0);
    b32 is_passive = view_get_is_passive(app, search_view);
    if (is_passive)
        search_face = get_face_id(app, view_get_buffer(app, global_compilation_view, 0));
    buffer_set_face(app, search_buffer, search_face);
    
    View_ID view = get_active_view(app, Access_Always);
    Buffer_ID current_buffer = view_get_buffer(app, view, Access_Always);
    String8Array patterns = { &needle, 1 };
    Long_Matches_Print(app, patterns, must_have_flags, must_not_have_flags,
                       search_buffer, all_buffer ? 0 : current_buffer);
    
    Scratch_Block scratch(app);
    String8 no_match = S8Lit("no matches");
    String8 search_result = push_buffer_range(app, scratch, search_buffer, Ii64_size(0, no_match.size));
    if (!string_match(search_result, no_match))
    {
        if (is_passive)
        {
            global_compilation_view_expanded = 0;
            long_toggle_compilation_expand(app);
        }
        Long_MC_ListAllLocations(app, view, search_buffer, needle, needle.size);
        
        if (!buffer_exists(app, search_buffer))
        {
            view_set_buffer(app, search_view, before_search_buffer, 0);
            view_set_cursor(app, search_view, seek_pos(before_search_cursor));
            view_set_mark(app, search_view, seek_pos(before_search_mark));
            
            if (is_passive)
            {
                global_compilation_view_expanded = 1;
                long_toggle_compilation_expand(app);
            }
        }
    }
}

function void Long_ListAllLocations_Query(Application_Links* app, char* query, List_All_Locations_Flag flags, b32 all_buffer)
{
    Scratch_Block scratch(app);
    String8 needle = fcoder_mode == FCoderMode_NotepadLike ? push_view_range_string(app, scratch) : String8{};
    needle = Long_Query_String(app, scratch, "List Locations For: ", needle);
    clear_all_query_bars(app, get_active_view(app, 0));
    Long_ListAllLocations(app, needle, flags, all_buffer);
}

function void Long_ListAllLocations_Identifier(Application_Links* app, List_All_Locations_Flag flags, b32 all_buffer)
{
    Scratch_Block scratch(app);
    View_ID view = get_active_view(app, 0);
    Buffer_ID buffer = view_get_buffer(app, view, 0);
    i64 pos = view_get_cursor_pos(app, view);
    String8 needle = push_enclose_range_at_pos(app, scratch, buffer, pos, enclose_alpha_numeric_underscore);
    Long_ListAllLocations(app, needle, flags, all_buffer);
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

//~ NOTE(long): MC Commands

function void Long_MC_InsertTrail(Application_Links* app, f32 pixels)
{
    View_ID view = get_active_view(app, 0);
    i64 cursor = view_get_cursor_pos(app, view);
    i64 marker = mc_context.active ? view_get_mark_pos(app, view) : cursor;
    
    if (!mc_context.active_cursor)
    {
        MC_insert(app, cursor, marker);
        
        if (pixels < 0)
            move_up(app);
        else
            move_down(app);
        
        if (mc_context.active)
        {
            // @COPYPASTA(long): move_vertical_pixels
            i64 line = view_compute_cursor(app, view, seek_pos(marker)).line;
            Rect_f32 r = view_padded_box_of_pos(app, view, line, marker);
            Vec2_f32 p = V2f32(r.x0, (pixels > 0.f ? r.y1 : r.y0) + pixels);
            i64 new_pos = view_pos_at_relative_xy(app, view, line, p);
            view_set_mark(app, view, seek_pos(new_pos));
        }
        else MC_begin(app);
    }
}

CUSTOM_COMMAND_SIG(long_mc_up_trail)
CUSTOM_DOC("moves up, leaving a multi-cursor behind it")
{
    Long_MC_InsertTrail(app, -1);
}

CUSTOM_COMMAND_SIG(long_mc_down_trail)
CUSTOM_DOC("moves down, leaving a multi-cursor behind it")
{
    Long_MC_InsertTrail(app, +1);
}

CUSTOM_COMMAND_SIG(long_mc_begin_multi_block)
CUSTOM_DOC("[MC] begins multi-cursor using cursor-mark block-rect")
{
    View_ID view = get_active_view(app, Access_Always);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
    u64 cursor = view_get_cursor_pos(app, view);
    u64 mark = view_get_mark_pos(app, view);
    
    MC_begin_multi_block(app);
    
    for (MC_Node** ptr = &mc_context.cursors; *ptr;)
    {
        i64 pos = (*ptr)->cursor_pos;
        i64 beg = get_pos_past_lead_whitespace(app, buffer, pos);
        i64 end = get_line_end_pos_from_pos   (app, buffer, pos);
        Token* token = get_token_from_pos(app, buffer, beg);
        
        if (token && token->kind == TokenBaseKind_Comment && token->pos + token->size >= end)
        {
            MC_Node* node = *ptr;
            *ptr = node->next;
            sll_stack_push(mc_context.free_list, node);
        }
        else ptr = &(*ptr)->next;
    }
    
    // Either there are no cursors originally or we've removed them all
    if (!mc_context.cursors)
    {
        view_set_cursor(app, view, seek_pos(cursor));
        view_set_mark  (app, view, seek_pos(mark));
        view_set_buffer_scroll(app, view, scroll, SetBufferScroll_SnapCursorIntoView);
        MC_end(app);
        Long_Render_FadeError(app, buffer, Ii64(cursor, mark));
    }
}

//~ NOTE(long): Query Commands

function Query_Bar* Long_Query_StartBar(Application_Links* app, Arena* arena, char* prompt, String8 string)
{
    Query_Bar* bar = push_array_zero(arena, Query_Bar, 1);
    bar->prompt = prompt ? SCu8(prompt) : string_u8_empty;
    bar->string = string;
    
    if (!start_query_bar(app, bar, 0))
    {
        bar = 0;
        pop_array(arena, Query_Bar, 1);
    }
    
    return bar;
}

function Query_Bar* Long_Query_ReserveBar(Application_Links* app, Arena* arena, char* prompt, String8 string)
{
    u64 reserve = LONG_QUERY_STRING_SIZE;
    String_u8 bar_string = string_u8_push(arena, clamp_bot(reserve, string.size));
    string_append(&bar_string, string);
    
    Query_Bar* bar = Long_Query_StartBar(app, arena, prompt, bar_string.string);
    if (bar)
        bar->string_capacity = reserve;
    
    return bar;
}

function void Long_Query_AppendBar(Query_Bar* bar, String8 string)
{
    String_u8 bar_string = Su8(bar->string, bar->string_capacity);
    string_append(&bar_string, string);
    bar->string.size = bar_string.size;
}

function b32 Long_Query_DefaultInput(Application_Links* app, Query_Bar* bar, View_ID view, User_Input* in)
{
    Scratch_Block scratch(app);
    b32 result = 1;
    String8 string = {};
    
    if (match_key_code(in, KeyCode_Backspace))
    {
        if (has_modifier(in, KeyCode_Control))
            bar->string.size = 0;
        else
            bar->string = backspace_utf8(bar->string);
    }
    
    else if (match_key_code(in, KeyCode_Tab))
    {
        Buffer_ID buffer = view_get_buffer(app, view, 0);
        Range_i64 range = get_view_range(app, view);
        
        if (!has_modifier(in, KeyCode_Control))
        {
            i64 pos = view_get_cursor_pos(app, view);
            range = enclose_pos_alpha_numeric_underscore_utf8(app, buffer, pos);
            
            // NOTE(long): The cursor can still be right after
            if (range.max == range.min)
                range = enclose_pos_alpha_numeric_underscore_utf8(app, buffer, pos - 1);
        }
        
        string = push_buffer_range(app, scratch, buffer, range);
    }
    
    else if (match_key_code(in, KeyCode_V) && has_modifier(in, KeyCode_Control))
        string = push_clipboard_index(scratch, 0, 0);
    else if (match_key_code(in, KeyCode_E) && has_modifier(in, KeyCode_Control))
        center_view(app);
    
    else if (in->event.kind == InputEventKind_TextInsert)
        string = in->event.text.string;
    else if (in->event.kind == InputEventKind_MouseWheel)
        Long_Mouse_ScrollWheel(app, view);
    
    else result = 0;
    
    if (string.size)
        Long_Query_AppendBar(bar, string);
    
    return result;
}

// @COPYPASTA(long): query_user_general
function b32 Long_Query_General(Application_Links* app, Query_Bar* bar, String8 init, u32 force_radix)
{
    if (init.size > 0)
        Long_Query_AppendBar(bar, init);
    
    View_ID view = get_active_view(app, 0);
    Buffer_ID buffer = view_get_buffer(app, view, 0);
    Rect_f32 bounds = Long_Camera_PushBounds(app, view);
    
    Mapping* mapping = 0;
    Command_Map* map = 0;
    
    b32 success = 1;
    for (;;)
    {
        User_Input in = get_next_input(app, EventPropertyGroup_Any, EventProperty_Escape);
        if (in.abort)
        {
            success = 0;
            break;
        }
        
        u64 old_size = bar->string.size;
        if (match_key_code(&in, KeyCode_Return))
            break;
        
        else if (Long_Query_DefaultInput(app, bar, view, &in))
        {
            if (force_radix && bar->string.size > old_size)
            {
                String8 new_str = string_skip(bar->string, old_size);
                if (!string_is_integer(new_str, 10))
                    bar->string.size = old_size;
            }
        }
        
        else if (Long_Mapping_HandleCommand(app, view, &mapping, &map, &in))
        {
            success = 0;
            break;
        }
    }
    
    Long_Camera_PopBounds(app, view, bounds);
    return success;
}

// @COPYPASTA(long): get_query_string
function String8 Long_Query_StringEx(Application_Links* app, Arena* arena, char* prompt,
                                     u8* string_space, u64 space_size, String8 init)
{
    String8 result = {};
    Query_Bar* bar = Long_Query_StartBar(app, arena, prompt, SCu8(string_space, 0ULL));
    
    if (bar)
    {
        bar->string_capacity = space_size;
        if (Long_Query_General(app, bar, init))
            result = bar->string;
    }
    
    // NOTE(long): result.size equals 0 doesn't necessarily mean that the query failed, but result.str equals 0 does
    return result;
}

function String8 Long_Query_String(Application_Links* app, Arena* arena, char* prompt, String8 init)
{
    u64 string_size = LONG_QUERY_STRING_SIZE;
    u8* string_space = push_array(arena, u8, string_size);
    return Long_Query_StringEx(app, arena, prompt, string_space, string_size, init);
}

function i64 Long_Query_Number(Application_Links* app, char* prompt, u64 max_digit)
{
    Scratch_Block scratch(app);
    i64 result = 0;
    
    String_u8 number_space = push_string_u8(scratch, max_digit);
    Query_Bar* bar = Long_Query_StartBar(app, scratch, prompt, number_space.string);
    
    if (bar)
    {
        bar->string_capacity = max_digit;
        if (Long_Query_General(app, bar, {}, 10))
            result = string_to_integer(bar->string, 10);
        end_query_bar(app, bar, 0);
    }
    
    return result;
}

function void Long_ISearch_Replace(Application_Links* app)
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    if (buffer)
    {
        Scratch_Block scratch(app);
        String8 replace = SCu8(previous_isearch_query);
        Query_Bar* bar = Long_Query_StartBar(app, scratch, "Replace: ", replace);
        Long_Query_Replace(app, replace, view_get_cursor_pos(app, view));
        end_query_bar(app, bar, 0);
    }
}

// @COPYPASTA(long): isearch
function void Long_ISearch(Application_Links* app, Scan_Direction start_scan, i64 first_pos, String8 init, b32 insensitive)
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    if (!buffer_exists(app, buffer))
        return;
    
    Scratch_Block scratch(app);
    Query_Bar_Group group(app);
    
    Query_Bar* bar = Long_Query_ReserveBar(app, scratch, 0, init);
    if (!bar) return;
    Rect_f32 bounds = Long_Camera_PushBounds(app, view);
    
    insensitive = !!insensitive;
    String8 isearch_str = insensitive ? S8Lit("I-Search: ") : S8Lit("I-Search-Sensitive: ");
    String8 rsearch_str = insensitive ? S8Lit("Reverse-I-Search: ") : S8Lit("Reverse-I-Search-Sensitve: ");
    
    Mapping* mapping = 0;
    Command_Map* map = Long_Mapping_GetMap(app, view, &mapping);
    
    Scan_Direction scan = 0;
    i64 pos = first_pos;
    i64 buffer_size = buffer_get_size(app, buffer);
    i64 match_size = 0;
    
    u64 old_size = bar->string.size + 1;
    Scan_Direction change_scan = start_scan;
    User_Input in = {};
    for (;;)
    {
        b32 string_change = old_size != bar->string.size;
        b32 do_scan_action = change_scan != 0; // Scan_Backward != 0 && Scan_Forward != 0
        
        if (string_change || do_scan_action)
        {
            if (do_scan_action)
                scan = change_scan;
            
            b32 backward = scan == Scan_Backward;
            i64 offset = string_change ? (backward ? +1 : -1) : 0;
            i64 new_pos = 0;
            seek_string(app, buffer, pos + offset, 0, 0, bar->string, &new_pos,
                        (backward*BufferSeekString_Backward)|(insensitive*BufferSeekString_CaseInsensitive));
            
            b32 pos_is_valid = new_pos >= 0 && new_pos < buffer_size;
            if (pos_is_valid)
            {
                if (do_scan_action && has_modifier(&in, KeyCode_Shift))
                {
                    b32 remove = 0;
                    for_mc(node, mc_context.cursors)
                    {
                        if (range_contains(Ii64(node->cursor_pos, node->mark_pos), pos))
                        {
                            remove = 1;
                            break;
                        }
                    }
                    
                    if (remove)
                    {
                        // @COPYPASTA(long): MC_remove
                        MC_Node** ptr = &mc_context.cursors;
                        while (*ptr)
                        {
                            if (range_contains(Ii64((*ptr)->cursor_pos, (*ptr)->mark_pos), pos))
                            {
                                MC_Node* node = *ptr;
                                *ptr = node->next;
                                sll_stack_push(mc_context.free_list, node);
                                break;
                            }
                            ptr = &(*ptr)->next;
                        }
                        
                        // TODO(long): This is very dumb. REWRITE THIS PLEASE!!!
                        Long_Highlight_Clear(app, view);
                        for_mc(node, mc_context.cursors)
                            Long_Highlight_Push(app, view, Ii64(node->cursor_pos, node->mark_pos));
                    }
                    else
                    {
                        MC_insert(app, pos, pos + match_size);
                        Long_Highlight_Push(app, view, Ii64_size(pos, match_size));
                    }
                }
                
                pos = new_pos;
            }
            
            if (pos_is_valid || bar->string.size == 0)
            {
                match_size = bar->string.size;
                isearch__update_highlight(app, view, Ii64_size(pos, match_size));
            }
        }
        
        old_size = bar->string.size;
        change_scan = 0;
        bar->prompt = scan == Scan_Forward ? isearch_str : rsearch_str;
        
        in = get_next_input(app, EventPropertyGroup_Any, EventProperty_Escape);
        if (in.abort)
        {
            view_set_cursor_and_preferred_x(app, view, seek_pos(first_pos));
            MC_end(app);
            break;
        }
        
        if (match_key_code(&in, KeyCode_Return))
        {
            if (!has_modifier(&in, KeyCode_Control))
            {
                if (mc_context.cursors)
                {
                    MC_begin(app);
                    view_set_mark(app, view, seek_pos(pos + match_size));
                }
                break;
            }
            
            Long_Query_AppendBar(bar, SCu8(previous_isearch_query));
        }
        
        else if (match_key_code(&in, KeyCode_PageDown) || match_key_code(&in, KeyCode_Down))
            change_scan = Scan_Forward;
        else if (match_key_code(&in, KeyCode_PageUp) || match_key_code(&in, KeyCode_Up))
            change_scan = Scan_Backward;
        
        else if (!Long_Query_DefaultInput(app, bar, view, &in))
        {
            Command_Metadata* metadata = Long_Mapping_GetMetadata(mapping, map, &in);
            Custom_Command_Function* custom = 0;
            
            if (metadata && (in.event.kind == InputEventKind_KeyStroke || in.event.kind == InputEventKind_KeyRelease))
            {
                custom = metadata->proc;
                if (custom == long_query_replace || custom == query_replace ||
                    custom == long_query_replace_identifier || custom == query_replace_identifier)
                    custom = Long_ISearch_Replace;
                else
                    custom = 0;
            }
            
            if (custom)
            {
                view_enqueue_command_function(app, view, custom);
                break;
            }
            leave_current_input_unhandled(app);
        }
    }
    
    if (bar->string.size)
        Long_Block_CopyStr(ExpandArray(previous_isearch_query), bar->string);
    Long_Camera_PopBounds(app, view, bounds);
    Long_Highlight_Clear(app, view);
}

// @COPYPASTA(long): F4_Search
function void Long_Search(Application_Links* app, Scan_Direction dir, b32 identifier, b32 insensitive)
{
    Scratch_Block scratch(app);
    View_ID view = get_active_view(app, Access_Read);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Read);
    
    if (buffer)
    {
        i64 cursor = view_get_cursor_pos(app, view);
        i64 mark = view_get_mark_pos(app, view);
        
        Range_i64 range = Ii64(cursor);
        if (identifier)
            range = enclose_pos_alpha_numeric_underscore(app, buffer, cursor);
        else if (fcoder_mode == FCoderMode_NotepadLike && cursor != mark)
            range = Ii64(cursor, mark);
        
        String8 query = push_buffer_range(app, scratch, buffer, range);
        Long_ISearch(app, dir, range.min, query, insensitive);
    }
}

CUSTOM_COMMAND_SIG(long_search)
CUSTOM_DOC("Begins an incremental search down through the current buffer for a user specified string.")
{
    Long_Search(app, Scan_Forward, 0, 1);
}

CUSTOM_COMMAND_SIG(long_reverse_search)
CUSTOM_DOC("Begins an incremental search up through the current buffer for a user specified string.")
{
    Long_Search(app, Scan_Backward, 0, 1);
}

CUSTOM_COMMAND_SIG(long_search_case_sensitive)
CUSTOM_DOC("Searches the current buffer forward for the exact string. If something is highlighted, will fill search query with it.")
{
    Long_Search(app, Scan_Forward, 0, 0);
}

CUSTOM_COMMAND_SIG(long_reverse_search_case_sensitive)
CUSTOM_DOC("Searches the current buffer backwards for the exact string. If something is highlighted, will fill search query with it.")
{
    Long_Search(app, Scan_Backward, 0, 0);
}

CUSTOM_COMMAND_SIG(long_search_identifier)
CUSTOM_DOC("Begins an incremental search down through the current buffer for the word or token under the cursor.")
{
    Long_Search(app, Scan_Forward, 1, 1);
}

CUSTOM_COMMAND_SIG(long_reverse_search_identifier)
CUSTOM_DOC("Begins an incremental search up through the current buffer for the word or token under the cursor.")
{
    Long_Search(app, Scan_Backward, 1, 1);
}

CUSTOM_COMMAND_SIG(long_search_identifier_case_sensitive)
CUSTOM_DOC("Begins an incremental search down through the current buffer for the exact word or token under the cursor.")
{
    Long_Search(app, Scan_Forward, 1, 0);
}

// @COPYPASTA(long): query_replace_parameter
function void Long_Query_Replace(Application_Links* app, String8 replace_str, i64 start_pos)
{
    if (!replace_str.size)
        return;
    
    Scratch_Block scratch(app);
    Query_Bar_Group group(app);
    
    String8 with_str = Long_Query_String(app, scratch, "With: ");
    if (!with_str.str)
        return;
    
    Long_Query_StartBar(app, scratch, "Replace? (enter), (Page)Up, (Page)Down, (esc)\n", {});
    
    i64 edit_size = with_str.size - replace_str.size;
    
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    Rect_f32 bounds = Long_Camera_PushBounds(app, view);
    
    Scan_Direction dir = Scan_Forward;
    Range_i64 range = buffer_range(app, buffer);
    Range_i64 match = Ii64(start_pos - 1);
    
    Mapping* mapping = 0;
    Command_Map* map = 0;
    
    User_Input in = {};
    for (b32 first_time = 1, replaced = 0; ; first_time = 0)
    {
        b32 do_scan_action = 1;
        
        if (match_key_code(&in, KeyCode_Return) || match_key_code(&in, KeyCode_Tab))
        {
            if (match.max > match.min)
            {
                buffer_replace_range(app, buffer, match, with_str);
                Long_Render_FadeHighlight(app, buffer, Ii64_size(match.min, with_str.size));
                
                if (match.min < start_pos)
                {
                    if (match.max < start_pos)
                        start_pos += edit_size;
                    else
                        start_pos = match.min;
                }
                
                range.max += edit_size;
                match.min += with_str.size;
                match.max = match.min;
                replaced = 1;
            }
            
            else
            {
                if (!replaced)
                    Long_Render_FadeError(app, buffer, Ii64_size(match.min, replace_str.size));
                break;
            }
        }
        
        else if (match_key_code(&in, KeyCode_PageDown) || match_key_code(&in, KeyCode_Down))
            dir = Scan_Forward;
        else if (match_key_code(&in, KeyCode_PageUp) || match_key_code(&in, KeyCode_Up))
            dir = Scan_Backward;
        
        else
        {
            do_scan_action = 0;
            if (match_key_code(&in, KeyCode_E) && has_modifier(&in, KeyCode_Control))
                center_view(app);
            else if (in.event.kind == InputEventKind_MouseWheel)
                Long_Mouse_ScrollWheel(app, view);
            else if (Long_Mapping_HandleCommand(app, view, &mapping, &map, &in))
                break;
        }
        
        if (do_scan_action || first_time)
        {
            i64 new_pos;
            seek_string(app, buffer, match.min, range.max, range.min, replace_str, &new_pos,
                        dir == Scan_Backward ? BufferSeekString_Backward : 0);
            
            if (range_contains(range, new_pos))
                match = Ii64_size(new_pos, replace_str.size);
            
            else if (first_time)
            {
                match.min += 1;
                seek_string_backward(app, buffer, match.min, 0, replace_str, &new_pos);
                
                if (range_contains(range, new_pos))
                    match = Ii64_size(new_pos, replace_str.size);
                else
                    match.max = match.min;
            }
            
            isearch__update_highlight(app, view, match);
        }
        
        in = get_next_input(app, EventPropertyGroup_Any, EventProperty_Escape);
        if (in.abort)
        {
            if (!has_modifier(&in, KeyCode_Shift))
                match.min = start_pos;
            break;
        }
    }
    
    view_set_cursor_and_preferred_x(app, view, seek_pos(match.min));
    Long_Camera_PopBounds(app, view, bounds);
}

CUSTOM_COMMAND_SIG(long_query_replace)
CUSTOM_DOC("Queries the user for two strings, and incrementally replaces every occurence of the first string with the second string.")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    if (buffer)
    {
        Scratch_Block scratch(app);
        Query_Bar_Group group(app);
        String8 replace = Long_Query_String(app, scratch, "Replace: ");
        Long_Query_Replace(app, replace, view_get_cursor_pos(app, view));
    }
}

function void Long_Query_Replace(Application_Links* app, Buffer_ID buffer, Range_i64 range)
{
    Scratch_Block scratch(app);
    String8 replace = push_buffer_range(app, scratch, buffer, range);
    Query_Bar* bar = Long_Query_StartBar(app, scratch, "Replace: ", replace);
    Long_Query_Replace(app, replace, range.min);
    end_query_bar(app, bar, 0);
}

CUSTOM_COMMAND_SIG(long_query_replace_identifier)
CUSTOM_DOC("Queries the user for a string, and incrementally replace every occurence of the word or token found at the cursor with the specified string.")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    if (buffer)
    {
        i64 pos = view_get_cursor_pos(app, view);
        Range_i64 range = enclose_pos_alpha_numeric_underscore(app, buffer, pos);
        Long_Query_Replace(app, buffer, range);
    }
}

CUSTOM_COMMAND_SIG(long_query_replace_selection)
CUSTOM_DOC("Queries the user for a string, and incrementally replace every occurence of the string found in the selected range with the specified string.")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    if (buffer)
        Long_Query_Replace(app, buffer, get_view_range(app, view));
}

function void Long_Query_ReplaceRange(Application_Links* app, View_ID view, Range_i64 range)
{
    Scratch_Block scratch(app);
    Query_Bar_Group group(app);
    
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    b32 readonly_buffer = buffer == 0;
    if (readonly_buffer)
        buffer = view_get_buffer(app, view, 0);
    
    b32 select_all = 0;
    String8 replace = {};
    
    Query_Bar* bar = Long_Query_ReserveBar(app, scratch, "Replace: ", {});
    if (bar)
    {
        Rect_f32 bounds = Long_Camera_PushBounds(app, view);
        Mapping* mapping = 0;
        Command_Map* map = 0;
        User_Input in = {};
        
        for (;;)
        {
            in = get_next_input(app, EventPropertyGroup_Any, EventProperty_Escape);
            if (in.abort)
                break;
            
            u64 old_size = bar->string.size;
            
            if (match_key_code(&in, KeyCode_Return))
            {
                if (has_modifier(&in, KeyCode_Shift))
                {
                    select_all = 1;
                    goto DONE;
                }
                
                if (!has_modifier(&in, KeyCode_Control))
                {
                    DONE:
                    replace = bar->string;
                    break;
                }
                
                Long_Query_AppendBar(bar, SCu8(previous_isearch_query));
            }
            
            else if (!Long_Query_DefaultInput(app, bar, view, &in))
            {
                if (Long_Mapping_HandleCommand(app, view, &mapping, &map, &in))
                    break;
            }
            
            if (old_size != bar->string.size)
            {
                i64 pos = range.min - 1;
                Long_Highlight_Clear(app, view);
                REPEAT:
                seek_string_forward(app, buffer, pos, range.end, bar->string, &pos);
                if (pos < range.end)
                {
                    Long_Highlight_Push(app, view, Ii64_size(pos, bar->string.size));
                    goto REPEAT;
                }
            }
        }
        
        Long_Camera_PopBounds(app, view, bounds);
        if (!replace.size)
        {
            Long_Highlight_Clear(app, view);
            return;
        }
    }
    
    String8 with = {};
    Long_Highlight_List* list = Long_Highlight_GetList(app, view);
    if (list->first)
    {
        if (readonly_buffer)
        {
            for (Long_Highlight_Node* node = list->first; node; node = node->next)
                Long_Render_FadeError(app, buffer, node->range);
        }
        
        else if (select_all)
        {
            for (Long_Highlight_Node* node = list->first->next; node; node = node->next)
                MC_insert(app, node->range.min, node->range.max);
            
            view_set_cursor_and_preferred_x(app, view, seek_pos(list->first->range.min));
            MC_begin(app);
            view_set_mark(app, view, seek_pos(list->first->range.max));
        }
        
        else with = Long_Query_String(app, scratch, "With: ");
        
        Long_Highlight_ClearList(list);
    }
    else Long_Render_FadeError(app, buffer, range);
    
    // @COPYPASTA(long): replace_in_range
    if (with.str)
    {
        History_Group history = history_group_begin(app, buffer);
        
        i64 pos = range.min - 1;
        i64 new_pos = 0;
        u32 match_count = 0;
        seek_string_forward(app, buffer, pos, range.end, replace, &new_pos);
        
        i64 shift = replace_range_shift(replace.size, with.size);
        while (new_pos + (i64)replace.size <= range.end)
        {
            buffer_replace_range(app, buffer, Ii64_size(new_pos, replace.size), with);
            Long_Render_FadeHighlight(app, buffer, Ii64_size(new_pos, with.size ? with.size : replace.size));
            match_count++;
            
            range.end += shift;
            pos = new_pos + (i32)with.size - 1;
            seek_string_forward(app, buffer, pos, range.end, replace, &new_pos);
        }
        
        String8 buffer_name = push_buffer_base_name(app, scratch, buffer);
        Long_Print_Messagef(app, "\n<%.*s>: Replaced '%.*s' in %u %s\n", string_expand(buffer_name),
                            string_expand(replace), match_count, match_count > 1 ? "matches" : "match");
        history_group_end(history);
    }
}

CUSTOM_COMMAND_SIG(long_replace_in_range)
CUSTOM_DOC("Queries the user for a needle and string. Replaces all occurences of needle with string in the range between cursor and the mark in the active buffer.")
{
    View_ID view = get_active_view(app, 0);
    Range_i64 range = get_view_range(app, view);
    Long_Query_ReplaceRange(app, view, range);
}

CUSTOM_COMMAND_SIG(long_replace_all_buffers)
CUSTOM_DOC("Queries the user for a needle and string. Replaces all occurences of needle with string in all editable buffers.")
{
    Scratch_Block scratch(app);
    Query_Bar_Group group(app);
    
    String8 replace_str = Long_Query_String(app, scratch, "Replace: ", {});
    if (!replace_str.size)
        return;
    
    String8 with_str = Long_Query_String(app, scratch, "With: ", {});
    if (!with_str.str)
        return;
    
    if (string_match(replace_str, with_str))
        return;
    
    global_history_edit_group_begin(app);
    String_Match_List matches = find_all_matches_all_buffers(app, scratch, replace_str, StringMatch_CaseSensitive, 0);
    string_match_list_filter_remove_buffer_predicate(app, &matches, buffer_has_name_with_star);
    
    Batch_Edit* head = 0;
    Batch_Edit* tail = 0;
    Buffer_ID buffer = 0;
    u32 buffer_count = 0;
    u32  match_count = 0;
    
    for (String_Match* match = matches.first; match; match = match->next)
    {
        if (buffer != match->buffer)
        {
            if (buffer)
            {
                buffer_batch_edit(app, buffer, head);
                head = tail = 0;
            }
            
            buffer = match->buffer;
            buffer_count++;
        }
        
        Batch_Edit* edit = push_array_zero(scratch, Batch_Edit, 1);
        edit->edit.text = with_str;
        edit->edit.range = match->range;
        sll_queue_push(head, tail, edit);
        match_count++;
    }
    
    if (tail)
        buffer_batch_edit(app, buffer, head);
    
    Long_Print_Messagef(app, "\n<long_replace_all_buffers>: Replaced '%.*s' in %u match(es) across %u file(s)\n",
                        string_expand(replace_str), match_count, buffer_count);
    global_history_edit_group_end(app);
}

//~ NOTE(long): Index Commands

//- NOTE(long): Code Peeking
CUSTOM_COMMAND_SIG(long_code_peek)
CUSTOM_DOC("Toggles code peek.")
{
    // @COPYPASTA(long): f4_code_peek
    long_global_code_peek_open ^= 1;
}

//- NOTE(long): Positional Context
CUSTOM_COMMAND_SIG(long_toggle_pos_context)
CUSTOM_DOC("Toggles position context window.")
{
    long_global_pos_context_open ^= 1;
}

CUSTOM_COMMAND_SIG(long_switch_pos_context_option)
CUSTOM_DOC("Switches the position context mode.")
{
    long_active_pos_context_option = (long_active_pos_context_option + 1) % ArrayCount(long_ctx_opts);
}

CUSTOM_COMMAND_SIG(long_switch_pos_context_draw_position)
CUSTOM_DOC("Switches between drawing the position context at cursor position or at bottom of buffer.")
{
    def_toggle_config_b32_lit("f4_poscontext_draw_at_bottom_of_buffer");
}

//- NOTE(long): Jumping
function i64 Long_Boundary_ANUD(Application_Links* app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos)
{
    return boundary_predicate(app, buffer, side, direction, pos, &long_predicate_alpha_numeric_underscore_dot);
}

function void Long_Jump_Definition(Application_Links* app, b32 handle_jump_location, b32 same_panel)
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
        i64 pos = view_get_cursor_pos(app, view);
        Token* token = get_token_from_pos(app, buffer, pos);
        
        if (token != 0 && token->size > 0)
        {
            if (token->kind == TokenBaseKind_Whitespace && pos == token->pos)
                token = get_token_from_pos(app, buffer, pos - 1);
            
            if (token->kind == TokenBaseKind_Comment || token->kind == TokenBaseKind_LiteralString)
            {
                Scratch_Block scratch(app);
                Range_i64 range = enclose_boundary(app, buffer, Ii64(pos), Long_Boundary_ANUD);
                if (range_size(range) == 0)
                    range = enclose_boundary(app, buffer, Ii64(pos-1), Long_Boundary_ANUD);
                
                if (range_size(range) != 0)
                {
                    String8 lexeme = push_buffer_range(app, scratch, buffer, range);
                    note = Long_Index_LookupNote(lexeme);
                    if (!note)
                    {
                        Buffer_ID dst_buffer = get_buffer_by_name(app, lexeme, 0);
                        if (dst_buffer)
                            Long_Jump_ToBuffer(app, view, dst_buffer);
                    }
                }
            }
            else if (token->kind != TokenBaseKind_Whitespace)
            {
                Token_Array array = get_token_array_from_buffer(app, buffer);
                note = Long_Index_LookupBestNote(app, buffer, &array, token);
            }
            
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
    // The difference between F4_JumpToLocation and F4_GoToDefinition is:
    // 1. The first one scroll to the target while the second one snap to the target instantly (scroll.position = scroll.target)
    // 2. The first one call view_set_preferred_x in set_view_to_location
    //    Preferred X is typically used for moving up/down from a larger line to a smaller line
    // For simplicity, I just use Long_PointStack_Jump (which calls F4_JumpToLocation) for all jumping behaviors
    // And if I want to snap the view directly, I just call Long_View_Snap
    if (note)
    {
        Long_PointStack_Jump(app, goto_view, note->file->buffer, note->range.min);
        if (!same_panel)
            Long_View_Snap(app, view);
    }
}

CUSTOM_COMMAND_SIG(long_go_to_definition)
CUSTOM_DOC("Goes to the jump location at the cursor or the definition of the identifier under the cursor.")
{
    Long_Jump_Definition(app, 1, 0);
}

CUSTOM_COMMAND_SIG(long_go_to_definition_same_panel)
CUSTOM_DOC("Goes to the jump location at the cursor or the definition of the identifier under the cursor in the same panel.")
{
    Long_Jump_Definition(app, 1, 1);
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
    Lister_Result l_result = Long_Lister_Run(app, lister);
    Long_Lister_Data result = {};
    if (Long_Lister_IsResultValid(l_result))
        block_copy_struct(&result, (Long_Lister_Data*)l_result.user_data);
    if (result.buffer != 0)
    {
        Long_PointStack_Jump(app, view, result.buffer, result.pos);
        Long_View_Snap(app, view);
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
function void Long_Note_PushString(Application_Links* app, Arena* arena, String8List* list, int* count,
                                   F4_Index_Note* note, NoteFilter* filter)
{
    if (*count > 3000)
        system_error_box("PushNoteString is overflowed");
    
    if (!((note->scope_range.min == note->scope_range.max) || (note->scope_range.min && note->scope_range.max)))
    {
        String8 string = push_stringf(arena, "Note: %.*s:%d (%d, %d) isn't initialized correctly!",
                                      string_expand(note->string), note->range.min, note->scope_range.min, note->scope_range.max);
        system_error_box((char*)string.str);
    }
    
    if (!filter || filter(note))
    {
        (*count)++;
        
        String8 string = note->string;
        u8 c = string_get_character(string, string.size-1);
        if (c == '\r' || c == '\n')
            string = string_chop(string, 1);
        
        String8 tag = Long_Note_PushTag(app, arena, note);
        String8 parentName = note->parent ? note->parent->string : S8Lit("NULL");
        string_list_pushf(arena, list, "[%.*s] %.*s { %.*s }",
                          string_expand(parentName), string_expand(string), string_expand(tag));
    }
    
    Long_Index_IterateValidNoteInFile(child, note)
        Long_Note_PushString(app, arena, list, count, child, filter);
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
    view_set_active(app, view);
}

function void Long_Note_DumpTree(Application_Links* app, String8 name, NoteFilter* filter)
{
    Buffer_ID buffer = view_get_buffer(app, get_active_view(app, Access_Always), Access_Always);
    Scratch_Block scratch(app);
    String8List list = {};
    
    F4_Index_Lock();
    {
        i32 count = 0;
        F4_Index_File* file = F4_Index_LookupFile(app, buffer);
        for (F4_Index_Note* note = file ? file->first_note : 0; note; note = note->next_sibling)
            Long_Note_PushString(app, scratch, &list, &count, note, filter);
        string_list_pushf(scratch, &list, "Note count: %d", count);
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
                String8 string = note->string;
                u8 c = string_get_character(string, string.size-1);
                if (c == '\r' || c == '\n')
                    string = string_chop(string, 1);
                
                String8 tag = Long_Note_PushTag(app, scratch, note);
                String8 parentName = note->parent ? note->parent->string : S8Lit("NULL");
                string_list_pushf(scratch, &list, "[%.*s] %.*s { %.*s }",
                                  string_expand(parentName), string_expand(string), string_expand(tag));
            }
            
            else
            {
                String8 string = push_token_lexeme(app, scratch, buffer, token);
                string_list_pushf(scratch, &list, "NULL: %.*s", string_expand(string));
            }
        }
        
        if (!token_it_inc_non_whitespace(&it))
            break;
    }
    
    string_list_pushf(scratch, &list, "Note count: %d", list.node_count);
    String8 string = string_list_flatten(scratch, list, 0, S8Lit("\n"), 0, 0);
    Long_Buffer_Write(app, scratch, S8Lit("identifiers.txt"), string);
}

//~ NOTE(long): Navigate Commands

//- NOTE(long): Cursor/Mark
CUSTOM_COMMAND_SIG(long_cursor_mark_swap)
CUSTOM_DOC("Swaps the position of the cursor and the mark.")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    i64 cursor = view_get_cursor_pos(app, view);
    i64 mark = view_get_mark_pos(app, view);
    view_set_cursor_and_preferred_x(app, view, seek_pos(mark));
    view_set_mark(app, view, seek_pos(cursor));
    no_mark_snap_to_cursor(app, view);
}

CUSTOM_COMMAND_SIG(long_copy)
CUSTOM_DOC("Copy the text in the range from the cursor to the mark onto the clipboard.")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    Range_i64 range = get_view_range(app, view);
    clipboard_post_buffer_range(app, 0, buffer, range);
    no_mark_snap_to_cursor(app, view);
}

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
        if (paste_index)
        {
            Scratch_Block scratch(app);
            *paste_index = 0;
            String8 string = push_clipboard_index(scratch, 0, *paste_index);
            
            if (string.size > 0)
            {
                Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
                Range_i64 range = get_view_range(app, view);
                
                buffer_replace_range(app, buffer, range, string);
                range = Ii64_size(range.min, string.size);
                
                view_set_cursor_and_preferred_x(app, view, seek_pos(range.max));
                buffer_post_fade(app, buffer, 0.667f, range, Long_ARGBFromID(defcolor_paste));
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

function void Long_Move_Line(Application_Links* app, Scan_Direction direction)
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    i64 pos = view_get_cursor_pos(app, view);
    i64 line = get_line_number_from_pos(app, buffer, pos);
    i64 offset = pos - get_line_side_pos(app, buffer, line, Side_Min);
    pos = move_line(app, buffer, line, direction) + offset;
    view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
}

CUSTOM_COMMAND_SIG(long_move_line_up)
CUSTOM_DOC("Swaps the line under the cursor with the line above it, and moves the cursor up with it.")
{
    Long_Move_Line(app, Scan_Backward);
}

CUSTOM_COMMAND_SIG(long_move_line_down)
CUSTOM_DOC("Swaps the line under the cursor with the line below it, and moves the cursor down with it.")
{
    Long_Move_Line(app, Scan_Forward);
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
    buffer_replace_range(app, buffer, range, S8Lit(""));
}

function void Long_Scan_Delete(Application_Links* app, Scan_Direction direction, Boundary_Function func)
{
    Long_Scan_Delete(app, Long_Scan_GetSide(direction), direction, func);
}

function i64 Long_Boundary_TokenAndWhitespace(Application_Links* app, Buffer_ID buffer, 
                                              Side side, Scan_Direction direction, i64 pos)
{
    /* NOTE(long): https://halt76.notion.site/On-word-boundaries-6f68d5d6dda34859ab2a9d54425c4667
    
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

CUSTOM_COMMAND_SIG(long_backspace_token)
CUSTOM_DOC("Deletes left to a token boundary.")
{
    Scratch_Block scratch(app);
    Long_Scan_Delete(app, Scan_Backward, Long_Boundary_TokenAndWhitespace);
}

CUSTOM_COMMAND_SIG(long_delete_token)
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

CUSTOM_COMMAND_SIG(long_move_next_alpha_numeric_or_camel)
CUSTOM_DOC("Seek right for boundary between alphanumeric characters or camel case word and non-alphanumeric characters.")
{
    Long_Scan_Move(app, Scan_Forward, Long_Boundary_AlphaNumericCamel);
}

CUSTOM_COMMAND_SIG(long_move_prev_alpha_numeric_or_camel)
CUSTOM_DOC("Seek left for boundary between alphanumeric characters or camel case word and non-alphanumeric characters.")
{
    Long_Scan_Move(app, Scan_Backward, Long_Boundary_AlphaNumericCamel);
}

CUSTOM_COMMAND_SIG(long_backspace_alpha_numeric_or_camel)
CUSTOM_DOC("Deletes left to a alphanumeric or camel boundary.")
{
    Scratch_Block scratch(app);
    Long_Scan_Delete(app, Scan_Backward, Long_Boundary_AlphaNumericCamel);
}

CUSTOM_COMMAND_SIG(long_delete_alpha_numeric_or_camel)
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
    Long_Scan_Move(app, Scan_Forward, Long_Boundary_FunctionAndType);
    Long_SnapMarkToCursor(app);
}

CUSTOM_COMMAND_SIG(long_move_to_prev_function_and_type)
CUSTOM_DOC("Seek left for the previous function or type in the buffer.")
{
    Long_Scan_Move(app, Scan_Backward, Long_Boundary_FunctionAndType);
    Long_SnapMarkToCursor(app);
}

// @COPYPASTA(long):
// https://github.com/Jack-Punter/4coder_punter/blob/0b43bad07998132e76d7094ed7ee385151a52ab7/4coder_fleury_base_commands.cpp#L651
function i64 Long_Boundary_CursorToken(Application_Links* app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos)
{
    Scratch_Block scratch(app);
    i64 result = pos;
    
    // NOTE(jack): Iterate the code index file for the buffer to find the search range for tokens
    // The search range_bounds should contain the function body, and its parameter list
    Code_Index_File* file = code_index_get_file(buffer);
    Range_i64 search_bounds = {};
    
    if (file) 
    {
        Code_Index_Nest_Ptr_Array file_nests = file->nest_array;
        Code_Index_Nest* prev_important_nest = 0;
        
        for (Code_Index_Nest* nest = file->nest_list.first; nest; nest = nest->next)
        {
            if (range_contains(Ii64(nest->open.start, nest->close.end), pos))
            {
                switch (nest->kind)
                {
                    case CodeIndexNest_Paren:
                    {
                        // NOTE(jack): Iterate to the next scope nest. We occasionally see CodeIndexNest_Preprocessor or 
                        // CodeIndexNest_Statement types between function parameter lists and the fucntion body
                        Code_Index_Nest* funciton_body_nest = nest->next;
                        while (funciton_body_nest && funciton_body_nest ->kind != CodeIndexNest_Scope)
                            funciton_body_nest = funciton_body_nest->next;
                        
                        search_bounds.start = nest->open.start;
                        search_bounds.end = (funciton_body_nest ? funciton_body_nest->close.end : nest->close.end);
                    } goto SCAN; 
                    
                    case CodeIndexNest_Scope:
                    {
                        search_bounds.start = (prev_important_nest ? prev_important_nest->open.start : nest->open.start);
                        search_bounds.end = nest->close.end;
                    } goto SCAN;
                    
                    //case CodeIndexNest_Preprocessor: break;
                }
            }
            
            // NOTE(jack): Keep track of the most recent Paren scope (parameter list) so that we can use it directly
            // if the cursor is within a function body.
            if (nest->kind == CodeIndexNest_Paren) 
                prev_important_nest = nest;
            else if (nest->kind != CodeIndexNest_Preprocessor)
                prev_important_nest = 0;
        }
    }
    
    SCAN:
    if (range_contains(search_bounds, pos))
    {
        View_ID view = get_active_view(app, Access_Always);
        i64 active_cursor_pos = view_get_cursor_pos(app, view);
        Token_Array tokens = get_token_array_from_buffer(app, buffer);
        Token* active_cursor_token = get_token_from_pos(app, &tokens, active_cursor_pos);
        
        // NOTE(jack): Loop to find the next cursor token occurance in the search_bounds. 
        // (only if we are on an identifier and have valid search bounds.
        if (active_cursor_token && active_cursor_token->kind == TokenBaseKind_Identifier)
        {
            String8 cursor_string = push_buffer_range(app, scratch, buffer, Ii64(active_cursor_token));
            Token_Iterator_Array it = token_iterator_pos(0, &tokens, pos);
            
            for (;;)
            {
                b32 out_of_file = 0;
                switch (direction)
                {
                    case Scan_Forward:  out_of_file = !token_it_inc_non_whitespace(&it); break;
                    case Scan_Backward: out_of_file = !token_it_dec_non_whitespace(&it); break;
                }
                
                if (out_of_file || !range_contains(search_bounds, token_it_read(&it)->pos))
                    break;
                
                Token* token = token_it_read(&it);
                if (token->kind == TokenBaseKind_Identifier) 
                {
                    String8 lexeme = push_buffer_range(app, scratch, buffer, Ii64(token));
                    if (string_match(cursor_string, lexeme))
                    {
                        result = token->pos + pos - active_cursor_token->pos;
                        break;
                    }
                }
            }
        }
    }
    
    return result;
}

CUSTOM_COMMAND_SIG(long_move_up_token_occurrence)
CUSTOM_DOC("Moves the cursor to the previous occurrence of the token that the cursor is over.")
{
    Long_Scan_Move(app, Scan_Backward, Long_Boundary_CursorToken);
}

CUSTOM_COMMAND_SIG(long_move_down_token_occurrence)
CUSTOM_DOC("Moves the cursor to the next occurrence of the token that the cursor is over.")
{
    Long_Scan_Move(app, Scan_Forward, Long_Boundary_CursorToken);
}

// @COPYPASTA(long): _F4_Boundary_DividerComment
function i64 Long_Boundary_DividerComment(Application_Links* app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos)
{
    Scratch_Block scratch(app);
    i64 result = pos;
    i64 current_line = get_line_number_from_pos(app, buffer, pos);
    
    String8 signifier = S8Lit("//~");
    b32 (*inc)(Token_Iterator_Array*) = token_it_inc_non_whitespace;
    if (direction == Scan_Backward)
        inc = token_it_dec_non_whitespace;
    
    Token_Array tokens = get_token_array_from_buffer(app, buffer);
    if (tokens.tokens != 0)
    {
        Token_Iterator_Array it = token_iterator_pos(0, &tokens, pos);
        while (inc(&it))
        {
            Token* token = token_it_read(&it);
            
            if (token->kind == TokenBaseKind_Comment)
            {
                String8 lexeme = push_buffer_range(app, scratch, buffer, Ii64(token));
                String8 substr = string_substring(lexeme, Ii64(0, signifier.size));
                i64 line = get_line_number_from_pos(app, buffer, token->pos);
                
                if (lexeme.size >= signifier.size && string_match(substr, signifier) && line != current_line)
                {
                    result = token->pos;
                    break;
                }
            }
        }
    }
    
    return result;
}

CUSTOM_COMMAND_SIG(long_move_to_next_divider_comment)
CUSTOM_DOC("Seek right for next divider comment in the buffer.")
{
    Long_Scan_Move(app, Scan_Forward, Long_Boundary_DividerComment);
    Long_SnapMarkToCursor(app);
}

CUSTOM_COMMAND_SIG(long_move_to_prev_divider_comment)
CUSTOM_DOC("Seek left for previous divider comment in the buffer.")
{
    Long_Scan_Move(app, Scan_Backward, Long_Boundary_DividerComment);
    Long_SnapMarkToCursor(app);
}

//- NOTE(long): Upper/Lower Case
function void Long_UpperLowerRange(Application_Links* app, b32 upper, b32 lower)
{
    Scratch_Block scratch(app);
    View_ID view = get_active_view(app, 0);
    Range_i64 range = get_view_range(app, view);
    Buffer_ID buffer = view_get_buffer(app, view, 0);
    
    b32 modify = 0;
    String8 str = push_buffer_range(app, scratch, buffer, range);
    for (u64 i = 0; i < str.size; ++i)
    {
        u8 c = str.str[i];
        
        if (c < 'A' || c > 'z')
            continue;
        else if (upper && c >= 'a')
            c -= 'a' - 'A';
        else if (lower && c <= 'Z')
            c += 'a' - 'A';
        
        modify = str.str[i] != c;
        str.str[i] = c;
    }
    
    if (modify)
    {
        // NOTE(long): The default to_upper/lowercase command reset the cursor and mark position
        // This remembers which one came first
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

//~ NOTE(long): Build Commands

// @COPYPASTA(long): standard_search_and_build_from_dir
function b32 Long_Build_FromDir(Application_Links* app, View_ID view, String8 start_dir)
{
    Scratch_Block scratch(app);
    
    // NOTE(allen): Search
    String8 cmd_string = {};
    String8  full_path = {};
    if (start_dir.size)
    {
        for (i32 i = 0; i < ArrayCount(standard_build_file_name_array) && !cmd_string.size; ++i)
        {
            full_path = push_file_search_up_path(app, scratch, start_dir, standard_build_file_name_array[i]);
            if (full_path.size)
                cmd_string = standard_build_cmd_string_array[i];
        }
    }
    
    b32 result = full_path.size > 0;
    if (result)
    {
        // NOTE(allen): Build
        b32 auto_save = def_get_config_b32_lit("automatically_save_changes_on_build");
        if (auto_save)
            save_all_dirty_buffers(app);
        
        String8 path = string_remove_last_folder(full_path);
        String8 command = push_u8_stringf(scratch, "\"%.*s/%.*s\"", string_expand(path), string_expand(cmd_string));
        standard_build_exec_command(app, view, path, command);
        
        Buffer_ID buffer = get_buffer_by_name(app, S8Lit("*messages*"), 0);
        i64 line_count = buffer_get_line_count(app, buffer);
        String8 last_line = push_buffer_line(app, scratch, buffer, line_count-1);
        if (!StrStartsWith(last_line, S8Lit("Building with:")))
            print_message(app, S8Lit("\n"));
        
        string_mod_replace_character(full_path, '\\', '/');
        Long_Print_Messagef(app, "Building with: %.*s\n", string_expand(full_path));
    }
    
    return result;
}

// @COPYPASTA(long): build_in_build_panel
CUSTOM_COMMAND_SIG(long_build_in_build_panel)
CUSTOM_DOC("Looks for a build.bat, build.sh, or makefile in the current and parent directories. Runs the first that it finds and prints the output to *compilation*. Puts the *compilation* buffer in a panel at the footer of the current view.")
{
    Scratch_Block scratch(app);
    View_ID build_view = get_or_open_build_panel(app);
    View_ID view = get_active_view(app, Access_Always);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    
    // @COPYPASTA(long): standard_search_and_build
    {
        String8 build_dir = push_build_directory_at_file(app, scratch, buffer);
        String8   hot_dir = push_hot_directory(app, scratch);
        
        b32 did_build = 0;
        if (!did_build)
            did_build = Long_Build_FromDir(app, build_view, build_dir);
        if (!did_build)
            did_build = Long_Build_FromDir(app, build_view, hot_dir);
        if (!did_build)
            standard_build_exec_command(app, build_view, hot_dir, push_fallback_command(scratch));
    }
    
    set_fancy_compilation_buffer_font(app);
    block_zero_struct(&prev_location);
    lock_jump_buffer(app, S8Lit("*compilation*"));
}

//~ NOTE(long): Project Commands

function String8 Long_Prj_RelBufferName(Application_Links* app, Arena* arena, Buffer_ID buffer)
{
    Scratch_Block scratch(app, arena);
    String8 result = {};
    Variable_Handle prj_var = vars_read_root_key_lit("prj_config");
    
    String8 filepath = push_buffer_file_name(app, scratch, buffer);
    {
        String8 buffer_name = push_buffer_base_name(app, scratch, buffer);
        filepath = string_chop(filepath, buffer_name.size);
        if (string_get_character(filepath, filepath.size - 1) == '\\')
            filepath.size--;
    }
    
    if (filepath.size)
    {
        Variable_Handle reference_path_var = vars_read_key_lit(prj_var, "reference_paths");
        i32 i = 0;
        b32 multi_paths = !vars_is_nil(vars_next_sibling(vars_first_child(reference_path_var)));
        
        for (Vars_Children(path_var, reference_path_var), ++i)
        {
            String8 ref_dir = vars_string_from_var(scratch, path_var);
            if (ref_dir.size)
            {
                ref_dir = string_mod_replace_character(ref_dir, '/', '\\');
                if (ref_dir.str[ref_dir.size - 1] == '\\')
                    ref_dir = push_stringf(scratch, "%.*s\\", string_expand(ref_dir));
                
                if (StrStartsWith(filepath, ref_dir))
                {
                    String8 relpath = string_skip(filepath, ref_dir.size);
                    String8 path_index = multi_paths ? push_stringf(scratch, ":%d", i) : String8{};
                    result = push_stringf(arena, "REFPATH%.*s%.*s", string_expand(path_index), string_expand(relpath));
                    break;
                }
            }
        }
        
        if (!result.size)
        {
            String8 prj_dir = prj_path_from_project(scratch, prj_var);
            // NOTE(long): prj_dir always has a slash at the end
            prj_dir = string_mod_replace_character(prj_dir, '/', '\\');
            
            if (StrStartsWith(filepath, prj_dir))
            {
                String8 relpath = string_skip(filepath, prj_dir.size);
                result = push_string_copy(arena, relpath);
            }
        }
    }
    
    return result;
}

function void Long_Prj_Print(Application_Links* app, Variable_Handle prj_var)
{
    Scratch_Block scratch(app);
    
    String8 filename = prj_full_file_path_from_project(scratch, prj_var);
    String8 prj_name = vars_str_from_key_lit(scratch, prj_var, "project_name");
    Long_Print_Messagef(app, "\nProject: %.*s, Name: %.*s\n", string_expand(filename), string_expand(prj_name));
    
    Variable_Handle whitelist = vars_read_key_lit(prj_var, "patterns");
    String8 whitelist_msg = Long_Vars_SingleStrArray(app, scratch, whitelist);
    Long_Print_Messagef(app, "  Whitelist: %.*s\n", string_expand(whitelist_msg));
    
    Variable_Handle blacklist = vars_read_key_lit(prj_var, "blacklist_patterns");
    String8 blacklist_msg = Long_Vars_SingleStrArray(app, scratch, blacklist);
    Long_Print_Messagef(app, "  Blacklist: %.*s\n", string_expand(blacklist_msg));
    
    Long_Print_Messagef(app, "  Commands:\n");
    Variable_Handle commands = vars_read_key_lit(prj_var, "commands");
    String_ID out_id = vars_save_string_lit("out");
    String_ID footer_id = vars_save_string_lit("footer_panel");
    String_ID save_id = vars_save_string_lit("save_dirty_files");
    
    for (Vars_Children(command_var, commands))
    {
        String8 name = vars_key_from_var(scratch, command_var);
        String8 out = vars_str_from_key(scratch, command_var, out_id);
        b32 footer = vars_b32_from_key(command_var, footer_id);
        b32 save = vars_b32_from_key(command_var, save_id);
        Long_Print_Messagef(app, "    %.*s: \"%.*s\"%s%s\n", string_expand(name), string_expand(out),
                            footer ? ", footer panel" : "", save ? ", save dirty files" : "");
        
        String8List ignore = {};
        string_list_push_u8_lit(scratch, &ignore, "footer_panel");
        string_list_push_u8_lit(scratch, &ignore, "save_dirty_files");
        string_list_push_u8_lit(scratch, &ignore, "out");
        String8 os_commands = Long_Vars_StrFromArray(app, scratch, command_var, 6, ignore);
        print_message(app, os_commands);
        print_message(app, S8Lit("\n"));
    }
    
    Variable_Handle fkeys = vars_read_key_lit(prj_var, "fkey_command");
    String8 fkey_msg = Long_Vars_StrFromArray(app, scratch, fkeys, 4);
    Long_Print_Messagef(app, "  Fkeys:\n%.*s\n", string_expand(fkey_msg));
    
    Variable_Handle refpaths = vars_read_key_lit(prj_var, "reference_paths");
    String8 refpath_msg = Long_Vars_StrFromArray(app, scratch, refpaths, 4);
    if (refpath_msg.size)
        Long_Print_Messagef(app, "  Reference:\n%.*s\n", string_expand(refpath_msg));
    else
        Long_Print_Messagef(app, "  Referece: None\n");
}

// @COPYPASTA(long): load_project
function Variable_Handle Long_Prj_Parse(Application_Links* app, String8 filename, String8 data)
{
    Scratch_Block scratch(app);
    Variable_Handle prj_var = vars_get_nil();
    
    if (!data.size)
    {
        print_message(app, S8Lit("Did not find project.4coder.\n"));
        return prj_var;
    }
    
    Config* config_parse = def_config_from_text(app, scratch, filename, data);
    if (config_parse)
    {
        i32 version = config_parse->version ? *config_parse->version : 0;
        switch (version)
        {
            case 0:
            case 1:
            {
                String8 project_root = string_remove_last_folder(filename);
                prj_var = prj_v1_to_v2(app, project_root, config_parse);
            } break;
            
            default:
            {
                prj_var = def_fill_var_from_config(app, vars_get_root(),
                                                   vars_save_string_lit("prj_config"), config_parse);
            } break;
        }
        
        String8 error_text = config_stringize_errors(app, scratch, config_parse);
        if (error_text.size > 0)
            Long_Print_Error(app, error_text);
        else if (!vars_is_nil(prj_var))
            Long_Prj_Print(app, prj_var);
    }
    
    return prj_var;
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

function void Long_Prj_Load(Application_Links* app, Variable_Handle prj_var)
{
    Scratch_Block scratch(app);
    
    // NOTE(allen): Open All Project Files
    Variable_Handle load_paths_var = vars_read_key_lit(prj_var, "load_paths");
    Variable_Handle load_paths_os_var = vars_read_key_lit(load_paths_var, OS_NAME);
    
    Variable_Handle whitelist_var = vars_read_key_lit(prj_var, "patterns");
    Variable_Handle blacklist_var = vars_read_key_lit(prj_var, "blacklist_patterns");
    
    Prj_Pattern_List whitelist = prj_pattern_list_from_var(scratch, whitelist_var);
    Prj_Pattern_List blacklist = prj_pattern_list_from_var(scratch, blacklist_var);
    
    String_ID path_id = vars_save_string_lit("path");
    String_ID recursive_id = vars_save_string_lit("recursive");
    String_ID relative_id = vars_save_string_lit("relative");
    
    for (Vars_Children(load_path_var, load_paths_os_var))
    {
        String8 path = vars_str_from_key(scratch, load_path_var, path_id);
        b32 recursive = vars_b32_from_key(load_path_var, recursive_id);
        b32  relative = vars_b32_from_key(load_path_var, relative_id);
        
        String8 file_dir = path;
        if (relative)
        {
            String8List file_dir_list = {};
            String8 prj_dir = prj_path_from_project(scratch, prj_var);
            string_list_push(scratch, &file_dir_list, prj_dir);
            
            string_list_push_overlap(scratch, &file_dir_list, '/', path);
            string_list_push_overlap(scratch, &file_dir_list, '/', SCu8());
            file_dir = string_list_flatten(scratch, file_dir_list, StringFill_NullTerminate);
        }
        
        Prj_Open_File_Flags flags = recursive ? PrjOpenFileFlag_Recursive : 0;
        prj_open_files_pattern_filter(app, file_dir, whitelist, blacklist, flags);
    }
    
    // NOTE(long): Open All Reference Files (This must use the v2 project file format)
    Variable_Handle reference_path_var = vars_read_key_lit(prj_var, "reference_paths");
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
        
        else Long_Print_Messagef(app, "Error: Path \"%.*s\" doesn't exists\n", string_expand(path));
    }
    
    // NOTE(allen): Set Window Title
    String8 proj_name = vars_str_from_key_lit(scratch, prj_var, "project_name");
    String8 title = push_u8_stringf(scratch, "4coder project: %.*s", string_expand(proj_name));
    set_window_title(app, title);
}

CUSTOM_COMMAND_SIG(long_load_project)
CUSTOM_DOC("Looks for a project.4coder file in the current directory and tries to load it.  Looks in parent directories until a project file is found or there are no more parents.")
{
    ProfileScope(app, "[Long] Load Project");
    save_all_dirty_buffers(app);
    Scratch_Block scratch(app);
    
    String8 project_path = push_hot_directory(app, scratch);
    File_Name_Data dump = dump_file_search_up_path(app, scratch, project_path, S8Lit("project.4coder"));
    Variable_Handle prj_var = Long_Prj_Parse(app, dump.file_name, dump.data);
    Long_Prj_Load(app, prj_var);
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
        {
            String8 replace = S8Lit("F1 = \"run\";\n.F2 = \"run\";");
            String8 with = S8Lit("F1 = \"build\",\n.F2 = \"run\",");
            buffer_replace_range(app, buffer, Ii64_size(pos, replace.size), with);
        }
    }
    
    load_project(app);
}

CUSTOM_COMMAND_SIG(long_reload_all_themes_default_folder)
CUSTOM_DOC("Clears and reloads all the theme files in the default theme folder.")
{
    Scratch_Block scratch(app);
    String8 current_name = S8Lit("4coder");
    for (Color_Table_Node* node = global_theme_list.first; node != 0; node = node->next)
    {
        if (node->table.arrays == active_color_table.arrays)
        {
            current_name = push_string_copy(scratch, node->name);
            break;
        }
    }
    
    clear_all_themes(app);
    load_themes_default_folder(app);
    
    for (Color_Table_Node* node = global_theme_list.first; node != 0; node = node->next)
        if (string_match(node->name, current_name))
            active_color_table = node->table;
}

function String8 Long_Config_StrFromB32Vars(Application_Links* app, Arena* arena, String8List* list)
{
    Scratch_Block scratch(app, arena);
    String8List true_list = {};
    
    for (String8Node* node = list->first; node; node = node->next)
    {
        b32 var = def_get_config_b32(vars_save_string(node->string));
        if (var)
            string_list_push(scratch, &true_list, node->string);
    }
    
    String8 result = StrListJoin(arena, true_list, S8Lit(", "), 0);
    return result;
}

function void Long_Config_Print(Application_Links* app)
{
    Scratch_Block scratch(app);
    String8List var_list = {};
    Long_Print_Messagef(app, "\nDefault Config: config.4coder\n");
    
    //- NOTE(long): User
    b32 bind_by_physical_key = def_get_config_b32_lit("bind_by_physical_key");
    b32 lalt_lctrl_is_altgr = def_get_config_b32_lit("lalt_lctrl_is_altgr");
    
    String8 user_name = def_get_config_str_lit(scratch, "user_name");
    String8 mode = def_get_config_str_lit(scratch, "mode");
    
    String8 mapping = def_get_config_str_lit(scratch, "mapping");
    if (!mapping.size)
        mapping = OS_MAC ? S8Lit("mac-bindings.4coder") : S8Lit("bindings.4coder");
    
    Long_Print_Messagef(app, "  User: %.*s, Mode: %.*s, Mapping: %.*s, Keymode: %s%s\n",
                        string_expand(user_name), string_expand(mode), string_expand(mapping),
                        bind_by_physical_key ? "Physical" : "Language",
                        lalt_lctrl_is_altgr ? ", LAlt+LCtrl is AltGr" : "");
    string_list_push_u8_lit(scratch, &var_list, "user_name");
    string_list_push_u8_lit(scratch, &var_list, "mode");
    string_list_push_u8_lit(scratch, &var_list, "mapping");
    string_list_push_u8_lit(scratch, &var_list, "bind_by_physical_key");
    string_list_push_u8_lit(scratch, &var_list, "lalt_lctrl_is_altgr");
    
    Long_Print_Messagef(app, "  UI:\n");
    
    //- NOTE(long): UI Flags
    String8List allow_list = {};
    string_list_push_u8_lit(scratch, &allow_list, "use_scroll_bars");
    string_list_push_u8_lit(scratch, &allow_list, "use_file_bars");
    string_list_push_u8_lit(scratch, &allow_list, "use_comment_keywords");
    string_list_push_u8_lit(scratch, &allow_list, "use_paren_helper");
    string_list_push_u8_lit(scratch, &allow_list, "use_error_highlight");
    string_list_push_u8_lit(scratch, &allow_list, "use_jump_highlight");
    string_list_push_u8_lit(scratch, &allow_list, "use_scope_highlight");
    string_list_push_u8_lit(scratch, &allow_list, "highlight_line_at_cursor");
    string_list_push_u8_lit(scratch, &allow_list, "enable_undo_fade_out");
    
    String8 allow_str = Long_Config_StrFromB32Vars(app, scratch, &allow_list);
    Long_Print_Messagef(app, "    Allow: %s\n", !allow_str.size ? "none" : (char*)allow_str.str);
    string_list_push(&var_list, &allow_list);
    
    //- NOTE(long): UI Layout
    String8List layout_list = {};
    string_list_push_u8_lit(scratch, &layout_list, "show_line_number_margins");
    string_list_push_u8_lit(scratch, &layout_list, "long_show_line_number_offset");
    string_list_push_u8_lit(scratch, &layout_list, "enable_output_wrapping");
    
    String8 layout_str = Long_Config_StrFromB32Vars(app, scratch, &layout_list);
    Long_Print_Messagef(app, "    Layout: %s\n", !layout_str.size ? "none" : (char*)layout_str.str);
    string_list_push(&var_list, &layout_list);
    
    //- NOTE(long): Cursor/Mark
    u64 cursor_roundness = def_get_config_u64_lit(app, "cursor_roundness");
    u64 mark_thickness = def_get_config_u64_lit(app, "mark_thickness");
    u64 lister_roundness = def_get_config_u64_lit(app, "lister_roundness");
    Long_Print_Messagef(app, "  Cursor Roundness: %llu, Mark Thickness: %llu, Lister Roundness: %llu\n",
                        cursor_roundness, mark_thickness, lister_roundness);
    
    string_list_push_u8_lit(scratch, &var_list, "cursor_roundness");
    string_list_push_u8_lit(scratch, &var_list, "mark_thickness");
    string_list_push_u8_lit(scratch, &var_list, "lister_roundness");
    
    Long_Print_Messagef(app, "  Code:\n");
    
    //- NOTE(long): Extensions
    String8 treat_as_code = def_get_config_str_lit(scratch, "treat_as_code");
    Long_Print_Messagef(app, "    Extensions: \"%.*s\"\n", string_expand(treat_as_code));
    string_list_push_u8_lit(scratch, &var_list, "treat_as_code");
    
    //- NOTE(long): Virtual Whitespace
    b32 enable_vw = def_get_config_b32_lit("enable_virtual_whitespace");
    u64 vw_indent = def_get_config_u64_lit(app, "virtual_whitespace_regular_indent");
    b32 wrap_lines = def_get_config_b32_lit("enable_code_wrapping");
    Long_Print_Messagef(app, "    Virtual Whitespace: %s, Indent: %llu%s\n",
                        enable_vw ? "Enable" : "Disable", vw_indent, wrap_lines ? ", Code Wrapping" : "");
    
    string_list_push_u8_lit(scratch, &var_list, "enable_virtual_whitespace");
    string_list_push_u8_lit(scratch, &var_list, "virtual_whitespace_regular_indent");
    string_list_push_u8_lit(scratch, &var_list, "enable_code_wrapping");
    
    //- NOTE(long): Indent
    b32 tab_indent = def_get_config_b32_lit("indent_with_tabs");
    u64 indent_width = def_get_config_u64_lit(app, "indent_width");
    u64 tab_width = def_get_config_u64_lit(app, "default_tab_width");
    Long_Print_Messagef(app, "    Indent with: %s, Indent Width: %llu, Tab Width: %llu\n",
                        tab_indent ? "tabs" : "spaces", indent_width, tab_width);
    
    string_list_push_u8_lit(scratch, &var_list, "indent_with_tabs");
    string_list_push_u8_lit(scratch, &var_list, "indent_width");
    string_list_push_u8_lit(scratch, &var_list, "default_tab_width");
    
    //- NOTE(long): Automatic
    String8List auto_list = {};
    string_list_push_u8_lit(scratch, &auto_list, "automatically_indent_text_on_save");
    string_list_push_u8_lit(scratch, &auto_list, "automatically_save_changes_on_build");
    string_list_push_u8_lit(scratch, &auto_list, "automatically_load_project");
    
    String8 auto_str = Long_Config_StrFromB32Vars(app, scratch, &auto_list);
    Long_Print_Messagef(app, "  Auto: %s\n", !auto_str.size ? "none" : (char*)auto_str.str);
    string_list_push(&var_list, &auto_list);
    
    //- NOTE(long): Theme
    String8 theme = def_get_config_str_lit(scratch, "default_theme_name");
    Long_Print_Messagef(app, "  Default Theme: %.*s\n", string_expand(theme));
    string_list_push_u8_lit(scratch, &var_list, "default_theme_name");
    
    //- NOTE(long): Font
    u32 default_font_size = (u32)def_get_config_u64_lit(app, "default_font_size");
    b32 default_font_hinting = def_get_config_b32_lit("default_font_hinting");
    String8 default_font_name = def_get_config_str_lit(scratch, "default_font_name");
    String8 aa_mode = def_get_config_str_lit(scratch, "default_font_aa_mode");
    if (!string_match(aa_mode, S8Lit("1bit")))
        aa_mode = S8Lit("8bit");
    
    Long_Print_Messagef(app, "  Default Font: \"%.*s\", Size: %u, %sMode: %.*s\n",
                        string_expand(default_font_name), default_font_size,
                        default_font_hinting ? "Hinting, " : "", string_expand(aa_mode));
    
    string_list_push_u8_lit(scratch, &var_list, "default_font_name");
    string_list_push_u8_lit(scratch, &var_list, "default_font_size");
    string_list_push_u8_lit(scratch, &var_list, "default_font_hinting");
    string_list_push_u8_lit(scratch, &var_list, "default_font_aa_mode");
    
    //- NOTE(long): Compilers
    Long_Print_Messagef(app, "  Default Compilers:\n");
    
    String8 flags_bat = def_get_config_str_lit(scratch, "default_flags_bat");
    String8 compiler_bat = def_get_config_str_lit(scratch, "default_compiler_bat");
    Long_Print_Messagef(app, "    bat: \"%.*s\", flags: \"%.*s\"\n",
                        string_expand(compiler_bat), string_expand(flags_bat));
    
    String8 flags_sh = def_get_config_str_lit(scratch, "default_flags_sh");
    String8 compiler_sh = def_get_config_str_lit(scratch, "default_compiler_sh");
    Long_Print_Messagef(app, "    sh: \"%.*s\", flags: \"%.*s\"\n",
                        string_expand(compiler_sh), string_expand(flags_sh));
    
    string_list_push_u8_lit(scratch, &var_list, "default_flags_bat");
    string_list_push_u8_lit(scratch, &var_list, "default_compiler_bat");
    string_list_push_u8_lit(scratch, &var_list, "default_flags_sh");
    string_list_push_u8_lit(scratch, &var_list, "default_compiler_sh");
    
    //- NOTE(long): Misc
    Long_Print_Messagef(app, "  Misc:\n");
    Variable_Handle def_cfg = vars_read_root_key_lit("def_config");
    String8 str = Long_Vars_StrFromArray(app, scratch, def_cfg, 4, var_list);
    Long_Print_Messagef(app, "%.*s\n", string_expand(str));
}

// @COPYPASTA(long): load_config_and_apply
function void Long_Config_ApplyFromData(Application_Links* app, String8 data, i32 override_font_size, b32 override_hinting)
{
    Scratch_Block scratch(app);
    
    Variable_Handle cfg_var = {};
    if (data.size)
    {
        Config* parsed = def_config_from_text(app, scratch, S8Lit("config.4coder"), data);
        String8 error_str = config_stringize_errors(app, scratch, parsed);
        
        if (!error_str.size)
            cfg_var = def_fill_var_from_config(app, vars_get_root(), vars_save_string_lit("def_config"), parsed);
        else
            Long_Print_Errorf(app, "trying to load config file:\n%.*s", string_expand(error_str));
    }
    
    else
    {
        Long_Print_Errorf(app, "Invalid config file, using the default config instead\n");
        Face_Description description = get_global_face_description(app);
        String8 filename = description.font.file_name;
        if (filename.size)
            def_set_config_string(vars_save_string_lit("default_font_name"), filename);
    }
    
    // Apply config
    String8 mode = def_get_config_str_lit(scratch, "mode");
    change_mode(app, mode);
    
    b32 lalt_lctrl_is_altgr = def_get_config_b32_lit("lalt_lctrl_is_altgr");
    global_set_setting(app, GlobalSetting_LAltLCtrlIsAltGr, lalt_lctrl_is_altgr);
    
    String8 theme = def_get_config_str_lit(scratch, "default_theme_name");
    Color_Table* colors = get_color_table_by_name(theme);
    set_active_color(colors);
    
    Face_Description description = {};
    u32 default_font_size = override_font_size;
    if (!default_font_size)
        default_font_size = (u32)def_get_config_u64_lit(app, "default_font_size");
    if (!default_font_size)
        default_font_size = 12;
    description.parameters.pt_size = default_font_size;
    
    b32 default_font_hinting = def_get_config_b32_lit("default_font_hinting");
    description.parameters.hinting = default_font_hinting || override_hinting;
    
    String8 aa_mode = def_get_config_str_lit(scratch, "default_font_aa_mode");
    description.parameters.aa_mode = FaceAntialiasingMode_8BitMono;
    if (string_match(aa_mode, S8Lit("1bit")))
        description.parameters.aa_mode = FaceAntialiasingMode_1BitMono;
    else
        aa_mode = S8Lit("8bit");
    
    String8 fallback_font_name = S8Lit("liberation-mono.ttf");
    String8  default_font_name = def_get_config_str_lit(scratch, "default_font_name");
    if (!default_font_name.size)
        default_font_name = fallback_font_name;
    
    REPEAT:
    description.font.file_name = default_font_name;
    if (!modify_global_face_by_description(app, description))
    {
        // @COPYPASTA(long): def_search_normal_full_path
        String8List list = {};
        def_search_normal_load_list(scratch, &list);
        for (String8Node* node = list.first; node; node = node->next)
        {
            if (node->string.size)
            {
                string_mod_replace_character(node->string, '\\', '/');
                if (node->string.str[node->string.size-1] == '/')
                    node->string.size -= 1;
            }
        }
        
        String8 filepath = push_u8_stringf(scratch, "fonts/%.*s", string_expand(default_font_name));
        String8 filename = def_search_get_full_path(scratch, &list, filepath);
        
        description.font.file_name = filename;
        if (!modify_global_face_by_description(app, description))
        {
            if (!string_match(default_font_name, fallback_font_name))
            {
                Long_Print_Errorf(app, "Failed to load the font: %.*s\n\n", string_expand(default_font_name));
                default_font_name = fallback_font_name;
                goto REPEAT;
            }
        }
    }
    
    b32 bind_by_physical_key = def_get_config_b32_lit("bind_by_physical_key");
    system_set_key_mode(bind_by_physical_key ? KeyMode_Physical : KeyMode_LanguageArranged);
    
    Long_Config_Print(app);
}

//~ NOTE(long): Command-related Commands

// @COPYPASTA(long): get_command_from_user
function Custom_Command_Function* Long_Lister_GetCommand(Application_Links* app, String8 query, Command_Lister_Status_Rule* rule)
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
                string_list_push_u8_lit(scratch, &list, " ");
        }
        
        String8 name = SCu8(fcoder_metacmd_table[j].name, fcoder_metacmd_table[j].name_len);
        String8 status = string_list_flatten(scratch, list);
        // NOTE(long): fcoder_metacmd_table[j].description_len isn't escaped correctly
        // (e.g "\n" is 2 bytes rather than 1) but is probably null-terminated.
        String8 tooltip = SCu8(fcoder_metacmd_table[j].description);
        Long_Lister_AddItem(app, lister, name, status, 0, 0, PtrAsInt(proc), tooltip);
    }
    
    Lister_Result l_result = Long_Lister_Run(app, lister);
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
        
        Custom_Command_Function* func = Long_Lister_GetCommand(app, S8Lit("Command:"), &rule);
        if (func)
            view_enqueue_command_function(app, view, func);
    }
}

// @COPYPASTA(long): execute_any_cli
CUSTOM_COMMAND_SIG(long_execute_any_cli)
CUSTOM_DOC("Queries for an output buffer name and system command, runs the system command as a CLI and prints the output to the specified buffer.")
{
    Scratch_Block scratch(app);
    Query_Bar_Group group(app);
    
    String8 out = Long_Query_StringEx(app, scratch, "Output Buffer: ", ExpandArray(out_buffer_space) - 1, S8Lit("*run*"));
    if (!out.size) return;
    out_buffer_space[out.size] = 0;
    
    String8 cmd = Long_Query_StringEx(app, scratch, "Command: ", ExpandArray(command_space) - 1, {});
    if (!cmd.size) return;
    command_space[cmd.size] = 0;
    
    String8 hot = push_hot_directory(app, scratch);
    Long_Block_CopyStr(ExpandArray(hot_directory_space), hot);
    
    execute_previous_cli(app);
}

CUSTOM_COMMAND_SIG(long_explorer)
CUSTOM_DOC("Opens file explorer in cmd")
{
    Scratch_Block scratch(app);
    String8 hot = push_hot_directory(app, scratch);
    exec_system_command(app, 0, buffer_identifier(0), hot, S8Lit("explorer ."), 0);
}

//~ NOTE(long): Theme Commands

global Color_Table long_active_color_table;

function void Long_Lister_NavTheme(Application_Links* app, View_ID view, Lister* lister, i32 delta)
{
    Long_Lister_Navigate_NoSelect(app, view, lister, delta);
    Lister_Node* node = lister->highlighted_node;
    if (node)
        active_color_table = *(Color_Table*)node->user_data;
    else
        active_color_table = long_active_color_table;
}

function Color_Table* Long_Lister_ColorTable(Application_Links* app, String8 query)
{
    Scratch_Block scratch(app);
    Lister_Block lister(app, scratch);
    lister_set_query(lister, query);
    
    Lister_Handlers handlers = lister_get_default_handlers();
    handlers.navigate = Long_Lister_NavTheme;
    lister_set_handlers(lister, &handlers);
    
    for (Color_Table_Node* node = global_theme_list.first; node; node = node->next)
    {
        String8 tag = long_active_color_table.arrays == node->table.arrays ? S8Lit("current") : S8Lit("");
        lister_add_item(lister, node->name, tag, (void*)&node->table, 0);
    }
    
    Lister_Result l_result = Long_Lister_Run(app, lister, 0);
    return (Color_Table*)l_result.user_data;
}

CUSTOM_UI_COMMAND_SIG(long_theme_lister)
CUSTOM_DOC("Opens an interactive list of all registered themes.")
{
    long_active_color_table = active_color_table;
    Color_Table* color_table = Long_Lister_ColorTable(app, S8Lit("Theme:"));
    if (color_table)
        long_active_color_table = active_color_table = *color_table;
    else
        active_color_table = long_active_color_table;
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
    i64 repeats = Long_Query_Number(app, "Repeat Count: ", 4);
    for (i64 i = 0; i < repeats; ++i)
        keyboard_macro_replay(app);
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
        
        String8 file_name = result.file_name_activated;
        if (file_name.size == 0)
        {
            file_name = result.file_name_in_text_field;
        }
        if (file_name.size == 0) break;
        
        String8 path = result.path_in_text_field;
        String8 full_file_name = push_u8_stringf(scratch, "%.*s/%.*s",
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
            String8 string = push_buffer_line(app, scratch, buffer, line);
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
            String8 string = push_buffer_range(app, scratch, buffer, Ii64(start_pos, end_pos));
            
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

function Range_i64 F4_LineRangeFromPosRange(Application_Links *app, Buffer_ID buffer, Range_i64 pos_range)
{
    Range_i64 lines_range = Ii64(get_line_number_from_pos(app, buffer, pos_range.min),
                                 get_line_number_from_pos(app, buffer, pos_range.max));
    return lines_range;
}

function void Long_ReIndentRange(Application_Links* app, View_ID view, Buffer_ID buffer, i64 indent_delta)
{
    i64 pos = view_get_cursor_pos(app, view);
    i64 mark = view_get_mark_pos(app, view);
    Range_i64 line_range = F4_LineRangeFromPosRange(app, buffer, Ii64(pos, mark));
    
    History_Group group = history_group_begin(app, buffer);
    
    // @COPYPASTA(long): F4_ReIndentLineRange
    for (i64 line = line_range.min; line <= line_range.max; line += 1)
    {
        // @COPYPASTA(long): F4_ReIndentLine
        Scratch_Block scratch(app);
        String8 line_string = push_buffer_line(app, scratch, buffer, line);
        i64 line_start_pos = get_line_start_pos(app, buffer, line);
        
        Range_i64 line_indent_range = Ii64(0, 0);
        i64 tabs_at_beginning = 0;
        i64 spaces_at_beginning = 0;
        for (u64 i = 0; i < line_string.size; i += 1)
        {
            char c = line_string.str[i];
            if (c == '\t')
                tabs_at_beginning++;
            else if(character_is_whitespace(c))
                spaces_at_beginning++;
            else
            {
                line_indent_range.max = (i64)i;
                break;
            }
        }
        
        // NOTE(rjf): Indent lines.
        {
            i64 indent_width = (i64)def_get_config_u64_lit(app, "indent_width");
            b32 indent_with_tabs = def_get_config_b32_lit("indent_with_tabs");
            i64 spaces_per_indent_level = indent_width;
            i64 indent_level = spaces_at_beginning / spaces_per_indent_level + tabs_at_beginning;
            i64 new_indent_level = indent_level + indent_delta;
            
            Range_i64 indent_range = Ii64(line_indent_range.min + line_start_pos, line_indent_range.max + line_start_pos);
            buffer_replace_range(app, buffer, indent_range, S8Lit(""));
            
            String8 indent_string = indent_with_tabs ? S8Lit("\t") : SCu8("                ", clamp_top(indent_width, 16));
            for (i64 i = 0; i < new_indent_level; i += 1)
                buffer_replace_range(app, buffer, Ii64(line_start_pos), indent_string);
        }
    }
    
    // @COPYPASTA(long): F4_AdjustCursorAndMarkForIndentation
    if (pos == mark)
    {
        Scratch_Block scratch(app);
        i64 start_pos = get_line_start_pos(app, buffer, line_range.min);
        i64 new_pos = start_pos;
        String8 line = push_buffer_line(app, scratch, buffer, line_range.min);
        for (u64 i = 0; i < line.size; i += 1)
        {
            if (!character_is_whitespace(line.str[i]))
            {
                new_pos = start_pos + (i64)i;
                break;
            }
        }
        
        view_set_cursor(app, view, seek_pos(new_pos));
        view_set_mark  (app, view, seek_pos(new_pos));
    }
    else
    {
        Range_i64 range;
        {
            if (line_range.min > line_range.max)
                Swap(i64, line_range.min, line_range.max);
            range = Ii64(get_line_start_pos(app, buffer, line_range.min), get_line_end_pos(app, buffer, line_range.max));
        }
        
        view_set_cursor(app, view, seek_pos(pos > mark ? range.max : range.min));
        view_set_mark  (app, view, seek_pos(pos > mark ? range.min : range.max));
    }
    
    history_group_end(group);
    no_mark_snap_to_cursor(app, view);
}

CUSTOM_COMMAND_SIG(long_autocomplete)
CUSTOM_DOC("When not in multi-cursor mode, tries to autocomplete the word currently being typed if such a word is found.")
{
    ProfileScope(app, "[Long] Word Complete");
    
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    Managed_Scope scope = view_get_managed_scope(app, view);
    
    if (buffer)
    {
        b32 first_completion = false;
        Rewrite_Type *rewrite = scope_attachment(app, scope, view_rewrite_loc, Rewrite_Type);
        if (*rewrite != Rewrite_WordComplete)
            first_completion = true;
        
        set_next_rewrite(app, view, Rewrite_WordComplete);
        
        Word_Complete_Iterator *it = word_complete_get_shared_iter(app);
        local_persist b32 initialized = 0;
        local_persist Range_i64 range = {};
        
        if (first_completion || !initialized)
        {
            ProfileScope(app, "[Long] Word Complete State Init");
            initialized = 0;
            i64 pos = view_get_cursor_pos(app, view);
            Range_i64 needle_range = get_word_complete_needle_range(app, buffer, pos);
            if (range_size(needle_range) > 0)
            {
                initialized = 1;
                range = needle_range;
                word_complete_iter_init(buffer, needle_range, it);
            }
        }
        
        b32 is_mc_mode = mc_context.active && mc_context.view == view;
        if (initialized && !is_mc_mode)
        {
            ProfileScope(app, "[Long] Word Complete Apply");
            
            word_complete_iter_next(it);
            String8 str = word_complete_iter_read(it);
            buffer_replace_range(app, buffer, range, str);
            
            range.max = range.min + str.size;
            view_set_cursor_and_preferred_x(app, view, seek_pos(range.max));
        }
        else leave_current_input_unhandled(app);
    }
}

CUSTOM_COMMAND_SIG(f4_unindent)
CUSTOM_DOC("Unindent the selected range.")
{
    View_ID view = get_active_view(app, Access_ReadWrite);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWrite);
    Long_ReIndentRange(app, view, buffer, -1);
}

function i64 Long_Line_FirstNonWhitespace(Application_Links* app, Buffer_ID buffer, i64 line)
{
    Scratch_Block scratch(app);
    
    // @COPYPASTA(long): push_buffer_line
    Range_i64 range = get_line_pos_range(app, buffer, line);
    String8 str = push_buffer_range(app, scratch, buffer, range);
    while (str.size > 0 && str.str[str.size - 1] == '\r')
        str.size -= 1;
    
    i64 result = string_find_first_non_whitespace(str) + range.first;
    return result;
}

function i64 Long_Line_LastNonWhitespace(Application_Links* app, Buffer_ID buffer, i64 line)
{
    Scratch_Block scratch(app);
    
    // @COPYPASTA(long): push_buffer_line
    Range_i64 range = get_line_pos_range(app, buffer, line);
    String8 str = push_buffer_range(app, scratch, buffer, range);
    while (str.size > 0 && str.str[str.size - 1] == '\r')
        str.size -= 1;
    
    i64 result = string_find_last_non_whitespace(str) + range.first;
    return result;
}

// @COPYPASTA(long): F4_SetLineCommentedOnLine
function b32 Long_Comment_ToggleLine(Application_Links* app, Buffer_ID buffer, i64 cursor, b32 force_comment)
{
    b32 result = 0;
    i64 cursor_line = get_line_number_from_pos(app, buffer, cursor);
    
    if (!line_is_blank(app, buffer, cursor_line))
    {
        i64 line_beg = Long_Line_FirstNonWhitespace(app, buffer, cursor_line);
        i64 line_end = line_beg;
        String8 str = string_u8_empty;
        
        b32 already_has_comment = c_line_comment_starts_at_position(app, buffer, line_beg);
        result = already_has_comment && !force_comment;
        if (result)
            line_end += 2;
        else
            str = S8Lit("//");
        
        buffer_replace_range(app, buffer, Ii64(line_beg, line_end), str);
    }
    return result;
}

// @COPYPASTA(long): F4_SetBlockCommentedOnRange
function void Long_Comment_SetRange(Application_Links* app, Buffer_ID buffer, i64* cursor_p, i64* mark_p, b32 commented)
{
    i64 cursor = *cursor_p;
    i64 mark = *mark_p;
    i64 shift = 0;
    Range_i64 range = Ii64(cursor, mark);
    
    if (commented)
    {
        buffer_replace_range(app, buffer, Ii64(range.max), S8Lit("*/"));
        buffer_replace_range(app, buffer, Ii64(range.min), S8Lit("/*"));
        shift = 4;
    }
    
    else if (range.max - range.min >= 2)
    {
        i64 end_pos;
        seek_string_forward(app, buffer, range.min+2, range.max, S8Lit("*/"), &end_pos);
        
        if (end_pos < range.max)
        {
            buffer_replace_range(app, buffer, Ii64_size(  end_pos, 2), S8Lit(""));
            buffer_replace_range(app, buffer, Ii64_size(range.min, 2), S8Lit(""));
            shift = -4;
        }
    }
    
    if (cursor < mark) mark += shift;
    else             cursor += shift;
    
    *cursor_p = cursor;
    *mark_p = mark;
}

// @COPYPASTA(long): F4_SetCommentedOnRange
function void Long_Comment_ToggleRange(Application_Links* app, Buffer_ID buffer, i64* cursor_p, i64* mark_p)
{
    i64 cursor = *cursor_p;
    i64 mark = *mark_p;
    Range_i64 range = Ii64(cursor, mark);
    
    Range_i64 line_range = F4_LineRangeFromPosRange(app, buffer, range);
    i64 beg_of_min_line = Long_Line_FirstNonWhitespace(app, buffer, line_range.min);
    i64 end_of_max_line =  Long_Line_LastNonWhitespace(app, buffer, line_range.max);
    
    Token_Array tokens = get_token_array_from_buffer(app, buffer);
    Token* min_token = get_token_from_pos(app, &tokens, range.min);
    Token* max_token = get_token_from_pos(app, &tokens, range.max);
    
    b32 already_has_block_comment = 0;
    {
        u8 check_buffer[2];
        i64 check_pos = clamp_bot(beg_of_min_line, range.min);
        if (buffer_read_range(app, buffer, Ii64_size(check_pos, 2), check_buffer))
            if (check_buffer[0] == '/' && check_buffer[1] == '*')
                already_has_block_comment = 1;
        
        if (already_has_block_comment)
        {
            if (cursor > mark) mark = check_pos;
            else             cursor = check_pos;
        }
    }
    
    // NOTE(rjf): Selection is inside comment
    if (min_token == max_token && min_token && min_token->kind == TokenBaseKind_Comment)
    {
        u8 c = buffer_get_char(app, buffer, min_token->pos+1);
        if (c == '*')
            buffer_replace_range(app, buffer, Ii64_size(min_token->pos + min_token->size, -2), S8Lit(""));
        buffer_replace_range(app, buffer, Ii64_size(min_token->pos, 2), S8Lit(""));
        cursor = clamp_bot(cursor - 2, min_token->pos);
        mark = clamp_bot(mark - 2, min_token->pos);
    }
    
    // NOTE(rjf): No selection
    else if (range.min == range.max)
    {
        b32 was_comment = Long_Comment_ToggleLine(app, buffer, cursor, 0);
        if (was_comment) cursor = mark -= 2;
        else             cursor = mark += 2;
    }
    
    // NOTE(rjf): Single-Line Selection
    else if (line_range.min == line_range.max)
    {
        if (range.min == beg_of_min_line && range.max == end_of_max_line + 1 && !already_has_block_comment)
        {
            b32 was_comment = Long_Comment_ToggleLine(app, buffer, cursor, 0);
            if (cursor < mark) mark += was_comment ? -2 : 2;
            else             cursor += was_comment ? -2 : 2;
        }
        
        // NOTE(rjf): Selection is not inside comment
        else Long_Comment_SetRange(app, buffer, &cursor, &mark, !already_has_block_comment);
    }
    
    // NOTE(rjf): Multi-Line Selection
    else if (line_range.min != line_range.max)
    {
        i64 min_pos = Min(cursor, mark);
        i64 max_pos = Max(cursor, mark);
        
        // NOTE(long): Use a multi-line block comment if there's at least one line that has a single-line comment
        b32 has_some_comment_lines = 0, has_all_comment_lines = 1;
        for (i64 i = line_range.min; i <= line_range.max; i += 1)
        {
            if (!line_is_blank(app, buffer, i))
            {
                i64 pos = Long_Line_FirstNonWhitespace(app, buffer, i);
                if (c_line_comment_starts_at_position(app, buffer, pos))
                    has_some_comment_lines = 1;
                else
                    has_all_comment_lines = 0;
            }
        }
        
        // NOTE(long): Selection starts on first column and ends on last column
        if (min_pos == beg_of_min_line && max_pos > end_of_max_line)
        {
            for (i64 i = line_range.min; i <= line_range.max; i += 1)
            {
                i64 cursor2 = get_line_start_pos(app, buffer, i);
                Long_Comment_ToggleLine(app, buffer, cursor2, has_some_comment_lines && !has_all_comment_lines);
            }
            
            end_of_max_line = Long_Line_LastNonWhitespace(app, buffer, line_range.max) + 1;
            if (cursor < mark)
            {
                cursor = beg_of_min_line;
                mark = end_of_max_line;
            }
            
            else
            {
                mark = beg_of_min_line;
                cursor = end_of_max_line;
            }
        }
        
        // NOTE(long): Selection does not start on first column or end on last column
        else Long_Comment_SetRange(app, buffer, &cursor, &mark, !already_has_block_comment);
    }
    
    Long_Render_FadeHighlight(app, buffer, Ii64(cursor, mark));
    *cursor_p = cursor;
    *mark_p = mark;
}

CUSTOM_COMMAND_SIG(long_toggle_comment_selection)
CUSTOM_DOC("Performs VS-style (un)commenting on the selected range.")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    i64 cursor = view_get_cursor_pos(app, view);
    i64 mark = view_get_mark_pos(app, view);
    
    History_Group group = history_group_begin(app, buffer);
    Long_Comment_ToggleRange(app, buffer, &cursor, &mark);
    view_set_cursor(app, view, seek_pos(cursor));
    view_set_mark(app, view, seek_pos(mark));
    history_group_end(group);
    no_mark_snap_to_cursor(app, view);
}

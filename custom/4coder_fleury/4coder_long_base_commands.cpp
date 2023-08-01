
//~NOTE(long): Helper Functions

function Buffer_ID Long_Buffer_GetJumpBuffer(Application_Links* app)
{
    Buffer_ID result = get_locked_jump_buffer(app);
    if (!result)
    result = get_buffer_by_name(app, search_name, Access_Always);
    return result;
}

function String8 Long_Buffer_PushLine(Application_Links* app, Arena* arena, Buffer_ID buffer, i64 pos)
{
    Buffer_Cursor cursor = buffer_compute_cursor(app, buffer, seek_pos(pos));
    String_Const_u8 line = push_buffer_line(app, arena, buffer, cursor.line);
    return line;
}

function String8 Long_Buffer_GetNameWithoutPrjPath(Application_Links* app, Arena* arena, Buffer_ID buffer)
{
    Scratch_Block scratch(app, arena);
    String8 filepath = push_buffer_file_name(app, scratch, buffer);
    {
        String8 buffer_name = push_buffer_base_name(app, scratch, buffer);
        filepath = string_chop(filepath, buffer_name.size);
        // NOTE(long): the push_buffer_base_name returns a string with '\\'
        //filepath = string_mod_replace_character(filepath, '/', '\\');
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

// TODO(long): Make a utility function for getting the cursor's pos and buffer from a given view

struct LongListerData
{
    b32 only_draw_path;
    Buffer_ID buffer;
    i64 pos;
    i64 user_index;
};

function void Long_Lister_AddItem(Application_Links* app, Lister* lister, String8 name, String8 tag,
                                  Buffer_ID buffer, i64 pos, u64 index = 0)
{
    LongListerData* data = (LongListerData*)lister_add_item(lister, name, tag, 0, sizeof(LongListerData));
    data->only_draw_path = 0;
    data->buffer = buffer;
    data->pos = pos;
    data->user_index = index;
    ((Lister_Node*)data - 1)->user_data = data;
}

function void Long_Lister_AddBuffer(Application_Links* app, Lister* lister, String8 name, String8 tag, Buffer_ID buffer)
{
    LongListerData* data = (LongListerData*)lister_add_item(lister, name, tag, 0, sizeof(LongListerData));
    data->only_draw_path = 1;
    data->buffer = buffer;
    data->pos = 0;
    data->user_index = 0;
    ((Lister_Node*)data - 1)->user_data = data;
}

function String8 Long_Lister_GetHeaderString(Application_Links* app, Arena* arena, Lister_Node* node)
{
    String8 result = {};
    if (node->user_data == (node + 1))
    {
        LongListerData data = *(LongListerData*)(node + 1);
        String8 filename = push_buffer_unique_name(app, arena, data.buffer);
        if (data.only_draw_path)
        {
            String8 filepath = Long_Buffer_GetNameWithoutPrjPath(app, arena, data.buffer);
            if (filepath.size)
            result = push_stringf(arena, "[%.*s] ", string_expand(filepath));
        }
        else
        {
            Buffer_Cursor cursor = buffer_compute_cursor(app, data.buffer, seek_pos(data.pos));
            result = push_stringf(arena, "[%.*s:%d:%d] ", string_expand(filename), cursor.line, cursor.col);
        }
    }
    return result;
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

function void Long_ReadPointStack(Application_Links* app, Long_Point_Stack* stack, i32 index, Buffer_ID* out_buffer, i64* out_pos)
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
        Long_ReadPointStack(app, stack, clamp_loop(stack->top - 1, size), &top_buffer, &top_pos);
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

function void Long_JumpToLocation(Application_Links* app, View_ID view,
                                  Buffer_ID current_buffer, i64 current_pos, Buffer_ID target_buffer, i64 target_pos)
{
    Long_PointStack_Push(app, current_buffer, current_pos, view);
    F4_JumpToLocation(app, view, target_buffer, target_pos);
    Long_PointStack_Push(app, target_buffer, target_pos, view);
}

function void Long_JumpToLocation(Application_Links* app, View_ID view, Buffer_ID target_buffer, i64 target_pos)
{
    Long_JumpToLocation(app, view, view_get_buffer(app, view, Access_Always), view_get_cursor_pos(app, view), target_buffer, target_pos);
}

#define Long_IteratePointStack(app, stack, start, end, advance, size, func) \
for (i32 i = start; i != end; i = clamp_loop(i + (advance), size)) \
{ \
Buffer_ID buffer = 0; \
i64 pos = 0; \
Long_ReadPointStack(app, stack, i, &buffer, &pos); \
if (!buffer && !pos) continue; \
func; \
}

CUSTOM_UI_COMMAND_SIG/*CUSTOM_COMMAND_SIG*/(long_list_all_jump_points)
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
                               String8 tag = (i == stack->current - 1) ? S8Lit("current") : String8{};
                               Long_Lister_AddItem(app, lister, line, tag, buffer, pos, i);
                           });
    
    lister_set_query(lister, push_u8_stringf(scratch, "Top: %d, Bot: %d, Current: %d", stack->top, stack->bot, stack->current));
    Lister_Result l_result = run_lister(app, lister);
    
    LongListerData result = {};
    if (!l_result.canceled && l_result.user_data)
    block_copy_struct(&result, (LongListerData*)l_result.user_data);
    
    if (result.buffer != 0)
    {
        F4_JumpToLocation(app, get_this_ctx_view(app, Access_Always), result.buffer, result.pos);
        stack->current = clamp_loop(result.user_index + 1, size);
    }
}

function void Long_JumpToNextPointStack(Application_Links* app, i32 start, i32 end, i32 advance)
{
    Long_Point_Stack* stack = Long_GetPointStack(app);
    if (stack->top == stack->bot)
    return;
    
    View_ID view = get_active_view(app, Access_Visible);
    if (view != 0)
    {
        i32 size = ArrayCount(stack->markers);
        Buffer_ID current_buffer = view_get_buffer(app, view, Access_Always);
        i64 current_pos = view_get_cursor_pos(app, view);
        
        Long_IteratePointStack(app, stack, clamp_loop(start, size), clamp_loop(end, size), advance, size,
                               if (buffer != current_buffer || pos != current_pos)
                               {
                                   F4_JumpToLocation(app, view, buffer, pos);
                                   stack->current = clamp_loop(i + 1, size);
                                   break;
                               });
    }
}

CUSTOM_COMMAND_SIG(long_undo_jump)
CUSTOM_DOC("Read from the current point stack and jump there; if already there go to the previous option")
{
    Long_Point_Stack* stack = Long_GetPointStack(app);
    Long_JumpToNextPointStack(app, stack->current - 1, stack->bot - 1, -1);
}

CUSTOM_COMMAND_SIG(long_redo_jump)
CUSTOM_DOC("Read from the current point stack and jump there; if already there go to the next option")
{
    Long_Point_Stack* stack = Long_GetPointStack(app);
    Long_JumpToNextPointStack(app, stack->current - 1, stack->top - 0, +1);
}

CUSTOM_COMMAND_SIG(long_push_new_jump)
CUSTOM_DOC("Push the current position to the point stack; if the stack's current is the position then ignore it")
{
    View_ID view = get_active_view(app, Access_Always);
    Long_PointStack_Push(app, view_get_buffer(app, view, Access_Always), view_get_cursor_pos(app, view), view);
}

//~ NOTE(long): List/Search Commands

global Buffer_ID long_buffer_before_search = 0;

function void Long_JumpToBuffer(Application_Links* app, View_ID view, Buffer_ID buffer)
{
    Long_PointStack_Push(app, view_get_buffer(app, view, Access_Always), view_get_cursor_pos(app, view));
    view_set_buffer(app, view, buffer, 0);
    Long_PointStack_Push(app, buffer, view_get_cursor_pos(app, view));
}

function b32 Long_IsSearchBuffer(Application_Links* app, Buffer_ID buffer)
{
    Scratch_Block scratch(app);
    String8 name = push_buffer_base_name(app, scratch, buffer);
    return string_match(name, search_name);
}

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
    String8 filepath = Long_Buffer_GetNameWithoutPrjPath(app, scratch, buffer);
    if (filepath.size)
    status = push_stringf(scratch, "<%.*s>%s%.*s", string_expand(filepath), dirty ? " " : "", string_expand(status));
    lister_add_item(lister, buffer_name, status, IntAsPtr(buffer), 0);
#endif
}

// NOTE(long): This function was copied from generate_all_buffers_list
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
    if (!l_result.canceled)
#if OUTPUT_BUFFER_HEADER
	result = ((LongListerData*)l_result.user_data)->buffer;
#else
	result = (Buffer_ID)(PtrAsInt(l_result.user_data));
#endif
    return result;
}

CUSTOM_UI_COMMAND_SIG/*CUSTOM_COMMAND_SIG*/(long_interactive_switch_buffer)
CUSTOM_DOC("Interactively switch to an open buffer.")
{
    // NOTE(long): This function was copied from 4coder's interactive_switch_buffer and modified to call Long_JumpToBuffer
    Buffer_ID buffer = Long_Buffer_RunLister(app, "Switch:");
    if (buffer != 0)
    {
        View_ID view = get_this_ctx_view(app, Access_Always);
        if (Long_IsSearchBuffer(app, buffer))
        long_buffer_before_search = view_get_buffer(app, view, 0);
        Long_JumpToBuffer(app, view, buffer);
    }
}

CUSTOM_UI_COMMAND_SIG/*CUSTOM_COMMAND_SIG*/(long_switch_to_jump_buffer)
CUSTOM_DOC("Interactively kill an open buffer.")
{
    view_set_buffer(app, get_this_ctx_view(app, Access_Always), Long_Buffer_GetJumpBuffer(app), 0);
}

function void Long_KillBuffer(Application_Links* app, Buffer_ID buffer, View_ID view)
{
    // NOTE(long): `buffer_view` is the view that has the buffer while `view` is the view that shows the kill options
    View_ID buffer_view = get_first_view_with_buffer(app, buffer);
    
    // NOTE(long): Must do this check before killing the buffer
    b32 is_search_buffer = Long_IsSearchBuffer(app, buffer);
    // NOTE(long): I don't need this check here because the search buffer is never dirty and always killable
    b32 killed = try_buffer_kill(app, buffer, view, 0) == BufferKillResult_Killed;
    
    if (is_search_buffer && killed)
    {
        if (!get_first_view_with_buffer(app, long_buffer_before_search))
        view_set_buffer(app, buffer_view, long_buffer_before_search, 0);
        long_buffer_before_search = 0;
    }
}

CUSTOM_UI_COMMAND_SIG/*CUSTOM_COMMAND_SIG*/(long_interactive_kill_buffer)
CUSTOM_DOC("Interactively kill an open buffer.")
{
    Long_KillBuffer(app, Long_Buffer_RunLister(app, "Kill:"), get_this_ctx_view(app, Access_Always));
}

CUSTOM_COMMAND_SIG(long_kill_buffer)
CUSTOM_DOC("Kills the current buffer.")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    Long_KillBuffer(app, buffer, view);
}

CUSTOM_COMMAND_SIG(long_kill_jump_buffer)
CUSTOM_DOC("Kills the current locked jump buffer.")
{
    Long_KillBuffer(app, Long_Buffer_GetJumpBuffer(app), get_active_view(app, Access_ReadVisible));
}

CUSTOM_COMMAND_SIG(long_view_jump_list_with_lister)
CUSTOM_DOC("When executed on a buffer with jumps, creates a persistent lister for all the jumps")
{
    View_ID view = get_active_view(app, Access_Always);
    Buffer_ID jump_buffer = Long_Buffer_GetJumpBuffer(app);
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
                line = string_skip_chop_whitespace(line);
                Long_Lister_AddItem(app, lister, line, {}, buffer, pos, i);
            }
        }
        
        Lister_Result l_result = run_lister(app, lister);
        if (!l_result.canceled && l_result.user_data)
        {
            jump.success = true;
            jump.index = (i32)((LongListerData*)l_result.user_data)->user_index;
        }
        
        jump_to_jump_lister_result(app, view, list, &jump);
    }
}

function Buffer_ID Long_CreateOrSwitchBuffer(Application_Links *app, String_Const_u8 name_string, View_ID default_target_view)
{
    Buffer_ID search_buffer = get_buffer_by_name(app, name_string, Access_Always);
    long_buffer_before_search = view_get_buffer(app, default_target_view, Access_Always);
    Scratch_Block scratch(app);
    
    if (search_buffer)
    {
        {
            View_ID target_view = get_first_view_with_buffer(app, search_buffer);
            if (!target_view)
            {
                target_view = default_target_view;
                view_set_buffer(app, target_view, search_buffer, 0);
            }
            view_set_active(app, target_view);
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
        view_set_buffer(app, default_target_view, search_buffer, 0);
        view_set_active(app, default_target_view);
    }
    
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

function void print_all_matches(Application_Links *app, String_Const_u8_Array match_patterns, String_Match_Flag must_have_flags,
                                String_Match_Flag must_not_have_flags, Buffer_ID out_buffer_id, b32 current_buffer)
{
    Scratch_Block scratch(app);
    String_Match_List matches = Long_FindAllMatches(app, scratch, match_patterns, must_have_flags, must_not_have_flags, current_buffer);
    string_match_list_filter_remove_buffer(&matches, out_buffer_id);
    string_match_list_filter_remove_buffer_predicate(app, &matches, buffer_has_name_with_star);
    print_string_match_list_to_buffer(app, out_buffer_id, matches);
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
    
    Buffer_ID current_buffer = 0;
    if (!all_buffer)
    current_buffer = view_get_buffer(app, get_active_view(app, Access_Always), Access_Always);
    Buffer_ID search_buffer = Long_CreateOrSwitchBuffer(app, search_name, get_next_view_after_active(app, Access_Always));
    print_all_matches(app, { &needle, 1 }, must_have_flags, must_not_have_flags, search_buffer, current_buffer);
}

function void Long_ListAllLocations_Query(Application_Links *app, List_All_Locations_Flag flags, b32 all_buffer)
{
    Scratch_Block scratch(app);
    u8 *space = push_array(scratch, u8, KB(1));
    String_Const_u8 needle = get_query_string(app, "List Locations For: ", space, KB(1));
    Long_ListAllLocations(app, needle, flags, all_buffer);
}

function void Long_ListAllLocations_Identifier(Application_Links *app, List_All_Locations_Flag flags, b32 all_buffer)
{
    Scratch_Block scratch(app);
    String_Const_u8 needle = push_token_or_word_under_active_cursor(app, scratch);
    Long_ListAllLocations(app, needle, flags, all_buffer);
}

CUSTOM_COMMAND_SIG(long_list_all_locations)
CUSTOM_DOC("Queries the user for a string and lists all exact case-sensitive matches found in all open buffers.")
{
    Long_ListAllLocations_Query(app, ListAllLocationsFlag_CaseSensitive, 1);
}

CUSTOM_COMMAND_SIG(long_list_all_substring_locations_case_insensitive)
CUSTOM_DOC("Queries the user for a string and lists all case-insensitive substring matches found in all open buffers.")
{
    Long_ListAllLocations_Query(app, ListAllLocationsFlag_MatchSubstring, 1);
}

CUSTOM_COMMAND_SIG(long_list_all_locations_current_buffer)
CUSTOM_DOC("Queries the user for a string and lists all exact case-sensitive matches found in the current buffer.")
{
    Long_ListAllLocations_Query(app, ListAllLocationsFlag_CaseSensitive, 0);
}

CUSTOM_COMMAND_SIG(long_list_all_substring_locations_case_insensitive_current_buffer)
CUSTOM_DOC("Queries the user for a string and lists all case-insensitive substring matches found in the current buffer.")
{
    Long_ListAllLocations_Query(app, ListAllLocationsFlag_MatchSubstring, 0);
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

// NOTE(long): This function was copied from isearch and modified so that it handle case-sensitive string
function void Long_Search(Application_Links *app, Scan_Direction start_scan, i64 first_pos, String_Const_u8 query_init, b32 insensitive)
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
    
    u8 bar_string_space[256];
    bar.string = SCu8(bar_string_space, query_init.size);
    block_copy(bar.string.str, query_init.str, query_init.size);
    
    String_Const_u8 isearch_str = string_u8_litexpr("I-Search: ");
    String_Const_u8 rsearch_str = string_u8_litexpr("Reverse-I-Search: ");
    
    u64 match_size = bar.string.size;
    
    User_Input in = {};
    for (;;){
        switch (scan){
            case Scan_Forward:
            {
                bar.prompt = isearch_str;
            }break;
            case Scan_Backward:
            {
                bar.prompt = rsearch_str;
            }break;
        }
        isearch__update_highlight(app, view, Ii64_size(pos, match_size));
        
        in = get_next_input(app, EventPropertyGroup_Any, EventProperty_Escape);
        if (in.abort){
            break;
        }
        
        String_Const_u8 string = to_writable(&in);
        
        b32 string_change = false;
        if (match_key_code(&in, KeyCode_Return) ||
            match_key_code(&in, KeyCode_Tab)){
            Input_Modifier_Set *mods = &in.event.key.modifiers;
            if (has_modifier(mods, KeyCode_Control)){
                bar.string.size = cstring_length(previous_isearch_query);
                block_copy(bar.string.str, previous_isearch_query, bar.string.size);
            }
            else{
                u64 size = bar.string.size;
                size = clamp_top(size, sizeof(previous_isearch_query) - 1);
                block_copy(previous_isearch_query, bar.string.str, size);
                previous_isearch_query[size] = 0;
                break;
            }
        }
        else if (string.str != 0 && string.size > 0){
            String_u8 bar_string = Su8(bar.string, sizeof(bar_string_space));
            string_append(&bar_string, string);
            bar.string = bar_string.string;
            string_change = true;
        }
        else if (match_key_code(&in, KeyCode_Backspace)){
            if (is_unmodified_key(&in.event)){
                u64 old_bar_string_size = bar.string.size;
                bar.string = backspace_utf8(bar.string);
                string_change = (bar.string.size < old_bar_string_size);
            }
            else if (has_modifier(&in.event.key.modifiers, KeyCode_Control)){
                if (bar.string.size > 0){
                    string_change = true;
                    bar.string.size = 0;
                }
            }
        }
        
        b32 do_scan_action = false;
        b32 do_scroll_wheel = false;
        Scan_Direction change_scan = scan;
        if (!string_change){
            if (match_key_code(&in, KeyCode_PageDown) ||
                match_key_code(&in, KeyCode_Down)){
                change_scan = Scan_Forward;
                do_scan_action = true;
            }
            else if (match_key_code(&in, KeyCode_PageUp) ||
                     match_key_code(&in, KeyCode_Up)){
                change_scan = Scan_Backward;
                do_scan_action = true;
            }
            else{
                // NOTE(allen): is the user trying to execute another command?
                View_Context ctx = view_current_context(app, view);
                Mapping *mapping = ctx.mapping;
                Command_Map *map = mapping_get_map(mapping, ctx.map_id);
                Command_Binding binding = map_get_binding_recursive(mapping, map, &in.event);
                if (binding.custom != 0){
                    if (binding.custom == search){
                        change_scan = Scan_Forward;
                        do_scan_action = true;
                    }
                    else if (binding.custom == reverse_search){
                        change_scan = Scan_Backward;
                        do_scan_action = true;
                    }
                    else{
                        Command_Metadata *metadata = get_command_metadata(binding.custom);
                        if (metadata != 0){
                            if (metadata->is_ui){
                                view_enqueue_command_function(app, view, binding.custom);
                                break;
                            }
                        }
                        binding.custom(app);
                    }
                }
                else{
                    leave_current_input_unhandled(app);
                }
            }
        }
        
        if (string_change || do_scan_action)
        {
            if (do_scan_action)
            scan = change_scan;
            b32 backward = scan == Scan_Backward;
            i64 offset = 0;
            if (string_change)
            offset = (backward ? +1 : -1);
            
            i64 new_pos = 0;
            seek_string(app, buffer, pos + offset, 0, 0, bar.string, &new_pos, backward|insensitive);
            if (new_pos >= 0 && new_pos < buffer_size)
            {
                pos = new_pos;
                match_size = bar.string.size;
            }
        }
        
        else if (do_scroll_wheel){
            mouse_wheel_scroll(app);
        }
    }
    
    view_disable_highlight_range(app, view);
    
    if (in.abort){
        u64 size = bar.string.size;
        size = clamp_top(size, sizeof(previous_isearch_query) - 1);
        block_copy(previous_isearch_query, bar.string.str, size);
        previous_isearch_query[size] = 0;
        view_set_cursor_and_preferred_x(app, view, seek_pos(first_pos));
    }
    
    view_set_camera_bounds(app, view, old_margin, old_push_in);
}

// NOTE(long): This function was copied from F4_Search and changed so it calls Long_Search
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
        Long_Search(app, dir, cursor, query_init, insensitive);
    }
}

// NOTE(long): This function was copied from isearch_identifier and changed so it calls Long_Search
function void Long_Search_Identifier(Application_Links *app, Scan_Direction scan, b32 insensitive)
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer_id = view_get_buffer(app, view, Access_ReadVisible);
    i64 pos = view_get_cursor_pos(app, view);
    Scratch_Block scratch(app);
    Range_i64 range = enclose_pos_alpha_numeric_underscore(app, buffer_id, pos);
    String_Const_u8 query = push_buffer_range(app, scratch, buffer_id, range);
    Long_Search(app, scan, range.first, query, insensitive);
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

//~ NOTE(long): Index Commands

function void Long_GoToDefinition(Application_Links* app, b32 same_panel)
{
    F4_Index_Note* note = 0;
    View_ID view = get_active_view(app, Access_Always);
    {
        Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
        Token* token = get_token_from_pos(app, buffer, view_get_cursor_pos(app, view));
        if (token != 0 && token->size > 0 && token->kind != TokenBaseKind_Whitespace)
        {
            Token_Array array = get_token_array_from_buffer(app, buffer);
            note = Long_Index_LookupBestNote(app, buffer, &array, token);
            if (note && note->range.min == token->pos && LONG_INDEX_FILTER_NOTE(note))
            {
                F4_Index_Note* next_note = note;
                do
                {
                    next_note = next_note->next;
                    if (!next_note)
                    next_note = Long_Index_LookupNote(note->string);
                    if (next_note == note)
                    break;
                } while (!LONG_INDEX_FILTER_NOTE(next_note));
                
                if (next_note)
                note = next_note;
            }
        }
    }
    
    // NOTE(long): This was copied from F4_GoToDefinition and modified so it calls Long_JumpToLocation
    if (note)
    {
        if(!same_panel)
        view = get_next_view_looped_primary_panels(app, view, Access_Always);
        
        // NOTE(long): The difference between F4_JumpToLocation and F4_GoToDefinition is:
        // 1. The first one scroll quickly to the target while the second one snap to the target instantly
        // (scroll.position.line_number/pixel_shift.y = scroll.target.line_number/pixel_shift.y)
        // 2. The first one call view_set_preferred_x (I don't really know what this function does)
        // For simplicity, I just use Long_JumpToLocation (which calls F4_JumpToLocation) for all jumping behaviors
        Long_JumpToLocation(app, view, note->file->buffer, note->range.min);
    }
}

CUSTOM_COMMAND_SIG(long_go_to_definition)
CUSTOM_DOC("Goes to the definition of the identifier under the cursor.")
{
    Long_GoToDefinition(app, 0);
}

CUSTOM_COMMAND_SIG(long_go_to_definition_same_panel)
CUSTOM_DOC("Goes to the definition of the identifier under the cursor in the same panel.")
{
    Long_GoToDefinition(app, 1);
}

function String8 _Long_PushNoteTag(Application_Links* app, Arena* arena, F4_Index_Note* note)
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
                if (note->range.max < note->parent->scope_range.min)
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
    if (Long_Index_IsNamespace(note))
    result = push_stringf(arena, "%.*s%s[namespace]", string_expand(result), result.size ? " " : "");
    
    return result;
}

function LONG_INDEX_FILTER(Long_Filter_Scopes)
{
    return note->kind != F4_Index_NoteKind_Scope;
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
    return note->kind == F4_Index_NoteKind_Type || note->kind == F4_Index_NoteKind_Function;
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
            if (note->range.max < parent->scope_range.min)
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
        String8 string = note->string;
        if (string_match(string, S8Lit("Vector2")))
        string = note->string;
        
        Scratch_Block scratch(app, arena);
        b32 has_arg = note->scope_range.min > note->range.min;
        if (note->base_range.max || has_arg)
        {
            String8List list = {};
            if (note->base_range.max)
            string_list_push(scratch, &list, push_buffer_range(app, scratch, buffer, note->base_range));
            string_list_push(scratch, &list, push_buffer_range(app, scratch, buffer, { note->range.min,
                                                                   has_arg ? note->scope_range.min : note->range.max }));
            string = string_list_flatten(arena, list, S8Lit(" "), 0, 0);
            string = string_condense_whitespace(scratch, string);
        }
        
        Long_Lister_AddItem(app, lister, string, _Long_PushNoteTag(app, arena, note), buffer, note->range.first);
    }
    
    Long_Index_IterateValidNoteInFile(child, note)
    Long_Lister_PushIndexNote(app, arena, lister, child, filter);
}

function void Long_Lister_PushDebugNote(Application_Links* app, Arena* arena, Lister* lister, F4_Index_Note* note, b32 search_file)
{
    Buffer_ID buffer = note->file->buffer;
    String8 string = note->string;
    
    Scratch_Block scratch(app, arena);
    b32 has_arg = note->scope_range.min > note->range.min;
    if (note->base_range.max || has_arg)
    {
        String8List list = {};
        if (note->base_range.max)
        string_list_push(scratch, &list, push_buffer_range(app, scratch, buffer, note->base_range));
        string_list_push(scratch, &list, push_buffer_range(app, scratch, buffer,{ note->range.min,
                                                               has_arg ? note->scope_range.min : note->range.max }));
        string = string_list_flatten(arena, list, S8Lit(" "), 0, 0);
        string = string_condense_whitespace(scratch, string);
    }
    
    Long_Lister_AddItem(app, lister, string, _Long_PushNoteTag(app, arena, note), buffer, note->range.first);
    
    if (!search_file || !Long_Index_IsNamespace(note))
    for (F4_Index_Note* child = note->first_child; child; child = child->next_sibling)
    Long_Lister_PushDebugNote(app, arena, lister, child, search_file);
}

function void _Long_SearchDefinition(Application_Links* app, NoteFilter* filter, b32 project_wide)
{
    View_ID view = get_active_view(app, Access_Always);
    
    Scratch_Block scratch(app);
    Lister_Block lister(app, scratch);
    lister_set_default_handlers(lister);
    
    F4_Index_Lock();
    {
        for (Buffer_ID buffer = project_wide ? get_buffer_next(app, 0, Access_Always) : view_get_buffer(app, view, Access_Always);
             buffer; buffer = get_buffer_next(app, buffer, Access_Always))
        {
            F4_Index_File* file = F4_Index_LookupFile(app, buffer);
            String8 filename = push_buffer_base_name(app, scratch, buffer);
            for (F4_Index_Note* note = file ? file->first_note : 0; note; note = note->next_sibling)
            Long_Lister_PushIndexNote(app, scratch, lister, note, filter);
            if (!project_wide)
            break;
        }
    }
    F4_Index_Unlock();
    
    lister_set_query(lister, push_u8_stringf(scratch, "Index (%s):", project_wide ? "Project" : "File"));
    Lister_Result l_result = run_lister(app, lister);
    LongListerData result = {};
    if (!l_result.canceled && l_result.user_data)
    block_copy_struct(&result, (LongListerData*)l_result.user_data);
    if (result.buffer != 0)
    Long_JumpToLocation(app, view, result.buffer, result.pos);
}

CUSTOM_UI_COMMAND_SIG(long_search_for_definition__debug)
CUSTOM_DOC("List all definitions in the index and jump to the one selected by the user.")
{
    View_ID view = get_active_view(app, Access_Always);
    
    Scratch_Block scratch(app);
    Lister_Block lister(app, scratch);
    lister_set_default_handlers(lister);
    
    F4_Index_Lock();
    for (F4_Index_Note* note = namespace_root.first_child; note; note = note->next_sibling)
    Long_Lister_PushDebugNote(app, scratch, lister, note, 0);
    for (Buffer_ID buffer = get_buffer_next(app, 0, Access_Always); buffer; buffer = get_buffer_next(app, buffer, Access_Always))
    {
        F4_Index_File* file = F4_Index_LookupFile(app, buffer);
        for (F4_Index_Note* note = file ? file->first_note : 0; note; note = note->next_sibling)
        Long_Lister_PushDebugNote(app, scratch, lister, note, 1);
    }
    F4_Index_Unlock();
    
    lister_set_query(lister, S8Lit("Index (Debug):"));
    Lister_Result l_result = run_lister(app, lister);
    LongListerData result = {};
    if (!l_result.canceled && l_result.user_data)
    block_copy_struct(&result, (LongListerData*)l_result.user_data);
    if (result.buffer != 0)
    Long_JumpToLocation(app, view, result.buffer, result.pos);
}

CUSTOM_UI_COMMAND_SIG/*CUSTOM_COMMAND_SIG*/(long_search_for_definition__project_wide)
CUSTOM_DOC("List all definitions in the index and jump to the one selected by the user.")
{
    _Long_SearchDefinition(app, LONG_INDEX_FILTER_NOTE, 1);
}

CUSTOM_UI_COMMAND_SIG/*CUSTOM_COMMAND_SIG*/(long_search_for_definition__current_file)
CUSTOM_DOC("List all definitions in the current file and jump to the one selected by the user.")
{
    _Long_SearchDefinition(app, LONG_INDEX_FILTER_NOTE, 0);
}

CUSTOM_UI_COMMAND_SIG/*CUSTOM_COMMAND_SIG*/(long_search_for_definition_no_filter__project_file)
CUSTOM_DOC("List all definitions in the current file and jump to the one selected by the user.")
{
    _Long_SearchDefinition(app, /*Long_Filter_Scopes*/0, 1);
}

function void _Long_PushNoteString(Application_Links* app, Arena* arena, String8List* list, int* noteCount,
                                   F4_Index_Note* note, NoteFilter* filter)
{
    if (*noteCount > 3000)
    system_error_box("PushNoteString is overflowed");
    if (!((note->scope_range.min == note->scope_range.max) || (note->scope_range.min && note->scope_range.max)))
    {
        String8 string = push_stringf(arena, "Note: %.*s (%d, %d) isn't initialized correctly!",
                                      string_expand(note->string), note->scope_range.min, note->scope_range.max);
        system_error_box((char*)string.str);
    }
    
    if (!filter || filter(note))
    {
        *noteCount += 1;
        String8 tag = _Long_PushNoteTag(app, arena, note);
        u64 size = note->string.size;
        u8* str = note->string.str;
        if (size && (str[size-1] == '\r' || str[size-1] == '\n'))
        --size;
        String8 parentName = note->parent ? note->parent->string : S8Lit("NULL");
        String8 string = push_stringf(arena, "[%.*s] %.*s { %.*s }", string_expand(parentName), size, str, string_expand(tag));
        string_list_push(arena, list, string);
    }
    
    Long_Index_IterateValidNoteInFile(child, note)
    _Long_PushNoteString(app, arena, list, noteCount, child, filter);
}

function void Long_WriteToBuffer(Application_Links* app, Arena* arena, String8 name, String8 string)
{
    View_ID view = get_next_view_after_active(app, Access_Always);
    Buffer_ID outBuffer = Long_CreateOrSwitchBuffer(app, name, view);
    Buffer_Insertion out = begin_buffer_insertion_at_buffered(app, outBuffer, 0, arena, KB(64));
    buffer_set_setting(app, outBuffer, BufferSetting_ReadOnly      , true);
    buffer_set_setting(app, outBuffer, BufferSetting_RecordsHistory, false);
    insert_string(&out, string);
    end_buffer_insertion(&out);
}

function void Long_WriteNoteTreeToFile(Application_Links* app, String8 name, NoteFilter* filter)
{
    Buffer_ID buffer = view_get_buffer(app, get_active_view(app, Access_Always), Access_Always);
    Scratch_Block scratch(app);
    String8List list = {};
    
    F4_Index_Lock();
    {
        int noteCount = 0;
        F4_Index_File* file = F4_Index_LookupFile(app, buffer);
        for (F4_Index_Note *note = file ? file->first_note : 0; note; note = note->next_sibling)
        _Long_PushNoteString(app, scratch, &list, &noteCount, note, filter);
        string_list_push(scratch, &list, push_stringf(scratch, "Note count: %d", noteCount));
    }
    F4_Index_Unlock();
    
    String8 string = string_list_flatten(scratch, list, 0, S8Lit("\n"), 0, 0);
    Long_WriteToBuffer(app, scratch, name, string);
}

CUSTOM_UI_COMMAND_SIG/*CUSTOM_COMMAND_SIG*/(long_write_to_file_all_definitions)
CUSTOM_DOC("Save all definitions in the hash table.")
{
    Long_WriteNoteTreeToFile(app, S8Lit("definitions.txt"), 0);
}

CUSTOM_UI_COMMAND_SIG/*CUSTOM_COMMAND_SIG*/(long_write_to_file_all_empty_scopes)
CUSTOM_DOC("Save all definitions in the hash table.")
{
    Long_WriteNoteTreeToFile(app, S8Lit("scopes.txt"), Long_Filter_Empty_Scopes);
}

CUSTOM_UI_COMMAND_SIG/*CUSTOM_COMMAND_SIG*/(long_write_to_file_all_declarations)
CUSTOM_DOC("Save all definitions in the hash table.")
{
    Long_WriteNoteTreeToFile(app, S8Lit("declarations.txt"), Long_Filter_Declarations);
}

CUSTOM_UI_COMMAND_SIG/*CUSTOM_COMMAND_SIG*/(long_write_to_file_all_identifiers)
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
                String8 tag = _Long_PushNoteTag(app, scratch, note);
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
    Long_WriteToBuffer(app, scratch, S8Lit("identifiers.txt"), string);
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
    auto_indent_range(app);
}

CUSTOM_COMMAND_SIG(long_select_current_line)
CUSTOM_DOC("Set the mark and cursor to the start and end of the current line")
{
    View_ID view = get_active_view(app, Access_Read);
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
    long_select_current_line(app);
    copy(app);
}

CUSTOM_COMMAND_SIG(long_cut_line)
CUSTOM_DOC("Cut the text in the current line onto the clipboard.")
{
    long_select_current_line(app);
    cut(app);
}

//~ NOTE(long): Move Commands

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
    current_view_scan_move(app, Scan_Forward, push_boundary_list(scratch, Long_Boundary));
}

CUSTOM_COMMAND_SIG(long_move_left_boundary)
CUSTOM_DOC("Seek left for the next beginning of a token.")
{
    Scratch_Block scratch(app);
    current_view_scan_move(app, Scan_Backward, push_boundary_list(scratch, Long_Boundary));
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
    current_view_scan_move(app, Scan_Forward, push_boundary_list(scratch, Long_Boundary_AlphaNumericCamel));
}

CUSTOM_COMMAND_SIG(long_move_left_alpha_numeric_or_camel_boundary)
CUSTOM_DOC("Seek left for boundary between alphanumeric characters or camel case word and non-alphanumeric characters.")
{
    Scratch_Block scratch(app);
    current_view_scan_move(app, Scan_Backward, push_boundary_list(scratch, Long_Boundary_AlphaNumericCamel));
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
    current_view_scan_move(app, Scan_Forward, push_boundary_list(scratch, Long_Boundary_FunctionAndType));
}

CUSTOM_COMMAND_SIG(long_move_to_prev_function_and_type)
CUSTOM_DOC("Seek left for the previous function or type in the buffer.")
{
    Scratch_Block scratch(app);
    current_view_scan_move(app, Scan_Backward, push_boundary_list(scratch, Long_Boundary_FunctionAndType));
}

function void Long_SelectScopeBeforePos(Application_Links *app, View_ID view, Buffer_ID buffer, i64 pos)
{
    Find_Nest_Flag flags = FindNest_Scope|FindNest_Balanced;
    Range_i64 range = {};
    if (find_nest_side(app, buffer, pos - 1, flags, Scan_Backward, NestDelim_Close, &range) &&
        find_nest_side(app, buffer, range.start - 1, flags, Scan_Backward, NestDelim_Open, &range.start))
    select_scope(app, view, range);
}

CUSTOM_COMMAND_SIG(long_select_prev_scope_after_current)
CUSTOM_DOC("If a scope is selected, find the first scope that starts before the selected scope. Otherwise, find the first scope that starts before the cursor.")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    i64 cursor_pos = view_get_cursor_pos(app, view);
    Range_i64 range = Ii64(cursor_pos, view_get_mark_pos(app, view));
    if (range_is_scope_selection(app, buffer, range))
    Long_SelectScopeBeforePos(app, view, buffer, range.min);
    else
        Long_SelectScopeBeforePos(app, view, buffer, cursor_pos);
}

CUSTOM_COMMAND_SIG(long_select_next_top_most_scope)
CUSTOM_DOC("Finds the first scope that starts after the cursor, then finds the top most scope that contains that scope.")
{
    select_surrounding_scope_maximal(app);
    select_next_scope_after_current(app);
}

//~ NOTE(long): Misc Commands

CUSTOM_COMMAND_SIG(reload_all_themes_default_folder)
CUSTOM_DOC("Clears and reloads all the theme files in the default theme folder.")
{
    clear_all_themes(app);
    load_themes_default_folder(app);
}

CUSTOM_COMMAND_SIG(long_macro_toggle_recording)
CUSTOM_DOC("Toggle macro recording")
{
    if (global_keyboard_macro_is_recording)
    keyboard_macro_finish_recording(app);
    else
        keyboard_macro_start_recording(app);
}

CUSTOM_COMMAND_SIG(long_open_matching_file_same_panel)
CUSTOM_DOC("If the current file is a *.cpp or *.h, attempts to open the corresponding *.h or *.cpp file in the same view.")
{
    View_ID view = get_active_view(app, Access_Always);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    Buffer_ID new_buffer = 0;
    if (get_cpp_matching_file(app, buffer, &new_buffer))
    Long_JumpToBuffer(app, view, new_buffer);
}

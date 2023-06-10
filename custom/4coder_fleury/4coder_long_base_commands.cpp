
//~ NOTE(long): Point Stack Commands

#define clamp_loop(x, size) ((x) < 0 ? ((size) - 1) : ((x) % (size)))

function void Long_ReadPointStack(Application_Links* app, i32 index, Buffer_ID* out_buffer, i64* out_pos)
{
    Point_Stack_Slot slot = point_stack.markers[index];
    Marker* marker = (Marker*)managed_object_get_pointer(app, slot.object);
    Scratch_Block scratch(app);
    
    if (buffer_exists(app, slot.buffer))
    {
        *out_buffer = slot.buffer;
        *out_pos = marker->pos;
    }
}

i32 long_point_stack_current = 0; // NOTE(long): This's actually one past the current point (like the top index)
/*
long_point_stack_current_
Long_point_stack_current
_long_point_stack_current
_Long_point_stack_current_
long_point_stack_current)
(long_point_stack_current;
*/

function void Long_PointStack_Push(Application_Links* app, Buffer_ID buffer, i64 pos)
{
    i32 size = ArrayCount(point_stack.markers);
    point_stack.top = long_point_stack_current;
    
    if (point_stack.bot != point_stack.top)
    {
        Buffer_ID top_buffer = 0; i64 top_pos = 0;
        Long_ReadPointStack(app, clamp_loop(point_stack.top - 1, size), &top_buffer, &top_pos);
        if (buffer == top_buffer && pos == top_pos)
            return;
    }
    
    // NOTE(long): Free and assign the new item to the top(current) position
    {
        Point_Stack_Slot* slot = &point_stack.markers[point_stack.top];
        slot->buffer = buffer;
        
        Managed_Object object = alloc_buffer_markers_on_buffer(app, buffer, 1, 0);
        Marker *marker = (Marker*)managed_object_get_pointer(app, object);
        marker->pos = pos;
        marker->lean_right = false;
        managed_object_free(app, slot->object);
        
        slot->object = object;
    }
    
    // NOTE(long): Increase the top (and bot if need to)
    i32 next_top = clamp_loop(point_stack.top + 1, size);
    point_stack.top = next_top;
    long_point_stack_current = next_top;
    if (next_top == point_stack.bot)
        point_stack.bot = clamp_loop(point_stack.bot + 1, size);
}

function void Long_JumpToLocation(Application_Links* app, View_ID view,
                                  Buffer_ID current_buffer, i64 current_pos, Buffer_ID target_buffer, i64 target_pos)
{
    Long_PointStack_Push(app, current_buffer, current_pos);
    F4_JumpToLocation(app, view, target_buffer, target_pos);
    Long_PointStack_Push(app, target_buffer, target_pos);
}

function void Long_JumpToLocation(Application_Links* app, View_ID view, Buffer_ID target_buffer, i64 target_pos)
{
    Long_JumpToLocation(app, view, view_get_buffer(app, view, Access_Always), view_get_cursor_pos(app, view), target_buffer, target_pos);
}

#define Long_IteratePointStack(app, start, end, advance, size, func) \
for (i32 i = start; i != end; i = clamp_loop(i + (advance), size)) \
{ \
Buffer_ID buffer = 0; \
i64 pos = 0; \
Long_ReadPointStack(app, i, &buffer, &pos); \
if (!buffer && !pos) continue; \
func; \
}

CUSTOM_COMMAND_SIG(long_list_all_jump_points)
CUSTOM_DOC("List jump history")
{
    Scratch_Block scratch(app);
    Lister_Block lister(app, scratch);
    lister_set_default_handlers(lister);
    
#define MAX_LISTER_NAME_SIZE 80
    Long_IteratePointStack(app, point_stack.bot, point_stack.top, 1, ArrayCount(point_stack.markers),
                           {
                               Tiny_Jump* jump = push_array(scratch, Tiny_Jump, 1);
                               jump->buffer = buffer;
                               jump->pos = pos;
                               
                               Token_Array tokens = get_token_array_from_buffer(app, buffer);
                               String8 filename = push_buffer_unique_name(app, scratch, buffer);
                               Buffer_Cursor cursor = buffer_compute_cursor(app, buffer, seek_pos(pos));
                               String8 string = {};
                               i64 original_size = 0;
                               {
                                   string = push_buffer_line(app, scratch, buffer, cursor.line);
                                   string = string_skip_chop_whitespace(string);
                                   original_size = string.size;
                                   string = string_prefix(string, MAX_LISTER_NAME_SIZE);
                               }
                               String8 name = push_stringf(scratch, "[%.*s](%d, %d) %.*s%s", string_expand(filename),
                                                           cursor.line, cursor.col, string_expand(string),
                                                           original_size > MAX_LISTER_NAME_SIZE ? " ..." : "");
                               String8 tag = (i == long_point_stack_current - 1) ? S8Lit("current") : String8{};
                               lister_add_item(lister, name, tag, jump, 0);
                           });
    
    Lister* stupidCPPLister = lister;
    lister_set_query(lister, push_u8_stringf(scratch, "Points: %d, Top: %d, Bot: %d, Current: %d", stupidCPPLister->options.count,
                                             point_stack.top, point_stack.bot, long_point_stack_current));
    Lister_Result l_result = run_lister(app, lister);
    
    Tiny_Jump result = {};
    if (!l_result.canceled && l_result.user_data != 0)
        block_copy_struct(&result, (Tiny_Jump*)l_result.user_data);
    if (result.buffer != 0)
    {
        F4_JumpToLocation(app, get_this_ctx_view(app, Access_Always), result.buffer, result.pos);
        long_push_new_jump(app);
    }
}

#define Long_GetFirstUniquePoint(app, start, end, advance, each, post) \
do { \
View_ID view = get_active_view(app, Access_Visible); \
if (view != 0) \
{ \
Buffer_ID current_buffer = view_get_buffer(app, view, Access_Always); \
i64 current_pos = view_get_cursor_pos(app, view); \
Long_IteratePointStack(app, start, end, advance, ArrayCount(point_stack.markers), { each; }); \
post; \
} \
} while (0)

function void Long_JumpToNextPointStack(Application_Links* app, i32 start, i32 end, i32 advance)
{
    if (point_stack.top == point_stack.bot)
        return;
    
    i32 size = ArrayCount(point_stack.markers);
    start = clamp_loop(start, size);
    end = clamp_loop(end, size);
    
    Long_GetFirstUniquePoint(app, start, end, advance,
                             if (buffer != current_buffer || pos != current_pos)
                             {
                                 long_point_stack_current = clamp_loop(i + 1, size);
                                 view_set_buffer(app, view, buffer, 0);
                                 view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
                                 break;
                             }, 0);
}

CUSTOM_COMMAND_SIG(long_undo_jump)
CUSTOM_DOC("Read from the current point stack and jump there; if already there go to the previous option")
{
    Long_JumpToNextPointStack(app, long_point_stack_current - 1, point_stack.bot - 1, -1);
}

CUSTOM_COMMAND_SIG(long_redo_jump)
CUSTOM_DOC("Read from the current point stack and jump there; if already there go to the next option")
{
    Long_JumpToNextPointStack(app, long_point_stack_current - 1, point_stack.top - 0, +1);
}

CUSTOM_COMMAND_SIG(long_push_new_jump)
CUSTOM_DOC("Push the current position to the point stack; if the stack already contains the position then set it to current")
{
    i32 current = -1;
    Long_GetFirstUniquePoint(app, point_stack.bot, point_stack.top, 1,
                             if (buffer == current_buffer && pos == current_pos)
                             {
                                 current = i;
                                 long_point_stack_current = clamp_loop(current + 1, ArrayCount(point_stack.markers));
                                 break;
                             },
                             if (current == -1)
                             {
                                 long_point_stack_current = point_stack.top;
                                 Long_PointStack_Push(app, current_buffer, current_pos);
                             });
}

//~ NOTE(long): List/Search Commands

function void Long_JumpToBuffer(Application_Links* app, View_ID view, Buffer_ID buffer)
{
    Long_PointStack_Push(app, view_get_buffer(app, view, Access_Always), view_get_cursor_pos(app, view));
    view_set_buffer(app, view, buffer, 0);
    Long_PointStack_Push(app, buffer, view_get_cursor_pos(app, view));
}

CUSTOM_UI_COMMAND_SIG(long_interactive_switch_buffer)
CUSTOM_DOC("Interactively switch to an open buffer.")
{
    // NOTE(long): This function was copied from 4coder's interactive_switch_buffer and modified to call Long_JumpToBuffer
    Buffer_ID buffer = get_buffer_from_user(app, "Switch:");
    if (buffer != 0)
    {
        View_ID view = get_this_ctx_view(app, Access_Always);
        Long_JumpToBuffer(app, view, buffer);
    }
}

global Buffer_ID long_buffer_before_search = 0;

function void Long_KillBuffer(Application_Links* app, Buffer_ID buffer, View_ID view)
{
    Scratch_Block scratch(app);
    String8 name = push_buffer_unique_name(app, scratch, buffer);
    View_ID buffer_view = get_first_view_with_buffer(app, buffer);
    try_buffer_kill(app, buffer, view, 0);
    if (long_buffer_before_search && string_match(name, search_name))
    {
        view_set_buffer(app, buffer_view, long_buffer_before_search, 0);
        long_buffer_before_search = 0;
    }
}

CUSTOM_UI_COMMAND_SIG(long_interactive_kill_buffer)
CUSTOM_DOC("Interactively kill an open buffer.")
{
    View_ID view = get_this_ctx_view(app, Access_Always);
    Buffer_ID buffer = get_buffer_from_user(app, "Kill:");
    Long_KillBuffer(app, buffer, view);
}

CUSTOM_COMMAND_SIG(long_kill_buffer)
CUSTOM_DOC("Kills the current buffer.")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    Long_KillBuffer(app, buffer, view);
}

CUSTOM_COMMAND_SIG(long_if_read_only_goto_position)
CUSTOM_DOC("If the buffer in the active view is writable, inserts a character, otherwise performs goto_jump_at_cursor.")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    if (!buffer)
    {
        buffer = view_get_buffer(app, view, Access_ReadVisible);
        if (buffer)
        {
            Buffer_ID prev_buffer = get_buffer_next_looped(app, buffer, Access_Always);
            if (prev_buffer && !long_buffer_before_search)
                long_buffer_before_search = prev_buffer;
            goto_jump_at_cursor(app);
            lock_jump_buffer(app, buffer);
        }
    }
    else leave_current_input_unhandled(app);
}

function Buffer_ID Long_CreateOrSwitchBuffer(Application_Links *app, String_Const_u8 name_string, View_ID default_target_view)
{
    Buffer_ID search_buffer = get_buffer_by_name(app, name_string, Access_Always);
    if (search_buffer)
    {
        {
            View_ID target_view = get_first_view_with_buffer(app, search_buffer);
            if (!target_view)
            {
                target_view = default_target_view;
                long_buffer_before_search = view_get_buffer(app, target_view, Access_Always);
                // TODO(long): Don't save if the buffer's name starts with a *
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
        String8 name = push_buffer_unique_name(app, arena, buffer); name;
        
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
    Long_Search(app, dir, cursor, query_init, insensitive);
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
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    i64 pos = view_get_cursor_pos(app, view);
    {
        Token *token = get_token_from_pos(app, buffer, pos);
        if (token != 0 && token->size > 0 && token->kind != TokenBaseKind_Whitespace)
        {
            Token_Array array = get_token_array_from_buffer(app, buffer);
            note = Long_Index_LookupBestNote(app, buffer, &array, token, 1);
        }
    }
    
    // NOTE(long): This was copied from F4_GoToDefinition and modified so it calls Long_JumpToLocation
    if (note)
    {
        if(!same_panel)
        {
            view = get_next_view_looped_primary_panels(app, view, Access_Always);
            buffer = view_get_buffer(app, view, Access_Always);
            pos = view_get_cursor_pos(app, view);
        }
        
        // NOTE(long): The difference between F4_JumpToLocation and F4_GoToDefinition is:
        // 1. The first one scroll quickly to the target while the second one snap to the target instantly
        // (scroll.position.line_number/pixel_shift.y = scroll.target.line_number/pixel_shift.y)
        // 2. The first one call view_set_preferred_x (I don't really know what this function does)
        // For simplicity, I just just Long_JumpToLocation (which calls F4_JumpToLocation) for all jumping behaviors
        Long_JumpToLocation(app, view, buffer, pos, note->file->buffer, note->range.min);
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
            result = push_stringf(arena, "type [%s] [%s]",
                                  note->flags & F4_Index_NoteFlag_Prototype ? "prototype" : "def",
                                  note->flags & F4_Index_NoteFlag_SumType ? "sum" : "product");
        }break;
        
        case F4_Index_NoteKind_Function:
        {
            String8 type = S8Lit("function");
            if (note->base_range.min)
            {
                String8 string = push_buffer_range(app, arena, note->file->buffer, note->base_range);
                if (string_match(string, S8Lit("operator")))
                    type = S8Lit("operator");
            }
            else
            {
                if (!range_size(note->range)) type = S8Lit("lambda");
                else if (note->parent)
                {
                    if (note->parent->kind == F4_Index_NoteKind_Type)
                    {
                        if (string_match(note->string, note->parent->string))
                            type = S8Lit("constructor");
                    }
                    else if (note->parent->kind == F4_Index_NoteKind_Decl)
                    {
                        if (string_match(note->string, S8Lit("get")))
                            type = S8Lit("getter");
                        else if (string_match(note->string, S8Lit("set")))
                            type = S8Lit("setter");
                    }
                }
            }
            result = push_stringf(arena, "%s [%s]", type.str, note->flags & F4_Index_NoteFlag_Prototype ? "prototype" : "def");
        }break;
        
        case F4_Index_NoteKind_Macro:
        {
            result = S8Lit("macro");
        }break;
        
        case F4_Index_NoteKind_Constant:
        {
            result = S8Lit("constant");
        }break;
        
        case F4_Index_NoteKind_CommentTag:
        {
            result = S8Lit("comment tag");
        }break;
        
        case F4_Index_NoteKind_CommentToDo:
        {
            result = S8Lit("TODO");
        }break;
        
        case F4_Index_NoteKind_Decl:
        {
            String8 type = note->base_range.min ? push_buffer_range(app, arena, note->file->buffer, note->base_range) : S8Lit("NULL");
            String8 string = {};
            if (note->parent)
            {
                if (note->range.max < note->parent->scope_range.min)
                    string = S8Lit("argument");
                else if (note->parent->kind == F4_Index_NoteKind_Type)
                {
                    string = S8Lit("field");
                    if (range_size(note->scope_range) >= 2)
                    {
                        String8 body = push_buffer_range(app, arena, note->file->buffer, Ii64_size(note->scope_range.min, 2));
                        if (string_match(body, S8Lit("=>")) || body.str[0] == '{')
                            string = S8Lit("property");
                    }
                }
                else if (note->parent->kind == F4_Index_NoteKind_Function || note->parent->kind == F4_Index_NoteKind_Scope)
                    string = S8Lit("local");
            }
            else string = S8Lit("global");
            
            result = push_stringf(arena, "declaration [type: %.*s] [%s]", string_expand(type), string.str);
        } break;
        
        default: break;
    }
    return result;
}

typedef b32 NoteFilter(F4_Index_Note* note);

function b32 _Long_Filter_Scopes(F4_Index_Note* note)
{
    return note->kind != F4_Index_NoteKind_Scope;
}

function b32 _Long_Filter_Empty_Scopes(F4_Index_Note* note)
{
    return note->kind == F4_Index_NoteKind_Scope && note->first_child == 0;
}

function b32 _Long_Filter_Declarations(F4_Index_Note* note)
{
    return note->kind == F4_Index_NoteKind_Decl;
}

function b32 _Long_Filter_FunctionAndType(F4_Index_Note* note)
{
    return note->kind == F4_Index_NoteKind_Type || note->kind == F4_Index_NoteKind_Function;
}

function void _PushListerOptionRecursive(Application_Links* app, Arena* arena, Lister* lister, F4_Index_Note* note, NoteFilter* filter)
{
    if (!filter || filter(note))
    {
        Buffer_ID buffer = note->file->buffer;
        
        Tiny_Jump *jump = push_array(arena, Tiny_Jump, 1);
        jump->buffer = buffer;
        jump->pos = note->range.first;
        
        String_Const_u8 bufferName = push_buffer_unique_name(app, arena, buffer);
        String_Const_u8 parentName = note->parent ? note->parent->string : S8Lit("NULL");
        String_Const_u8 name = push_stringf(arena, "[%.*s][%.*s](%d, %d) %.*s", string_expand(bufferName), string_expand(parentName),
                                            note->scope_range.min, note->scope_range.max, string_expand(note->string));
        String_Const_u8 sort = _Long_PushNoteTag(app, arena, note);
        lister_add_item(lister, name, sort, jump, 0);
    }
    
    for (F4_Index_Note* child = note->first_child; child; child = child->next_sibling)
        _PushListerOptionRecursive(app, arena, lister, child, filter);
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
            F4_Index_File *file = F4_Index_LookupFile(app, buffer);
            for(F4_Index_Note *note = file ? file->first_note : 0; note; note = note->next_sibling)
                _PushListerOptionRecursive(app, scratch, lister, note, filter);
            if (!project_wide)
                break;
        }
    }
    F4_Index_Unlock();
    
    i32 option_count = ((Lister*)lister)->options.count;
    lister_set_query(lister, push_u8_stringf(scratch, "Index (%s): %d", project_wide ? "Project" : "File", option_count));
    Lister_Result l_result = run_lister(app, lister);
    Tiny_Jump result = {};
    if (!l_result.canceled && l_result.user_data != 0)
        block_copy_struct(&result, (Tiny_Jump*)l_result.user_data);
    if (result.buffer != 0)
        Long_JumpToLocation(app, view, result.buffer, result.pos);
}

CUSTOM_UI_COMMAND_SIG(long_search_for_definition__project_wide)
CUSTOM_DOC("List all definitions in the index and jump to the one selected by the user.")
{
    _Long_SearchDefinition(app, 0, 1);
}

CUSTOM_UI_COMMAND_SIG(long_search_for_definition__current_file)
CUSTOM_DOC("List all definitions in the current file and jump to the one selected by the user.")
{
    _Long_SearchDefinition(app, 0, 0);
}

CUSTOM_UI_COMMAND_SIG(long_search_for_all_definitions)
CUSTOM_DOC("List all definitions in the hash table.")
{
    _Long_SearchDefinition(app, _Long_Filter_Scopes, 0);
}

CUSTOM_UI_COMMAND_SIG(long_search_for_all_definitions_including_scope)
CUSTOM_DOC("List all definitions in the hash table.")
{
    _Long_SearchDefinition(app, 0, 0);
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
    
    for (F4_Index_Note* child = note->first_child; child; child = child->next_sibling)
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

CUSTOM_UI_COMMAND_SIG(long_write_to_file_all_definitions)
CUSTOM_DOC("Save all definitions in the hash table.")
{
    Long_WriteNoteTreeToFile(app, S8Lit("definitions.txt"), 0);
}

CUSTOM_UI_COMMAND_SIG(long_write_to_file_all_empty_scopes)
CUSTOM_DOC("Save all definitions in the hash table.")
{
    Long_WriteNoteTreeToFile(app, S8Lit("scopes.txt"), _Long_Filter_Empty_Scopes);
}

CUSTOM_UI_COMMAND_SIG(long_write_to_file_all_declarations)
CUSTOM_DOC("Save all definitions in the hash table.")
{
    Long_WriteNoteTreeToFile(app, S8Lit("declarations.txt"), _Long_Filter_Declarations);
}

CUSTOM_UI_COMMAND_SIG(long_write_to_file_all_identifiers)
CUSTOM_DOC("Save all identifiers in the hash table.")
{
    Buffer_ID buffer = view_get_buffer(app, get_active_view(app, Access_Always), Access_Always);
    Scratch_Block scratch(app);
    String8List list = {};
    Token_Array array = get_token_array_from_buffer(app, buffer);
    Token_Iterator_Array it = token_iterator(0, &array);
    
    u32 noteCount = 0;
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
                noteCount++;
            }
        }
        
        if (!token_it_inc_non_whitespace(&it))
            break;
    }
    
    string_list_push(scratch, &list, push_stringf(scratch, "Note count: %d", noteCount));
    String8 string = string_list_flatten(scratch, list, 0, S8Lit("\n"), 0, 0);
    Long_WriteToBuffer(app, scratch, S8Lit("identifiers.txt"), string);
}

#if 0
function b32 Long_IndentBuffer(Application_Links *app, Buffer_ID buffer, Range_i64 pos)
{
    ProfileScope(app, "[Long] Indent Buffer");
    Token_Array token_array = get_token_array_from_buffer(app, buffer);
    Token_Array *tokens = &token_array;
    
    b32 result = false;
    if (tokens->tokens != 0)
    {
        result = true;
        Scratch_Block scratch(app);
        f32 width = 0;
        Layout_Item_List list = layout_virt_indent_index_generic(app, scratch, buffer, pos, get_face_id(app, buffer), width);
        for (Layout_Item_Block* block = list.first; block; block = block->next)
        {
            Layout_Item* item = block->items;
            for (i32 i = 0; i < block->item_count; ++i, ++item)
            {
                //Vec2_f32 p = item->rect.p0 + shift_p;
            }
        }
    }
    
    return(result);
}

BUFFER_HOOK_SIG(Long_SaveFile)
{
    // buffer_id
    ProfileScope(app, "[Long] Save File");
    
    b32 auto_indent = def_get_config_b32(vars_save_string_lit("automatically_indent_text_on_save"));
    b32 is_virtual = def_get_config_b32(vars_save_string_lit("enable_virtual_whitespace"));
    if (auto_indent && is_virtual)
        Long_IndentBuffer(app, buffer_id, buffer_range(app, buffer_id));
    
    Managed_Scope scope = buffer_get_managed_scope(app, buffer_id);
    Line_Ending_Kind *eol = scope_attachment(app, scope, buffer_eol_setting,
                                             Line_Ending_Kind);
    switch (*eol){
        case LineEndingKind_LF:   { rewrite_lines_to_lf  (app, buffer_id); } break;
        case LineEndingKind_CRLF: { rewrite_lines_to_crlf(app, buffer_id); } break;
    }
    // no meaning for return
    return 0;
}
#endif

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

function F4_Index_Note* Long_Scan_Note(Scan_Direction direction, F4_Index_Note* note, NoteFilter* filter, i64 pos)
{
    F4_Index_Note* result = 0;
    
    if (!filter || filter(note))
    {
        switch (direction)
        {
            case Scan_Forward:  { if (note->range.min > pos) result = note; } break;
            case Scan_Backward: { if (note->range.min < pos) result = note; } break;
        }
    }
    
    for (F4_Index_Note* child = direction == Scan_Forward ? note->first_child : note->last_child; child;
         child = direction == Scan_Forward ? child->next_sibling : child->prev_sibling)
    {
        F4_Index_Note* scan_note = Long_Scan_Note(direction, child, filter, pos);
        if (scan_note)
        {
            if (direction == Scan_Backward || !result)
                result = scan_note;
            break;
        }
    }
    
    return result;
}

function i64 Long_Boundary_FunctionAndType(Application_Links *app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos)
{
    F4_Index_File* file = F4_Index_LookupFile(app, buffer);
    if (file)
    {
        for (F4_Index_Note* note = direction == Scan_Forward ? file->first_note : file->last_note; note;
             note = direction == Scan_Forward ? note->next_sibling : note->prev_sibling)
        {
            F4_Index_Note* scan_note = Long_Scan_Note(direction, note, _Long_Filter_FunctionAndType, pos);
            if (scan_note)
            {
                pos = scan_note->range.min;
                break;
            }
        }
    }
    return pos;
}

CUSTOM_COMMAND_SIG(long_move_to_next_function)
CUSTOM_DOC("Seek right for next function in the buffer.")
{
    Scratch_Block scratch(app);
    current_view_scan_move(app, Scan_Forward, push_boundary_list(scratch, Long_Boundary_FunctionAndType));
}

CUSTOM_COMMAND_SIG(long_move_to_prev_function)
CUSTOM_DOC("Seek left for previous function in the buffer.")
{
    Scratch_Block scratch(app);
    current_view_scan_move(app, Scan_Backward, push_boundary_list(scratch, Long_Boundary_FunctionAndType));
}

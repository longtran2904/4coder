
//~ NOTE(long): Bindings

// @COPYPASTA(long): setup_essential_mapping
function void Long_Binding_SetupEssential(Mapping* mapping, i64 global_id, i64 file_id, i64 code_id)
{
    if (!implicit_map_function)
        implicit_map_function = default_implicit_map;
    
    MappingScope();
    SelectMapping(mapping);
    
    SelectMap(global_id);
    BindCore(long_startup, CoreCode_Startup);
    BindCore(default_try_exit, CoreCode_TryExit);
    Bind(exit_4coder,          KeyCode_F4, KeyCode_Alt);
    BindMouseWheel(mouse_wheel_scroll);
    BindMouseWheel(mouse_wheel_change_face_size, KeyCode_Control);
    
    SelectMap(file_id);
    ParentMap(global_id);
    BindTextInput(write_text_input);
    BindMouse(click_set_cursor_and_mark, MouseCode_Left);
    BindMouseRelease(click_set_cursor, MouseCode_Left);
    BindCore(click_set_cursor_and_mark, CoreCode_ClickActivateView);
    BindMouseMove(click_set_cursor_if_lbutton);
    
    SelectMap(code_id);
    ParentMap(file_id);
    BindTextInput(long_write_text_and_auto_indent);
}

// @COPYPASTA(long): F4_SetDefaultBindings
function void Long_Binding_SetupDefault(Mapping* mapping, i64 global_id, i64 file_id, i64 code_id)
{
    MappingScope();
    SelectMapping(mapping);
    
    SelectMap(global_id);
    {
        Bind(long_change_active_panel,              KeyCode_Comma,  KeyCode_Control);
        Bind(long_change_active_panel_backwards,    KeyCode_Comma,  KeyCode_Control, KeyCode_Shift);
        Bind(project_go_to_root_directory,          KeyCode_H,      KeyCode_Alt);
        Bind(long_change_to_build_panel,            KeyCode_Period, KeyCode_Control);
        Bind(close_build_panel,                     KeyCode_Comma,  KeyCode_Alt);
        Bind(toggle_virtual_whitespace,             KeyCode_Equal,  KeyCode_Control);
        Bind(long_macro_toggle_recording,           KeyCode_U,      KeyCode_Control);
        Bind(keyboard_macro_replay,                 KeyCode_U,      KeyCode_Control, KeyCode_Shift);
        Bind(long_macro_repeat,                     KeyCode_U,      KeyCode_Control, KeyCode_Alt);
        Bind(long_toggle_compilation_expand,        KeyCode_Insert);
        Bind(long_toggle_panel_expand,              KeyCode_Insert, KeyCode_Control);
        Bind(long_toggle_panel_expand_big,          KeyCode_Insert, KeyCode_Control, KeyCode_Shift);
        Bind(toggle_filebar,                        KeyCode_B,      KeyCode_Alt);
        Bind(toggle_line_numbers,                   KeyCode_Equal,  KeyCode_Control, KeyCode_Shift);
        Bind(toggle_fullscreen,                     KeyCode_F11,    KeyCode_Control);
        Bind(interactive_new,                       KeyCode_N,      KeyCode_Control);
        Bind(long_setup_new_project,                KeyCode_N,      KeyCode_Control, KeyCode_Shift);
        Bind(interactive_open_or_new,               KeyCode_O,      KeyCode_Control);
        Bind(f4_open_project,                       KeyCode_O,      KeyCode_Control, KeyCode_Shift);
        Bind(long_interactive_kill_buffer,          KeyCode_K,      KeyCode_Control);
        Bind(long_interactive_switch_buffer,        KeyCode_I,      KeyCode_Control);
        Bind(save_all_dirty_buffers,                KeyCode_S,      KeyCode_Control, KeyCode_Shift);
        Bind(long_recent_files_menu,                KeyCode_V,      KeyCode_Alt);
        Bind(theme_lister,                          KeyCode_H,      KeyCode_Control, KeyCode_Shift);
        Bind(long_kill_search_buffer,               KeyCode_K,      KeyCode_Control, KeyCode_Alt);
        Bind(long_switch_to_search_buffer,          KeyCode_I,      KeyCode_Control, KeyCode_Alt);
        Bind(goto_next_jump,                        KeyCode_N,      KeyCode_Alt);
        Bind(goto_prev_jump,                        KeyCode_N,      KeyCode_Alt,     KeyCode_Shift);
        Bind(goto_first_jump,                       KeyCode_M,      KeyCode_Alt,     KeyCode_Shift);
        Bind(long_execute_any_cli,                  KeyCode_Z,      KeyCode_Alt);
        Bind(execute_previous_cli,                  KeyCode_Z,      KeyCode_Alt,     KeyCode_Shift);
        Bind(long_command_lister,                   KeyCode_X,      KeyCode_Alt);
        Bind(build_in_build_panel,                  KeyCode_M,      KeyCode_Alt);
        Bind(project_command_lister,                KeyCode_X,      KeyCode_Alt,     KeyCode_Shift);
        
        Bind(project_fkey_command, KeyCode_F1);
        Bind(project_fkey_command, KeyCode_F2);
        Bind(project_fkey_command, KeyCode_F3);
        Bind(project_fkey_command, KeyCode_F4);
        Bind(project_fkey_command, KeyCode_F5);
        Bind(project_fkey_command, KeyCode_F6);
        Bind(project_fkey_command, KeyCode_F7);
        Bind(project_fkey_command, KeyCode_F8);
        Bind(project_fkey_command, KeyCode_F9);
        Bind(project_fkey_command, KeyCode_F10);
        Bind(project_fkey_command, KeyCode_F11);
        Bind(project_fkey_command, KeyCode_F12);
        Bind(project_fkey_command, KeyCode_F13);
        Bind(project_fkey_command, KeyCode_F14);
        Bind(project_fkey_command, KeyCode_F15);
        Bind(project_fkey_command, KeyCode_F16);
        
        Bind(open_panel_vsplit,                        KeyCode_BackwardSlash, KeyCode_Control);
        Bind(open_panel_hsplit,                        KeyCode_Minus,         KeyCode_Control);
        Bind(close_panel,                              KeyCode_P,             KeyCode_Control, KeyCode_Shift);
        Bind(long_search_for_definition__project_wide, KeyCode_J,             KeyCode_Control);
        Bind(long_search_for_definition__current_file, KeyCode_J,             KeyCode_Control, KeyCode_Shift);
        Bind(long_undo_jump,                           KeyCode_J,             KeyCode_Alt);
        Bind(long_redo_jump,                           KeyCode_K,             KeyCode_Alt);
        Bind(long_push_new_jump,                       KeyCode_L,             KeyCode_Alt);
        Bind(long_point_lister,                        KeyCode_L,             KeyCode_Control);
    }
    
    SelectMap(file_id);
    ParentMap(global_id);
    {
        // NOTE(long): None => Characters
        //             Ctrl => Tokens
        //             Alt  => Alphanumeric/Camel
        // Shift binding will make notepad-mode unselectable
        Bind(delete_char,                                    KeyCode_Delete);
        Bind(long_delete_token_boundary,                     KeyCode_Delete,    KeyCode_Control);
        Bind(long_delete_alpha_numeric_or_camel_boundary,    KeyCode_Delete,    KeyCode_Alt);
        Bind(backspace_char,                                 KeyCode_Backspace);
        Bind(long_backspace_token_boundary,                  KeyCode_Backspace, KeyCode_Control);
        Bind(long_backspace_alpha_numeric_or_camel_boundary, KeyCode_Backspace, KeyCode_Alt);
        Bind(move_left,                                      KeyCode_Left);
        Bind(long_move_prev_word,                            KeyCode_Left,      KeyCode_Control);
        Bind(long_move_prev_alpha_numeric_or_camel_boundary, KeyCode_Left,      KeyCode_Alt);
        Bind(move_right,                                     KeyCode_Right);
        Bind(long_move_next_word,                            KeyCode_Right,     KeyCode_Control);
        Bind(long_move_next_alpha_numeric_or_camel_boundary, KeyCode_Right,     KeyCode_Alt);
        Bind(long_switch_move_side_mode,                     KeyCode_Minus,     KeyCode_Alt);
        Bind(move_up,                                        KeyCode_Up);
        Bind(move_down,                                      KeyCode_Down);
        Bind(move_line_up,                                   KeyCode_Up,        KeyCode_Alt);
        Bind(move_line_down,                                 KeyCode_Down,      KeyCode_Alt);
        Bind(move_up_to_blank_line_end,                      KeyCode_Up,        KeyCode_Control);
        Bind(move_down_to_blank_line_end,                    KeyCode_Down,      KeyCode_Control);
        Bind(long_move_up_token_occurrence,                  KeyCode_Up,        KeyCode_Control, KeyCode_Alt);
        Bind(long_move_down_token_occurrence,                KeyCode_Down,      KeyCode_Control, KeyCode_Alt);
        
        Bind(seek_end_of_line,                    KeyCode_End);
        Bind(f4_home_first_non_whitespace,        KeyCode_Home);
        Bind(page_up,                             KeyCode_PageUp);
        Bind(page_down,                           KeyCode_PageDown);
        Bind(goto_beginning_of_file,              KeyCode_PageUp,   KeyCode_Control);
        Bind(goto_end_of_file,                    KeyCode_PageDown, KeyCode_Control);
        Bind(long_move_to_prev_divider_comment,   KeyCode_PageUp,   KeyCode_Control, KeyCode_Shift);
        Bind(long_move_to_next_divider_comment,   KeyCode_PageDown, KeyCode_Control, KeyCode_Shift);
        Bind(long_move_to_prev_function_and_type, KeyCode_Up,       KeyCode_Control, KeyCode_Shift, KeyCode_Alt);
        Bind(long_move_to_next_function_and_type, KeyCode_Down,     KeyCode_Control, KeyCode_Shift, KeyCode_Alt);
        Bind(set_mark,                            KeyCode_Space,    KeyCode_Control);
        Bind(long_cursor_mark_swap,               KeyCode_Space,    KeyCode_Control, KeyCode_Shift);
        Bind(long_clean_whitespace_at_cursor,     KeyCode_B,        KeyCode_Control); // @RECONSIDER(long)
        
        // NOTE(long): Ctrl  => Current Buffer vs All Buffers
        //             Alt   => List vs Search/Query
        //             Shift => Sensitive-String vs Insensitive-Substring
        Bind(long_search,                                                      KeyCode_F, KeyCode_Control);
        Bind(long_reverse_search,                                              KeyCode_R, KeyCode_Control);
        Bind(long_search_identifier,                                           KeyCode_T, KeyCode_Control);
        Bind(goto_line,                                                        KeyCode_G, KeyCode_Control);
        Bind(long_search_case_sensitive,                                       KeyCode_F, KeyCode_Control, KeyCode_Shift);
        Bind(long_reverse_search_case_sensitive,                               KeyCode_R, KeyCode_Control, KeyCode_Shift);
        Bind(long_search_identifier_case_sensitive,                            KeyCode_T, KeyCode_Control, KeyCode_Shift);
        Bind(long_list_all_locations,                                          KeyCode_F, KeyCode_Alt,     KeyCode_Shift);
        Bind(long_list_all_substring_locations_case_insensitive,               KeyCode_F, KeyCode_Alt);
        Bind(long_list_all_locations_current_buffer,                           KeyCode_F, KeyCode_Alt,     KeyCode_Shift, KeyCode_Control);
        Bind(long_list_all_substring_locations_case_insensitive_current_buffer,KeyCode_F, KeyCode_Alt,     KeyCode_Control);
        Bind(long_list_all_locations_of_identifier,                            KeyCode_T, KeyCode_Alt,     KeyCode_Shift);
        Bind(long_list_all_substring_locations_of_identifier_case_insensitive, KeyCode_T, KeyCode_Alt);
        Bind(long_list_all_locations_of_identifier_current_buffer,             KeyCode_T, KeyCode_Alt,     KeyCode_Shift, KeyCode_Control);
        Bind(long_list_all_substring_locations_of_identifier_case_insensitive_current_buffer, KeyCode_T,   KeyCode_Alt,   KeyCode_Control);
        Bind(long_list_all_lines_in_range_non_white,                           KeyCode_R, KeyCode_Alt);
        Bind(long_list_all_lines_in_range,                                     KeyCode_R, KeyCode_Alt,     KeyCode_Shift);
        Bind(long_list_all_lines_in_range_seek_end_non_white,                  KeyCode_E, KeyCode_Alt);
        Bind(long_list_all_lines_in_range_seek_end,                            KeyCode_E, KeyCode_Alt,     KeyCode_Shift);
        Bind(long_list_all_lines_from_start_to_cursor_absolute,                KeyCode_C, KeyCode_Alt);
        Bind(long_list_all_lines_from_start_to_cursor_relative,                KeyCode_C, KeyCode_Alt,     KeyCode_Shift);
        
        Bind(replace_in_range,                             KeyCode_A,      KeyCode_Control);
        Bind(long_query_replace,                           KeyCode_Q,      KeyCode_Control);
        Bind(long_query_replace_identifier,                KeyCode_Q,      KeyCode_Control, KeyCode_Shift);
        Bind(long_query_replace_selection,                 KeyCode_Q,      KeyCode_Alt);
        Bind(copy,                                         KeyCode_C,      KeyCode_Control);
        Bind(long_copy_line,                               KeyCode_C,      KeyCode_Control, KeyCode_Shift);
        Bind(cut,                                          KeyCode_X,      KeyCode_Control);
        Bind(long_cut_line,                                KeyCode_X,      KeyCode_Control, KeyCode_Shift);
        Bind(long_paste_and_indent,                        KeyCode_V,      KeyCode_Control);
        Bind(long_paste_and_replace_range,                 KeyCode_V,      KeyCode_Control, KeyCode_Shift);
        Bind(long_select_current_line,                     KeyCode_A,      KeyCode_Control, KeyCode_Shift);
        Bind(long_delete_range,                            KeyCode_D,      KeyCode_Control);
        Bind(delete_line,                                  KeyCode_D,      KeyCode_Control, KeyCode_Shift);
        Bind(duplicate_line,                               KeyCode_D,      KeyCode_Alt);
        Bind(center_view,                                  KeyCode_E,      KeyCode_Control);
        Bind(left_adjust_view,                             KeyCode_E,      KeyCode_Control, KeyCode_Shift);
        Bind(long_kill_buffer,                             KeyCode_K,      KeyCode_Control, KeyCode_Shift);
        Bind(reopen,                                       KeyCode_O,      KeyCode_Alt,     KeyCode_Shift);
        Bind(save,                                         KeyCode_S,      KeyCode_Control);
        Bind(long_redo,                                    KeyCode_Y,      KeyCode_Control);
        Bind(long_undo,                                    KeyCode_Z,      KeyCode_Control);
        Bind(long_redo_same_pos,                           KeyCode_Y,      KeyCode_Control, KeyCode_Shift);
        Bind(long_undo_same_pos,                           KeyCode_Z,      KeyCode_Control, KeyCode_Shift);
        Bind(view_buffer_other_panel,                      KeyCode_W,      KeyCode_Control, KeyCode_Shift);
        Bind(swap_panels,                                  KeyCode_W,      KeyCode_Control);
        Bind(if_read_only_goto_position,                   KeyCode_Return);
        Bind(if_read_only_goto_position_same_panel,        KeyCode_Return,  KeyCode_Shift);
        Bind(long_jump_lister,                             KeyCode_Period, KeyCode_Control, KeyCode_Shift);
        Bind(long_history_lister,                          KeyCode_H,      KeyCode_Control);
        Bind(long_code_peek,                               KeyCode_Tick,   KeyCode_Control);
        Bind(long_toggle_pos_context,                      KeyCode_Tick,   KeyCode_Alt);
        Bind(long_switch_pos_context_option,               KeyCode_Equal,  KeyCode_Alt);
        Bind(long_switch_pos_context_draw_position,        KeyCode_Equal,  KeyCode_Alt,     KeyCode_Shift);
        Bind(long_go_to_definition,                        KeyCode_Return, KeyCode_Control);
        Bind(long_go_to_definition_same_panel,             KeyCode_Return, KeyCode_Control, KeyCode_Shift);
        Bind(f4_unindent,                                  KeyCode_Tab,    KeyCode_Shift);
    }
    
    SelectMap(code_id);
    ParentMap(file_id);
    {
        Bind(long_toggle_comment_selection,        KeyCode_Semicolon,    KeyCode_Control);
        Bind(long_autocomplete,                    KeyCode_Tab);
        Bind(long_indent_range,                    KeyCode_Tab,          KeyCode_Control);
        Bind(word_complete_drop_down,              KeyCode_Tab,          KeyCode_Control, KeyCode_Shift);
        Bind(select_prev_scope_absolute,           KeyCode_LeftBracket,  KeyCode_Control);
        Bind(select_next_scope_absolute,           KeyCode_RightBracket, KeyCode_Control);
        Bind(long_select_upper_scope,              KeyCode_LeftBracket,  KeyCode_Control, KeyCode_Shift);
        Bind(long_select_lower_scope,              KeyCode_RightBracket, KeyCode_Control, KeyCode_Shift);
        Bind(long_select_prev_scope_current_level, KeyCode_LeftBracket,  KeyCode_Control, KeyCode_Alt);
        Bind(long_select_next_scope_current_level, KeyCode_RightBracket, KeyCode_Control, KeyCode_Alt);
        Bind(long_select_surrounding_scope,        KeyCode_LeftBracket,  KeyCode_Alt,     KeyCode_Shift);
        Bind(open_matching_file_cpp,               KeyCode_W,            KeyCode_Alt);
        Bind(long_open_matching_file_same_panel,   KeyCode_W,            KeyCode_Alt,KeyCode_Shift);
    }
}

function void Long_Binding_MultiCursor(Mapping* mapping, i64 global_id, i64 file_id, i64 code_id)
{
    MC_register(exit_4coder,                     MC_Command_Global);
    MC_register(default_try_exit,                MC_Command_Global);
    MC_register(mouse_wheel_change_face_size,    MC_Command_Global);
    MC_register(swap_panels,                     MC_Command_Global);
    
    MC_register(copy,                            MC_Command_CursorCopy);
    MC_register(cut,                             MC_Command_CursorCopy);
    MC_register(long_paste_and_indent,           MC_Command_CursorPaste);
    MC_register(long_copy_line,                  MC_Command_CursorCopy);
    MC_register(long_cut_line,                   MC_Command_CursorCopy);
    MC_register(long_paste_and_replace_range,    MC_Command_CursorPaste);
    
    MC_register(write_text_input,                MC_Command_Cursor);
    MC_register(long_write_text_and_auto_indent, MC_Command_Cursor);
    
    MC_register(delete_char,                                    MC_Command_Cursor);
    MC_register(backspace_char,                                 MC_Command_Cursor);
    MC_register(move_up,                                        MC_Command_Cursor);
    MC_register(move_down,                                      MC_Command_Cursor);
    MC_register(move_left,                                      MC_Command_Cursor);
    MC_register(move_right,                                     MC_Command_Cursor);
    MC_register(long_delete_token_boundary,                     MC_Command_Cursor);
    MC_register(long_backspace_token_boundary,                  MC_Command_Cursor);
    MC_register(long_move_up_token_occurrence,                  MC_Command_Cursor);
    MC_register(long_move_down_token_occurrence,                MC_Command_Cursor);
    MC_register(long_move_prev_word,                            MC_Command_Cursor);
    MC_register(long_move_next_word,                            MC_Command_Cursor);
    MC_register(long_delete_alpha_numeric_or_camel_boundary,    MC_Command_Cursor);
    MC_register(long_backspace_alpha_numeric_or_camel_boundary, MC_Command_Cursor);
    MC_register(move_up_to_blank_line_end,                      MC_Command_Cursor);
    MC_register(move_down_to_blank_line_end,                    MC_Command_Cursor);
    MC_register(long_move_prev_alpha_numeric_or_camel_boundary, MC_Command_Cursor);
    MC_register(long_move_next_alpha_numeric_or_camel_boundary, MC_Command_Cursor);
    
    MC_register(if_read_only_goto_position,      MC_Command_Cursor);
    MC_register(seek_end_of_line,                MC_Command_Cursor);
    MC_register(f4_home_first_non_whitespace,    MC_Command_Cursor);
    MC_register(move_line_up,                    MC_Command_Cursor);
    MC_register(move_line_down,                  MC_Command_Cursor);
    MC_register(long_delete_range,               MC_Command_Cursor);
    MC_register(delete_line,                     MC_Command_Cursor);
    MC_register(long_cursor_mark_swap,           MC_Command_Cursor);
    MC_register(set_mark,                        MC_Command_Cursor);
    MC_register(long_toggle_comment_selection,   MC_Command_Cursor);
    MC_register(long_autocomplete,               MC_Command_Cursor);
    MC_register(f4_unindent,                     MC_Command_Cursor);
    MC_register(duplicate_line,                  MC_Command_Cursor);
    
    {
        MappingScope();
        SelectMapping(&framework_mapping);
        
        SelectMap(global_id);
        MC_Bind(MC_end_multi, KeyCode_Escape);
        
        SelectMap(file_id);
        ParentMap(global_id);
        Bind(MC_add_at_pos,         KeyCode_BackwardSlash, KeyCode_Control, KeyCode_Shift);
        Bind(MC_begin_multi,        KeyCode_Return,        KeyCode_Alt);
        Bind(MC_begin_multi_block,  KeyCode_L,             KeyCode_Control, KeyCode_Shift);
        
        MC_Bind(long_mc_up_trail,   KeyCode_Up,            KeyCode_Alt,     KeyCode_Shift);
        MC_Bind(long_mc_down_trail, KeyCode_Down,          KeyCode_Alt,     KeyCode_Shift);
    }
}

// @COPYPASTA(long): dynamic_binding_load_from_file
function b32 Long_Binding_LoadData(Application_Links* app, Mapping* mapping, String8 filename, String8 data)
{
    b32 result = 0;
    
    if (data.size)
    {
        Scratch_Block scratch(app);
        Long_Print_Errorf(app, "loading bindings: %.*s\n", string_expand(filename));
        
        Config* parsed = 0;
        {
            parsed = def_config_from_text(app, scratch, filename, data);
            String8 error_str = config_stringize_errors(app, scratch, parsed);
            if (error_str.size)
                Long_Print_Error(app, error_str);
        }
        
        if (parsed)
        {
            result = 1;
            
            Thread_Context* tctx = get_thread_context(app);
            mapping_release(tctx, mapping);
            mapping_init(tctx, mapping);
            MappingScope();
            SelectMapping(mapping);
            
            for (Config_Assignment *assignment = parsed->first; assignment != 0; assignment = assignment->next)
            {
                Config_LValue *l = assignment->l;
                if (l != 0 && l->index == 0)
                {
                    Config_Get_Result rvalue = config_evaluate_rvalue(parsed, assignment, assignment->r);
                    if (rvalue.type == ConfigRValueType_Compound)
                    {
                        String_Const_u8 map_name = l->identifier;
                        String_ID map_name_id = vars_save_string(map_name);
                        
                        SelectMap(map_name_id);
                        
                        Config_Compound *compound = rvalue.compound;
                        
                        Config_Get_Result_List list = typed_compound_array_reference_list(scratch, parsed, compound);
                        for (Config_Get_Result_Node *node = list.first; node != 0; node = node->next)
                        {
                            Config_Compound *src = node->result.compound;
                            String_Const_u8 cmd_string = {0};
                            String_Const_u8 key_string = {0};
                            String_Const_u8 mod_string[9] = {0};
                            
                            if (!config_compound_string_member(parsed, src, "cmd", 0, &cmd_string))
                            {
                                def_config_push_error(scratch, parsed, node->result.pos, "Command string is required in binding");
                                goto finish_map;
                            }
                            
                            if (!config_compound_string_member(parsed, src, "key", 1, &key_string))
                            {
                                def_config_push_error(scratch, parsed, node->result.pos, "Key string is required in binding");
                                goto finish_map;
                            }
                            
                            for (i32 mod_idx = 0; mod_idx < ArrayCount(mod_string); mod_idx += 1)
                            {
                                String_Const_u8 str = push_stringf(scratch, "mod_%i", mod_idx);
                                if (config_compound_string_member(parsed, src, str, 2 + mod_idx, &mod_string[mod_idx]))
                                {
                                    // NOTE(rjf): No-Op
                                }
                            }
                            
                            // NOTE(rjf): Map read in successfully.
                            {
                                // NOTE(rjf): Find command.
                                Command_Metadata *command = get_command_metadata_from_name(cmd_string);
                                
                                // NOTE(rjf): Find keycode.
                                Key_Code keycode = dynamic_binding_key_code_from_string(key_string);
                                
                                // NOTE(rjf): Find mods.
                                i32 mod_count = 0;
                                Key_Code mods[ArrayCount(mod_string)] = {0};
                                for (i32 i = 0; i < ArrayCount(mod_string); i += 1)
                                {
                                    if (mod_string[i].str)
                                    {
                                        mods[mod_count] = dynamic_binding_key_code_from_string(mod_string[i]);
                                        mod_count += 1;
                                    }
                                }
                                
                                if (keycode != 0 && command != 0)
                                {
                                    Input_Modifier_Set mods_set = { mods, mod_count, };
                                    map_set_binding(mapping, map, command->proc, InputEventKind_KeyStroke, keycode, &mods_set);
                                }
                                else
                                {
                                    def_config_push_error(scratch, parsed, node->result.pos,
                                                          (keycode != 0) ? (char*)"Invalid command" :
                                                          (command != 0) ? (char*)"Invalid key":
                                                          (char*)"Invalid command and key");
                                }
                            }
                            
                            finish_map:;
                        }
                        
                        
                        if (parsed->errors.first != 0)
                        {
                            String_Const_u8 error_str = config_stringize_errors(app, scratch, parsed);
                            if (error_str.size)
                                Long_Print_Error(app, error_str);
                        }
                    }
                }
            }
        }
    }
    
    return result;
}

//~ NOTE(long): Mapping

function Command_Map* Long_Mapping_GetMap(Application_Links* app, View_ID view, Mapping** mapping)
{
    View_Context ctx = view_current_context(app, view);
    Buffer_ID buffer = view_get_buffer(app, view, 0);
    *mapping = ctx.mapping;
    
    Command_Map* map = 0;
    Managed_Scope buffer_scope = buffer_get_managed_scope(app, buffer);
    Command_Map_ID* map_id = scope_attachment(app, buffer_scope, buffer_map_id, Command_Map_ID);
    map = mapping_get_map(*mapping, map_id ? *map_id : ctx.map_id);
    
    return map;
}

function Command_Metadata* Long_Mapping_GetMetadata(Mapping* mapping, Command_Map* map, User_Input* in)
{
    Command_Binding binding = map_get_binding_recursive(mapping, map, &in->event);
    Custom_Command_Function* custom = binding.custom;
    Command_Metadata* metadata = custom ? get_command_metadata(custom) : 0;
    return metadata;
}

function b32 Long_Mapping_HandleCommand(Application_Links* app, View_ID view, Mapping** mapping, Command_Map** map, User_Input* in)
{
    if (!*mapping || !*map)
        *map = Long_Mapping_GetMap(app, view, mapping);
    Command_Metadata* metadata = Long_Mapping_GetMetadata(*mapping, *map, in);
    
    b32 result = metadata && metadata->is_ui;
    if (result)
        view_enqueue_command_function(app, view, metadata->proc);
    else
        leave_current_input_unhandled(app);
    return result;
}

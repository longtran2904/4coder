#if !defined(META_PASS)
#define command_id(c) (fcoder_metacmd_ID_##c)
#define command_metadata(c) (&fcoder_metacmd_table[command_id(c)])
#define command_metadata_by_id(id) (&fcoder_metacmd_table[id])
#define command_one_past_last_id 432
#if defined(CUSTOM_COMMAND_SIG)
#define PROC_LINKS(x,y) x
#else
#define PROC_LINKS(x,y) y
#endif
#if defined(CUSTOM_COMMAND_SIG)
CUSTOM_COMMAND_SIG(allow_mouse);
CUSTOM_COMMAND_SIG(auto_indent_line_at_cursor);
CUSTOM_COMMAND_SIG(auto_indent_range);
CUSTOM_COMMAND_SIG(auto_indent_whole_file);
CUSTOM_COMMAND_SIG(backspace_alpha_numeric_boundary);
CUSTOM_COMMAND_SIG(backspace_char);
CUSTOM_COMMAND_SIG(basic_change_active_panel);
CUSTOM_COMMAND_SIG(begin_clipboard_collection_mode);
CUSTOM_COMMAND_SIG(build_in_build_panel);
CUSTOM_COMMAND_SIG(build_search);
CUSTOM_COMMAND_SIG(casey_clean_file_and_save);
CUSTOM_COMMAND_SIG(casey_delete_to_end_of_line);
CUSTOM_COMMAND_SIG(casey_demo_audio);
CUSTOM_COMMAND_SIG(casey_demo_audio_one_shot);
CUSTOM_COMMAND_SIG(casey_demo_audio_switch_panel);
CUSTOM_COMMAND_SIG(casey_find_matching_file);
CUSTOM_COMMAND_SIG(casey_go_to_code_peek);
CUSTOM_COMMAND_SIG(casey_newline_and_indent);
CUSTOM_COMMAND_SIG(casey_seek_beginning_of_line_and_tab);
CUSTOM_COMMAND_SIG(casey_switch_to_keybinding_0);
CUSTOM_COMMAND_SIG(casey_switch_to_keybinding_1);
CUSTOM_COMMAND_SIG(center_view);
CUSTOM_COMMAND_SIG(change_active_panel);
CUSTOM_COMMAND_SIG(change_active_panel_backwards);
CUSTOM_COMMAND_SIG(change_to_build_panel);
CUSTOM_COMMAND_SIG(clean_all_lines);
CUSTOM_COMMAND_SIG(clean_trailing_whitespace);
CUSTOM_COMMAND_SIG(clear_all_themes);
CUSTOM_COMMAND_SIG(clear_clipboard);
CUSTOM_COMMAND_SIG(click_set_cursor);
CUSTOM_COMMAND_SIG(click_set_cursor_and_mark);
CUSTOM_COMMAND_SIG(click_set_cursor_if_lbutton);
CUSTOM_COMMAND_SIG(click_set_mark);
CUSTOM_COMMAND_SIG(clipboard_record_clip);
CUSTOM_COMMAND_SIG(close_all_code);
CUSTOM_COMMAND_SIG(close_build_panel);
CUSTOM_COMMAND_SIG(close_panel);
CUSTOM_COMMAND_SIG(command_documentation);
CUSTOM_COMMAND_SIG(command_lister);
CUSTOM_COMMAND_SIG(comment_line);
CUSTOM_COMMAND_SIG(comment_line_toggle);
CUSTOM_COMMAND_SIG(copy);
CUSTOM_COMMAND_SIG(cursor_mark_swap);
CUSTOM_COMMAND_SIG(custom_api_documentation);
CUSTOM_COMMAND_SIG(cut);
CUSTOM_COMMAND_SIG(decrease_face_size);
CUSTOM_COMMAND_SIG(default_file_externally_modified);
CUSTOM_COMMAND_SIG(default_startup);
CUSTOM_COMMAND_SIG(default_try_exit);
CUSTOM_COMMAND_SIG(default_view_input_handler);
CUSTOM_COMMAND_SIG(delete_alpha_numeric_boundary);
CUSTOM_COMMAND_SIG(delete_char);
CUSTOM_COMMAND_SIG(delete_current_scope);
CUSTOM_COMMAND_SIG(delete_file_query);
CUSTOM_COMMAND_SIG(delete_line);
CUSTOM_COMMAND_SIG(delete_range);
CUSTOM_COMMAND_SIG(display_key_codes);
CUSTOM_COMMAND_SIG(display_text_input);
CUSTOM_COMMAND_SIG(double_backspace);
CUSTOM_COMMAND_SIG(duplicate_line);
CUSTOM_COMMAND_SIG(execute_any_cli);
CUSTOM_COMMAND_SIG(execute_previous_cli);
CUSTOM_COMMAND_SIG(exit_4coder);
CUSTOM_COMMAND_SIG(f4_autocomplete_or_indent);
CUSTOM_COMMAND_SIG(f4_backspace_alpha_numeric_or_camel_boundary);
CUSTOM_COMMAND_SIG(f4_backspace_token_boundary);
CUSTOM_COMMAND_SIG(f4_bump_to_column);
CUSTOM_COMMAND_SIG(f4_code_peek);
CUSTOM_COMMAND_SIG(f4_code_peek_clear);
CUSTOM_COMMAND_SIG(f4_code_peek_yank);
CUSTOM_COMMAND_SIG(f4_comment_selection);
CUSTOM_COMMAND_SIG(f4_delete_alpha_numeric_or_camel_boundary);
CUSTOM_COMMAND_SIG(f4_delete_token_boundary);
CUSTOM_COMMAND_SIG(f4_go_to_definition);
CUSTOM_COMMAND_SIG(f4_go_to_definition_same_panel);
CUSTOM_COMMAND_SIG(f4_home);
CUSTOM_COMMAND_SIG(f4_home_first_non_whitespace);
CUSTOM_COMMAND_SIG(f4_interactive_open_or_new_in_project);
CUSTOM_COMMAND_SIG(f4_leave_event_unhandled);
CUSTOM_COMMAND_SIG(f4_lego_buffer_place);
CUSTOM_COMMAND_SIG(f4_lego_click_store_token_1);
CUSTOM_COMMAND_SIG(f4_lego_click_store_token_2);
CUSTOM_COMMAND_SIG(f4_lego_store_line);
CUSTOM_COMMAND_SIG(f4_lego_store_range);
CUSTOM_COMMAND_SIG(f4_lego_store_token);
CUSTOM_COMMAND_SIG(f4_loc);
CUSTOM_COMMAND_SIG(f4_move_down_token_occurrence);
CUSTOM_COMMAND_SIG(f4_move_left);
CUSTOM_COMMAND_SIG(f4_move_left_token_boundary);
CUSTOM_COMMAND_SIG(f4_move_right);
CUSTOM_COMMAND_SIG(f4_move_right_token_boundary);
CUSTOM_COMMAND_SIG(f4_move_to_next_divider_comment);
CUSTOM_COMMAND_SIG(f4_move_to_prev_divider_comment);
CUSTOM_COMMAND_SIG(f4_move_up_token_occurrence);
CUSTOM_COMMAND_SIG(f4_open_project);
CUSTOM_COMMAND_SIG(f4_powermode_allow);
CUSTOM_COMMAND_SIG(f4_powermode_disallow);
CUSTOM_COMMAND_SIG(f4_recent_files_menu);
CUSTOM_COMMAND_SIG(f4_remedy_open_cursor);
CUSTOM_COMMAND_SIG(f4_reverse_search);
CUSTOM_COMMAND_SIG(f4_search);
CUSTOM_COMMAND_SIG(f4_search_for_definition__current_file);
CUSTOM_COMMAND_SIG(f4_search_for_definition__project_wide);
CUSTOM_COMMAND_SIG(f4_setup_new_project);
CUSTOM_COMMAND_SIG(f4_switch_syntax_option);
CUSTOM_COMMAND_SIG(f4_toggle_battery_saver);
CUSTOM_COMMAND_SIG(f4_toggle_compilation_expand);
CUSTOM_COMMAND_SIG(f4_toggle_enclosure_side);
CUSTOM_COMMAND_SIG(f4_uncomment_selection);
CUSTOM_COMMAND_SIG(f4_unindent);
CUSTOM_COMMAND_SIG(f4_write_text_and_auto_indent);
CUSTOM_COMMAND_SIG(f4_write_text_input);
CUSTOM_COMMAND_SIG(f4_write_zero_struct);
CUSTOM_COMMAND_SIG(fleury_go_to_definition);
CUSTOM_COMMAND_SIG(fleury_go_to_definition_same_panel);
CUSTOM_COMMAND_SIG(fleury_home);
CUSTOM_COMMAND_SIG(fleury_startup);
CUSTOM_COMMAND_SIG(fleury_toggle_battery_saver);
CUSTOM_COMMAND_SIG(fleury_toggle_compilation_expand);
CUSTOM_COMMAND_SIG(fleury_write_text_and_auto_indent);
CUSTOM_COMMAND_SIG(fleury_write_text_input);
CUSTOM_COMMAND_SIG(fleury_write_zero_struct);
CUSTOM_COMMAND_SIG(go_to_user_directory);
CUSTOM_COMMAND_SIG(goto_beginning_of_file);
CUSTOM_COMMAND_SIG(goto_end_of_file);
CUSTOM_COMMAND_SIG(goto_first_jump);
CUSTOM_COMMAND_SIG(goto_first_jump_same_panel_sticky);
CUSTOM_COMMAND_SIG(goto_jump_at_cursor);
CUSTOM_COMMAND_SIG(goto_jump_at_cursor_same_panel);
CUSTOM_COMMAND_SIG(goto_line);
CUSTOM_COMMAND_SIG(goto_next_jump);
CUSTOM_COMMAND_SIG(goto_next_jump_no_skips);
CUSTOM_COMMAND_SIG(goto_prev_jump);
CUSTOM_COMMAND_SIG(goto_prev_jump_no_skips);
CUSTOM_COMMAND_SIG(hide_filebar);
CUSTOM_COMMAND_SIG(hide_scrollbar);
CUSTOM_COMMAND_SIG(hit_sfx);
CUSTOM_COMMAND_SIG(hms_demo_tutorial);
CUSTOM_COMMAND_SIG(if0_off);
CUSTOM_COMMAND_SIG(if_read_only_goto_position);
CUSTOM_COMMAND_SIG(if_read_only_goto_position_same_panel);
CUSTOM_COMMAND_SIG(increase_face_size);
CUSTOM_COMMAND_SIG(interactive_kill_buffer);
CUSTOM_COMMAND_SIG(interactive_new);
CUSTOM_COMMAND_SIG(interactive_open);
CUSTOM_COMMAND_SIG(interactive_open_or_new);
CUSTOM_COMMAND_SIG(interactive_switch_buffer);
CUSTOM_COMMAND_SIG(jump_to_definition);
CUSTOM_COMMAND_SIG(jump_to_definition_at_cursor);
CUSTOM_COMMAND_SIG(jump_to_last_point);
CUSTOM_COMMAND_SIG(keyboard_macro_finish_recording);
CUSTOM_COMMAND_SIG(keyboard_macro_replay);
CUSTOM_COMMAND_SIG(keyboard_macro_start_recording);
CUSTOM_COMMAND_SIG(kill_buffer);
CUSTOM_COMMAND_SIG(kill_tutorial);
CUSTOM_COMMAND_SIG(left_adjust_view);
CUSTOM_COMMAND_SIG(list_all_functions_all_buffers);
CUSTOM_COMMAND_SIG(list_all_functions_all_buffers_lister);
CUSTOM_COMMAND_SIG(list_all_functions_current_buffer);
CUSTOM_COMMAND_SIG(list_all_functions_current_buffer_lister);
CUSTOM_COMMAND_SIG(list_all_locations);
CUSTOM_COMMAND_SIG(list_all_locations_case_insensitive);
CUSTOM_COMMAND_SIG(list_all_locations_of_identifier);
CUSTOM_COMMAND_SIG(list_all_locations_of_identifier_case_insensitive);
CUSTOM_COMMAND_SIG(list_all_locations_of_selection);
CUSTOM_COMMAND_SIG(list_all_locations_of_selection_case_insensitive);
CUSTOM_COMMAND_SIG(list_all_locations_of_type_definition);
CUSTOM_COMMAND_SIG(list_all_locations_of_type_definition_of_identifier);
CUSTOM_COMMAND_SIG(list_all_substring_locations);
CUSTOM_COMMAND_SIG(list_all_substring_locations_case_insensitive);
CUSTOM_COMMAND_SIG(load_project);
CUSTOM_COMMAND_SIG(load_theme_current_buffer);
CUSTOM_COMMAND_SIG(load_themes_default_folder);
CUSTOM_COMMAND_SIG(load_themes_hot_directory);
CUSTOM_COMMAND_SIG(long_clean_whitespace_at_cursor);
CUSTOM_COMMAND_SIG(long_command_lister);
CUSTOM_COMMAND_SIG(long_copy_line);
CUSTOM_COMMAND_SIG(long_copy_token);
CUSTOM_COMMAND_SIG(long_cut_line);
CUSTOM_COMMAND_SIG(long_cut_token);
CUSTOM_COMMAND_SIG(long_go_to_definition);
CUSTOM_COMMAND_SIG(long_go_to_definition_same_panel);
CUSTOM_COMMAND_SIG(long_history_lister);
CUSTOM_COMMAND_SIG(long_indent_range);
CUSTOM_COMMAND_SIG(long_indent_whole_file);
CUSTOM_COMMAND_SIG(long_interactive_kill_buffer);
CUSTOM_COMMAND_SIG(long_interactive_switch_buffer);
CUSTOM_COMMAND_SIG(long_jump_lister);
CUSTOM_COMMAND_SIG(long_kill_buffer);
CUSTOM_COMMAND_SIG(long_kill_search_buffer);
CUSTOM_COMMAND_SIG(long_list_all_lines_from_start_to_cursor_absolute);
CUSTOM_COMMAND_SIG(long_list_all_lines_from_start_to_cursor_relative);
CUSTOM_COMMAND_SIG(long_list_all_lines_in_range);
CUSTOM_COMMAND_SIG(long_list_all_lines_in_range_non_white);
CUSTOM_COMMAND_SIG(long_list_all_lines_in_range_seek_end);
CUSTOM_COMMAND_SIG(long_list_all_lines_in_range_seek_end_non_white);
CUSTOM_COMMAND_SIG(long_list_all_locations);
CUSTOM_COMMAND_SIG(long_list_all_locations_current_buffer);
CUSTOM_COMMAND_SIG(long_list_all_locations_of_identifier);
CUSTOM_COMMAND_SIG(long_list_all_locations_of_identifier_current_buffer);
CUSTOM_COMMAND_SIG(long_list_all_substring_locations_case_insensitive);
CUSTOM_COMMAND_SIG(long_list_all_substring_locations_case_insensitive_current_buffer);
CUSTOM_COMMAND_SIG(long_list_all_substring_locations_of_identifier_case_insensitive);
CUSTOM_COMMAND_SIG(long_list_all_substring_locations_of_identifier_case_insensitive_current_buffer);
CUSTOM_COMMAND_SIG(long_macro_repeat);
CUSTOM_COMMAND_SIG(long_macro_toggle_recording);
CUSTOM_COMMAND_SIG(long_move_down_token_occurrence);
CUSTOM_COMMAND_SIG(long_move_left_alpha_numeric_or_camel_boundary);
CUSTOM_COMMAND_SIG(long_move_left_boundary);
CUSTOM_COMMAND_SIG(long_move_right_alpha_numeric_or_camel_boundary);
CUSTOM_COMMAND_SIG(long_move_right_boundary);
CUSTOM_COMMAND_SIG(long_move_to_next_function_and_type);
CUSTOM_COMMAND_SIG(long_move_to_prev_function_and_type);
CUSTOM_COMMAND_SIG(long_move_up_token_occurrence);
CUSTOM_COMMAND_SIG(long_open_matching_file_same_panel);
CUSTOM_COMMAND_SIG(long_paste_and_indent);
CUSTOM_COMMAND_SIG(long_paste_and_replace_range);
CUSTOM_COMMAND_SIG(long_point_lister);
CUSTOM_COMMAND_SIG(long_push_new_jump);
CUSTOM_COMMAND_SIG(long_query_replace);
CUSTOM_COMMAND_SIG(long_query_replace_identifier);
CUSTOM_COMMAND_SIG(long_query_replace_selection);
CUSTOM_COMMAND_SIG(long_recent_files_menu);
CUSTOM_COMMAND_SIG(long_redo);
CUSTOM_COMMAND_SIG(long_redo_all_buffers);
CUSTOM_COMMAND_SIG(long_redo_jump);
CUSTOM_COMMAND_SIG(long_redo_same_pos);
CUSTOM_COMMAND_SIG(long_reload_all_themes_default_folder);
CUSTOM_COMMAND_SIG(long_reverse_search);
CUSTOM_COMMAND_SIG(long_reverse_search_case_sensitive);
CUSTOM_COMMAND_SIG(long_reverse_search_identifier);
CUSTOM_COMMAND_SIG(long_search);
CUSTOM_COMMAND_SIG(long_search_case_sensitive);
CUSTOM_COMMAND_SIG(long_search_for_definition__current_file);
CUSTOM_COMMAND_SIG(long_search_for_definition__project_wide);
CUSTOM_COMMAND_SIG(long_search_for_definition_no_filter__project_file);
CUSTOM_COMMAND_SIG(long_search_identifier);
CUSTOM_COMMAND_SIG(long_search_identifier_case_sensitive);
CUSTOM_COMMAND_SIG(long_select_current_line);
CUSTOM_COMMAND_SIG(long_select_current_token);
CUSTOM_COMMAND_SIG(long_select_lower_scope);
CUSTOM_COMMAND_SIG(long_select_next_scope_current_level);
CUSTOM_COMMAND_SIG(long_select_prev_scope_current_level);
CUSTOM_COMMAND_SIG(long_select_surrounding_scope);
CUSTOM_COMMAND_SIG(long_select_upper_scope);
CUSTOM_COMMAND_SIG(long_switch_pos_context_draw_position);
CUSTOM_COMMAND_SIG(long_switch_pos_context_option);
CUSTOM_COMMAND_SIG(long_switch_to_search_buffer);
CUSTOM_COMMAND_SIG(long_test_lister_render);
CUSTOM_COMMAND_SIG(long_theme_lister);
CUSTOM_COMMAND_SIG(long_toggle_compilation_expand);
CUSTOM_COMMAND_SIG(long_toggle_line_offset);
CUSTOM_COMMAND_SIG(long_toggle_panel_expand);
CUSTOM_COMMAND_SIG(long_toggle_pos_context);
CUSTOM_COMMAND_SIG(long_undo);
CUSTOM_COMMAND_SIG(long_undo_all_buffers);
CUSTOM_COMMAND_SIG(long_undo_jump);
CUSTOM_COMMAND_SIG(long_undo_same_pos);
CUSTOM_COMMAND_SIG(long_write_text_and_auto_indent);
CUSTOM_COMMAND_SIG(long_write_to_file_all_declarations);
CUSTOM_COMMAND_SIG(long_write_to_file_all_definitions);
CUSTOM_COMMAND_SIG(long_write_to_file_all_empty_scopes);
CUSTOM_COMMAND_SIG(long_write_to_file_all_identifiers);
CUSTOM_COMMAND_SIG(make_directory_query);
CUSTOM_COMMAND_SIG(miblo_decrement_basic);
CUSTOM_COMMAND_SIG(miblo_decrement_time_stamp);
CUSTOM_COMMAND_SIG(miblo_decrement_time_stamp_minute);
CUSTOM_COMMAND_SIG(miblo_increment_basic);
CUSTOM_COMMAND_SIG(miblo_increment_time_stamp);
CUSTOM_COMMAND_SIG(miblo_increment_time_stamp_minute);
CUSTOM_COMMAND_SIG(mouse_wheel_change_face_size);
CUSTOM_COMMAND_SIG(mouse_wheel_scroll);
CUSTOM_COMMAND_SIG(move_down);
CUSTOM_COMMAND_SIG(move_down_10);
CUSTOM_COMMAND_SIG(move_down_textual);
CUSTOM_COMMAND_SIG(move_down_to_blank_line);
CUSTOM_COMMAND_SIG(move_down_to_blank_line_end);
CUSTOM_COMMAND_SIG(move_down_to_blank_line_skip_whitespace);
CUSTOM_COMMAND_SIG(move_left);
CUSTOM_COMMAND_SIG(move_left_alpha_numeric_boundary);
CUSTOM_COMMAND_SIG(move_left_alpha_numeric_or_camel_boundary);
CUSTOM_COMMAND_SIG(move_left_token_boundary);
CUSTOM_COMMAND_SIG(move_left_whitespace_boundary);
CUSTOM_COMMAND_SIG(move_left_whitespace_or_token_boundary);
CUSTOM_COMMAND_SIG(move_line_down);
CUSTOM_COMMAND_SIG(move_line_up);
CUSTOM_COMMAND_SIG(move_right);
CUSTOM_COMMAND_SIG(move_right_alpha_numeric_boundary);
CUSTOM_COMMAND_SIG(move_right_alpha_numeric_or_camel_boundary);
CUSTOM_COMMAND_SIG(move_right_token_boundary);
CUSTOM_COMMAND_SIG(move_right_whitespace_boundary);
CUSTOM_COMMAND_SIG(move_right_whitespace_or_token_boundary);
CUSTOM_COMMAND_SIG(move_up);
CUSTOM_COMMAND_SIG(move_up_10);
CUSTOM_COMMAND_SIG(move_up_to_blank_line);
CUSTOM_COMMAND_SIG(move_up_to_blank_line_end);
CUSTOM_COMMAND_SIG(move_up_to_blank_line_skip_whitespace);
CUSTOM_COMMAND_SIG(multi_paste);
CUSTOM_COMMAND_SIG(multi_paste_interactive);
CUSTOM_COMMAND_SIG(multi_paste_interactive_quick);
CUSTOM_COMMAND_SIG(music_start);
CUSTOM_COMMAND_SIG(music_stop);
CUSTOM_COMMAND_SIG(open_all_code);
CUSTOM_COMMAND_SIG(open_all_code_recursive);
CUSTOM_COMMAND_SIG(open_file_in_quotes);
CUSTOM_COMMAND_SIG(open_in_other);
CUSTOM_COMMAND_SIG(open_long_braces);
CUSTOM_COMMAND_SIG(open_long_braces_break);
CUSTOM_COMMAND_SIG(open_long_braces_semicolon);
CUSTOM_COMMAND_SIG(open_matching_file_cpp);
CUSTOM_COMMAND_SIG(open_panel_hsplit);
CUSTOM_COMMAND_SIG(open_panel_vsplit);
CUSTOM_COMMAND_SIG(page_down);
CUSTOM_COMMAND_SIG(page_up);
CUSTOM_COMMAND_SIG(paste);
CUSTOM_COMMAND_SIG(paste_and_indent);
CUSTOM_COMMAND_SIG(paste_next);
CUSTOM_COMMAND_SIG(paste_next_and_indent);
CUSTOM_COMMAND_SIG(place_in_scope);
CUSTOM_COMMAND_SIG(play_with_a_counter);
CUSTOM_COMMAND_SIG(profile_clear);
CUSTOM_COMMAND_SIG(profile_disable);
CUSTOM_COMMAND_SIG(profile_enable);
CUSTOM_COMMAND_SIG(profile_inspect);
CUSTOM_COMMAND_SIG(project_command_F1);
CUSTOM_COMMAND_SIG(project_command_F10);
CUSTOM_COMMAND_SIG(project_command_F11);
CUSTOM_COMMAND_SIG(project_command_F12);
CUSTOM_COMMAND_SIG(project_command_F13);
CUSTOM_COMMAND_SIG(project_command_F14);
CUSTOM_COMMAND_SIG(project_command_F15);
CUSTOM_COMMAND_SIG(project_command_F16);
CUSTOM_COMMAND_SIG(project_command_F2);
CUSTOM_COMMAND_SIG(project_command_F3);
CUSTOM_COMMAND_SIG(project_command_F4);
CUSTOM_COMMAND_SIG(project_command_F5);
CUSTOM_COMMAND_SIG(project_command_F6);
CUSTOM_COMMAND_SIG(project_command_F7);
CUSTOM_COMMAND_SIG(project_command_F8);
CUSTOM_COMMAND_SIG(project_command_F9);
CUSTOM_COMMAND_SIG(project_command_lister);
CUSTOM_COMMAND_SIG(project_fkey_command);
CUSTOM_COMMAND_SIG(project_go_to_root_directory);
CUSTOM_COMMAND_SIG(project_reprint);
CUSTOM_COMMAND_SIG(query_replace);
CUSTOM_COMMAND_SIG(query_replace_identifier);
CUSTOM_COMMAND_SIG(query_replace_selection);
CUSTOM_COMMAND_SIG(quick_swap_buffer);
CUSTOM_COMMAND_SIG(redo);
CUSTOM_COMMAND_SIG(redo_all_buffers);
CUSTOM_COMMAND_SIG(rename_file_query);
CUSTOM_COMMAND_SIG(reopen);
CUSTOM_COMMAND_SIG(replace_in_all_buffers);
CUSTOM_COMMAND_SIG(replace_in_buffer);
CUSTOM_COMMAND_SIG(replace_in_range);
CUSTOM_COMMAND_SIG(reverse_search);
CUSTOM_COMMAND_SIG(reverse_search_identifier);
CUSTOM_COMMAND_SIG(save);
CUSTOM_COMMAND_SIG(save_all_dirty_buffers);
CUSTOM_COMMAND_SIG(save_to_query);
CUSTOM_COMMAND_SIG(search);
CUSTOM_COMMAND_SIG(search_identifier);
CUSTOM_COMMAND_SIG(seek_beginning_of_line);
CUSTOM_COMMAND_SIG(seek_beginning_of_textual_line);
CUSTOM_COMMAND_SIG(seek_end_of_line);
CUSTOM_COMMAND_SIG(seek_end_of_textual_line);
CUSTOM_COMMAND_SIG(select_all);
CUSTOM_COMMAND_SIG(select_next_scope_absolute);
CUSTOM_COMMAND_SIG(select_next_scope_after_current);
CUSTOM_COMMAND_SIG(select_prev_scope_absolute);
CUSTOM_COMMAND_SIG(select_prev_top_most_scope);
CUSTOM_COMMAND_SIG(select_surrounding_scope);
CUSTOM_COMMAND_SIG(select_surrounding_scope_maximal);
CUSTOM_COMMAND_SIG(set_eol_mode_from_contents);
CUSTOM_COMMAND_SIG(set_eol_mode_to_binary);
CUSTOM_COMMAND_SIG(set_eol_mode_to_crlf);
CUSTOM_COMMAND_SIG(set_eol_mode_to_lf);
CUSTOM_COMMAND_SIG(set_face_size);
CUSTOM_COMMAND_SIG(set_face_size_this_buffer);
CUSTOM_COMMAND_SIG(set_mark);
CUSTOM_COMMAND_SIG(set_mode_to_notepad_like);
CUSTOM_COMMAND_SIG(set_mode_to_original);
CUSTOM_COMMAND_SIG(setup_build_bat);
CUSTOM_COMMAND_SIG(setup_build_bat_and_sh);
CUSTOM_COMMAND_SIG(setup_build_sh);
CUSTOM_COMMAND_SIG(setup_new_project);
CUSTOM_COMMAND_SIG(show_filebar);
CUSTOM_COMMAND_SIG(show_scrollbar);
CUSTOM_COMMAND_SIG(show_the_log_graph);
CUSTOM_COMMAND_SIG(snipe_backward_whitespace_or_token_boundary);
CUSTOM_COMMAND_SIG(snipe_forward_whitespace_or_token_boundary);
CUSTOM_COMMAND_SIG(snippet_lister);
CUSTOM_COMMAND_SIG(string_repeat);
CUSTOM_COMMAND_SIG(suppress_mouse);
CUSTOM_COMMAND_SIG(swap_panels);
CUSTOM_COMMAND_SIG(switch_to_keybinding_0);
CUSTOM_COMMAND_SIG(switch_to_keybinding_1);
CUSTOM_COMMAND_SIG(switch_to_keybinding_2);
CUSTOM_COMMAND_SIG(switch_to_keybinding_3);
CUSTOM_COMMAND_SIG(theme_lister);
CUSTOM_COMMAND_SIG(to_lowercase);
CUSTOM_COMMAND_SIG(to_uppercase);
CUSTOM_COMMAND_SIG(toggle_command_server);
CUSTOM_COMMAND_SIG(toggle_filebar);
CUSTOM_COMMAND_SIG(toggle_fps_meter);
CUSTOM_COMMAND_SIG(toggle_fullscreen);
CUSTOM_COMMAND_SIG(toggle_highlight_enclosing_scopes);
CUSTOM_COMMAND_SIG(toggle_highlight_line_at_cursor);
CUSTOM_COMMAND_SIG(toggle_line_numbers);
CUSTOM_COMMAND_SIG(toggle_line_wrap);
CUSTOM_COMMAND_SIG(toggle_mouse);
CUSTOM_COMMAND_SIG(toggle_paren_matching_helper);
CUSTOM_COMMAND_SIG(toggle_show_whitespace);
CUSTOM_COMMAND_SIG(toggle_virtual_whitespace);
CUSTOM_COMMAND_SIG(tutorial_maximize);
CUSTOM_COMMAND_SIG(tutorial_minimize);
CUSTOM_COMMAND_SIG(uncomment_line);
CUSTOM_COMMAND_SIG(undo);
CUSTOM_COMMAND_SIG(undo_all_buffers);
CUSTOM_COMMAND_SIG(view_buffer_other_panel);
CUSTOM_COMMAND_SIG(view_jump_list_with_lister);
CUSTOM_COMMAND_SIG(word_complete);
CUSTOM_COMMAND_SIG(word_complete_drop_down);
CUSTOM_COMMAND_SIG(write_block);
CUSTOM_COMMAND_SIG(write_hack);
CUSTOM_COMMAND_SIG(write_note);
CUSTOM_COMMAND_SIG(write_space);
CUSTOM_COMMAND_SIG(write_text_and_auto_indent);
CUSTOM_COMMAND_SIG(write_text_input);
CUSTOM_COMMAND_SIG(write_todo);
CUSTOM_COMMAND_SIG(write_underscore);
CUSTOM_COMMAND_SIG(write_zero_struct);
#endif
struct Command_Metadata{
PROC_LINKS(Custom_Command_Function, void) *proc;
b32 is_ui;
char *name;
i32 name_len;
char *description;
i32 description_len;
char *source_name;
i32 source_name_len;
i32 line_number;
};
static Command_Metadata fcoder_metacmd_table[432] = {
{ PROC_LINKS(allow_mouse, 0), false, "allow_mouse", 11, "Shows the mouse and causes all mouse input to be processed normally.", 68, "D:\\Programs\\4coder\\custom\\4coder_default_framework.cpp", 54, 481 },
{ PROC_LINKS(auto_indent_line_at_cursor, 0), false, "auto_indent_line_at_cursor", 26, "Auto-indents the line on which the cursor sits.", 47, "D:\\Programs\\4coder\\custom\\4coder_auto_indent.cpp", 48, 423 },
{ PROC_LINKS(auto_indent_range, 0), false, "auto_indent_range", 17, "Auto-indents the range between the cursor and the mark.", 55, "D:\\Programs\\4coder\\custom\\4coder_auto_indent.cpp", 48, 433 },
{ PROC_LINKS(auto_indent_whole_file, 0), false, "auto_indent_whole_file", 22, "Audo-indents the entire current buffer.", 39, "D:\\Programs\\4coder\\custom\\4coder_auto_indent.cpp", 48, 414 },
{ PROC_LINKS(backspace_alpha_numeric_boundary, 0), false, "backspace_alpha_numeric_boundary", 32, "Delete characters between the cursor position and the first alphanumeric boundary to the left.", 94, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 154 },
{ PROC_LINKS(backspace_char, 0), false, "backspace_char", 14, "Deletes the character to the left of the cursor.", 48, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 96 },
{ PROC_LINKS(basic_change_active_panel, 0), false, "basic_change_active_panel", 25, "Change the currently active panel, moving to the panel with the next highest view_id.  Will not skipe the build panel if it is open.", 132, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 668 },
{ PROC_LINKS(begin_clipboard_collection_mode, 0), true, "begin_clipboard_collection_mode", 31, "Allows the user to copy multiple strings from other applications before switching to 4coder and pasting them all.", 113, "D:\\Programs\\4coder\\custom\\4coder_clipboard.cpp", 46, 71 },
{ PROC_LINKS(build_in_build_panel, 0), false, "build_in_build_panel", 20, "Looks for a build.bat, build.sh, or makefile in the current and parent directories.  Runs the first that it finds and prints the output to *compilation*.  Puts the *compilation* buffer in a panel at the footer of the current view.", 230, "D:\\Programs\\4coder\\custom\\4coder_build_commands.cpp", 51, 161 },
{ PROC_LINKS(build_search, 0), false, "build_search", 12, "Looks for a build.bat, build.sh, or makefile in the current and parent directories.  Runs the first that it finds and prints the output to *compilation*.", 153, "D:\\Programs\\4coder\\custom\\4coder_build_commands.cpp", 51, 124 },
{ PROC_LINKS(casey_clean_file_and_save, 0), false, "casey_clean_file_and_save", 25, "Standardizes line endings and tabs, then saves the active buffer.", 65, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_casey.cpp", 63, 42 },
{ PROC_LINKS(casey_delete_to_end_of_line, 0), false, "casey_delete_to_end_of_line", 27, "Deletes everything from the cursor to the end of the line.", 58, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_casey.cpp", 63, 88 },
{ PROC_LINKS(casey_demo_audio, 0), false, "casey_demo_audio", 16, "He used to cut my grass... he was a very nice boy!", 50, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_casey.cpp", 63, 5 },
{ PROC_LINKS(casey_demo_audio_one_shot, 0), false, "casey_demo_audio_one_shot", 25, "... if you gotta load, or if you gotta unload, you go to the white zone...", 74, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_casey.cpp", 63, 26 },
{ PROC_LINKS(casey_demo_audio_switch_panel, 0), false, "casey_demo_audio_switch_panel", 29, "The white zone is for loading and unloading only...", 51, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_casey.cpp", 63, 17 },
{ PROC_LINKS(casey_find_matching_file, 0), false, "casey_find_matching_file", 24, "If the current file is a *.cpp or *.h, attempts to open the corresponding *.h or *.cpp file in the same view.", 109, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_casey.cpp", 63, 116 },
{ PROC_LINKS(casey_go_to_code_peek, 0), false, "casey_go_to_code_peek", 21, "Jumps to the most likely thing you'd want to see for the identifier you're on.", 78, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_casey.cpp", 63, 127 },
{ PROC_LINKS(casey_newline_and_indent, 0), false, "casey_newline_and_indent", 24, "Inserts a newline at the cursor position and indent the next line automatically.", 80, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_casey.cpp", 63, 67 },
{ PROC_LINKS(casey_seek_beginning_of_line_and_tab, 0), false, "casey_seek_beginning_of_line_and_tab", 36, "Goes to the beginning of the line and indents the line with default indenting.", 78, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_casey.cpp", 63, 35 },
{ PROC_LINKS(casey_switch_to_keybinding_0, 0), false, "casey_switch_to_keybinding_0", 28, "WENSLEYDALE.", 12, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_casey.cpp", 63, 53 },
{ PROC_LINKS(casey_switch_to_keybinding_1, 0), false, "casey_switch_to_keybinding_1", 28, "STILTON.", 8, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_casey.cpp", 63, 60 },
{ PROC_LINKS(center_view, 0), false, "center_view", 11, "Centers the view vertically on the line on which the cursor sits.", 65, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 197 },
{ PROC_LINKS(change_active_panel, 0), false, "change_active_panel", 19, "Change the currently active panel, moving to the panel with the next highest view_id.", 85, "D:\\Programs\\4coder\\custom\\4coder_default_framework.cpp", 54, 356 },
{ PROC_LINKS(change_active_panel_backwards, 0), false, "change_active_panel_backwards", 29, "Change the currently active panel, moving to the panel with the next lowest view_id.", 84, "D:\\Programs\\4coder\\custom\\4coder_default_framework.cpp", 54, 362 },
{ PROC_LINKS(change_to_build_panel, 0), false, "change_to_build_panel", 21, "If the special build panel is open, makes the build panel the active panel.", 75, "D:\\Programs\\4coder\\custom\\4coder_build_commands.cpp", 51, 182 },
{ PROC_LINKS(clean_all_lines, 0), false, "clean_all_lines", 15, "Removes trailing whitespace from all lines and removes all blank lines in the current buffer.", 93, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 648 },
{ PROC_LINKS(clean_trailing_whitespace, 0), false, "clean_trailing_whitespace", 25, "Removes trailing whitespace from all lines in the current buffer.", 65, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 657 },
{ PROC_LINKS(clear_all_themes, 0), false, "clear_all_themes", 16, "Clear the theme list", 20, "D:\\Programs\\4coder\\custom\\4coder_default_framework.cpp", 54, 565 },
{ PROC_LINKS(clear_clipboard, 0), false, "clear_clipboard", 15, "Clears the history of the clipboard", 35, "D:\\Programs\\4coder\\custom\\4coder_clipboard.cpp", 46, 221 },
{ PROC_LINKS(click_set_cursor, 0), false, "click_set_cursor", 16, "Sets the cursor position to the mouse position.", 47, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 235 },
{ PROC_LINKS(click_set_cursor_and_mark, 0), false, "click_set_cursor_and_mark", 25, "Sets the cursor position and mark to the mouse position.", 56, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 225 },
{ PROC_LINKS(click_set_cursor_if_lbutton, 0), false, "click_set_cursor_if_lbutton", 27, "If the mouse left button is pressed, sets the cursor position to the mouse position.", 84, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 245 },
{ PROC_LINKS(click_set_mark, 0), false, "click_set_mark", 14, "Sets the mark position to the mouse position.", 45, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 258 },
{ PROC_LINKS(clipboard_record_clip, 0), false, "clipboard_record_clip", 21, "In response to a new clipboard contents events, saves the new clip onto the clipboard history", 93, "D:\\Programs\\4coder\\custom\\4coder_clipboard.cpp", 46, 7 },
{ PROC_LINKS(close_all_code, 0), false, "close_all_code", 14, "Closes any buffer with a filename ending with an extension configured to be recognized as a code file type.", 107, "D:\\Programs\\4coder\\custom\\4coder_project_commands.cpp", 53, 840 },
{ PROC_LINKS(close_build_panel, 0), false, "close_build_panel", 17, "If the special build panel is open, closes it.", 46, "D:\\Programs\\4coder\\custom\\4coder_build_commands.cpp", 51, 176 },
{ PROC_LINKS(close_panel, 0), false, "close_panel", 11, "Closes the currently active panel if it is not the only panel open.", 67, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 676 },
{ PROC_LINKS(command_documentation, 0), true, "command_documentation", 21, "Prompts the user to select a command then loads a doc buffer for that item", 74, "D:\\Programs\\4coder\\custom\\4coder_docs.cpp", 41, 190 },
{ PROC_LINKS(command_lister, 0), true, "command_lister", 14, "Opens an interactive list of all registered commands.", 53, "D:\\Programs\\4coder\\custom\\4coder_lists.cpp", 42, 761 },
{ PROC_LINKS(comment_line, 0), false, "comment_line", 12, "Insert '//' at the beginning of the line after leading whitespace.", 66, "D:\\Programs\\4coder\\custom\\4coder_combined_write_commands.cpp", 60, 125 },
{ PROC_LINKS(comment_line_toggle, 0), false, "comment_line_toggle", 19, "Turns uncommented lines into commented lines and vice versa for comments starting with '//'.", 92, "D:\\Programs\\4coder\\custom\\4coder_combined_write_commands.cpp", 60, 149 },
{ PROC_LINKS(copy, 0), false, "copy", 4, "Copy the text in the range from the cursor to the mark onto the clipboard.", 74, "D:\\Programs\\4coder\\custom\\4coder_clipboard.cpp", 46, 110 },
{ PROC_LINKS(cursor_mark_swap, 0), false, "cursor_mark_swap", 16, "Swaps the position of the cursor and the mark.", 46, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 124 },
{ PROC_LINKS(custom_api_documentation, 0), true, "custom_api_documentation", 24, "Prompts the user to select a Custom API item then loads a doc buffer for that item", 82, "D:\\Programs\\4coder\\custom\\4coder_docs.cpp", 41, 175 },
{ PROC_LINKS(cut, 0), false, "cut", 3, "Cut the text in the range from the cursor to the mark onto the clipboard.", 73, "D:\\Programs\\4coder\\custom\\4coder_clipboard.cpp", 46, 119 },
{ PROC_LINKS(decrease_face_size, 0), false, "decrease_face_size", 18, "Decrease the size of the face used by the current buffer.", 57, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 759 },
{ PROC_LINKS(default_file_externally_modified, 0), false, "default_file_externally_modified", 32, "Notes the external modification of attached files by printing a message.", 72, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 2067 },
{ PROC_LINKS(default_startup, 0), false, "default_startup", 15, "Default command for responding to a startup event", 49, "D:\\Programs\\4coder\\custom\\4coder_default_hooks.cpp", 50, 7 },
{ PROC_LINKS(default_try_exit, 0), false, "default_try_exit", 16, "Default command for responding to a try-exit event", 50, "D:\\Programs\\4coder\\custom\\4coder_default_hooks.cpp", 50, 33 },
{ PROC_LINKS(default_view_input_handler, 0), false, "default_view_input_handler", 26, "Input consumption loop for default view behavior", 48, "D:\\Programs\\4coder\\custom\\4coder_default_hooks.cpp", 50, 77 },
{ PROC_LINKS(delete_alpha_numeric_boundary, 0), false, "delete_alpha_numeric_boundary", 29, "Delete characters between the cursor position and the first alphanumeric boundary to the right.", 95, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 162 },
{ PROC_LINKS(delete_char, 0), false, "delete_char", 11, "Deletes the character to the right of the cursor.", 49, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 79 },
{ PROC_LINKS(delete_current_scope, 0), false, "delete_current_scope", 20, "Deletes the braces surrounding the currently selected scope.  Leaves the contents within the scope.", 99, "D:\\Programs\\4coder\\custom\\4coder_scope_commands.cpp", 51, 112 },
{ PROC_LINKS(delete_file_query, 0), false, "delete_file_query", 17, "Deletes the file of the current buffer if 4coder has the appropriate access rights. Will ask the user for confirmation first.", 125, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 1385 },
{ PROC_LINKS(delete_line, 0), false, "delete_line", 11, "Delete the line the on which the cursor sits.", 45, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 1557 },
{ PROC_LINKS(delete_range, 0), false, "delete_range", 12, "Deletes the text in the range between the cursor and the mark.", 62, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 134 },
{ PROC_LINKS(display_key_codes, 0), false, "display_key_codes", 17, "Example of input handling loop", 30, "D:\\Programs\\4coder\\custom\\4coder_examples.cpp", 45, 90 },
{ PROC_LINKS(display_text_input, 0), false, "display_text_input", 18, "Example of to_writable and leave_current_input_unhandled", 56, "D:\\Programs\\4coder\\custom\\4coder_examples.cpp", 45, 137 },
{ PROC_LINKS(double_backspace, 0), false, "double_backspace", 16, "Example of history group helpers", 32, "D:\\Programs\\4coder\\custom\\4coder_examples.cpp", 45, 10 },
{ PROC_LINKS(duplicate_line, 0), false, "duplicate_line", 14, "Create a copy of the line on which the cursor sits.", 51, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 1543 },
{ PROC_LINKS(execute_any_cli, 0), false, "execute_any_cli", 15, "Queries for an output buffer name and system command, runs the system command as a CLI and prints the output to the specified buffer.", 133, "D:\\Programs\\4coder\\custom\\4coder_cli_command.cpp", 48, 22 },
{ PROC_LINKS(execute_previous_cli, 0), false, "execute_previous_cli", 20, "If the command execute_any_cli has already been used, this will execute a CLI reusing the most recent buffer name and command.", 126, "D:\\Programs\\4coder\\custom\\4coder_cli_command.cpp", 48, 7 },
{ PROC_LINKS(exit_4coder, 0), false, "exit_4coder", 11, "Attempts to close 4coder.", 25, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 848 },
{ PROC_LINKS(f4_autocomplete_or_indent, 0), false, "f4_autocomplete_or_indent", 25, "Tries to autocomplete the word currently being typed, and inserts indentation if such a word is not found.", 106, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 1036 },
{ PROC_LINKS(f4_backspace_alpha_numeric_or_camel_boundary, 0), false, "f4_backspace_alpha_numeric_or_camel_boundary", 44, "Deletes left to a alphanumeric or camel boundary.", 49, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 826 },
{ PROC_LINKS(f4_backspace_token_boundary, 0), false, "f4_backspace_token_boundary", 27, "Deletes left to a token boundary.", 33, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 812 },
{ PROC_LINKS(f4_bump_to_column, 0), false, "f4_bump_to_column", 17, "Insert the required number of spaces to get to a specified column number.", 73, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 1711 },
{ PROC_LINKS(f4_code_peek, 0), false, "f4_code_peek", 12, "Toggles code peek.", 18, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_code_peek.cpp", 67, 125 },
{ PROC_LINKS(f4_code_peek_clear, 0), false, "f4_code_peek_clear", 18, "Clears the *peek* buffer.", 25, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_code_peek.cpp", 67, 145 },
{ PROC_LINKS(f4_code_peek_yank, 0), false, "f4_code_peek_yank", 17, "Yanks the current cursor identifier into the *peek* buffer.", 59, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_code_peek.cpp", 67, 131 },
{ PROC_LINKS(f4_comment_selection, 0), false, "f4_comment_selection", 20, "Performs VS-style commenting on the selected range.", 51, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 1488 },
{ PROC_LINKS(f4_delete_alpha_numeric_or_camel_boundary, 0), false, "f4_delete_alpha_numeric_or_camel_boundary", 41, "Deletes right to an alphanumeric or camel boundary.", 51, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 835 },
{ PROC_LINKS(f4_delete_token_boundary, 0), false, "f4_delete_token_boundary", 24, "Deletes right to a token boundary.", 34, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 819 },
{ PROC_LINKS(f4_go_to_definition, 0), false, "f4_go_to_definition", 19, "Goes to the definition of the identifier under the cursor.", 58, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 174 },
{ PROC_LINKS(f4_go_to_definition_same_panel, 0), false, "f4_go_to_definition_same_panel", 30, "Goes to the definition of the identifier under the cursor in the same panel.", 76, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 186 },
{ PROC_LINKS(f4_home, 0), false, "f4_home", 7, "Goes to the beginning of the line.", 34, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 68 },
{ PROC_LINKS(f4_home_first_non_whitespace, 0), false, "f4_home_first_non_whitespace", 28, "Goes to the beginning of the line.", 34, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 844 },
{ PROC_LINKS(f4_interactive_open_or_new_in_project, 0), true, "f4_interactive_open_or_new_in_project", 37, "Interactively open a file out of the file system, filtered to files only in the project.", 88, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 1243 },
{ PROC_LINKS(f4_leave_event_unhandled, 0), false, "f4_leave_event_unhandled", 24, "when bound to keystroke, ensures the event falls through to text insertion", 74, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 5 },
{ PROC_LINKS(f4_lego_buffer_place, 0), false, "f4_lego_buffer_place", 20, "Will place the lego, determined by the pressed F-key, at the cursor in the active buffer.", 89, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_lego.cpp", 62, 71 },
{ PROC_LINKS(f4_lego_click_store_token_1, 0), false, "f4_lego_click_store_token_1", 27, "Sets the cursor to the clicked position, and then stores the token under that position into the F1 slot.", 104, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_lego.cpp", 62, 149 },
{ PROC_LINKS(f4_lego_click_store_token_2, 0), false, "f4_lego_click_store_token_2", 27, "Sets the cursor to the clicked position, and then stores the token under that position into the F2 slot.", 104, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_lego.cpp", 62, 155 },
{ PROC_LINKS(f4_lego_store_line, 0), false, "f4_lego_store_line", 18, "Will store the selected range into the lego determined by the associated F-key.", 79, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_lego.cpp", 62, 116 },
{ PROC_LINKS(f4_lego_store_range, 0), false, "f4_lego_store_range", 19, "Will store the selected range into the lego determined by the associated F-key.", 79, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_lego.cpp", 62, 101 },
{ PROC_LINKS(f4_lego_store_token, 0), false, "f4_lego_store_token", 19, "Will store the token under the cursor into the lego determined by the associated F-key.", 87, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_lego.cpp", 62, 83 },
{ PROC_LINKS(f4_loc, 0), false, "f4_loc", 6, "Counts the lines of code in the current buffer, breaks it down by section, and outputs to the *loc* buffer.", 107, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 1626 },
{ PROC_LINKS(f4_move_down_token_occurrence, 0), false, "f4_move_down_token_occurrence", 29, "Moves the cursor to the next occurrence of the token that the cursor is over.", 77, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 791 },
{ PROC_LINKS(f4_move_left, 0), false, "f4_move_left", 12, "Moves the cursor one character to the left.", 43, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 756 },
{ PROC_LINKS(f4_move_left_token_boundary, 0), false, "f4_move_left_token_boundary", 27, "Seek left for boundary between alphanumeric characters and non-alphanumeric characters.", 87, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 805 },
{ PROC_LINKS(f4_move_right, 0), false, "f4_move_right", 13, "Moves the cursor one character to the right.", 44, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 770 },
{ PROC_LINKS(f4_move_right_token_boundary, 0), false, "f4_move_right_token_boundary", 28, "Seek right for boundary between alphanumeric characters and non-alphanumeric characters.", 88, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 798 },
{ PROC_LINKS(f4_move_to_next_divider_comment, 0), false, "f4_move_to_next_divider_comment", 31, "Seek right for next divider comment in the buffer.", 50, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_divider_comments.cpp", 74, 77 },
{ PROC_LINKS(f4_move_to_prev_divider_comment, 0), false, "f4_move_to_prev_divider_comment", 31, "Seek left for previous divider comment in the buffer.", 53, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_divider_comments.cpp", 74, 84 },
{ PROC_LINKS(f4_move_up_token_occurrence, 0), false, "f4_move_up_token_occurrence", 27, "Moves the cursor to the previous occurrence of the token that the cursor is over.", 81, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 784 },
{ PROC_LINKS(f4_open_project, 0), true, "f4_open_project", 15, "Open a project by navigating to the project file.", 49, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 410 },
{ PROC_LINKS(f4_powermode_allow, 0), false, "f4_powermode_allow", 18, "Allow power mode.", 17, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_power_mode.cpp", 68, 35 },
{ PROC_LINKS(f4_powermode_disallow, 0), false, "f4_powermode_disallow", 21, "Disallow power mode.", 20, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_power_mode.cpp", 68, 39 },
{ PROC_LINKS(f4_recent_files_menu, 0), true, "f4_recent_files_menu", 20, "Lists the recent files used in the current panel.", 49, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_recent_files.cpp", 70, 77 },
{ PROC_LINKS(f4_remedy_open_cursor, 0), false, "f4_remedy_open_cursor", 21, "Opens the active panel's file in an actively-running RemedyBG instance, and moves to the cursor's line position.", 112, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 1697 },
{ PROC_LINKS(f4_reverse_search, 0), false, "f4_reverse_search", 17, "Searches the current buffer backwards. If something is highlighted, will fill search query with it.", 99, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 34 },
{ PROC_LINKS(f4_search, 0), false, "f4_search", 9, "Searches the current buffer forward. If something is highlighted, will fill search query with it.", 97, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 28 },
{ PROC_LINKS(f4_search_for_definition__current_file, 0), true, "f4_search_for_definition__current_file", 38, "List all definitions in the current file and jump to the one selected by the user.", 82, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 313 },
{ PROC_LINKS(f4_search_for_definition__project_wide, 0), true, "f4_search_for_definition__project_wide", 38, "List all definitions in the index and jump to the one selected by the user.", 75, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 272 },
{ PROC_LINKS(f4_setup_new_project, 0), false, "f4_setup_new_project", 20, "Sets up a blank 4coder project provided some user folder.", 57, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 461 },
{ PROC_LINKS(f4_switch_syntax_option, 0), false, "f4_switch_syntax_option", 23, "Switches the syntax highlighting mode.", 38, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_colors.cpp", 64, 57 },
{ PROC_LINKS(f4_toggle_battery_saver, 0), false, "f4_toggle_battery_saver", 23, "Toggles battery saving mode.", 28, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 78 },
{ PROC_LINKS(f4_toggle_compilation_expand, 0), false, "f4_toggle_compilation_expand", 28, "Expand the compilation window.", 30, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 84 },
{ PROC_LINKS(f4_toggle_enclosure_side, 0), false, "f4_toggle_enclosure_side", 24, "Moves the cursor between the open/close brace/paren/bracket of the closest enclosure.", 85, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 353 },
{ PROC_LINKS(f4_uncomment_selection, 0), false, "f4_uncomment_selection", 22, "Performs VS-style uncommenting on the selected range.", 53, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 1504 },
{ PROC_LINKS(f4_unindent, 0), false, "f4_unindent", 11, "Unindent the selected range.", 28, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 1105 },
{ PROC_LINKS(f4_write_text_and_auto_indent, 0), false, "f4_write_text_and_auto_indent", 29, "Inserts text and auto-indents the line on which the cursor sits if any of the text contains 'layout punctuation' such as ;:{}()[]# and new lines.", 145, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 50 },
{ PROC_LINKS(f4_write_text_input, 0), false, "f4_write_text_input", 19, "Inserts whatever text was used to trigger this command.", 55, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 40 },
{ PROC_LINKS(f4_write_zero_struct, 0), false, "f4_write_zero_struct", 20, "At the cursor, insert a ' = {0};'.", 34, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 60 },
{ PROC_LINKS(fleury_go_to_definition, 0), false, "fleury_go_to_definition", 23, "Deprecated name. Please update to f4_go_to_definition.", 54, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 1762 },
{ PROC_LINKS(fleury_go_to_definition_same_panel, 0), false, "fleury_go_to_definition_same_panel", 34, "Deprecated name. Please update to f4_go_to_definition_same_panel.", 65, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 1765 },
{ PROC_LINKS(fleury_home, 0), false, "fleury_home", 11, "Deprecated name. Please update to f4_home.", 42, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 1753 },
{ PROC_LINKS(fleury_startup, 0), false, "fleury_startup", 14, "Fleury startup event", 20, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury.cpp", 57, 545 },
{ PROC_LINKS(fleury_toggle_battery_saver, 0), false, "fleury_toggle_battery_saver", 27, "Deprecated name. Please update to f4_toggle_battery_saver.", 58, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 1756 },
{ PROC_LINKS(fleury_toggle_compilation_expand, 0), false, "fleury_toggle_compilation_expand", 32, "Deprecated name. Please update to f4_toggle_compilation_expand.", 63, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 1759 },
{ PROC_LINKS(fleury_write_text_and_auto_indent, 0), false, "fleury_write_text_and_auto_indent", 33, "Deprecated name. Please update to f4_write_text_and_auto_indent.", 64, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 1747 },
{ PROC_LINKS(fleury_write_text_input, 0), false, "fleury_write_text_input", 23, "Deprecated name. Please update to f4_write_text_input.", 54, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 1744 },
{ PROC_LINKS(fleury_write_zero_struct, 0), false, "fleury_write_zero_struct", 24, "Deprecated name. Please update to f4_write_zero_struct.", 55, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_base_commands.cpp", 71, 1750 },
{ PROC_LINKS(go_to_user_directory, 0), false, "go_to_user_directory", 20, "Go to the 4coder user directory", 31, "D:\\Programs\\4coder\\custom\\4coder_config.cpp", 43, 1661 },
{ PROC_LINKS(goto_beginning_of_file, 0), false, "goto_beginning_of_file", 22, "Sets the cursor to the beginning of the file.", 45, "D:\\Programs\\4coder\\custom\\4coder_helper.cpp", 43, 2258 },
{ PROC_LINKS(goto_end_of_file, 0), false, "goto_end_of_file", 16, "Sets the cursor to the end of the file.", 39, "D:\\Programs\\4coder\\custom\\4coder_helper.cpp", 43, 2266 },
{ PROC_LINKS(goto_first_jump, 0), false, "goto_first_jump", 15, "If a buffer containing jump locations has been locked in, goes to the first jump in the buffer.", 95, "D:\\Programs\\4coder\\custom\\4coder_jump_sticky.cpp", 48, 528 },
{ PROC_LINKS(goto_first_jump_same_panel_sticky, 0), false, "goto_first_jump_same_panel_sticky", 33, "If a buffer containing jump locations has been locked in, goes to the first jump in the buffer and views the buffer in the panel where the jump list was.", 153, "D:\\Programs\\4coder\\custom\\4coder_jump_sticky.cpp", 48, 545 },
{ PROC_LINKS(goto_jump_at_cursor, 0), false, "goto_jump_at_cursor", 19, "If the cursor is found to be on a jump location, parses the jump location and brings up the file and position in another view and changes the active panel to the view containing the jump.", 187, "D:\\Programs\\4coder\\custom\\4coder_jump_sticky.cpp", 48, 348 },
{ PROC_LINKS(goto_jump_at_cursor_same_panel, 0), false, "goto_jump_at_cursor_same_panel", 30, "If the cursor is found to be on a jump location, parses the jump location and brings up the file and position in this view, losing the compilation output or jump list.", 167, "D:\\Programs\\4coder\\custom\\4coder_jump_sticky.cpp", 48, 375 },
{ PROC_LINKS(goto_line, 0), false, "goto_line", 9, "Queries the user for a number, and jumps the cursor to the corresponding line.", 78, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 856 },
{ PROC_LINKS(goto_next_jump, 0), false, "goto_next_jump", 14, "If a buffer containing jump locations has been locked in, goes to the next jump in the buffer, skipping sub jump locations.", 123, "D:\\Programs\\4coder\\custom\\4coder_jump_sticky.cpp", 48, 465 },
{ PROC_LINKS(goto_next_jump_no_skips, 0), false, "goto_next_jump_no_skips", 23, "If a buffer containing jump locations has been locked in, goes to the next jump in the buffer, and does not skip sub jump locations.", 132, "D:\\Programs\\4coder\\custom\\4coder_jump_sticky.cpp", 48, 497 },
{ PROC_LINKS(goto_prev_jump, 0), false, "goto_prev_jump", 14, "If a buffer containing jump locations has been locked in, goes to the previous jump in the buffer, skipping sub jump locations.", 127, "D:\\Programs\\4coder\\custom\\4coder_jump_sticky.cpp", 48, 483 },
{ PROC_LINKS(goto_prev_jump_no_skips, 0), false, "goto_prev_jump_no_skips", 23, "If a buffer containing jump locations has been locked in, goes to the previous jump in the buffer, and does not skip sub jump locations.", 136, "D:\\Programs\\4coder\\custom\\4coder_jump_sticky.cpp", 48, 514 },
{ PROC_LINKS(hide_filebar, 0), false, "hide_filebar", 12, "Sets the current view to hide it's filebar.", 43, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 706 },
{ PROC_LINKS(hide_scrollbar, 0), false, "hide_scrollbar", 14, "Sets the current view to hide it's scrollbar.", 45, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 692 },
{ PROC_LINKS(hit_sfx, 0), false, "hit_sfx", 7, "Play the hit sound effect", 25, "D:\\Programs\\4coder\\custom\\4coder_examples.cpp", 45, 240 },
{ PROC_LINKS(hms_demo_tutorial, 0), false, "hms_demo_tutorial", 17, "Tutorial for built in 4coder bindings and features.", 51, "D:\\Programs\\4coder\\custom\\4coder_tutorial.cpp", 45, 869 },
{ PROC_LINKS(if0_off, 0), false, "if0_off", 7, "Surround the range between the cursor and mark with an '#if 0' and an '#endif'", 78, "D:\\Programs\\4coder\\custom\\4coder_combined_write_commands.cpp", 60, 70 },
{ PROC_LINKS(if_read_only_goto_position, 0), false, "if_read_only_goto_position", 26, "If the buffer in the active view is writable, inserts a character, otherwise performs goto_jump_at_cursor.", 106, "D:\\Programs\\4coder\\custom\\4coder_jump_sticky.cpp", 48, 567 },
{ PROC_LINKS(if_read_only_goto_position_same_panel, 0), false, "if_read_only_goto_position_same_panel", 37, "If the buffer in the active view is writable, inserts a character, otherwise performs goto_jump_at_cursor_same_panel.", 117, "D:\\Programs\\4coder\\custom\\4coder_jump_sticky.cpp", 48, 584 },
{ PROC_LINKS(increase_face_size, 0), false, "increase_face_size", 18, "Increase the size of the face used by the current buffer.", 57, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 748 },
{ PROC_LINKS(interactive_kill_buffer, 0), true, "interactive_kill_buffer", 23, "Interactively kill an open buffer.", 34, "D:\\Programs\\4coder\\custom\\4coder_lists.cpp", 42, 521 },
{ PROC_LINKS(interactive_new, 0), true, "interactive_new", 15, "Interactively creates a new file.", 33, "D:\\Programs\\4coder\\custom\\4coder_lists.cpp", 42, 661 },
{ PROC_LINKS(interactive_open, 0), true, "interactive_open", 16, "Interactively opens a file.", 27, "D:\\Programs\\4coder\\custom\\4coder_lists.cpp", 42, 715 },
{ PROC_LINKS(interactive_open_or_new, 0), true, "interactive_open_or_new", 23, "Interactively open a file out of the file system.", 49, "D:\\Programs\\4coder\\custom\\4coder_lists.cpp", 42, 612 },
{ PROC_LINKS(interactive_switch_buffer, 0), true, "interactive_switch_buffer", 25, "Interactively switch to an open buffer.", 39, "D:\\Programs\\4coder\\custom\\4coder_lists.cpp", 42, 511 },
{ PROC_LINKS(jump_to_definition, 0), true, "jump_to_definition", 18, "List all definitions in the code index and jump to one chosen by the user.", 74, "D:\\Programs\\4coder\\custom\\4coder_code_index_listers.cpp", 55, 12 },
{ PROC_LINKS(jump_to_definition_at_cursor, 0), true, "jump_to_definition_at_cursor", 28, "Jump to the first definition in the code index matching an identifier at the cursor", 83, "D:\\Programs\\4coder\\custom\\4coder_code_index_listers.cpp", 55, 68 },
{ PROC_LINKS(jump_to_last_point, 0), false, "jump_to_last_point", 18, "Read from the top of the point stack and jump there; if already there pop the top and go to the next option", 107, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 1338 },
{ PROC_LINKS(keyboard_macro_finish_recording, 0), false, "keyboard_macro_finish_recording", 31, "Stop macro recording, do nothing if macro recording is not already started", 74, "D:\\Programs\\4coder\\custom\\4coder_keyboard_macro.cpp", 51, 54 },
{ PROC_LINKS(keyboard_macro_replay, 0), false, "keyboard_macro_replay", 21, "Replay the most recently recorded keyboard macro", 48, "D:\\Programs\\4coder\\custom\\4coder_keyboard_macro.cpp", 51, 77 },
{ PROC_LINKS(keyboard_macro_start_recording, 0), false, "keyboard_macro_start_recording", 30, "Start macro recording, do nothing if macro recording is already started", 71, "D:\\Programs\\4coder\\custom\\4coder_keyboard_macro.cpp", 51, 41 },
{ PROC_LINKS(kill_buffer, 0), false, "kill_buffer", 11, "Kills the current buffer.", 25, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 1728 },
{ PROC_LINKS(kill_tutorial, 0), false, "kill_tutorial", 13, "If there is an active tutorial, kill it.", 40, "D:\\Programs\\4coder\\custom\\4coder_tutorial.cpp", 45, 9 },
{ PROC_LINKS(left_adjust_view, 0), false, "left_adjust_view", 16, "Sets the left size of the view near the x position of the cursor.", 65, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 212 },
{ PROC_LINKS(list_all_functions_all_buffers, 0), false, "list_all_functions_all_buffers", 30, "Creates a jump list of lines from all buffers that appear to define or declare functions.", 89, "D:\\Programs\\4coder\\custom\\4coder_function_list.cpp", 50, 296 },
{ PROC_LINKS(list_all_functions_all_buffers_lister, 0), true, "list_all_functions_all_buffers_lister", 37, "Creates a lister of locations that look like function definitions and declarations all buffers.", 95, "D:\\Programs\\4coder\\custom\\4coder_function_list.cpp", 50, 302 },
{ PROC_LINKS(list_all_functions_current_buffer, 0), false, "list_all_functions_current_buffer", 33, "Creates a jump list of lines of the current buffer that appear to define or declare functions.", 94, "D:\\Programs\\4coder\\custom\\4coder_function_list.cpp", 50, 268 },
{ PROC_LINKS(list_all_functions_current_buffer_lister, 0), true, "list_all_functions_current_buffer_lister", 40, "Creates a lister of locations that look like function definitions and declarations in the buffer.", 97, "D:\\Programs\\4coder\\custom\\4coder_function_list.cpp", 50, 278 },
{ PROC_LINKS(list_all_locations, 0), false, "list_all_locations", 18, "Queries the user for a string and lists all exact case-sensitive matches found in all open buffers.", 99, "D:\\Programs\\4coder\\custom\\4coder_search.cpp", 43, 168 },
{ PROC_LINKS(list_all_locations_case_insensitive, 0), false, "list_all_locations_case_insensitive", 35, "Queries the user for a string and lists all exact case-insensitive matches found in all open buffers.", 101, "D:\\Programs\\4coder\\custom\\4coder_search.cpp", 43, 180 },
{ PROC_LINKS(list_all_locations_of_identifier, 0), false, "list_all_locations_of_identifier", 32, "Reads a token or word under the cursor and lists all exact case-sensitive mathces in all open buffers.", 102, "D:\\Programs\\4coder\\custom\\4coder_search.cpp", 43, 192 },
{ PROC_LINKS(list_all_locations_of_identifier_case_insensitive, 0), false, "list_all_locations_of_identifier_case_insensitive", 49, "Reads a token or word under the cursor and lists all exact case-insensitive mathces in all open buffers.", 104, "D:\\Programs\\4coder\\custom\\4coder_search.cpp", 43, 198 },
{ PROC_LINKS(list_all_locations_of_selection, 0), false, "list_all_locations_of_selection", 31, "Reads the string in the selected range and lists all exact case-sensitive mathces in all open buffers.", 102, "D:\\Programs\\4coder\\custom\\4coder_search.cpp", 43, 204 },
{ PROC_LINKS(list_all_locations_of_selection_case_insensitive, 0), false, "list_all_locations_of_selection_case_insensitive", 48, "Reads the string in the selected range and lists all exact case-insensitive mathces in all open buffers.", 104, "D:\\Programs\\4coder\\custom\\4coder_search.cpp", 43, 210 },
{ PROC_LINKS(list_all_locations_of_type_definition, 0), false, "list_all_locations_of_type_definition", 37, "Queries user for string, lists all locations of strings that appear to define a type whose name matches the input string.", 121, "D:\\Programs\\4coder\\custom\\4coder_search.cpp", 43, 216 },
{ PROC_LINKS(list_all_locations_of_type_definition_of_identifier, 0), false, "list_all_locations_of_type_definition_of_identifier", 51, "Reads a token or word under the cursor and lists all locations of strings that appear to define a type whose name matches it.", 125, "D:\\Programs\\4coder\\custom\\4coder_search.cpp", 43, 224 },
{ PROC_LINKS(list_all_substring_locations, 0), false, "list_all_substring_locations", 28, "Queries the user for a string and lists all case-sensitive substring matches found in all open buffers.", 103, "D:\\Programs\\4coder\\custom\\4coder_search.cpp", 43, 174 },
{ PROC_LINKS(list_all_substring_locations_case_insensitive, 0), false, "list_all_substring_locations_case_insensitive", 45, "Queries the user for a string and lists all case-insensitive substring matches found in all open buffers.", 105, "D:\\Programs\\4coder\\custom\\4coder_search.cpp", 43, 186 },
{ PROC_LINKS(load_project, 0), false, "load_project", 12, "Looks for a project.4coder file in the current directory and tries to load it.  Looks in parent directories until a project file is found or there are no more parents.", 167, "D:\\Programs\\4coder\\custom\\4coder_project_commands.cpp", 53, 867 },
{ PROC_LINKS(load_theme_current_buffer, 0), false, "load_theme_current_buffer", 25, "Parse the current buffer as a theme file and add the theme to the theme list. If the buffer has a .4coder postfix in it's name, it is removed when the name is saved.", 165, "D:\\Programs\\4coder\\custom\\4coder_config.cpp", 43, 1617 },
{ PROC_LINKS(load_themes_default_folder, 0), false, "load_themes_default_folder", 26, "Loads all the theme files in the default theme folder.", 54, "D:\\Programs\\4coder\\custom\\4coder_default_framework.cpp", 54, 535 },
{ PROC_LINKS(load_themes_hot_directory, 0), false, "load_themes_hot_directory", 25, "Loads all the theme files in the current hot directory.", 55, "D:\\Programs\\4coder\\custom\\4coder_default_framework.cpp", 54, 554 },
{ PROC_LINKS(long_clean_whitespace_at_cursor, 0), false, "long_clean_whitespace_at_cursor", 31, "Removes all trailing and leading whitespace at the cursor", 57, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 3045 },
{ PROC_LINKS(long_command_lister, 0), true, "long_command_lister", 19, "Opens an interactive list of all registered commands.", 53, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 3551 },
{ PROC_LINKS(long_copy_line, 0), false, "long_copy_line", 14, "Copy the text in the current line onto the clipboard.", 53, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 2959 },
{ PROC_LINKS(long_copy_token, 0), false, "long_copy_token", 15, "Copy the token that the cursor is currently on onto the clipboard.", 66, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 2973 },
{ PROC_LINKS(long_cut_line, 0), false, "long_cut_line", 13, "Cut the text in the current line onto the clipboard.", 52, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 2966 },
{ PROC_LINKS(long_cut_token, 0), false, "long_cut_token", 14, "Cut the token that the cursor is currently on onto the clipboard.", 65, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 2980 },
{ PROC_LINKS(long_go_to_definition, 0), false, "long_go_to_definition", 21, "Goes to the definition of the identifier under the cursor.", 58, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 2540 },
{ PROC_LINKS(long_go_to_definition_same_panel, 0), false, "long_go_to_definition_same_panel", 32, "Goes to the definition of the identifier under the cursor in the same panel.", 76, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 2546 },
{ PROC_LINKS(long_history_lister, 0), true, "long_history_lister", 19, "Opens an interactive list of the current buffer history.", 56, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 782 },
{ PROC_LINKS(long_indent_range, 0), false, "long_indent_range", 17, "Auto-indents the range between the cursor and the mark.", 55, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 1053 },
{ PROC_LINKS(long_indent_whole_file, 0), false, "long_indent_whole_file", 22, "Audo-indents the entire current buffer.", 39, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 1033 },
{ PROC_LINKS(long_interactive_kill_buffer, 0), true, "long_interactive_kill_buffer", 28, "Interactively kill an open buffer.", 34, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 730 },
{ PROC_LINKS(long_interactive_switch_buffer, 0), true, "long_interactive_switch_buffer", 30, "Interactively switch to an open buffer.", 39, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 701 },
{ PROC_LINKS(long_jump_lister, 0), true, "long_jump_lister", 16, "When executed on a buffer with jumps, creates a persistent lister for all the jumps", 83, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 620 },
{ PROC_LINKS(long_kill_buffer, 0), false, "long_kill_buffer", 16, "Kills the current buffer.", 25, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 1109 },
{ PROC_LINKS(long_kill_search_buffer, 0), false, "long_kill_search_buffer", 23, "Kills the current search jump buffer.", 37, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 1123 },
{ PROC_LINKS(long_list_all_lines_from_start_to_cursor_absolute, 0), false, "long_list_all_lines_from_start_to_cursor_absolute", 49, "Lists all the start-to-absolute-cursor range of all lines in the current buffer.", 80, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 1946 },
{ PROC_LINKS(long_list_all_lines_from_start_to_cursor_relative, 0), false, "long_list_all_lines_from_start_to_cursor_relative", 49, "Lists all the start-to-relative-cursor range of all lines in the current buffer.", 80, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 1940 },
{ PROC_LINKS(long_list_all_lines_in_range, 0), false, "long_list_all_lines_in_range", 28, "Lists all lines in between the mark and the cursor that are not blank.", 70, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 1903 },
{ PROC_LINKS(long_list_all_lines_in_range_non_white, 0), false, "long_list_all_lines_in_range_non_white", 38, "Lists all lines in between the mark and the cursor that are not blank.", 70, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 1909 },
{ PROC_LINKS(long_list_all_lines_in_range_seek_end, 0), false, "long_list_all_lines_in_range_seek_end", 37, "Lists all the end position of all lines in between the mark and the cursor.", 75, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 1915 },
{ PROC_LINKS(long_list_all_lines_in_range_seek_end_non_white, 0), false, "long_list_all_lines_in_range_seek_end_non_white", 47, "Lists all the end position of all lines in between the mark and the cursor.", 75, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 1921 },
{ PROC_LINKS(long_list_all_locations, 0), false, "long_list_all_locations", 23, "Queries the user for a string and lists all exact case-sensitive matches found in all open buffers.", 99, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 1809 },
{ PROC_LINKS(long_list_all_locations_current_buffer, 0), false, "long_list_all_locations_current_buffer", 38, "Queries the user for a string and lists all exact case-sensitive matches found in the current buffer.", 101, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 1821 },
{ PROC_LINKS(long_list_all_locations_of_identifier, 0), false, "long_list_all_locations_of_identifier", 37, "Reads a token or word under the cursor and lists all exact case-sensitive mathces in all open buffers.", 102, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 1833 },
{ PROC_LINKS(long_list_all_locations_of_identifier_current_buffer, 0), false, "long_list_all_locations_of_identifier_current_buffer", 52, "Reads a token or word under the cursor and lists all exact case-sensitive mathces in the current buffer.", 104, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 1845 },
{ PROC_LINKS(long_list_all_substring_locations_case_insensitive, 0), false, "long_list_all_substring_locations_case_insensitive", 50, "Queries the user for a string and lists all case-insensitive substring matches found in all open buffers.", 105, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 1815 },
{ PROC_LINKS(long_list_all_substring_locations_case_insensitive_current_buffer, 0), false, "long_list_all_substring_locations_case_insensitive_current_buffer", 65, "Queries the user for a string and lists all case-insensitive substring matches found in the current buffer.", 107, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 1827 },
{ PROC_LINKS(long_list_all_substring_locations_of_identifier_case_insensitive, 0), false, "long_list_all_substring_locations_of_identifier_case_insensitive", 64, "Reads a token or word under the cursor and lists all exact case-insensitive mathces in all open buffers.", 104, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 1839 },
{ PROC_LINKS(long_list_all_substring_locations_of_identifier_case_insensitive_current_buffer, 0), false, "long_list_all_substring_locations_of_identifier_case_insensitive_current_buffer", 79, "Reads a token or word under the cursor and lists all exact case-insensitive mathces in the current buffer.", 106, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 1851 },
{ PROC_LINKS(long_macro_repeat, 0), false, "long_macro_repeat", 17, "Repeat most recently recorded keyboard macro n times.", 53, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 3630 },
{ PROC_LINKS(long_macro_toggle_recording, 0), false, "long_macro_toggle_recording", 27, "Toggle macro recording", 22, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 3621 },
{ PROC_LINKS(long_move_down_token_occurrence, 0), false, "long_move_down_token_occurrence", 31, "Moves the cursor to the next occurrence of the token that the cursor is over.", 77, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 3332 },
{ PROC_LINKS(long_move_left_alpha_numeric_or_camel_boundary, 0), false, "long_move_left_alpha_numeric_or_camel_boundary", 46, "Seek left for boundary between alphanumeric characters or camel case word and non-alphanumeric characters.", 106, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 3142 },
{ PROC_LINKS(long_move_left_boundary, 0), false, "long_move_left_boundary", 23, "Seek left for the next beginning of a token.", 44, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 3106 },
{ PROC_LINKS(long_move_right_alpha_numeric_or_camel_boundary, 0), false, "long_move_right_alpha_numeric_or_camel_boundary", 47, "Seek right for boundary between alphanumeric characters or camel case word and non-alphanumeric characters.", 107, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 3135 },
{ PROC_LINKS(long_move_right_boundary, 0), false, "long_move_right_boundary", 24, "Seek right for the next end of a token.", 39, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 3099 },
{ PROC_LINKS(long_move_to_next_function_and_type, 0), false, "long_move_to_next_function_and_type", 35, "Seek right for the next function or type in the buffer.", 55, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 3195 },
{ PROC_LINKS(long_move_to_prev_function_and_type, 0), false, "long_move_to_prev_function_and_type", 35, "Seek left for the previous function or type in the buffer.", 58, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 3202 },
{ PROC_LINKS(long_move_up_token_occurrence, 0), false, "long_move_up_token_occurrence", 29, "Moves the cursor to the previous occurrence of the token that the cursor is over.", 81, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 3325 },
{ PROC_LINKS(long_open_matching_file_same_panel, 0), false, "long_open_matching_file_same_panel", 34, "If the current file is a *.cpp or *.h, attempts to open the corresponding *.h or *.cpp file in the same view.", 109, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 1129 },
{ PROC_LINKS(long_paste_and_indent, 0), false, "long_paste_and_indent", 21, "Paste from the top of clipboard and run auto-indent on the newly pasted text.", 77, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 2923 },
{ PROC_LINKS(long_paste_and_replace_range, 0), false, "long_paste_and_replace_range", 28, "replace the text between the mark and the cursor with the text from the top of the clipboard.", 93, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 2885 },
{ PROC_LINKS(long_point_lister, 0), true, "long_point_lister", 17, "List jump history", 17, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 435 },
{ PROC_LINKS(long_push_new_jump, 0), false, "long_push_new_jump", 18, "Push the current position to the point stack; if the stack's current is the position then ignore it", 99, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 523 },
{ PROC_LINKS(long_query_replace, 0), false, "long_query_replace", 18, "Queries the user for two strings, and incrementally replaces every occurence of the first string with the second string.", 120, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 2406 },
{ PROC_LINKS(long_query_replace_identifier, 0), false, "long_query_replace_identifier", 29, "Queries the user for a string, and incrementally replace every occurence of the word or token found at the cursor with the specified string.", 140, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 2427 },
{ PROC_LINKS(long_query_replace_selection, 0), false, "long_query_replace_selection", 28, "Queries the user for a string, and incrementally replace every occurence of the string found in the selected range with the specified string.", 141, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 2443 },
{ PROC_LINKS(long_recent_files_menu, 0), true, "long_recent_files_menu", 22, "Lists the recent files used in the current panel.", 49, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 662 },
{ PROC_LINKS(long_redo, 0), false, "long_redo", 9, "Advances forwards through the undo history of the current buffer.", 65, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 909 },
{ PROC_LINKS(long_redo_all_buffers, 0), false, "long_redo_all_buffers", 21, "Advances forward through the undo history in the buffer containing the most recent regular edit.", 96, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 945 },
{ PROC_LINKS(long_redo_jump, 0), false, "long_redo_jump", 14, "Read from the current point stack and jump there; if already there go to the next option", 88, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 517 },
{ PROC_LINKS(long_redo_same_pos, 0), false, "long_redo_same_pos", 18, "Advances forwards through the undo history of the current buffer but doesn't move the cursor.", 93, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 979 },
{ PROC_LINKS(long_reload_all_themes_default_folder, 0), false, "long_reload_all_themes_default_folder", 37, "Clears and reloads all the theme files in the default theme folder.", 67, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 3590 },
{ PROC_LINKS(long_reverse_search, 0), false, "long_reverse_search", 19, "Begins an incremental search up through the current buffer for a user specified string.", 87, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 2256 },
{ PROC_LINKS(long_reverse_search_case_sensitive, 0), false, "long_reverse_search_case_sensitive", 34, "Searches the current buffer backwards for the exact string. If something is highlighted, will fill search query with it.", 120, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 2280 },
{ PROC_LINKS(long_reverse_search_identifier, 0), false, "long_reverse_search_identifier", 30, "Begins an incremental search up through the current buffer for the word or token under the cursor.", 98, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 2268 },
{ PROC_LINKS(long_search, 0), false, "long_search", 11, "Begins an incremental search down through the current buffer for a user specified string.", 89, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 2250 },
{ PROC_LINKS(long_search_case_sensitive, 0), false, "long_search_case_sensitive", 26, "Searches the current buffer forward for the exact string. If something is highlighted, will fill search query with it.", 118, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 2274 },
{ PROC_LINKS(long_search_for_definition__current_file, 0), true, "long_search_for_definition__current_file", 40, "List all definitions in the current file and jump to the one selected by the user.", 82, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 2747 },
{ PROC_LINKS(long_search_for_definition__project_wide, 0), true, "long_search_for_definition__project_wide", 40, "List all definitions in the index and jump to the one selected by the user.", 75, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 2741 },
{ PROC_LINKS(long_search_for_definition_no_filter__project_file, 0), true, "long_search_for_definition_no_filter__project_file", 50, "List all definitions in the current file and jump to the one selected by the user.", 82, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 2753 },
{ PROC_LINKS(long_search_identifier, 0), false, "long_search_identifier", 22, "Begins an incremental search down through the current buffer for the word or token under the cursor.", 100, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 2262 },
{ PROC_LINKS(long_search_identifier_case_sensitive, 0), false, "long_search_identifier_case_sensitive", 37, "Begins an incremental search down through the current buffer for the exact word or token under the cursor.", 106, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 2286 },
{ PROC_LINKS(long_select_current_line, 0), false, "long_select_current_line", 24, "Set the mark to the end of the previous line and cursor to the end of the current line", 86, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 2990 },
{ PROC_LINKS(long_select_current_token, 0), false, "long_select_current_token", 25, "Set the mark and cursor to the current token's boundary.", 56, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 3007 },
{ PROC_LINKS(long_select_lower_scope, 0), false, "long_select_lower_scope", 23, "Find the first child scope that starts inside the current selected scope or surrounding scope.", 94, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 3464 },
{ PROC_LINKS(long_select_next_scope_current_level, 0), false, "long_select_next_scope_current_level", 36, "If a scope is selected, find the next scope that starts after the selected scope on the same level. Otherwise, find the first scope that starts before the cursor.", 162, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 3449 },
{ PROC_LINKS(long_select_prev_scope_current_level, 0), false, "long_select_prev_scope_current_level", 36, "If a scope is selected, find the prev scope that starts before the selected scope on the same level. Otherwise, find the first scope that starts before the cursor.", 163, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 3440 },
{ PROC_LINKS(long_select_surrounding_scope, 0), false, "long_select_surrounding_scope", 29, "Select the surrounding scope.", 29, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 3481 },
{ PROC_LINKS(long_select_upper_scope, 0), false, "long_select_upper_scope", 23, "Select the surrounding scope.", 29, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 3458 },
{ PROC_LINKS(long_switch_pos_context_draw_position, 0), false, "long_switch_pos_context_draw_position", 37, "Switches between drawing the position context at cursor position or at bottom of buffer.", 88, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 2473 },
{ PROC_LINKS(long_switch_pos_context_option, 0), false, "long_switch_pos_context_option", 30, "Switches the position context mode.", 35, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 2467 },
{ PROC_LINKS(long_switch_to_search_buffer, 0), false, "long_switch_to_search_buffer", 28, "Switch to the search buffer.", 28, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 1117 },
{ PROC_LINKS(long_test_lister_render, 0), false, "long_test_lister_render", 23, "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec placerat tellus vitae feugiat tincidunt. Suspendisse sagittis velit porttitor justo commodo sagittis. Etiam erat metus, elementum eu aliquam et, dictum at eros. Vivamus nulla ex, gravida malesuada iaculis id, maximus at quam. Fusce sodales, velit id varius rhoncus, odio ipsum placerat eros, nec euismod dui nunc in mauris. Donec quis commodo enim. Etiam sed efficitur elit, in interdum lacus. Vivamus sollicitudin hendrerit lacinia. Suspendisse aliquet bibendum nunc, eget fermentum quam feugiat ac. Sed at fringilla neque, eu aliquam risus. Donec ut ante eu erat cursus semper eget et velit. Quisque ut aliquam nibh. Curabitur et justo hendrerit, finibus sapien quis, fringilla ante. Nullam vehicula, nisi in facilisis egestas, tellus nunc faucibus lacus, tempor aliquet nulla felis sit amet felis. Nam non vulputate elit.\n\nVestibulum volutpat est vel felis tincidunt, sed imperdiet neque feugiat. Integer placerat dignissim eros, in sollicitudin lacus venenatis varius. Maecenas in feugiat ex. Nunc elementum sem est, sodales facilisis ligula hendrerit interdum. Integer pulvinar orci eget ipsum porta dapibus. Pellentesque sapien eros, semper sit amet placerat a, viverra malesuada nulla. Donec cursus turpis ut metus auctor pellentesque. Etiam dolor dui, maximus vitae malesuada ac, tincidunt eu tortor. Nullam felis ante, varius elementum mattis nec, pretium in diam.\n\nUt ut malesuada justo. Donec consequat magna sed diam feugiat pellentesque. Duis quis tempus tortor. Donec vulputate ullamcorper massa, eget porta metus ultrices nec. Cras dignissim dictum blandit. Nullam pellentesque volutpat purus vitae bibendum. Aenean quis neque eget orci imperdiet lacinia id finibus lorem. Interdum et malesuada fames ac ante ipsum primis in faucibus. Interdum et malesuada fames ac ante ipsum primis in faucibus. Praesent dictum lectus a ligula venenatis, nec ultricies turpis placerat. Proin euismod ut odio eu luctus. Vivamus eleifend eros sit amet felis dapibus, ac tempus est feugiat. Vivamus sit amet quam id lorem commodo volutpat. Maecenas ac nulla non turpis euismod vestibulum eget vitae tortor.\n\nMauris venenatis nunc ac enim fringilla, vitae varius neque imperdiet. Duis odio purus, commodo in dolor in, mollis malesuada tellus. Duis pharetra vulputate mauris ut cursus. Cras non eros feugiat, lacinia augue ut, tincidunt arcu. Donec pulvinar pulvinar lorem, vel sollicitudin arcu commodo ac. Sed facilisis lorem elit, sit amet dapibus urna varius elementum. Cras at viverra urna, eu vehicula ligula. Etiam ut convallis magna. Suspendisse feugiat quam sit amet accumsan aliquet. Pellentesque vestibulum sapien ut urna sollicitudin consequat. Duis non ullamcorper nibh.\n\nNullam hendrerit, sem et dictum faucibus, neque purus tristique ligula, eu sollicitudin arcu orci in magna. Vivamus auctor, enim varius ornare mattis, dolor magna condimentum enim, nec convallis sem velit nec augue. Pellentesque rutrum mauris ut nulla consectetur condimentum. Aliquam nec massa eu metus sollicitudin tincidunt. Donec lobortis ultricies sem id pretium. Donec quis felis vel ante fermentum pretium. Nulla at mi sit amet ex molestie imperdiet a eget lacus. Nullam rutrum aliquet tellus interdum bibendum.", 3269, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 3508 },
{ PROC_LINKS(long_theme_lister, 0), true, "long_theme_lister", 17, "Opens an interactive list of all registered themes.", 51, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 3609 },
{ PROC_LINKS(long_toggle_compilation_expand, 0), false, "long_toggle_compilation_expand", 30, "Expand the compilation window.", 30, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 1139 },
{ PROC_LINKS(long_toggle_line_offset, 0), false, "long_toggle_line_offset", 23, "Toggles between line numbers and offsets.", 41, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 324 },
{ PROC_LINKS(long_toggle_panel_expand, 0), false, "long_toggle_panel_expand", 24, "Expand the compilation window.", 30, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 1151 },
{ PROC_LINKS(long_toggle_pos_context, 0), false, "long_toggle_pos_context", 23, "Toggles position context window.", 32, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 2461 },
{ PROC_LINKS(long_undo, 0), false, "long_undo", 9, "Advances backwards through the undo history of the current buffer.", 66, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 856 },
{ PROC_LINKS(long_undo_all_buffers, 0), false, "long_undo_all_buffers", 21, "Advances backward through the undo history in the buffer containing the most recent regular edit.", 97, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 937 },
{ PROC_LINKS(long_undo_jump, 0), false, "long_undo_jump", 14, "Read from the current point stack and jump there; if already there go to the previous option", 92, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 511 },
{ PROC_LINKS(long_undo_same_pos, 0), false, "long_undo_same_pos", 18, "Advances backward through the undo history of the current buffer but doesn't move the cursor.", 93, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 973 },
{ PROC_LINKS(long_write_text_and_auto_indent, 0), false, "long_write_text_and_auto_indent", 31, "Inserts text and auto-indents the line on which the cursor sits if any of the text contains 'layout punctuation' such as ;:{}()[]# and new lines.", 145, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 1060 },
{ PROC_LINKS(long_write_to_file_all_declarations, 0), false, "long_write_to_file_all_declarations", 35, "Save all definitions in the hash table.", 39, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 2832 },
{ PROC_LINKS(long_write_to_file_all_definitions, 0), false, "long_write_to_file_all_definitions", 34, "Save all definitions in the hash table.", 39, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 2820 },
{ PROC_LINKS(long_write_to_file_all_empty_scopes, 0), false, "long_write_to_file_all_empty_scopes", 35, "Save all definitions in the hash table.", 39, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 2826 },
{ PROC_LINKS(long_write_to_file_all_identifiers, 0), false, "long_write_to_file_all_identifiers", 34, "Save all identifiers in the hash table.", 39, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_long_base_commands.cpp", 69, 2838 },
{ PROC_LINKS(make_directory_query, 0), false, "make_directory_query", 20, "Queries the user for a name and creates a new directory with the given name.", 76, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 1497 },
{ PROC_LINKS(miblo_decrement_basic, 0), false, "miblo_decrement_basic", 21, "Decrement an integer under the cursor by one.", 45, "D:\\Programs\\4coder\\custom\\4coder_miblo_numbers.cpp", 50, 44 },
{ PROC_LINKS(miblo_decrement_time_stamp, 0), false, "miblo_decrement_time_stamp", 26, "Decrement a time stamp under the cursor by one second. (format [m]m:ss or h:mm:ss", 81, "D:\\Programs\\4coder\\custom\\4coder_miblo_numbers.cpp", 50, 237 },
{ PROC_LINKS(miblo_decrement_time_stamp_minute, 0), false, "miblo_decrement_time_stamp_minute", 33, "Decrement a time stamp under the cursor by one minute. (format [m]m:ss or h:mm:ss", 81, "D:\\Programs\\4coder\\custom\\4coder_miblo_numbers.cpp", 50, 249 },
{ PROC_LINKS(miblo_increment_basic, 0), false, "miblo_increment_basic", 21, "Increment an integer under the cursor by one.", 45, "D:\\Programs\\4coder\\custom\\4coder_miblo_numbers.cpp", 50, 29 },
{ PROC_LINKS(miblo_increment_time_stamp, 0), false, "miblo_increment_time_stamp", 26, "Increment a time stamp under the cursor by one second. (format [m]m:ss or h:mm:ss", 81, "D:\\Programs\\4coder\\custom\\4coder_miblo_numbers.cpp", 50, 231 },
{ PROC_LINKS(miblo_increment_time_stamp_minute, 0), false, "miblo_increment_time_stamp_minute", 33, "Increment a time stamp under the cursor by one minute. (format [m]m:ss or h:mm:ss", 81, "D:\\Programs\\4coder\\custom\\4coder_miblo_numbers.cpp", 50, 243 },
{ PROC_LINKS(mouse_wheel_change_face_size, 0), false, "mouse_wheel_change_face_size", 28, "Reads the state of the mouse wheel and uses it to either increase or decrease the face size.", 92, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 801 },
{ PROC_LINKS(mouse_wheel_scroll, 0), false, "mouse_wheel_scroll", 18, "Reads the scroll wheel value from the mouse state and scrolls accordingly.", 74, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 268 },
{ PROC_LINKS(move_down, 0), false, "move_down", 9, "Moves the cursor down one line.", 31, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 342 },
{ PROC_LINKS(move_down_10, 0), false, "move_down_10", 12, "Moves the cursor down ten lines.", 32, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 354 },
{ PROC_LINKS(move_down_textual, 0), false, "move_down_textual", 17, "Moves down to the next line of actual text, regardless of line wrapping.", 72, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 360 },
{ PROC_LINKS(move_down_to_blank_line, 0), false, "move_down_to_blank_line", 23, "Seeks the cursor down to the next blank line.", 45, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 413 },
{ PROC_LINKS(move_down_to_blank_line_end, 0), false, "move_down_to_blank_line_end", 27, "Seeks the cursor down to the next blank line and places it at the end of the line.", 82, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 437 },
{ PROC_LINKS(move_down_to_blank_line_skip_whitespace, 0), false, "move_down_to_blank_line_skip_whitespace", 39, "Seeks the cursor down to the next blank line and places it at the end of the line.", 82, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 425 },
{ PROC_LINKS(move_left, 0), false, "move_left", 9, "Moves the cursor one character to the left.", 43, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 443 },
{ PROC_LINKS(move_left_alpha_numeric_boundary, 0), false, "move_left_alpha_numeric_boundary", 32, "Seek left for boundary between alphanumeric characters and non-alphanumeric characters.", 87, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 520 },
{ PROC_LINKS(move_left_alpha_numeric_or_camel_boundary, 0), false, "move_left_alpha_numeric_or_camel_boundary", 41, "Seek left for boundary between alphanumeric characters or camel case word and non-alphanumeric characters.", 106, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 534 },
{ PROC_LINKS(move_left_token_boundary, 0), false, "move_left_token_boundary", 24, "Seek left for the next beginning of a token.", 44, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 492 },
{ PROC_LINKS(move_left_whitespace_boundary, 0), false, "move_left_whitespace_boundary", 29, "Seek left for the next boundary between whitespace and non-whitespace.", 70, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 477 },
{ PROC_LINKS(move_left_whitespace_or_token_boundary, 0), false, "move_left_whitespace_or_token_boundary", 38, "Seek left for the next end of a token or boundary between whitespace and non-whitespace.", 88, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 506 },
{ PROC_LINKS(move_line_down, 0), false, "move_line_down", 14, "Swaps the line under the cursor with the line below it, and moves the cursor down with it.", 90, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 1537 },
{ PROC_LINKS(move_line_up, 0), false, "move_line_up", 12, "Swaps the line under the cursor with the line above it, and moves the cursor up with it.", 88, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 1531 },
{ PROC_LINKS(move_right, 0), false, "move_right", 10, "Moves the cursor one character to the right.", 44, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 451 },
{ PROC_LINKS(move_right_alpha_numeric_boundary, 0), false, "move_right_alpha_numeric_boundary", 33, "Seek right for boundary between alphanumeric characters and non-alphanumeric characters.", 88, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 513 },
{ PROC_LINKS(move_right_alpha_numeric_or_camel_boundary, 0), false, "move_right_alpha_numeric_or_camel_boundary", 42, "Seek right for boundary between alphanumeric characters or camel case word and non-alphanumeric characters.", 107, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 527 },
{ PROC_LINKS(move_right_token_boundary, 0), false, "move_right_token_boundary", 25, "Seek right for the next end of a token.", 39, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 485 },
{ PROC_LINKS(move_right_whitespace_boundary, 0), false, "move_right_whitespace_boundary", 30, "Seek right for the next boundary between whitespace and non-whitespace.", 71, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 469 },
{ PROC_LINKS(move_right_whitespace_or_token_boundary, 0), false, "move_right_whitespace_or_token_boundary", 39, "Seek right for the next end of a token or boundary between whitespace and non-whitespace.", 89, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 499 },
{ PROC_LINKS(move_up, 0), false, "move_up", 7, "Moves the cursor up one line.", 29, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 336 },
{ PROC_LINKS(move_up_10, 0), false, "move_up_10", 10, "Moves the cursor up ten lines.", 30, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 348 },
{ PROC_LINKS(move_up_to_blank_line, 0), false, "move_up_to_blank_line", 21, "Seeks the cursor up to the next blank line.", 43, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 407 },
{ PROC_LINKS(move_up_to_blank_line_end, 0), false, "move_up_to_blank_line_end", 25, "Seeks the cursor up to the next blank line and places it at the end of the line.", 80, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 431 },
{ PROC_LINKS(move_up_to_blank_line_skip_whitespace, 0), false, "move_up_to_blank_line_skip_whitespace", 37, "Seeks the cursor up to the next blank line and places it at the end of the line.", 80, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 419 },
{ PROC_LINKS(multi_paste, 0), false, "multi_paste", 11, "Paste multiple entries from the clipboard at once", 49, "D:\\Programs\\4coder\\custom\\4coder_clipboard.cpp", 46, 229 },
{ PROC_LINKS(multi_paste_interactive, 0), false, "multi_paste_interactive", 23, "Paste multiple lines from the clipboard history, controlled with arrow keys", 75, "D:\\Programs\\4coder\\custom\\4coder_clipboard.cpp", 46, 371 },
{ PROC_LINKS(multi_paste_interactive_quick, 0), false, "multi_paste_interactive_quick", 29, "Paste multiple lines from the clipboard history, controlled by inputing the number of lines to paste", 100, "D:\\Programs\\4coder\\custom\\4coder_clipboard.cpp", 46, 380 },
{ PROC_LINKS(music_start, 0), false, "music_start", 11, "Starts the music.", 17, "D:\\Programs\\4coder\\custom\\4coder_examples.cpp", 45, 213 },
{ PROC_LINKS(music_stop, 0), false, "music_stop", 10, "Stops the music.", 16, "D:\\Programs\\4coder\\custom\\4coder_examples.cpp", 45, 234 },
{ PROC_LINKS(open_all_code, 0), false, "open_all_code", 13, "Open all code in the current directory. File types are determined by extensions. An extension is considered code based on the extensions specified in 4coder.config.", 164, "D:\\Programs\\4coder\\custom\\4coder_project_commands.cpp", 53, 849 },
{ PROC_LINKS(open_all_code_recursive, 0), false, "open_all_code_recursive", 23, "Works as open_all_code but also runs in all subdirectories.", 59, "D:\\Programs\\4coder\\custom\\4coder_project_commands.cpp", 53, 858 },
{ PROC_LINKS(open_file_in_quotes, 0), false, "open_file_in_quotes", 19, "Reads a filename from surrounding '\"' characters and attempts to open the corresponding file.", 94, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 1578 },
{ PROC_LINKS(open_in_other, 0), false, "open_in_other", 13, "Interactively opens a file in the other panel.", 46, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 2061 },
{ PROC_LINKS(open_long_braces, 0), false, "open_long_braces", 16, "At the cursor, insert a '{' and '}' separated by a blank line.", 62, "D:\\Programs\\4coder\\custom\\4coder_combined_write_commands.cpp", 60, 46 },
{ PROC_LINKS(open_long_braces_break, 0), false, "open_long_braces_break", 22, "At the cursor, insert a '{' and '}break;' separated by a blank line.", 68, "D:\\Programs\\4coder\\custom\\4coder_combined_write_commands.cpp", 60, 62 },
{ PROC_LINKS(open_long_braces_semicolon, 0), false, "open_long_braces_semicolon", 26, "At the cursor, insert a '{' and '};' separated by a blank line.", 63, "D:\\Programs\\4coder\\custom\\4coder_combined_write_commands.cpp", 60, 54 },
{ PROC_LINKS(open_matching_file_cpp, 0), false, "open_matching_file_cpp", 22, "If the current file is a *.cpp or *.h, attempts to open the corresponding *.h or *.cpp file in the other view.", 110, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 1661 },
{ PROC_LINKS(open_panel_hsplit, 0), false, "open_panel_hsplit", 17, "Create a new panel by horizontally splitting the active panel.", 62, "D:\\Programs\\4coder\\custom\\4coder_default_framework.cpp", 54, 382 },
{ PROC_LINKS(open_panel_vsplit, 0), false, "open_panel_vsplit", 17, "Create a new panel by vertically splitting the active panel.", 60, "D:\\Programs\\4coder\\custom\\4coder_default_framework.cpp", 54, 372 },
{ PROC_LINKS(page_down, 0), false, "page_down", 9, "Scrolls the view down one view height and moves the cursor down one view height.", 80, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 378 },
{ PROC_LINKS(page_up, 0), false, "page_up", 7, "Scrolls the view up one view height and moves the cursor up one view height.", 76, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 370 },
{ PROC_LINKS(paste, 0), false, "paste", 5, "At the cursor, insert the text at the top of the clipboard.", 59, "D:\\Programs\\4coder\\custom\\4coder_clipboard.cpp", 46, 130 },
{ PROC_LINKS(paste_and_indent, 0), false, "paste_and_indent", 16, "Paste from the top of clipboard and run auto-indent on the newly pasted text.", 77, "D:\\Programs\\4coder\\custom\\4coder_clipboard.cpp", 46, 207 },
{ PROC_LINKS(paste_next, 0), false, "paste_next", 10, "If the previous command was paste or paste_next, replaces the paste range with the next text down on the clipboard, otherwise operates as the paste command.", 156, "D:\\Programs\\4coder\\custom\\4coder_clipboard.cpp", 46, 164 },
{ PROC_LINKS(paste_next_and_indent, 0), false, "paste_next_and_indent", 21, "Paste the next item on the clipboard and run auto-indent on the newly pasted text.", 82, "D:\\Programs\\4coder\\custom\\4coder_clipboard.cpp", 46, 214 },
{ PROC_LINKS(place_in_scope, 0), false, "place_in_scope", 14, "Wraps the code contained in the range between cursor and mark with a new curly brace scope.", 91, "D:\\Programs\\4coder\\custom\\4coder_scope_commands.cpp", 51, 106 },
{ PROC_LINKS(play_with_a_counter, 0), false, "play_with_a_counter", 19, "Example of query bar", 20, "D:\\Programs\\4coder\\custom\\4coder_examples.cpp", 45, 29 },
{ PROC_LINKS(profile_clear, 0), false, "profile_clear", 13, "Clear all profiling information from 4coder's self profiler.", 60, "D:\\Programs\\4coder\\custom\\4coder_profile.cpp", 44, 226 },
{ PROC_LINKS(profile_disable, 0), false, "profile_disable", 15, "Prevent 4coder's self profiler from gathering new profiling information.", 72, "D:\\Programs\\4coder\\custom\\4coder_profile.cpp", 44, 219 },
{ PROC_LINKS(profile_enable, 0), false, "profile_enable", 14, "Allow 4coder's self profiler to gather new profiling information.", 65, "D:\\Programs\\4coder\\custom\\4coder_profile.cpp", 44, 212 },
{ PROC_LINKS(profile_inspect, 0), true, "profile_inspect", 15, "Inspect all currently collected profiling information in 4coder's self profiler.", 80, "D:\\Programs\\4coder\\custom\\4coder_profile_inspect.cpp", 52, 886 },
{ PROC_LINKS(project_command_F1, 0), false, "project_command_F1", 18, "Run the command with index 1", 28, "D:\\Programs\\4coder\\custom\\4coder_project_commands.cpp", 53, 1118 },
{ PROC_LINKS(project_command_F10, 0), false, "project_command_F10", 19, "Run the command with index 10", 29, "D:\\Programs\\4coder\\custom\\4coder_project_commands.cpp", 53, 1172 },
{ PROC_LINKS(project_command_F11, 0), false, "project_command_F11", 19, "Run the command with index 11", 29, "D:\\Programs\\4coder\\custom\\4coder_project_commands.cpp", 53, 1178 },
{ PROC_LINKS(project_command_F12, 0), false, "project_command_F12", 19, "Run the command with index 12", 29, "D:\\Programs\\4coder\\custom\\4coder_project_commands.cpp", 53, 1184 },
{ PROC_LINKS(project_command_F13, 0), false, "project_command_F13", 19, "Run the command with index 13", 29, "D:\\Programs\\4coder\\custom\\4coder_project_commands.cpp", 53, 1190 },
{ PROC_LINKS(project_command_F14, 0), false, "project_command_F14", 19, "Run the command with index 14", 29, "D:\\Programs\\4coder\\custom\\4coder_project_commands.cpp", 53, 1196 },
{ PROC_LINKS(project_command_F15, 0), false, "project_command_F15", 19, "Run the command with index 15", 29, "D:\\Programs\\4coder\\custom\\4coder_project_commands.cpp", 53, 1202 },
{ PROC_LINKS(project_command_F16, 0), false, "project_command_F16", 19, "Run the command with index 16", 29, "D:\\Programs\\4coder\\custom\\4coder_project_commands.cpp", 53, 1208 },
{ PROC_LINKS(project_command_F2, 0), false, "project_command_F2", 18, "Run the command with index 2", 28, "D:\\Programs\\4coder\\custom\\4coder_project_commands.cpp", 53, 1124 },
{ PROC_LINKS(project_command_F3, 0), false, "project_command_F3", 18, "Run the command with index 3", 28, "D:\\Programs\\4coder\\custom\\4coder_project_commands.cpp", 53, 1130 },
{ PROC_LINKS(project_command_F4, 0), false, "project_command_F4", 18, "Run the command with index 4", 28, "D:\\Programs\\4coder\\custom\\4coder_project_commands.cpp", 53, 1136 },
{ PROC_LINKS(project_command_F5, 0), false, "project_command_F5", 18, "Run the command with index 5", 28, "D:\\Programs\\4coder\\custom\\4coder_project_commands.cpp", 53, 1142 },
{ PROC_LINKS(project_command_F6, 0), false, "project_command_F6", 18, "Run the command with index 6", 28, "D:\\Programs\\4coder\\custom\\4coder_project_commands.cpp", 53, 1148 },
{ PROC_LINKS(project_command_F7, 0), false, "project_command_F7", 18, "Run the command with index 7", 28, "D:\\Programs\\4coder\\custom\\4coder_project_commands.cpp", 53, 1154 },
{ PROC_LINKS(project_command_F8, 0), false, "project_command_F8", 18, "Run the command with index 8", 28, "D:\\Programs\\4coder\\custom\\4coder_project_commands.cpp", 53, 1160 },
{ PROC_LINKS(project_command_F9, 0), false, "project_command_F9", 18, "Run the command with index 9", 28, "D:\\Programs\\4coder\\custom\\4coder_project_commands.cpp", 53, 1166 },
{ PROC_LINKS(project_command_lister, 0), false, "project_command_lister", 22, "Open a lister of all commands in the currently loaded project.", 62, "D:\\Programs\\4coder\\custom\\4coder_project_commands.cpp", 53, 1070 },
{ PROC_LINKS(project_fkey_command, 0), false, "project_fkey_command", 20, "Run an 'fkey command' configured in a project.4coder file.  Determines the index of the 'fkey command' by which function key or numeric key was pressed to trigger the command.", 175, "D:\\Programs\\4coder\\custom\\4coder_project_commands.cpp", 53, 1008 },
{ PROC_LINKS(project_go_to_root_directory, 0), false, "project_go_to_root_directory", 28, "Changes 4coder's hot directory to the root directory of the currently loaded project. With no loaded project nothing hapepns.", 125, "D:\\Programs\\4coder\\custom\\4coder_project_commands.cpp", 53, 1034 },
{ PROC_LINKS(project_reprint, 0), false, "project_reprint", 15, "Prints the current project to the file it was loaded from; prints in the most recent project file version", 105, "D:\\Programs\\4coder\\custom\\4coder_project_commands.cpp", 53, 1080 },
{ PROC_LINKS(query_replace, 0), false, "query_replace", 13, "Queries the user for two strings, and incrementally replaces every occurence of the first string with the second string.", 120, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 1284 },
{ PROC_LINKS(query_replace_identifier, 0), false, "query_replace_identifier", 24, "Queries the user for a string, and incrementally replace every occurence of the word or token found at the cursor with the specified string.", 140, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 1305 },
{ PROC_LINKS(query_replace_selection, 0), false, "query_replace_selection", 23, "Queries the user for a string, and incrementally replace every occurence of the string found in the selected range with the specified string.", 141, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 1321 },
{ PROC_LINKS(quick_swap_buffer, 0), false, "quick_swap_buffer", 17, "Change to the most recently used buffer in this view - or to the top of the buffer stack if the most recent doesn't exist anymore", 129, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 1708 },
{ PROC_LINKS(redo, 0), false, "redo", 4, "Advances forwards through the undo history of the current buffer.", 65, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 1888 },
{ PROC_LINKS(redo_all_buffers, 0), false, "redo_all_buffers", 16, "Advances forward through the undo history in the buffer containing the most recent regular edit.", 96, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 1985 },
{ PROC_LINKS(rename_file_query, 0), false, "rename_file_query", 17, "Queries the user for a new name and renames the file of the current buffer, altering the buffer's name too.", 107, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 1462 },
{ PROC_LINKS(reopen, 0), false, "reopen", 6, "Reopen the current buffer from the hard drive.", 46, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 1746 },
{ PROC_LINKS(replace_in_all_buffers, 0), false, "replace_in_all_buffers", 22, "Queries the user for a needle and string. Replaces all occurences of needle with string in all editable buffers.", 112, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 1194 },
{ PROC_LINKS(replace_in_buffer, 0), false, "replace_in_buffer", 17, "Queries the user for a needle and string. Replaces all occurences of needle with string in the active buffer.", 109, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 1185 },
{ PROC_LINKS(replace_in_range, 0), false, "replace_in_range", 16, "Queries the user for a needle and string. Replaces all occurences of needle with string in the range between cursor and the mark in the active buffer.", 150, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 1176 },
{ PROC_LINKS(reverse_search, 0), false, "reverse_search", 14, "Begins an incremental search up through the current buffer for a user specified string.", 87, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 1117 },
{ PROC_LINKS(reverse_search_identifier, 0), false, "reverse_search_identifier", 25, "Begins an incremental search up through the current buffer for the word or token under the cursor.", 98, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 1129 },
{ PROC_LINKS(save, 0), false, "save", 4, "Saves the current buffer.", 25, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 1736 },
{ PROC_LINKS(save_all_dirty_buffers, 0), false, "save_all_dirty_buffers", 22, "Saves all buffers marked dirty (showing the '*' indicator).", 59, "D:\\Programs\\4coder\\custom\\4coder_default_framework.cpp", 54, 454 },
{ PROC_LINKS(save_to_query, 0), false, "save_to_query", 13, "Queries the user for a file name and saves the contents of the current buffer, altering the buffer's name too.", 110, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 1429 },
{ PROC_LINKS(search, 0), false, "search", 6, "Begins an incremental search down through the current buffer for a user specified string.", 89, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 1111 },
{ PROC_LINKS(search_identifier, 0), false, "search_identifier", 17, "Begins an incremental search down through the current buffer for the word or token under the cursor.", 100, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 1123 },
{ PROC_LINKS(seek_beginning_of_line, 0), false, "seek_beginning_of_line", 22, "Seeks the cursor to the beginning of the visual line.", 53, "D:\\Programs\\4coder\\custom\\4coder_helper.cpp", 43, 2246 },
{ PROC_LINKS(seek_beginning_of_textual_line, 0), false, "seek_beginning_of_textual_line", 30, "Seeks the cursor to the beginning of the line across all text.", 62, "D:\\Programs\\4coder\\custom\\4coder_helper.cpp", 43, 2234 },
{ PROC_LINKS(seek_end_of_line, 0), false, "seek_end_of_line", 16, "Seeks the cursor to the end of the visual line.", 47, "D:\\Programs\\4coder\\custom\\4coder_helper.cpp", 43, 2252 },
{ PROC_LINKS(seek_end_of_textual_line, 0), false, "seek_end_of_textual_line", 24, "Seeks the cursor to the end of the line across all text.", 56, "D:\\Programs\\4coder\\custom\\4coder_helper.cpp", 43, 2240 },
{ PROC_LINKS(select_all, 0), false, "select_all", 10, "Puts the cursor at the top of the file, and the mark at the bottom of the file.", 79, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 543 },
{ PROC_LINKS(select_next_scope_absolute, 0), false, "select_next_scope_absolute", 26, "Finds the first scope started by '{' after the cursor and puts the cursor and mark on the '{' and '}'.", 102, "D:\\Programs\\4coder\\custom\\4coder_scope_commands.cpp", 51, 57 },
{ PROC_LINKS(select_next_scope_after_current, 0), false, "select_next_scope_after_current", 31, "If a scope is selected, find first scope that starts after the selected scope. Otherwise find the first scope that starts after the cursor.", 139, "D:\\Programs\\4coder\\custom\\4coder_scope_commands.cpp", 51, 66 },
{ PROC_LINKS(select_prev_scope_absolute, 0), false, "select_prev_scope_absolute", 26, "Finds the first scope started by '{' before the cursor and puts the cursor and mark on the '{' and '}'.", 103, "D:\\Programs\\4coder\\custom\\4coder_scope_commands.cpp", 51, 82 },
{ PROC_LINKS(select_prev_top_most_scope, 0), false, "select_prev_top_most_scope", 26, "Finds the first scope that starts before the cursor, then finds the top most scope that contains that scope.", 108, "D:\\Programs\\4coder\\custom\\4coder_scope_commands.cpp", 51, 99 },
{ PROC_LINKS(select_surrounding_scope, 0), false, "select_surrounding_scope", 24, "Finds the scope enclosed by '{' '}' surrounding the cursor and puts the cursor and mark on the '{' and '}'.", 107, "D:\\Programs\\4coder\\custom\\4coder_scope_commands.cpp", 51, 27 },
{ PROC_LINKS(select_surrounding_scope_maximal, 0), false, "select_surrounding_scope_maximal", 32, "Selects the top-most scope that surrounds the cursor.", 53, "D:\\Programs\\4coder\\custom\\4coder_scope_commands.cpp", 51, 39 },
{ PROC_LINKS(set_eol_mode_from_contents, 0), false, "set_eol_mode_from_contents", 26, "Sets the buffer's line ending mode to match the contents of the buffer.", 71, "D:\\Programs\\4coder\\custom\\4coder_eol.cpp", 40, 125 },
{ PROC_LINKS(set_eol_mode_to_binary, 0), false, "set_eol_mode_to_binary", 22, "Puts the buffer in bin line ending mode.", 40, "D:\\Programs\\4coder\\custom\\4coder_eol.cpp", 40, 112 },
{ PROC_LINKS(set_eol_mode_to_crlf, 0), false, "set_eol_mode_to_crlf", 20, "Puts the buffer in crlf line ending mode.", 41, "D:\\Programs\\4coder\\custom\\4coder_eol.cpp", 40, 86 },
{ PROC_LINKS(set_eol_mode_to_lf, 0), false, "set_eol_mode_to_lf", 18, "Puts the buffer in lf line ending mode.", 39, "D:\\Programs\\4coder\\custom\\4coder_eol.cpp", 40, 99 },
{ PROC_LINKS(set_face_size, 0), false, "set_face_size", 13, "Set face size of the face used by the current buffer.", 53, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 728 },
{ PROC_LINKS(set_face_size_this_buffer, 0), false, "set_face_size_this_buffer", 25, "Set face size of the face used by the current buffer; if any other buffers are using the same face a new face is created so that only this buffer is effected", 157, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 770 },
{ PROC_LINKS(set_mark, 0), false, "set_mark", 8, "Sets the mark to the current position of the cursor.", 52, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 115 },
{ PROC_LINKS(set_mode_to_notepad_like, 0), false, "set_mode_to_notepad_like", 24, "Sets the edit mode to Notepad like.", 35, "D:\\Programs\\4coder\\custom\\4coder_default_framework.cpp", 54, 499 },
{ PROC_LINKS(set_mode_to_original, 0), false, "set_mode_to_original", 20, "Sets the edit mode to 4coder original.", 38, "D:\\Programs\\4coder\\custom\\4coder_default_framework.cpp", 54, 493 },
{ PROC_LINKS(setup_build_bat, 0), false, "setup_build_bat", 15, "Queries the user for several configuration options and initializes a new build batch script.", 92, "D:\\Programs\\4coder\\custom\\4coder_project_commands.cpp", 53, 1052 },
{ PROC_LINKS(setup_build_bat_and_sh, 0), false, "setup_build_bat_and_sh", 22, "Queries the user for several configuration options and initializes a new build batch script.", 92, "D:\\Programs\\4coder\\custom\\4coder_project_commands.cpp", 53, 1064 },
{ PROC_LINKS(setup_build_sh, 0), false, "setup_build_sh", 14, "Queries the user for several configuration options and initializes a new build shell script.", 92, "D:\\Programs\\4coder\\custom\\4coder_project_commands.cpp", 53, 1058 },
{ PROC_LINKS(setup_new_project, 0), false, "setup_new_project", 17, "Queries the user for several configuration options and initializes a new 4coder project with build scripts for every OS.", 120, "D:\\Programs\\4coder\\custom\\4coder_project_commands.cpp", 53, 1045 },
{ PROC_LINKS(show_filebar, 0), false, "show_filebar", 12, "Sets the current view to show it's filebar.", 43, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 699 },
{ PROC_LINKS(show_scrollbar, 0), false, "show_scrollbar", 14, "Sets the current view to show it's scrollbar.", 45, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 685 },
{ PROC_LINKS(show_the_log_graph, 0), true, "show_the_log_graph", 18, "Parses *log* and displays the 'log graph' UI", 44, "D:\\Programs\\4coder\\custom\\4coder_log_parser.cpp", 47, 991 },
{ PROC_LINKS(snipe_backward_whitespace_or_token_boundary, 0), false, "snipe_backward_whitespace_or_token_boundary", 43, "Delete a single, whole token on or to the left of the cursor and post it to the clipboard.", 90, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 179 },
{ PROC_LINKS(snipe_forward_whitespace_or_token_boundary, 0), false, "snipe_forward_whitespace_or_token_boundary", 42, "Delete a single, whole token on or to the right of the cursor and post it to the clipboard.", 91, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 187 },
{ PROC_LINKS(snippet_lister, 0), true, "snippet_lister", 14, "Opens a snippet lister for inserting whole pre-written snippets of text.", 72, "D:\\Programs\\4coder\\custom\\4coder_combined_write_commands.cpp", 60, 237 },
{ PROC_LINKS(string_repeat, 0), false, "string_repeat", 13, "Example of query_user_string and query_user_number", 50, "D:\\Programs\\4coder\\custom\\4coder_examples.cpp", 45, 179 },
{ PROC_LINKS(suppress_mouse, 0), false, "suppress_mouse", 14, "Hides the mouse and causes all mosue input (clicks, position, wheel) to be ignored.", 83, "D:\\Programs\\4coder\\custom\\4coder_default_framework.cpp", 54, 475 },
{ PROC_LINKS(swap_panels, 0), false, "swap_panels", 11, "Swaps the active panel with it's sibling.", 41, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 1686 },
{ PROC_LINKS(switch_to_keybinding_0, 0), false, "switch_to_keybinding_0", 22, "Switch the keybindings to mode 0.", 33, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_bindings.cpp", 66, 9 },
{ PROC_LINKS(switch_to_keybinding_1, 0), false, "switch_to_keybinding_1", 22, "Switch the keybindings to mode 1.", 33, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_bindings.cpp", 66, 15 },
{ PROC_LINKS(switch_to_keybinding_2, 0), false, "switch_to_keybinding_2", 22, "Switch the keybindings to mode 2.", 33, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_bindings.cpp", 66, 21 },
{ PROC_LINKS(switch_to_keybinding_3, 0), false, "switch_to_keybinding_3", 22, "Switch the keybindings to mode 3.", 33, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_bindings.cpp", 66, 27 },
{ PROC_LINKS(theme_lister, 0), true, "theme_lister", 12, "Opens an interactive list of all registered themes.", 51, "D:\\Programs\\4coder\\custom\\4coder_lists.cpp", 42, 785 },
{ PROC_LINKS(to_lowercase, 0), false, "to_lowercase", 12, "Converts all ascii text in the range between the cursor and the mark to lowercase.", 82, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 569 },
{ PROC_LINKS(to_uppercase, 0), false, "to_uppercase", 12, "Converts all ascii text in the range between the cursor and the mark to uppercase.", 82, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 556 },
{ PROC_LINKS(toggle_command_server, 0), false, "toggle_command_server", 21, "Toggle command server.", 22, "D:\\Programs\\4coder\\custom\\4coder_fleury\\4coder_fleury_command_server.cpp", 72, 734 },
{ PROC_LINKS(toggle_filebar, 0), false, "toggle_filebar", 14, "Toggles the visibility status of the current view's filebar.", 60, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 713 },
{ PROC_LINKS(toggle_fps_meter, 0), false, "toggle_fps_meter", 16, "Toggles the visibility of the FPS performance meter", 51, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 722 },
{ PROC_LINKS(toggle_fullscreen, 0), false, "toggle_fullscreen", 17, "Toggle fullscreen mode on or off.  The change(s) do not take effect until the next frame.", 89, "D:\\Programs\\4coder\\custom\\4coder_default_framework.cpp", 54, 529 },
{ PROC_LINKS(toggle_highlight_enclosing_scopes, 0), false, "toggle_highlight_enclosing_scopes", 33, "In code files scopes surrounding the cursor are highlighted with distinguishing colors.", 87, "D:\\Programs\\4coder\\custom\\4coder_default_framework.cpp", 54, 513 },
{ PROC_LINKS(toggle_highlight_line_at_cursor, 0), false, "toggle_highlight_line_at_cursor", 31, "Toggles the line highlight at the cursor.", 41, "D:\\Programs\\4coder\\custom\\4coder_default_framework.cpp", 54, 505 },
{ PROC_LINKS(toggle_line_numbers, 0), false, "toggle_line_numbers", 19, "Toggles the left margin line numbers.", 37, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 827 },
{ PROC_LINKS(toggle_line_wrap, 0), false, "toggle_line_wrap", 16, "Toggles the line wrap setting on this buffer.", 45, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 835 },
{ PROC_LINKS(toggle_mouse, 0), false, "toggle_mouse", 12, "Toggles the mouse suppression mode, see suppress_mouse and allow_mouse.", 71, "D:\\Programs\\4coder\\custom\\4coder_default_framework.cpp", 54, 487 },
{ PROC_LINKS(toggle_paren_matching_helper, 0), false, "toggle_paren_matching_helper", 28, "In code files matching parentheses pairs are colored with distinguishing colors.", 80, "D:\\Programs\\4coder\\custom\\4coder_default_framework.cpp", 54, 521 },
{ PROC_LINKS(toggle_show_whitespace, 0), false, "toggle_show_whitespace", 22, "Toggles the current buffer's whitespace visibility status.", 58, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 818 },
{ PROC_LINKS(toggle_virtual_whitespace, 0), false, "toggle_virtual_whitespace", 25, "Toggles virtual whitespace for all files.", 41, "D:\\Programs\\4coder\\custom\\4coder_code_index.cpp", 47, 1337 },
{ PROC_LINKS(tutorial_maximize, 0), false, "tutorial_maximize", 17, "Expand the tutorial window", 26, "D:\\Programs\\4coder\\custom\\4coder_tutorial.cpp", 45, 20 },
{ PROC_LINKS(tutorial_minimize, 0), false, "tutorial_minimize", 17, "Shrink the tutorial window", 26, "D:\\Programs\\4coder\\custom\\4coder_tutorial.cpp", 45, 34 },
{ PROC_LINKS(uncomment_line, 0), false, "uncomment_line", 14, "If present, delete '//' at the beginning of the line after leading whitespace.", 78, "D:\\Programs\\4coder\\custom\\4coder_combined_write_commands.cpp", 60, 137 },
{ PROC_LINKS(undo, 0), false, "undo", 4, "Advances backwards through the undo history of the current buffer.", 66, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 1836 },
{ PROC_LINKS(undo_all_buffers, 0), false, "undo_all_buffers", 16, "Advances backward through the undo history in the buffer containing the most recent regular edit.", 97, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 1914 },
{ PROC_LINKS(view_buffer_other_panel, 0), false, "view_buffer_other_panel", 23, "Set the other non-active panel to view the buffer that the active panel views, and switch to that panel.", 104, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 1674 },
{ PROC_LINKS(view_jump_list_with_lister, 0), false, "view_jump_list_with_lister", 26, "When executed on a buffer with jumps, creates a persistent lister for all the jumps", 83, "D:\\Programs\\4coder\\custom\\4coder_jump_lister.cpp", 48, 59 },
{ PROC_LINKS(word_complete, 0), false, "word_complete", 13, "Iteratively tries completing the word to the left of the cursor with other words in open buffers that have the same prefix string.", 130, "D:\\Programs\\4coder\\custom\\4coder_search.cpp", 43, 433 },
{ PROC_LINKS(word_complete_drop_down, 0), false, "word_complete_drop_down", 23, "Word complete with drop down menu.", 34, "D:\\Programs\\4coder\\custom\\4coder_search.cpp", 43, 679 },
{ PROC_LINKS(write_block, 0), false, "write_block", 11, "At the cursor, insert a block comment.", 38, "D:\\Programs\\4coder\\custom\\4coder_combined_write_commands.cpp", 60, 94 },
{ PROC_LINKS(write_hack, 0), false, "write_hack", 10, "At the cursor, insert a '// HACK' comment, includes user name if it was specified in config.4coder.", 99, "D:\\Programs\\4coder\\custom\\4coder_combined_write_commands.cpp", 60, 82 },
{ PROC_LINKS(write_note, 0), false, "write_note", 10, "At the cursor, insert a '// NOTE' comment, includes user name if it was specified in config.4coder.", 99, "D:\\Programs\\4coder\\custom\\4coder_combined_write_commands.cpp", 60, 88 },
{ PROC_LINKS(write_space, 0), false, "write_space", 11, "Inserts a space.", 16, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 67 },
{ PROC_LINKS(write_text_and_auto_indent, 0), false, "write_text_and_auto_indent", 26, "Inserts text and auto-indents the line on which the cursor sits if any of the text contains 'layout punctuation' such as ;:{}()[]# and new lines.", 145, "D:\\Programs\\4coder\\custom\\4coder_auto_indent.cpp", 48, 443 },
{ PROC_LINKS(write_text_input, 0), false, "write_text_input", 16, "Inserts whatever text was used to trigger this command.", 55, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 59 },
{ PROC_LINKS(write_todo, 0), false, "write_todo", 10, "At the cursor, insert a '// TODO' comment, includes user name if it was specified in config.4coder.", 99, "D:\\Programs\\4coder\\custom\\4coder_combined_write_commands.cpp", 60, 76 },
{ PROC_LINKS(write_underscore, 0), false, "write_underscore", 16, "Inserts an underscore.", 22, "D:\\Programs\\4coder\\custom\\4coder_base_commands.cpp", 50, 73 },
{ PROC_LINKS(write_zero_struct, 0), false, "write_zero_struct", 17, "At the cursor, insert a ' = {};'.", 33, "D:\\Programs\\4coder\\custom\\4coder_combined_write_commands.cpp", 60, 100 },
};
static i32 fcoder_metacmd_ID_allow_mouse = 0;
static i32 fcoder_metacmd_ID_auto_indent_line_at_cursor = 1;
static i32 fcoder_metacmd_ID_auto_indent_range = 2;
static i32 fcoder_metacmd_ID_auto_indent_whole_file = 3;
static i32 fcoder_metacmd_ID_backspace_alpha_numeric_boundary = 4;
static i32 fcoder_metacmd_ID_backspace_char = 5;
static i32 fcoder_metacmd_ID_basic_change_active_panel = 6;
static i32 fcoder_metacmd_ID_begin_clipboard_collection_mode = 7;
static i32 fcoder_metacmd_ID_build_in_build_panel = 8;
static i32 fcoder_metacmd_ID_build_search = 9;
static i32 fcoder_metacmd_ID_casey_clean_file_and_save = 10;
static i32 fcoder_metacmd_ID_casey_delete_to_end_of_line = 11;
static i32 fcoder_metacmd_ID_casey_demo_audio = 12;
static i32 fcoder_metacmd_ID_casey_demo_audio_one_shot = 13;
static i32 fcoder_metacmd_ID_casey_demo_audio_switch_panel = 14;
static i32 fcoder_metacmd_ID_casey_find_matching_file = 15;
static i32 fcoder_metacmd_ID_casey_go_to_code_peek = 16;
static i32 fcoder_metacmd_ID_casey_newline_and_indent = 17;
static i32 fcoder_metacmd_ID_casey_seek_beginning_of_line_and_tab = 18;
static i32 fcoder_metacmd_ID_casey_switch_to_keybinding_0 = 19;
static i32 fcoder_metacmd_ID_casey_switch_to_keybinding_1 = 20;
static i32 fcoder_metacmd_ID_center_view = 21;
static i32 fcoder_metacmd_ID_change_active_panel = 22;
static i32 fcoder_metacmd_ID_change_active_panel_backwards = 23;
static i32 fcoder_metacmd_ID_change_to_build_panel = 24;
static i32 fcoder_metacmd_ID_clean_all_lines = 25;
static i32 fcoder_metacmd_ID_clean_trailing_whitespace = 26;
static i32 fcoder_metacmd_ID_clear_all_themes = 27;
static i32 fcoder_metacmd_ID_clear_clipboard = 28;
static i32 fcoder_metacmd_ID_click_set_cursor = 29;
static i32 fcoder_metacmd_ID_click_set_cursor_and_mark = 30;
static i32 fcoder_metacmd_ID_click_set_cursor_if_lbutton = 31;
static i32 fcoder_metacmd_ID_click_set_mark = 32;
static i32 fcoder_metacmd_ID_clipboard_record_clip = 33;
static i32 fcoder_metacmd_ID_close_all_code = 34;
static i32 fcoder_metacmd_ID_close_build_panel = 35;
static i32 fcoder_metacmd_ID_close_panel = 36;
static i32 fcoder_metacmd_ID_command_documentation = 37;
static i32 fcoder_metacmd_ID_command_lister = 38;
static i32 fcoder_metacmd_ID_comment_line = 39;
static i32 fcoder_metacmd_ID_comment_line_toggle = 40;
static i32 fcoder_metacmd_ID_copy = 41;
static i32 fcoder_metacmd_ID_cursor_mark_swap = 42;
static i32 fcoder_metacmd_ID_custom_api_documentation = 43;
static i32 fcoder_metacmd_ID_cut = 44;
static i32 fcoder_metacmd_ID_decrease_face_size = 45;
static i32 fcoder_metacmd_ID_default_file_externally_modified = 46;
static i32 fcoder_metacmd_ID_default_startup = 47;
static i32 fcoder_metacmd_ID_default_try_exit = 48;
static i32 fcoder_metacmd_ID_default_view_input_handler = 49;
static i32 fcoder_metacmd_ID_delete_alpha_numeric_boundary = 50;
static i32 fcoder_metacmd_ID_delete_char = 51;
static i32 fcoder_metacmd_ID_delete_current_scope = 52;
static i32 fcoder_metacmd_ID_delete_file_query = 53;
static i32 fcoder_metacmd_ID_delete_line = 54;
static i32 fcoder_metacmd_ID_delete_range = 55;
static i32 fcoder_metacmd_ID_display_key_codes = 56;
static i32 fcoder_metacmd_ID_display_text_input = 57;
static i32 fcoder_metacmd_ID_double_backspace = 58;
static i32 fcoder_metacmd_ID_duplicate_line = 59;
static i32 fcoder_metacmd_ID_execute_any_cli = 60;
static i32 fcoder_metacmd_ID_execute_previous_cli = 61;
static i32 fcoder_metacmd_ID_exit_4coder = 62;
static i32 fcoder_metacmd_ID_f4_autocomplete_or_indent = 63;
static i32 fcoder_metacmd_ID_f4_backspace_alpha_numeric_or_camel_boundary = 64;
static i32 fcoder_metacmd_ID_f4_backspace_token_boundary = 65;
static i32 fcoder_metacmd_ID_f4_bump_to_column = 66;
static i32 fcoder_metacmd_ID_f4_code_peek = 67;
static i32 fcoder_metacmd_ID_f4_code_peek_clear = 68;
static i32 fcoder_metacmd_ID_f4_code_peek_yank = 69;
static i32 fcoder_metacmd_ID_f4_comment_selection = 70;
static i32 fcoder_metacmd_ID_f4_delete_alpha_numeric_or_camel_boundary = 71;
static i32 fcoder_metacmd_ID_f4_delete_token_boundary = 72;
static i32 fcoder_metacmd_ID_f4_go_to_definition = 73;
static i32 fcoder_metacmd_ID_f4_go_to_definition_same_panel = 74;
static i32 fcoder_metacmd_ID_f4_home = 75;
static i32 fcoder_metacmd_ID_f4_home_first_non_whitespace = 76;
static i32 fcoder_metacmd_ID_f4_interactive_open_or_new_in_project = 77;
static i32 fcoder_metacmd_ID_f4_leave_event_unhandled = 78;
static i32 fcoder_metacmd_ID_f4_lego_buffer_place = 79;
static i32 fcoder_metacmd_ID_f4_lego_click_store_token_1 = 80;
static i32 fcoder_metacmd_ID_f4_lego_click_store_token_2 = 81;
static i32 fcoder_metacmd_ID_f4_lego_store_line = 82;
static i32 fcoder_metacmd_ID_f4_lego_store_range = 83;
static i32 fcoder_metacmd_ID_f4_lego_store_token = 84;
static i32 fcoder_metacmd_ID_f4_loc = 85;
static i32 fcoder_metacmd_ID_f4_move_down_token_occurrence = 86;
static i32 fcoder_metacmd_ID_f4_move_left = 87;
static i32 fcoder_metacmd_ID_f4_move_left_token_boundary = 88;
static i32 fcoder_metacmd_ID_f4_move_right = 89;
static i32 fcoder_metacmd_ID_f4_move_right_token_boundary = 90;
static i32 fcoder_metacmd_ID_f4_move_to_next_divider_comment = 91;
static i32 fcoder_metacmd_ID_f4_move_to_prev_divider_comment = 92;
static i32 fcoder_metacmd_ID_f4_move_up_token_occurrence = 93;
static i32 fcoder_metacmd_ID_f4_open_project = 94;
static i32 fcoder_metacmd_ID_f4_powermode_allow = 95;
static i32 fcoder_metacmd_ID_f4_powermode_disallow = 96;
static i32 fcoder_metacmd_ID_f4_recent_files_menu = 97;
static i32 fcoder_metacmd_ID_f4_remedy_open_cursor = 98;
static i32 fcoder_metacmd_ID_f4_reverse_search = 99;
static i32 fcoder_metacmd_ID_f4_search = 100;
static i32 fcoder_metacmd_ID_f4_search_for_definition__current_file = 101;
static i32 fcoder_metacmd_ID_f4_search_for_definition__project_wide = 102;
static i32 fcoder_metacmd_ID_f4_setup_new_project = 103;
static i32 fcoder_metacmd_ID_f4_switch_syntax_option = 104;
static i32 fcoder_metacmd_ID_f4_toggle_battery_saver = 105;
static i32 fcoder_metacmd_ID_f4_toggle_compilation_expand = 106;
static i32 fcoder_metacmd_ID_f4_toggle_enclosure_side = 107;
static i32 fcoder_metacmd_ID_f4_uncomment_selection = 108;
static i32 fcoder_metacmd_ID_f4_unindent = 109;
static i32 fcoder_metacmd_ID_f4_write_text_and_auto_indent = 110;
static i32 fcoder_metacmd_ID_f4_write_text_input = 111;
static i32 fcoder_metacmd_ID_f4_write_zero_struct = 112;
static i32 fcoder_metacmd_ID_fleury_go_to_definition = 113;
static i32 fcoder_metacmd_ID_fleury_go_to_definition_same_panel = 114;
static i32 fcoder_metacmd_ID_fleury_home = 115;
static i32 fcoder_metacmd_ID_fleury_startup = 116;
static i32 fcoder_metacmd_ID_fleury_toggle_battery_saver = 117;
static i32 fcoder_metacmd_ID_fleury_toggle_compilation_expand = 118;
static i32 fcoder_metacmd_ID_fleury_write_text_and_auto_indent = 119;
static i32 fcoder_metacmd_ID_fleury_write_text_input = 120;
static i32 fcoder_metacmd_ID_fleury_write_zero_struct = 121;
static i32 fcoder_metacmd_ID_go_to_user_directory = 122;
static i32 fcoder_metacmd_ID_goto_beginning_of_file = 123;
static i32 fcoder_metacmd_ID_goto_end_of_file = 124;
static i32 fcoder_metacmd_ID_goto_first_jump = 125;
static i32 fcoder_metacmd_ID_goto_first_jump_same_panel_sticky = 126;
static i32 fcoder_metacmd_ID_goto_jump_at_cursor = 127;
static i32 fcoder_metacmd_ID_goto_jump_at_cursor_same_panel = 128;
static i32 fcoder_metacmd_ID_goto_line = 129;
static i32 fcoder_metacmd_ID_goto_next_jump = 130;
static i32 fcoder_metacmd_ID_goto_next_jump_no_skips = 131;
static i32 fcoder_metacmd_ID_goto_prev_jump = 132;
static i32 fcoder_metacmd_ID_goto_prev_jump_no_skips = 133;
static i32 fcoder_metacmd_ID_hide_filebar = 134;
static i32 fcoder_metacmd_ID_hide_scrollbar = 135;
static i32 fcoder_metacmd_ID_hit_sfx = 136;
static i32 fcoder_metacmd_ID_hms_demo_tutorial = 137;
static i32 fcoder_metacmd_ID_if0_off = 138;
static i32 fcoder_metacmd_ID_if_read_only_goto_position = 139;
static i32 fcoder_metacmd_ID_if_read_only_goto_position_same_panel = 140;
static i32 fcoder_metacmd_ID_increase_face_size = 141;
static i32 fcoder_metacmd_ID_interactive_kill_buffer = 142;
static i32 fcoder_metacmd_ID_interactive_new = 143;
static i32 fcoder_metacmd_ID_interactive_open = 144;
static i32 fcoder_metacmd_ID_interactive_open_or_new = 145;
static i32 fcoder_metacmd_ID_interactive_switch_buffer = 146;
static i32 fcoder_metacmd_ID_jump_to_definition = 147;
static i32 fcoder_metacmd_ID_jump_to_definition_at_cursor = 148;
static i32 fcoder_metacmd_ID_jump_to_last_point = 149;
static i32 fcoder_metacmd_ID_keyboard_macro_finish_recording = 150;
static i32 fcoder_metacmd_ID_keyboard_macro_replay = 151;
static i32 fcoder_metacmd_ID_keyboard_macro_start_recording = 152;
static i32 fcoder_metacmd_ID_kill_buffer = 153;
static i32 fcoder_metacmd_ID_kill_tutorial = 154;
static i32 fcoder_metacmd_ID_left_adjust_view = 155;
static i32 fcoder_metacmd_ID_list_all_functions_all_buffers = 156;
static i32 fcoder_metacmd_ID_list_all_functions_all_buffers_lister = 157;
static i32 fcoder_metacmd_ID_list_all_functions_current_buffer = 158;
static i32 fcoder_metacmd_ID_list_all_functions_current_buffer_lister = 159;
static i32 fcoder_metacmd_ID_list_all_locations = 160;
static i32 fcoder_metacmd_ID_list_all_locations_case_insensitive = 161;
static i32 fcoder_metacmd_ID_list_all_locations_of_identifier = 162;
static i32 fcoder_metacmd_ID_list_all_locations_of_identifier_case_insensitive = 163;
static i32 fcoder_metacmd_ID_list_all_locations_of_selection = 164;
static i32 fcoder_metacmd_ID_list_all_locations_of_selection_case_insensitive = 165;
static i32 fcoder_metacmd_ID_list_all_locations_of_type_definition = 166;
static i32 fcoder_metacmd_ID_list_all_locations_of_type_definition_of_identifier = 167;
static i32 fcoder_metacmd_ID_list_all_substring_locations = 168;
static i32 fcoder_metacmd_ID_list_all_substring_locations_case_insensitive = 169;
static i32 fcoder_metacmd_ID_load_project = 170;
static i32 fcoder_metacmd_ID_load_theme_current_buffer = 171;
static i32 fcoder_metacmd_ID_load_themes_default_folder = 172;
static i32 fcoder_metacmd_ID_load_themes_hot_directory = 173;
static i32 fcoder_metacmd_ID_long_clean_whitespace_at_cursor = 174;
static i32 fcoder_metacmd_ID_long_command_lister = 175;
static i32 fcoder_metacmd_ID_long_copy_line = 176;
static i32 fcoder_metacmd_ID_long_copy_token = 177;
static i32 fcoder_metacmd_ID_long_cut_line = 178;
static i32 fcoder_metacmd_ID_long_cut_token = 179;
static i32 fcoder_metacmd_ID_long_go_to_definition = 180;
static i32 fcoder_metacmd_ID_long_go_to_definition_same_panel = 181;
static i32 fcoder_metacmd_ID_long_history_lister = 182;
static i32 fcoder_metacmd_ID_long_indent_range = 183;
static i32 fcoder_metacmd_ID_long_indent_whole_file = 184;
static i32 fcoder_metacmd_ID_long_interactive_kill_buffer = 185;
static i32 fcoder_metacmd_ID_long_interactive_switch_buffer = 186;
static i32 fcoder_metacmd_ID_long_jump_lister = 187;
static i32 fcoder_metacmd_ID_long_kill_buffer = 188;
static i32 fcoder_metacmd_ID_long_kill_search_buffer = 189;
static i32 fcoder_metacmd_ID_long_list_all_lines_from_start_to_cursor_absolute = 190;
static i32 fcoder_metacmd_ID_long_list_all_lines_from_start_to_cursor_relative = 191;
static i32 fcoder_metacmd_ID_long_list_all_lines_in_range = 192;
static i32 fcoder_metacmd_ID_long_list_all_lines_in_range_non_white = 193;
static i32 fcoder_metacmd_ID_long_list_all_lines_in_range_seek_end = 194;
static i32 fcoder_metacmd_ID_long_list_all_lines_in_range_seek_end_non_white = 195;
static i32 fcoder_metacmd_ID_long_list_all_locations = 196;
static i32 fcoder_metacmd_ID_long_list_all_locations_current_buffer = 197;
static i32 fcoder_metacmd_ID_long_list_all_locations_of_identifier = 198;
static i32 fcoder_metacmd_ID_long_list_all_locations_of_identifier_current_buffer = 199;
static i32 fcoder_metacmd_ID_long_list_all_substring_locations_case_insensitive = 200;
static i32 fcoder_metacmd_ID_long_list_all_substring_locations_case_insensitive_current_buffer = 201;
static i32 fcoder_metacmd_ID_long_list_all_substring_locations_of_identifier_case_insensitive = 202;
static i32 fcoder_metacmd_ID_long_list_all_substring_locations_of_identifier_case_insensitive_current_buffer = 203;
static i32 fcoder_metacmd_ID_long_macro_repeat = 204;
static i32 fcoder_metacmd_ID_long_macro_toggle_recording = 205;
static i32 fcoder_metacmd_ID_long_move_down_token_occurrence = 206;
static i32 fcoder_metacmd_ID_long_move_left_alpha_numeric_or_camel_boundary = 207;
static i32 fcoder_metacmd_ID_long_move_left_boundary = 208;
static i32 fcoder_metacmd_ID_long_move_right_alpha_numeric_or_camel_boundary = 209;
static i32 fcoder_metacmd_ID_long_move_right_boundary = 210;
static i32 fcoder_metacmd_ID_long_move_to_next_function_and_type = 211;
static i32 fcoder_metacmd_ID_long_move_to_prev_function_and_type = 212;
static i32 fcoder_metacmd_ID_long_move_up_token_occurrence = 213;
static i32 fcoder_metacmd_ID_long_open_matching_file_same_panel = 214;
static i32 fcoder_metacmd_ID_long_paste_and_indent = 215;
static i32 fcoder_metacmd_ID_long_paste_and_replace_range = 216;
static i32 fcoder_metacmd_ID_long_point_lister = 217;
static i32 fcoder_metacmd_ID_long_push_new_jump = 218;
static i32 fcoder_metacmd_ID_long_query_replace = 219;
static i32 fcoder_metacmd_ID_long_query_replace_identifier = 220;
static i32 fcoder_metacmd_ID_long_query_replace_selection = 221;
static i32 fcoder_metacmd_ID_long_recent_files_menu = 222;
static i32 fcoder_metacmd_ID_long_redo = 223;
static i32 fcoder_metacmd_ID_long_redo_all_buffers = 224;
static i32 fcoder_metacmd_ID_long_redo_jump = 225;
static i32 fcoder_metacmd_ID_long_redo_same_pos = 226;
static i32 fcoder_metacmd_ID_long_reload_all_themes_default_folder = 227;
static i32 fcoder_metacmd_ID_long_reverse_search = 228;
static i32 fcoder_metacmd_ID_long_reverse_search_case_sensitive = 229;
static i32 fcoder_metacmd_ID_long_reverse_search_identifier = 230;
static i32 fcoder_metacmd_ID_long_search = 231;
static i32 fcoder_metacmd_ID_long_search_case_sensitive = 232;
static i32 fcoder_metacmd_ID_long_search_for_definition__current_file = 233;
static i32 fcoder_metacmd_ID_long_search_for_definition__project_wide = 234;
static i32 fcoder_metacmd_ID_long_search_for_definition_no_filter__project_file = 235;
static i32 fcoder_metacmd_ID_long_search_identifier = 236;
static i32 fcoder_metacmd_ID_long_search_identifier_case_sensitive = 237;
static i32 fcoder_metacmd_ID_long_select_current_line = 238;
static i32 fcoder_metacmd_ID_long_select_current_token = 239;
static i32 fcoder_metacmd_ID_long_select_lower_scope = 240;
static i32 fcoder_metacmd_ID_long_select_next_scope_current_level = 241;
static i32 fcoder_metacmd_ID_long_select_prev_scope_current_level = 242;
static i32 fcoder_metacmd_ID_long_select_surrounding_scope = 243;
static i32 fcoder_metacmd_ID_long_select_upper_scope = 244;
static i32 fcoder_metacmd_ID_long_switch_pos_context_draw_position = 245;
static i32 fcoder_metacmd_ID_long_switch_pos_context_option = 246;
static i32 fcoder_metacmd_ID_long_switch_to_search_buffer = 247;
static i32 fcoder_metacmd_ID_long_test_lister_render = 248;
static i32 fcoder_metacmd_ID_long_theme_lister = 249;
static i32 fcoder_metacmd_ID_long_toggle_compilation_expand = 250;
static i32 fcoder_metacmd_ID_long_toggle_line_offset = 251;
static i32 fcoder_metacmd_ID_long_toggle_panel_expand = 252;
static i32 fcoder_metacmd_ID_long_toggle_pos_context = 253;
static i32 fcoder_metacmd_ID_long_undo = 254;
static i32 fcoder_metacmd_ID_long_undo_all_buffers = 255;
static i32 fcoder_metacmd_ID_long_undo_jump = 256;
static i32 fcoder_metacmd_ID_long_undo_same_pos = 257;
static i32 fcoder_metacmd_ID_long_write_text_and_auto_indent = 258;
static i32 fcoder_metacmd_ID_long_write_to_file_all_declarations = 259;
static i32 fcoder_metacmd_ID_long_write_to_file_all_definitions = 260;
static i32 fcoder_metacmd_ID_long_write_to_file_all_empty_scopes = 261;
static i32 fcoder_metacmd_ID_long_write_to_file_all_identifiers = 262;
static i32 fcoder_metacmd_ID_make_directory_query = 263;
static i32 fcoder_metacmd_ID_miblo_decrement_basic = 264;
static i32 fcoder_metacmd_ID_miblo_decrement_time_stamp = 265;
static i32 fcoder_metacmd_ID_miblo_decrement_time_stamp_minute = 266;
static i32 fcoder_metacmd_ID_miblo_increment_basic = 267;
static i32 fcoder_metacmd_ID_miblo_increment_time_stamp = 268;
static i32 fcoder_metacmd_ID_miblo_increment_time_stamp_minute = 269;
static i32 fcoder_metacmd_ID_mouse_wheel_change_face_size = 270;
static i32 fcoder_metacmd_ID_mouse_wheel_scroll = 271;
static i32 fcoder_metacmd_ID_move_down = 272;
static i32 fcoder_metacmd_ID_move_down_10 = 273;
static i32 fcoder_metacmd_ID_move_down_textual = 274;
static i32 fcoder_metacmd_ID_move_down_to_blank_line = 275;
static i32 fcoder_metacmd_ID_move_down_to_blank_line_end = 276;
static i32 fcoder_metacmd_ID_move_down_to_blank_line_skip_whitespace = 277;
static i32 fcoder_metacmd_ID_move_left = 278;
static i32 fcoder_metacmd_ID_move_left_alpha_numeric_boundary = 279;
static i32 fcoder_metacmd_ID_move_left_alpha_numeric_or_camel_boundary = 280;
static i32 fcoder_metacmd_ID_move_left_token_boundary = 281;
static i32 fcoder_metacmd_ID_move_left_whitespace_boundary = 282;
static i32 fcoder_metacmd_ID_move_left_whitespace_or_token_boundary = 283;
static i32 fcoder_metacmd_ID_move_line_down = 284;
static i32 fcoder_metacmd_ID_move_line_up = 285;
static i32 fcoder_metacmd_ID_move_right = 286;
static i32 fcoder_metacmd_ID_move_right_alpha_numeric_boundary = 287;
static i32 fcoder_metacmd_ID_move_right_alpha_numeric_or_camel_boundary = 288;
static i32 fcoder_metacmd_ID_move_right_token_boundary = 289;
static i32 fcoder_metacmd_ID_move_right_whitespace_boundary = 290;
static i32 fcoder_metacmd_ID_move_right_whitespace_or_token_boundary = 291;
static i32 fcoder_metacmd_ID_move_up = 292;
static i32 fcoder_metacmd_ID_move_up_10 = 293;
static i32 fcoder_metacmd_ID_move_up_to_blank_line = 294;
static i32 fcoder_metacmd_ID_move_up_to_blank_line_end = 295;
static i32 fcoder_metacmd_ID_move_up_to_blank_line_skip_whitespace = 296;
static i32 fcoder_metacmd_ID_multi_paste = 297;
static i32 fcoder_metacmd_ID_multi_paste_interactive = 298;
static i32 fcoder_metacmd_ID_multi_paste_interactive_quick = 299;
static i32 fcoder_metacmd_ID_music_start = 300;
static i32 fcoder_metacmd_ID_music_stop = 301;
static i32 fcoder_metacmd_ID_open_all_code = 302;
static i32 fcoder_metacmd_ID_open_all_code_recursive = 303;
static i32 fcoder_metacmd_ID_open_file_in_quotes = 304;
static i32 fcoder_metacmd_ID_open_in_other = 305;
static i32 fcoder_metacmd_ID_open_long_braces = 306;
static i32 fcoder_metacmd_ID_open_long_braces_break = 307;
static i32 fcoder_metacmd_ID_open_long_braces_semicolon = 308;
static i32 fcoder_metacmd_ID_open_matching_file_cpp = 309;
static i32 fcoder_metacmd_ID_open_panel_hsplit = 310;
static i32 fcoder_metacmd_ID_open_panel_vsplit = 311;
static i32 fcoder_metacmd_ID_page_down = 312;
static i32 fcoder_metacmd_ID_page_up = 313;
static i32 fcoder_metacmd_ID_paste = 314;
static i32 fcoder_metacmd_ID_paste_and_indent = 315;
static i32 fcoder_metacmd_ID_paste_next = 316;
static i32 fcoder_metacmd_ID_paste_next_and_indent = 317;
static i32 fcoder_metacmd_ID_place_in_scope = 318;
static i32 fcoder_metacmd_ID_play_with_a_counter = 319;
static i32 fcoder_metacmd_ID_profile_clear = 320;
static i32 fcoder_metacmd_ID_profile_disable = 321;
static i32 fcoder_metacmd_ID_profile_enable = 322;
static i32 fcoder_metacmd_ID_profile_inspect = 323;
static i32 fcoder_metacmd_ID_project_command_F1 = 324;
static i32 fcoder_metacmd_ID_project_command_F10 = 325;
static i32 fcoder_metacmd_ID_project_command_F11 = 326;
static i32 fcoder_metacmd_ID_project_command_F12 = 327;
static i32 fcoder_metacmd_ID_project_command_F13 = 328;
static i32 fcoder_metacmd_ID_project_command_F14 = 329;
static i32 fcoder_metacmd_ID_project_command_F15 = 330;
static i32 fcoder_metacmd_ID_project_command_F16 = 331;
static i32 fcoder_metacmd_ID_project_command_F2 = 332;
static i32 fcoder_metacmd_ID_project_command_F3 = 333;
static i32 fcoder_metacmd_ID_project_command_F4 = 334;
static i32 fcoder_metacmd_ID_project_command_F5 = 335;
static i32 fcoder_metacmd_ID_project_command_F6 = 336;
static i32 fcoder_metacmd_ID_project_command_F7 = 337;
static i32 fcoder_metacmd_ID_project_command_F8 = 338;
static i32 fcoder_metacmd_ID_project_command_F9 = 339;
static i32 fcoder_metacmd_ID_project_command_lister = 340;
static i32 fcoder_metacmd_ID_project_fkey_command = 341;
static i32 fcoder_metacmd_ID_project_go_to_root_directory = 342;
static i32 fcoder_metacmd_ID_project_reprint = 343;
static i32 fcoder_metacmd_ID_query_replace = 344;
static i32 fcoder_metacmd_ID_query_replace_identifier = 345;
static i32 fcoder_metacmd_ID_query_replace_selection = 346;
static i32 fcoder_metacmd_ID_quick_swap_buffer = 347;
static i32 fcoder_metacmd_ID_redo = 348;
static i32 fcoder_metacmd_ID_redo_all_buffers = 349;
static i32 fcoder_metacmd_ID_rename_file_query = 350;
static i32 fcoder_metacmd_ID_reopen = 351;
static i32 fcoder_metacmd_ID_replace_in_all_buffers = 352;
static i32 fcoder_metacmd_ID_replace_in_buffer = 353;
static i32 fcoder_metacmd_ID_replace_in_range = 354;
static i32 fcoder_metacmd_ID_reverse_search = 355;
static i32 fcoder_metacmd_ID_reverse_search_identifier = 356;
static i32 fcoder_metacmd_ID_save = 357;
static i32 fcoder_metacmd_ID_save_all_dirty_buffers = 358;
static i32 fcoder_metacmd_ID_save_to_query = 359;
static i32 fcoder_metacmd_ID_search = 360;
static i32 fcoder_metacmd_ID_search_identifier = 361;
static i32 fcoder_metacmd_ID_seek_beginning_of_line = 362;
static i32 fcoder_metacmd_ID_seek_beginning_of_textual_line = 363;
static i32 fcoder_metacmd_ID_seek_end_of_line = 364;
static i32 fcoder_metacmd_ID_seek_end_of_textual_line = 365;
static i32 fcoder_metacmd_ID_select_all = 366;
static i32 fcoder_metacmd_ID_select_next_scope_absolute = 367;
static i32 fcoder_metacmd_ID_select_next_scope_after_current = 368;
static i32 fcoder_metacmd_ID_select_prev_scope_absolute = 369;
static i32 fcoder_metacmd_ID_select_prev_top_most_scope = 370;
static i32 fcoder_metacmd_ID_select_surrounding_scope = 371;
static i32 fcoder_metacmd_ID_select_surrounding_scope_maximal = 372;
static i32 fcoder_metacmd_ID_set_eol_mode_from_contents = 373;
static i32 fcoder_metacmd_ID_set_eol_mode_to_binary = 374;
static i32 fcoder_metacmd_ID_set_eol_mode_to_crlf = 375;
static i32 fcoder_metacmd_ID_set_eol_mode_to_lf = 376;
static i32 fcoder_metacmd_ID_set_face_size = 377;
static i32 fcoder_metacmd_ID_set_face_size_this_buffer = 378;
static i32 fcoder_metacmd_ID_set_mark = 379;
static i32 fcoder_metacmd_ID_set_mode_to_notepad_like = 380;
static i32 fcoder_metacmd_ID_set_mode_to_original = 381;
static i32 fcoder_metacmd_ID_setup_build_bat = 382;
static i32 fcoder_metacmd_ID_setup_build_bat_and_sh = 383;
static i32 fcoder_metacmd_ID_setup_build_sh = 384;
static i32 fcoder_metacmd_ID_setup_new_project = 385;
static i32 fcoder_metacmd_ID_show_filebar = 386;
static i32 fcoder_metacmd_ID_show_scrollbar = 387;
static i32 fcoder_metacmd_ID_show_the_log_graph = 388;
static i32 fcoder_metacmd_ID_snipe_backward_whitespace_or_token_boundary = 389;
static i32 fcoder_metacmd_ID_snipe_forward_whitespace_or_token_boundary = 390;
static i32 fcoder_metacmd_ID_snippet_lister = 391;
static i32 fcoder_metacmd_ID_string_repeat = 392;
static i32 fcoder_metacmd_ID_suppress_mouse = 393;
static i32 fcoder_metacmd_ID_swap_panels = 394;
static i32 fcoder_metacmd_ID_switch_to_keybinding_0 = 395;
static i32 fcoder_metacmd_ID_switch_to_keybinding_1 = 396;
static i32 fcoder_metacmd_ID_switch_to_keybinding_2 = 397;
static i32 fcoder_metacmd_ID_switch_to_keybinding_3 = 398;
static i32 fcoder_metacmd_ID_theme_lister = 399;
static i32 fcoder_metacmd_ID_to_lowercase = 400;
static i32 fcoder_metacmd_ID_to_uppercase = 401;
static i32 fcoder_metacmd_ID_toggle_command_server = 402;
static i32 fcoder_metacmd_ID_toggle_filebar = 403;
static i32 fcoder_metacmd_ID_toggle_fps_meter = 404;
static i32 fcoder_metacmd_ID_toggle_fullscreen = 405;
static i32 fcoder_metacmd_ID_toggle_highlight_enclosing_scopes = 406;
static i32 fcoder_metacmd_ID_toggle_highlight_line_at_cursor = 407;
static i32 fcoder_metacmd_ID_toggle_line_numbers = 408;
static i32 fcoder_metacmd_ID_toggle_line_wrap = 409;
static i32 fcoder_metacmd_ID_toggle_mouse = 410;
static i32 fcoder_metacmd_ID_toggle_paren_matching_helper = 411;
static i32 fcoder_metacmd_ID_toggle_show_whitespace = 412;
static i32 fcoder_metacmd_ID_toggle_virtual_whitespace = 413;
static i32 fcoder_metacmd_ID_tutorial_maximize = 414;
static i32 fcoder_metacmd_ID_tutorial_minimize = 415;
static i32 fcoder_metacmd_ID_uncomment_line = 416;
static i32 fcoder_metacmd_ID_undo = 417;
static i32 fcoder_metacmd_ID_undo_all_buffers = 418;
static i32 fcoder_metacmd_ID_view_buffer_other_panel = 419;
static i32 fcoder_metacmd_ID_view_jump_list_with_lister = 420;
static i32 fcoder_metacmd_ID_word_complete = 421;
static i32 fcoder_metacmd_ID_word_complete_drop_down = 422;
static i32 fcoder_metacmd_ID_write_block = 423;
static i32 fcoder_metacmd_ID_write_hack = 424;
static i32 fcoder_metacmd_ID_write_note = 425;
static i32 fcoder_metacmd_ID_write_space = 426;
static i32 fcoder_metacmd_ID_write_text_and_auto_indent = 427;
static i32 fcoder_metacmd_ID_write_text_input = 428;
static i32 fcoder_metacmd_ID_write_todo = 429;
static i32 fcoder_metacmd_ID_write_underscore = 430;
static i32 fcoder_metacmd_ID_write_zero_struct = 431;
#endif

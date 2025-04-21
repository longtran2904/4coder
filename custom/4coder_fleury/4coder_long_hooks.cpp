
//~ NOTE(long): Essential Hooks

// @COPYPASTA(long): default_try_exit
CUSTOM_COMMAND_SIG(long_try_exit)
CUSTOM_DOC("Command for responding to a try-exit event")
{
    User_Input input = get_current_input(app);
    if (match_core_code(&input, CoreCode_TryExit))
    {
        b32 do_exit = 1;
        if (!allow_immediate_close_without_checking_for_changes)
        {
            Scratch_Block scratch(app);
            String8List dirty_list = {};
            
            for (Buffer_ID buffer = get_buffer_next(app, 0, Access_Always); buffer;
                 buffer = get_buffer_next(app, buffer, Access_Always))
            {
                Dirty_State dirty = buffer_get_dirty_state(app, buffer);
                if (HasFlag(dirty, DirtyState_UnsavedChanges))
                {
                    String8 name = push_buffer_unique_name(app, scratch, buffer);
                    string_list_push(scratch, &dirty_list, name);
                }
            }
            
            u64 dirty_count = dirty_list.node_count;
            if (dirty_count)
            {
                String8 dirty_names = string_list_flatten(scratch, dirty_list, S8Lit(", "), 0, 0);
                String8 query = S8Lit("There is one dirty buffer, close anyway?");
                if (dirty_count > 1)
                    query = push_stringf(scratch, "There are %llu dirty buffers, close anyway?", dirty_count);
                
                Lister_Choice_List list = {};
                lister_choice(scratch, &list, "(N)o",            "", KeyCode_N, SureToKill_No);
                lister_choice(scratch, &list, "(Y)es",           "", KeyCode_Y, SureToKill_Yes);
                lister_choice(scratch, &list, "(S)ave", dirty_names, KeyCode_S, SureToKill_Save);
                
                do_exit = 0;
                Lister_Choice* choice = get_choice_from_user(app, query, list);
                
                if (choice != 0)
                {
                    switch (choice->user_data)
                    {
                        case SureToKill_Save: save_all_dirty_buffers(app);
                        case SureToKill_Yes:
                        {
                            allow_immediate_close_without_checking_for_changes = 1;
                            do_exit = 1;
                        } break;
                    }
                }
            }
        }
        
        if (do_exit)
            hard_exit(app);
    }
}

function void Long_MC_Pull(Application_Links *app, View_ID view, MC_Command_Kind kind, MC_Node* node)
{
    node->cursor_pos = view_get_cursor_pos(app, view);
    node->mark_pos = view_get_mark_pos(app, view);
    if (kind == MC_Command_CursorCopy)
        node->clipboard = push_clipboard_index(&mc_context.arena_clipboard, 0, 0);
}

function void Long_MC_Push(Application_Links *app, View_ID view, MC_Command_Kind kind, MC_Node* node)
{
    view_set_cursor(app, view, seek_pos(node->cursor_pos));
    view_set_mark(app, view, seek_pos(node->mark_pos));
    if (kind == MC_Command_CursorPaste)
        MCi_clipboard_push(node->clipboard);
}

// @COPYPASTA(long): MC_apply
function void Long_MC_Apply(Application_Links* app, Managed_Scope scope, Custom_Command_Function* func, MC_Command_Kind kind)
{
    View_ID view = mc_context.view;
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    i64 old_pos = view_get_cursor_pos(app, view);
    
    // Run standard cursor
    default_pre_command(app, scope);
    func(app);
    Long_ConfirmMarkSnapToCursor(app);
    
    // Check that `func` should be safe to re-run
    if (view != get_active_view(app, Access_Always) || buffer != view_get_buffer(app, view, Access_Always))
    {
        END:
        MC_end(app);
        return;
    }
    
    b32 pos_has_changed = old_pos != view_get_cursor_pos(app, view);
    Command_Metadata* meta = get_command_metadata(func);
    if (meta && meta->is_ui && pos_has_changed)
        goto END;
    
    Input_Event event = get_current_input(app).event;
    // When the user is moving the mouse, end MC mode when the cursor position has changed
    if ((event.kind == InputEventKind_MouseMove || event.kind == InputEventKind_MouseButton ||
         event.kind == InputEventKind_MouseButtonRelease) && pos_has_changed)
        goto END;
    
    // @CONSIDER(long): Handle the case where the user switches views by clicking
    //if (event.kind == InputEventKind_Core && event.core.code == CoreCode_ClickActivateView);
    
    if (kind == MC_Command_Global) return;
    
    // Since copy doesn't read from MC clipboards, clear it before doing multiple copies
    // _CopyPaste would require double-buffering arenas, rather not complicate for nothing
    if (kind == MC_Command_CursorCopy)
        linalloc_clear(&mc_context.arena_clipboard);
    
    Buffer_Scroll scroll = view_get_buffer_scroll(app, mc_context.view);
    String8 clipboard = MCi_clipboard_pull();
    MC_Node prev = {mc_context.cursors};
    Long_MC_Pull(app, mc_context.view, kind, &prev);
    mc_context.cursors = &prev;
    
    for (MC_Node* n = prev.next; n; n = n->next)
    {
        Long_MC_Push(app, mc_context.view, kind, n);
        Long_SnapMarkToCursor(app);
        
        mc_context.active_cursor = n;
        func(app);
        
        Long_ConfirmMarkSnapToCursor(app);
        Long_MC_Pull(app, mc_context.view, kind, n);
    }
    
    Rewrite_Type* next_rewrite = scope_attachment(app, scope, view_next_rewrite_loc, Rewrite_Type);
    if (next_rewrite)
    {
        if (*next_rewrite != Rewrite_NoChange)
        {
            Rewrite_Type* rewrite = scope_attachment(app, scope, view_rewrite_loc, Rewrite_Type);
            *rewrite = *next_rewrite;
        }
    }
    
    mc_context.active_cursor = 0;
    mc_context.cursors = prev.next;
    
    Long_MC_Push(app, mc_context.view, kind, &prev);
    MCi_clipboard_push(clipboard);
    view_set_buffer_scroll(app, mc_context.view, scroll, SetBufferScroll_NoCursorChange);
}

CUSTOM_COMMAND_SIG(long_view_input_handler)
CUSTOM_DOC("Input consumption loop for default view behavior")
{
    Scratch_Block scratch(app);
    default_input_handler_init(app, scratch);
    
    View_ID view = get_this_ctx_view(app, Access_Always);
    Managed_Scope scope = view_get_managed_scope(app, view);
    
    for (;;)
    {
        // NOTE(allen): Get input
        User_Input input = get_next_input(app, EventPropertyGroup_Any, 0);
        Input_Event* event = &input.event;
        if (input.abort)
            break;
        
        // NOTE(allen): Mouse Suppression
        Event_Property event_properties = get_event_properties(event);
        if (suppressing_mouse && (event_properties & EventPropertyGroup_AnyMouseEvent) != 0)
            continue;
        
        // NOTE(allen): Get binding
        if (!implicit_map_function)
            implicit_map_function = default_implicit_map;
        Implicit_Map_Result map_result = implicit_map_function(app, 0, 0, event);
        if (map_result.command == 0)
        {
            leave_current_input_unhandled(app);
            continue;
        }
        
        if (mc_context.active && mc_context.view == get_this_ctx_view(app, Access_Always))
        {
            if (event->kind == InputEventKind_Core && event->core.code == CoreCode_NewClipboardContents)
            {
                linalloc_clear(&mc_context.arena_clipboard);
                String8 string = push_string_copy(&mc_context.arena_clipboard, event->core.string);
                for_mc(node, mc_context.cursors)
                    node->clipboard = string;
            }
            
            Table_Lookup lookup = table_lookup(&mc_context.table, HandleAsU64(map_result.command));
            if (lookup.found_match)
            {
                u64 val = mc_context.table.vals[lookup.index];
                Long_MC_Apply(app, scope, map_result.command, MC_Command_Kind(val));
            }
            
            else
            {
                MC_apply(app, MC_error_fade, MC_Command_Cursor);
                Command_Metadata* meta = get_command_metadata(map_result.command);
                Long_Print_Messagef(app, "[MC] <%s> not available during multi-cursor mode\n", meta ? meta->name : "Command");
            }
        }
        
        else
        {
            // NOTE(allen): Run the command and pre/post command stuff
            default_pre_command(app, scope);
            map_result.command(app);
            default_post_command(app, scope);
        }
    }
}

//~ NOTE(long): Render Hooks

// @COPYPASTA(long): F4_RenderBuffer
function void Long_RenderBuffer(Application_Links* app, View_ID view_id, Buffer_ID buffer, Face_ID face_id,
                                Text_Layout_ID text_layout_id,
                                Rect_f32 rect, Frame_Info frame_info)
{
    Scratch_Block scratch(app);
    ProfileScope(app, "[Long] Render Buffer");
    
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
    
    View_ID active_view = get_active_view(app, Access_Always);
    b32 is_active_view = (active_view == view_id);
    
    Rect_f32 prev_clip = draw_set_clip(app, rect);
    Rect_f32 view_rect = view_get_screen_rect(app, view_id);
    
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    String8 buffer_name = push_buffer_base_name(app, scratch, buffer);
    
    i64 cursor_pos = view_correct_cursor(app, view_id);
    view_correct_mark(app, view_id);
    
    f32 cursor_roundness, mark_thickness;
    {
        Face_Metrics metrics = get_face_metrics(app, face_id);
        u64 cursor_roundness_100 = def_get_config_u64_lit(app, "cursor_roundness");
        cursor_roundness = metrics.normal_advance*cursor_roundness_100*0.01f;
        mark_thickness = (f32)def_get_config_u64_lit(app, "mark_thickness");
    }
    
    // NOTE(allen): Token colorizing
    Token_Array token_array = get_token_array_from_buffer(app, buffer);
    if (token_array.tokens != 0)
    {
        Long_SyntaxHighlight(app, text_layout_id, &token_array);
        
        // NOTE(allen): Scan for TODOs and NOTEs
        b32 use_comment_keywords = def_get_config_b32_lit("use_comment_keywords");
        if (use_comment_keywords)
        {
            String8 user_name = def_get_config_str_lit(scratch, "user_name");
            Comment_Highlight_Pair pairs[] =
            {
                {S8Lit("NOTE"), finalize_color(defcolor_comment_pop, 0)},
                {S8Lit("TODO"), finalize_color(defcolor_comment_pop, 1)},
                {    user_name, finalize_color(fleury_color_comment_user_name, 0)},
            };
            Long_Render_CommentHighlight(app, buffer, text_layout_id, &token_array, pairs, ArrayCount(pairs));
        }
    }
    else
        paint_text_color_fcolor(app, text_layout_id, visible_range, fcolor_id(defcolor_text_default));
    
    // NOTE(allen): Scope highlight
    if (def_get_config_b32_lit("use_scope_highlight"))
    {
        Color_Array colors = finalize_color_array(defcolor_back_cycle);
        draw_scope_highlight(app, buffer, text_layout_id, cursor_pos, colors.vals, colors.count);
    }
    
    // NOTE(rjf): Brace highlight
#ifdef FCODER_FLEURY_BRACE_H
    {
        Color_Array colors = finalize_color_array(fleury_color_brace_highlight);
        if (colors.count >= 1 && F4_ARGBIsValid(colors.vals[0]))
            F4_Brace_RenderHighlight(app, buffer, text_layout_id, cursor_pos, colors.vals, colors.count);
    }
#endif
    
    // NOTE(allen): Line highlight
    {
        b32 highlight_line_at_cursor = def_get_config_b32_lit("highlight_line_at_cursor");
        b32 special_buffer = string_match(buffer_name, S8Lit("*compilation*")) || string_match(buffer_name, S8Lit("*search*"));
        if (highlight_line_at_cursor && (is_active_view || special_buffer))
        {
            i64 line = get_line_number_from_pos(app, buffer, cursor_pos);
            draw_line_highlight(app, text_layout_id, line, fcolor_id(defcolor_highlight_cursor_line));
        }
    }
    
    // NOTE(rjf): Error/Search Highlight
    {
        b32 use_error_highlight = def_get_config_b32_lit("use_error_highlight");
        b32  use_jump_highlight = def_get_config_b32_lit( "use_jump_highlight");
        
        if (use_error_highlight || use_jump_highlight)
        {
            Buffer_ID compilation_buffer = get_buffer_by_name(app, S8Lit("*compilation*"), Access_Always);
            
            // NOTE(allen): Error highlight
            if (use_error_highlight)
                Long_Highlight_DrawErrors(app, buffer, text_layout_id, compilation_buffer);
            
            // NOTE(allen): Search highlight
            if (use_jump_highlight)
            {
                Buffer_ID jump_buffer = get_locked_jump_buffer(app);
                if (jump_buffer != compilation_buffer)
                    draw_jump_highlights(app, buffer, text_layout_id, jump_buffer, fcolor_id(defcolor_highlight_white));
            }
        }
    }
    
    Long_Highlight_DrawRangeList(app, view_id, buffer, text_layout_id, cursor_roundness);
    Long_Highlight_DrawRange(app, view_id, buffer, text_layout_id, cursor_roundness);
    
    // NOTE(jack): Token Occurrence Highlight
    if (!def_get_config_b32_lit("f4_disable_cursor_token_occurance")) 
    {
        ProfileScope(app, "[Long] Token Occurrence Highlight");
        
        // NOTE(jack): Get the active cursor's token string
        Buffer_ID active_buffer = view_get_buffer(app, active_view, Access_Always);
        i64 active_cursor_pos = view_get_cursor_pos(app, active_view);
        Token_Array active_array = get_token_array_from_buffer(app, active_buffer);
        Token_Iterator_Array cursor_it = token_iterator_pos(0, &active_array, active_cursor_pos);
        Token* cursor_token = token_it_read(&cursor_it);
        
        if (cursor_token)
        {
            // Loop the visible tokens
            i64 first_index = token_index_from_pos(&token_array, visible_range.first);
            Token_Iterator_Array it = token_iterator_index(0, &token_array, first_index);
            for (;;)
            {
                Token* token = token_it_read(&it);
                if (!token || token->pos >= visible_range.one_past_last)
                    break;
                
                if (token->kind == TokenBaseKind_Identifier)
                {
                    String8 cursor_lexeme = push_token_lexeme(app, scratch, active_buffer, cursor_token);
                    Range_i64 range = Ii64(token);
                    String8 lexeme = push_buffer_range(app, scratch, buffer, range);
                    
                    // NOTE(jack) If this is the buffers cursor token, highlight it with an Underline
                    if (range_contains(range, view_get_cursor_pos(app, view_id)))
                        F4_RenderRangeHighlight(app, view_id, text_layout_id, range, F4_RangeHighlightKind_Underline,
                                                Long_ARGBFromID(fleury_color_token_highlight));
                    // NOTE(jack): If the token matches the active buffer token. highlight it with a Minor Underline
                    else if (cursor_token->kind == TokenBaseKind_Identifier && string_match(lexeme, cursor_lexeme))
                        F4_RenderRangeHighlight(app, view_id, text_layout_id, range, F4_RangeHighlightKind_MinorUnderline,
                                                Long_ARGBFromID(fleury_color_token_minor_highlight)); 
                }
                
                if (!token_it_inc_non_whitespace(&it))
                    break;
            }
        }
    }
    
    // NOTE(jack): if "f4_disable_cursor_token_occurance" is set, just highlight the cusror 
    else
    {
        ProfileScope(app, "[Long] Token Highlight");
        Token_Iterator_Array it = token_iterator_pos(0, &token_array, cursor_pos);
        Token* token = token_it_read(&it);
        if (token && token->kind == TokenBaseKind_Identifier)
            F4_RenderRangeHighlight(app, view_id, text_layout_id, Ii64(token), F4_RangeHighlightKind_Underline,
                                    Long_ARGBFromID(fleury_color_token_highlight));
    }
    
    // NOTE(rjf): Flashes
    F4_RenderFlashes(app, view_id, text_layout_id);
    
    // NOTE(allen): Color parens
    if (def_get_config_b32_lit("use_paren_helper"))
    {
        Color_Array colors = finalize_color_array(defcolor_text_cycle);
        draw_paren_highlight(app, buffer, text_layout_id, cursor_pos, colors.vals, colors.count);
    }
    
    // NOTE(rjf): Divider Comments
    Long_Render_DividerComments(app, buffer, text_layout_id);
    
    // NOTE(rjf): Cursor Mark Range
    if (is_active_view && fcoder_mode == FCoderMode_Original)
        Long_HighlightCursorMarkRange(app, view_id);
    
    // NOTE(long): Jump list highlights
    Long_MC_DrawHighlights(app, view_id, buffer, text_layout_id, cursor_roundness, mark_thickness, is_active_view);
    Long_Render_HexColor(app, view_id, buffer, text_layout_id);
    
    // NOTE(rjf): Cursor
    switch (fcoder_mode)
    {
        case FCoderMode_Original:      Long_Render_EmacsCursor(app, view_id, buffer, text_layout_id, frame_info,
                                                               cursor_roundness, mark_thickness, is_active_view); break;
        case FCoderMode_NotepadLike: Long_Render_NotepadCursor(app, view_id, buffer, text_layout_id, frame_info,
                                                               cursor_roundness, mark_thickness, is_active_view); break;
    }
    
#ifdef FCODER_FLEURY_BRACE_H
    // NOTE(rjf): Brace annotations/lines
    F4_Brace_RenderCloseBraceAnnotation(app, buffer, text_layout_id, cursor_pos);
    F4_Brace_RenderLines(app, buffer, view_id, text_layout_id, cursor_pos);
#endif
    
    // NOTE(allen): Fade ranges
    paint_fade_ranges(app, text_layout_id, buffer);
    
    // NOTE(allen): put the actual text on the actual screen
    draw_text_layout_default(app, text_layout_id);
    
#ifdef FCODER_FLEURY_CALC_H
    // NOTE(rjf): Interpret buffer as calc code, if it's the calc buffer.
    if (string_match(buffer_name, S8Lit("*calc*")))
        F4_CLC_RenderBuffer(app, buffer, view_id, text_layout_id, frame_info);
    
    // NOTE(rjf): Draw calc comments.
    Long_Render_CalcComments(app, view_id, buffer, text_layout_id, frame_info);
#endif
    
    draw_set_clip(app, prev_clip);
    
    // NOTE(rjf): Draw tooltips and stuff.
    if (is_active_view)
    {
        // NOTE(rjf): Position context helper
        F4_Language* language = F4_LanguageFromBuffer(app, buffer);
        if (language)
            Long_Index_DrawPosContext(app, view_id, language->PosContext(app, scratch, buffer, cursor_pos));
        
        // NOTE(rjf): Draw tooltip list.
        {
            Mouse_State mouse = get_mouse_state(app);
            Face_ID tooltip_face_id = global_small_code_face;
            Face_Metrics tooltip_face_metrics = get_face_metrics(app, tooltip_face_id);
            
            Rect_f32 tooltip_rect =
            {
                (f32)mouse.x + 16,
                (f32)mouse.y + 16,
                (f32)mouse.x + 16,
                (f32)mouse.y + 16 + tooltip_face_metrics.line_height + 8,
            };
            
            for (i32 i = 0; i < global_tooltip_count; ++i)
            {
                String8 string = global_tooltips[i].string;
                tooltip_rect.x1 = tooltip_rect.x0;
                tooltip_rect.x1 += get_string_advance(app, tooltip_face_id, string) + 4;
                
                if (tooltip_rect.x1 > view_rect.x1)
                {
                    f32 difference = tooltip_rect.x1 - view_rect.x1;
                    tooltip_rect.x1 = (f32)(i32)(tooltip_rect.x1 - difference);
                    tooltip_rect.x0 = (f32)(i32)(tooltip_rect.x0 - difference);
                }
                
                Long_Render_TooltipRect(app, tooltip_rect);
                draw_string(app, tooltip_face_id, string, V2f32(tooltip_rect.x0 + 4, tooltip_rect.y0 + 4),
                            global_tooltips[i].color);
                
                f32 height = rect_height(tooltip_rect) + 1;
                tooltip_rect.y0 += height;
                tooltip_rect.y1 += height;
            }
        }
    }
    
    // NOTE(rjf): Draw inactive rectangle
    else
    {
        ARGB_Color color = Long_ARGBFromID(fleury_color_inactive_pane_overlay);
        if (F4_ARGBIsValid(color))
            draw_rectangle(app, view_rect, 0.f, color);
    }
    
    // NOTE(rjf): Render code peek.
    if (!view_get_is_passive(app, view_id) && !is_active_view)
        Long_Index_DrawCodePeek(app, view_id);
}

// @COPYPASTA(long): F4_DrawFileBar
function void Long_DrawFileBar(Application_Links* app, View_ID view_id, Buffer_ID buffer, Face_ID face_id, Rect_f32 bar)
{
    Scratch_Block scratch(app);
    
    FColor base_color = fcolor_id(defcolor_base);
    FColor pop2_color = fcolor_id(defcolor_pop2);
    Buffer_Cursor cursor = view_compute_cursor(app, view_id, seek_pos(view_get_cursor_pos(app, view_id)));
    
    draw_rectangle_fcolor(app, bar, 0.f, fcolor_id(defcolor_bar));
    
    if (!def_get_config_b32_lit("f4_disable_progress_bar"))
    {
        f32 progress = (f32)cursor.line / (f32)buffer_get_line_count(app, buffer);
        Rect_f32 progress_bar_rect =
        {
            bar.x0,
            bar.y0,
            bar.x0 + (bar.x1 - bar.x0) * progress,
            bar.y1,
        };
        
        ARGB_Color progress_bar_color = Long_ARGBFromID(fleury_color_file_progress_bar);
        if (F4_ARGBIsValid(progress_bar_color))
            draw_rectangle(app, progress_bar_rect, 0, progress_bar_color);
    }
    
    Fancy_Line list = {};
    String_Const_u8 unique_name = push_buffer_unique_name(app, scratch, buffer);
    push_fancy_string(scratch, &list, base_color, unique_name);
    
    if (buffer == get_comp_buffer(app))
    {
        String8 str = push_whole_buffer(app, scratch, buffer);
        u32 counts[2] = {};
        for (u64 i = 0; i < str.size; ++i)
        {
            String8 searches[] = { S8Lit(": error"), S8Lit(": warning") };
            for (u64 search = 0; search < ArrayCount(searches); ++search)
            {
                String8 substr = string_substring(str, Ii64_size(i, searches[search].size));
                if (string_match(substr, searches[search], StringMatch_CaseInsensitive))
                {
                    i += substr.size - 1;
                    counts[search]++;
                    break;
                }
            }
        }
        
        i64 line_count = buffer_get_line_count(app, buffer);
        push_fancy_stringf(scratch, &list, base_color,
                           " - Line Count: %3llu Row: %3.lld Col: %3.lld - %3u Error(s) %3u Warning(s)",
                           line_count, cursor.line, cursor.col, counts[0], counts[1]);
    }
    else
    {
        push_fancy_stringf(scratch, &list, base_color, ": %d", buffer);
        push_fancy_stringf(scratch, &list, base_color, " - Row: %3.lld Col: %3.lld Pos: %4lld -",
                           cursor.line, cursor.col, cursor.pos);
        
        Managed_Scope scope = buffer_get_managed_scope(app, buffer);
        Line_Ending_Kind* eol_setting = scope_attachment(app, scope, buffer_eol_setting, Line_Ending_Kind);
        switch (*eol_setting)
        {
            case LineEndingKind_Binary: push_fancy_string(scratch, &list, base_color, S8Lit( " bin")); break;
            case LineEndingKind_LF:     push_fancy_string(scratch, &list, base_color, S8Lit(  " lf")); break;
            case LineEndingKind_CRLF:   push_fancy_string(scratch, &list, base_color, S8Lit(" crlf")); break;
        }
        
        u8 space[3];
        {
            Dirty_State dirty = buffer_get_dirty_state(app, buffer);
            String_u8 str = Su8(space, 0, 3);
            if (dirty != DirtyState_UpToDate)
                string_append(&str, S8Lit(" "));
            if (HasFlag(dirty, DirtyState_UnsavedChanges))
                string_append(&str, S8Lit("*"));
            if (HasFlag(dirty, DirtyState_UnloadedChanges))
                string_append(&str, S8Lit("!"));
            push_fancy_string(scratch, &list, pop2_color, str.string);
        }
        
        b32 enable_virtual_whitespace = def_get_config_b32_lit("enable_virtual_whitespace");
        push_fancy_stringf(scratch, &list, base_color, " Virtual Whitespace: %s",
                           enable_virtual_whitespace ? "On" : "Off");
        
        push_fancy_stringf(scratch, &list, base_color, " - Left/Right: %s/%s",
                           (long_global_move_side>>1) ? "Max" : "Min", (long_global_move_side&1) ? "Max" : "Min");
    }
    
    draw_fancy_line(app, face_id, fcolor_zero(), &list, bar.p0 + V2f32(2.f, 2.f));
}

function void Long_Render(Application_Links* app, Frame_Info frame_info, View_ID view_id)
{
#ifdef FCODER_FLEURY_RECENT_FILES_H
    F4_RecentFiles_RefreshView(app, view_id);
#endif
    
    ProfileScope(app, "[Long] Render");
    Scratch_Block scratch(app);
    
    View_ID active_view = get_active_view(app, Access_Always);
    b32 is_active_view = (active_view == view_id);
    
    f32 margin_size = (f32)def_get_config_u64_lit(app, "f4_margin_size");
    Rect_f32 view_rect = view_get_screen_rect(app, view_id);
    Rect_f32 region = rect_inner(view_rect, margin_size);
    
    Buffer_ID buffer = view_get_buffer(app, view_id, Access_Always);
    String_Const_u8 buffer_name = push_buffer_base_name(app, scratch, buffer);
    
    //- NOTE(long): Draw background
    {
        ARGB_Color color = Long_ARGBFromID(defcolor_back);
        if (string_match(buffer_name, S8Lit("*compilation*")))
            color = color_blend(color, 0.5f, 0xff000000);
        
        // NOTE(rjf): Inactive background color.
        else if (!is_active_view)
        {
            ARGB_Color inactive_bg_color = Long_ARGBFromID(fleury_color_inactive_pane_background);
            if (F4_ARGBIsValid(inactive_bg_color))
                color = inactive_bg_color;
        }
        
        draw_rectangle(app, region, 0.f, color);
        draw_margin(app, view_rect, region, color);
    }
    
    //- NOTE(long): Draw margin
    {
        ARGB_Color color = Long_ARGBFromID(defcolor_margin);
        if (def_get_config_b32_lit("f4_margin_use_mode_color") && is_active_view)
            color = Long_Color_Cursor(app, 0, GlobalKeybindingMode);
        draw_margin(app, view_rect, region, color);
    }
    
    Rect_f32 prev_clip = draw_set_clip(app, region);
    
    Face_ID face_id = get_face_id(app, buffer);
    f32 line_height, digit_advance;
    {
        Face_Metrics face_metrics = get_face_metrics(app, face_id);
        line_height = face_metrics.line_height;
        digit_advance = face_metrics.decimal_digit_advance;
    }
    
    // NOTE(allen): file bar
    b64 showing_file_bar = false;
    if (view_get_setting(app, view_id, ViewSetting_ShowFileBar, &showing_file_bar) && showing_file_bar)
    {
        Face_ID global_face = get_face_id(app, 0);
        Face_Metrics global_metrics = get_face_metrics(app, global_face);
        
        Rect_f32_Pair pair = layout_file_bar_on_top(region, global_metrics.line_height);
        Long_DrawFileBar(app, view_id, buffer, global_face, pair.min);
        region = pair.max;
    }
    
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view_id);
    {
        Buffer_Point_Delta_Result delta = delta_apply(app, view_id, frame_info.animation_dt, scroll);
        if (!block_match_struct(&scroll.position, &delta.point))
        {
            block_copy_struct(&scroll.position, &delta.point);
            view_set_buffer_scroll(app, view_id, scroll, SetBufferScroll_NoCursorChange);
        }
        
        if (delta.still_animating)
            animate_in_n_milliseconds(app, 0);
    }
    
    // NOTE(allen): query bars
    {
        Query_Bar* space[32];
        Query_Bar_Ptr_Array query_bars = {};
        query_bars.ptrs = space;
        if (get_active_query_bars(app, view_id, ArrayCount(space), &query_bars))
        {
            for (i32 i = 0; i < query_bars.count; i += 1)
            {
                Rect_f32_Pair pair = layout_query_bar_on_top(region, line_height, 1);
                draw_query_bar(app, query_bars.ptrs[i], face_id, pair.min);
                region = pair.max;
            }
        }
    }
    
    // NOTE(allen): FPS hud
    if (show_fps_hud)
    {
        Rect_f32_Pair pair = layout_fps_hud_on_bottom(region, line_height);
        draw_fps_hud(app, frame_info, face_id, pair.max);
        region = pair.min;
        animate_in_n_milliseconds(app, 1000);
    }
    
    // NOTE(allen): layout line numbers
    Rect_f32 line_number_rect = {};
    if (def_get_config_b32_lit("show_line_number_margins"))
    {
        Rect_f32_Pair pair = layout_line_number_margin(app, buffer, region, digit_advance);
        line_number_rect = pair.min;
        line_number_rect.x1 += 4;
        region = pair.max;
    }
    
    // NOTE(allen): begin buffer render
    Text_Layout_ID text_layout_id = text_layout_create(app, buffer, region, scroll.position);
    {
        // NOTE(allen): draw line numbers
        if (def_get_config_b32_lit("show_line_number_margins"))
            Long_Render_LineOffsetNumber(app, view_id, buffer, face_id, text_layout_id, line_number_rect);
        
        // NOTE(allen): draw the buffer
        Long_RenderBuffer(app, view_id, buffer, face_id, text_layout_id, region, frame_info);
    }
    text_layout_free(app, text_layout_id);
    
    draw_set_clip(app, prev_clip);
}

//~ NOTE(rjf): Tick Hook

// @COPYPASTA(long): F4_Tick
function void Long_Tick(Application_Links* app, Frame_Info frame_info)
{
    linalloc_clear(&global_frame_arena);
    global_tooltip_count = 0;
    
    View_ID view = get_active_view(app, 0);
    if (!view_get_is_passive(app, view))
        long_global_active_view = view;
    
    F4_TickColors(app, frame_info);
    Long_Index_Tick(app);
#ifdef FCODER_FLEURY_CALC_H
    F4_CLC_Tick(frame_info);
#endif
    F4_UpdateFlashes(app, frame_info);
    
    // NOTE(rjf): Default tick stuff from the 4th dimension:
    default_tick(app, frame_info);
    
    MC_tick_inner(app, frame_info);
}

//~ NOTE(long): Buffer Hooks

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

function i32 Long_SaveFile(Application_Links* app, Buffer_ID buffer)
{
    ProfileScope(app, "[Long] Save File");
    Scratch_Block scratch(app);
    String8 name = push_buffer_base_name(app, scratch, buffer);
    String8 path = push_buffer_file_name(app, scratch, buffer);
    
    // @COPYPASTA(long): default_file_save
    {
        b32 auto_indent = def_get_config_b32_lit("automatically_indent_text_on_save");
        b32  is_virtual = def_get_config_b32_lit("enable_virtual_whitespace");
        //History_Record_Index index = buffer_history_get_current_state_index(app, buffer);
        if (auto_indent && is_virtual /*&& index*/)
            Long_Index_IndentBuffer(app, buffer, buffer_range(app, buffer), true);
        
        Managed_Scope scope = buffer_get_managed_scope(app, buffer);
        Line_Ending_Kind* eol = scope_attachment(app, scope, buffer_eol_setting, Line_Ending_Kind);
        switch (*eol)
        {
            case LineEndingKind_LF:   rewrite_lines_to_lf  (app, buffer); break;
            case LineEndingKind_CRLF: rewrite_lines_to_crlf(app, buffer); break;
        }
    }
    
    //- NOTE(long): Update history
    {
        Long_Buffer_History  current = Long_Buffer_GetCurrentHistory (app, buffer);
        Long_Buffer_History* history = Long_Buffer_GetAttachedHistory(app, buffer);
        *history = current;
    }
    
    //- NOTE(long): Print saving message
#if 0
    {
        Date_Time date_time = system_now_date_time_universal();
        date_time = system_local_date_time_from_universal(&date_time);
        String8 date = date_time_format(scratch, "dd/mm/yyyy hh24:mimi:ss", &date_time);
        
        f32 size = (f32)buffer_get_size(app, buffer) / 1024;
        Long_Print_Messagef(app, "Saving Buffer: \"%.*s\" (%.*s) %.2fKiB\n",
                            string_expand(name), string_expand(date), size);
    }
#endif
    
    String8 theme_name = {0};
    {
        String8 target_prefix = S8Lit("theme-");
        String8 target_suffix = S8Lit(".4coder");
        String8 actual_prefix =  string_prefix(name, target_prefix.size);
        String8 actual_suffix = string_postfix(name, target_suffix.size);
        if (string_match(actual_prefix, target_prefix) && string_match(actual_suffix, target_suffix))
            theme_name = string_chop(name, target_suffix.size);
    }
    
    //- NOTE(long): Hot-reloading
    String8 data = push_whole_buffer(app, scratch, buffer);
    if (theme_name.size)
    {
        for (Color_Table_Node* node = global_theme_list.first; node != 0; node = node->next)
        {
            if (string_match(node->name, theme_name))
            {
                Color_Table color_table = make_color_table(app, scratch);
                Config* config = theme_parse__buffer(app, scratch, buffer, scratch, &color_table);
                String8 error_str = config_stringize_errors(app, scratch, config);
                
                if (error_str.size)
                    Long_Print_Error(app, error_str);
                else for (i64 i = 0; i < color_table.count; i += 1)
                {
                    Color_Array* dst_array = &active_color_table.arrays[i];
                    Color_Array* src_array = &color_table.arrays[i];
                    dst_array->count = src_array->count;
                    block_copy(dst_array->vals, src_array->vals, src_array->count*sizeof(ARGB_Color));
                }
                
                break;
            }
        }
    }
    
    else if (string_match(name, S8Lit("config.4coder")))
    {
        Face_ID face = get_face_id(app, buffer);
        Face_Description desc = get_face_description(app, face);
        Long_Config_ApplyFromData(app, data, desc.parameters.pt_size, desc.parameters.hinting);
    }
    
    else if (string_match(name, S8Lit("bindings.4coder")))
    {
        if (Long_Binding_LoadData(app, &framework_mapping, path, data))
        {
            String_ID global_map = vars_save_string_lit("keys_global");
            String_ID file_map = vars_save_string_lit("keys_file");
            String_ID code_map = vars_save_string_lit("keys_code");
            Long_Binding_SetupEssential(&framework_mapping, global_map, file_map, code_map);
        }
    }
    
    else if (string_match(name, S8Lit("project.4coder")))
        Long_Prj_Parse(app, path, data);
    
    return 0;
}

// @COPYPASTA(long): F4_LayoutInner
function Layout_Item_List Long_Layout(Application_Links* app, Arena* arena, Buffer_ID buffer, Range_i64 range, Face_ID face, f32 width)
{
    Scratch_Block scratch(app);
    Layout_Item_List list = get_empty_item_list(range);
    String_Const_u8 text = push_buffer_range(app, scratch, buffer, range);
    
    LefRig_TopBot_Layout_Vars pos_vars;
    {
        Face_Advance_Map advance_map = get_face_advance_map(app, face);
        Face_Metrics metrics = get_face_metrics(app, face);
        f32 tab_width = (f32)def_get_config_u64_lit(app, "default_tab_width");
        tab_width = clamp_bot(1, tab_width);
        pos_vars = get_lr_tb_layout_vars(&advance_map, &metrics, tab_width, width);
    }
    
    if (text.size)
    {
        b32 skipping_leading_whitespace = 0;
        Newline_Layout_Vars newline_vars = get_newline_layout_vars();
        
        u8* ptr = text.str;
        u8* end_ptr = ptr + text.size;
        while (ptr < end_ptr)
        {
            Character_Consume_Result consume = utf8_consume(ptr, (u64)(end_ptr - ptr));
            i64 index = layout_index_from_ptr(ptr, text.str, range.first);
            
            switch (consume.codepoint)
            {
                case '\t':
                case ' ':
                {
                    newline_layout_consume_default(&newline_vars);
                    f32 advance = lr_tb_advance(&pos_vars, face, consume.codepoint);
                    if (!skipping_leading_whitespace)
                        lr_tb_write_with_advance(&pos_vars, face, advance, arena, &list, index, consume.codepoint);
                    else
                        lr_tb_advance_x_without_item(&pos_vars, advance);
                } break;
                
                default:
                {
                    newline_layout_consume_default(&newline_vars);
                    lr_tb_write(&pos_vars, face, arena, &list, index, consume.codepoint);
                } break;
                
                case '\r': newline_layout_consume_CR(&newline_vars, index); break;
                
                case '\n':
                {
                    i64 newline_index = newline_layout_consume_LF(&newline_vars, index);
                    lr_tb_write_blank(&pos_vars, face, arena, &list, newline_index);
                    lr_tb_next_line(&pos_vars);
                } break;
                
                case max_u32:
                {
                    newline_layout_consume_default(&newline_vars);
                    lr_tb_write_byte(&pos_vars, face, arena, &list, index, *ptr);
                } break;
            }
            
            ptr += consume.inc;
        }
        
        if (newline_layout_consume_finish(&newline_vars))
        {
            i64 index = layout_index_from_ptr(ptr, text.str, range.first);
            lr_tb_write_blank(&pos_vars, face, arena, &list, index);
        }
    }
    else
        lr_tb_write_blank(&pos_vars, face, arena, &list, range.first);
    
    layout_item_list_finish(&list, -pos_vars.line_to_text_shift);
    return list;
}

//~ NOTE(long): F4 Clone Hooks

// @COPYPASTA(long): F4_DoFullLex_ASYNC
function void Long_DoFullLex_ASYNC(Async_Context* actx, String8 data)
{
    if (data.size == sizeof(Buffer_ID))
    {
        Application_Links* app = actx->app;
        ProfileScope(app, "[F4] Async Lex");
        Scratch_Block scratch(app);
        Buffer_ID buffer = *(Buffer_ID*)data.str;
        
        String8 contents = {};
        {
            ProfileBlock(app, "[F4] Async Lex Contents (before mutex)");
            acquire_global_frame_mutex(app);
            ProfileBlock(app, "[F4] Async Lex Contents (after mutex)");
            contents = push_whole_buffer(app, scratch, buffer);
            release_global_frame_mutex(app);
        }
        
        Token_List list = {};
        
        F4_Language* language = F4_LanguageFromBuffer(app, buffer);
        if (!language)
            language = F4_LanguageFromString(S8Lit("cpp"));
        
        b32 canceled = false;
        if (language)
        {
            void* lexing_state = push_array_zero(scratch, u8, language->lex_state_size);
            language->LexInit(lexing_state, contents);
            
            for (;;)
            {
                ProfileBlock(app, "[F4] Async Lex Block");
                i32 limit_factor = 10000;
                if (language->LexFullInput(scratch, &list, lexing_state, limit_factor))
                    break;
                
                if (async_check_canceled(actx))
                {
                    canceled = true;
                    break;
                }
            }
        }
        
        if (!canceled)
        {
            ProfileBlock(app, "[F4] Async Lex Save Results (before mutex)");
            acquire_global_frame_mutex(app);
            ProfileBlock(app, "[F4] Async Lex Save Results (after mutex)");
            
            Managed_Scope scope = buffer_get_managed_scope(app, buffer);
            if (scope != 0)
            {
                Base_Allocator* allocator = managed_scope_allocator(app, scope);
                Token_Array* tokens_ptr = scope_attachment(app, scope, attachment_tokens, Token_Array);
                base_free(allocator, tokens_ptr->tokens);
                
                Token_Array tokens = {};
                tokens.tokens = base_array(allocator, Token, list.total_count);
                tokens.count = list.total_count;
                tokens.max = list.total_count;
                
                token_fill_memory_from_list(tokens.tokens, &list);
                block_copy_struct(tokens_ptr, &tokens);
            }
            
            buffer_mark_as_modified(buffer);
            release_global_frame_mutex(app);
        }
    }
}

// @COPYPASTA(long): F4_BeginBuffer
function BUFFER_HOOK_SIG(Long_BeginBuffer)
{
    ProfileScope(app, "[Long] Begin Buffer");
    
    Scratch_Block scratch(app);
    b32 treat_as_code = false;
    String_Const_u8 file_name = push_buffer_file_name(app, scratch, buffer_id);
    String_Const_u8 buffer_name = push_buffer_base_name(app, scratch, buffer_id);
    
    // NOTE(rjf): Treat as code if the config tells us to.
    if (treat_as_code == false)
    {
        if (file_name.size > 0)
        {
            String_Const_u8 treat_as_code_string = def_get_config_str_lit(scratch, "treat_as_code");
            String_Const_u8_Array extensions = parse_extension_line_to_extension_list(app, scratch, treat_as_code_string);
            String_Const_u8 ext = string_file_extension(file_name);
            for (i32 i = 0; i < extensions.count; ++i)
            {
                if (string_match(ext, extensions.strings[i]))
                {
                    treat_as_code = true;
                    break;
                }
            }
        }
    }
    
    // NOTE(rjf): Treat as code for *calc* and *peek* buffers.
    if (treat_as_code == false && buffer_name.size > 0)
        if (string_match(buffer_name, S8Lit("*calc*")) || string_match(buffer_name, S8Lit("*peek*")))
            treat_as_code = true;
    
    // NOTE(rjf): Treat as code if we've identified the language of a file.
    if (treat_as_code == false)
    {
        F4_Language* language = F4_LanguageFromBuffer(app, buffer_id);
        if (language)
            treat_as_code = true;
    }
    
    String_ID file_map_id = vars_save_string_lit("keys_file");
    String_ID code_map_id = vars_save_string_lit("keys_code");
    
    Command_Map_ID map_id = treat_as_code ? code_map_id : file_map_id;
    Managed_Scope scope = buffer_get_managed_scope(app, buffer_id);
    Command_Map_ID* map_id_ptr = scope_attachment(app, scope, buffer_map_id, Command_Map_ID);
    *map_id_ptr = map_id;
    
    Line_Ending_Kind setting = guess_line_ending_kind_from_buffer(app, buffer_id);
    Line_Ending_Kind* eol_setting = scope_attachment(app, scope, buffer_eol_setting, Line_Ending_Kind);
    *eol_setting = setting;
    
    // NOTE(allen): Decide buffer settings
    b32 wrap_lines = true;
    b32 use_lexer = false;
    if (treat_as_code)
    {
        wrap_lines = def_get_config_b32_lit("enable_code_wrapping");
        use_lexer = true;
    }
    
    if (string_match(buffer_name, S8Lit("*compilation*")))
        wrap_lines = false;
    
    if (use_lexer)
    {
        ProfileBlock(app, "begin buffer kick off lexer");
        Async_Task* lex_task_ptr = scope_attachment(app, scope, buffer_lex_task, Async_Task);
        *lex_task_ptr = async_task_no_dep(&global_async_system, Long_DoFullLex_ASYNC, make_data_struct(&buffer_id));
    }
    
    {
        b32* wrap_lines_ptr = scope_attachment(app, scope, buffer_wrap_lines, b32);
        *wrap_lines_ptr = wrap_lines;
    }
    
    if (use_lexer)
        buffer_set_layout(app, buffer_id, layout_virt_indent_index_generic);
    else if (treat_as_code)
        buffer_set_layout(app, buffer_id, layout_virt_indent_literal_generic);
    else
        buffer_set_layout(app, buffer_id, layout_generic);
    
    // no meaning for return
    return 0;
}

// @COPYPASTA(long): F4_BufferEditRange
function BUFFER_EDIT_RANGE_SIG(Long_BufferEditRange)
{
    // buffer_id, new_range, original_size
    ProfileScope(app, "[Long] Buffer Edit Range");
    Scratch_Block scratch(app);
    
    MC_buffer_edit_range_inner(app, buffer_id, new_range, old_cursor_range);
    
    Range_i64 old_range = Ii64(old_cursor_range.min.pos, old_cursor_range.max.pos);
    buffer_shift_fade_ranges(buffer_id, old_range.max, (new_range.max - old_range.max));
    
    {
        code_index_lock();
        Code_Index_File* file = code_index_get_file(buffer_id);
        if (file)
            code_index_shift(file, old_range, range_size(new_range));
        code_index_unlock();
    }
    
    Managed_Scope scope = buffer_get_managed_scope(app, buffer_id);
    i64 text_shift = replace_range_shift(old_range, range_size(new_range));
    b32 do_full_relex = false;
    
    Async_Task* lex_task_ptr = scope_attachment(app, scope, buffer_lex_task, Async_Task);
    if (async_task_is_running_or_pending(&global_async_system, *lex_task_ptr))
    {
        async_task_cancel(app, &global_async_system, *lex_task_ptr);
        buffer_unmark_as_modified(buffer_id);
        do_full_relex = true;
        *lex_task_ptr = 0;
    }
    
    Token_Array* ptr = scope_attachment(app, scope, attachment_tokens, Token_Array);
    if (ptr != 0 && ptr->tokens != 0)
    {
        ProfileBlockNamed(app, "attempt resync", profile_attempt_resync);
        
        i64 token_index_first = token_relex_first(ptr, old_range.first, 1);
        i64 token_index_resync_guess = token_relex_resync(ptr, old_range.one_past_last, 16);
        
        if (token_index_resync_guess - token_index_first >= 4000)
            do_full_relex = true;
        else
        {
            Token* token_first = ptr->tokens + token_index_first;
            Token* token_resync = ptr->tokens + token_index_resync_guess;
            
            Range_i64 relex_range = Ii64(token_first->pos, token_resync->pos + token_resync->size + text_shift);
            String_Const_u8 partial_text = push_buffer_range(app, scratch, buffer_id, relex_range);
            
            // NOTE(rjf): Lex
            F4_Language* language = F4_LanguageFromBuffer(app, buffer_id);
            // NOTE(rjf): Fall back to C++ if we don't have a proper language.
            if (!language)
                language = F4_LanguageFromString(S8Lit("cpp"));
            
            Token_List relex_list = F4_Language_LexFullInput_NoBreaks(app, language, scratch, partial_text);
            if (relex_range.one_past_last < buffer_get_size(app, buffer_id))
                token_drop_eof(&relex_list);
            
            Token_Relex relex = token_relex(relex_list, relex_range.first - text_shift, ptr->tokens,
                                            token_index_first, token_index_resync_guess);
            ProfileCloseNow(profile_attempt_resync);
            
            if (!relex.successful_resync)
                do_full_relex = true;
            else
            {
                ProfileBlock(app, "apply resync");
                Base_Allocator* allocator = managed_scope_allocator(app, scope);
                i64 token_index_resync = relex.first_resync_index;
                
                Range_i64 head = Ii64(0, token_index_first);
                Range_i64 replaced = Ii64(token_index_first, token_index_resync);
                Range_i64 tail = Ii64(token_index_resync, ptr->count);
                
                i64 resynced_count = (token_index_resync_guess + 1) - token_index_resync;
                i64 relexed_count = relex_list.total_count - resynced_count;
                i64 tail_shift = relexed_count - (token_index_resync - token_index_first);
                
                i64 new_tokens_count = ptr->count + tail_shift;
                Token* new_tokens = base_array(allocator, Token, new_tokens_count);
                
                Token* old_tokens = ptr->tokens;
                block_copy_array_shift(new_tokens, old_tokens, head, 0);
                token_fill_memory_from_list(new_tokens + replaced.first, &relex_list, relexed_count);
                for (i64 i = 0, index = replaced.first; i < relexed_count; i += 1, index += 1)
                    new_tokens[index].pos += relex_range.first;
                for (i64 i = tail.first; i < tail.one_past_last; i += 1)
                    old_tokens[i].pos += text_shift;
                block_copy_array_shift(new_tokens, ptr->tokens, tail, tail_shift);
                
                base_free(allocator, ptr->tokens);
                
                ptr->tokens = new_tokens;
                ptr->count = new_tokens_count;
                ptr->max = new_tokens_count;
                
                buffer_mark_as_modified(buffer_id);
            }
        }
    }
    
    if (do_full_relex)
        *lex_task_ptr = async_task_no_dep(&global_async_system, Long_DoFullLex_ASYNC, make_data_struct(&buffer_id));
    
    // no meaning for return
    return 0;
}

// @COPYPASTA(long): F4_DeltaRule
function DELTA_RULE_SIG(Long_DeltaRule)
{
    Vec2_f32* velocity = (Vec2_f32*)data;
    if (velocity->x == 0.f)
        *velocity = { 1.f, 1.f };
    
    Smooth_Step step_x = smooth_camera_step(pending.x, velocity->x, 80.f, 1.f/4.f);
    Smooth_Step step_y = smooth_camera_step(pending.y, velocity->y, 80.f, 1.f/4.f);
    *velocity = V2f32(step_x.v, step_y.v);
    
    return V2f32(step_x.p, step_y.p);
}

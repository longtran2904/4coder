
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

//~ NOTE(rjf): Tick Hook

// @COPYPASTA(long): F4_Tick
function void Long_Tick(Application_Links* app, Frame_Info frame)
{
    linalloc_clear(&global_frame_arena);
    long_fancy_tooltips = {};
    global_tooltip_count = 0;
    
    View_ID view = get_active_view(app, 0);
    if (!view_get_is_passive(app, view))
        long_global_active_view = view;
    
    F4_TickColors(app, frame);
    Long_Index_Tick(app);
#ifdef FCODER_FLEURY_CALC_H
    F4_CLC_Tick(frame);
#endif
    F4_UpdateFlashes(app, frame);
    
    // NOTE(rjf): Default tick stuff from the 4th dimension:
    default_tick(app, frame);
    
    MC_tick_inner(app, frame);
}

//~ NOTE(long): Render Hooks

//- GLOBAL
// mode, bind_by_physical_key
// use_file_bars, use_error_highlight, use_jump_highlight, use_scope_highlight, use_paren_helper, use_comment_keywords
// show_line_number_margins, long_show_line_number_offset, enable_output_wrapping (?), enable_undo_fade_out
// cursor_roundness, mark_thickness, lister_roundness
// enable_virtual_whitespace, virtual_whitespace_regular_indent, enable_code_wrapping
// toggle_mouse, show_fps_hud, global face size

//- BUFFER
// toggle_show_whitespace, toggle_file_bar, fps
// buffer face size, buffer name, buffer size, line count, current location (row/col/pos)

//- LONG
// long_global_move_side
// f4_poscontext_draw_at_bottom_of_buffer, long_global_pos_context_open
// long_global_child_tooltip_count, long_active_pos_context_option
// long_lister_tooltip_peek

function void Long_Render(Application_Links* app, Frame_Info frame, View_ID view)
{
#ifdef FCODER_FLEURY_RECENT_FILES_H
    F4_RecentFiles_RefreshView(app, view);
#endif
    
    ProfileScope(app, "[Long] Render");
    Scratch_Block scratch(app);
    
    //- NOTE(long): Setup Context
    Long_Render_Context _ctx = Long_Render_CreateCtx(app, frame, view, 0);
    Long_Render_Context* ctx = &_ctx;
    
    Rect_f32 view_rect = ctx->rect;
    f32 margin_size = def_get_config_f32_lit(app, "long_margin_size");
    ctx->rect = rect_inner(view_rect, margin_size);
    
    //- NOTE(long): Draw Background/Margin
    b32 is_active_view = ctx->is_active_view;
    {
        ARGB_Color back_color = Long_ARGBFromID(defcolor_back);
        if (!is_active_view)
        {
            ARGB_Color inactive_bg_color = Long_ARGBFromID(fleury_color_inactive_pane_background);
            if (F4_ARGBIsValid(inactive_bg_color))
                back_color = inactive_bg_color;
        }
        draw_rectangle(app, ctx->rect, 0, back_color);
        
        ARGB_Color margin_color = Long_ARGBFromID(defcolor_margin);
        if (def_get_config_b32_lit("f4_margin_use_mode_color") && is_active_view)
            margin_color = Long_Color_Cursor();
        draw_margin(app, view_rect, ctx->rect, margin_color);
    }
    
    //- NOTE(long): Draw Panel
    Rect_f32 prev_clip = draw_set_clip(app, ctx->rect);
    
    // NOTE(long): File Bar
    b64 showing_file_bar = false;
    if (view_get_setting(app, view, ViewSetting_ShowFileBar, &showing_file_bar) && showing_file_bar)
        Long_Render_FileBar(ctx);
    
    // NOTE(long): Buffer Scroll
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
    {
        Buffer_Point_Delta_Result delta = delta_apply(app, view, frame.animation_dt, scroll);
        if (!block_match_struct(&scroll.position, &delta.point))
        {
            block_copy_struct(&scroll.position, &delta.point);
            view_set_buffer_scroll(app, view, scroll, SetBufferScroll_NoCursorChange);
        }
        if (delta.still_animating)
            animate_in_n_milliseconds(app, 0);
    }
    
    // NOTE(long): Query Bars
    Query_Bar* space[32];
    Query_Bar_Ptr_Array query_bars = {space};
    if (get_active_query_bars(app, view, ArrayCount(space), &query_bars))
    {
        for (i32 i = 0; i < query_bars.count; ++i)
        {
            Rect_f32_Pair pair = layout_query_bar_on_top(ctx->rect, ctx->metrics.line_height, 1);
            draw_query_bar(app, query_bars.ptrs[i], ctx->face, pair.min);
            // TODO(long): Query bar background or margin
            ctx->rect = pair.max;
        }
    }
    
    //- NOTE(long): Draw Line/Buffer Region
    Rect_f32 layout_rect = ctx->rect;
    draw_set_clip(app, layout_rect);
    
    b32 show_line_number = def_get_config_b32_lit("show_line_number_margins");
    String8 buffer_name = push_buffer_base_name(app, scratch, ctx->buffer);
    if (show_line_number && !Long_Buffer_IsSpecial(app, buffer_name))
        layout_rect = Long_Render_LineNumber(ctx);
    
    // TODO(long): Helper function
    f32 cursor_width = Long_Render_CursorThickness(ctx);
    f32  emacs_width = f32_ceil32(ctx->metrics.text_height/2 - cursor_width); // Long_Render_EmacsSymbol
    if (is_active_view && fcoder_mode == FCoderMode_Original)
    {
        f32 highlight_width  = clamp_top(cursor_width * 2.f, emacs_width / 2.f);
        f32 highlight_offset = clamp_bot((emacs_width - highlight_width) / 2.f, 0.f);
        Long_Highlight_CursorMark(app, layout_rect.x0 + highlight_offset, highlight_width);
    }
    layout_rect.x0 += emacs_width;
    
    // NOTE(long): Buffer Layout
    Long_Render_InitCtxBuffer(ctx, ctx->buffer, layout_rect, scroll.position);
    Long_Render_Buffer(ctx);
    text_layout_free(app, ctx->layout);
    draw_set_clip(app, prev_clip);
    
    if (!is_active_view)
    {
        ARGB_Color overlay_color = Long_ARGBFromID(fleury_color_inactive_pane_overlay);
        if (F4_ARGBIsValid(overlay_color))
            draw_rectangle(app, view_rect, 0.f, overlay_color);
    }
}

function void Long_WholeScreenRender(Application_Links* app, Frame_Info frame)
{
    Scratch_Block scratch(app);
    View_ID active_view = get_active_view(app, 0);
    Buffer_ID active_buffer = view_get_buffer(app, active_view, 0);
    Rect_f32 region = global_get_screen_rectangle(app);
    Rect_f32 active_rect = view_get_screen_rect(app, active_view);
    
    Long_Render_Context _ctx = Long_Render_CreateCtx(app, frame, active_view, 1);
    Long_Render_Context* ctx = &_ctx;
    {
        // NOTE(long): Code Peek HUD
        ctx->rect = region;
        f32 center_x = (region.x1 + region.x0) / 2.f;
        if (active_rect.x0 < center_x)
            ctx->rect.x0 = center_x;
        else
            ctx->rect.x1 = center_x;
        Long_Index_DrawCodePeek(ctx);
        
        // NOTE(long): Position Context Rendering
        F4_Language* language = F4_LanguageFromBuffer(app, active_buffer);
        if (language)
        {
            i64 cursor_pos = view_get_cursor_pos(app, active_view);
            Long_Index_DrawPosContext(ctx, language->PosContext(app, scratch, active_buffer, cursor_pos));
        }
    }
    
    //- NOTE(long): Lister HUD
    Long_View_ForEach(view, app, 0)
    {
        ctx->view = view;
        ctx->is_active_view = view == active_view;
        
        Lister* lister = view_get_lister(app, view);
        if (lister)
        {
            Rect_f32 max_rect = view_get_screen_rect(app, view);
            Vec2_f32 max_size = rect_dim(max_rect);
            Vec2_f32 hud_size = hadamard(max_size, V2f32(.7f, .875f)); // @CONSTANT
            Rect_f32 hud_rect = Rf32_xy_wh(max_rect.p0 + (max_size - hud_size)/2.f, hud_size);
            
            ctx->rect = Long_Rf32_Round(hud_rect);
            Long_Lister_RenderHUD(ctx, lister, 0);
        }
    }
    
    // NOTE(long): FPS HUD
    if (show_fps_hud)
    {
        Long_Render_FPS(ctx);
        animate_in_n_milliseconds(app, 1000);
    }
    
    // NOTE(long): Tooltip Rendering
    Long_Render_SetCtxFace(ctx, global_small_code_face);
    {
        // @CONSTANT
        Vec2_f32 mouse_offset = V2f32(16, 16);
        f32 height_offset = 0;
        
        Vec2_f32 tooltip_pos = ctx->mouse + mouse_offset;
        for (Fancy_Line* line = long_fancy_tooltips.first; line; line = line->next)
        {
            Fancy_Block block = Long_Render_LayoutLine(ctx, scratch, line);
            tooltip_pos.y = Long_Render_FancyBlock(ctx, &block, tooltip_pos).y;
            tooltip_pos.y += height_offset;
        }
    }
}

//~ NOTE(long): Buffer Hooks

function i32 Long_EndBuffer(Application_Links* app, Buffer_ID buffer)
{
    F4_Index_Lock();
    F4_Index_File* file = F4_Index_LookupFile(app, buffer);
    if (file)
    {
        Long_Index_ClearFile(file);
        F4_Index_EraseFile(app, buffer);
    }
    F4_Index_Unlock();
    return end_buffer_close_jump_list(app, buffer);
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
            Long_Binding_SetupEssential(&framework_mapping);
    }
    
    else if (string_match(name, S8Lit("project.4coder")))
        Long_Prj_Parse(app, path, data);
    
    return 0;
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
    String8 file_name = push_buffer_file_name(app, scratch, buffer_id);
    String8 buffer_name = push_buffer_base_name(app, scratch, buffer_id);
    
    // NOTE(rjf): Treat as code if the config tells us to.
    if (treat_as_code == false)
    {
        if (file_name.size > 0)
        {
            String8 treat_as_code_string = def_get_config_str_lit(scratch, "treat_as_code");
            String8Array extensions = parse_extension_line_to_extension_list(app, scratch, treat_as_code_string);
            String8 ext = string_file_extension(file_name);
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
            String8 partial_text = push_buffer_range(app, scratch, buffer_id, relex_range);
            
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

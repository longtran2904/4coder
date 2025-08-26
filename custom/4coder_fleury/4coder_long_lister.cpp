
function void Long_Lister_AddItem(Application_Links* app, Lister* lister, String8 name, String8 tag,
                                  Buffer_ID buffer, i64 pos, u64 index, String8 tooltip)
{
    Long_Lister_Data* data = (Long_Lister_Data*)lister_add_item(lister, name, tag, 0, sizeof(Long_Lister_Data));
    *data = {};
    data->type = buffer ? Long_Header_Location : Long_Header_None;
    data->buffer = buffer;
    data->pos = pos;
    data->user_index = index;
    data->tooltip = tooltip;
    ((Lister_Node*)data - 1)->user_data = data;
}

function void Long_Lister_AddBuffer(Application_Links* app, Lister* lister, String8 name, String8 tag, Buffer_ID buffer)
{
    Long_Lister_Data* data = (Long_Lister_Data*)lister_add_item(lister, name, tag, 0, sizeof(Long_Lister_Data));
    *data = {};
    //data->type = Long_Header_Path;
    data->buffer = buffer;
    ((Lister_Node*)data - 1)->user_data = data;
}

function String8 Long_Lister_GetHeaderString(Application_Links* app, Arena* arena, Lister_Node* node)
{
    String8 result = {};
    if (node->user_data == (node + 1))
    {
        Long_Lister_Data data = *(Long_Lister_Data*)(node + 1);
        switch (data.type)
        {
            case Long_Header_Location:
            {
                String8 filename = push_buffer_unique_name(app, arena, data.buffer);
                Buffer_Cursor cursor = buffer_compute_cursor(app, data.buffer, seek_pos(data.pos));
                result = push_stringf(arena, "[%.*s:%d:%d] ", string_expand(filename), cursor.line, cursor.col);
            } break;
        }
    }
    return result;
}

// @COPYPASTA(long): lister_render
function void Long_Lister_RenderHUD(Long_Render_Context* ctx, Lister* lister, b32 show_file_bar)
{
    Application_Links* app = ctx->app;
    ProfileScope(app, "[Long] Render Lister");
    Scratch_Block scratch(app);
    Face_ID face = ctx->face;
    
    i32 item_count = lister->filtered.count;
    i32 item_index = lister->item_index;
    
    f32 margin_size = def_get_config_f32_lit(app, "lister_margin_size");
    f32 text_offset = f32_round32(ctx->metrics.normal_advance * .75f);
    f32 padding = def_get_config_f32_lit(app, "tooltip_padding") + margin_size;
    f32 lister_roundness = def_get_config_f32_lit(app, "lister_roundness");
    
    f32 line_height = ctx->metrics.line_height;
    f32 block_height = lister_get_block_height(line_height);
    f32 item_virtual_y = item_index * block_height;
    
    FColor pop1_color = fcolor_id(defcolor_pop1);
    FColor pop2_color = fcolor_id(defcolor_pop2);
    FColor text_color = fcolor_id(defcolor_text_default);
    
    //- @COPYPASTA(long): draw_background_and_margin
    ARGB_Color margin_color = Long_ARGBFromID(ctx->is_active_view ? defcolor_margin_active : defcolor_margin);
    Rect_f32 view_rect = ctx->rect;
    Rect_f32 prev_clip = Long_Render_SetClip(app, view_rect);
    ctx->rect = rect_inner(view_rect, margin_size);
    draw_rectangle(app, ctx->rect, 0.f, Long_ARGBFromID(defcolor_back));
    if (margin_size > 0.f)
        Long_Render_Outline(app, view_rect, lister_roundness, margin_size, margin_color);
    
    //- NOTE(long): File Bar
    if (show_file_bar)
        Long_Render_FileBar(ctx);
    
    //- NOTE(long): Query Bar
    Rect_f32 region = ctx->rect;
    Rect_f32_Pair pair = lister_get_top_level_layout(region, line_height + text_offset*2 + margin_size);
    {
        Rect_f32 margin_rect = pair.min;
        margin_rect.y0 = margin_rect.y1 - margin_size;
        draw_rectangle(app, margin_rect, 0, margin_color);
        
        Fancy_Line prompt = {}, text_field = {};
        push_fancy_string(scratch, &prompt, pop1_color, lister->query.string);
        push_fancy_stringf(scratch, &prompt, pop1_color, " (%d/%d) ", item_index + !!item_count, item_count);
        push_fancy_string(scratch, &text_field, text_color, lister->text_field.string);
        
        Rect_f32 query_rect = rect_inner(pair.min, text_offset);
        Vec2_f32 p = draw_fancy_line(app, face, fcolor_zero(), &prompt, query_rect.p0);
        f32 cap_width = query_rect.x1 - p.x;
        f32 overflow = get_fancy_line_width(app, face, &text_field) - cap_width;
        
        if (overflow > 0)
        {
            // NOTE(long): We don't need to reset the clip after draw_fancy_line
            draw_set_clip(app, Rf32(If32_size(p.x, cap_width), rect_range_y(query_rect)));
            p.x -= overflow;
        }
        draw_fancy_line(app, face, fcolor_zero(), &text_field, p);
    }
    
#define Long_Lister_PosFromVirtual(virtual_y) (list_rect.y0 - lister->scroll.position.y + (virtual_y))
    
    Rect_f32 list_rect = pair.max;
    Vec2_f32 list_size = rect_dim(list_rect);
    Lister_Node* current = lister->highlighted_node;
    Long_Lister_Data* item_data = current ? (Long_Lister_Data*)current->user_data : 0;
    
    //- NOTE(long): Tooltip Check
    if (item_data != (Long_Lister_Data*)(current+1) || !(long_lister_tooltip_peek & 0x80000000))
        item_data = 0;
    else if (long_lister_tooltip_peek == Long_Tooltip_NextToItem && !lister->set_vertical_focus_to_item)
    {
        f32 item_y_pos = Long_Lister_PosFromVirtual(item_virtual_y);
        if (item_y_pos <= list_rect.y0 - block_height || item_y_pos >= list_rect.y1)
            item_data = 0;
    }
    
    //- NOTE(long): Sizing Tooltip
    Rect_f32 screen_rect = global_get_screen_rectangle(app);
    f32 screen_extent = rect_width(screen_rect) * .5f;
    b32 draw_tooltip_left = region.x0 > rect_center(screen_rect).x;
    b32 under_item = long_lister_tooltip_peek == Long_Tooltip_UnderItem;
    Fancy_Block block = {};
    Vec2_f32 tooltip_size = {};
    
    f32 under_item_padding = line_height;
    f32  next_item_padding = line_height * .5f;
    if (item_data)
    {
        if (item_data->tooltip.size)
        {
            f32 under_total_padding = under_item_padding * 2;
            f32  next_total_padding =  next_item_padding * 2;
            
            f32 max_width = list_size.x - under_total_padding;
            if (!under_item)
                max_width = screen_extent + (screen_extent-list_size.x)*.5f - padding*2.f - next_total_padding;
            block = Long_Render_LayoutString(ctx, scratch, item_data->tooltip, max_width);
            tooltip_size = get_fancy_block_dim(app, face, &block);
            
            if (under_item)
            {
                tooltip_size.y += under_total_padding + block_height;
                tooltip_size.x = list_size.x;
            }
            else
                tooltip_size += Long_V2f32(next_total_padding);
            
#if 0 // NOTE(long): This will clamp the tooltip border
            else tooltip_size.x = clamp_top(tooltip_size.x, max_width + line_height);
#endif
        }
        
        else
        {
            tooltip_size = V2f32(list_size.x, line_height * 10); // @CONSTANT
            tooltip_size.y += block_height;
        }
    }
    else tooltip_size.y = block_height;
    
    //- NOTE(long): Auto-scroll
    if (lister->set_vertical_focus_to_item)
    {
        lister->set_vertical_focus_to_item = 0;
        f32 list_height = list_size.y;
        f32 item_height = tooltip_size.y;
        Range_f32 list_y = If32_size(lister->scroll.position.y, list_height);
        Range_f32 item_y = If32_size(           item_virtual_y, item_height);
        
        if (item_height >= list_height)
            lister->scroll.target.y = item_y.min;
        else if (list_y.min > item_y.min || item_y.max > list_y.max)
        {
            // @CONSTANT
            f32 margin = list_height*.3f;
            if ((long_lister_tooltip_peek & 0x80000000) && item_y.max > list_y.max)
                margin = clamp_top(margin, block_height * 2);
            else
                margin = clamp_top(margin, block_height * 3);
            
            if (item_y.min < list_y.min)
                lister->scroll.target.y = item_y.min - margin;
            else
                lister->scroll.target.y = item_y.max + margin - list_height;
        }
    }
    
    lister->visible_count = (i32)(list_size.y/block_height) - 3; // @CONSTANT
    lister->visible_count = clamp_bot(1, lister->visible_count);
    
    //- NOTE(long): Smooth Scroll
    f32 tooltip_advance = under_item * (tooltip_size.y - block_height);
    {
        Range_f32 scroll_range = If32(0.f, clamp_bot(0.f, (item_count-1)*block_height + tooltip_advance));
        lister->scroll.target.y = clamp_range(scroll_range, lister->scroll.target.y);
        lister->scroll.target.x = 0.f;
        
        Vec2_f32_Delta_Result delta = delta_apply(app, ctx->view, ctx->frame.animation_dt, lister->scroll);
        lister->scroll.position = delta.p;
        if (delta.still_animating)
            animate_in_n_milliseconds(app, 0);
        
        lister->scroll.position.y = clamp_range(scroll_range, lister->scroll.position.y);
        lister->scroll.position.x = 0.f;
    }
    
    //- NOTE(long): Windowing (Culling)
    Range_i64 index_range = {};
    {
        f32 scroll_y = lister->scroll.position.y;
        i64 size = i32_ceil32(list_size.y/block_height) + 1;
        index_range = Ii64_size(i32(scroll_y/block_height), size);
        
        if (tooltip_advance)
        {
            i64 last_index = index_range.max - 1;
            if (index_range.min > item_index)
            {
                index_range.min = (i32)((scroll_y-tooltip_advance)/block_height);
                index_range.min = clamp_bot(index_range.min, item_index);
                index_range.max = index_range.min + size;
            }
            
            else if (last_index > item_index)
            {
                last_index = index_range.min + i32((list_size.y-tooltip_advance)/block_height);
                last_index = clamp_bot(index_range.max, item_index);
                index_range.max = last_index + 1;
            }
        }
        
        index_range.max = clamp_top(index_range.max, item_count);
    }
    
    Managed_ID item_colors[] = { defcolor_list_item, defcolor_list_item_hover, defcolor_list_item_active };
    Range_f32 x = rect_range_x(list_rect);
    f32 y_offset = index_range.min > item_index ? tooltip_advance : 0;
    f32 y_pos = Long_Lister_PosFromVirtual(index_range.min * block_height + y_offset);
    
    //- NOTE(long): Draw Item
    for (i64 i = index_range.min; i < index_range.max; ++i)
    {
        ProfileBlock(app, "[Long] Render Lister Item");
        Range_f32 y = If32_size(y_pos, block_height);
        Rect_f32 item_outer = Rf32(x, y);
        Rect_f32 item_inner = rect_inner(item_outer, margin_size);
        
        Lister_Node* node = lister->filtered.node_ptrs[i];
        b32 hovered = rect_contains_point(item_outer, ctx->mouse);
        UI_Highlight_Level highlight = UIHighlight_None;
        
        if (node == current)
        {
            highlight = UIHighlight_Active;
            
            //- NOTE(long): Item Tooltip
            if (item_data)
            {
                f32 x_pos = list_rect.x0;
                if (!under_item)
                {
                    if (draw_tooltip_left)
                        x_pos = list_rect.x0 - padding - tooltip_size.x;
                    else
                        x_pos = list_rect.x1 + padding;
                }
                
                Rect_f32 tooltip_rect = Rf32_xy_wh(V2f32(x_pos, y.min), tooltip_size);
                Rect_f32 content_rect = tooltip_rect;
                if (under_item)
                    content_rect.y0 += block_height;
                
                Long_Render_SetClip(app, under_item ? list_rect : tooltip_rect);
                Long_Render_TooltipRect(app, tooltip_rect);
                
                if (item_data->tooltip.size)
                {
                    content_rect = rect_inner(content_rect, under_item ? under_item_padding : next_item_padding);
                    Rect_f32 content_clip = rect_intersect(content_rect, list_rect);
                    Long_Render_SetClip(app, under_item ? content_clip : content_rect);
                    draw_fancy_block(app, face, text_color, &block, content_rect.p0);
                }
                else Long_Render_DrawPeek(app, content_rect, item_data->buffer, Ii64(item_data->pos), 1);
            }
        }
        
        else if (node->user_data == lister->hot_user_data)
            highlight = hovered ? UIHighlight_Active : UIHighlight_Hover;
        else if (hovered)
            highlight = UIHighlight_Hover;
        
        //- NOTE(long): Item Rect
        Long_Render_SetClip(app, list_rect);
        Long_Render_BorderRect(app, item_outer, margin_size, lister_roundness,
                               Long_ARGBFromID(item_colors[highlight], 1), Long_ARGBFromID(item_colors[highlight], 0));
        
        //- NOTE(long): Item Content
        Fancy_Line line = {};
        push_fancy_string(scratch, &line, pop1_color, Long_Lister_GetHeaderString(app, scratch, node));
        push_fancy_string(scratch, &line, text_color, node->string);
        push_fancy_string(scratch, &line, S8Lit(" "));
        push_fancy_string(scratch, &line, pop2_color, node->status);
        
        Rect_f32 text_rect = rect_intersect(item_inner, list_rect);
        text_rect.x0 += text_offset;
        text_rect.x1 -= text_offset;
        Long_Render_SetClip(app, text_rect);
        
        Vec2_f32 p = V2f32(text_rect.x0, item_outer.y0 + (block_height - line_height) * .5f);
        p = draw_fancy_line(app, face, fcolor_zero(), &line, p);
        if (p.x > text_rect.x1 && hovered)
        {
            Fancy_Line* tooltip = Long_Render_PushTooltip(0, node->string, text_color);
            Long_Render_PushTooltip(tooltip, node->status, pop2_color);
        }
        
        //- NOTE(long): Advance Y
        y_pos = y.max;
        if (node == current)
            y_pos += tooltip_advance;
    }
    
    draw_set_clip(app, prev_clip);
}

function void Long_Lister_Render(Application_Links* app, Frame_Info frame, View_ID view)
{
    Lister* lister = view_get_lister(app, view);
    if (!lister)
        return;
    
    // TODO(long): Helper Function
    Long_Render_Context ctx = Long_Render_CreateCtx(app, frame, view);
    Long_Render_InitCtxFace(&ctx, get_face_id(app, 0));
    
    b64 show_file_bar = 0;
    view_get_setting(app, view, ViewSetting_ShowFileBar, &show_file_bar);
    if (show_file_bar)
        show_file_bar = !def_get_config_b32_lit("hide_file_bar_in_ui");
    
    Long_Lister_RenderHUD(&ctx, lister, (b32)show_file_bar);
}

// @COPYPASTA(long): lister_get_filtered
function Lister_Filtered Long_Lister_GetFilter(Application_Links* app, Arena* arena, Lister* lister)
{
    Scratch_Block scratch(app, arena);
    i32 node_count = lister->options.count;
    
    Lister_Filtered filtered = {};
    filtered.exact_matches.node_ptrs = push_array(arena, Lister_Node*, 1);
    filtered.before_extension_matches.node_ptrs = push_array(arena, Lister_Node*, node_count);
    filtered.substring_matches.node_ptrs = push_array(arena, Lister_Node*, node_count);
    
    String8 key = lister->key_string.string;
    if (key.size == 0)
    {
        // NOTE(long): This is both for optimizing and preventing an empty key from matching with an empty item
        // The latter is important if you want to match the order of Lister:filtered with the order of Lister:options
        for (Lister_Node* node = lister->options.first; node; node = node->next)
            filtered.substring_matches.node_ptrs[filtered.substring_matches.count++] = node;
        return filtered;
    }
    
    key = push_string_copy(scratch, key);
    string_mod_replace_character(key, '_', '*');
    string_mod_replace_character(key, ' ', '*');
    
    String8List absolutes = {};
    string_list_push(scratch, &absolutes, S8Lit(""));
    String8List splits = string_split(scratch, key, (u8*)"*", 1);
    b32 has_wildcard = splits.node_count > 1;
    string_list_push(&absolutes, &splits);
    string_list_push(scratch, &absolutes, S8Lit(""));
    
    String8List include_tags = {};
    String8List exclude_tags = {};
    String8List header_filters = {};
    
    for (String8Node** ptr = &absolutes.first; *ptr;)
    {
        String8List* list = 0;
        String8 string = (*ptr)->string;
        if (0) { }
        
        else if (string.str[0] == '\\')
            list = &include_tags;
        else if (string.str[0] == '~')
            list = &exclude_tags;
        else if (string.str[0] == '`')
            list = &header_filters;
        
        String8Node* node = *ptr;
        if (list)
        {
            *ptr = node->next;
            absolutes.node_count--;
            
            node->string = string_skip(string, 1);
            string_list_push(list, node);
        }
        else ptr = &node->next;
    }
    
    for (Lister_Node* node = lister->options.first; node; node = node->next)
    {
        for (String8Node* tag = include_tags.first; tag; tag = tag->next)
            if (!string_has_substr(node->status, tag->string, StringMatch_CaseInsensitive))
                goto NEXT;
        
        for (String8Node* tag = exclude_tags.first; tag; tag = tag->next)
            if (string_has_substr(node->status, tag->string, StringMatch_CaseInsensitive))
                goto NEXT;
        
        String8 header = Long_Lister_GetHeaderString(app, scratch, node);
        if (header.size)
            for (String8Node* tag = header_filters.first; tag; tag = tag->next)
                if (tag->string.size && !string_has_substr(header, tag->string, StringMatch_CaseInsensitive))
                    goto NEXT;
        
        String8 string = node->string;
        if (key.size == 0 || string_wildcard_match_insensitive(absolutes, string))
        {
            if (string_match_insensitive(string, key) && filtered.exact_matches.count == 0)
                filtered.exact_matches.node_ptrs[filtered.exact_matches.count++] = node;
            else if (key.size > 0 && !has_wildcard && StrStartsWith(string, key, StringMatch_CaseInsensitive) &&
                     node->string.size > key.size && node->string.str[key.size] == '.')
                filtered.before_extension_matches.node_ptrs[filtered.before_extension_matches.count++] = node;
            else
                filtered.substring_matches.node_ptrs[filtered.substring_matches.count++] = node;
        }
        
        NEXT:;
    }
    
    return filtered;
}

// @COPYPASTA(long): lister_update_filtered_list
function void Long_Lister_FilterList(Application_Links* app, Lister* lister)
{
    Scratch_Block scratch(app, lister->arena);
    
    Lister_Filtered filtered = Long_Lister_GetFilter(app, scratch, lister);
    Lister_Node_Ptr_Array node_ptr_arrays[] = {
        filtered.exact_matches,
        filtered.before_extension_matches,
        filtered.substring_matches,
    };
    
    end_temp(lister->filter_restore_point);
    
    i32 total_count = 0;
    for (i32 i = 0; i < ArrayCount(node_ptr_arrays); ++i)
        total_count += node_ptr_arrays[i].count;
    
    Lister_Node** node_ptrs = push_array(lister->arena, Lister_Node*, total_count);
    lister->filtered.node_ptrs = node_ptrs;
    lister->filtered.count = total_count;
    
    for (i32 i = 0, counter = 0; i < ArrayCount(node_ptr_arrays); ++i)
        for (i32 node_index = 0; node_index < node_ptr_arrays[i].count; ++node_index)
            node_ptrs[counter++] = node_ptr_arrays[i].node_ptrs[node_index];
    
    lister_update_selection_values(lister);
}

// @COPYPASTA(long): lister_call_refresh_handler
function void Long_Lister_Refresh(Application_Links* app, Lister* lister)
{
    if (lister->handlers.refresh)
    {
        lister->handlers.refresh(app, lister);
        lister->filter_restore_point = begin_temp(lister->arena);
        Long_Lister_FilterList(app, lister);
    }
}

function void Long_Lister_Reset(Application_Links* app, Lister* lister, b32 auto_select_first)
{
    lister->item_index = auto_select_first ? 0 : -1;
    lister_zero_scroll(lister);
    Long_Lister_FilterList(app, lister);
}

// NOTE(long): I could move this to the key_stroke handler if there were a way to change the `handled` bool.
// If only the idea of key_stroke was fleshed out just a little bit more and its signature was like this.
// @CONSIDER(long): Rather than hard-coded key, I can match each command's binding.
function b32 Long_Lister_HandleKeyStroke(Application_Links* app, Lister* lister, User_Input in)
{
    Scratch_Block scratch(app, lister->arena);
    b32 result = 0;
    
    switch (in.event.key.code)
    {
        case KeyCode_V:
        {
            if (has_modifier(&in, KeyCode_Control))
            {
                String8 string = push_clipboard_index(scratch, 0, 0);
                if (string.size)
                {
                    lister_append_text_field(lister, string);
                    lister_append_key(lister, string);
                }
                
                result = 1;
            }
        } break;
        
        case KeyCode_C:
        {
            if (has_modifier(&in, KeyCode_Control))
            {
                clipboard_post(0, lister->text_field.string);
                result = 1;
            }
        } break;
        
        case KeyCode_Tick:
        {
            result = 1;
            if (has_modifier(&in, KeyCode_Control))
                long_lister_tooltip_peek ^= 0x80000000;
            else if (has_modifier(&in, KeyCode_Alt))
                long_lister_tooltip_peek ^= 1;
            else
                result = 0;
            
            lister->set_vertical_focus_to_item = result;
        } break;
    }
    
    return result;
}

// @COPYPASTA(long): lister__write_string__default
function Lister_Activation_Code Long_Lister_WriteString(Application_Links* app)
{
    View_ID view = get_active_view(app, Access_Always);
    Lister* lister = view_get_lister(app, view);
    
    if (lister)
    {
        User_Input in = get_current_input(app);
        String8 string = to_writable(&in);
        
        if (string.size)
        {
            lister_append_text_field(lister, string);
            lister_append_key(lister, string);
        }
    }
    
    return ListerActivation_Continue;
}

// @COPYPASTA(long): lister__backspace_text_field__default
function void Long_Lister_Backspace(Application_Links* app)
{
    View_ID view = get_active_view(app, Access_Always);
    Lister* lister = view_get_lister(app, view);
    
    if (lister)
    {
        User_Input in = get_current_input(app);
        if (has_modifier(&in, KeyCode_Control))
        {
            lister->text_field.size = 0;
            lister->key_string.size = 0;
        }
        
        else
        {
            lister->text_field.string = backspace_utf8(lister->text_field.string);
            lister->key_string.string = backspace_utf8(lister->key_string.string);
        }
    }
}

// @COPYPASTA(long): lister__backspace_text_field__file_path
function void Long_Lister_Backspace_Path(Application_Links* app)
{
    View_ID view = get_this_ctx_view(app, Access_Always);
    Lister* lister = view_get_lister(app, view);
    if (lister)
    {
        if (lister->text_field.size > 0)
        {
            String8 text_field = lister->text_field.string;
            
            u32 slash_count = 0;
            while (character_is_slash(text_field.str[text_field.size-1])) // remove all duplicate slashes
            {
                slash_count++;
                text_field = string_chop(text_field, 1);
                if (!text_field.size)
                    return; // TODO(long): Is this good enough?
            }
            
            b32 whole_word_backspace;
            {
                User_Input input = get_current_input(app);
                b32 has_mod = has_modifier(&input, KeyCode_Control);
                b32 whole_word_when_mod = def_get_config_b32_lit("lister_whole_word_backspace_when_modified");
                whole_word_backspace = (has_mod == whole_word_when_mod);
            }
            
            if (slash_count > 1)
                text_field.size += whole_word_backspace ? 1 : (slash_count - 1);
            else
            {
                String8 hot = string_remove_last_folder(text_field);
                if (whole_word_backspace)
                    text_field.size = hot.size;
                else if (slash_count == 0)
                    text_field = backspace_utf8(text_field);
                
                set_hot_directory(app, hot);
                lister->text_field.string = text_field;
                Long_Lister_Refresh(app, lister);
            }
            
            lister_set_key(lister, string_front_of_path(text_field));
            lister->text_field.string = text_field;
        }
    }
}

// @COPYPASTA(long): lister__navigate__default
function void Long_Lister_Navigate(Application_Links* app, View_ID view, Lister* lister, i32 delta)
{
    i32 new_index = lister->item_index + delta;
    
    if (new_index < 0 && lister->item_index <= 0)
        lister->item_index = lister->filtered.count - 1;
    else if (new_index >= lister->filtered.count && lister->item_index == lister->filtered.count - 1)
        lister->item_index = 0;
    else
        lister->item_index = clamp(0, new_index, lister->filtered.count - 1);
    
    lister->set_vertical_focus_to_item = 1;
    lister_update_selection_values(lister);
}

function void Long_Lister_Navigate_NoSelect(Application_Links* app, View_ID view, Lister* lister, i32 delta)
{
    i32 new_index = lister->item_index + delta;
    if (0) { }
    
    else if (new_index < 0 && lister->item_index < 0)
        lister->item_index = lister->filtered.count - 1;
    else if (new_index < 0 && lister->item_index == 0)
        lister->item_index = -1;
    
    else if (new_index >= lister->filtered.count && lister->item_index == lister->filtered.count - 1)
        lister->item_index = -1;
    else
        lister->item_index = clamp(0, new_index, lister->filtered.count - 1);
    
    if (lister->item_index >= 0)
        lister->set_vertical_focus_to_item = 1;
    lister_update_selection_values(lister);
}

// @COPYPASTA(long): run_lister
function Lister_Result Long_Lister_Run(Application_Links* app, Lister* lister, b32 auto_select_first)
{
    lister->filter_restore_point = begin_temp(lister->arena);
    Long_Lister_FilterList(app, lister);
    
    // NOTE(long): I didn't add a custom handler for lister__write_character__file_path because it doesn't need filtered tags.
    if (lister->handlers.write_character == lister__write_string__default)
        lister->handlers.write_character = Long_Lister_WriteString;
    if (lister->handlers.backspace == lister__backspace_text_field__default)
        lister->handlers.backspace = Long_Lister_Backspace;
    if (lister->handlers.backspace == lister__backspace_text_field__file_path)
        lister->handlers.backspace = Long_Lister_Backspace_Path;
    if (lister->handlers.navigate == lister__navigate__default)
        lister->handlers.navigate = auto_select_first ? Long_Lister_Navigate : Long_Lister_Navigate_NoSelect;
    
    // TODO(long): Load this from the config file dynamically
    long_lister_tooltip_peek &= 0x7FFFFFFF;
    
    View_ID view = get_this_ctx_view(app, Access_Always);
    Mapping* mapping = 0;
    Command_Map* map = Long_Mapping_GetMap(app, view, &mapping);
    
    // TODO(long):
    //View_Context ctx = view_current_context(app, view);
    //ctx.render_caller = Long_Lister_Render;
    //ctx.hides_buffer = true;
    //View_Context_Block ctx_block(app, view, &ctx);
    
    if (!auto_select_first)
    {
        lister->item_index = -1;
        lister->highlighted_node = 0;
    }
    
    for (;;)
    {
        User_Input in = get_next_input(app, EventPropertyGroup_Any, EventProperty_Escape);
        if (in.abort)
        {
            block_zero_struct(&lister->out);
            lister->out.canceled = true;
            break;
        }
        
        Lister_Activation_Code result = ListerActivation_Continue;
        String8 text_field = lister->text_field.string;
        b32 handled = 1;
        i32 dir = -1;
        
        switch (in.event.kind)
        {
            case InputEventKind_TextInsert:
            {
                if (lister->handlers.write_character != 0)
                    result = lister->handlers.write_character(app);
            } break;
            
            case InputEventKind_KeyStroke:
            {
                switch (in.event.key.code)
                {
                    case KeyCode_Return:
                    case KeyCode_Tab:
                    {
                        if (lister->item_index < 0)
                        {
                            lister->item_index = 0;
                            lister_update_selection_values(lister);
                        }
                        
                        void* user_data = 0;
                        if (0 <= lister->raw_item_index && lister->raw_item_index < lister->options.count)
                            user_data = lister_get_user_data(lister, lister->raw_item_index);
                        lister_activate(app, lister, user_data, 0);
                        result = ListerActivation_Finished;
                    } break;
                    
                    case KeyCode_Backspace:
                    {
                        if (lister->handlers.backspace)
                            lister->handlers.backspace(app);
                        else
                            handled = 0;
                    } break;
                    
                    case KeyCode_Down: dir = 1;
                    case KeyCode_Up:
                    {
                        if (lister->handlers.navigate)
                            lister->handlers.navigate(app, view, lister, dir);
                        else
                            handled = 0;
                    } break;
                    
                    case KeyCode_PageDown: dir = 1;
                    case KeyCode_PageUp:
                    {
                        if (lister->handlers.navigate)
                            lister->handlers.navigate(app, view, lister, lister->visible_count * dir);
                        else
                            handled = 0;
                    } break;
                    
                    default: handled = 0; break;
                }
                
                if (!handled)
                {
                    handled = 1;
                    if (lister->handlers.key_stroke)
                        result = lister->handlers.key_stroke(app);
                    else
                        handled = Long_Lister_HandleKeyStroke(app, lister, in);
                }
            } break;
            
            case InputEventKind_MouseButton:
            {
                switch (in.event.mouse.code)
                {
                    case MouseCode_Left: lister->hot_user_data = lister_user_data_at_p(app, view, lister, V2f32(in.event.mouse.p)); break;
                    default: handled = 0; break;
                }
            } break;
            
            case InputEventKind_MouseButtonRelease:
            {
                switch (in.event.mouse.code)
                {
                    case MouseCode_Left:
                    {
                        if (lister->hot_user_data)
                        {
                            void* clicked = lister_user_data_at_p(app, view, lister, V2f32(in.event.mouse.p));
                            
                            if (lister->hot_user_data == clicked)
                            {
                                lister_activate(app, lister, clicked, true);
                                result = ListerActivation_Finished;
                            }
                        }
                        
                        lister->hot_user_data = 0;
                    } break;
                    
                    default: handled = 0; break;
                }
            } break;
            
            case InputEventKind_MouseWheel:
            {
                Mouse_State mouse = get_mouse_state(app);
                lister->scroll.target.y += mouse.wheel;
            } break;
            
            default: handled = 0; break;
        }
        
        if (result == ListerActivation_Finished)
            break;
        
        if (!handled)
        {
            Command_Metadata* metadata = Long_Mapping_GetMetadata(mapping, map, &in);
            if (metadata && metadata->is_ui)
            {
                view_enqueue_command_function(app, view, metadata->proc);
                break;
            }
            leave_current_input_unhandled(app);
        }
        
        if (!string_match(text_field, lister->text_field.string))
            Long_Lister_Reset(app, lister, auto_select_first);
    }
    
    return lister->out;
}

CUSTOM_COMMAND_SIG(long_test_lister_render_1)
CUSTOM_DOC("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec placerat tellus vitae feugiat tincidunt. Suspendisse sagittis velit porttitor justo commodo sagittis. Etiam erat metus, elementum eu aliquam et, dictum at eros. Vivamus nulla ex, gravida malesuada iaculis id, maximus at quam. Fusce sodales, velit id varius rhoncus, odio ipsum placerat eros, nec euismod dui nunc in mauris. Donec quis commodo enim. Etiam sed efficitur elit, in interdum lacus. Vivamus sollicitudin hendrerit lacinia. Suspendisse aliquet bibendum nunc, eget fermentum quam feugiat ac. Sed at fringilla neque, eu aliquam risus. Donec ut ante eu erat cursus semper eget et velit. Quisque ut aliquam nibh. Curabitur et justo hendrerit, finibus sapien quis, fringilla ante. Nullam vehicula, nisi in facilisis egestas, tellus nunc faucibus lacus, tempor aliquet nulla felis sit amet felis. Nam non vulputate elit.\n\nVestibulum volutpat est vel felis tincidunt, sed imperdiet neque feugiat. Integer placerat dignissim eros, in sollicitudin lacus venenatis varius. Maecenas in feugiat ex. Nunc elementum sem est, sodales facilisis ligula hendrerit interdum. Integer pulvinar orci eget ipsum porta dapibus. Pellentesque sapien eros, semper sit amet placerat a, viverra malesuada nulla. Donec cursus turpis ut metus auctor pellentesque. Etiam dolor dui, maximus vitae malesuada ac, tincidunt eu tortor. Nullam felis ante, varius elementum mattis nec, pretium in diam.\n\nUt ut malesuada justo. Donec consequat magna sed diam feugiat pellentesque. Duis quis tempus tortor. Donec vulputate ullamcorper massa, eget porta metus ultrices nec. Cras dignissim dictum blandit. Nullam pellentesque volutpat purus vitae bibendum. Aenean quis neque eget orci imperdiet lacinia id finibus lorem. Interdum et malesuada fames ac ante ipsum primis in faucibus. Interdum et malesuada fames ac ante ipsum primis in faucibus. Praesent dictum lectus a ligula venenatis, nec ultricies turpis placerat. Proin euismod ut odio eu luctus. Vivamus eleifend eros sit amet felis dapibus, ac tempus est feugiat. Vivamus sit amet quam id lorem commodo volutpat. Maecenas ac nulla non turpis euismod vestibulum eget vitae tortor.\n\nMauris venenatis nunc ac enim fringilla, vitae varius neque imperdiet. Duis odio purus, commodo in dolor in, mollis malesuada tellus. Duis pharetra vulputate mauris ut cursus. Cras non eros feugiat, lacinia augue ut, tincidunt arcu. Donec pulvinar pulvinar lorem, vel sollicitudin arcu commodo ac. Sed facilisis lorem elit, sit amet dapibus urna varius elementum. Cras at viverra urna, eu vehicula ligula. Etiam ut convallis magna. Suspendisse feugiat quam sit amet accumsan aliquet. Pellentesque vestibulum sapien ut urna sollicitudin consequat. Duis non ullamcorper nibh.\n\nNullam hendrerit, sem et dictum faucibus, neque purus tristique ligula, eu sollicitudin arcu orci in magna. Vivamus auctor, enim varius ornare mattis, dolor magna condimentum enim, nec convallis sem velit nec augue. Pellentesque rutrum mauris ut nulla consectetur condimentum. Aliquam nec massa eu metus sollicitudin tincidunt. Donec lobortis ultricies sem id pretium. Donec quis felis vel ante fermentum pretium. Nulla at mi sit amet ex molestie imperdiet a eget lacus. Nullam rutrum aliquet tellus interdum bibendum.") { }

CUSTOM_COMMAND_SIG(long_test_lister_render_2)
CUSTOM_DOC("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec placerat tellus vitae feugiat tincidunt. Suspendisse sagittis velit porttitor justo commodo sagittis. Etiam erat metus, elementum eu aliquam et, dictum at eros. Vivamus nulla ex, gravida malesuada iaculis id, maximus at quam. Fusce sodales, velit id varius rhoncus, odio ipsum placerat eros, nec euismod dui nunc in mauris. Donec quis commodo enim. Etiam sed efficitur elit, in interdum lacus. Vivamus sollicitudin hendrerit lacinia. Suspendisse aliquet bibendum nunc, eget fermentum quam feugiat ac. Sed at fringilla neque, eu aliquam risus. Donec ut ante eu erat cursus semper eget et velit. Quisque ut aliquam nibh. Curabitur et justo hendrerit, finibus sapien quis, fringilla ante. Nullam vehicula, nisi in facilisis egestas, tellus nunc faucibus lacus, tempor aliquet nulla felis sit amet felis. Nam non vulputate elit.\n\nVestibulum volutpat est vel felis tincidunt, sed imperdiet neque feugiat. Integer placerat dignissim eros, in sollicitudin lacus venenatis varius. Maecenas in feugiat ex. Nunc elementum sem est, sodales facilisis ligula hendrerit interdum. Integer pulvinar orci eget ipsum porta dapibus. Pellentesque sapien eros, semper sit amet placerat a, viverra malesuada nulla. Donec cursus turpis ut metus auctor pellentesque. Etiam dolor dui, maximus vitae malesuada ac, tincidunt eu tortor. Nullam felis ante, varius elementum mattis nec, pretium in diam.\n\nUt ut malesuada justo. Donec consequat magna sed diam feugiat pellentesque. Duis quis tempus tortor. Donec vulputate ullamcorper massa, eget porta metus ultrices nec. Cras dignissim dictum blandit. Nullam pellentesque volutpat purus vitae bibendum. Aenean quis neque eget orci imperdiet lacinia id finibus lorem. Interdum et malesuada fames ac ante ipsum primis in faucibus. Interdum et malesuada fames ac ante ipsum primis in faucibus. Praesent dictum lectus a ligula venenatis, nec ultricies turpis placerat. Proin euismod ut odio eu luctus. Vivamus eleifend eros sit amet felis dapibus, ac tempus est feugiat. Vivamus sit amet quam id lorem commodo volutpat. Maecenas ac nulla non turpis euismod vestibulum eget vitae tortor.\n\nMauris venenatis nunc ac enim fringilla, vitae varius neque imperdiet. Duis odio purus, commodo in dolor in, mollis malesuada tellus. Duis pharetra vulputate mauris ut cursus. Cras non eros feugiat, lacinia augue ut, tincidunt arcu. Donec pulvinar pulvinar lorem, vel sollicitudin arcu commodo ac. Sed facilisis lorem elit, sit amet dapibus urna varius elementum. Cras at viverra urna, eu vehicula ligula. Etiam ut convallis magna. Suspendisse feugiat quam sit amet accumsan aliquet. Pellentesque vestibulum sapien ut urna sollicitudin consequat. Duis non ullamcorper nibh.\n\nNullam hendrerit, sem et dictum faucibus, neque purus tristique ligula, eu sollicitudin arcu orci in magna. Vivamus auctor, enim varius ornare mattis, dolor magna condimentum enim, nec convallis sem velit nec augue. Pellentesque rutrum mauris ut nulla consectetur condimentum. Aliquam nec massa eu metus sollicitudin tincidunt. Donec lobortis ultricies sem id pretium. Donec quis felis vel ante fermentum pretium. Nulla at mi sit amet ex molestie imperdiet a eget lacus. Nullam rutrum aliquet tellus interdum bibendum. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec placerat tellus vitae feugiat tincidunt. Suspendisse sagittis velit porttitor justo commodo sagittis. Etiam erat metus, elementum eu aliquam et, dictum at eros. Vivamus nulla ex, gravida malesuada iaculis id, maximus at quam. Fusce sodales, velit id varius rhoncus, odio ipsum placerat eros, nec euismod dui nunc in mauris. Donec quis commodo enim. Etiam sed efficitur elit, in interdum lacus. Vivamus sollicitudin hendrerit lacinia. Suspendisse aliquet bibendum nunc, eget fermentum quam feugiat ac. Sed at fringilla neque, eu aliquam risus. Donec ut ante eu erat cursus semper eget et velit. Quisque ut aliquam nibh. Curabitur et justo hendrerit, finibus sapien quis, fringilla ante. Nullam vehicula, nisi in facilisis egestas, tellus nunc faucibus lacus, tempor aliquet nulla felis sit amet felis. Nam non vulputate elit.\n\nVestibulum volutpat est vel felis tincidunt, sed imperdiet neque feugiat. Integer placerat dignissim eros, in sollicitudin lacus venenatis varius. Maecenas in feugiat ex. Nunc elementum sem est, sodales facilisis ligula hendrerit interdum. Integer pulvinar orci eget ipsum porta dapibus. Pellentesque sapien eros, semper sit amet placerat a, viverra malesuada nulla. Donec cursus turpis ut metus auctor pellentesque. Etiam dolor dui, maximus vitae malesuada ac, tincidunt eu tortor. Nullam felis ante, varius elementum mattis nec, pretium in diam.\n\nUt ut malesuada justo. Donec consequat magna sed diam feugiat pellentesque. Duis quis tempus tortor. Donec vulputate ullamcorper massa, eget porta metus ultrices nec. Cras dignissim dictum blandit. Nullam pellentesque volutpat purus vitae bibendum. Aenean quis neque eget orci imperdiet lacinia id finibus lorem. Interdum et malesuada fames ac ante ipsum primis in faucibus. Interdum et malesuada fames ac ante ipsum primis in faucibus. Praesent dictum lectus a ligula venenatis, nec ultricies turpis placerat. Proin euismod ut odio eu luctus. Vivamus eleifend eros sit amet felis dapibus, ac tempus est feugiat. Vivamus sit amet quam id lorem commodo volutpat. Maecenas ac nulla non turpis euismod vestibulum eget vitae tortor.\n\nMauris venenatis nunc ac enim fringilla, vitae varius neque imperdiet. Duis odio purus, commodo in dolor in, mollis malesuada tellus. Duis pharetra vulputate mauris ut cursus. Cras non eros feugiat, lacinia augue ut, tincidunt arcu. Donec pulvinar pulvinar lorem, vel sollicitudin arcu commodo ac. Sed facilisis lorem elit, sit amet dapibus urna varius elementum. Cras at viverra urna, eu vehicula ligula. Etiam ut convallis magna. Suspendisse feugiat quam sit amet accumsan aliquet. Pellentesque vestibulum sapien ut urna sollicitudin consequat. Duis non ullamcorper nibh.\n\nNullam hendrerit, sem et dictum faucibus, neque purus tristique ligula, eu sollicitudin arcu orci in magna. Vivamus auctor, enim varius ornare mattis, dolor magna condimentum enim, nec convallis sem velit nec augue. Pellentesque rutrum mauris ut nulla consectetur condimentum. Aliquam nec massa eu metus sollicitudin tincidunt. Donec lobortis ultricies sem id pretium. Donec quis felis vel ante fermentum pretium. Nulla at mi sit amet ex molestie imperdiet a eget lacus. Nullam rutrum aliquet tellus interdum bibendum.") { }

CUSTOM_COMMAND_SIG(long_test_lister_render_3)
CUSTOM_DOC("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ123456789ABCDEF") { }

CUSTOM_COMMAND_SIG(long_test_lister_render_4)
CUSTOM_DOC("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ") { }

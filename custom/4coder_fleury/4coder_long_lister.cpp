
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
    data->type = Long_Header_Path;
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
            case Long_Header_Path:
            {
                String8 filepath = Long_Buffer_NameNoProjPath(app, arena, data.buffer);
                if (filepath.size)
                    result = push_stringf(arena, "[%.*s] ", string_expand(filepath));
            } break;
            
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

function f32 Long_Lister_GetHeight(Lister* lister, f32 line_height, f32 block_height, f32* out_list_y)
{
    f32 file_bar_height = line_height + 2.f; // layout_file_bar_on_top
    f32 text_field_height = lister_get_text_field_height(line_height);
    f32 scroll_y = lister->scroll.position.y;
    f32 margin = 3.f; // draw_background_and_margin
    
    i32 first_index = (i32)(scroll_y/block_height);
    f32 y_pos = margin + text_field_height + file_bar_height;
    if (out_list_y)
        *out_list_y = y_pos;
    y_pos = y_pos - scroll_y + first_index * block_height;
    
    i32 offset = first_index < lister->item_index ? 1 : -1;
    for (i32 i = first_index; i != lister->item_index; i += offset)
        y_pos += block_height * offset;
    
    return y_pos;
}

// @COPYPASTA(long): lister_render
function void Long_Lister_Render(Application_Links* app, Frame_Info frame_info, View_ID view)
{
    Scratch_Block scratch(app);
    
    Lister* lister = view_get_lister(app, view);
    if (lister == 0)
        return;
    
    Rect_f32 region = draw_background_and_margin(app, view);
    Rect_f32 prev_clip = draw_set_clip(app, region);
    
    Face_ID face_id = get_face_id(app, 0);
    Face_Metrics metrics = get_face_metrics(app, face_id);
    f32 line_height = metrics.line_height;
    f32 block_height = lister_get_block_height(line_height);
    f32 text_field_height = lister_get_text_field_height(line_height);
    
    f32 tooltip_height = 0;
    if (long_lister_tooltip_peek == Long_Tooltip_UnderItem && lister->highlighted_node)
    {
        Lister_Node* current = lister->highlighted_node;
        Long_Lister_Data* data = (Long_Lister_Data*)(current ? current->user_data : 0);
        if (data == (Long_Lister_Data*)(current + 1))
        {
            f32 pos_y;
            Rect_f32 draw_rect = region;
            pos_y = Long_Lister_GetHeight(lister, line_height, block_height, &draw_rect.y0);
            
            tooltip_height = line_height * 10;
            Rect_f32 tooltip_rect = Rf32(rect_range_x(region), If32_size(pos_y, tooltip_height + block_height));
            Rect_f32 content_rect = tooltip_rect;
            content_rect.y0 += block_height;
            content_rect = rect_inner(content_rect, line_height);
            
            if (data->tooltip.size)
            {
                Fancy_Block block = Long_Render_LayoutString(app, scratch, data->tooltip, face_id, rect_width(content_rect), 0);
                tooltip_height = get_fancy_block_height(app, face_id, &block) + line_height * 2;
                tooltip_rect.y1 = tooltip_rect.y0 + tooltip_height + block_height;
                content_rect.y1 = content_rect.y0 + tooltip_height;
                
                Long_Render_TooltipRect(app, tooltip_rect, 0);
                draw_fancy_block(app, face_id, fcolor_id(defcolor_text_default, 0), &block, content_rect.p0);
            }
            else
                Long_Render_DrawPeek(app, tooltip_rect, content_rect, draw_rect, data->buffer, Ii64(data->pos), 1, 0);
        }
    }
    
    // NOTE(allen): file bar
    // TODO(allen): What's going on with 'showing_file_bar'? I found it like this.
    b64 showing_file_bar = false;
    b32 hide_file_bar_in_ui = def_get_config_b32(vars_save_string_lit("hide_file_bar_in_ui"));
    if (view_get_setting(app, view, ViewSetting_ShowFileBar, &showing_file_bar) &&
        showing_file_bar && !hide_file_bar_in_ui)
    {
        Rect_f32_Pair pair = layout_file_bar_on_top(region, line_height);
        Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
        F4_DrawFileBar(app, view, buffer, face_id, pair.min); // NOTE(long): Fix render the wrong file bar
        region = pair.max;
    }
    
    Mouse_State mouse = get_mouse_state(app);
    Vec2_f32 m_p = V2f32(mouse.p);
    
    lister->visible_count = (i32)((rect_height(region)/block_height)) - 3;
    lister->visible_count = clamp_bot(1, lister->visible_count);
    
    Rect_f32 text_field_rect = {};
    Rect_f32 list_rect = {};
    {
        Rect_f32_Pair pair = lister_get_top_level_layout(region, text_field_height);
        text_field_rect = pair.min;
        list_rect = pair.max;
    }
    
    {
        Vec2_f32 p = V2f32(text_field_rect.x0 + 3.f, text_field_rect.y0);
        Fancy_Line text_field = {};
        push_fancy_string (scratch, &text_field, fcolor_id(defcolor_pop1), lister->query.string);
        // NOTE(long): Display the current index and count
        push_fancy_stringf(scratch, &text_field, fcolor_id(defcolor_pop1), " (%d/%d) ",
                           lister->filtered.count ? lister->item_index + 1 : 0, lister->filtered.count);
        p = draw_fancy_line(app, face_id, fcolor_zero(), &text_field, p);
        
        // TODO(allen): This is a bit of a hack. Maybe an upgrade to fancy to focus
        // more on being good at this and less on overriding everything 10 ways to sunday
        // would be good.
        block_zero_struct(&text_field);
        push_fancy_string(scratch, &text_field, fcolor_id(defcolor_text_default),
                          lister->text_field.string);
        f32 width = get_fancy_line_width(app, face_id, &text_field);
        f32 cap_width = text_field_rect.x1 - p.x - 6.f;
        if (cap_width < width){
            Rect_f32 prect = draw_set_clip(app, Rf32(p.x, text_field_rect.y0, p.x + cap_width, text_field_rect.y1));
            p.x += cap_width - width;
            draw_fancy_line(app, face_id, fcolor_zero(), &text_field, p);
            draw_set_clip(app, prect);
        }
        else
            draw_fancy_line(app, face_id, fcolor_zero(), &text_field, p);
    }
    
    Range_f32 x = rect_range_x(list_rect);
    draw_set_clip(app, list_rect);
    
    // NOTE(allen): auto scroll to the item if the flag is set.
    f32 scroll_y = lister->scroll.position.y;
    
    if (lister->set_vertical_focus_to_item)
    {
        // NOTE(long): Read the comment at LONG_LISTER_TOOLTIP_SPECIAL_VALUE for more info
        if (lister->set_vertical_focus_to_item == LONG_LISTER_TOOLTIP_SPECIAL_VALUE ||
            long_lister_tooltip_peek != Long_Tooltip_NextToItem)
            lister->set_vertical_focus_to_item = 0;
        else
            lister->set_vertical_focus_to_item = LONG_LISTER_TOOLTIP_SPECIAL_VALUE;
        
        Range_f32 item_y = If32_size(lister->item_index*block_height, block_height + tooltip_height);
        f32 view_h = rect_height(list_rect);
        Range_f32 view_y = If32_size(scroll_y, view_h);
        if (view_y.min > item_y.min || item_y.max > view_y.max)
        {
            f32 item_center = (item_y.min + item_y.max)*0.5f;
            f32 view_center = (view_y.min + view_y.max)*0.5f;
            f32 margin = view_h*.3f;
            margin = clamp_top(margin, block_height*3.f);
            if (item_center < view_center)
                lister->scroll.target.y = item_y.min - margin;
            else
                lister->scroll.target.y = item_y.max + margin - view_h;
        }
    }
    
    // NOTE(allen): clamp scroll target and position; smooth scroll rule
    i32 count = lister->filtered.count;
    Range_f32 scroll_range = If32(0.f, clamp_bot(0.f, count*block_height - block_height));
    lister->scroll.target.y = clamp_range(scroll_range, lister->scroll.target.y);
    lister->scroll.target.x = 0.f;
    
    Vec2_f32_Delta_Result delta = delta_apply(app, view, frame_info.animation_dt, lister->scroll);
    lister->scroll.position = delta.p;
    if (delta.still_animating)
        animate_in_n_milliseconds(app, 0);
    
    lister->scroll.position.y = clamp_range(scroll_range, lister->scroll.position.y);
    lister->scroll.position.x = 0.f;
    
    scroll_y = lister->scroll.position.y;
    f32 y_pos = list_rect.y0 - scroll_y;
    
    // TODO(long): This is probably stupid (I wrote this at 1 a.m). Rethink this whole thing more!!!
    i32 first_index = 0;
    f32 height = 0;
    {
        f32 prev_height = 0;
        i32 prev_idx = 0;
        while (height < scroll_y)
        {
            prev_height = height;
            prev_idx = first_index;
            
            height += block_height;
            if (first_index == lister->item_index)
                height += tooltip_height;
            ++first_index;
        }
        height = prev_height;
        first_index = prev_idx;
    }
    y_pos += height;
    
    for (i32 i = first_index; i < count; i += 1)
    {
        Range_f32 y = If32(y_pos, y_pos + block_height);
        y_pos = y.max;
        Rect_f32 item_rect = Rf32(x, y);
        Rect_f32 item_inner = rect_inner(item_rect, 3.f);
        
        if (item_rect.y0 > region.y1) break; // NOTE(long): Fix overdraw thing
        
        Lister_Node* node = lister->filtered.node_ptrs[i];
        b32 hovered = rect_contains_point(item_rect, m_p);
        UI_Highlight_Level highlight = UIHighlight_None;
        if (node == lister->highlighted_node)
            highlight = UIHighlight_Active;
        else if (node->user_data == lister->hot_user_data)
        {
            if (hovered)
                highlight = UIHighlight_Active;
            else
                highlight = UIHighlight_Hover;
        }
        else if (hovered)
            highlight = UIHighlight_Hover;
        
        u64 lister_roundness_100 = def_get_config_u64(app, vars_save_string_lit("lister_roundness"));
        f32 roundness = block_height*lister_roundness_100*0.01f;
        draw_rectangle_fcolor(app, item_rect , roundness, get_item_margin_color(highlight));
        draw_rectangle_fcolor(app, item_inner, roundness, get_item_margin_color(highlight, 1));
        
        Fancy_Line line = {};
        push_fancy_string(scratch, &line, fcolor_id(defcolor_pop1), Long_Lister_GetHeaderString(app, scratch, node));
        push_fancy_string(scratch, &line, fcolor_id(defcolor_text_default), node->string);
        push_fancy_string(scratch, &line, S8Lit(" "));
        push_fancy_string(scratch, &line, fcolor_id(defcolor_pop2), node->status);
        
        if (node == lister->highlighted_node)
            y_pos += tooltip_height;
        
        Vec2_f32 p = item_inner.p0 + V2f32(3.f, (block_height - line_height)*0.5f);
        draw_fancy_line(app, face_id, fcolor_zero(), &line, p);
    }
    
    draw_set_clip(app, prev_clip);
}

// @COPYPASTA(long): lister_get_filtered
function Lister_Filtered Long_Lister_GetFilter(Application_Links* app, Arena* arena, Lister* lister)
{
    i32 node_count = lister->options.count;
    
    Lister_Filtered filtered = {};
    filtered.exact_matches.node_ptrs = push_array(arena, Lister_Node*, 1);
    filtered.before_extension_matches.node_ptrs = push_array(arena, Lister_Node*, node_count);
    filtered.substring_matches.node_ptrs = push_array(arena, Lister_Node*, node_count);
    
    Temp_Memory_Block temp(arena);
    
    String_Const_u8 key = lister->key_string.string;
    key = push_string_copy(arena, key);
    string_mod_replace_character(key, '_', '*');
    string_mod_replace_character(key, ' ', '*');
    
    List_String_Const_u8 absolutes = {};
    string_list_push(arena, &absolutes, string_u8_litexpr(""));
    List_String_Const_u8 splits = string_split(arena, key, (u8*)"*", 1);
    b32 has_wildcard = (splits.node_count > 1);
    string_list_push(&absolutes, &splits);
    string_list_push(arena, &absolutes, string_u8_litexpr(""));
    
    // NOTE(long): /inclusive_tag: or, ~exclusive_tag: not
    String8List include_tags = {};
    {
        String8Node** prev = &absolutes.first->next;
        for (String8Node* node = absolutes.first; node;)
        {
            String8Node* next = node->next;
            String8 string = node->string;
            if (string.size >= 1 && string.str[0] == '\\')
            {
                string_list_push(&include_tags, node);
                node->string = string_skip(node->string, 1);
                (*prev) = next;
                absolutes.node_count--;
            }
            else prev = &node->next;
            node = next;
        }
    }
    
    String8List exclude_tags = {};
    {
        String8Node** prev = &absolutes.first->next;
        for (String8Node* node = absolutes.first; node;)
        {
            String8Node* next = node->next;
            String8 string = node->string;
            if (string.size > 1 && string.str[0] == '~')
            {
                string_list_push(&exclude_tags, node);
                node->string = string_skip(node->string, 1);
                (*prev) = next;
                absolutes.node_count--;
            }
            else prev = &node->next;
            node = next;
        }
    }
    
    String8List header_filters = {};
    {
        String8Node** prev = &absolutes.first->next;
        for (String8Node* node = absolutes.first; node;)
        {
            String8Node* next = node->next;
            String8 string = node->string;
            if (string.size > 1 && string.str[0] == '`')
            {
                string_list_push(&header_filters, node);
                node->string = string_skip(node->string, 1);
                (*prev) = next;
                absolutes.node_count--;
            }
            else prev = &node->next;
            node = next;
        }
    }
    
    for (Lister_Node *node = lister->options.first;
         node != 0;
         node = node->next){
        {
            b32 has_tag = true;
            for (String8Node* tag = include_tags.first; tag; tag = tag->next)
                if (!(has_tag = string_has_substr(node->status, tag->string, StringMatch_CaseInsensitive)))
                    break;
            if (!has_tag)
                continue;
        }
        
        {
            b32 has_tag = false;
            for (String8Node* tag = exclude_tags.first; tag; tag = tag->next)
                if (has_tag = string_has_substr(node->status, tag->string, StringMatch_CaseInsensitive))
                    break;
            if (has_tag)
                continue;
        }
        
        {
            Scratch_Block scratch(app, arena);
            String8 header = Long_Lister_GetHeaderString(app, scratch, node);
            b32 has_tag = true;
            for (String8Node* tag = header_filters.first; tag; tag = tag->next)
                if (has_tag = string_has_substr(header, tag->string, StringMatch_CaseInsensitive))
                    break;
            if (!has_tag)
                continue;
        }
        
        String_Const_u8 node_string = node->string;
        if (key.size == 0 || string_wildcard_match_insensitive(absolutes, node_string)){
            if (string_match_insensitive(node_string, key) && filtered.exact_matches.count == 0){
                filtered.exact_matches.node_ptrs[filtered.exact_matches.count++] = node;
            }
            else if (key.size > 0 &&
                     !has_wildcard &&
                     string_match_insensitive(string_prefix(node_string, key.size), key) &&
                     node->string.size > key.size &&
                     node->string.str[key.size] == '.'){
                filtered.before_extension_matches.node_ptrs[filtered.before_extension_matches.count++] = node;
            }
            else{
                filtered.substring_matches.node_ptrs[filtered.substring_matches.count++] = node;
            }
        }
    }
    
    return(filtered);
}

// @COPYPASTA(long): lister_update_filtered_list
function void Long_Lister_FilterList(Application_Links *app, Lister *lister)
{
    Arena *arena = lister->arena;
    Scratch_Block scratch(app, arena);
    
    Lister_Filtered filtered = Long_Lister_GetFilter(app, scratch, lister);
    
    Lister_Node_Ptr_Array node_ptr_arrays[] = {
        filtered.exact_matches,
        filtered.before_extension_matches,
        filtered.substring_matches,
    };
    
    end_temp(lister->filter_restore_point);
    
    i32 total_count = 0;
    for (i32 array_index = 0; array_index < ArrayCount(node_ptr_arrays); array_index += 1){
        Lister_Node_Ptr_Array node_ptr_array = node_ptr_arrays[array_index];
        total_count += node_ptr_array.count;
    }
    
    Lister_Node **node_ptrs = push_array(arena, Lister_Node*, total_count);
    lister->filtered.node_ptrs = node_ptrs;
    lister->filtered.count = total_count;
    i32 counter = 0;
    for (i32 array_index = 0; array_index < ArrayCount(node_ptr_arrays); array_index += 1){
        Lister_Node_Ptr_Array node_ptr_array = node_ptr_arrays[array_index];
        for (i32 node_index = 0; node_index < node_ptr_array.count; node_index += 1){
            Lister_Node *node = node_ptr_array.node_ptrs[node_index];
            node_ptrs[counter] = node;
            counter += 1;
        }
    }
    
    lister_update_selection_values(lister);
}

// @COPYPASTA(long): lister_call_refresh_handler
function void Long_Lister_Refresh(Application_Links *app, Lister *lister){
    if (lister->handlers.refresh != 0){
        lister->handlers.refresh(app, lister);
        lister->filter_restore_point = begin_temp(lister->arena);
        Long_Lister_FilterList(app, lister);
    }
}

// NOTE(long): I could move this to the key_stroke handler if there were a way to change the `handled` bool.
// If only the idea of key_stroke was fleshed out just a little bit more and its signature was like this.
// @CONSIDER(long): Rather than hard-coded key, I can match each command's binding.
function b32 Long_Lister_HandleKeyStroke(Application_Links* app, Lister* lister, User_Input in)
{
    b32 result = 0;
    
    switch (in.event.key.code)
    {
        case KeyCode_V:
        {
            if (result = has_modifier(&in, KeyCode_Control))
            {
                Scratch_Block scratch(app, lister->arena);
                String8 string = push_clipboard_index(scratch, 0, 0);
                if (string.size)
                {
                    lister_append_text_field(lister, string);
                    lister_append_key(lister, string);
                    lister->item_index = 0;
                    lister_zero_scroll(lister);
                    Long_Lister_FilterList(app, lister);
                }
            }
        } break;
        
        case KeyCode_C:
        {
            if (result = has_modifier(&in, KeyCode_Control))
                clipboard_post(0, lister->text_field.string);
        } break;
        
        case KeyCode_Tick:
        {
            if (result = has_modifier(&in, KeyCode_Control))
                long_lister_tooltip_peek ^= 0x80000000;
            else if (result = has_modifier(&in, KeyCode_Alt))
                long_lister_tooltip_peek ^= 1;
            lister->set_vertical_focus_to_item = result;
        } break;
    }
    
    return result;
}

// @COPYPASTA(long): lister__write_string__default
function Lister_Activation_Code Long_Lister_WriteString(Application_Links *app)
{
    Lister_Activation_Code result = ListerActivation_Continue;
    View_ID view = get_active_view(app, Access_Always);
    Lister *lister = view_get_lister(app, view);
    if (lister != 0){
        User_Input in = get_current_input(app);
        String_Const_u8 string = to_writable(&in);
        if (string.str != 0 && string.size > 0){
            lister_append_text_field(lister, string);
            lister_append_key(lister, string);
            lister->item_index = 0;
            lister_zero_scroll(lister);
            Long_Lister_FilterList(app, lister);
        }
    }
    return(result);
}

// @COPYPASTA(long): lister__backspace_text_field__default
function void Long_Lister_Backspace(Application_Links *app){
    View_ID view = get_active_view(app, Access_Always);
    Lister *lister = view_get_lister(app, view);
    if (lister != 0){
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
        lister->item_index = 0;
        lister_zero_scroll(lister);
        Long_Lister_FilterList(app, lister);
    }
}

// @COPYPASTA(long): lister__backspace_text_field__file_path
function void
Long_Lister_Backspace_Path(Application_Links *app){
    View_ID view = get_this_ctx_view(app, Access_Always);
    Lister *lister = view_get_lister(app, view);
    if (lister != 0){
        if (lister->text_field.size > 0){
            char last_char = lister->text_field.str[lister->text_field.size - 1];
            lister->text_field.string = backspace_utf8(lister->text_field.string);
            
            if (character_is_slash(last_char))
            {
                User_Input input = get_current_input(app);
                String_Const_u8 new_hot = string_remove_last_folder(lister->text_field.string);
                
                b32 is_modified = has_modifier(&input, KeyCode_Control);
                b32 whole_word_when_mod = def_get_config_b32(vars_save_string_lit("lister_whole_word_backspace_when_modified"));
                b32 whole_word_backspace = (is_modified == whole_word_when_mod);
                if (whole_word_backspace)
                    lister->text_field.size = new_hot.size;
                
                set_hot_directory(app, new_hot);
                // TODO(allen): We have to protect against lister_call_refresh_handler
                // changing the text_field here. Clean this up.
                String_u8 dingus = lister->text_field;
                Long_Lister_Refresh(app, lister);
                lister->text_field = dingus;
            }
            else
                lister_set_key(lister, string_front_of_path(lister->text_field.string));
            
            lister->item_index = 0;
            lister_zero_scroll(lister);
            Long_Lister_FilterList(app, lister);
        }
    }
}

// @COPYPASTA(long): run_lister
function Lister_Result Long_Lister_Run(Application_Links *app, Lister *lister)
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
    
    // TODO(long): Load this from the config file dynamically
    long_lister_tooltip_peek &= 0x7FFFFFFF;
    
    View_ID view = get_this_ctx_view(app, Access_Always);
    View_Context ctx = view_current_context(app, view);
    ctx.render_caller = Long_Lister_Render;
    ctx.hides_buffer = true;
    View_Context_Block ctx_block(app, view, &ctx);
    
    for (;;){
        User_Input in = get_next_input(app, EventPropertyGroup_Any, EventProperty_Escape);
        if (in.abort){
            block_zero_struct(&lister->out);
            lister->out.canceled = true;
            break;
        }
        
        Lister_Activation_Code result = ListerActivation_Continue;
        b32 handled = true;
        switch (in.event.kind){
            case InputEventKind_TextInsert:
            {
                if (lister->handlers.write_character != 0){
                    result = lister->handlers.write_character(app);
                }
            }break;
            
            case InputEventKind_KeyStroke:
            {
                switch (in.event.key.code){
                    case KeyCode_Return:
                    case KeyCode_Tab:
                    {
                        void *user_data = 0;
                        if (0 <= lister->raw_item_index &&
                            lister->raw_item_index < lister->options.count){
                            user_data = lister_get_user_data(lister, lister->raw_item_index);
                        }
                        lister_activate(app, lister, user_data, false);
                        result = ListerActivation_Finished;
                    }break;
                    
                    case KeyCode_Backspace:
                    {
                        if (lister->handlers.backspace != 0){
                            lister->handlers.backspace(app);
                        }
                        else if (lister->handlers.key_stroke != 0){
                            result = lister->handlers.key_stroke(app);
                        }
                        else{
                            handled = false;
                        }
                    }break;
                    
                    case KeyCode_Up:
                    {
                        if (lister->handlers.navigate != 0){
                            lister->handlers.navigate(app, view, lister, -1);
                        }
                        else if (lister->handlers.key_stroke != 0){
                            result = lister->handlers.key_stroke(app);
                        }
                        else{
                            handled = false;
                        }
                    }break;
                    
                    case KeyCode_Down:
                    {
                        if (lister->handlers.navigate != 0){
                            lister->handlers.navigate(app, view, lister, 1);
                        }
                        else if (lister->handlers.key_stroke != 0){
                            result = lister->handlers.key_stroke(app);
                        }
                        else{
                            handled = false;
                        }
                    }break;
                    
                    case KeyCode_PageUp:
                    {
                        if (lister->handlers.navigate != 0){
                            lister->handlers.navigate(app, view, lister,
                                                      -lister->visible_count);
                        }
                        else if (lister->handlers.key_stroke != 0){
                            result = lister->handlers.key_stroke(app);
                        }
                        else{
                            handled = false;
                        }
                    }break;
                    
                    case KeyCode_PageDown:
                    {
                        if (lister->handlers.navigate != 0){
                            lister->handlers.navigate(app, view, lister,
                                                      lister->visible_count);
                        }
                        else if (lister->handlers.key_stroke != 0){
                            result = lister->handlers.key_stroke(app);
                        }
                        else{
                            handled = false;
                        }
                    }break;
                    
                    default:
                    {
                        if (lister->handlers.key_stroke != 0){
                            result = lister->handlers.key_stroke(app);
                        }
                        else{
                            handled = Long_Lister_HandleKeyStroke(app, lister, in);
                        }
                    }break;
                }
            }break;
            
            case InputEventKind_MouseButton:
            {
                switch (in.event.mouse.code){
                    case MouseCode_Left:
                    {
                        Vec2_f32 p = V2f32(in.event.mouse.p);
                        void *clicked = lister_user_data_at_p(app, view, lister, p);
                        lister->hot_user_data = clicked;
                    }break;
                    
                    default:
                    {
                        handled = false;
                    }break;
                }
            }break;
            
            case InputEventKind_MouseButtonRelease:
            {
                switch (in.event.mouse.code){
                    case MouseCode_Left:
                    {
                        if (lister->hot_user_data != 0){
                            Vec2_f32 p = V2f32(in.event.mouse.p);
                            void *clicked = lister_user_data_at_p(app, view, lister, p);
                            if (lister->hot_user_data == clicked){
                                lister_activate(app, lister, clicked, true);
                                result = ListerActivation_Finished;
                            }
                        }
                        lister->hot_user_data = 0;
                    }break;
                    
                    default:
                    {
                        handled = false;
                    }break;
                }
            }break;
            
            case InputEventKind_MouseWheel:
            {
                Mouse_State mouse = get_mouse_state(app);
                lister->scroll.target.y += mouse.wheel;
                Long_Lister_FilterList(app, lister);
            }break;
            
            case InputEventKind_MouseMove:
            {
                Long_Lister_FilterList(app, lister);
            }break;
            
            case InputEventKind_Core:
            {
                switch (in.event.core.code){
                    case CoreCode_Animate:
                    {
                        Long_Lister_FilterList(app, lister);
                    }break;
                    
                    default:
                    {
                        handled = false;
                    }break;
                }
            }break;
            
            default:
            {
                handled = false;
            }break;
        }
        
        if (result == ListerActivation_Finished){
            break;
        }
        
        if (!handled){
            Mapping *mapping = lister->mapping;
            Command_Map *map = lister->map;
            
            Fallback_Dispatch_Result disp_result = fallback_command_dispatch(app, mapping, map, &in);
            if (disp_result.code == FallbackDispatch_DelayedUICall){
                call_after_ctx_shutdown(app, view, disp_result.func);
                break;
            }
            if (disp_result.code == FallbackDispatch_Unhandled){
                leave_current_input_unhandled(app);
            }
            else{
                Long_Lister_Refresh(app, lister);
            }
        }
    }
    
    return(lister->out);
}


function b32 Long_Index_IsMatch(F4_Index_ParseCtx* ctx, Token* token, String8* array, u64 count)
{
    return Long_Index_IsMatch(ctx, Ii64(token), array, count);
}

function b32 Long_Index_IsMatch(F4_Index_ParseCtx* ctx, Range_i64 range, String8* array, u64 count)
{
    b32 result = 0;
    String8 string = F4_Index_StringFromRange(ctx, range);
    for (u64 i = 0; i < count; ++i)
    {
        result = string_match(string, array[i]);
        if (result)
            break;
    }
    return result;
}

function b32 Long_Index_SkipExpression(F4_Index_ParseCtx* ctx, i16 seperator, i16 terminator)
{
    int paren_nest = 0;
    int scope_nest = 0;
    b32 result = false;
    
    while (!ctx->done)
    {
        Token* token = token_it_read(&ctx->it);
        b32 done = false;
        
        if (paren_nest == 0 && scope_nest == 0)
        {
            done = token->sub_kind == terminator || token->sub_kind == seperator;
            result = token->sub_kind == seperator;
        }
        
        switch (token->kind)
        {
            case TokenBaseKind_ParentheticalOpen:  { paren_nest++; } break;
            case TokenBaseKind_ParentheticalClose: { paren_nest--; } break;
            case TokenBaseKind_ScopeOpen:          { scope_nest++; } break;
            case TokenBaseKind_ScopeClose:         { scope_nest--; } break;
        }
        
        if (paren_nest < 0 || scope_nest < 0)
            done = true;
        
        if (done)
            break;
        F4_Index_ParseCtx_Inc(ctx, F4_Index_TokenSkipFlag_SkipWhitespace);
    }
    
    return result;
}

function b32 Long_Index_SkipBody(F4_Index_ParseCtx* ctx, b32 dec, b32 exit_early)
{
    return Long_Index_SkipBody(ctx->app, &ctx->it, ctx->file->buffer, dec);
}

function b32 Long_Index_SkipBody(Application_Links* app, Token_Iterator_Array* it, Buffer_ID buffer, b32 dec, b32 exit_early)
{
    i32 paren_nest = 0;
    i32 scope_nest = 0;
    i32 gener_nest = 0;
    i32 final_nest = 0;
    
    b32 (*next_token)(Token_Iterator_Array* it) = token_it_inc;
    if (dec) next_token = token_it_dec;
    
    while (true)
    {
        Token* token = token_it_read(it);
        b32 done = false, nest_found = false;
        switch (token->kind)
        {
            case TokenBaseKind_ScopeOpen:          { scope_nest++; nest_found = true; final_nest = +1; } break;
            case TokenBaseKind_ScopeClose:         { scope_nest--; nest_found = true; final_nest = -1; } break;
            case TokenBaseKind_ParentheticalOpen:  { paren_nest++; nest_found = true; final_nest = +1; } break;
            case TokenBaseKind_ParentheticalClose: { paren_nest--; nest_found = true; final_nest = -1; } break;
            
            case TokenBaseKind_Operator:
            {
                Scratch_Block scratch(app);
                String8 string = push_buffer_range(app, scratch, buffer, Ii64(token));
                
                // TODO(long): Replace this with a for loop and compare each char manually
                while (string.size)
                {
                    String8 sub_string = string_prefix(string, 1);
                    string = string_skip(string, 1);
                    
                    // NOTE(long): I use string comparison because > and >> are 2 seperate operators, and >>> or more is valid
                    if      (string_match(sub_string, S8Lit("<"))) { gener_nest++; nest_found = true; final_nest = +1; }
                    else if (string_match(sub_string, S8Lit(">"))) { gener_nest--; nest_found = true; final_nest = -1; }
                    else break;
                }
            } break;
        }
        
        {
            done = paren_nest == 0 && scope_nest == 0 && gener_nest == 0;
            if (dec) done |= (paren_nest > 0 || scope_nest > 0 || gener_nest > 0);
            else     done |= (paren_nest < 0 || scope_nest < 0 || gener_nest < 0);
        }
        
        if (done)
        {
            if (exit_early)
            {
                if (nest_found)
                    next_token(it);
                break;
            }
            
            if (!nest_found)
                break;
        }
        
        if (!next_token(it))
            break;
    }
    
    return paren_nest == 0 && scope_nest == 0 && gener_nest == 0 && (dec ? (final_nest >= 0) : (final_nest <= 0));
}

// NOTE(long): This function was ripped from F4_Index_ParsePattern and modified so that it takes a va_list and %k output a Range_i64
function b32 _Long_Index_ParsePattern(F4_Index_ParseCtx* ctx, char* fmt, va_list _args)
{
    b32 parsed = 1;
    
    F4_Index_ParseCtx ctx_restore = *ctx;
    F4_Index_TokenSkipFlags flags = F4_Index_TokenSkipFlag_SkipWhitespace|F4_Index_TokenSkipFlag_SkipComment;
    
    va_list args;
    va_copy(args, _args);
    for(int i = 0; fmt[i];)
    {
        if(fmt[i] == '%')
        {
            switch(fmt[i+1])
            {
                case 't':
                {
                    char *cstring = va_arg(args, char *);
                    String8 string = SCu8((u8 *)cstring, cstring_length(cstring));
                    parsed = parsed && F4_Index_RequireToken(ctx, string, flags);
                }break;
                
                case 'k':
                {
                    Token_Base_Kind kind = (Token_Base_Kind)va_arg(args, int);
                    Range_i64* output_range = va_arg(args, Range_i64*);
                    Token* token = 0;
                    parsed = parsed && F4_Index_RequireTokenKind(ctx, kind, &token, flags);
                    if (parsed && output_range)
                        *output_range = Ii64(token);
                }break;
                
                case 'b':
                {
                    i16 kind = (i16)va_arg(args, int);
                    Token **output_token = va_arg(args, Token **);
                    parsed = parsed && F4_Index_RequireTokenSubKind(ctx, kind, output_token, flags);
                }break;
                
                case 'n':
                {
                    F4_Index_NoteKind kind = (F4_Index_NoteKind)va_arg(args, int);
                    F4_Index_Note **output_note = va_arg(args, F4_Index_Note **);
                    Token *token = 0;
                    parsed = parsed && F4_Index_RequireTokenKind(ctx, TokenBaseKind_Identifier, &token, flags);
                    parsed = parsed && !!token;
                    if (parsed)
                    {
                        String8 token_string = F4_Index_StringFromToken(ctx, token);
                        F4_Index_Note *note = F4_Index_LookupNote(token_string, 0);
                        b32 kind_match = 0;
                        for(F4_Index_Note *n = note; n; n = n->next)
                        {
                            if(n->kind == kind)
                            {
                                kind_match = 1;
                                note = n;
                                break;
                            }
                        }
                        if (note && kind_match)
                        {
                            *output_note = note;
                            parsed = 1;
                        }
                        else
                        {
                            parsed = 0;
                        }
                    }
                }break;
                
                case 's':
                {
                    F4_Index_SkipSoftTokens(ctx, 0);
                }break;
                
                case 'o':
                {
                    F4_Index_SkipOpTokens(ctx);
                }break;
                
                default: break;
            }
            i += 1;
        }
        else
        {
            i += 1;
        }
    }
    
    va_end(args);
    
    if (!parsed)
        *ctx = ctx_restore;
    return parsed;
}

function b32 Long_Index_PeekPattern(F4_Index_ParseCtx* ctx, char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    F4_Index_ParseCtx ctx_restore = *ctx;
    b32 result = _Long_Index_ParsePattern(ctx, fmt, args);
    *ctx = ctx_restore;
    return result;
}

function b32 Long_Index_ParsePattern(F4_Index_ParseCtx* ctx, char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    return _Long_Index_ParsePattern(ctx, fmt, args);
}

function F4_Index_Note* Long_Index_MakeNote(F4_Index_ParseCtx* ctx, Range_i64 base_range, Range_i64 range, F4_Index_NoteKind kind,
                                            b32 push_parent)
{
    F4_Index_Note* note = F4_Index_MakeNote(ctx, range, kind, 0);
    note->base_range = base_range;
    if (push_parent)
        F4_Index_PushParent(ctx, note);
    return note;
}

function F4_Index_Note* Long_Index_LookupNote(String_Const_u8 string)
{
    F4_Index_Note* result = 0;
    u64 hash = table_hash_u8(string.str, string.size);
    u64 slot = hash % ArrayCount(f4_index.note_table);
    for (F4_Index_Note* note = f4_index.note_table[slot]; note; note = note->hash_next)
    {
        if (note->hash == hash && string_match(string, note->string))
        {
            result = note;
            break;
        }
    }
    return result;
}

function F4_Index_Note* Long_Index_LookupChild(F4_Index_Note* parent, String8 name)
{
    F4_Index_Note* result = 0;
    for (F4_Index_Note* child = parent->first_child; child; child = child->next_sibling)
    {
        if (string_match(child->string, name))
        {
            result = child;
            break;
        }
    }
    return result;
}

function F4_Index_Note* Long_Index_LookupChild(F4_Index_Note* parent, i32 index)
{
    F4_Index_Note* result = 0;
    i32 i = 0;
    for (F4_Index_Note* child = parent->first_child; child; child = child->next_sibling)
    {
        if (i == index)
        {
            result = child;
            break;
        }
        ++i;
    }
    return result;
}

function F4_Index_Note* Long_Index_LookupRef(Application_Links* app, Token_Array* array, F4_Index_Note* note)
{
    Token* token = token_from_pos(array, note->base_range.max - 1);
    F4_Index_Note* result = Long_Index_LookupBestNote(app, note->file->buffer, array, token);
    return result;
}

function void _Long_Index_ParseSelection(Application_Links* app, Arena* arena, Token_Iterator_Array* it, Buffer_ID buffer, String8List* list)
{
    Long_Index_SkipBody(app, it, buffer, true);
    Token* current = token_it_read(it);
    if (current && current->kind == TokenBaseKind_Identifier)
    {
        String8 string = push_buffer_range(app, arena, buffer, Ii64(current));
        
#define SLLQueuePushFront(f, l, n) ((f) == 0 ? ((f)=(l)=(n),(n)->next=0) : ((n)->next=(f),(f)=(n)))
        String8Node* node = push_array(arena, String8Node, 1);
        node->string = string;
        SLLQueuePushFront(list->first, list->last, node);
        list->node_count += 1;
        list->total_size += string.size;
        
        if (token_it_dec(it) && Long_CS_IsTokenSelection(token_it_read(it)) && token_it_dec(it))
            _Long_Index_ParseSelection(app, arena, it, buffer, list);
    }
}

// 2400 no decl
// 2526 no generic and array
// 2555 no this
// 2565 no initializer list

// TODO(long):
// Handle initializer list and constructor
// Handle inheritant
function F4_Index_Note* Long_Index_LookupBestNote(Application_Links* app, Buffer_ID buffer, Token_Array* array, Token* token, b32 preferNewNote)
{
    F4_Index_Note* result = 0;
    Range_i64 range = Ii64(token);
    Token_Iterator_Array it = token_iterator_pos(0, array, range.start);
    Scratch_Block scratch(app);
    String8List names = {};
    _Long_Index_ParseSelection(app, scratch, &it, buffer, &names);
    if (!names.first || !names.total_size || !names.node_count)
        return result;
    
#define MAX_DUPLICATE_NOTE_COUNT 128
    F4_Index_Note** duplicateNotes   = push_array_zero(scratch, F4_Index_Note*, MAX_DUPLICATE_NOTE_COUNT);
    F4_Index_Note** duplicateParents = push_array_zero(scratch, F4_Index_Note*, MAX_DUPLICATE_NOTE_COUNT);
    int count = 0;
    
    // NOTE(long): C# specific crap
    //b32 preferType = token_it_read(&it)->sub_kind == TokenCsKind_This;
    b32 preferType = string_match(push_token_lexeme(app, scratch, buffer, token), S8Lit("this"));
    
    for (F4_Index_Note* note = Long_Index_LookupNote(names.first->string); note; note = note->next)
    {
        if (count == MAX_DUPLICATE_NOTE_COUNT)
            break;
        
        if (note->range == Ii64(token))
        {
            if (preferNewNote)
                continue;
            result = note;
            goto DONE;
        }
        
        if (preferType && !(note->file->buffer == buffer && note->parent && note->parent->kind == F4_Index_NoteKind_Type))
            continue;
        
        F4_Index_Note* duplicateParent = 0;
        
        if (note->file->buffer == buffer)
        {
            if (range_contains(note->scope_range, token->pos))
                duplicateParent = note;
            else if (note->parent && range_contains(note->parent->scope_range, token->pos))
                duplicateParent = note->parent;
        }
        
        if (!note->parent || duplicateParent)
        {
            F4_Index_Note* child = note;
            
            for (String8Node* name = names.first->next; name; name = name->next)
            {
                F4_Index_Note* parent = child;
                child = Long_Index_LookupChild(parent, name->string);
                
                if (!child)
                {
                    F4_Index_Note* ref = Long_Index_LookupRef(app, array, parent);
                    if (ref)
                        child = Long_Index_LookupChild(ref, name->string);
                }
                
                if (!child)
                    break;
            }
            
            if (child)
            {
                duplicateNotes  [count] = child;
                duplicateParents[count] = duplicateParent;
                count++;
            }
        }
    }
    
    {
        int resultIndex = 0;
        for (i32 i = 1; i < count; ++i)
        {
            // NOTE(long): Prioritize parent->scope_range > note->scope_range > !parent
            if (duplicateParents[i])
            {
                F4_Index_Note** notes = duplicateParents[i] == duplicateParents[resultIndex] ? duplicateNotes : duplicateParents;
                if (!duplicateParents[resultIndex] || notes[i]->scope_range.min > notes[resultIndex]->scope_range.min)
                    resultIndex = i;
            }
        }
        result = duplicateNotes[resultIndex];
    }
    
    DONE:
    return result;
}

function String8 Long_GetStringAdvance(Application_Links* app, Face_ID face, String8 string, Token* token, i64 startPos, f32* advance)
{
    String8 result = string_substring(string, Ii64_size(token->pos - startPos, token->size));
    *advance += get_string_advance(app, face, result);
    return result;
}

#define Long_IterateRanges(tokens, ranges, count, foreach, callback) \
do { \
Range_i64* _ranges_ = (ranges); \
i64 _count_ = (count); \
for (i32 i = 0; i < _count_; ++i) \
{ \
{ foreach; } \
Token_Iterator_Array it = token_iterator_pos(0, (tokens), _ranges_[i].min); \
do { \
Token* token = token_it_read(&it); \
if (token->pos >= _ranges_[i].max) break; \
{ callback; } \
} while (token_it_inc_all(&it)); \
} \
} while (0)

function Rect_f32 Long_Index_DrawNote(Application_Links* app, Token_Array* array, F4_Index_Note* note,
                                      Face_ID face, f32 line_height, f32 padding, ARGB_Color color,
                                      Vec2_f32 tooltip_position, Range_f32 max_range_x,
                                      ARGB_Color highlight_color, Range_i64 highlight_range)
{
    Range_i64 total_range = { note->base_range.min, note->scope_range.min ? note->scope_range.min : note->range.max };
    {
        Token_Iterator_Array it = token_iterator_pos(0, array, total_range.max - 1);
        Token* token = token_it_read(&it);
        if (token->kind == TokenBaseKind_Whitespace)
            total_range.max = token->pos;
    }
    
    Scratch_Block scratch(app);
    String_Const_u8 string = push_buffer_range(app, scratch, note->file->buffer, total_range);
    f32 space_size = get_string_advance(app, face, S8Lit(" "));
    Vec2_f32 needed_size = { -space_size, line_height };
    Range_i64 ranges[] =
    {
        note->base_range,
        { note->range.min, total_range.max },
    };
    
    {
        Vec2_f32 maxP = { 0, line_height };
        b32 was_whitespace = 0;
        Long_IterateRanges(array, ranges, ArrayCount(ranges), needed_size.x += space_size,
                           f32 size_x = needed_size.x;
                           if (token->kind == TokenBaseKind_Whitespace) needed_size.x += space_size;
                           else Long_GetStringAdvance(app, face, string, token, total_range.min, &needed_size.x);
                           if (needed_size.x > range_size(max_range_x))
                           {
                               needed_size.x = 0;
                               if (was_whitespace)
                                   size_x -= space_size;
                               maxP.x = Max(maxP.x, size_x);
                               maxP.y += line_height;
                           }
                           was_whitespace = token->kind == TokenBaseKind_Whitespace);
        if (maxP.x)
            needed_size = maxP;
    }
    
    Rect_f32 draw_rect =
    {
        tooltip_position.x,
        tooltip_position.y,
        tooltip_position.x + needed_size.x + 2*padding,
        tooltip_position.y + needed_size.y + 2*padding,
    };
    
    if (draw_rect.x1 > max_range_x.max)
    {
        draw_rect.x0 -= draw_rect.x1 - max_range_x.max;
        draw_rect.x1 = max_range_x.max;
    }
    
    F4_DrawTooltipRect(app, draw_rect);
    
    Vec2_f32 text_position = draw_rect.p0 + Vec2_f32{ padding, padding };
    f32 start_x = text_position.x;
    text_position.x -= space_size;
    Long_IterateRanges(array, ranges, ArrayCount(ranges), text_position.x += space_size,
                       if (token->kind == TokenBaseKind_Whitespace) text_position.x += space_size;
                       else
                       {
                           Vec2_f32 pos = text_position;
                           String8 token_string = Long_GetStringAdvance(app, face, string, token, total_range.min, &text_position.x);
                           if (text_position.x >= draw_rect.x1)
                           {
                               text_position.x = start_x + text_position.x - pos.x;
                               pos.x = start_x;
                               pos.y = text_position.y += line_height;
                           }
                           draw_string(app, face, token_string, pos, color);
                           if (range_contains(highlight_range, token->pos))
                               draw_rectangle(app, Rf32(pos.x, pos.y + line_height, text_position.x, pos.y + line_height + 2), 1, color);
                       });
    
    return draw_rect;
}

function void Long_Index_DrawTooltip(Application_Links* app, View_ID view, Token_Array* array, F4_Index_Note* note, i32 index, Vec2_f32* tooltip_offset)
{
    Face_ID face = global_small_code_face;
    f32 line_height = get_face_metrics(app, face).line_height;
    f32 padding = 4.f;
    Vec2_f32 tooltip_position =
    {
        global_cursor_rect.x0,
        global_cursor_rect.y1,
    };
    Vec2_f32 offset = tooltip_offset ? *tooltip_offset : Vec2_f32{};
    tooltip_position += offset;
    offset = tooltip_position;
    ARGB_Color color = finalize_color(defcolor_text_default, 0);
    ARGB_Color highlight_color = finalize_color(fleury_color_token_highlight, 0);
    Rect_f32 screen_rect = view_get_screen_rect(app, view);
    Range_f32 screen_x = { screen_rect.x0 + padding * 4, screen_rect.x1 - padding * 4 };
    Range_i64 highlight_range = {};
    
    /*if (display_ref)
    {
        note = Long_Index_LookupRef(app, array, note);
        if (note && note->kind == F4_Index_NoteKind_Type)
        {
            for (F4_Index_Note* member = note->first_child; member; member = member->next_sibling)
            {
                Rect_f32 rect = Long_Index_DrawNote(app, array, member, face, line_height, padding, color, tooltip_position, screen_x);
                tooltip_position.y += rect.y1 - rect.y0;
            }
        }
    }
    else
    {
        Rect_f32 rect = Long_Index_DrawNote(app, array, note, face, line_height, padding, color, tooltip_position, screen_x);
        tooltip_position.y += rect.y1 - rect.y0;
    }*/
    
    if (note->kind == F4_Index_NoteKind_Decl && index)
        note = Long_Index_LookupRef(app, array, note);
    
    if (note->kind == F4_Index_NoteKind_Type && index)
    {
        for (F4_Index_Note* member = note->first_child; member; member = member->next_sibling)
        {
            Rect_f32 rect = Long_Index_DrawNote(app, array, member, face, line_height, padding, color,
                                                tooltip_position, screen_x, highlight_color, highlight_range);
            tooltip_position.y += rect.y1 - rect.y0;
        }
    }
    else
    {
        if (note->kind == F4_Index_NoteKind_Function)
        {
            F4_Index_Note* argument = Long_Index_LookupChild(note, index);
            if (argument)
                highlight_range = { argument->base_range.min, argument->scope_range.min };
        }
        Rect_f32 rect = Long_Index_DrawNote(app, array, note, face, line_height, padding, color,
                                            tooltip_position, screen_x, highlight_color, highlight_range);
        tooltip_position.y += rect.y1 - rect.y0;
    }
    
    if (tooltip_offset)
        *tooltip_offset += tooltip_position - offset;
}
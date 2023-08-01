
//~ NOTE(long): Predicate Functions

function b32 Long_Index_IsComment(F4_Index_Note* note)
{
    return note->kind == F4_Index_NoteKind_CommentTag || note->kind == F4_Index_NoteKind_CommentToDo;
}

function b32 Long_Index_IsGenericArgument(F4_Index_Note* note)
{
    F4_Index_Note* parent = note->parent;
    return (parent && note->kind == F4_Index_NoteKind_Type && Long_Index_IsArgument(note) &&
            (parent->kind == F4_Index_NoteKind_Type || parent->kind == F4_Index_NoteKind_Function));
}

function b32 Long_Index_IsNamespace(F4_Index_Note* note)
{
    return note->flags & F4_Index_NoteFlag_Namespace;
}

function b32 Long_Index_MatchNote(Application_Links* app, F4_Index_Note* note, Range_i64 range, String8 match)
{
    Scratch_Block scratch(app);
    String8 string = push_buffer_range(app, scratch, note->file->buffer, range);
    return string_match(string, match);
}

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

function b32 Long_Index_IsMatch(String8 string, String8* array, u64 count)
{
    for (u64 i = 0; i < count; ++i)
    if (string_match(string, array[i]))
    return true;
    return false;
}

//~ NOTE(long): Parsing Functions

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
    return Long_Index_SkipBody(ctx->app, &ctx->it, ctx->file->buffer, dec, exit_early);
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

// NOTE(long): This function was ripped from F4_Index_ParsePattern and modified so that it takes a va_list and output a Range_i64
function b32 _Long_Index_ParsePattern(F4_Index_ParseCtx* ctx, char* fmt, va_list _args)
{
    Long_Index_ProfileScope(ctx->app, "[Long] Parse Pattern");
    b32 parsed = 1;
    
    F4_Index_ParseCtx ctx_restore = *ctx;
    F4_Index_TokenSkipFlags flags = F4_Index_TokenSkipFlag_SkipAll;
    
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
                    Range_i64* output_range = va_arg(args, Range_i64*);
                    Token* token = 0;
                    parsed = parsed && F4_Index_RequireTokenSubKind(ctx, kind, &token, flags);
                    if (parsed && output_range)
                    *output_range = Ii64(token);
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

//~ NOTE(long): Indexing Function

function F4_Index_Note* Long_Index_MakeNote(F4_Index_ParseCtx* ctx, Range_i64 base_range, Range_i64 range, F4_Index_NoteKind kind,
                                            b32 push_parent)
{
    Assert(!ctx->active_parent || ctx->active_parent->kind != F4_Index_NoteKind_Scope || !Long_Index_IsNamespace(ctx->active_parent));
    F4_Index_Note* note = F4_Index_MakeNote(ctx, range, kind, 0);
    note->base_range = base_range;
    note->scope_range = {};
    if (push_parent)
    F4_Index_PushParent(ctx, note);
    return note;
}

function F4_Index_Note InitializeNamespaceRootBecauseCppIsAStupidLanguage()
{
    F4_Index_Note stupid_placeholder = {};
    stupid_placeholder.flags = F4_Index_NoteFlag_Namespace;
    return stupid_placeholder;
}

global F4_Index_Note namespace_root = InitializeNamespaceRootBecauseCppIsAStupidLanguage();

function F4_Index_Note* Long_Index_MakeNamespace(F4_Index_ParseCtx* ctx, Range_i64 base, Range_i64 name)
{
    String8 name_string = F4_Index_StringFromRange(ctx, name);
    if (!ctx->active_parent)
    ctx->active_parent = &namespace_root;
    F4_Index_Note* note = Long_Index_LookupChild(name_string, ctx->active_parent);
    if (note)   F4_Index_PushParent(ctx, note);
    else note = Long_Index_MakeNote(ctx, base, name, F4_Index_NoteKind_Type);
    Long_Index_CtxScope(ctx) = {};
    note->flags |= F4_Index_NoteFlag_Namespace;
    return note;
}

function F4_Index_Note* Long_Index_PushNamespaceScope(F4_Index_ParseCtx* ctx)
{
    F4_Index_Note* current_namespace = F4_Index_PushParent(ctx, 0);
    F4_Index_Note* namespace_scope = Long_Index_MakeNote(ctx, {}, Ii64(Long_Index_Token(ctx)), F4_Index_NoteKind_Scope);
    namespace_scope->flags |= F4_Index_NoteFlag_Namespace;
    namespace_scope->first_child = current_namespace->last_child;
    namespace_scope->parent = current_namespace->parent;
    current_namespace->parent = namespace_scope;
    F4_Index_PopParent(ctx, current_namespace);
    return namespace_scope;
}

function F4_Index_Note* Long_Index_PopNamespaceScope(F4_Index_ParseCtx* ctx)
{
    Long_Index_CtxScope(ctx) = {};
    F4_Index_Note* current_namespace = ctx->active_parent;
    F4_Index_Note* namespace_scope = current_namespace->parent;
    Assert(namespace_scope->kind == F4_Index_NoteKind_Scope);
    Assert(namespace_scope->parent);
    
    // Reset the parent note
    current_namespace->parent = namespace_scope->parent;
    namespace_scope->parent = 0;
    
    // Update the child note
    if (namespace_scope->first_child)
    namespace_scope->first_child = namespace_scope->first_child->next_sibling;
    else
        namespace_scope->first_child = current_namespace->first_child;
    namespace_scope->last_child = namespace_scope->first_child ? current_namespace->last_child : 0;
    return namespace_scope;
}

//~ NOTE(long): Lookup Functions

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

function F4_Index_Note* Long_Index_LookupChild(String8 name, F4_Index_Note* parent)
{
    if (parent)
    {
        for (F4_Index_Note* child = parent->first_child; child; child = child->next_sibling)
        if (string_match(child->string, name))
        return child;
    }
    else
    {
        for (F4_Index_Note* note = Long_Index_LookupNote(name); note; note = note->next)
        if (!note->parent || note->parent == &namespace_root)
        return note;
    }
    return 0;
}

function F4_Index_Note* Long_Index_LookupChild(F4_Index_Note* parent, i32 index)
{
    F4_Index_Note* result = 0;
    i32 i = 0;
    for (F4_Index_Note* child = parent ? parent->first_child : 0; child; child = child->next_sibling)
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
    F4_Index_Note* result = 0;
    if (note && range_size(note->base_range))
    {
        Token* token = token_from_pos(array, note->base_range.max - 1);
        result = Long_Index_LookupBestNote(app, note->file->buffer, array, token);
    }
    return result;
}

function void
Long_Index_ParseSelection(Application_Links* app, Arena* arena, Token_Iterator_Array* it, Buffer_ID buffer, String8List* list)
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
        
        if (token_it_dec(it))
        {
            b32 is_selection = 0;
            // TODO(long): Abstract selection function out
            {
                Scratch_Block scratch(app);
                String8 selections[] = { S8Lit("."), S8Lit("->"), S8Lit("?.") };
                String8 lexeme = push_token_lexeme(app, scratch, buffer, it->ptr);
                is_selection = Long_Index_IsMatch(lexeme, ExpandArray(selections));
            }
            if (is_selection && token_it_dec(it))
            Long_Index_ParseSelection(app, arena, it, buffer, list);
        }
    }
}

function F4_Index_Note* Long_Index_LookupNoteTree(F4_Index_File* file, F4_Index_Note* note, String8 string)
{
    for (F4_Index_Note* parent = note; ; parent = parent->parent)
    {
        F4_Index_Note* child = Long_Index_LookupChild(string, parent);
        if (child  ) return child;
        if (!parent) break;
    }
    
    for (String8ListNode* ref = file ? file->references : 0; ref; ref = ref->next)
    {
        F4_Index_Note* parent = 0;
        for (String8Node* name = ref->list.first; name; name = name->next)
        {
            parent = Long_Index_LookupChild(name->string, parent);
            if (!parent)
            break;
        }
        
        if (parent)
        {
            F4_Index_Note* child = Long_Index_LookupChild(string, parent);
            if (child)
            return child;
        }
    }
    
    return 0;
}

function F4_Index_Note* Long_Index_LookupScope(F4_Index_Note* note, i64 pos)
{
    F4_Index_Note* result = 0;
    
    if (note->range.min == pos)
    result = note;
    else if (note->scope_range.max && range_contains(Range_i64{ note->range.min, note->scope_range.max }, pos))
    {
        Long_Index_IterateValidNoteInFile(child, note)
        if (result = Long_Index_LookupScope(child, pos))
        break;
        
        if (!result)
        result = note;
    }
    return result;
}

function F4_Index_Note* Long_Index_GetSurroundingNote(F4_Index_File* file, i64 pos)
{
    F4_Index_Note* result = 0;
    for (F4_Index_Note* note = file ? file->first_note : 0; note; note = note->next_sibling)
    if (result = Long_Index_LookupScope(note, pos))
    break;
    return result;
}

// TODO(long):
// Handle initializer lists and constructors
// Handle inheritance
function F4_Index_Note* Long_Index_LookupBestNote(Application_Links* app, Buffer_ID buffer, Token_Array* array, Token* token)
{
    Long_Index_ProfileScope(app, "[Long] Lookup Best Note");
    F4_Index_Note* result = 0;
    
    Range_i64 range = Ii64(token);
    i64 pos = range.min;
    Token_Iterator_Array it = token_iterator_pos(0, array, pos);
    Scratch_Block scratch(app);
    String8List names = {};
    
    Long_Index_ParseSelection(app, scratch, &it, buffer, &names);
    if (!names.first || !names.total_size || !names.node_count)
    return result;
    
    Buffer_Cursor debug_cursor = buffer_compute_cursor(app, buffer, seek_pos(pos));
    
    for (F4_Index_Note* note = Long_Index_LookupNote(names.first->string); note; note = note->next)
    {
        if (note->range == range)
        {
            result = note;
            goto DONE;
        }
    }
    
    F4_Index_File* file = F4_Index_LookupFile(app, buffer);
    F4_Index_Note* surrounding_note = Long_Index_GetSurroundingNote(file, pos);
    // TODO(long): C# and C++ specific crap - Abstract this out for other languages
    if (string_match(push_token_lexeme(app, scratch, buffer, it.ptr), S8Lit("this")))
    while (surrounding_note && surrounding_note->kind != F4_Index_NoteKind_Type)
    surrounding_note = surrounding_note->parent;
    
    F4_Index_Note* note = Long_Index_LookupNoteTree(file, surrounding_note, names.first->string);
    // NOTE(long): Choose parent types over the child functions when they have the same name (constructors, conversion operators, etc)
    if (note && note->parent)
    if (note->kind == F4_Index_NoteKind_Function && note->parent->kind == F4_Index_NoteKind_Type)
    if (string_match(note->string, note->parent->string)) 
    note = note->parent;
    
    if (note)
    {
        F4_Index_Note* child = note;
        for (String8Node* name = names.first->next; name; name = name->next)
        {
            F4_Index_Note* parent = child;
            child = Long_Index_LookupChild(name->string, parent);
            
            if (!child)
            {
                F4_Index_Note* ref = Long_Index_LookupRef(app, array, parent);
                if (ref)     child = Long_Index_LookupChild(name->string, ref);
                if (!child) break;
            }
        }
        result = child;
    }
    
    DONE:
    return result;
}

//~ NOTE(long): Render Functions

function String8 Long_GetStringAdvance(Application_Links* app, Face_ID face, String8 string, Token* token, i64 startPos, f32* advance)
{
    Range_i64 range = Ii64_size(token->pos - startPos, token->size);
    String8 result = string_substring(string, range);
    *advance += get_string_advance(app, face, result);
    return result;
}

function Rect_f32 Long_Index_DrawNote(Application_Links* app, Range_i64_Array ranges, Buffer_ID buffer,
                                      Face_ID face, f32 line_height, f32 padding, ARGB_Color color,
                                      Vec2_f32 tooltip_position, Range_f32 max_range_x,
                                      ARGB_Color highlight_color, Range_i64 highlight_range)
{
    Token_Array* array = 0;
    {
        Token_Array _array = get_token_array_from_buffer(app, buffer);
        array = &_array;
    }
    
    i64 max_range_pos = ranges.ranges[ranges.count - 1].max;
    if (!max_range_pos)
    system_error_box("The last range of the note is incorrect");
    {
        Token_Iterator_Array it = token_iterator_pos(0, array, max_range_pos - 1);
        Token* token = token_it_read(&it);
        if (token->kind == TokenBaseKind_Whitespace)
        max_range_pos = token->pos;
    }
    Range_i64 total_range = { ranges.ranges[0].min, max_range_pos };
    
    Scratch_Block scratch(app);
    String_Const_u8 string = push_buffer_range(app, scratch, buffer, total_range);
    {
        Range_i64 range = buffer_range(app, buffer);
        Assert(range_union(range, total_range) == range);
        Assert(string.str && string.size);
    }
    f32 space_size = get_string_advance(app, face, S8Lit(" "));
    Vec2_f32 needed_size = { 0, line_height };
    
#define Long_IterateRanges(tokens, ranges, foreach, callback) \
do { \
Range_i64* _ranges_ = (ranges).ranges; \
i64 _count_ = (ranges).count; \
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
    
    {
        Vec2_f32 maxP = needed_size;
        b32 was_whitespace = 0;
        // TODO(long): Maybe skip comment?
        Long_IterateRanges(array, ranges, needed_size.x += (needed_size.x ? space_size : 0),
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
    Long_IterateRanges(array, ranges, text_position.x += (text_position.x != start_x) ? space_size : 0,
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
                           {
                               Rect_f32 rect = Rf32(pos.x, pos.y + line_height, text_position.x, pos.y + line_height + 2);
                               draw_rectangle(app, rect, 1, highlight_color);
                           }
                       });
#undef Long_IterateRanges
    
    return draw_rect;
}

function Range_i64_Array Long_Index_GetNoteRanges(Application_Links* app, Arena* arena, F4_Index_Note* note, Range_i64 range)
{
    // NOTE(long): I need an array of ranges for handling trailing decl: int a = ..., b = ..., c = ...;
    // if I want to display b, I needs to draw `int b = ...`
    Range_i64_Array result = { push_array_zero(arena, Range_i64, 3), 0 };
    
    if (note->base_range.max)
    result.ranges[result.count++] = note->base_range;
    result.ranges[result.count++] = Long_Index_ArgumentRange(note);
    if (range.max)
    result.ranges[result.count++] = range;
    
    return result;
}

function void Long_Index_DrawTooltip(Application_Links* app, Rect_f32 screen_rect, Token_Array* array, Vec2_f32* tooltip_offset,
                                     F4_Index_Note* note, i32 index, Range_i64 range, Range_i64 highlight_range)
{
    Vec2_f32 tooltip_position =
    {
        global_cursor_rect.x0,
        global_cursor_rect.y1,
    };
    Vec2_f32 offset = tooltip_offset ? *tooltip_offset : Vec2_f32{};
    tooltip_position += offset;
    offset = tooltip_position;
    
    Face_ID face = global_small_code_face;
    f32 padding = 4.f;
    f32 line_height = get_face_metrics(app, face).line_height;
    ARGB_Color color = finalize_color(defcolor_text_default, 0);
    ARGB_Color highlight_color = finalize_color(fleury_color_token_highlight, 0);
    Range_f32 screen_x = { screen_rect.x0 + padding * 4, screen_rect.x1 - padding * 4 };
    
    Scratch_Block scratch(app);
    
    if (note->kind == F4_Index_NoteKind_Decl && index)
    {
        note = Long_Index_LookupRef(app, array, note);
        if (!note)
        return;
    }
    
    if (note->kind == F4_Index_NoteKind_Type && index)
    {
        for (note = note->first_child; note; note = note->next_sibling)
        {
            if (note->kind == F4_Index_NoteKind_Scope || Long_Index_IsComment(note))
            continue;
            Range_i64_Array ranges = Long_Index_GetNoteRanges(app, scratch, note, range);
            Rect_f32 rect = Long_Index_DrawNote(app, ranges, note->file->buffer,
                                                face, line_height, padding, color,
                                                tooltip_position, screen_x, highlight_color, highlight_range);
            tooltip_position.y += rect.y1 - rect.y0;
        }
    }
    else
    {
        if (note->kind == F4_Index_NoteKind_Function && index != -1)
        {
            F4_Index_Note* argument = Long_Index_LookupChild(note, index);
            if (argument && argument->kind == F4_Index_NoteKind_Decl)
            // NOTE(long): The CS parser always parses base_range and scope_range for decl, while the CPP parser never parses decl
            // so I don't need to check these ranges
            highlight_range = { argument->base_range.min, argument->scope_range.min };
        }
        Range_i64_Array ranges = Long_Index_GetNoteRanges(app, scratch, note, range);
        Rect_f32 rect = Long_Index_DrawNote(app, ranges, note->file->buffer,
                                            face, line_height, padding, color,
                                            tooltip_position, screen_x, highlight_color, highlight_range);
        tooltip_position.y += rect.y1 - rect.y0;
    }
    
    if (tooltip_offset)
    *tooltip_offset += tooltip_position - offset;
}

function void Long_Index_DrawPosContext(Application_Links* app, View_ID view, Token_Array* array, F4_Language_PosContextData* first_ctx)
{
    Vec2_f32 offset = {};
    Rect_f32 screen = view_get_screen_rect(app, view);
    for (F4_Language_PosContextData* ctx = first_ctx; ctx; ctx = ctx->next)
    {
        if (ctx->relevant_note)
        Long_Index_DrawTooltip(app, screen, array, &offset,
                               ctx->relevant_note, ctx->argument_index, ctx->range, ctx->highlight_range);
    }
}

#define LONG_INDEX_DRAW_PARTIAL 0

function void Long_Index_DrawCodePeek(Application_Links* app, View_ID view)
{
    if (!global_code_peek_open)
    return;
    
    F4_Index_Note* note = 0;
    {
        View_ID active_view = get_active_view(app, Access_Always);
        i64 pos = view_get_cursor_pos(app, active_view);
        Buffer_ID buffer = view_get_buffer(app, active_view, Access_Always);
        Token* token = get_token_from_pos(app, buffer, pos);
        if (token != 0 && token->size > 0 && token->kind == TokenBaseKind_Identifier)
        {
            Token_Array array = get_token_array_from_buffer(app, buffer);
            note = Long_Index_LookupBestNote(app, buffer, &array, token);
            if (note && note->range.min == token->pos && (note->flags & F4_Index_NoteFlag_Prototype))
            {
                F4_Index_Note* parent = note->parent;
                F4_Index_Note* old_note = note;
                while (note->prev)
                note = note->prev;
                for (; note; note = note->next)
                if (note->parent == parent && !(note->flags & F4_Index_NoteFlag_Prototype))
                break;
                if (!note)
                note = old_note;
            }
        }
    }
    
    if (!note)
    return;
    
    Buffer_ID buffer = note->file->buffer;
    Range_i64 range = note->range;
#if LONG_INDEX_DRAW_PARTIAL
    range = {
        note-> base_range.max ? note-> base_range.min : note->range.min,
        note->scope_range.max ? note->scope_range.max : note->range.max,
    };
    range.max += 1;
#endif
    
    Rect_f32 view_rect = view_get_screen_rect(app, view);
    i32 peek_count = 1;
    f32 peek_height = (f32)((view_rect.y1 - view_rect.y0) * (0.5f + 0.4f*(clamp_top(peek_count / 4, 1)))) / peek_count;
    Rect_f32 rect = {0};
    {
        rect.x0 = view_rect.x0;
        rect.x1 = view_rect.x1;
        rect.y0 = view_rect.y1 - peek_height*peek_count;
        rect.y1 = rect.y0 + peek_height;
    }
    Rect_f32 inner_rect = rect_inner(rect, 30);
    
    F4_DrawTooltipRect(app, rect);
    
    Buffer_Point buffer_point = { get_line_number_from_pos(app, buffer, range.min) };
    range.max = clamp_bot(range.max, get_line_end(app, buffer, buffer_point.line_number).pos);
    Text_Layout_ID text_layout_id = text_layout_create(app, buffer, inner_rect, buffer_point);
    
    Rect_f32 prev_prev_clip = draw_set_clip(app, inner_rect);
    {
        Token_Array array = get_token_array_from_buffer(app, buffer);
        if(array.tokens != 0)
        {
            F4_SyntaxHighlight(app, text_layout_id, &array);
#if LONG_INDEX_DRAW_PARTIAL
            Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
            if (visible_range.max > range.max)
            paint_text_color(app, text_layout_id, Ii64(range.max, visible_range.max), 0);
#endif
        }
        else
            paint_text_color_fcolor(app, text_layout_id, range, fcolor_id(defcolor_text_default));
        
        draw_text_layout_default(app, text_layout_id);
    }
    draw_set_clip(app, prev_prev_clip);
    text_layout_free(app, text_layout_id);
}

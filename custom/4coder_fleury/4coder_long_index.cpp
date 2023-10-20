
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

function b32 Long_Index_IsLambda(F4_Index_Note* note)
{
    return note->kind == F4_Index_NoteKind_Function && !range_size(note->range);
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

function b32 Long_Index_ParseStr(F4_Index_ParseCtx* ctx, String8 str)
{
    Long_Index_ProfileScope(ctx->app, "[Long] Parse String");
    F4_Index_ParseCtx ctx_restore = *ctx;
    b32 result = 0;
    
    Token* token = token_it_read(&ctx->it);
    if (token)
    result = string_match(str, string_substring(ctx->string, Ii64(token)));
    else
        ctx->done = 1;
    
    if (result)
    F4_Index_ParseCtx_Inc(ctx, F4_Index_TokenSkipFlag_SkipAll);
    else
        *ctx = ctx_restore;
    
    return result;
}

function b32 Long_Index_PeekStr(F4_Index_ParseCtx* ctx, String8 str)
{
    F4_Index_ParseCtx ctx_restore = *ctx;
    b32 result = Long_Index_ParseStr(ctx, str);
    *ctx = ctx_restore;
    return result;
}

function b32 Long_Index_ParseKind(F4_Index_ParseCtx* ctx, Token_Base_Kind kind, Range_i64* out_range)
{
    F4_Index_ParseCtx ctx_restore = *ctx;
    b32 result = 0;
    
    Token* token = token_it_read(&ctx->it);
    if (token)
    result = token->kind == kind;
    else
        ctx->done = 1;
    
    if(result)
    {
        *out_range = Ii64(token);
        F4_Index_ParseCtx_Inc(ctx, F4_Index_TokenSkipFlag_SkipAll);
    }
    else
        *ctx = ctx_restore;
    
    return result;
}

function b32 Long_Index_ParseSubKind(F4_Index_ParseCtx* ctx, i64 sub_kind, Range_i64* out_range)
{
    F4_Index_ParseCtx ctx_restore = *ctx;
    b32 result = 0;
    
    Token* token = token_it_read(&ctx->it);
    if (token)
    result = token->sub_kind == sub_kind;
    else
        ctx->done = 1;
    
    if(result)
    {
        if (out_range) *out_range = Ii64(token);
        F4_Index_ParseCtx_Inc(ctx, F4_Index_TokenSkipFlag_SkipAll);
    }
    else *ctx = ctx_restore;
    
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

// COPYPASTA(long): F4_Index_ParsePattern
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

function F4_Index_Note InitializeNamespaceRootBecauseCppIsAStupidLanguage()
{
    F4_Index_Note stupid_placeholder = {};
    stupid_placeholder.flags = F4_Index_NoteFlag_Namespace;
    return stupid_placeholder;
}

global F4_Index_Note namespace_root = InitializeNamespaceRootBecauseCppIsAStupidLanguage();

function F4_Index_Note* Long_Index_LoadRef(Application_Links* app, F4_Index_Note* note, F4_Index_Note* filter_note)
{
    Long_Index_ProfileScope(app, "[Long] Lookup References");
    if (!note->file) return 0;
    
    F4_Index_Note* result = 0;
    Buffer_ID buffer = note->file->buffer;
    Token_Array array = get_token_array_from_buffer(app, buffer);
    
    // NOTE(long): Normally, ParseSelection will stop when it doesn't see any identifiers or selection operators,
    // so if you have a derived class that inherits from a based class and multiple interfaces, it will return the last interface.
    // But you usually want it to return the based class, and because C# is stupid, the based class always needs to be declared first.
    // So passing continue_after_comma as true here is just a quick get-around.
    // Presumably, when we get more serious about this, LookupRef should just return an array.
    if (note->kind == F4_Index_NoteKind_Type && Long_Index_HasScopeRange(note))
    result = Long_Index_LookupBestNote(app, buffer, &array, token_from_pos(&array, note->scope_range.min - 1), filter_note, 1);
    else if (range_size(note->base_range))
    result = Long_Index_LookupBestNote(app, buffer, &array, token_from_pos(&array, note-> base_range.max - 1), filter_note);
    
    if (result == note || result == filter_note) result = 0;
    return result;
}

function void Long_Index_PreloadRef(Application_Links* app, F4_Index_Note* note)
{
    for (F4_Index_Note* child = note->first_child; child; child = child->next_sibling)
    Long_Index_PreloadRef(app, child);
    note->ref = Long_Index_LoadRef(app, note, note);
}

function void Long_Index_ClearRef(Application_Links* app, F4_Index_Note* note)
{
    for (F4_Index_Note* child = note->first_child; child; child = child->next_sibling)
    Long_Index_ClearRef(app, child);
    note->ref = 0;
}

function void Long_Index_PreloadRef(Application_Links* app)
{
    Long_Index_ClearRef  (app, &namespace_root);
    Long_Index_PreloadRef(app, &namespace_root);
}

function void Long_Index_EraseNote(Application_Links* app, F4_Index_Note* note)
{
    Long_Index_IterateValidNoteInFile(child, note)
    Long_Index_EraseNote(app, child);
    
    Assert(!(Long_Index_IsNamespace(note) && note->kind == F4_Index_NoteKind_Type));
    
    F4_Index_Note* prev = note->prev_sibling;
    F4_Index_Note* next = note->next_sibling;
    if (prev) prev->next_sibling = next;
    if (next) next->prev_sibling = prev;
    
    F4_Index_Note* parent = note->parent;
    if (parent && Long_Index_IsNamespace(parent))
    {
        if (!prev)
        parent->first_child = next;
        if (!next)
        parent->last_child = prev;
    }
}

// COPYPASTA(long): _F4_Index_FreeNoteTree
function void Long_Index_FreeNoteTree(F4_Index_Note *note)
{
    Long_Index_IterateValidNoteInFile(child, note)
    Long_Index_FreeNoteTree(child);
    
    F4_Index_Note *prev = note->prev;
    F4_Index_Note *next = note->next;
    F4_Index_Note *hash_prev = note->hash_prev;
    F4_Index_Note *hash_next = note->hash_next;
    
    if (prev) prev->next = next;
    if (next) next->prev = prev;
    
    if (!prev)
    {
        if (next)
        {
            next->hash_prev = hash_prev;
            next->hash_next = hash_next;
            if (hash_prev) hash_prev->hash_next = next;
            if (hash_next) hash_next->hash_prev = next;
        }
        else
        {
            if (hash_prev) hash_prev->hash_next = hash_next;
            if (hash_next) hash_next->hash_prev = hash_prev;
        }
        
        if (!hash_prev)
        f4_index.note_table[note->hash % ArrayCount(f4_index.note_table)] = next ? next : hash_next;
    }
}

function i32 Long_EndBuffer(Application_Links* app, Buffer_ID buffer_id)
{
    F4_Index_Lock();
    F4_Index_File* file = F4_Index_LookupFile(app, buffer_id);
    if (file)
    {
        // NOTE(long): This is like F4_Index_ClearFile but doesn't reset the first/last_note pointer because EraseNote needs those.
        {
            for(F4_Index_Note *note = file->first_note; note; note = note->next_sibling)
            Long_Index_FreeNoteTree(note);
            linalloc_clear(&file->arena);
        }
        
        for (F4_Index_Note* note = file->first_note; note; note = note->next_sibling)
        Long_Index_EraseNote(app, note);
        
        F4_Index_EraseFile(app, buffer_id);
#if LONG_INDEX_PRELOAD_REF
        Long_Index_PreloadRef(app);
#endif
    }
    F4_Index_Unlock();
    return end_buffer_close_jump_list(app, buffer_id);
}

function F4_Index_Note* Long_Index_MakeNote(F4_Index_ParseCtx* ctx, Range_i64 base_range, Range_i64 range, F4_Index_NoteKind kind,
                                            b32 push_parent)
{
    Long_Index_ProfileScope(ctx->app, "[Long] Make Note");
    Assert(!ctx->active_parent || ctx->active_parent->kind != F4_Index_NoteKind_Scope || !Long_Index_IsNamespace(ctx->active_parent));
    F4_Index_Note* note = F4_Index_MakeNote(ctx, range, kind, 0);
    note->base_range = base_range;
    note->scope_range = {};
    if (push_parent)
    F4_Index_PushParent(ctx, note);
    return note;
}

function F4_Index_Note* Long_Index_MakeNamespace(F4_Index_ParseCtx* ctx, Range_i64 base, Range_i64 name)
{
    String8 name_string = F4_Index_StringFromRange(ctx, name);
    if (!ctx->active_parent)
    ctx->active_parent = &namespace_root;
    F4_Index_Note* note = Long_Index_LookupChild(name_string, ctx->active_parent);
    if (note) F4_Index_PushParent(ctx, note);
    else
    {
        // NOTE(long): Because note always has at least namespace_root as parent, file->first/last_note will never point to it
        note = Long_Index_MakeNote(ctx, base, name, F4_Index_NoteKind_Type);
        // NOTE(long): F4_Index_MakeNote has already copied the string to the file->arena, so there's some memory leak
        note->string = push_string_copy(&f4_index.arena, note->string);
        note->file = 0;
    }
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
    
    namespace_scope->scope_range = { current_namespace->scope_range.min, ctx->it.ptr->pos };
    current_namespace = {};
    
    return namespace_scope;
}

//~ NOTE(long): Lookup Functions

function F4_Index_Note* Long_Index_LookupNote(String8 string)
{
    u64 hash = table_hash_u8(string.str, string.size);
    u64 slot = hash % ArrayCount(f4_index.note_table);
    
    for (F4_Index_Note* note = f4_index.note_table[slot]; note; note = note->hash_next)
    if (note->hash == hash && string_match(string, note->string))
    return note;
    
    return 0;
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

function F4_Index_Note* Long_Index_LookupRef(Application_Links* app, F4_Index_Note* note, F4_Index_Note* filter_note)
{
#if LONG_INDEX_PRELOAD_REF
    return note->ref != filter_note ? note->ref : 0;
#else
    return Long_Index_LoadRef(app, note, filter_note);
#endif
}

function void Long_Index_ParseSelection(Application_Links* app, Arena* arena, Token_Iterator_Array* it, Buffer_ID buffer,
                                        String8List* list, b32 continue_after_comma)
{
    Token* current = token_it_read(it);
    if (current && (current->kind == TokenBaseKind_Whitespace || current->kind == TokenBaseKind_Comment))
    token_it_dec(it);
    
    Long_Index_SkipBody(app, it, buffer, 1);
    current = token_it_read(it);
    
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
            // TODO(long): Abstract selection function and is_comma out
            b32 is_selection = 0;
            {
                String8 selections[] = { S8Lit("."), S8Lit("->"), S8Lit("?.") };
                Scratch_Block scratch(app);
                String8 lexeme = push_token_lexeme(app, scratch, buffer, it->ptr);
                is_selection = Long_Index_IsMatch(lexeme, ExpandArray(selections));
            }
            
            if (is_selection && token_it_dec(it))
            Long_Index_ParseSelection(app, arena, it, buffer, list, continue_after_comma);
            
            b32 is_comma = 0;
            if (continue_after_comma)
            {
                Scratch_Block scratch(app);
                String8 lexeme = push_token_lexeme(app, scratch, buffer, it->ptr);
                is_comma = string_match(lexeme, S8Lit(","));
            }
            
            if (is_comma && token_it_dec(it))
            {
                *list = {};
                Long_Index_ParseSelection(app, arena, it, buffer, list, continue_after_comma);
            }
        }
    }
}

function F4_Index_Note* Long_Index_LookupNoteTree(Application_Links* app, F4_Index_File* file, F4_Index_Note* note, String8 string)
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
            Long_Index_ProfileBlock(app, "[Long] Lookup Imports");
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

function F4_Index_Note* Long_Index_LookupNoteFromList(Application_Links* app, String8List names, i64 pos,
                                                      F4_Index_File* file, F4_Index_Note* surrounding_note, F4_Index_Note* filter_note,
                                                      b32 prefer_func, b32 prefer_type)
{
    Long_Index_ProfileScope(app, "[Long] Lookup List");
    for (F4_Index_Note* note = names.first ? Long_Index_LookupNote(names.first->string) : 0; note; note = note->next)
    if (note->file == file && note->range.min == pos)
    return note;
    
    F4_Index_Note* result = 0;
    if (names.first)
    {
        F4_Index_Note* note = Long_Index_LookupNoteTree(app, file, surrounding_note, names.first->string);
        
        // NOTE(long): Lookup note from the inheritance tree
        if (!note)
        {
            for (F4_Index_Note* parent = surrounding_note; parent; parent = parent->parent)
            {
                if (parent != filter_note && parent->kind == F4_Index_NoteKind_Type)
                {
                    F4_Index_Note* base_type = parent;
                    while (!note && (base_type = Long_Index_LookupRef(app, base_type, parent)))
                    note = Long_Index_LookupChild(names.first->string, base_type);
                    if (note) break;
                }
            }
        }
        
        // NOTE(long): Choose parent types over the child functions when they have the same name (constructors, conversion operators, etc)
        if (note)
        {
            if (prefer_func)
            {
                if (note->kind == F4_Index_NoteKind_Type)
                {
                    for (F4_Index_Note* constructor = note->first_child; constructor; constructor = constructor->next_sibling)
                    if (string_match(constructor->string, note->string) && !constructor->base_range.max)
                    note = constructor;
                }
            }
            else if (note->parent)
            {
                if (note->kind == F4_Index_NoteKind_Function && note->parent->kind == F4_Index_NoteKind_Type)
                if (string_match(note->string, note->parent->string))
                note = note->parent;
            }
        }
        
        if (note)
        {
            F4_Index_Note* child = note;
            
            i32 i = 0;
            for (String8Node* name = names.first->next; name; name = name->next, ++i)
            {
                // NOTE(long): The reason we need a filter_note is to make sure cases like this never crash:
                // TestA.anything TestA;
                // Or
                // TestA.stuff1 TestB;
                // TestB.stuff2 TestA;
                // Or so on for TestC, TestD, etc
                
                F4_Index_Note* parent = child;
                child = Long_Index_LookupChild(name->string, parent);
                
                while (!child && parent && parent != filter_note)
                {
                    if (!filter_note) filter_note = parent;
                    parent = Long_Index_LookupRef(app, parent, filter_note);
                    child = Long_Index_LookupChild(name->string, parent);
                }
                
                if (!child)
                break;
            }
            result = child;
        }
    }
    else if (prefer_type) result = surrounding_note;
    
    return result;
}



// TODO(long):
// Handle initializer lists
// Handle generic fields
// NOTE(long): The result isn't filtered out by filter_note, so be careful
function F4_Index_Note* Long_Index_LookupBestNote(Application_Links* app, Buffer_ID buffer, Token_Array* array, Token* token,
                                                  F4_Index_Note* filter_note, b32 use_first)
{
    Long_Index_ProfileScope(app, "[Long] Lookup Best Note");
    Scratch_Block scratch(app);
    
    String8List names = {};
    Range_i64 range = Ii64(token);
    Token_Iterator_Array it = token_iterator_pos(0, array, range.min);
    Long_Index_ParseSelection(app, scratch, &it, buffer, &names, use_first);
    
    Buffer_Cursor debug_cursor = buffer_compute_cursor(app, buffer, seek_pos(range.min));
    String8 debug_filename = push_buffer_base_name(app, scratch, buffer);
    
    F4_Index_File* file = F4_Index_LookupFile(app, buffer);
    F4_Index_Note* scope_note = Long_Index_GetSurroundingNote(file, range.min);
    
    // TODO(long): C# and C++ specific crap - Abstract this out for other languages
    b32 prefer_type, prefer_func;
    {
        String8 start_string = push_token_lexeme(app, scratch, buffer, it.ptr);
        prefer_type = string_match(start_string, S8Lit("this"));
        prefer_func = string_match(start_string, S8Lit("new"));
        
        // NOTE(long): This is for `this.myField`
        if (prefer_type)
        {
            while (scope_note && scope_note->kind != F4_Index_NoteKind_Type)
            scope_note = scope_note->parent;
        }
        // NOTE(long): This is for Object Initializers in C#. This can be slightly modified to handle Designated Initializers in C/C++
        else if (scope_note)
        {
            b32 is_obj_field = it.ptr->kind == TokenBaseKind_ScopeOpen;
            if (!is_obj_field && string_match(start_string, S8Lit(",")))
            {
                Token_Iterator_Array iterator = it;
                if (token_it_inc(&iterator) && token_it_inc(&iterator))
                is_obj_field = iterator.ptr->kind == TokenBaseKind_ScopeClose ||
                    string_match(push_token_lexeme(app, scratch, buffer, iterator.ptr), S8Lit("="));
            }
            
            if (is_obj_field)
            {
                Token_Iterator_Array it = token_iterator_pos(0, array, scope_note->range.min);
                if (token_it_dec(&it) && it.ptr->kind == TokenBaseKind_Identifier)
                {
                    Token* type_token = it.ptr;
                    if (token_it_dec(&it) && it.ptr->kind == TokenBaseKind_Keyword)
                    {
                        if (string_match(push_token_lexeme(app, scratch, buffer, it.ptr), S8Lit("new")))
                        {
                            F4_Index_Note* new_scope = Long_Index_LookupBestNote(app, buffer, array, type_token, filter_note);
                            if (new_scope)
                            scope_note = new_scope;
                        }
                    }
                }
            }
        }
        
        // NOTE(long): This is for constructor: `a = new MyType(...)`
        if (prefer_func)
        {
            Token_Iterator_Array it = token_iterator_pos(0, array, range.min);
            if (token_it_inc(&it))
            prefer_func = it.ptr->kind != TokenBaseKind_ScopeOpen;
        }
    }
    
    F4_Index_Note* result = Long_Index_LookupNoteFromList(app, names, range.min, file, scope_note, filter_note, prefer_func, prefer_type);
    if (!result && names.last)
    {
        names.last->string = push_stringf(scratch, "%.*s%s", string_expand(names.last->string), "Attribute");
        result = Long_Index_LookupNoteFromList(app, names, range.min, file, scope_note, filter_note, prefer_func, prefer_type);
    }
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
        if (token && token->kind == TokenBaseKind_Whitespace)
        max_range_pos = token->pos;
    }
    Range_i64 total_range = { ranges.ranges[0].min, max_range_pos };
    
    Scratch_Block scratch(app);
    String8 string = push_buffer_range(app, scratch, buffer, total_range);
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
if (token && token->pos >= _ranges_[i].max) break; \
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

global b32 long_global_pos_context_open    = 1;
global i32 long_global_child_tooltip_count = 20;
global i32 long_active_pos_context_option  = 0;

function LONG_INDEX_FILTER(Long_Filter_Decl) { return note->kind == F4_Index_NoteKind_Decl || note->kind == F4_Index_NoteKind_Constant; }
function LONG_INDEX_FILTER(Long_Filter_Func) { return note->kind == F4_Index_NoteKind_Function && !Long_Index_IsLambda(note); }
function LONG_INDEX_FILTER(Long_Filter_Type) { return note->kind == F4_Index_NoteKind_Type/**/ && !Long_Index_IsGenericArgument(note); }

global NoteFilter* long_global_context_opts[] =
{
    Long_Filter_Decl,
    Long_Filter_Func,
    Long_Filter_Type,
};

function Rect_f32 Long_Index_DrawString(Application_Links* app, String8 string, Vec2_f32 tooltip_position,
                                        Face_ID face, f32 line_height, f32 padding, ARGB_Color color)
{
    Vec2_f32 needed_size = { get_string_advance(app, face, string), line_height };
    Rect_f32 draw_rect =
    {
        tooltip_position.x,
        tooltip_position.y,
        tooltip_position.x + needed_size.x + 2*padding,
        tooltip_position.y + needed_size.y + 2*padding,
    };
    Vec2_f32 text_position = draw_rect.p0 + Vec2_f32{ padding, padding };
    F4_DrawTooltipRect(app, draw_rect);
    draw_string(app, face, string, text_position, color);
    return draw_rect;
}

function void Long_Index_DrawTooltip(Application_Links* app, Rect_f32 screen_rect, Vec2_f32* tooltip_offset,
                                     F4_Index_Note* note, i32 index, Range_i64 range, Range_i64 highlight_range)
{
    Vec2_f32 tooltip_pos =
    {
        global_cursor_rect.x0,
        global_cursor_rect.y1,
    };
    Vec2_f32 offset = tooltip_offset ? *tooltip_offset : Vec2_f32{};
    tooltip_pos += offset;
    offset = tooltip_pos;
    
    Face_ID face = global_small_code_face;
    f32 padding = 4.f;
    f32 line_height = get_face_metrics(app, face).line_height;
    ARGB_Color color = finalize_color(defcolor_text_default, 0);
    ARGB_Color highlight_color = finalize_color(fleury_color_token_highlight, 0);
    Range_f32 screen_x = { screen_rect.x0 + padding * 4, screen_rect.x1 - padding * 4 };
    
    Scratch_Block scratch(app);
    
    if (note->kind == F4_Index_NoteKind_Decl && index)
    {
        note = Long_Index_LookupRef(app, note, 0);
        if (!note)
        return;
    }
    
    if (note->kind == F4_Index_NoteKind_Type && index)
    {
        i32 opt = long_active_pos_context_option;
        NoteFilter* filter = long_global_context_opts[opt];
        
        f32 start_x = tooltip_pos.x;
        Rect_f32 rects[3] = {};
        i32 counts[3] = {};
        
        for (opt = 0; opt < ArrayCount(long_global_context_opts); ++opt)
        {
            ARGB_Color option_colors[] =
            {
                finalize_color(fleury_color_index_decl, 0),
                finalize_color(fleury_color_index_function, 0),
                finalize_color(fleury_color_index_product_type, 0),
            };
            ARGB_Color color = option_colors[opt];
            
            String8 option_names[] =
            {
                S8Lit("DECLS"),
                S8Lit("FUNCS"),
                S8Lit("TYPES"),
            };
            String8 string = option_names[opt];
            
            i32 member_count = 0;
            for (F4_Index_Note* child = note->first_child; child; child = child->next_sibling)
            if (long_global_context_opts[opt](child))
            member_count++;
            counts[opt] = member_count;
            string = push_stringf(scratch, "[%.*s(%d)]", string_expand(string), member_count);
            
            Rect_f32 rect = Long_Index_DrawString(app, string, tooltip_pos, face, line_height, padding * 1.5f, color);
            rects[opt] = rect;
            if (opt < ArrayCount(long_global_context_opts) - 1)
            {
                f32 width = rect_width(rect);
                tooltip_pos.x += width;
                // NOTE(long): This width is of the current option, not the next option
                // So if the next option is _way_ bigger than then current one, this code will not newline it, but it's not a problem currently
                f32 onscreen_percent = .75f;
                if (tooltip_pos.x + width * onscreen_percent > screen_rect.x1)
                {
                    tooltip_pos.x  = start_x;
                    tooltip_pos.y += rect_height(rect);
                }
            }
            else tooltip_pos.y += rect_height(rect);
        }
        
        {
            i32 highlight = long_active_pos_context_option;
            b32 empty = false;
            while (!counts[highlight])
            {
                highlight = (highlight + 1) % ArrayCount(long_global_context_opts);
                if (highlight == long_active_pos_context_option)
                {
                    empty = true;
                    break;
                }
            }
            filter = long_global_context_opts[highlight];
            
            if (!empty)
            {
                // NOTE(long): Because the header for each options use a much bigger padding, I like to increase the highlight's y slightly
                Vec2_f32 end_pos = rects[highlight].p1 - Vec2_f32 { padding, padding * 1.5f };
                Rect_f32 rect = Rf32(rects[highlight].x0 + padding, end_pos.y, end_pos.x, end_pos.y + 2);
                draw_rectangle(app, rect, 1, highlight_color);
                long_active_pos_context_option = highlight;
            }
        }
        
        tooltip_pos.x = start_x;
        
        i32 i = 0;
        for (F4_Index_Note* child = note->first_child; child; child = child->next_sibling)
        {
            if (i == long_global_child_tooltip_count)
            {
                tooltip_pos.y += rect_height(Long_Index_DrawString(app, S8Lit("..."), tooltip_pos, face, line_height, padding * .5f, color));
                break;
            }
            
            if (child->kind == F4_Index_NoteKind_Scope || Long_Index_IsComment(child))
            continue;
            
            if (!filter(child))
            continue;
            
            Range_i64_Array ranges = Long_Index_GetNoteRanges(app, scratch, child, range);
            Rect_f32 rect;
            if (Long_Index_IsNamespace(child))
            rect = Long_Index_DrawString(app, push_stringf(scratch, "namespace %.*s", string_expand(child->string)),
                                         tooltip_pos, face, line_height, padding, color);
            else
                rect = Long_Index_DrawNote(app, ranges, child->file->buffer,
                                           face, line_height, padding, color,
                                           tooltip_pos, screen_x, highlight_color, highlight_range);
            tooltip_pos.y += rect_height(rect);
            ++i;
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
        Rect_f32 rect;
        if (Long_Index_IsNamespace(note))
        rect = Long_Index_DrawString(app, push_stringf(scratch, "namespace %.*s", string_expand(note->string)),
                                     tooltip_pos, face, line_height, padding, color);
        else
            rect = Long_Index_DrawNote(app, ranges, note->file->buffer,
                                       face, line_height, padding, color,
                                       tooltip_pos, screen_x, highlight_color, highlight_range);
        tooltip_pos.y += rect_height(rect);
    }
    
    if (tooltip_offset)
    *tooltip_offset += tooltip_pos - offset;
}

function void Long_Index_DrawPosContext(Application_Links* app, View_ID view, F4_Language_PosContextData* first_ctx)
{
    if (!long_global_pos_context_open) return;
    
    // NOTE(long): In emacs style, has_highlight_range is only true when searching/multi-selecting
    b32 has_highlight_range = *scope_attachment(app, view_get_managed_scope(app, view), view_highlight_buffer, Buffer_ID) != 0;
    if (has_highlight_range) return;
    
    Vec2_f32 offset = {};
    Rect_f32 screen = view_get_screen_rect(app, view);
    for (F4_Language_PosContextData* ctx = first_ctx; ctx; ctx = ctx->next)
    {
        if (ctx->relevant_note)
        {
            Long_Index_DrawTooltip(app, screen, &offset, ctx->relevant_note, ctx->argument_index, ctx->range, ctx->highlight_range);
            offset.x -= 16;
        }
    }
}

#define LONG_INDEX_DRAW_PARTIAL 0

function F4_Index_Note* Long_Index_GetDefNote(F4_Index_Note* note)
{
    F4_Index_Note* parent = note->parent;
    F4_Index_Note* old_note = note;
    while (note->prev) note = note->prev;
    
    for (; note; note = note->next)
    if (note->parent == parent && !(note->flags & F4_Index_NoteFlag_Prototype))
    break;
    
    if (!note) note = old_note;
    return note;
}

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
            note = Long_Index_GetDefNote(note);
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

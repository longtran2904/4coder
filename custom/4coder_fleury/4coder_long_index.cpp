
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

// @COPYPASTA(long): F4_Index_ParsePattern
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

// @COPYPASTA(long): F4_Index_Tick
function void Long_Index_Tick(Application_Links* app)
{
    Scratch_Block scratch(app);
    
    for (Buffer_Modified_Node *node = global_buffer_modified_set.first; node != 0;node = node->next)
    {
        Temp_Memory_Block temp(scratch);
        Buffer_ID buffer_id = node->buffer;
        
        String_Const_u8 contents = push_whole_buffer(app, scratch, buffer_id);
        Token_Array tokens = get_token_array_from_buffer(app, buffer_id);
        if(tokens.count == 0) { continue; }
        
        F4_Index_Lock();
        F4_Index_File *file = F4_Index_LookupOrMakeFile(app, buffer_id);
        if(file)
        {
            ProfileScope(app, "[f] reparse");
            Long_Index_ClearFile(file);
            F4_Index_ParseFile(app, file, contents, tokens);
        }
        F4_Index_Unlock();
        buffer_clear_layout_cache(app, buffer_id);
    }
}

// @COPYPASTA(long): F4_Tick
function void Long_Tick(Application_Links* app, Frame_Info frame_info)
{
    linalloc_clear(&global_frame_arena);
    global_tooltip_count = 0;
    
    View_ID view = get_active_view(app, 0);
    if (view != global_compilation_view)
        long_global_active_view = view;
    
    F4_TickColors(app, frame_info);
    Long_Index_Tick(app);
    F4_CLC_Tick(frame_info);
    F4_PowerMode_Tick(app, frame_info);
    F4_UpdateFlashes(app, frame_info);
    
    // NOTE(rjf): Default tick stuff from the 4th dimension:
    default_tick(app, frame_info);
}

// @COPYPASTA(long): _F4_Index_FreeNoteTree
function void Long_Index_FreeHashTree(F4_Index_Note* note)
{
    Long_Index_IterateValidNoteInFile(child, note)
        Long_Index_FreeHashTree(child);
    
    Assert(!(Long_Index_IsNamespace(note) && note->kind == F4_Index_NoteKind_Type));
    
    F4_Index_Note* prev = note->prev;
    F4_Index_Note* next = note->next;
    F4_Index_Note* hash_prev = note->hash_prev;
    F4_Index_Note* hash_next = note->hash_next;
    
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
    
    // NOTE(long): Consider adding a bool to determine whether or not to push this note to the free list
    note->hash_next = f4_index.free_note;
    f4_index.free_note = note;
    
    // TODO(long): Maybe change the note's name to "__Free_Note__: %old_name%"
    note->string = push_stringf(&note->file->arena, "__Free_Note__: %.*s", string_expand(note->string));
}

function void Long_Index_FreeParentTree(F4_Index_Note* note)
{
    Long_Index_IterateValidNoteInFile(child, note)
        Long_Index_FreeParentTree(child);
    
    Assert(!(Long_Index_IsNamespace(note) && note->kind == F4_Index_NoteKind_Type));
    
    F4_Index_Note* prev = note->prev_sibling;
    F4_Index_Note* next = note->next_sibling;
    if (prev) prev->next_sibling = next;
    if (next) next->prev_sibling = prev;
    
    F4_Index_Note* parent = note->parent;
    if (parent && Long_Index_IsNamespace(parent))
    {
        if (!prev) parent->first_child = next;
        if (!next) parent-> last_child = prev;
    }
}

function void Long_Index_EraseNamespace(F4_Index_Note* note)
{
    if (!Long_Index_IsNamespace(note))
        return;
    
    for (F4_Index_Note* child = note->first_child; child; child = child->next_sibling)
        Long_Index_EraseNamespace(child);
    
    if (!note->first_child)
    {
        // @COPYPASTA(long): Long_Index_FreeParentTree
        {
            F4_Index_Note* prev = note->prev_sibling;
            F4_Index_Note* next = note->next_sibling;
            if (prev) prev->next_sibling = next;
            if (next) next->prev_sibling = prev;
            
            if (!prev) note->parent->first_child = next;
            if (!next) note->parent-> last_child = prev;
        }
        
        // @COPYPASTA(long): Long_Index_FreeHashTree
        {
            F4_Index_Note* prev = note->prev;
            F4_Index_Note* next = note->next;
            F4_Index_Note* hash_prev = note->hash_prev;
            F4_Index_Note* hash_next = note->hash_next;
            
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
            
            note->hash_next = f4_index.free_note;
            f4_index.free_note = note;
            
            // NOTE(long): This will make sure that later iteration won't erase this note again because IsNamespace will fail
            note->flags = 0;
            note->string = push_stringf(&f4_index.arena, "__Free_Note__: %.*s", string_expand(note->string));
        }
    }
}

function void Long_Index_ClearFile(F4_Index_File* file)
{
    if (file)
    {
        file->generation++;
        
        for (F4_Index_Note *note = file->first_note; note; note = note->next_sibling)
            Long_Index_FreeHashTree(note);
        
        for (F4_Index_Note* note = file->first_note; note; note = note->next_sibling)
            Long_Index_FreeParentTree(note);
        
        // NOTE(long): This will clear out all the empty namespaces, which means syntax highlight
        // won't work properly on them. If the namespace only exists inside a single file, then
        // this wouldn't be a problem. Because ClearFile will only be called right before ParseFile,
        // which would parse it again right after. The problem arises when the namespace exists in
        // multiple files, and you delete it in one of the files. In which case, ParseFile won't
        // see it in the modified file, and because other files aren't modified ParseFile wouldn't
        // parse those either.
        // I need this code because I need to remove deleted namespaces somehow, but there isn't
        // any way to know if a file contains a namespace or not, and the namespace also doesn't
        // store any file information. Deleting empty namespaces is the closest solution that I
        // can think of.
        // TODO(long): In the future, consider adding a list of contained namespaces to F4_Index_File.
        // Each file already contains a list of referenced namespaces. 
        for (F4_Index_Note* note = file->first_note; note; note = note->next_sibling)
            if (Long_Index_IsNamespace(note) && note->first_child)
                Long_Index_EraseNamespace(note->first_child->parent);
        
        for (u64 i = 0; i < ArrayCount(f4_index.note_table); ++i)
            for (F4_Index_Note* hash_note = f4_index.note_table[i]; hash_note; hash_note = hash_note->hash_next)
                for (F4_Index_Note* note = hash_note; note; note = note->next)
                    if (note->file)
                        Assert(note->file->generation == note->file_generation);
        
        linalloc_clear(&file->arena);
        file->first_note = file->last_note = 0;
        file->references = 0;
    }
}

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
    current_namespace->scope_range = {};
    
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

// NOTE(long): This function will not search for generic arguments
function F4_Index_Note* Long_Index_LookupChild(F4_Index_Note* parent, i32 index)
{
    F4_Index_Note* result = 0;
    i32 i = 0;
    for (F4_Index_Note* child = parent ? parent->first_child : 0; child; child = child->next_sibling)
    {
        if (Long_Index_IsGenericArgument(child))
            continue;
        
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
    return Long_Index_LoadRef(app, note, filter_note);
}

function void Long_Index_ParseSelection(Application_Links* app, Arena* arena, Token_Iterator_Array* it, Buffer_ID buffer,
                                        String8List* list, b32 continue_after_comma)
{
    Token* current = token_it_read(it);
    if (current && (current->kind == TokenBaseKind_Whitespace || current->kind == TokenBaseKind_Comment))
        token_it_dec(it);
    
    current = token_it_read(it);
    if (current && current->kind == TokenBaseKind_ScopeClose)
        return;
    
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

function F4_Index_Note* Long_Index_LookupSurroundingNote(F4_Index_File* file, i64 pos)
{
    F4_Index_Note* result = 0;
    for (F4_Index_Note* note = file ? file->first_note : 0; note; note = note->next_sibling)
        if (result = Long_Index_LookupScope(note, pos))
            break;
    return result;
}

enum Long_Index_LookupType
{
    Long_LookupType_None,
    Long_LookupType_PreferFunc,
    Long_LookupType_PreferConstructor,
    Long_LookupType_PreferBase,
    Long_LookupType_PreferMember,
};

function F4_Index_Note* Long_Index_LookupNoteByKind(F4_Index_Note* parent, String8 string, F4_Index_NoteKind kind, b32 equal = 1)
{
    F4_Index_Note* note = 0;
    for (F4_Index_Note* head = Long_Index_LookupChild(string, parent); head; note = head, head = head->prev);
    
    for (; note; note = note->next)
        if (note->parent == parent && (note->kind == kind) == equal)
            return note;
    return 0;
}

function F4_Index_Note* Long_Index_LookupNoteFromList(Application_Links* app, String8List names, i64 pos,
                                                      F4_Index_File* file, F4_Index_Note* surrounding_note, F4_Index_Note* filter_note,
                                                      Long_Index_LookupType lookup_type)
{
    Long_Index_ProfileScope(app, "[Long] Lookup List");
    for (F4_Index_Note* note = names.first ? Long_Index_LookupNote(names.first->string) : 0; note; note = note->next)
        if (note->file == file && note->range.min == pos)
            return note;
    
    F4_Index_Note* result = 0;
    if (names.first)
    {
        F4_Index_Note* note = 0;
        if (lookup_type == Long_LookupType_PreferMember)
            note = Long_Index_LookupChild(names.first->string, surrounding_note);
        else
            note = Long_Index_LookupNoteTree(app, file, surrounding_note, names.first->string);
        
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
        
        if (note)
        {
            // TODO(long): This is stupid and incorrect! Plz fix this!
            if (names.last != names.first)
            {
                F4_Index_Note* non_func = Long_Index_LookupNoteByKind(note->parent, note->string, F4_Index_NoteKind_Function, 0);
                if (non_func)
                    note = non_func;
            }
            
            // NOTE(long): Choose parent types over the child functions when they have the same name
            // e.g constructors, conversion operators, etc
            if (note->parent)
                if (note->kind == F4_Index_NoteKind_Function && note->parent->kind == F4_Index_NoteKind_Type)
                    if (string_match(note->string, note->parent->string))
                        note = note->parent;
        }
        
        if (note)
        {
            F4_Index_Note* child = note;
            
            for (String8Node* name = names.first->next; name; name = name->next)
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
                    if (parent)
                        child = Long_Index_LookupChild(name->string, parent);
                }
                
                if (!child)
                    break;
            }
            
            note = child;
        }
        
        if (note)
        {
            F4_Index_Note* new_note = 0;
            
            switch (lookup_type)
            {
                case Long_LookupType_PreferFunc:
                {
                    if (note->kind == F4_Index_NoteKind_Type || note->kind == F4_Index_NoteKind_Decl)
                        new_note = Long_Index_LookupNoteByKind(note->parent, note->string, F4_Index_NoteKind_Function);
                } break;
                
                case Long_LookupType_PreferConstructor:
                {
                    if (note->kind != F4_Index_NoteKind_Type)
                    {
                        F4_Index_Note* type = Long_Index_LookupNoteByKind(note->parent, note->string, F4_Index_NoteKind_Type);
                        if (type) note = type;
                        else break;
                    }
                    
                    for (F4_Index_Note* constructor = note->first_child; constructor; constructor = constructor->next_sibling)
                    {
                        if (string_match(constructor->string, note->string) && !constructor->base_range.max)
                        {
                            note = constructor;
                            break;
                        }
                    }
                } break;
                
                default:
                {
                    if (note->kind == F4_Index_NoteKind_Function)
                        new_note = Long_Index_LookupNoteByKind(note->parent, note->string, F4_Index_NoteKind_Function, 0);
                } break;
            }
            
            if (new_note)
                note = new_note;
        }
        
        result = note;
    }
    else if (lookup_type == Long_LookupType_PreferBase) result = surrounding_note;
    
    return result;
}

function void Long_Index_SkipSelection(Application_Links* app, Token_Iterator_Array* it, Buffer_ID buffer, b32 dec = 0)
{
    Scratch_Block scratch(app);
    if (dec)
    {
        REPEAT_DEC:
        if (token_it_dec(it) && string_match(S8Lit("."), push_token_lexeme(app, scratch, buffer, it->ptr)))
            if (token_it_dec(it) && it->ptr->kind == TokenBaseKind_Identifier)
                goto REPEAT_DEC;
    }
    else
    {
        REPEAT_INC:
        if (token_it_inc(it) && string_match(push_token_lexeme(app, scratch, buffer, it->ptr), S8Lit(".")))
            if (token_it_inc(it) && it->ptr->kind == TokenBaseKind_Identifier)
                goto REPEAT_INC;
    }
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
    
    b32 has_paren = 0;
    Token_Iterator_Array saved = it;
    if (token_it_inc(&it))
    {
        String8 lexeme = push_token_lexeme(app, scratch, buffer, it.ptr);
        if (string_match(lexeme, S8Lit("<")))
        {
            Long_Index_SkipBody(app, &it, buffer, 0, 1);
            lexeme = push_token_lexeme(app, scratch, buffer, it.ptr);
        }
        has_paren = string_match(lexeme, S8Lit("(")); 
    }
    it = saved;
    
    Long_Index_ParseSelection(app, scratch, &it, buffer, &names, use_first);
    
    Buffer_Cursor debug_cursor = buffer_compute_cursor(app, buffer, seek_pos(range.min));
    String8 debug_filename = push_buffer_base_name(app, scratch, buffer);
    
    F4_Index_File* file = F4_Index_LookupFile(app, buffer);
    F4_Index_Note* scope_note = Long_Index_LookupSurroundingNote(file, range.min);
    
    // TODO(long): C# and C++ specific crap - Abstract this out for other languages
    Long_Index_LookupType lookup_type = Long_LookupType_None;
    {
        String8 start_string = push_token_lexeme(app, scratch, buffer, it.ptr);
        if (string_match(start_string, S8Lit("this")))
            lookup_type = Long_LookupType_PreferBase;
        else
        {
            b32 has_new = string_match(start_string, S8Lit("new"));
            if (has_paren)
                lookup_type = has_new ? Long_LookupType_PreferConstructor : Long_LookupType_PreferFunc;
        }
        
        if (lookup_type == Long_LookupType_None)
        {
            // NOTE(long): This is for Object Initializers in C#. This can be slightly modified to handle Designated Initializers in C/C++
            if (scope_note)
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
                        Long_Index_SkipSelection(app, &it, buffer, 1);
                        if (string_match(push_token_lexeme(app, scratch, buffer, it.ptr), S8Lit("new")))
                        {
                            F4_Index_Note* type_scope = Long_Index_LookupBestNote(app, buffer, array, type_token, filter_note);
                            if (type_scope)
                            {
                                scope_note = type_scope;
                                lookup_type = Long_LookupType_PreferMember;
                            }
                        }
                    }
                }
            }
        }
        
        // NOTE(long): This is for `this.myField`
        else if (lookup_type == Long_LookupType_PreferBase)
        {
            while (scope_note && scope_note->kind != F4_Index_NoteKind_Type)
                scope_note = scope_note->parent;
        }
    }
    
    F4_Index_Note* result = Long_Index_LookupNoteFromList(app, names, range.min, file, scope_note, filter_note, lookup_type);
    if (!result && names.last)
    {
        names.last->string = push_stringf(scratch, "%.*s%s", string_expand(names.last->string), "Attribute");
        result = Long_Index_LookupNoteFromList(app, names, range.min, file, scope_note, filter_note, lookup_type);
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
function LONG_INDEX_FILTER(Long_Filter_Type) { return note->kind == F4_Index_NoteKind_Type     && !Long_Index_IsGenericArgument(note); }

global NoteFilter* long_global_context_opts[] =
{
    Long_Filter_Decl,
    Long_Filter_Func,
    Long_Filter_Type,
};

function Vec2_f32 Long_Index_DrawTooltip(Application_Links* app, Rect_f32 screen_rect, Vec2_f32 tooltip_pos,
                                         Face_ID face, f32 padding, f32 line_height, ARGB_Color color, ARGB_Color highlight_color,
                                         F4_Index_Note* note, i32 index, Range_i64 range, Range_i64 highlight_range)
{
    Scratch_Block scratch(app);
    
    if (note->kind == F4_Index_NoteKind_Decl && index)
    {
        note = Long_Index_LookupRef(app, note, 0);
        if (!note)
            return tooltip_pos;
    }
    
    if (note->kind == F4_Index_NoteKind_Type && index)
    {
        i32 opt = long_active_pos_context_option;
        NoteFilter* filter = long_global_context_opts[opt];
        
        f32 start_x = tooltip_pos.x;
        Rect_f32 rects[3] = {};
        i32 counts[3] = {};
        i32 option_count = ArrayCount(long_global_context_opts);
        
        for (opt = 0; opt < option_count; ++opt)
        {
            i32 member_count = 0;
            for (F4_Index_Note* child = note->first_child; child; child = child->next_sibling)
                if (long_global_context_opts[opt](child))
                    member_count++;
            counts[opt] = member_count;
        }
        
        {
            i32 highlight = long_active_pos_context_option;
            b32 empty = false;
            while (!counts[highlight])
            {
                highlight = (highlight + 1) % option_count;
                if (highlight == long_active_pos_context_option)
                {
                    empty = true;
                    break;
                }
            }
            filter = long_global_context_opts[highlight];
            
            if (!empty)
            {
                long_active_pos_context_option = highlight;
                
                FColor option_colors[] =
                {
                    fcolor_id(fleury_color_index_decl),
                    fcolor_id(fleury_color_index_function),
                    fcolor_id(fleury_color_index_product_type),
                };
                
                char* option_names[] =
                {
                    "DECL",
                    "FUNC",
                    "TYPE",
                };
                
                i32 prev_idx = clamp_loop(highlight - 1, option_count);
                i32 next_idx = clamp_loop(highlight + 1, option_count);
                Fancy_Line list = {};
                
                push_fancy_stringf(scratch, &list, option_colors[prev_idx], "<(%.2d)", counts[prev_idx]);
                list.last->post_margin = .5f;
                
                f32 beg_offset = get_fancy_line_width(app, face, &list);
                push_fancy_stringf(scratch, &list, option_colors[highlight], "[%s(%.2d)]", option_names[highlight], counts[highlight]);
                f32 end_offset = get_fancy_line_width(app, face, &list);
                
                push_fancy_stringf(scratch, &list, option_colors[next_idx], "(%.2d)>", counts[next_idx]);
                list.last->pre_margin = .5f;
                
                f32 title_padding = padding * 1.5f;
                Vec2_f32 padding_vec = Vec2_f32{ title_padding, title_padding };
                Rect_f32 rect = Rf32_xy_wh(tooltip_pos, get_fancy_line_dim(app, face, &list) + padding_vec * 2);
                Vec2_f32 text_pos = tooltip_pos + padding_vec;
                
                F4_DrawTooltipRect(app, rect);
                draw_fancy_line(app, face, fcolor_zero(), &list, text_pos);
                tooltip_pos.y += rect_height(rect);
                
                // NOTE(long): Because the title uses a much bigger padding, I like to increase the highlight's y slightly
                f32 highlight_y = rect.y1 - title_padding;
                Rect_f32 highlight_rect = Rf32_xy_wh(text_pos.x + beg_offset, highlight_y, end_offset - beg_offset, 2);
                highlight_rect.x0 += padding;
                highlight_rect.x1 -= padding;
                draw_rectangle(app, highlight_rect, 1, highlight_color);
            }
        }
        
        tooltip_pos.x = start_x;
        
        i32 i = 0;
        for (F4_Index_Note* child = note->first_child; child; child = child->next_sibling)
        {
            if (i == long_global_child_tooltip_count)
            {
                tooltip_pos.y += rect_height(Long_Render_DrawString(app, S8Lit("..."), tooltip_pos, screen_rect,
                                                                    face, line_height, padding * .5f, color));
                break;
            }
            
            if (child->kind == F4_Index_NoteKind_Scope || Long_Index_IsComment(child))
                continue;
            
            if (!filter(child))
                continue;
            
            Range_i64_Array ranges = Long_Index_GetNoteRanges(app, scratch, child, range);
            Rect_f32 rect;
            if (Long_Index_IsNamespace(child))
                rect = Long_Render_DrawString(app, push_stringf(scratch, "namespace %.*s", string_expand(child->string)),
                                              tooltip_pos, screen_rect, face, line_height, padding, color);
            else
                rect = Long_Index_DrawNote(app, ranges, child->file->buffer,
                                           face, line_height, padding, color,
                                           tooltip_pos, Range_f32{ screen_rect.x0, screen_rect.x1 }, highlight_color, highlight_range);
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
            rect = Long_Render_DrawString(app, push_stringf(scratch, "namespace %.*s", string_expand(note->string)),
                                          tooltip_pos, screen_rect, face, line_height, padding, color);
        else
            rect = Long_Index_DrawNote(app, ranges, note->file->buffer,
                                       face, line_height, padding, color,
                                       tooltip_pos, Range_f32{ screen_rect.x0, screen_rect.x1 }, highlight_color, highlight_range);
        tooltip_pos.y += rect_height(rect);
    }
    
    return tooltip_pos;
}

// NOTE(long): This utility function is to stop displaying the outer function while inside a lambda function
// It'll only check for the nearest scope that crosses multiple lines so one-liners or multi-lines macro's
// code arguments will bypass this
function Range_i64 Long_Index_PosContextRange(Application_Links* app, Buffer_ID buffer, i64 pos)
{
    Range_i64 scope_range = Ii64(pos);
    while (find_surrounding_nest(app, buffer, scope_range.min, FindNest_Scope, &scope_range))
    {
        Range_i64 line_range = get_line_range_from_pos_range(app, buffer, scope_range);
        if (range_size(line_range))
            return scope_range;
    }
    return {};
}

function void Long_Index_DrawPosContext(Application_Links* app, View_ID view, F4_Language_PosContextData* first_ctx)
{
    if (!long_global_pos_context_open) return;
    
    // NOTE(long): In emacs style, has_highlight_range is only true when searching/multi-selecting
    b32 has_highlight_range = *scope_attachment(app, view_get_managed_scope(app, view), view_highlight_buffer, Buffer_ID) != 0;
    if (has_highlight_range) return;
    
    Face_ID face = global_small_code_face;
    f32 padding = 4.f;
    f32 line_height = get_face_metrics(app, face).line_height;
    ARGB_Color color = finalize_color(defcolor_text_default, 0);
    ARGB_Color highlight_color = finalize_color(fleury_color_token_highlight, 0);
    
    Rect_f32 screen = view_get_screen_rect(app, view);
    screen.x0 += padding * 4;
    screen.x1 -= padding * 2;
    
    Vec2_f32 tooltip_pos =
    {
        global_cursor_rect.x0,
        global_cursor_rect.y1,
    };
    
    b32 render_at_cursor = !def_get_config_b32(vars_save_string_lit("f4_poscontext_draw_at_bottom_of_buffer"));
    if (!render_at_cursor)
    {
        f32 height = padding * 2;
        for (F4_Language_PosContextData *ctx = first_ctx; ctx; ctx = ctx->next)
            height += line_height + 2*padding;
        tooltip_pos = V2f32(screen.x0, screen.y1 - height);
    }
    
    for (F4_Language_PosContextData* ctx = first_ctx; ctx; ctx = ctx->next)
    {
        if (ctx->relevant_note)
        {
            tooltip_pos = Long_Index_DrawTooltip(app, screen, tooltip_pos,
                                                 face, padding, line_height, color, highlight_color,
                                                 ctx->relevant_note, ctx->argument_index, ctx->range, ctx->highlight_range);
            if (render_at_cursor)
                tooltip_pos.x -= padding * 4;
        }
    }
}

function F4_Index_Note* Long_Index_GetDefNote(F4_Index_Note* note, b32 any_note = 0)
{
    F4_Index_Note* parent = note->parent;
    F4_Index_Note* old_note = note;
    while (note->prev) note = note->prev;
    // NOTE(long): This is a clever way to do the above thing but it still works when the note pass-in is null
    // But every place that uses this function already has to check whether or not the note is null anyway
    // for (F4_Index_Note* head = note; head; note = head, head = head->prev);
    
    for (; note; note = note->next)
        if (note->parent == parent && note->kind == old_note->kind && note->file && note != old_note)
            if (!(note->flags & F4_Index_NoteFlag_Prototype) || any_note)
                break;
    
    if (!note) note = old_note;
    return note;
}

function void Long_Index_DrawCodePeek(Application_Links* app, View_ID view)
{
    View_ID active_view = get_active_view(app, Access_Always);
    Lister* lister = view_get_lister(app, active_view);
    Rect_f32 screen_rect = view_get_screen_rect(app, view);
    
    F4_Index_Note* note = 0;
    
    if (long_lister_tooltip_peek == Long_Tooltip_NextToItem && lister && lister->highlighted_node)
    {
        Long_Lister_Data* data = (Long_Lister_Data*)lister->highlighted_node->user_data;
        if ((Lister_Node*)data == lister->highlighted_node + 1)
        {
            // @COPYPASTA(long): lister_render
            Face_ID face = get_face_id(app, 0);
            f32 line_height = get_face_metrics(app, face).line_height;
            f32 block_height = lister_get_block_height(line_height);
            f32 lister_margin = 3.f; // draw_background_and_margin
            
            Rect_f32 lister_rect = view_get_screen_rect(app, active_view);
            lister_rect = rect_inner(lister_rect, lister_margin);
            
            f32 y_pos = Long_Lister_GetHeight(lister, line_height, block_height, &lister_rect.y0);
            f32 width = line_height * 12;
            f32 padding = 6.f;
            
            Rect_f32 clip_rect = layout_file_bar_on_top(rect_inner(screen_rect, lister_margin), line_height).max;
            Rect_f32 rect = screen_rect;
            rect.x0 += padding;
            rect.x1 -= padding;
            rect.y0 = y_pos;
            rect.y1 = rect.y0 + width;
            
            if (data->tooltip.size)
            {
                Vec2_f32 tooltip_pos = rect.p0;
                if (rect.x0 < lister_rect.x0)
                    tooltip_pos.x = rect.x1; // NOTE(long): This will get clamped inside the DrawString call
                
                Rect_f32 prev_clip = draw_set_clip(app, clip_rect);
                rect = Long_Render_DrawString(app, data->tooltip, tooltip_pos, rect_inner(screen_rect, padding), face, line_height,
                                              (block_height-line_height)/2.f, finalize_color(defcolor_text_default, 0), 0);
                draw_set_clip(app, prev_clip);
            }
            else
            {
                f32 draw_width = rect_width(rect) * .75f;
                if (rect.x0 > lister_rect.x0)
                    rect.x1 = rect.x0 + draw_width;
                else
                    rect.x0 = rect.x1 - draw_width;
                
                Long_Render_DrawPeek(app, rect, rect_inner(rect, line_height), clip_rect, data->buffer, Ii64(data->pos), 1, 0);
            }
            
            if (lister->set_vertical_focus_to_item)
            {
                if (rect.y1 > lister_rect.y1)
                {
                    f32 scroll_margin = block_height;
                    f32 current_height = lister->item_index * block_height + rect_height(rect);
                    
                    lister->scroll.target.y = current_height + scroll_margin - rect_height(lister_rect);
                    lister->set_vertical_focus_to_item = 0;
                }
                else if (lister->set_vertical_focus_to_item == LONG_LISTER_TOOLTIP_SPECIAL_VALUE)
                    lister->set_vertical_focus_to_item = 0;
            }
        }
    }
    else if (global_code_peek_open)
    {
        i64 pos = view_get_cursor_pos(app, active_view);
        Buffer_ID buffer = view_get_buffer(app, active_view, Access_Always);
        Token* token = get_token_from_pos(app, buffer, pos);
        if (token != 0 && token->size > 0 && token->kind == TokenBaseKind_Identifier)
        {
            Token_Array array = get_token_array_from_buffer(app, buffer);
            note = Long_Index_LookupBestNote(app, buffer, &array, token);
            if (note)
            {
                if (!note->file)
                    note = 0;
                else if (note->flags & F4_Index_NoteFlag_Prototype)
                    note = Long_Index_GetDefNote(note);
                else if (range_contains_inclusive(note->range, pos) && note->file->buffer == buffer)
                    note = Long_Index_GetDefNote(note, 1);
            }
        }
    }
    
    if (note)
    {
        Rect_f32 rect = screen_rect;
        i32 peek_count = 1;
        f32 padding = 5.f;
        {
            f32 peek_height = (f32)((screen_rect.y1 - screen_rect.y0) * (0.3f + 0.4f*(clamp_top(peek_count / 4, 1)))) / peek_count;
            rect.y0 = screen_rect.y1 - peek_height*peek_count;
            rect.y1 = rect.y0 + peek_height;
            rect = rect_inner(rect, padding);
        }
        
        Long_Render_DrawPeek(app, rect, note->file->buffer, note->range, 3);
    }
}

//~ NOTE(long): Indent Functions

// @COPYPASTA(long): generic_parse_statement
function Code_Index_Nest*
Long_Index_ParseGenericStatement(Code_Index_File *index, Generic_Parse_State *state){
    generic_parse_skip_soft_tokens(index, state);
    
    Token *token = token_it_read(&state->it);
    Code_Index_Nest *result = push_array_zero(state->arena, Code_Index_Nest, 1);
    result->kind = CodeIndexNest_Statement;
    result->open = Ii64(token->pos);
    result->close = Ii64(max_i64);
    result->file = index;
    
    state->in_statement = true;
    
    b32 is_keyword = 0;
    if (token->kind == TokenBaseKind_Keyword)
    {
        String8 keywords[] =
        {
            S8Lit("if"), S8Lit("else"), S8Lit("do"),
            S8Lit("for"), S8Lit("foreach"), S8Lit("while"),
            S8Lit("try"), S8Lit("catch"), S8Lit("except"),
        };
        
        Scratch_Block scratch(state->app);
        String8 lexeme = push_token_lexeme(state->app, scratch, index->buffer, token);
        for (u64 i = 0; i < ArrayCount(keywords); ++i)
        {
            if (string_match(lexeme, keywords[i]))
            {
                is_keyword = 1;
                break;
            }
        }
    }
    
    b32 is_metadesk = 0;
    {
        F4_Language* language = F4_LanguageFromBuffer(state->app, index->buffer);
        if (language && language->IndexFile == F4_MD_IndexFile)
            is_metadesk = 1;
    }
    
    b32 is_operator = token->kind == TokenBaseKind_Operator;
    
    i64 start_paren_pos = 0;
    Token_Iterator_Array restored = state->it;
    if (token_it_inc(&state->it))
    {
        Token* next = token_it_read(&state->it);
        if (next->kind == TokenBaseKind_ParentheticalOpen)
            start_paren_pos = next->pos;
        
        if (next->kind == TokenBaseKind_Operator)
            is_operator = 1;
        
        // NOTE(long): I reset the state here rather than calling token_it_dec because the state could have been at a whitespace/comment.
        // token_it_dec will decrease past the previous point, causing an infinite loop.
        // Ex: #define SomeMacro { (
        state->it = restored;
    }
    
    for (b32 first_time = 1; ; first_time = 0){
        generic_parse_skip_soft_tokens(index, state);
        token = token_it_read(&state->it);
        if (token == 0 || state->finished){
            break;
        }
        
        if (state->in_preprocessor){
            if (!HasFlag(token->flags, TokenBaseFlag_PreprocessorBody) ||
                token->kind == TokenBaseKind_Preprocessor){
                result->is_closed = true;
                result->close = Ii64(token->pos);
                break;
            }
        }
        else{
            if (token->kind == TokenBaseKind_Preprocessor){
                result->is_closed = true;
                result->close = Ii64(token->pos);
                break;
            }
        }
        
        if (is_keyword && token->pos == start_paren_pos)
        {
            Code_Index_Nest *nest = generic_parse_paren(index, state);
            nest->parent = result;
            code_index_push_nest(&result->nest_list, nest);
            
            // NOTE(long): After a parenthetical group we consider ourselves immediately transitioning into a statement
            nest = generic_parse_statement(index, state);
            nest->parent = result;
            code_index_push_nest(&result->nest_list, nest);
            Range_i64 close = nest->close;
            
            token = token_it_read(&state->it);
            // NOTE(long): Check if the child statement was ended by a scope open
            if (token->pos != nest->open.max && token->kind == TokenBaseKind_ScopeOpen)
            {
                nest = generic_parse_scope(index, state);
                nest->parent = result;
                code_index_push_nest(&result->nest_list, nest);
                close = Ii64(nest->close.max);
            }
            
            result->is_closed = nest->is_closed;
            result->close = close;
            break;
        }
        
        if (!first_time && token->kind == TokenBaseKind_ParentheticalOpen)
        {
            Code_Index_Nest* nest = generic_parse_paren(index, state);
            nest->parent = result;
            code_index_push_nest(&result->nest_list, nest);
            continue;
        }
        
        if (is_metadesk)
        {
            //if (is_tag || token->kind == TokenBaseKind_LiteralString)
            if (!is_operator)
            {
                result->is_closed = true;
                result->close = Ii64(token);
                generic_parse_inc(state);
                break;
            }
        }
        
        if (state->paren_counter > 0 && token->kind == TokenBaseKind_ParentheticalClose)
        {
            result->is_closed = true;
            result->close = Ii64(token->pos);
            break;
        }
        
        if (token->kind == TokenBaseKind_ScopeOpen  ||
            token->kind == TokenBaseKind_ScopeClose ||
            token->kind == TokenBaseKind_ParentheticalOpen // NOTE(long): This case can only be true on the first iteration
            )
        {
            result->is_closed = true;
            result->close = Ii64(token->pos);
            break;
        }
        
        if (token->kind == TokenBaseKind_StatementClose){
            result->is_closed = true;
            result->close = Ii64(token);
            generic_parse_inc(state);
            break;
        }
        
        generic_parse_inc(state);
    }
    
    result->nest_array = code_index_nest_ptr_array_from_list(state->arena, &result->nest_list);
    state->in_statement = false;
    
    return(result);
}

function void Long_Index_IndentBuffer(Application_Links* app, Buffer_ID buffer, Range_i64 pos,
                                      Indent_Flag flags, i32 tab_width, i32 indent_width)
{
    Scratch_Block scratch(app);
    
    // @COPYPASTA(long): auto_indent_buffer
    if (HasFlag(flags, Indent_FullTokens)){
        i32 safety_counter = 0;
        for (;;){
            Range_i64 expanded = enclose_tokens(app, buffer, pos);
            expanded = enclose_whole_lines(app, buffer, expanded);
            if (expanded == pos){
                break;
            }
            pos = expanded;
            safety_counter += 1;
            if (safety_counter == 20){
                pos = buffer_range(app, buffer);
                break;
            }
        }
    }
    
    Range_i64 lines = get_line_range_from_pos_range(app, buffer, pos);
    i64* indentations = push_array_zero(scratch, i64, lines.max - lines.min + 1);
    i64* shifted_indentations = indentations - lines.first; // NOTE(long): Because lines.first is 1-based
    
    code_index_lock();
    
    Code_Index_File* file = code_index_get_file(buffer);
    if (file)
    {
        for (i64 line = lines.min; line <= lines.max; ++line)
        {
            Indent_Info info = get_indent_info_range(app, buffer, get_line_pos_range(app, buffer, line), tab_width);
            i64 indent = 0, pos = info.first_char_pos;
            for (Code_Index_Nest* nest = code_index_get_nest(file, pos); nest; nest = nest->parent)
            {
                // NOTE(long): These 2 checks can only be true in the first iteration
                {
                    // NOTE(long): This check is for CodeIndexNest_Preprocessor and CodeIndexNest_Statement
                    // because they have `nest->open = Ii64(token->pos)` rather than `nest->open = Ii64(token)`
                    if (nest->open.min == pos)
                        continue;
                    
                    if (nest->close.min == pos && nest->kind != CodeIndexNest_Paren)
                        continue;
                }
                
                if (nest->kind == CodeIndexNest_Paren)
                {
                    i64 paren_line = get_line_number_from_pos(app, buffer, nest->open.min);
                    Assert(paren_line < line);
                    Indent_Info paren_indent = get_indent_info_range(app, buffer, get_line_pos_range(app, buffer, paren_line), tab_width);
                    i64 start_text_size = nest->open.min - paren_indent.first_char_pos;
                    indent += (paren_line >= lines.min ? shifted_indentations[paren_line] : paren_indent.indent_pos) + start_text_size + 1;
                    break;
                }
                
                indent += indent_width;
            }
            shifted_indentations[line] = indent;
        }
        
        // @COPYPASTA(long): make_batch_from_indentations
        Batch_Edit* batch_head = 0;
        Batch_Edit* batch_tail = 0;
        for (i64 line = lines.min; line <= lines.max; ++line)
        {
            i64 line_start_pos = get_line_start_pos(app, buffer, line);
            Indent_Info info = get_indent_info_line_number_and_start(app, buffer, line, line_start_pos, tab_width);
            
            i64 correct_indentation = shifted_indentations[line];
            if (info.is_blank && HasFlag(flags, Indent_ClearLine))
                correct_indentation = 0;
            
            if (correct_indentation != info.indent_pos)
            {
                u64 str_size = 0;
                u8* str = 0;
                if (HasFlag(flags, Indent_UseTab))
                {
                    i64 tab_count = correct_indentation / tab_width;
                    i64 indent = tab_count * tab_width;
                    i64 space_count = correct_indentation - indent;
                    str_size = tab_count + space_count;
                    str = push_array(scratch, u8, str_size);
                    block_fill_u8(str, tab_count, '\t');
                    block_fill_u8(str + tab_count, space_count, ' ');
                }
                else
                {
                    str_size = correct_indentation;
                    str = push_array(scratch, u8, str_size);
                    block_fill_u8(str, str_size, ' ');
                }
                
                Batch_Edit *batch = push_array(scratch, Batch_Edit, 1);
                sll_queue_push(batch_head, batch_tail, batch);
                batch->edit.text = SCu8(str, str_size);
                batch->edit.range = Ii64(line_start_pos, info.first_char_pos);
            }
        }
        
        buffer_batch_edit(app, buffer, batch_head);
    }
    // NOTE(long): I don't need to actually do anything when the code can't find any Code_Index_File
    // But for completeness, I just call the default auto_indent one, which uses tokens rather than index notes
    else auto_indent_whole_file(app);
    
    code_index_unlock();
}

// TODO(long): Actually indent only the range pass in rather than the whole buffer
function void Long_Index_IndentBuffer(Application_Links* app, Buffer_ID buffer, Range_i64 range, b32 merge_history)
{
    i32 tab_width = clamp_bot((i32)def_get_config_u64(app, vars_save_string_lit("default_tab_width")), 1);
    i32 indent_width = (i32)def_get_config_u64(app, vars_save_string_lit("indent_width"));
    
    Indent_Flag flags = 0;
    //AddFlag(flags, Indent_ClearLine);
    AddFlag(flags, Indent_FullTokens);
    if (def_get_config_b32(vars_save_string_lit("indent_with_tabs")))
        AddFlag(flags, Indent_UseTab);
    
    History_Record_Index first = buffer_history_get_current_state_index(app, buffer);
    Long_Index_IndentBuffer(app, buffer, range, flags, tab_width, indent_width);
    if (merge_history)
    {
        History_Record_Index last = buffer_history_get_current_state_index(app, buffer);
        if (first < last)
            buffer_history_merge_record_range(app, buffer, first, last, RecordMergeFlag_StateInRange_MoveStateForward);
    }
}


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

function b32 Long_Index_IsMatch(F4_Index_ParseCtx* ctx, Range_i64 range, String8 str)
{
    String8 lexeme = F4_Index_StringFromRange(ctx, range);
    b32 result = string_match(lexeme, str);
    return result;
}

function b32 Long_Index_IsMatch(String8 string, String8* array, u64 count)
{
    for (u64 i = 0; i < count; ++i)
        if (string_match(string, array[i]))
            return 1;
    return 0;
}

//~ NOTE(long): Parsing Functions

function b32 Long_ParseCtx_Inc(F4_Index_ParseCtx* ctx)
{
    b32 result = 0;
    
    repeat:
    if (token_it_inc_all(&ctx->it))
    {
        Token* token = ctx->it.ptr;
        if (token->kind == TokenBaseKind_Comment)
        {
            F4_Index_Note* last_note = ctx->active_parent;
            
            if (!last_note)
                last_note = ctx->file->last_note;
            else if (last_note->last_child)
                last_note = last_note->last_child;
            
            if (!range_contains_inclusive(Ii64(token), last_note ? last_note->range.min : 0))
                F4_Index_ParseComment(ctx, token);
        }
        
        if (token->kind == TokenBaseKind_Comment || token->kind == TokenBaseKind_Whitespace)
            goto repeat;
        result = 1;
    }
    
    ctx->done = !result;
    return ctx->done;
}

function b32 Long_Index_SkipExpression(F4_Index_ParseCtx* ctx, i16 seperator, i16 terminator)
{
    i32 paren_nest = 0;
    i32 scope_nest = 0;
    b32 result = 0;
    
    while (!ctx->done)
    {
        Token* token = token_it_read(&ctx->it);
        b32 done = 0;
        
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
        
        done |= paren_nest < 0 || scope_nest < 0;
        if (done)
            break;
        Long_ParseCtx_Inc(ctx);
    }
    
    return result;
}

function b32 Long_Index_ParseAngleBody(Application_Links* app, Buffer_ID buffer, Token* token, i32* out_nest)
{
    Scratch_Block scratch(app);
    String8 lexeme = push_token_lexeme(app, scratch, buffer, token);
    i32 angle_nest = 0;
    
    for (u64 i = 0; i < lexeme.size; ++i)
    {
        switch(lexeme.str[i])
        {
            case '<' : angle_nest++; break;
            case '>' : angle_nest--; break;
            default: return 0;
        }
    }
    
    if (out_nest)
        *out_nest += angle_nest;
    return 1;
}

function b32 Long_Index_SkipBody(F4_Index_ParseCtx* ctx, b32 dec, b32 exit_early)
{
    return Long_Index_SkipBody(ctx->app, &ctx->it, ctx->file->buffer, dec, exit_early);
}

function b32 Long_Index_SkipBody(Application_Links* app, Token_Iterator_Array* it, Buffer_ID buffer, b32 dec, b32 exit_early)
{
    i32 paren_nest = 0;
    i32 scope_nest = 0;
    i32 angle_nest = 0;
    i32 final_nest = 0;
    
    b32 (*next_token)(Token_Iterator_Array* it) = token_it_inc;
    if (dec) next_token = token_it_dec;
    
    while (1)
    {
        Token* token = token_it_read(it);
        b32 nest_found = 0;
        switch (token->kind)
        {
            case TokenBaseKind_ScopeOpen:          { scope_nest++; nest_found = 1; final_nest = +1; } break;
            case TokenBaseKind_ScopeClose:         { scope_nest--; nest_found = 1; final_nest = -1; } break;
            case TokenBaseKind_ParentheticalOpen:  { paren_nest++; nest_found = 1; final_nest = +1; } break;
            case TokenBaseKind_ParentheticalClose: { paren_nest--; nest_found = 1; final_nest = -1; } break;
            
            case TokenBaseKind_Operator:
            {
                Scratch_Block scratch(app);
                String8 lexeme = push_token_lexeme(app, scratch, buffer, token);
                
                for (u64 i = 0; i < lexeme.size; ++i)
                {
                    switch(lexeme.str[i])
                    {
                        case '<' : angle_nest++; nest_found = 1; final_nest = +1; break;
                        case '>' : angle_nest--; nest_found = 1; final_nest = -1; break;
                        default: goto DONE;
                    }
                }
                
                DONE:;
            } break;
        }
        
        b32 done = paren_nest == 0 && scope_nest == 0 && angle_nest == 0;
        if (dec)
            done |= (paren_nest > 0 || scope_nest > 0 || angle_nest > 0);
        else
            done |= (paren_nest < 0 || scope_nest < 0 || angle_nest < 0);
        
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
    
    return paren_nest == 0 && scope_nest == 0 && angle_nest == 0 && (dec ? (final_nest >= 0) : (final_nest <= 0));
}

function b32 Long_Index_PeekStr(F4_Index_ParseCtx* ctx, String8 str)
{
    b32 result = string_match(str, F4_Index_StringFromToken(ctx, ctx->it.ptr));
    return result;
}

function b32 Long_Index_ParseStr(F4_Index_ParseCtx* ctx, String8 str)
{
    b32 result = Long_Index_PeekStr(ctx, str);
    if (result)
        Long_ParseCtx_Inc(ctx);
    return result;
}

function b32 Long_Index_PeekKind(F4_Index_ParseCtx* ctx, Token_Base_Kind kind, Range_i64* out_range)
{
    b32 result = ctx->it.ptr->kind == kind;
    if (result && out_range)
        *out_range = Ii64(ctx->it.ptr);
    return result;
}

function b32 Long_Index_ParseKind(F4_Index_ParseCtx* ctx, Token_Base_Kind kind, Range_i64* out_range)
{
    b32 result = Long_Index_PeekKind(ctx, kind, out_range);
    if (result)
        Long_ParseCtx_Inc(ctx);
    return result;
}

function b32 Long_Index_PeekSubKind(F4_Index_ParseCtx* ctx, i64 sub_kind, Range_i64* out_range)
{
    b32 result = ctx->it.ptr->sub_kind == sub_kind;
    if (result && out_range)
        *out_range = Ii64(ctx->it.ptr);
    return result;
}

function b32 Long_Index_ParseSubKind(F4_Index_ParseCtx* ctx, i64 sub_kind, Range_i64* out_range)
{
    b32 result = Long_Index_PeekSubKind(ctx, sub_kind, out_range);
    if (result)
        Long_ParseCtx_Inc(ctx);
    return result;
}

function void Long_Index_SkipPreproc(F4_Index_ParseCtx* ctx)
{
    while (!ctx->done)
    {
        Token* token = token_it_read(&ctx->it);
        if (!(token->flags & TokenBaseFlag_PreprocessorBody) || token->kind == TokenBaseKind_Preprocessor)
            break;
        Long_ParseCtx_Inc(ctx);
    }
}

// @COPYPASTA(long): F4_Index_ParsePattern
function b32 _Long_Index_ParsePattern(F4_Index_ParseCtx* ctx, char* fmt, va_list _args)
{
    Long_Index_ProfileScope(ctx->app, "[Long] Parse Pattern");
    F4_Index_ParseCtx ctx_restore = *ctx;
    b32 parsed = 1;
    
    va_list args;
    va_copy(args, _args);
    for (i32 i = 0; fmt[i] && parsed; ++i)
    {
        if (fmt[i] == '%')
        {
            switch (fmt[i+1])
            {
                case 't':
                {
                    char* cstring = va_arg(args, char*);
                    String8 string = SCu8((u8*)cstring, cstring_length(cstring));
                    parsed = parsed && Long_Index_ParseStr(ctx, string);
                } break;
                
                case 'k':
                {
                    Token_Base_Kind kind = (Token_Base_Kind)va_arg(args, i32);
                    Range_i64* output_range = va_arg(args, Range_i64*);
                    parsed = parsed && Long_Index_ParseKind(ctx, kind, output_range);
                } break;
                
                case 'b':
                {
                    i16 kind = (i16)va_arg(args, i32);
                    Range_i64* output_range = va_arg(args, Range_i64*);
                    parsed = parsed && Long_Index_ParseSubKind(ctx, kind, output_range);
                } break;
                
                case 'o': F4_Index_SkipOpTokens(ctx); break;
                default: break;
            }
            i++;
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
    
    for (Buffer_Modified_Node* node = global_buffer_modified_set.first; node != 0;node = node->next)
    {
        Temp_Memory_Block temp(scratch);
        Buffer_ID buffer_id = node->buffer;
        
        String8 contents = push_whole_buffer(app, scratch, buffer_id);
        Token_Array tokens = get_token_array_from_buffer(app, buffer_id);
        if (!tokens.count)
            continue;
        
        F4_Index_Lock();
        F4_Index_File* file = F4_Index_LookupOrMakeFile(app, buffer_id);
        if (file)
        {
            ProfileScope(app, "[f] reparse");
            Long_Index_ClearFile(file);
            F4_Index_ParseFile(app, file, contents, tokens);
        }
        F4_Index_Unlock();
        buffer_clear_layout_cache(app, buffer_id);
    }
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
        
        for (F4_Index_Note* note = file->first_note; note; note = note->next_sibling)
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
    Long_Parse_Scope(ctx) = {};
    note->flags |= F4_Index_NoteFlag_Namespace;
    return note;
}

function F4_Index_Note* Long_Index_PushNamespaceScope(F4_Index_ParseCtx* ctx)
{
    F4_Index_Note* current_namespace = F4_Index_PushParent(ctx, 0);
    F4_Index_Note* namespace_scope = Long_Index_MakeNote(ctx, {}, Ii64(ctx->it.ptr), F4_Index_NoteKind_Scope);
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
                    Token_Iterator_Array _it = token_iterator_pos(0, array, scope_note->range.min);
                    if (token_it_dec(&_it) && _it.ptr->kind == TokenBaseKind_Identifier)
                    {
                        Token* type_token = _it.ptr;
                        Long_Index_SkipSelection(app, &_it, buffer, 1);
                        if (string_match(push_token_lexeme(app, scratch, buffer, _it.ptr), S8Lit("new")))
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
    
    // TODO(long): This totally is a *hack* for C# attributes
    if (!result && names.last)
    {
        names.last->string = push_stringf(scratch, "%.*s%s", string_expand(names.last->string), "Attribute");
        result = Long_Index_LookupNoteFromList(app, names, range.min, file, scope_note, filter_note, lookup_type);
        if (result && result->kind != F4_Index_NoteKind_Type)
            result = 0;
    }
    return result;
}

//~ NOTE(long): Render Functions

function LONG_INDEX_FILTER(Long_Filter_Decl) { return note->kind == F4_Index_NoteKind_Decl || note->kind == F4_Index_NoteKind_Constant; }
function LONG_INDEX_FILTER(Long_Filter_Func) { return note->kind == F4_Index_NoteKind_Function && !Long_Index_IsLambda(note); }
function LONG_INDEX_FILTER(Long_Filter_Type) { return note->kind == F4_Index_NoteKind_Type && !Long_Index_IsGenericArgument(note); }
global NoteFilter* const long_ctx_opts[] = { Long_Filter_Decl, Long_Filter_Func, Long_Filter_Type, };

function Token_List Long_Token_ListFromRange(Application_Links* app, Arena* arena, Token_Array* all_tokens, Range_i64 range)
{
    Token_List result = {};
    if (range.max - range.min > 0)
    {
        Token_Iterator_Array it = token_iterator_pos(0, all_tokens, range.min);
        do token_list_push(arena, &result, it.ptr);
        while (token_it_inc(&it) && it.ptr->pos < range.max);
    }
    return result;
}

function Fancy_Block Long_Index_LayoutNote(Long_Render_Context* ctx, Arena* arena, F4_Index_Note* note,
                                           Range_i64 highlight_range, Range_i64 note_range)
{
    Application_Links* app = ctx->app;
    Face_ID face = ctx->face;
    f32 max_width = rect_width(ctx->rect);
    
    local_const FColor color_table[TokenBaseKind_COUNT] = {
#define X(kind, color) fcolor_id(color),
        Long_Token_ColorTable(X)
#undef X
    };
    
    Fancy_Block result = {};
    Fancy_Line* line = push_fancy_line(arena, &result);
    
    if (Long_Index_IsNamespace(note))
    {
        push_fancy_string(arena, line, color_table[TokenBaseKind_Keyword], S8Lit("namespace "));
        push_fancy_string(arena, line, fcolor_id(fleury_color_index_product_type), note->string);
        return result;
    }
    
    Buffer_ID buffer = /*ctx->buffer*/note->file->buffer;
    Token_Array array_ = /*&ctx->array*/get_token_array_from_buffer(app, buffer);
    Token_Array* array = &array_;
    
    Scratch_Block scratch(app, arena);
    Range_i64 range = Long_Index_ArgumentRange(note);
    if (range.max < note_range.max)
        range.max = note_range.max;
    Token_List list = Long_Token_ListFromRange(app, scratch, array, range);
    
    if (note->base_range.max)
    {
        Token_List base = Long_Token_ListFromRange(app, scratch, array, note->base_range);
        if (base.first)
        {
            if (list.first)
            {
                base.last->next = list.first;
                list.first->prev = base.last;
            }
            
            list.first = base.first;
            list. node_count += base. node_count;
            list.total_count += base.total_count;
        }
    }
    
    for (Token_Block* block = list.first; block; block = block->next)
    {
        Token_Array note_tokens = { block->tokens, block->count, block->max };
        
        for (i64 i = 0; i < note_tokens.count; ++i)
        {
            Token* token = note_tokens.tokens + i;
            String8 lexeme = push_token_lexeme(app, arena, buffer, token);
            
            b32 push_space = 0;
            if (i > 0)
            {
                String8 one_space_ops[] = { S8Lit("*"), S8Lit(">"), };
                String8  no_space_ops[] = { S8Lit("."), S8Lit("::"), S8Lit("<"), };
                
                switch (token->kind)
                {
                    // Operators
                    case TokenBaseKind_Operator:
                    {
                        push_space = (!Long_Index_IsMatch(lexeme, ExpandArray(one_space_ops)) &&
                                      !Long_Index_IsMatch(lexeme, ExpandArray( no_space_ops)) &&
                                      !Long_Index_ParseAngleBody(app, buffer, token, 0));
                    } break;
                    
                    // Atoms
                    case TokenBaseKind_Keyword:
                    case TokenBaseKind_Identifier:
                    case TokenBaseKind_LiteralInteger:
                    case TokenBaseKind_LiteralFloat:
                    case TokenBaseKind_LiteralString:
                    {
                        Token* prev = token - 1;
                        String8 prev_lexeme = push_token_lexeme(app, scratch, buffer, prev);
                        push_space = (!Long_Index_IsMatch(prev_lexeme, ExpandArray(no_space_ops)) &&
                                      prev->kind != TokenBaseKind_ParentheticalOpen);
                    } break;
                }
            }
            else push_space = block != list.first;
            
            b32 overflow = 0;
            if (push_space)
            {
                f32 advance = get_string_advance(app, ctx->face, lexeme);
                f32 width = get_fancy_line_width(app, ctx->face, line);
                Assert(width);
                overflow = (advance + width + ctx->metrics.space_advance) > max_width;
            }
            
            if (overflow)
                line = push_fancy_line(arena, &result);
            else if (push_space)
                push_fancy_string(arena, line, S8Lit(" "));
            
            // TODO(long): Compress this and Long_Syntax_Highlight
            FColor color = color_table[token->kind];
            if (highlight_range.min <= token->pos && token->pos + token->size <= highlight_range.max)
                color = fcolor_id(fleury_color_token_highlight);
            
            else if (token->kind == TokenBaseKind_Identifier)
            {
                F4_Index_Note* note = Long_Index_LookupBestNote(app, buffer, array, token);
                switch (note ? note->kind : 0)
                {
                    case F4_Index_NoteKind_Type:
                    {
                        b32 is_sum_type = note->flags & F4_Index_NoteFlag_SumType;
                        color = fcolor_id(is_sum_type ? fleury_color_index_sum_type : fleury_color_index_product_type);
                    } break;
                    
                    case F4_Index_NoteKind_Macro:    color = fcolor_id(fleury_color_index_macro);    break;
                    case F4_Index_NoteKind_Function: color = fcolor_id(fleury_color_index_function); break;
                    case F4_Index_NoteKind_Constant: color = fcolor_id(fleury_color_index_constant); break;
                    case F4_Index_NoteKind_Decl:     color = fcolor_id(fleury_color_index_decl);     break;
                }
            }
            
            Fancy_String* fancy = push_fancy_string(arena, line, lexeme);
            fancy->fore = color;
        }
    }
    
    return result;
}

function Rect_f32 Long_Index_RenderBlock(Long_Render_Context* ctx, Fancy_Block* block, Vec2_f32 tooltip_pos, Fancy_Line* title)
{
    Application_Links* app = ctx->app;
    Face_ID face = ctx->face;
    ARGB_Color highlight_color = Long_ARGBFromID(fleury_color_token_highlight);
    
    f32 thickness = def_get_config_f32_lit(app, "tooltip_thickness");
    f32 roundness = def_get_config_f32_lit(app, "tooltip_roundness");
    
    Vec2_f32 padding = Long_V2f32(Long_Render_TooltipOffset(ctx, normal_advance/2.f));
    b32 has_title = title && title->first;
    f32 title_padding = padding.x * 1.5f;
    f32 title_offset = 0;
    f32 title_height = 0;
    
    // NOTE(long): Render Background
    Rect_f32 draw_rect;
    {
        Vec2_f32 needed_size = get_fancy_block_dim(app, face, block) + 2 * padding;
        
        if (has_title)
        {
            Vec2_f32 title_size = get_fancy_line_dim(app, face, title);
            title_height = title_size.y;
            
            title_size += Long_V2f32(title_padding * 2);
            needed_size.y += title_size.y + thickness + padding.y;
            
            needed_size.x = clamp_bot(needed_size.x ,title_size.x);
            title_offset = (needed_size.x - title_size.x) / 2.f + title_padding;
        }
        draw_rect = Rf32_xy_wh(tooltip_pos, needed_size);
        
        Rect_f32 region = ctx->rect;
        Vec2_f32 offset = { clamp_bot(draw_rect.x1 - region.x1, 0), clamp_bot(draw_rect.y1 - region.y1, 0) };
        draw_rect.p0 -= offset;
        draw_rect.p1 -= offset;
    }
    Long_Render_TooltipRect(app, draw_rect);
    
    // NOTE(long): Render Title
    Vec2_f32 text_pos = draw_rect.p0 + padding;
    if (has_title)
    {
        Vec2_f32 title_pos = draw_rect.p0 + V2f32(title_offset, title_padding);
        draw_fancy_line(app, face, fcolor_zero(), title, title_pos);
        text_pos.y = title_pos.y + title_height;
        
        Range_f32 highlight_x = rect_range_x(draw_rect);
        f32 highlight_offset = title_offset + get_fancy_string_width(app, face, title->first);
        highlight_x.min += highlight_offset;
        highlight_x.max -= highlight_offset;
        
        // @CONSTANT(long): text_pos.y + thickness
        Rect_f32 highlight_rect = Rf32(highlight_x, If32_size(text_pos.y + thickness, thickness));
        Long_Render_Rect(app, highlight_rect, roundness, highlight_color);
        text_pos.y += title_padding;
        
        Range_f32 div_x = rect_range_x(draw_rect);
        f32 div_offset = /*title_offset*/title_padding;
        div_x.min += div_offset;
        div_x.max -= div_offset;
        
        Rect_f32 div_rect = Rf32(div_x, If32_size(text_pos.y, thickness));
        Long_Render_Rect(app, div_rect, roundness, Long_ARGBFromID(defcolor_margin_active));
        text_pos.y += thickness + padding.y;
    }
    
    // @COPYPASTA(long): draw_fancy_block
    f32 start_x = text_pos.x;
    for (Fancy_Line* line_node = block->first; line_node; line_node = line_node->next)
    {
        Range_f32 highlight_range = {};
        for (Fancy_String* str_node = line_node->first; str_node; str_node = str_node->next)
        {
            FColor color = str_node->fore;
            if (!fcolor_is_valid(color))
                color = fcolor_id(defcolor_text_default);
            
            f32 curr_x = text_pos.x;
            text_pos = draw_fancy_string(app, face, color, str_node, text_pos);
            
            if (color.id == fleury_color_token_highlight)
            {
                if (!highlight_range.max)
                    highlight_range = If32(curr_x, text_pos.x);
                else
                    highlight_range.max = text_pos.x;
            }
        }
        
        text_pos.x = start_x;
        text_pos.y += get_fancy_line_height(app, face, line_node);
        
        if (highlight_range.max)
        {
            Rect_f32 rect = Rf32(highlight_range.min, text_pos.y, highlight_range.max, text_pos.y + thickness);
            Long_Render_Rect(app, rect, roundness, highlight_color);
        }
    }
    
    return draw_rect;
}

function Rect_f32 Long_Index_RenderNote(Long_Render_Context* ctx, F4_Index_Note* note, Vec2_f32 tooltip_pos,
                                        Range_i64 highlight_range, Range_i64 note_range)
{
    Scratch_Block scratch(ctx->app);
    Fancy_Block block = Long_Index_LayoutNote(ctx, scratch, note, highlight_range, note_range);
    Rect_f32 draw_rect = Long_Index_RenderBlock(ctx, &block, tooltip_pos, 0);
    return draw_rect;
}

function Rect_f32 Long_Index_RenderMembers(Long_Render_Context* ctx, F4_Index_Note* note, Vec2_f32 tooltip_pos)
{
    Application_Links* app = ctx->app;
    Scratch_Block scratch(app);
    Fancy_Block block = {};
    Fancy_Line title = {};
    Rect_f32 draw_rect = {};
    
    //- NOTE(long): Init Option
    i32 option = long_active_pos_context_option;
    i32 option_count = ArrayCount(long_ctx_opts);
    {
        i32 counts[3] = {};
        for (i32 opt = 0; opt < option_count; ++opt)
        {
            i32 member_count = 0;
            for (F4_Index_Note* child = note->first_child; child; child = child->next_sibling)
                if (long_ctx_opts[opt](child))
                    member_count++;
            counts[opt] = member_count;
        }
        
        while (!counts[option])
        {
            option = (option + 1) % option_count;
            if (option == long_active_pos_context_option)
                break;
        }
        
        //- NOTE(long): Push Title
        if (counts[option])
        {
            long_active_pos_context_option = option;
            
            const char* option_names[] = { "DECL", "FUNC", "TYPE", };
            FColor option_colors[] =
            {
                fcolor_id(fleury_color_index_decl),
                fcolor_id(fleury_color_index_function),
                fcolor_id(fleury_color_index_product_type),
            };
            
            i32 prev_idx = WrapIndex(option - 1, option_count);
            i32 next_idx = WrapIndex(option + 1, option_count);
            
            push_fancy_stringf(scratch, &title, option_colors[prev_idx],  "<(%.2d) ", counts[prev_idx]);
            push_fancy_stringf(scratch, &title, option_colors[option], "[%s(%.2d)]", option_names[option], counts[option]);
            push_fancy_stringf(scratch, &title, option_colors[next_idx], " (%.2d)>", counts[next_idx]);
        }
        
        else return Long_Index_RenderNote(ctx, note, tooltip_pos, {}, {});
    }
    
    //- NOTE(long): Push Members
    i32 note_count = 0;
    for (F4_Index_Note* child = note->first_child; child; child = child->next_sibling)
    {
        if (note_count == long_global_child_tooltip_count)
        {
            push_fancy_line(scratch, &block, S8Lit("..."));
            break;
        }
        
        if (child->kind == F4_Index_NoteKind_Scope || Long_Index_IsComment(child) || !long_ctx_opts[option](child))
            continue;
        note_count++;
        
        Fancy_Block note_block = Long_Index_LayoutNote(ctx, scratch, child, {}, {});
        if (!block.first)
            block = note_block;
        
        else
        {
            block.last->next = note_block.first;
            block.last = note_block.last;
            block.line_count += note_block.line_count;
        }
    }
    
    draw_rect = Long_Index_RenderBlock(ctx, &block, tooltip_pos, &title);
    return draw_rect;
}

function Vec2_f32 Long_Index_RenderTooltip(Long_Render_Context* ctx, F4_Language_PosContextData* pos_ctx, Vec2_f32 tooltip_pos)
{
    Application_Links* app = ctx->app;
    Scratch_Block scratch(app);
    Rect_f32 region = ctx->rect;
    Face_ID face = ctx->face;
    
    F4_Index_Note* note = pos_ctx->relevant_note;
    i32 index = pos_ctx->argument_index;
    Range_i64 highlight_range = pos_ctx->highlight_range;
    
    if (note->kind == F4_Index_NoteKind_Function && index != -1)
    {
        F4_Index_Note* argument = Long_Index_LookupChild(note, index);
        if (argument && argument->kind == F4_Index_NoteKind_Decl)
            // NOTE(long): The CS parser always parses base_range/scope_range for decls
            // While the CPP parser never parses decls, so I don't need to check these ranges
            highlight_range = { argument->base_range.min, argument->scope_range.min };
    }
    
    if (note->kind == F4_Index_NoteKind_Decl && index)
    {
        note = Long_Index_LookupRef(app, note, 0);
        if (!note)
            goto EXIT;
    }
    
    Rect_f32 draw_rect;
    if (note->kind == F4_Index_NoteKind_Type && index)
        draw_rect = Long_Index_RenderMembers(ctx, note, tooltip_pos);
    else
        draw_rect = Long_Index_RenderNote(ctx, note, tooltip_pos, highlight_range, pos_ctx->range);
    
    tooltip_pos.y = draw_rect.y1;
    EXIT:
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

function void Long_Index_DrawPosContext(Long_Render_Context* ctx, F4_Language_PosContextData* first_pos_ctx)
{
    if (!long_global_pos_context_open)
        return;
    
    Long_Render_Context restore_ctx = *ctx;
    Application_Links* app = ctx->app;
    View_ID view = ctx->view;
    f32 line_height = ctx->metrics.line_height;
    f32 padding = def_get_config_f32_lit(app, "tooltip_padding");
    f32 margin_padding = padding + def_get_config_f32_lit(app, "long_margin_size");
    
    // NOTE(long): In emacs style, has_highlight_range is only true when searching/multi-selecting
    b32 has_highlight_range = Long_Highlight_HasRange(app, view);
    if (has_highlight_range)
        return;
    
    Long_Index_ProfileScope(app, "[Long] Draw Pos Context");
    ARGB_Color color = finalize_color(defcolor_text_default, 0);
    ARGB_Color highlight_color = finalize_color(fleury_color_token_highlight, 0);
    
    f32 thickness = Long_Render_CursorThickness(ctx); // Do this before switching the face
    Long_Render_InitCtxFace(ctx, global_small_code_face);
    Vec2_f32 tooltip_pos;
    Rect_f32 screen;
    
    b32 render_at_cursor = !def_get_config_b32_lit("f4_poscontext_draw_at_bottom_of_buffer");
    if (render_at_cursor)
    {
        screen = global_get_screen_rectangle(app);
        screen.x0 += margin_padding;
        screen.x1 -= margin_padding;
        
        // NOTE(long): The pos-context tooltip is always below the token occurrence underline
        // The underline uses text_height, and line_height >= text_height
        tooltip_pos = global_cursor_rect.p0 + V2f32(padding, line_height + thickness);
    }
    
    else
    {
        screen = view_get_screen_rect(app, view);
        screen.x0 += margin_padding;
        screen.x1 -= margin_padding;
        
        // TODO(long): Handle the case when the line overflows
        f32 height = margin_padding;
        for (F4_Language_PosContextData* pos_ctx = first_pos_ctx; pos_ctx; pos_ctx = pos_ctx->next)
            height += ctx->metrics.line_height+ padding * 2;
        tooltip_pos = V2f32(screen.x0, screen.y1 - height);
    }
    
    ctx->rect = screen;
    for (F4_Language_PosContextData* pos_ctx = first_pos_ctx; pos_ctx; pos_ctx = pos_ctx->next)
    {
        if (pos_ctx->relevant_note)
        {
            f32 old_y = tooltip_pos.y;
            tooltip_pos = Long_Index_RenderTooltip(ctx, pos_ctx, tooltip_pos);
            if (render_at_cursor)
                tooltip_pos.x -= ctx->metrics.line_height/2.f;
        }
    }
    *ctx = restore_ctx;
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

function void Long_Index_DrawCodePeek(Long_Render_Context* ctx)
{
    if (long_global_code_peek_open)
    {
        Application_Links* app = ctx->app;
        Long_Index_ProfileScope(app, "[Long] Draw Code Peek");
        View_ID active_view = get_active_view(app, Access_Always);
        
        i64 pos = view_get_cursor_pos(app, active_view);
        Buffer_ID buffer = view_get_buffer(app, active_view, Access_Always);
        Token* token = get_token_from_pos(app, buffer, pos);
        
        if (token != 0 && token->size > 0 && token->kind == TokenBaseKind_Identifier)
        {
            Token_Array array = get_token_array_from_buffer(app, buffer);
            F4_Index_Note* note = Long_Index_LookupBestNote(app, buffer, &array, token);
            
            if (note)
            {
                if (!note->file)
                    note = 0;
                else if (note->flags & F4_Index_NoteFlag_Prototype)
                    note = Long_Index_GetDefNote(note);
                else if (range_contains_inclusive(note->range, pos) && note->file->buffer == buffer)
                    note = Long_Index_GetDefNote(note, 1);
            }
            
            if (note)
            {
                f32 padding = def_get_config_f32_lit(app, "long_margin_size") * 2;
                f32 peek_height = def_get_config_f32_lit(app, "long_code_peek_height") * .01f;
                
                Rect_f32 tooltip_rect = ctx->rect;
                peek_height *= rect_height(tooltip_rect);
                
                if (peek_height > padding)
                {
                    tooltip_rect.y0 = tooltip_rect.y1 - peek_height;
                    tooltip_rect = rect_inner(tooltip_rect, padding);
                    
                    Long_Render_TooltipRect(app, tooltip_rect);
                    Long_Render_DrawPeek(app, tooltip_rect, note->file->buffer, note->range, 3);
                }
            } 
        }
    }
}

//~ NOTE(long): Indent Functions

// @COPYPASTA(long): generic_parse_statement
function Code_Index_Nest* Long_Index_ParseGenericStatement(Code_Index_File* index, Generic_Parse_State* state)
{
    generic_parse_skip_soft_tokens(index, state);
    Token* token = token_it_read(&state->it);
    
    Code_Index_Nest* result = push_array_zero(state->arena, Code_Index_Nest, 1);
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
    
    b32 is_identifier = token->kind == TokenBaseKind_Identifier;
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
    
    for (b32 first_time = 1; ; first_time = 0)
    {
        generic_parse_skip_soft_tokens(index, state);
        token = token_it_read(&state->it);
        if (token == 0 || state->finished)
            break;
        
        if (state->in_preprocessor)
        {
            if (!HasFlag(token->flags, TokenBaseFlag_PreprocessorBody) || token->kind == TokenBaseKind_Preprocessor)
            {
                result->is_closed = true;
                result->close = Ii64(token->pos);
                break;
            }
        }
        
        else if (token->kind == TokenBaseKind_Preprocessor)
        {
            result->is_closed = true;
            result->close = Ii64(token->pos);
            break;
        }
        
        if ((is_keyword || is_identifier) && token->pos == start_paren_pos)
        {
            Code_Index_Nest* nest = generic_parse_paren(index, state);
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
            // NOTE(long): This case can only be true on the first iteration
            token->kind == TokenBaseKind_ParentheticalOpen)
        {
            result->is_closed = true;
            result->close = Ii64(token->pos);
            break;
        }
        
        if (token->kind == TokenBaseKind_StatementClose)
        {
            result->is_closed = true;
            result->close = Ii64(token);
            generic_parse_inc(state);
            break;
        }
        
        generic_parse_inc(state);
    }
    
    result->nest_array = code_index_nest_ptr_array_from_list(state->arena, &result->nest_list);
    state->in_statement = 0;
    
    return(result);
}

function void Long_Index_IndentBuffer(Application_Links* app, Buffer_ID buffer, Range_i64 pos,
                                      Indent_Flag flags, i32 tab_width, i32 indent_width)
{
    Scratch_Block scratch(app);
    
    // @COPYPASTA(long): auto_indent_buffer
    if (HasFlag(flags, Indent_FullTokens))
    {
        for (i32 safety_counter = 0, max_counter = 20; safety_counter < max_counter; safety_counter++)
        {
            Range_i64 expanded = enclose_tokens(app, buffer, pos);
            expanded = enclose_whole_lines(app, buffer, expanded);
            if (expanded == pos)
                break;
            
            pos = expanded;
            if (safety_counter == max_counter-1)
                pos = buffer_range(app, buffer);
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
                
                Batch_Edit* batch = push_array(scratch, Batch_Edit, 1);
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
    i32 tab_width = clamp_bot((i32)def_get_config_u64_lit(app, "default_tab_width"), 1);
    i32 indent_width = (i32)def_get_config_u64_lit(app, "indent_width");
    
    Indent_Flag flags = 0;
    //AddFlag(flags, Indent_ClearLine);
    AddFlag(flags, Indent_FullTokens);
    if (def_get_config_b32_lit("indent_with_tabs"))
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

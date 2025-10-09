
#define Long_CPP_SkipExpression(ctx) Long_Index_SkipExpression(ctx, TokenCppKind_Comma, TokenCppKind_Semicolon)

function Token* Long_Index_PeekToken(F4_Index_ParseCtx* ctx, i64 pos, i32 dir)
{
    Token_Iterator_Array it = token_iterator_pos(0, &ctx->tokens, pos);
    if (dir == 0) { }
    else if (dir < 0) token_it_dec(&it);
    else if (dir > 0) token_it_inc(&it);
    return token_it_read(&it);
}

function b32 Long_CPP_IsTokenSelection(Token* token)
{
    return (token->sub_kind == TokenCppKind_Dot ||
            token->sub_kind == TokenCppKind_Arrow ||
            token->sub_kind == TokenCppKind_ColonColon);
}

function b32 Long_CPP_ParseBuiltinType(F4_Index_ParseCtx* ctx, Range_i64* range)
{
    local_const String8 cpp_types[] =
    {
        S8Lit("bool"),
        S8Lit("char"),
        S8Lit("wchar_t"),
        S8Lit("char16_t"),
        S8Lit("char32_t"),
        S8Lit("short"),
        S8Lit("int"),
        S8Lit("long"),
        S8Lit("signed"),
        S8Lit("unsigned"),
        S8Lit("float"),
        S8Lit("double"),
        S8Lit("void"),
        S8Lit("auto"),
    };
    
    b32 result = 0;
    for (u64 i = 0; i < ArrayCount(cpp_types); ++i)
    {
        result = string_match(cpp_types[i], F4_Index_StringFromToken(ctx, ctx->it.ptr));
        if (result)
        {
            if (range) *range = Ii64(ctx->it.ptr);
            Long_ParseCtx_Inc(ctx);
            break;
        }
    }
    return result;
}

function b32 Long_CPP_ParseQualifier(F4_Index_ParseCtx* ctx)
{
    local_const String8 cpp_qualifiers[] =
    {
        S8Lit("const"),
        S8Lit("volatile"),
        S8Lit("mutable"),
        S8Lit("restrict"),
        S8Lit("static"),
        S8Lit("extern"),
        S8Lit("thread_local"),
        S8Lit("register"),
        S8Lit("auto"),
    };
    
    b32 result = 0;
    for (u64 i = 0; i < ArrayCount(cpp_qualifiers); ++i)
    {
        result = string_match(cpp_qualifiers[i], F4_Index_StringFromToken(ctx, ctx->it.ptr));
        if (result)
        {
            Long_ParseCtx_Inc(ctx);
            break;
        }
    }
    return result;
}

// TODO(long): This is incomplete. It can't handle complex declarations.
// https://github.com/HappenLee/cdecl/blob/master/main.cpp
function b32 Long_CPP_ParseBase(F4_Index_ParseCtx* ctx, Range_i64* base_range)
{
    F4_Index_ParseCtx restore_ctx = *ctx;
    Range_i64 base = {};
    i64 start_pos = ctx->it.ptr->pos;
    b32 result = 0;
    
    do
    {
        result = Long_Index_ParseKind(ctx, TokenBaseKind_Identifier, &base);
        if (!result)
            while (Long_CPP_ParseBuiltinType(ctx, 0) || Long_CPP_ParseQualifier(ctx))
                result = 1;
        
        if (result && Long_Index_PeekSubKind(ctx, TokenCppKind_Less, 0))
            result = Long_Index_SkipBody(ctx);
        while (result && (Long_Index_ParseSubKind(ctx, TokenCppKind_Star, 0) || Long_Index_ParseSubKind(ctx, TokenCppKind_And, 0)));
        
        if (result)
        {
            if (!Long_CPP_IsTokenSelection(ctx->it.ptr) && !Long_CPP_ParseQualifier(ctx))
            {
                Token* end_token = Long_Index_PeekToken(ctx, ctx->it.ptr->pos, -1);
                base.end = end_token->pos + end_token->size;
                break;
            }
            Long_ParseCtx_Inc(ctx);
        }
    } while (result && !ctx->done);
    
    if (result)
        *base_range = Ii64(start_pos, base.max);
    else
        *ctx = restore_ctx;
    return result;
}

function b32 Long_CPP_ParseName(F4_Index_ParseCtx* ctx, Range_i64* name, b32 name_is_op)
{
    REP:
    b32 result = Long_Index_ParseKind(ctx, TokenBaseKind_Identifier, name);
    
    // TODO(long): This is extremely lazy. Do something better than skipping '::'
    if (result)
    {
        if (Long_Index_ParseSubKind(ctx, TokenCppKind_ColonColon, 0))
            goto REP;
    }
    
    else if (name_is_op)
        result = (Long_Index_ParseKind(ctx, TokenBaseKind_Operator, name) || Long_CPP_ParseBuiltinType(ctx, name));
    return result;
}

function b32 Long_CPP_ParseNameAndArg(F4_Index_ParseCtx* ctx, Range_i64* name_range, b32 name_is_op)
{
    F4_Index_ParseCtx restore_ctx = *ctx;
    Range_i64 name = {};
    
    b32 result = Long_CPP_ParseName(ctx, &name, name_is_op);
    if (result)
        result = Long_Index_ParseSubKind(ctx, TokenCppKind_ParenOp, 0);
    
    if (result)
        *name_range = name;
    else
        *ctx = restore_ctx;
    
    return result;
}

function b32 Long_CPP_ParseDecl(F4_Index_ParseCtx* ctx, Range_i64* base_range, Range_i64* name_range)
{
    F4_Index_ParseCtx restore_ctx = *ctx;
    Range_i64 base = {};
    Range_i64 name = {};
    
    b32 result = Long_CPP_ParseBase(ctx, &base);
    if (result)
        result = Long_CPP_ParseName(ctx, &name, 0);
    
    if (result)
    {
        if (base_range) *base_range = base;
        if (name_range) *name_range = name;
    }
    else *ctx = restore_ctx;
    return result;
}

internal F4_LANGUAGE_INDEXFILE(Long_CPP_IndexFile)
{
    while (!ctx->done)
    {
        Range_i64 name = {};
        Range_i64 base = {};
        b32 initialized = Long_Index_IsParentInitialized(ctx);
        
        //~ TODO(long): Parse '::' for scope resolution and member functions
        
        if (0) {}
        
        //~ NOTE(long): Parent Scope Changes
        else if (ctx->active_parent && Long_Parse_Scope(ctx).max == ctx->it.ptr->pos)
        {
            if (Long_Index_IsNamespace(ctx->active_parent))
                Long_Parse_Scope(ctx) = {};
            Long_Index_PopParent(ctx);
        }
        
        else if (!initialized && Long_Index_PeekSubKind(ctx, TokenCppKind_Semicolon, 0))
            Long_Index_ScopeBlock(ctx);
        
        else if (Long_Index_PeekKind(ctx, TokenBaseKind_ScopeOpen, &name))
        {
            if (initialized)
                Long_Index_MakeNote(ctx, {}, Ii64(ctx->it.ptr), F4_Index_NoteKind_Scope);
            // NOTE(long): namespaces are always uninitialized
            else if (ctx->active_parent && Long_Index_IsNamespace(ctx->active_parent))
                Long_Index_PushNamespaceScope(ctx);
            Long_Index_StartCtxScope(ctx);
            Long_ParseCtx_Inc(ctx);
        }
        
        else if (ctx->active_parent && Long_Index_PeekKind(ctx, TokenBaseKind_ScopeClose, 0))
        {
            // NOTE(long): The last leaf namespace can have scope_range in one file while (base)range in other files
            // So it must be cleared here at the end of its scope
            if (Long_Index_IsNamespace(ctx->active_parent))
                Long_Index_PopNamespaceScope(ctx);
            else
                Long_Index_EndCtxScope(ctx);
            do Long_Index_PopParent(ctx);
            while (!Long_Index_IsParentInitialized(ctx));
            Long_ParseCtx_Inc(ctx);
        }
        
        //~ NOTE(long): Extern "C"
        else if (Long_Index_ParseSubKind(ctx, TokenCppKind_Extern, 0) && Long_Index_ParseStr(ctx, S8Lit("C")))
        {
            // TODO(long): We don't need to do anything here, except not pushing the next scope
        }
        
        //~ NOTE(long): Namespaces
        else if (Long_Index_ParseSubKind(ctx, TokenCppKind_Namespace, &base))
        {
            // NOTE(long): Namespaces can only be global or inside another namespace.
            // A namespace can never be inside a class/type/function/decl/etc.
            // This will make the EraseNamespace code easier to handle.
            if (!ctx->active_parent || Long_Index_IsNamespace(ctx->active_parent))
            {
                while (Long_Index_ParseKind(ctx, TokenBaseKind_Identifier, &name))
                {
                    F4_Index_Note* current_namespace = Long_Index_MakeNamespace(ctx, base, name);
                    if (!Long_Index_ParseSubKind(ctx, TokenCppKind_Dot, 0))
                        break;
                }
            }
        }
        
        //~ TODO(long): Using
        //else if (Long_Index_ParseSubKind(ctx, TokenCppKind_Using, &base))
        //{
        //String8ListNode* node = push_array(&ctx->file->arena, String8ListNode, 1);
        //sll_stack_push(ctx->file->references, node);
        //while (Long_Index_ParseKind(ctx, TokenBaseKind_Identifier, &name))
        //{
        //String8 using_name = push_string_copy(&ctx->file->arena, F4_Index_StringFromRange(ctx, name));
        //string_list_push(&ctx->file->arena, &node->list, using_name);
        //if (Long_Index_ParseSubKind(ctx, TokenCppKind_Dot, 0))
        //break;
        //}
        //}
        
        //~ NOTE(long): Types
        else if (Long_Index_ParseSubKind(ctx, TokenCppKind_Enum, &base) ||
                 Long_Index_ParseSubKind(ctx, TokenCppKind_Class, &base) ||
                 Long_Index_ParseSubKind(ctx, TokenCppKind_Struct, &base))
        {
            F4_Index_Note* note = 0;
            
            if (Long_Index_ParseKind(ctx, TokenBaseKind_Identifier, &name))
            {
                Range_i64 name2 = {};
                if (Long_Index_ParseKind(ctx, TokenBaseKind_Identifier, &name2))
                    note = Long_Index_MakeNote(ctx, Ii64(base.min, name.max), name2, F4_Index_NoteKind_Decl);
                else
                    Long_Index_MakeNote(ctx, base, name, F4_Index_NoteKind_Type);
            }
            
            else if (Long_Index_PeekKind(ctx, TokenBaseKind_ScopeOpen, 0))
            {
                if (Long_Index_SkipBody(ctx, 0, 1))
                {
                    Token* end_token = Long_Index_PeekToken(ctx, ctx->it.ptr->pos, -1);
                    base.max = end_token->pos + end_token->size;
                    if (Long_Index_ParseKind(ctx, TokenBaseKind_Identifier, &name))
                        note = Long_Index_MakeNote(ctx, base, name, F4_Index_NoteKind_Decl);
                }
            }
            
            else if (Long_Index_IsMatch(ctx, base, S8Lit("enum")) && (Long_Index_ParseSubKind(ctx, TokenCppKind_Struct, &name) ||
                                                                      Long_Index_ParseSubKind(ctx, TokenCppKind_Class,  &name)))
            {
                // NOTE(long): `typedef enum class Foo Bar` and `enum class Foo decl` are invalid
                base.max = name.max;
                if (Long_Index_ParseKind(ctx, TokenBaseKind_Identifier, &name))
                    Long_Index_MakeNote(ctx, base, name, F4_Index_NoteKind_Type);
            }
            
            Token* token = Long_Index_PeekToken(ctx, base.min, -1);
            b32 has_typedef = token && token->sub_kind == TokenCppKind_Typedef;
            if (has_typedef && note)
            {
                note->kind = F4_Index_NoteKind_Type;
                note->flags = F4_Index_NoteFlag_Prototype;
            }
        }
        
        else if (Long_Parse_Comp(ctx, kind == F4_Index_NoteKind_Type) &&
                 Long_Index_IsMatch(ctx, Ii64_size(ctx->active_parent->base_range.min, sizeof("enum") - 1), S8Lit("enum")) &&
                 Long_Index_ParseKind(ctx, TokenBaseKind_Identifier, &name))
        {
            if (Long_Index_ParseSubKind(ctx, TokenCppKind_Comma, 0) || Long_Index_PeekKind(ctx, TokenBaseKind_ScopeClose, 0))
                Long_Index_MakeNote(ctx, base, name, F4_Index_NoteKind_Constant, 0);
            
            else if (Long_Index_ParseSubKind(ctx, TokenCppKind_Eq, 0))
            {
                Long_Index_MakeNote(ctx, base, name, F4_Index_NoteKind_Constant, 0);
                Long_CPP_SkipExpression(ctx);
            }
        }
        
        //~ NOTE(long): Constructors
        else if (Long_Parse_Comp(ctx, kind == F4_Index_NoteKind_Type) && Long_CPP_ParseNameAndArg(ctx, &name, 0))
        {
            if (Long_Index_IsMatch(ctx, name, ctx->active_parent->string))
                Long_Index_MakeNote(ctx, base, name, F4_Index_NoteKind_Function);
        }
        
        //~ NOTE(long): Operators
        else if (Long_Index_ParseSubKind(ctx, TokenCppKind_Operator, &base))
        {
            if (Long_CPP_ParseNameAndArg(ctx, &name, 1))
                Long_Index_MakeNote(ctx, base, name, F4_Index_NoteKind_Function);
        }
        
        //~ NOTE(long): Declarations
        else if (Long_CPP_ParseDecl(ctx, &base, &name))
        {
            F4_Index_Note* note;
            if (Long_Index_ParseSubKind(ctx, TokenCppKind_ParenOp, 0))
                note = Long_Index_MakeNote(ctx, base, name, F4_Index_NoteKind_Function);
            
            else
            {
                DECLARATION:
                b32 push_decl = (Long_Index_PeekSubKind(ctx, TokenCppKind_ParenOp, 0) || Long_Index_PeekSubKind(ctx, TokenCppKind_ParenCl, 0) ||
                                 Long_Index_PeekSubKind(ctx, TokenCppKind_Semicolon, 0) || Long_Index_PeekSubKind(ctx, TokenCppKind_Comma, 0) ||
                                 Long_Index_PeekSubKind(ctx, TokenCppKind_Eq, 0));
                
                if (!push_decl && Long_Index_PeekSubKind(ctx, TokenCppKind_BrackOp, 0))
                {
                    push_decl = 1;
                    Long_Index_SkipBody(ctx);
                }
                
                note = 0;
                if (push_decl)
                {
                    note = Long_Index_MakeNote(ctx, base, name, F4_Index_NoteKind_Decl);
                    Long_Index_IterBlock(ctx)
                        Long_Index_ScopeBlock(ctx)
                            if (Long_Index_ParseSubKind(ctx, TokenCppKind_Eq, 0))
                                Long_CPP_SkipExpression(ctx);
                }
                
                // TODO(long): This is a dump hack
                else token_it_dec(&ctx->it);
            }
            
            Token* token = Long_Index_PeekToken(ctx, base.min, -1);
            if (token && token->sub_kind == TokenCppKind_Typedef)
            {
                note->kind = F4_Index_NoteKind_Type;
                note->flags = F4_Index_NoteFlag_Prototype;
            }
        }
        
        else if (Long_Index_ParseKind(ctx, TokenBaseKind_Identifier, &name))
        {
            F4_Index_Note* prev_sibling = ctx->active_parent ? ctx->active_parent->last_child : ctx->file->last_note;
            if (prev_sibling && prev_sibling->kind == F4_Index_NoteKind_Decl && prev_sibling->scope_range.max)
            {
                Token_Iterator_Array it = token_iterator_pos(0, &ctx->tokens, prev_sibling->scope_range.max);
                if (it.ptr->sub_kind == TokenCppKind_Comma && token_it_inc(&it))
                {
                    if (name.min == it.ptr->pos)
                    {
                        base = prev_sibling->base_range;
                        goto DECLARATION;
                    }
                }
            }
        }
        
        //~ NOTE(long): Parse Define Macro
        else if (Long_Index_ParseSubKind(ctx, TokenCppKind_PPDefine, 0))
        {
            Long_Index_ParseKind(ctx, TokenBaseKind_Identifier, &name);
            Long_Index_MakeNote(ctx, base, name, F4_Index_NoteKind_Macro, 0);
            goto SKIP_PP_BODY;
        }
        
        // TODO(long): Simple macro substitution
        
        else if (Long_Index_ParseKind(ctx, TokenBaseKind_Preprocessor, 0))
        {
            SKIP_PP_BODY:
            Long_Index_SkipPreproc(ctx);
        }
        
        else Long_ParseCtx_Inc(ctx);
    }
    
    while (ctx->active_parent)
    {
        if (Long_Index_IsNamespace(ctx->active_parent) && Long_Parse_Scope(ctx).start)
            Long_Index_PopNamespaceScope(ctx);
        Long_Index_PopParent(ctx);
    }
}

//F4_Language_PosContextData * name(Application_Links *app, Arena *arena, Buffer_ID buffer, i64 pos)
internal F4_LANGUAGE_POSCONTEXT(Long_CPP_PosContext)
{
    Token_Array array = get_token_array_from_buffer(app, buffer);
    Token_Iterator_Array it = token_iterator_pos(0, &array, pos);
    Range_i64 scope = Long_Index_PosContextRange(app, buffer, pos);
    
    b32 access_found = 0;
    b32 has_arg = 0;
    i32 paren_nest = 0;
    i32 arg_idx = 0;
    Vec2_f32 offset = {};
    
    F4_Language_PosContextData* first = 0;
    F4_Language_PosContextData* last = 0;
    for (i32 i = 0, tooltip_count = 0; tooltip_count < 4; ++i)
    {
        Token* token = token_it_read(&it);
        if (token->pos < scope.min)
            break;
        
        if (token->kind == TokenBaseKind_ParentheticalClose && i > 0)
            paren_nest--;
        
        else if (token->kind == TokenBaseKind_ParentheticalOpen)
        {
            i32 old_nest = paren_nest;
            paren_nest = clamp_top(paren_nest + 1, 0);
            
            if (old_nest == 0 && token_it_dec(&it))
            {
                Token_Iterator_Array iterator = it;
                Long_Index_SkipBody(app, &it, buffer, 1, 1);
                token = token_it_read(&it);
                
                if (token->kind == TokenBaseKind_Identifier)
                {
                    has_arg = i > 0;
                    goto LOOKUP;
                }
                
                //token_it_inc(&it);
                it = iterator;
            }
        }
        
        else if (paren_nest >= 0)
        {
            if (token->kind == TokenBaseKind_Identifier && tooltip_count == 0)
            {
                b32 field_accessing = (i == 2 && access_found);
                b32 inside_token = range_contains_inclusive(Ii64(token), pos);
                
                if (field_accessing || inside_token)
                {
                    LOOKUP:
                    F4_Index_Note* note = Long_Index_LookupBestNote(app, buffer, &array, token);
                    if (note)
                    {
                        i32 index = access_found;
                        if (note->kind == F4_Index_NoteKind_Function)
                        {
                            index = has_arg ? arg_idx : -1; // NOTE(long): -1 means don't highlight any argument
                            arg_idx = 0;
                        }
                        
                        if (!note->file || note->file->buffer != buffer || !range_contains(Long_Index_ArgumentRange(note), token->pos))
                        {
                            F4_Language_PosContext_PushData(arena, &first, &last, note, 0, index);
                            tooltip_count++;
                        }
                    }
                }
            }
            
            if (!access_found)
                access_found = Long_CPP_IsTokenSelection(token);
            if (token->sub_kind == TokenCppKind_Comma && i > 0)
                arg_idx++;
        }
        
        if (!token_it_dec(&it))
            break;
    }
    
    return first;
}

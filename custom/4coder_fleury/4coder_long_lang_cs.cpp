
//~ NOTE(long): Util Functions

#if LONG_INDEX_INLINE
#define Long_CS_ParseKind(ctx, kind, range)    Long_Index_ParseKind   (ctx, kind, range)
#define Long_CS_ParseSubKind(ctx, kind, range) Long_Index_ParseSubKind(ctx, kind, range)
#define Long_CS_PeekKind(ctx, kind, range)     Long_Index_PeekKind    (ctx, kind, range)
#define Long_CS_PeekSubKind(ctx, kind, range)  Long_Index_PeekSubKind (ctx, kind, range)

#else
#define Long_CS_ParseKind(ctx, kind, range)    Long_Index_ParsePattern(ctx, "%k", kind, range)
#define Long_CS_ParseSubKind(ctx, kind, range) Long_Index_ParsePattern(ctx, "%b", kind, range)
#define Long_CS_PeekKind(ctx, kind, range)      Long_Index_PeekPattern(ctx, "%k", kind, range)
#define Long_CS_PeekSubKind(ctx, kind, range)   Long_Index_PeekPattern(ctx, "%b", kind, range)
#endif

#define Long_CS_SkipExpression(ctx) Long_Index_SkipExpression(ctx, TokenCsKind_Comma, TokenCsKind_Semicolon)

function b32 Long_Index_PeekNextKind(F4_Index_ParseCtx* ctx, Token_Base_Kind kind)
{
    b32 result = 0;
    Token_Iterator_Array it = ctx->it;
    if (token_it_inc(&it))
        result = it.ptr->kind == kind;
    return result;
}

function b32 Long_CS_IsTokenSelection(Token* token)
{
    return (token->sub_kind == TokenCsKind_Dot   ||
            token->sub_kind == TokenCsKind_Arrow ||
            token->sub_kind == TokenCsKind_TernaryDot);
}

//~ NOTE(long): CS Parsing

function void Long_CS_ParseGenericArg(F4_Index_ParseCtx* ctx)
{
    if (Long_CS_ParseSubKind(ctx, TokenCsKind_Less, 0))
    {
        Range_i64 generic;
        while (Long_CS_ParseKind(ctx, TokenBaseKind_Identifier, &generic))
        {
            Long_Index_MakeNote(ctx, {}, generic, F4_Index_NoteKind_Type, 0);
            if (!Long_CS_ParseSubKind(ctx, TokenCsKind_Comma, 0))
                break;
        }
    }
}

function b32 Long_CS_ParseGenericBase(F4_Index_ParseCtx* ctx)
{
    F4_Index_ParseCtx restore_ctx = *ctx;
    b32 result = 0;
    
    if (Long_CS_ParseSubKind(ctx, TokenCsKind_Less, 0))
    {
        i32 paren_nest = 0;
        i32 scope_nest = 0;
        i32 angle_nest = 1;
        
        while (!ctx->done && !result)
        {
            Token* token = token_it_read(&ctx->it);
            switch (token->kind)
            {
                case TokenBaseKind_ScopeOpen:          { scope_nest++; } break;
                case TokenBaseKind_ScopeClose:         { scope_nest--; } break;
                case TokenBaseKind_ParentheticalOpen:  { paren_nest++; } break;
                case TokenBaseKind_ParentheticalClose: { paren_nest--; } break;
                
                case TokenBaseKind_Operator:
                {
                    if (token->sub_kind != TokenCsKind_Dot && token->sub_kind != TokenCsKind_Comma)
                        if (!Long_Index_ParseAngleBody(ctx->app, ctx->file->buffer, token, &angle_nest))
                            goto DONE;
                } break;
            }
            
            result = paren_nest <= 0 && scope_nest <= 0 && angle_nest <= 0;
            Long_ParseCtx_Inc(ctx);
        }
    }
    
    DONE:
    if (!result)
        *ctx = restore_ctx;
    return result;
}

global String8 cs_types[] =
{
    S8Lit("bool"   ),
    S8Lit("byte"   ),
    S8Lit("sbyte"  ),
    S8Lit("char"   ),
    S8Lit("decimal"),
    S8Lit("double" ),
    S8Lit("float"  ),
    S8Lit("int"    ),
    S8Lit("uint"   ),
    S8Lit("nint"   ),
    S8Lit("nuint"  ),
    S8Lit("long"   ),
    S8Lit("ulong"  ),
    S8Lit("short"  ),
    S8Lit("ushort" ),
    S8Lit("object" ),
    S8Lit("string" ),
    S8Lit("dynamic"),
    S8Lit("void"   ),
    S8Lit("var"    ),
};

// NOTE(long): This doesn't parse pointers
function b32 Long_CS_ParseBase(F4_Index_ParseCtx* ctx, Range_i64* base_range)
{
    F4_Index_ParseCtx restore_ctx = *ctx;
    Range_i64 base = {};
    i64 start_pos = ctx->it.ptr->pos;
    b32 result = 0;
    
    // NOTE(long): Tupple Types
    if (Long_CS_ParseKind(ctx, TokenBaseKind_ParentheticalOpen, &base))
    {
        if (Long_Index_SkipExpression(ctx, TokenCsKind_Comma, 0)) // Skip to the first comma
        {
            Long_Index_SkipExpression(ctx, 0, 0); // Skip to before the close paren
            result = Long_CS_ParseKind(ctx, TokenBaseKind_ParentheticalClose, &base); // Advance over the close paren
        }
    }
    
    // NOTE(long): Built-in Types
    else if (Long_CS_ParseKind(ctx, TokenBaseKind_Keyword, &base))
    {
        result = Long_Index_IsMatch(ctx, base, ExpandArray(cs_types));
        if (result && Long_CS_PeekSubKind(ctx, TokenCsKind_BrackOp, 0))
            result = Long_Index_SkipBody(ctx);
    }
    
    // NOTE(long): Custom Types
    else do
    {
        result = Long_CS_ParseKind(ctx, TokenBaseKind_Identifier, &base);
        if (result && Long_CS_PeekSubKind(ctx, TokenCsKind_Less, 0))
            result = Long_CS_ParseGenericBase(ctx);
        if (result && Long_CS_PeekSubKind(ctx, TokenCsKind_BrackOp, 0))
            result = Long_Index_SkipBody(ctx);
        
        if (result)
        {
            if (!Long_CS_IsTokenSelection(ctx->it.ptr))
            {
                Long_Index_PeekPrevious(ctx, base.end = Ii64(ctx->it.ptr).end);
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

function b32 Long_CS_ParseNameAndArg(F4_Index_ParseCtx* ctx, Range_i64* name_range, b32 name_is_op)
{
    F4_Index_ParseCtx restore_ctx = *ctx;
    Range_i64 name = {};
    
    b32 result = Long_CS_ParseKind(ctx, TokenBaseKind_Identifier, &name);
    if (!result && name_is_op)
        result = (Long_CS_ParseKind(ctx, TokenBaseKind_Operator, &name) ||
                  Long_CS_ParseKind(ctx, TokenBaseKind_Keyword , &name));
    
    if (result)
    {
        if (Long_CS_PeekSubKind(ctx, TokenCsKind_Less, 0))
            Long_Index_SkipBody(ctx, 0, 1);
        // NOTE(long): Always pass over the '(' to differentiate between normal function and lambda
        result = Long_CS_ParseSubKind(ctx, TokenCsKind_ParenOp, 0);
    }
    
    if (result)
        *name_range = name;
    else
        *ctx = restore_ctx;
    
    return result;
}

//- TODO(long): Compress ParseFunction and ParseDecl into one
function b32 Long_CS_ParseFunction(F4_Index_ParseCtx* ctx, Range_i64* base_range, Range_i64* name_range, b32* out_modify)
{
    F4_Index_ParseCtx restore_ctx = *ctx;
    Range_i64 base = {};
    Range_i64 name = {};
    b32 use_modify = Long_CS_ParseSubKind(ctx, TokenCsKind_Delegate, 0);
    b32 result = Long_CS_ParseBase(ctx, &base) && Long_CS_ParseNameAndArg(ctx, &name, 0);
    
    if (result)
    {
        if (out_modify) *out_modify = use_modify;
        if (base_range) *base_range = base;
        if (name_range) *name_range = name;
    }
    else *ctx = restore_ctx;
    return result;
}

function b32 Long_CS_ParseDecl(F4_Index_ParseCtx* ctx, Range_i64* base_range, Range_i64* name_range, b32* out_modify)
{
    F4_Index_ParseCtx restore_ctx = *ctx;
    Range_i64 base = {}, name = {};
    b32 use_modify = Long_CS_ParseSubKind(ctx, TokenCsKind_Out, 0);
    b32 result = Long_CS_ParseBase(ctx, &base) && Long_CS_ParseKind(ctx, TokenBaseKind_Identifier, &name);
    
    if (result)
    {
        if (out_modify) *out_modify = use_modify;
        if (base_range) *base_range = base;
        if (name_range) *name_range = name;
    }
    else *ctx = restore_ctx;
    return result;
}

function b32 Long_CS_ParseLambda(F4_Index_ParseCtx* ctx, Range_i64* name_range)
{
    b32 result = 0;
    if (Long_CS_PeekSubKind(ctx, TokenCsKind_ParenOp, 0))
    {
        Token_Iterator_Array it = ctx->it;
        if (Long_Index_SkipBody(ctx->app, &it, ctx->file->buffer, 0, 1))
            result = it.ptr->sub_kind == TokenCsKind_FatArrow;
    }
    
    if (result)
    {
        *name_range = Ii64(ctx->it.ptr->pos);
        Long_ParseCtx_Inc(ctx);
    }
    return result;
}

function b32 Long_CS_ParseAccessor(F4_Index_ParseCtx* ctx, Range_i64* out_range)
{
    Token_Iterator_Array it = ctx->it;
    Token* ident = it.ptr;
    token_it_inc(&it);
    Token* op = it.ptr;
    
    String8 lexeme = F4_Index_StringFromToken(ctx, ident);
    b32 result = string_match(lexeme, S8Lit("get")) || string_match(lexeme, S8Lit("set"));
    result &= op->sub_kind == TokenCsKind_FatArrow || op->kind == TokenBaseKind_ScopeOpen;
    
    if (result)
    {
        if (out_range)
            *out_range = Ii64(ident);
        Long_ParseCtx_Inc(ctx);
    }
    return result;
}

internal F4_LANGUAGE_INDEXFILE(Long_CS_IndexFile)
{
    while (!ctx->done)
    {
        Range_i64 name = {};
        Range_i64 base = {};
        b32 use_modifier = 0;
        b32 initialized = Long_Index_IsParentInitialized(ctx);
        
        // TODO(long): Nil value for ctx->active_parent
        // TODO(long): Rework parsing comments
        // TODO(long): Parse anonymous arguments
        
        if (0) {}
        
        //~ NOTE(long): Parent Scope Changes
        else if (ctx->active_parent && Long_Parse_Scope(ctx).max == ctx->it.ptr->pos)
        {
            if (Long_Index_IsNamespace(ctx->active_parent))
                Long_Parse_Scope(ctx) = {};
            Long_Index_PopParent(ctx);
        }
        
        else if (!initialized && Long_CS_PeekSubKind(ctx, TokenCsKind_FatArrow, 0) &&
                 !Long_Index_PeekNextKind(ctx, TokenBaseKind_ScopeOpen))
        {
            Long_Index_IterBlock(ctx)
                Long_Index_ScopeBlock(ctx)
                    Long_CS_SkipExpression(ctx);
            Long_CS_ParseSubKind(ctx, TokenCsKind_FatArrow, 0);
        }
        
        else if (!initialized && Long_CS_PeekSubKind(ctx, TokenCsKind_Semicolon, 0))
            Long_Index_ScopeBlock(ctx);
        
        else if (Long_CS_PeekKind(ctx, TokenBaseKind_ScopeOpen, &name))
        {
            if (initialized)
                Long_Index_MakeNote(ctx, {}, Ii64(ctx->it.ptr), F4_Index_NoteKind_Scope);
            // NOTE(long): namespaces are always uninitialized
            else if (ctx->active_parent && Long_Index_IsNamespace(ctx->active_parent))
                Long_Index_PushNamespaceScope(ctx);
            Long_Index_StartCtxScope(ctx);
            Long_ParseCtx_Inc(ctx);
        }
        
        else if (ctx->active_parent && Long_CS_PeekKind(ctx, TokenBaseKind_ScopeClose, 0))
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
        
        //~ NOTE(long): Namespaces
        else if (Long_CS_ParseSubKind(ctx, TokenCsKind_Namespace, &base))
        {
            // NOTE(long): Namespaces can only be global or inside another namespace.
            // A namespace can never be inside a class/type/function/decl/etc.
            // This will make the EraseNamespace code easier to handle.
            if (!ctx->active_parent || Long_Index_IsNamespace(ctx->active_parent))
            {
                while (Long_CS_ParseKind(ctx, TokenBaseKind_Identifier, &name))
                {
                    F4_Index_Note* current_namespace = Long_Index_MakeNamespace(ctx, base, name);
                    if (!Long_CS_ParseSubKind(ctx, TokenCsKind_Dot, 0))
                        break;
                }
                
                // NOTE(long): This's a quick and dirty hack to not have to worry about other cases that aren't
                // a currly braces pair in the Parent Scope Changes section. I could just Push/PopNamespaceScope
                // in other cases but C# doesn't handle those so this's good enough for now.
                if (!Long_CS_PeekKind(ctx, TokenBaseKind_ScopeOpen, 0))
                    while (!Long_Index_IsParentInitialized(ctx) && Long_Index_IsNamespace(ctx->active_parent))
                        Long_Index_PopParent(ctx);
            }
        }
        
        //~ NOTE(long): Using
        else if (Long_CS_ParseSubKind(ctx, TokenCsKind_Using, &base))
        {
            String8ListNode* node = push_array(&ctx->file->arena, String8ListNode, 1);
            sll_stack_push(ctx->file->references, node);
            while (Long_CS_ParseKind(ctx, TokenBaseKind_Identifier, &name))
            {
                String8 using_name = push_string_copy(&ctx->file->arena, F4_Index_StringFromRange(ctx, name));
                string_list_push(&ctx->file->arena, &node->list, using_name);
                if (Long_CS_ParseSubKind(ctx, TokenCsKind_Dot, 0))
                    break;
            }
        }
        
        //~ NOTE(long): Types
        else if (Long_CS_ParseSubKind(ctx, TokenCsKind_Enum, &base) ||
                 Long_CS_ParseSubKind(ctx, TokenCsKind_Class, &base) ||
                 Long_CS_ParseSubKind(ctx, TokenCsKind_Struct, &base) ||
                 Long_CS_ParseSubKind(ctx, TokenCsKind_Interface, &base))
        {
            if (Long_CS_ParseKind(ctx, TokenBaseKind_Identifier, &name))
            {
                Long_Index_MakeNote(ctx, base, name, F4_Index_NoteKind_Type);
                Long_CS_ParseGenericArg(ctx);
            }
        }
        
        else if (Long_Parse_Comp(ctx, kind == F4_Index_NoteKind_Type) &&
                 Long_Index_IsMatch(ctx, ctx->active_parent->base_range, S8Lit("enum")) &&
                 Long_CS_ParseKind(ctx, TokenBaseKind_Identifier, &name))
        {
            if (Long_CS_ParseSubKind(ctx, TokenCsKind_Comma, 0) || Long_CS_PeekKind(ctx, TokenBaseKind_ScopeClose, 0))
                Long_Index_MakeNote(ctx, base, name, F4_Index_NoteKind_Constant, 0);
            
            else if (Long_CS_ParseSubKind(ctx, TokenCsKind_Eq, 0))
            {
                Long_Index_MakeNote(ctx, base, name, F4_Index_NoteKind_Constant, 0);
                Long_CS_SkipExpression(ctx);
            }
        }
        
        //~ NOTE(long): Functions
        else if (Long_CS_ParseFunction(ctx, &base, &name, &use_modifier))
        {
            FUNCTION:
            Long_Index_MakeNote(ctx, base, name, use_modifier ? F4_Index_NoteKind_Type : F4_Index_NoteKind_Function);
            Long_Index_IterBlock(ctx)
            {
                ctx->it = token_iterator_pos(0, &ctx->tokens, name.min);
                token_it_inc(&ctx->it);
                Long_CS_ParseGenericArg(ctx);
            }
        }
        
        //~ NOTE(long): Constructors
        else if (Long_Parse_Comp(ctx, kind == F4_Index_NoteKind_Type) && // NOTE(long): This removes the lambda-inside-lambda case
                 Long_CS_ParseNameAndArg(ctx, &name, 0))
        {
            if (Long_Index_IsMatch(ctx, name, ctx->active_parent->string))
                goto FUNCTION;
        }
        
        //~ NOTE(long): Operators
        else if (Long_Parse_Comp(ctx, kind == F4_Index_NoteKind_Type) && Long_CS_ParseSubKind(ctx, TokenCsKind_Operator, &base))
        {
            if (Long_CS_ParseNameAndArg(ctx, &name, 1))
                goto FUNCTION;
        }
        
        //~ NOTE(long): Getters/Setters
        else if (Long_Parse_Comp(ctx, kind == F4_Index_NoteKind_Decl) && Long_CS_ParseAccessor(ctx, &name))
        {
            Long_Index_MakeNote(ctx, base, name, F4_Index_NoteKind_Function);
        }
        
        //~ NOTE(long): Lambda (Anonymous Function)
        else if (Long_CS_ParseLambda(ctx, &name))
        {
            Long_Index_MakeNote(ctx, base, name, F4_Index_NoteKind_Function);
            while (Long_CS_ParseKind(ctx, TokenBaseKind_Identifier, &name))
            {
                Long_Index_MakeNote(ctx, base, name, F4_Index_NoteKind_Decl, 0);
                if (!Long_CS_ParseSubKind(ctx, TokenCsKind_Comma, 0))
                    break;
            }
        }
        
        //~ NOTE(long): Declarations
        else if (Long_CS_ParseDecl(ctx, &base, &name, &use_modifier))
        {
            DECLARATION:
            Long_Index_MakeNote(ctx, base, name, F4_Index_NoteKind_Decl);
            if (use_modifier)
                Long_Index_PopParent(ctx);
            
            else if (!Long_CS_PeekSubKind(ctx, TokenCsKind_FatArrow, 0) && !Long_CS_PeekKind(ctx, TokenBaseKind_ScopeOpen, 0))
            {
                Long_Index_IterBlock(ctx)
                    Long_Index_ScopeBlock(ctx)
                        if (Long_CS_ParseSubKind(ctx, TokenCsKind_Eq, 0))
                            Long_CS_SkipExpression(ctx);
            }
        }
        
        else if (Long_CS_ParseKind(ctx, TokenBaseKind_Identifier, &name))
        {
            // NOTE(long): This is for single argument lambda: x => { }
            if (Long_CS_PeekSubKind(ctx, TokenCsKind_FatArrow, 0))
            {
                Long_Index_MakeNote(ctx, base, Ii64(name.min), F4_Index_NoteKind_Function);
                Long_Index_MakeNote(ctx, base, name, F4_Index_NoteKind_Decl, 0);
            }
            
            else
            {
                F4_Index_Note* prev_sibling = ctx->active_parent ? ctx->active_parent->last_child : ctx->file->last_note;
                if (prev_sibling && prev_sibling->kind == F4_Index_NoteKind_Decl && prev_sibling->scope_range.max)
                {
                    Token_Iterator_Array it = token_iterator_pos(0, &ctx->tokens, prev_sibling->scope_range.max);
                    if (it.ptr->sub_kind == TokenCsKind_Comma && token_it_inc(&it))
                    {
                        if (name.min == it.ptr->pos)
                        {
                            base = prev_sibling->base_range;
                            goto DECLARATION;
                        }
                    }
                }
            }
        }
        
        //~ NOTE(long): Parse Define Macro
        else if (Long_CS_ParseSubKind(ctx, TokenCsKind_PPDefine, 0))
        {
            Long_CS_ParseKind(ctx, TokenBaseKind_Identifier, &name);
            Long_Index_MakeNote(ctx, base, name, F4_Index_NoteKind_Macro, 0);
            goto SKIP_PP_BODY;
        }
        
        else if (Long_CS_ParseSubKind(ctx, TokenCsKind_PPRegion, 0) ||
                 Long_CS_ParseSubKind(ctx, TokenCsKind_PPIf, 0) ||
                 Long_CS_ParseSubKind(ctx, TokenCsKind_PPElIf, 0))
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
internal F4_LANGUAGE_POSCONTEXT(Long_CS_PosContext)
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
                access_found = Long_CS_IsTokenSelection(token);
            if (token->sub_kind == TokenCsKind_Comma && i > 0)
                arg_idx++;
        }
        
        if (!token_it_dec(&it))
            break;
    }
    
    return first;
}

//void name(Application_Links *app, Text_Layout_ID text_layout_id, Token_Array *array, Color_Table color_table)
internal F4_LANGUAGE_HIGHLIGHT(Long_CS_Highlight)
{
#if 0
    Buffer_ID buffer = text_layout_get_buffer(app, text_layout_id);
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    Token_Iterator_Array it = token_iterator_pos(0, array, visible_range.min);
    
    for(;;)
    {
        Token* token = token_it_read(&it);
        if(!token || token->pos >= visible_range.one_past_last)
            break;
        
        if (token->kind == TokenBaseKind_Identifier)
        {
            F4_Index_Note* note = Long_Index_LookupBestNote(app, buffer, array, token);
            if (note)
            {
                switch (note->kind)
                {
                    case F4_Index_NoteKind_Decl:
                    {
                        if (note->parent)
                        {
                            ARGB_Color color;
                            if (Long_Index_IsArgument(note))
                                color = F4_ARGBFromID(color_table, long_color_index_param);
                            else if (note->parent->kind == F4_Index_NoteKind_Type)
                                color = F4_ARGBFromID(color_table, long_color_index_field);
                            else
                                color = F4_ARGBFromID(color_table, long_color_index_local);
                            
                            if (!F4_ARGBIsValid(color))
                                color = F4_ARGBFromID(color_table, defcolor_text_default);
                            paint_text_color(app, text_layout_id, Ii64(token), color);
                        }
                    } break;
                    
                    case F4_Index_NoteKind_Type:
                    {
                        if (Long_Index_IsNamespace(note)) break;
                        
                        NOTE_TYPE_CASE:
                        Scratch_Block scratch(app);
                        String8 base_type = push_buffer_range(app, scratch, note->file->buffer, note->base_range);
                        String8 types[] = { S8Lit("struct"), S8Lit("enum") };
                        ARGB_Color color = F4_ARGBFromID(color_table, fleury_color_index_product_type);
                        if (Long_Index_IsMatch(base_type, ExpandArray(types)))
                        {
                            ARGB_Color value_type_color = F4_ARGBFromID(color_table, fleury_color_index_sum_type);
                            if (F4_ARGBIsValid(value_type_color))
                                color = value_type_color;
                        }
                        paint_text_color(app, text_layout_id, Ii64(token), color);
                    } break;
                    
                    case F4_Index_NoteKind_Function:
                    {
                        if (note->parent && note->parent->kind == F4_Index_NoteKind_Type && string_match(note->string, note->parent->string))
                        {
                            note = note->parent;
                            goto NOTE_TYPE_CASE;
                        }
                    } break;
                }
            }
        }
        
        if(!token_it_inc(&it))
            break;
    }
#endif
}

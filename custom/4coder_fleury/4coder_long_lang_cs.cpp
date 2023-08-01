
#define Long_CS_SkipExpression(ctx) Long_Index_SkipExpression(ctx, TokenCsKind_Comma, TokenCsKind_Semicolon)

function b32 Long_CS_IsTokenSelection(Token* token)
{
    return (token->sub_kind == TokenCsKind_Dot   ||
            token->sub_kind == TokenCsKind_Arrow ||
            token->sub_kind == TokenCsKind_TernaryDot);
}

enum
{
    Long_ParseFlag_NoName    = 1 << 0,
    Long_ParseFlag_NoBase    = 1 << 1,
    Long_ParseFlag_Operator  = 1 << 2,
    Long_ParseFlag_HasArg    = 1 << 3,
    Long_ParseFlag_Anonymous = Long_ParseFlag_HasArg|Long_ParseFlag_NoBase,
};
typedef u32 Long_ParseFlags;

// TODO(long): Clean this into a generic index function
function b32 Long_CS_ParseDecl(F4_Index_ParseCtx* ctx, Range_i64* base_range, Range_i64* name_range,
                               String8* base_keywords, u64 keyword_count, Long_ParseFlags flags,
                               char* start_token = 0, b32* optional_start_token = 0, char* end_token = 0)
{
    Long_Index_ProfileScope(ctx->app, "[Long] Parse Decl");
    b32 result = true;
    F4_Index_ParseCtx restore_ctx = *ctx;
    Range_i64 base = {};
    Range_i64 name = {};
    
    if (start_token)
    {
        b32* dest = optional_start_token ? optional_start_token : &result;
        *dest = Long_Index_ParsePattern(ctx, "%t", start_token);
    }
    
    b32 has_base_type = !(flags & Long_ParseFlag_NoBase);
    if (has_base_type || base_keywords)
    {
        Range_i64 type_range = {};
        if (has_base_type && Long_Index_ParsePattern(ctx, "%k", TokenBaseKind_ParentheticalOpen, &base)) // NOTE(long): Tupple
        {
            result = Long_Index_SkipExpression(ctx, TokenCsKind_Comma, 0);
            Long_Index_SkipExpression(ctx, 0, 0);
            Long_Index_ParsePattern(ctx, "%k", TokenBaseKind_ParentheticalClose, &type_range);
        }
        else while (result && !ctx->done)
        {
            Long_Index_ProfileBlock(ctx->app, "[Long] Parse Base Type");
            {
                Long_Index_ProfileBlock(ctx->app, "[Long] Parse Identifier");
                if (base_keywords && Long_Index_ParsePattern(ctx, "%k", TokenBaseKind_Keyword, &type_range))
                result = Long_Index_IsMatch(ctx, type_range, base_keywords, keyword_count);
                else if (has_base_type)
                result = Long_Index_ParsePattern(ctx, "%k", TokenBaseKind_Identifier, &type_range);
                else
                    result = false;
            }
            
            if (result)
            {
                Long_Index_ProfileBlock(ctx->app, "[Long] Parse Operator");
                if (base == Range_i64{})
                base.start = type_range.start;
                if (Long_Index_PeekPattern(ctx, "%t", "[") || Long_Index_PeekPattern(ctx, "%t", "<"))
                {
                    result = Long_Index_SkipBody(ctx);
                    Long_Index_PeekPrevious(ctx, type_range.end = Ii64(Long_Index_Token(ctx)).end);
                }
            }
            
            if (!Long_CS_IsTokenSelection(Long_Index_Token(ctx)))
            break;
            
            token_it_inc(&ctx->it);
        }
        base.end = type_range.end;
    }
    
    if (result)
    {
        if (flags & Long_ParseFlag_NoName) name = Ii64(Long_Index_Token(ctx)->pos);
        else
        {
            result = Long_Index_ParsePattern(ctx, "%k", TokenBaseKind_Identifier, &name);
            if (!result && (flags & Long_ParseFlag_Operator))
            result = (Long_Index_ParsePattern(ctx, "%k", TokenBaseKind_Operator, &name) ||
                      Long_Index_ParsePattern(ctx, "%k", TokenBaseKind_Keyword , &name));
        }
    }
    
    b32 has_arg = (flags & Long_ParseFlag_HasArg);
    if (result && has_arg)
    {
        if (Long_Index_PeekPattern(ctx, "%t", "<"))
        Long_Index_SkipBody(ctx, 0, 1);
        // NOTE(long): Always pass over the '(' to differentiate between normal function and lambda
        result = Long_Index_ParsePattern(ctx, "%t", "(");
    }
    
    if (result && end_token)
    {
        Long_Index_BeginCtxChange(ctx);
        if (has_arg)
        {
            token_it_dec(&ctx->it);
            Long_Index_SkipBody(ctx, 0, 1);
        }
        if (result)
        result = Long_Index_ParsePattern(ctx, "%t", end_token);
        Long_Index_EndCtxChange(ctx);
    }
    
    if (result)
    {
        *name_range = name;
        *base_range = base;
    }
    else *ctx = restore_ctx;
    
    return result;
}

function void Long_CS_ParseGeneric(F4_Index_ParseCtx* ctx)
{
    if (Long_Index_ParsePattern(ctx, "%t", "<"))
    {
        Range_i64 generic;
        while (Long_Index_ParsePattern(ctx, "%k", TokenBaseKind_Identifier, &generic))
        {
            Long_Index_MakeNote(ctx, {}, generic, F4_Index_NoteKind_Type, 0);
            if (!Long_Index_ParsePattern(ctx, "%t", ","))
            break;
        }
    }
}

internal F4_LANGUAGE_INDEXFILE(Long_CS_IndexFile)
{
    String8 cs_types[] =
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
    
    String8 cs_type_keywords[] =
    {
        S8Lit("enum"), // NOTE(long): enum has to be at the top of the list
        S8Lit("struct"),
        S8Lit("class"),
        S8Lit("interface"),
    };
    
    String8 cs_operator[] = { S8Lit("operator") };
    
    {
        Scratch_Block scratch(ctx->app);
        String8 buffer_name = push_buffer_base_name(ctx->app, scratch, ctx->file->buffer);
        if (string_match(buffer_name, S8Lit("NavMesh.cs")))
        buffer_name = buffer_name;
    }
    
    while (!ctx->done)
    {
        Long_Index_ProfileBlock(ctx->app, "[Long] Parse CS");
        Range_i64 name = {};
        Range_i64 base = {};
        b32 use_modifier = 0;
        F4_Index_NoteKind kind = {};
        b32 initialized = Long_Index_IsParentInitialized(ctx);
        
        if (0){}
        
        //~ NOTE(long): Parent Scope Changes
        else if (ctx->active_parent && Long_Index_CtxScope(ctx).max == Long_Index_Token(ctx)->pos)
        {
            if (Long_Index_IsNamespace(ctx->active_parent))
            Long_Index_CtxScope(ctx) = {};
            Long_Index_PopParent(ctx);
        }
        else if (!initialized && Long_Index_PeekPattern(ctx, "%t", "=>") && !Long_Index_PeekPattern(ctx, "%t%t", "=>", "{"))
        {
            Long_Index_BlockCtxChange(ctx, Long_Index_BlockCtxScope(ctx, Long_CS_SkipExpression(ctx)));
            Long_Index_ParsePattern(ctx, "%t", "=>");
        }
        else if (!initialized && Long_Index_PeekPattern(ctx, "%t", ";"))
        {
            Long_Index_BlockCtxScope(ctx, 0);
        }
        
        else if (Long_Index_ParsePattern(ctx, "%t", "{"))
        {
            Long_Index_PeekPrevious(ctx,
                                    {
                                        if (initialized)
                                        Long_Index_MakeNote(ctx, {}, Ii64(Long_Index_Token(ctx)), F4_Index_NoteKind_Scope);
                                        // NOTE(long): namespaces are always uninitialized
                                        else if (ctx->active_parent && Long_Index_IsNamespace(ctx->active_parent))
                                        Long_Index_PushNamespaceScope(ctx);
                                        Long_Index_StartCtxScope(ctx);
                                    });
        }
        else if (ctx->active_parent && Long_Index_PeekPattern(ctx, "%t", "}"))
        {
            // NOTE(long): The last leaf namespace can have scope_range in one file while (base)range in other files
            // So it must be cleared here at the end of its scope
            if (Long_Index_IsNamespace(ctx->active_parent))
            Long_Index_PopNamespaceScope(ctx);
            else
                Long_Index_EndCtxScope(ctx);
            do Long_Index_PopParent(ctx);
            while (!Long_Index_IsParentInitialized(ctx));
            F4_Index_ParseCtx_Inc(ctx, F4_Index_TokenSkipFlag_SkipAll);
        }
        
        //~ NOTE(long): Namespaces
        else if (Long_Index_ParsePattern(ctx, "%b", TokenCsKind_Namespace, &base))
        {
            while (Long_Index_ParsePattern(ctx, "%k", TokenBaseKind_Identifier, &name))
            {
                Long_Index_MakeNamespace(ctx, base, name);
                if (!Long_Index_ParsePattern(ctx, "%t", "."))
                break;
            }
        }
        
        //~ NOTE(long): Using
        else if (Long_Index_ParsePattern(ctx, "%b", TokenCsKind_Using, &base))
        {
            String8ListNode* node = push_array(&ctx->file->arena, String8ListNode, 1);
            sll_stack_push(ctx->file->references, node);
            while (Long_Index_ParsePattern(ctx, "%k", TokenBaseKind_Identifier, &name))
            {
                String8 using_name = push_string_copy(&ctx->file->arena, F4_Index_StringFromRange(ctx, name));
                string_list_push(&ctx->file->arena, &node->list, using_name);
                if (!Long_Index_ParsePattern(ctx, "%t", "."))
                break;
            }
        }
        
        //~ NOTE(long): Types
        else if (Long_CS_ParseDecl(ctx, &base, &name, ExpandArray(cs_type_keywords), Long_ParseFlag_NoBase))
        {
            Long_Index_MakeNote(ctx, base, name, F4_Index_NoteKind_Type);
            Long_CS_ParseGeneric(ctx);
        }
        
        //~ NOTE(long): Functions
        else if (Long_CS_ParseDecl(ctx, &base, &name, ExpandArray(cs_types), Long_ParseFlag_HasArg, "delegate", &use_modifier))
        {
            Long_Index_MakeNote(ctx, base, name, use_modifier ? F4_Index_NoteKind_Type : F4_Index_NoteKind_Function);
            Long_Index_BeginCtxChange(ctx);
            ctx->it = token_iterator_pos(0, &ctx->tokens, name.min);
            token_it_inc(&ctx->it);
            Long_CS_ParseGeneric(ctx);
            Long_Index_EndCtxChange(ctx);
        }
        
        //~ NOTE(long): Constructors
        else if (Long_Index_CtxCompare(ctx, kind == F4_Index_NoteKind_Type) && // NOTE(long): This removes the lambda-inside-lambda case
                 Long_CS_ParseDecl(ctx, &base, &name, 0, 0, Long_ParseFlag_Anonymous))
        {
            if (string_match(F4_Index_StringFromRange(ctx, name), ctx->active_parent->string))
            Long_Index_MakeNote(ctx, {}, name, F4_Index_NoteKind_Function);
        }
        
        //~ NOTE(long): Operators
        else if (Long_Index_CtxCompare(ctx, kind == F4_Index_NoteKind_Type) &&
                 Long_CS_ParseDecl(ctx, &base, &name, ExpandArray(cs_operator), Long_ParseFlag_Operator|Long_ParseFlag_Anonymous))
        {
            Long_Index_MakeNote(ctx, base, name, F4_Index_NoteKind_Function);
        }
        
        //~ NOTE(long): Getters/Setters
        else if (Long_Index_CtxCompare(ctx, kind == F4_Index_NoteKind_Decl) &&
                 (Long_Index_PeekPattern(ctx, "%t%t", "get", "=>") || Long_Index_PeekPattern(ctx, "%t%t", "set", "=>") ||
                  Long_Index_PeekPattern(ctx, "%t%t", "get", "{" ) || Long_Index_PeekPattern(ctx, "%t%t", "set", "{" )))
        {
            Long_Index_ParsePattern(ctx, "%k", TokenBaseKind_Identifier, &name);
            Long_Index_MakeNote(ctx, {}, name, F4_Index_NoteKind_Function);
        }
        
        //~ NOTE(long): Lambda (Anonymous Function)
        else if (Long_CS_ParseDecl(ctx, &base, &name, 0, 0, Long_ParseFlag_NoName|Long_ParseFlag_Anonymous, 0, 0, "=>"))
        {
            Long_Index_MakeNote(ctx, base, name, F4_Index_NoteKind_Function);
        }
        
        // NOTE(long): This is for single argument lambda: x => { }
        else if (Long_Index_CtxCompare(ctx, range.min != Long_Index_Token(ctx)->pos) &&
                 Long_Index_PeekPattern(ctx, "%k%t", TokenBaseKind_Identifier, &name, "=>"))
        {
            Long_Index_MakeNote(ctx, {}, Ii64(name.min), F4_Index_NoteKind_Function);
        }
        
        //~ NOTE(long): Declarations
        else if (Long_CS_ParseDecl(ctx, &base, &name, ExpandArray(cs_types), 0, "out", &use_modifier))
        {
            kind = F4_Index_NoteKind_Decl;
            
            DECLARATION:
            Long_Index_MakeNote(ctx, base, name, kind);
            if (use_modifier)
            Long_Index_PopParent(ctx);
            else if (!range_size(base) || (!Long_Index_PeekPattern(ctx, "%t", "=>") && !Long_Index_PeekPattern(ctx, "%t", "{")))
            {
                Long_Index_BeginCtxChange(ctx);
                
                i64 start_pos = Long_Index_Token(ctx)->pos;
                if (Long_Index_ParsePattern(ctx, "%t", "="))
                Long_CS_SkipExpression(ctx);
                i64 end_pos = Long_Index_Token(ctx)->pos;
                Long_Index_CtxScope(ctx) = { start_pos, end_pos };
                
                Long_Index_EndCtxChange(ctx);
            }
        }
        
        else if (Long_Index_ParsePattern(ctx, "%k", TokenBaseKind_Identifier, &name))
        {
            F4_Index_Note* prev_sibling = ctx->active_parent ? ctx->active_parent->last_child : ctx->file->last_note;
            if (prev_sibling && prev_sibling->kind == F4_Index_NoteKind_Decl && prev_sibling->scope_range.max)
            {
                Long_Index_BeginCtxChange(ctx);
                ctx->it = token_iterator_pos(0, &ctx->tokens, prev_sibling->scope_range.max);
                Token* end = Long_Index_Token(ctx);
                if (end->sub_kind != TokenCsKind_Comma)
                {
                    token_it_inc(&ctx->it);
                    end = Long_Index_Token(ctx);
                }
                
                if (end->sub_kind == TokenCsKind_Comma && token_it_inc(&ctx->it))
                {
                    end = Long_Index_Token(ctx);
                    if (name.start == end->pos)
                    {
                        base = prev_sibling->base_range;
                        kind = prev_sibling->kind;
                    }
                }
                Long_Index_EndCtxChange(ctx);
            }
            
            if (ctx->active_parent)
            {
                switch (ctx->active_parent->kind)
                {
                    case F4_Index_NoteKind_Function: // NOTE(long): This is for lambda's arguments
                    {
                        if (range_size(ctx->active_parent->range) == 0 && Long_Index_CtxScope(ctx) == Range_i64{})
                        kind = F4_Index_NoteKind_Decl;
                    } break;
                    
                    case F4_Index_NoteKind_Type: // NOTE(long): This is for all the fields of an enum
                    {
                        if (Long_Index_IsMatch(ctx, ctx->active_parent->base_range, cs_type_keywords, 1))
                        kind = F4_Index_NoteKind_Constant;
                    } break;
                }
            }
            
            if (kind != F4_Index_NoteKind_Null)
            goto DECLARATION;
        }
        
        //~ NOTE(long): Parse Define Macro
        else if (Long_Index_ParsePattern(ctx, "%b", TokenCsKind_PPDefine, &name) ||
                 Long_Index_ParsePattern(ctx, "%b", TokenCsKind_PPRegion,     0) ||
                 Long_Index_ParsePattern(ctx, "%b", TokenCsKind_PPIf    ,     0) ||
                 Long_Index_ParsePattern(ctx, "%b", TokenCsKind_PPElIf  ,     0))
        {
            if (name.min)
            {
                Long_Index_ParsePattern(ctx, "%k", TokenBaseKind_Identifier, &name);
                Long_Index_MakeNote(ctx, {}, name, F4_Index_NoteKind_Macro, 0);
            }
            F4_Index_SkipSoftTokens(ctx, 1);
        }
        
        else F4_Index_ParseCtx_Inc(ctx, F4_Index_TokenSkipFlag_SkipAll);
    }
}

function F4_Language_PosContextData*
Long_CS_ParsePosContext(Application_Links* app, Arena* arena, Buffer_ID buffer, Token_Array* array, i64 pos)
{
    Token_Iterator_Array it = token_iterator_pos(0, array, pos);
    b32 access_found = 0;
    b32 has_arg = 0;
    i32 paren_nest = 0;
    i32 arg_idx = 0;
    Vec2_f32 offset = {};
    
    F4_Language_PosContextData *first = 0;
    F4_Language_PosContextData *last = 0;
    
    for (i32 i = 0, tooltip_count = 0; tooltip_count < 4; ++i)
    {
        Token* token = token_it_read(&it);
        
        if (token->kind == TokenBaseKind_ParentheticalClose && i > 0)
        paren_nest--;
        else if (token->kind == TokenBaseKind_ParentheticalOpen)
        {
            i32 old_nest = paren_nest;
            paren_nest = clamp_top(paren_nest + 1, 0);
            if (old_nest == 0 && token_it_dec(&it))
            {
                token = token_it_read(&it);
                if (token->kind == TokenBaseKind_Identifier)
                {
                    has_arg = i > 0;
                    goto LOOKUP;
                }
                token_it_inc(&it);
            }
        }
        else if (paren_nest >= 0)
        {
            if (token->kind == TokenBaseKind_Identifier && (i < 2 || (i == 2 && access_found)) && tooltip_count == 0)
            {
                LOOKUP:
                F4_Index_Note* note = Long_Index_LookupBestNote(app, buffer, array, token);
                if (note)
                {
                    i32 index = access_found;
                    if (note->kind == F4_Index_NoteKind_Function)
                    {
                        index = has_arg ? arg_idx : -1; // NOTE(long): -1 means don't highlight any argument
                        arg_idx = 0;
                    }
                    //Long_Index_DrawTooltip(app, view_get_screen_rect(app, view), array, note, index, &offset);
                    F4_Language_PosContext_PushData(arena, &first, &last, note, 0, index);
                    tooltip_count++;
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

//F4_Language_PosContextData * name(Application_Links *app, Arena *arena, Buffer_ID buffer, i64 pos)
internal F4_LANGUAGE_POSCONTEXT(Long_CS_PosContext)
{
    Token_Array tokens = get_token_array_from_buffer(app, buffer);
    return Long_CS_ParsePosContext(app, arena, buffer, &tokens, pos);
}

//void name(Application_Links *app, Text_Layout_ID text_layout_id, Token_Array *array, Color_Table color_table)
internal F4_LANGUAGE_HIGHLIGHT(Long_CS_Highlight)
{
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
            if (note && note->kind == F4_Index_NoteKind_Decl)
            {
                ARGB_Color color = F4_ARGBFromID(color_table, fleury_color_index_decl);
                if (note->parent)
                {
                    if (Long_Index_IsArgument(note))
                    color = F4_ARGBFromID(color_table, long_color_index_param);
                    else if (note->parent->kind == F4_Index_NoteKind_Type)
                    color = F4_ARGBFromID(color_table, long_color_index_field);
                    else
                        color = F4_ARGBFromID(color_table, long_color_index_local);
                    if (!F4_ARGBIsValid(color))
                    color = F4_ARGBFromID(color_table, defcolor_text_default);
                }
                paint_text_color(app, text_layout_id, Ii64(token), color);
            }
        }
        
        if(!token_it_inc(&it))
        break;
    }
    
    // NOTE(long): The Highlight function will get called before draw_text_layout_default inside F4_RenderBuffer,
    // while PosContext will get called after, which make it get drawed on top of the whole buffer.
#if 0
    View_ID view = get_active_view(app, Access_Always);
    if (buffer == view_get_buffer(app, view, Access_Always))
    {
        Scratch_Block scratch(app);
        i64 pos = view_correct_cursor(app, view);
        Long_Index_DrawPosContext(app, view, array, Long_CS_ParsePosContext(app, scratch, buffer, array, pos));
    }
#endif
}

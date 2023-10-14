
#if LONG_INDEX_INLINE
#define Long_CS_ParseStr(ctx, str)             Long_Index_ParseStr    (ctx, str)
#define Long_CS_ParseToken(ctx, str)           Long_Index_ParseStr    (ctx, S8Lit(str))
#define Long_CS_ParseKind(ctx, kind, range)    Long_Index_ParseKind   (ctx, kind, range)
#define Long_CS_ParseSubKind(ctx, kind, range) Long_Index_ParseSubKind(ctx, kind, range)
#define Long_CS_PeekToken(ctx, str)            Long_Index_PeekStr     (ctx, S8Lit(str))
#define Long_CS_PeekTwo(ctx, strA, strB)       Long_CS_PeekTwoStr     (ctx, S8Lit(strA), S8Lit(strB))
#else
#define Long_CS_ParseStr(ctx, string)          Long_Index_ParsePattern(ctx, "%t", string.str)
#define Long_CS_ParseToken(ctx, str)           Long_Index_ParsePattern(ctx, "%t", str)
#define Long_CS_ParseKind(ctx, kind, range)    Long_Index_ParsePattern(ctx, "%k", kind, range)
#define Long_CS_ParseSubKind(ctx, kind, range) Long_Index_ParsePattern(ctx, "%b", kind, range)
#define Long_CS_PeekToken(ctx, str)            Long_Index_PeekPattern (ctx, "%t", str)
#define Long_CS_PeekTwo(ctx, strA, strB)       Long_Index_PeekPattern (ctx, "%t%t", strA, strB)
#endif

#define Long_CS_SkipExpression(ctx) Long_Index_SkipExpression(ctx, TokenCsKind_Comma, TokenCsKind_Semicolon)

function b32 Long_CS_PeekTwoStr(F4_Index_ParseCtx* ctx, String8 strA, String8 strB)
{
    F4_Index_ParseCtx ctx_restore = *ctx;
    b32 result = 0;
    String8 strings[2] = { strA, strB };
    
    for (i32 i = 0; i < 2; ++i)
    {
        Token* token = token_it_read(&ctx->it);
        if (token)
        result = string_match(strings[i], string_substring(ctx->string, Ii64(token)));
        else
            ctx->done = 1;
        
        if(result)
        F4_Index_ParseCtx_Inc(ctx, F4_Index_TokenSkipFlag_SkipAll);
        else
            break;
    }
    
    *ctx = ctx_restore;
    return result;
}

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
                               String8 start_token = {}, b32* optional_start_token = 0, String8 end_token = {})
{
    Long_Index_ProfileScope(ctx->app, "[Long] Parse Decl");
    b32 result = true;
    F4_Index_ParseCtx restore_ctx = *ctx;
    Range_i64 base = {};
    Range_i64 name = {};
    
    if (start_token.str)
    {
        b32* dest = optional_start_token ? optional_start_token : &result;
        *dest = Long_CS_ParseStr(ctx, start_token);
    }
    
    b32 has_base_type = !(flags & Long_ParseFlag_NoBase);
    if (has_base_type || base_keywords)
    {
        Range_i64 type_range = {};
        if (has_base_type && Long_CS_ParseKind(ctx, TokenBaseKind_ParentheticalOpen, &base)) // NOTE(long): Tupple
        {
            result = Long_Index_SkipExpression(ctx, TokenCsKind_Comma, 0);
            Long_Index_SkipExpression(ctx, 0, 0);
            Long_CS_ParseKind(ctx, TokenBaseKind_ParentheticalClose, &type_range);
        }
        else while (result && !ctx->done)
        {
            Long_Index_ProfileBlock(ctx->app, "[Long] Parse Base Type");
            {
#if true//LONG_INDEX_INLINE
                result = false;
                F4_Index_ParseCtx temp_ctx = *ctx;
                
                Token* token = token_it_read(&ctx->it);
                if (token)
                {
                    if (base_keywords && token->kind == TokenBaseKind_Keyword)
                    {
                        type_range = Ii64(token);
                        result = Long_Index_IsMatch(ctx, type_range, base_keywords, keyword_count);
                    }
                    else if (has_base_type && token->kind == TokenBaseKind_Identifier)
                    {
                        type_range = Ii64(token);
                        result = true;
                    }
                }
                else
                    ctx->done = 1;
                
                if (result)
                F4_Index_ParseCtx_Inc(ctx, F4_Index_TokenSkipFlag_SkipAll);
                else
                    *ctx = temp_ctx;
#else
                if (base_keywords && Long_CS_ParseKind(ctx, TokenBaseKind_Keyword, &type_range))
                result = Long_Index_IsMatch(ctx, type_range, base_keywords, keyword_count);
                else if (has_base_type)
                result = Long_CS_ParseKind(ctx, TokenBaseKind_Identifier, &type_range);
                else
                    result = false;
#endif
            }
            
            if (result)
            {
                if (base == Range_i64{})
                base.start = type_range.start;
                if (Long_CS_PeekToken(ctx, "[") || Long_CS_PeekToken(ctx, "<"))
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
            result = Long_CS_ParseKind(ctx, TokenBaseKind_Identifier, &name);
            if (!result && (flags & Long_ParseFlag_Operator))
            result = (Long_CS_ParseKind(ctx, TokenBaseKind_Operator, &name) ||
                      Long_CS_ParseKind(ctx, TokenBaseKind_Keyword , &name));
        }
    }
    
    b32 has_arg = (flags & Long_ParseFlag_HasArg);
    if (result && has_arg)
    {
        if (Long_CS_PeekToken(ctx, "<"))
        Long_Index_SkipBody(ctx, 0, 1);
        // NOTE(long): Always pass over the '(' to differentiate between normal function and lambda
        result = Long_CS_ParseToken(ctx, "(");
    }
    
    if (result && end_token.str)
    {
        Long_Index_BeginCtxChange(ctx);
        if (has_arg)
        {
            token_it_dec(&ctx->it);
            Long_Index_SkipBody(ctx, 0, 1);
        }
        if (result)
        result = Long_CS_ParseStr(ctx, end_token);
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
    if (Long_CS_ParseToken(ctx, "<"))
    {
        Range_i64 generic;
        while (Long_CS_ParseKind(ctx, TokenBaseKind_Identifier, &generic))
        {
            Long_Index_MakeNote(ctx, {}, generic, F4_Index_NoteKind_Type, 0);
            if (!Long_CS_ParseToken(ctx, ","))
            break;
        }
    }
}

internal F4_LANGUAGE_INDEXFILE(Long_CS_IndexFile)
{
    while (!ctx->done)
    {
        Long_Index_ProfileBlock(ctx->app, "[Long] Parse CS");
        Range_i64 name = {};
        Range_i64 base = {};
        b32 use_modifier = 0;
        F4_Index_NoteKind kind = {};
        b32 initialized = Long_Index_IsParentInitialized(ctx);
        
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
        
        if (0) {}
        
        //~ NOTE(long): Parent Scope Changes
        else if (ctx->active_parent && Long_Index_CtxScope(ctx).max == Long_Index_Token(ctx)->pos)
        {
            if (Long_Index_IsNamespace(ctx->active_parent))
            Long_Index_CtxScope(ctx) = {};
            Long_Index_PopParent(ctx);
        }
        else if (!initialized && Long_CS_PeekToken(ctx, "=>") && !Long_CS_PeekTwo(ctx, "=>", "{"))
        {
            Long_Index_BlockCtxChange(ctx, Long_Index_BlockCtxScope(ctx, Long_CS_SkipExpression(ctx)));
            Long_CS_ParseToken(ctx, "=>");
        }
        else if (!initialized && Long_CS_PeekToken(ctx, ";"))
        {
            Long_Index_BlockCtxScope(ctx, 0);
        }
        
        else if (Long_CS_ParseToken(ctx, "{"))
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
        else if (ctx->active_parent && Long_CS_PeekToken(ctx, "}"))
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
        else if (Long_CS_ParseSubKind(ctx, TokenCsKind_Namespace, &base))
        {
            while (Long_CS_ParseKind(ctx, TokenBaseKind_Identifier, &name))
            {
                Long_Index_MakeNamespace(ctx, base, name);
                if (!Long_CS_ParseToken(ctx, "."))
                break;
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
                if (!Long_CS_ParseToken(ctx, "."))
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
        else if (Long_CS_ParseDecl(ctx, &base, &name, ExpandArray(cs_types), Long_ParseFlag_HasArg, S8Lit("delegate"), &use_modifier))
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
                 (Long_CS_PeekTwo(ctx, "get", "=>") || Long_CS_PeekTwo(ctx, "set", "=>") ||
                  Long_CS_PeekTwo(ctx, "get", "{" ) || Long_CS_PeekTwo(ctx, "set", "{" )))
        {
            Long_CS_ParseKind(ctx, TokenBaseKind_Identifier, &name);
            Long_Index_MakeNote(ctx, {}, name, F4_Index_NoteKind_Function);
        }
        
        //~ NOTE(long): Lambda (Anonymous Function)
        else if (Long_CS_ParseDecl(ctx, &base, &name, 0, 0, Long_ParseFlag_NoName|Long_ParseFlag_Anonymous, {}, 0, S8Lit("=>")))
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
        else if (Long_CS_ParseDecl(ctx, &base, &name, ExpandArray(cs_types), 0, S8Lit("out"), &use_modifier))
        {
            kind = F4_Index_NoteKind_Decl;
            
            DECLARATION:
            Long_Index_MakeNote(ctx, base, name, kind);
            if (use_modifier)
            Long_Index_PopParent(ctx);
            else if (!range_size(base) || (!Long_CS_PeekToken(ctx, "=>") && !Long_CS_PeekToken(ctx, "{")))
            {
                Long_Index_BeginCtxChange(ctx);
                
                i64 start_pos = Long_Index_Token(ctx)->pos;
                if (Long_CS_ParseToken(ctx, "="))
                Long_CS_SkipExpression(ctx);
                i64 end_pos = Long_Index_Token(ctx)->pos;
                Long_Index_CtxScope(ctx) = { start_pos, end_pos };
                
                Long_Index_EndCtxChange(ctx);
            }
        }
        
        else if (Long_CS_ParseKind(ctx, TokenBaseKind_Identifier, &name))
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
        else if (Long_CS_ParseSubKind(ctx, TokenCsKind_PPDefine, 0))
        {
            Long_CS_ParseKind(ctx, TokenBaseKind_Identifier, &name);
            Long_Index_MakeNote(ctx, {}, name, F4_Index_NoteKind_Macro, 0);
            goto SKIP_PP_BODY;
        }
        else if (Long_CS_ParseSubKind(ctx, TokenCsKind_PPRegion,     0) ||
                 Long_CS_ParseSubKind(ctx, TokenCsKind_PPIf    ,     0) ||
                 Long_CS_ParseSubKind(ctx, TokenCsKind_PPElIf  ,     0))
        {
            SKIP_PP_BODY:
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
            if (token->kind == TokenBaseKind_Identifier && tooltip_count == 0)
            {
                b32 field_accessing = (i == 2 && access_found);
                b32 inside_token = range_contains_inclusive(Ii64(token), pos);
                if (field_accessing || inside_token)
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
                        
                        if (note->range.min != token->pos)
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

#if 0
function void Long_CS_ParsePos(Application_Links* app, F4_Index_File* file, Token_Iterator_Array* it)
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
        
        if (token_it_dec(it) && Long_CS_IsTokenSelection(it->ptr) && token_it_dec(it))
        Long_Index_ParseSelection(app, arena, it, buffer, list);
    }
}
#endif

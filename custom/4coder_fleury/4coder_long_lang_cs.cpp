
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
    ProfileScope(ctx->app, "[Long] Parse Decl");
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
            ProfileBlock(ctx->app, "[Long] Parse Base Type");
            {
                ProfileBlock(ctx->app, "[Long] Parse Identifier");
                if (base_keywords && Long_Index_ParsePattern(ctx, "%k", TokenBaseKind_Keyword, &type_range))
                result = Long_Index_IsMatch(ctx, type_range, base_keywords, keyword_count);
                else if (has_base_type)
                result = Long_Index_ParsePattern(ctx, "%k", TokenBaseKind_Identifier, &type_range);
                else
                    result = false;
            }
            
            if (result)
            {
                ProfileBlock(ctx->app, "[Long] Parse Operator");
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

#define LONG_INDEX_PARSE_NAMESPACE 1

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
#if !LONG_INDEX_PARSE_NAMESPACE
        S8Lit("namespace"),
#endif
    };
    
    String8 cs_operator[] = { S8Lit("operator") };
    
    while (!ctx->done)
    {
        ProfileBlock(ctx->app, "[Long] Parse CS");
        Range_i64 name = {};
        Range_i64 base = {};
        b32 use_modifier = 0;
        F4_Index_NoteKind kind = {};
        b32 initialized = Long_Index_IsParentInitialized(ctx);
        
        if (0){}
        
        //~ NOTE(long): Parent Scope Changes
        else if (ctx->active_parent && Long_Index_CtxScope(ctx).max == Long_Index_Token(ctx)->pos)
        {
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
                                        Long_Index_StartCtxScope(ctx);
                                    });
        }
        else if (ctx->active_parent && Long_Index_ParsePattern(ctx, "%t", "}"))
        {
#if LONG_INDEX_PARSE_NAMESPACE
            Long_Index_PeekPrevious(ctx,
                                    {
                                        Long_Index_EndCtxScope(ctx);
                                        do Long_Index_PopParent(ctx);
                                        while (!Long_Index_IsParentInitialized(ctx));
                                    });
#else
            Long_Index_PeekPrevious(ctx, Long_Index_EndCtxScope(ctx); Long_Index_PopParent(ctx));
#endif
        }
        
        //~ NOTE(long): Namespaces
#if LONG_INDEX_PARSE_NAMESPACE
        else if (Long_Index_ParsePattern(ctx, "%b", TokenCsKind_Namespace, &base))
        {
            while (Long_Index_ParsePattern(ctx, "%k", TokenBaseKind_Identifier, &name))
            {
                String8 name_string = F4_Index_StringFromRange(ctx, name);
                F4_Index_Note* note = Long_Index_LookupNote(name_string);
                for (; note; note = note->next)
                if (note->parent == ctx->active_parent)
                break;
                if (note) F4_Index_PushParent(ctx, note);
                else      Long_Index_MakeNote(ctx, base, name, F4_Index_NoteKind_Type, 1);
                ctx->active_parent->scope_range = {};
                if (!Long_Index_ParsePattern(ctx, "%t", "."))
                break;
            }
        }
#endif
        
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
        
#define LONG_INDEX_PARSE_DECL 1
        
        //~ NOTE(long): Declarations
        else if (Long_CS_ParseDecl(ctx, &base, &name, ExpandArray(cs_types), 0, "out", &use_modifier))
        {
            kind = F4_Index_NoteKind_Decl;
#if !LONG_INDEX_PARSE_DECL
            // NOTE(long): "int A, int B": This will pop parent "A" because "A.scope_range" contains "int" from "int B"
            if (Long_Index_CtxCompare(ctx, kind == F4_Index_NoteKind_Decl) &&
                range_contains_inclusive(Long_Index_CtxScope(ctx), base.min))
            Long_Index_PopParent(ctx);
#endif
            
            DECLARATION:
            Long_Index_MakeNote(ctx, base, name, kind);
            
#if LONG_INDEX_PARSE_DECL
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
#else
            // NOTE(long): This check is for property's body (ex: int something => { ... } doesn't have a child scope)
            if (!range_size(base) || (!Long_Index_PeekPattern(ctx, "%t", "=>") && !Long_Index_PeekPattern(ctx, "%t", "{")))
            {
                Long_Index_BeginCtxChange(ctx);
                if (Long_Index_ParsePattern(ctx, "%t", "="))
                Long_CS_SkipExpression(ctx);
                Long_Index_BlockCtxScope(ctx,
                                         {
                                             // NOTE(long): 'out' is the only modifier that required a type
                                             // Ex: `in/ref/params myVar` vs `out MyType myVar`
                                             if (!use_modifier && Long_Index_ParsePattern(ctx, "%t", ","))
                                             Long_Index_ParsePattern(ctx, "%k", TokenBaseKind_Identifier, 0);
                                         });
                Long_Index_EndCtxChange(ctx);
            }
#endif
        }
        
        else if (Long_Index_ParsePattern(ctx, "%k", TokenBaseKind_Identifier, &name))
        {
#if LONG_INDEX_PARSE_DECL
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
#endif
            
            if (ctx->active_parent)
            {
                switch (ctx->active_parent->kind)
                {
#if !LONG_INDEX_PARSE_DECL
                    // NOTE(long): This is either for trailing declarations or fields of an enum
                    case F4_Index_NoteKind_Decl:
                    case F4_Index_NoteKind_Constant:
                    {
                        if (range_contains_inclusive(Long_Index_CtxScope(ctx), name.min))
                        {
                            base = ctx->active_parent->base_range;
                            kind = ctx->active_parent->kind;
                            Long_Index_PopParent(ctx);
                        }
                    } break;
#endif
                    
                    case F4_Index_NoteKind_Function: // NOTE(long): This is for lambda's arguments
                    {
                        if (range_size(ctx->active_parent->range) == 0 && Long_Index_CtxScope(ctx) == Range_i64{})
                        kind = F4_Index_NoteKind_Decl;
                    } break;
                    
                    case F4_Index_NoteKind_Type: // NOTE(long): This is for the first field of an enum
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
        
        else F4_Index_ParseCtx_Inc(ctx, F4_Index_TokenSkipFlag_SkipWhitespace|F4_Index_TokenSkipFlag_SkipComment);
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

function void RecursiveSuck(Application_Links* app, Text_Layout_ID layout, Token_Array* tokens, ARGB_Color color, F4_Index_Note* note)
{
    paint_text_color(app, layout, note->range, color);
    if (note->scope_range.min)
    {
        paint_text_color(app, layout, Ii64(token_from_pos(tokens, note->scope_range.min)), color);
        paint_text_color(app, layout, Ii64(token_from_pos(tokens, note->scope_range.max)), color);
    }
    for (F4_Index_Note* child = note->first_child; child; child = child->next_sibling)
    RecursiveSuck(app, layout, tokens, color, child);
}

//void name(Application_Links *app, Text_Layout_ID text_layout_id, Token_Array *array, Color_Table color_table)
internal F4_LANGUAGE_HIGHLIGHT(Long_CS_Highlight)
{
    Buffer_ID buffer = text_layout_get_buffer(app, text_layout_id); buffer;
#if 0
    {
        ARGB_Color color = F4_ARGBFromID(color_table, fleury_color_index_comment_tag, 0);
        paint_text_color(app, text_layout_id, buffer_range(app, buffer), F4_ARGBFromID(active_color_table, defcolor_text_default));
        for (F4_Index_Note* note = F4_Index_LookupFile(app, buffer)->first_note; note; note = note->next_sibling)
        RecursiveSuck(app, text_layout_id, array, color, note);
    }
#endif
    
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
                    if (note->range.max < note->parent->scope_range.min)
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

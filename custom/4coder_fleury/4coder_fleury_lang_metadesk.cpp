
struct F4_MD_LexerState
{
    String_Const_u8 string;
    u8 *at;
    u8 *one_past_last;
};

enum F4_MD_TokenSubKind
{
    F4_MD_TokenSubKind_Null,
    F4_MD_TokenSubKind_Tag,
};

internal F4_LANGUAGE_INDEXFILE(F4_MD_IndexFile)
{
    for(;!ctx->done;)
    {
        Token* name = 0;
        Token*  tag = 0;
        
        if (F4_Index_ParsePattern(ctx, "%b%o%k", F4_MD_TokenSubKind_Tag, &tag, TokenBaseKind_Identifier, &name))
        {
            String8 tag_str = F4_Index_StringFromToken(ctx, tag);
            if (0) { }
            
            else if (string_match(tag_str, S8Lit("@enum")) || string_match(tag_str, S8Lit("@union")) )
                F4_Index_MakeNote(ctx, Ii64(name), F4_Index_NoteKind_Type, F4_Index_NoteFlag_SumType);
            else if (string_match(tag_str, S8Lit("@text")) || string_match(tag_str, S8Lit("@embed")) || string_match(tag_str, S8Lit("@data")))
                F4_Index_MakeNote(ctx, Ii64(name), F4_Index_NoteKind_Decl, 0);
            else
                F4_Index_MakeNote(ctx, Ii64(name), F4_Index_NoteKind_Type, 0);
        }
        
        else if (F4_Index_RequireTokenKind(ctx, TokenBaseKind_Identifier, &name, F4_Index_TokenSkipFlag_SkipWhitespace))
        {
            if (F4_Index_RequireToken(ctx, S8Lit(":"), F4_Index_TokenSkipFlag_SkipWhitespace))
                F4_Index_MakeNote(ctx, Ii64(name), F4_Index_NoteKind_Type, 0);
        }
        else Long_ParseCtx_Inc(ctx);
    }
}

internal F4_LANGUAGE_LEXINIT(F4_MD_LexInit)
{
    F4_MD_LexerState *state = (F4_MD_LexerState *)state_ptr;
    state->string = contents;
    state->at = contents.str;
    state->one_past_last = contents.str + contents.size;
}

function Token Long_MD_LexStrLit(F4_MD_LexerState* state, String8 mark, i64 pos)
{
    i64 mark_size = mark.size;
    Token token = { pos, mark_size, TokenBaseKind_LiteralString, 0 };;
    for (i64 j = pos + mark_size; j + mark_size - 1 < (i64)state->string.size; j++, token.size++)
    {
        if (string_match(string_substring(state->string, Ii64_size(j, mark_size)), mark))
            break;
        
        if (state->string.str[j] == '\n' && mark_size == 1)
        {
            token.kind = TokenBaseKind_LexError;
            break;
        }
        
        if (state->string.str[j] == '\\')
        {
            j++;
            token.size++;
        }
    }
    
    token.size += mark_size;
    return token;
}

internal F4_LANGUAGE_LEXFULLINPUT(F4_MD_LexFullInput)
{
    b32 result = false;
    F4_MD_LexerState state_ = *(F4_MD_LexerState *)state_ptr;
    F4_MD_LexerState *state = &state_;
    u64 emit_counter = 0;
    i64 strmax = (i64)state->string.size;
    for(i64 i = (i64)(state->at - state->string.str);
        i < strmax && state->at < state->one_past_last;)
    {
        i64 start_i = i;
        u8 chr = state->string.str[i];
        
        // NOTE(rjf): Comments
        if(i+1 < strmax &&
           state->string.str[i] == '/' &&
           state->string.str[i+1] == '/')
        {
            Token token = { i, 1, TokenBaseKind_Comment, 0 };
            token.size += 1;
            for(i64 j = i+2; j < strmax && state->string.str[j] != '\n'; j += 1, token.size += 1);
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Comments
        else if(i+1 < strmax &&
                state->string.str[i] == '/' &&
                state->string.str[i+1] == '*')
        {
            Token token = { i, 1, TokenBaseKind_Comment, 0 };
            token.size += 1;
            for (i64 j = i+2, comment_count = 1; j+1 < strmax && comment_count; j += 1, token.size += 1)
            {
                if (state->string.str[j] == '/' && state->string.str[j+1] == '*')
                    comment_count++;
                if (state->string.str[j] == '*' && state->string.str[j+1] == '/')
                    comment_count--;
            }
            token.size += 2;
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Identifier
        else if(character_is_alpha(chr))
        {
            Token token = { i, 1, TokenBaseKind_Identifier, 0 };
            for(i64 j = i+1; j < (i64)state->string.size && 
                (character_is_alpha_numeric(state->string.str[j]) ||
                 state->string.str[j] == '_');
                j += 1, token.size += 1);
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Whitespace
        else if(character_is_whitespace(chr))
        {
            Token token = { i, 1, TokenBaseKind_Whitespace, 0 };
            for(i64 j = i+1; j < (i64)state->string.size && 
                character_is_whitespace(state->string.str[j]);
                j += 1, token.size += 1);
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Numeric Literal
        else if(chr >= '0' && chr <= '9')
        {
            Token token = { i, 1, TokenBaseKind_LiteralFloat, 0 };
            for(i64 j = i+1; j < (i64)state->string.size && 
                (character_is_alpha_numeric(state->string.str[j]) ||
                 state->string.str[j] == '_' ||
                 state->string.str[j] == '.');
                j += 1, token.size += 1);
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Multi-line String Literal
        else if(i+2 < strmax &&
                state->string.str[i]   == '"' &&
                state->string.str[i+1] == '"' &&
                state->string.str[i+2] == '"')
        {
            Token token = Long_MD_LexStrLit(state, S8Lit("\"\"\""), i);
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Multi-Line String Literal Marker (Bundle-Of-Tokens)
        else if(i+2 < strmax &&
                state->string.str[i]   == '`' &&
                state->string.str[i+1] == '`' &&
                state->string.str[i+2] == '`')
        {
            Token token = Long_MD_LexStrLit(state, S8Lit("```"), i);
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Multi-line Char Literal
        else if(i+2 < strmax &&
                state->string.str[i]   == '\'' &&
                state->string.str[i+1] == '\'' &&
                state->string.str[i+2] == '\'')
        {
            Token token = Long_MD_LexStrLit(state, S8Lit("'''"), i);
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Single-Line String Literal
        else if(chr == '"')
        {
            Token token = Long_MD_LexStrLit(state, S8Lit("\""), i);
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Single-Line String Literal Marker (Bundle-Of-Tokens)
        else if(chr == '`')
        {
            Token token = Long_MD_LexStrLit(state, S8Lit("`"), i);
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Single-Line Char Literal
        else if(chr == '\'')
        {
            Token token = Long_MD_LexStrLit(state, S8Lit("'"), i);
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Tags
        else if(chr == '@')
        {
            Token token = { i, 1, TokenBaseKind_Identifier, 0 };
            token.sub_kind = F4_MD_TokenSubKind_Tag;
            for(i64 j = i+1; j < (i64)state->string.size && 
                (character_is_alpha_numeric(state->string.str[j]) ||
                 state->string.str[j] == '_');
                j += 1, token.size += 1);
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Scope-Open
        else if(chr == '{')
        {
            Token token = { i, 1, TokenBaseKind_ScopeOpen, 0 };
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Scope-Close
        else if(chr == '}')
        {
            Token token = { i, 1, TokenBaseKind_ScopeClose, 0 };
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Paren-Open
        else if(chr == '(' || chr == '[')
        {
            Token token = { i, 1, TokenBaseKind_ParentheticalOpen, 0 };
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Scope-Close
        else if(chr == ')' || chr == ']')
        {
            Token token = { i, 1, TokenBaseKind_ParentheticalClose, 0 };
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Statement closes
        else if(chr == ',' || chr == ';' || (chr == '-' && i+1 < strmax && state->string.str[i+1] == '>'))
        {
            Token token = { i, 1, TokenBaseKind_StatementClose, 0 };
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Operators
        else if(chr == '~' || chr == '!' || chr == '@' || chr == '#' || chr == '$' ||
                chr == '%' || chr == '^' || chr == '&' || chr == '*' || chr == '(' ||
                chr == ')' || chr == '-' || chr == '=' || chr == '+' || chr == '[' ||
                chr == ']' || chr == '{' || chr == '}' || chr == ':' || chr == ';' ||
                chr == ',' || chr == '<' || chr == '.' || chr == '>' || chr == '/' ||
                chr == '?' || chr == '|' || chr == '\\')
        {
            Token token = { i, 1, TokenBaseKind_Operator, 0 };
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Catch-All
        else
        {
            Token token = {i, 1, TokenBaseKind_LexError, 0 };
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        if(state->at >= state->one_past_last)
        {
            goto eof;
        }
        else if(start_i == i)
        {
            i += 1;
            state->at = state->string.str + i;
        }
        else
        {
            state->at = state->string.str + i;
            emit_counter += 1;
            if(emit_counter >= max)
            {
                goto end;
            }
            
            if (state->at >= state->one_past_last)
            {
                if (list->last)
                {
                    Token* last = list->last->tokens + list->last->count - 1;
                    last->size = state->string.size - last->pos;
                }
            }
        }
    }
    
    // NOTE(rjf): Add EOF
    eof:;
    {
        Token* prev = 0;
        for (Token_Block* block = list->first; block; block = block->next)
        {
            for (Token* token = block->tokens; token < block->tokens + (u64)block->count; prev = token++)
            {
                Assert(token->size > 0);
                if (prev)
                {
                    Assert(token->pos > 0);
                    Assert(prev->pos + prev->size == token->pos);
                }
                
                if (block == list->last && token == block->tokens + (u64)block->count - 1)
                    Assert(state->string.size == token->pos + (u64)token->size);
            }
        }
        
        result = true;
        Token token = { (i64)state->string.size, 1, TokenBaseKind_EOF, 0 };
        token_list_push(arena, list, &token);
    }
    
    end:;
    *(F4_MD_LexerState *)state_ptr = *state;
    
    return result;
}

internal F4_LANGUAGE_POSCONTEXT(F4_MD_PosContext)
{
    return 0;
}

function Range_i64 Long_MD_ParseStrExpr(Application_Links* app, Buffer_ID buffer, Range_i64 range)
{
    Long_Index_ProfileScope(app, "[Long] Parse MD Expr");
    Scratch_Block scratch(app);
    Range_i64 result = {};
    
    String8 expr_str = S8Lit("$(");
    String8 str = push_buffer_range(app, scratch, buffer, range);
    
    for (i64 expr_pos = 0, size = range.max - range.min; expr_pos < size; ++expr_pos)
    {
        String8 substr = string_substring(str, Ii64_size(expr_pos, expr_str.size));
        if (string_match(substr, expr_str))
        {
            u64 i = expr_pos + expr_str.size;
            
            for (i64 paren = 1; i < str.size && paren; ++i)
            {
                switch (str.str[i])
                {
                    case '(': paren++; break;
                    case ')': paren--; break;
                }
            }
            
            result = Ii64(expr_pos+range.min, i+range.min);
            break;
        }
    }
    
    return result;
}

internal F4_LANGUAGE_HIGHLIGHT(F4_MD_Highlight)
{
    Buffer_ID buffer = text_layout_get_buffer(app, text_layout_id);
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    Token_Iterator_Array it = token_iterator_pos(0, array, visible_range.min);
    ARGB_Color tag_color = F4_ARGBFromID(color_table, defcolor_keyword, 0);
    
    while (1)
    {
        Token *token = token_it_read(&it);
        if(!token || token->pos >= visible_range.one_past_last)
            break;
        
        if(token->sub_kind == F4_MD_TokenSubKind_Tag)
            paint_text_color(app, text_layout_id, Ii64(token), tag_color);
        
        if (token->kind == TokenBaseKind_LiteralString)
        {
            for (Range_i64 range = Ii64(token); range_size(range) > 3; )
            {
                Range_i64 expr_range = Long_MD_ParseStrExpr(app, buffer, range);
                if (!range_size(expr_range)) break;
                
                paint_text_color(app, text_layout_id, expr_range, F4_ARGBFromID(color_table, fleury_color_operators, 0));
                range.min = expr_range.max;
                
                Scratch_Block scratch(app);
                String8 tag = push_buffer_range(app, scratch, buffer, expr_range);
                u64 tag_pos = 0;
                for (u64 i = 0; i < tag.size; ++i)
                {
                    if (tag_pos && !character_is_alpha(tag.str[i]))
                    {
                        paint_text_color(app, text_layout_id, Ii64(tag_pos, i) + expr_range.min, tag_color);
                        tag_pos = 0;
                    }
                    
                    if (tag.str[i] == '@')
                        tag_pos = i;
                }
            }
        }
        
        if(!token_it_inc_all(&it))
            break;
    }
}
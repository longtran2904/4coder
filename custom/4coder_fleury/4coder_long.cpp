/*date = January 12th 2023 5:22 pm */

#ifndef FCODER_LONG_H
#define FCODER_LONG_H

//~ @readme Tran Thanh Long's 4coder Custom Layer

// In general, I really want to structure my code into a bunch of isolated drop-in files without changing
// any code from the base and fleury's layer. But it would mean that I need to copy a lot of functions to
// modify them slightly (I did do some of it, search for all the COPYPASTA comments). There are also multiple
// bugs in the original layers that I fixed. That's why a lot of code still depends on these modified files:

//- @long_index_files

// @important
// 4coder_fleury_index.h: Add additional fields for types and enums
// 4coder_fleury_index.cpp:
// - F4_Index_InsertNote: I was using the built-in profiler in 4coder and noticed something weird in this
//   function. It does some alloc, hash lookups, and string comparisons, but most of the time was spent on
//   this seemingly innocent line:
//
//     for (F4_Index_Note *note = list_tail; note; list_tail = note, note = note->next);
//
//   This line tries to find the linked list's tail note, and then later append a new note to it.
//   But there was no reason to do so, you can insert the note at the start of the list because the order here
//   doesn't matter. So just remove this, and you improve the speed quite a lot.
//
// - F4_Index_ParseCtx_Inc: Handle the SkipComment flag
// - F4_Index_ClearFile: Reset the file->references to zero

// 4coder_code_index.cpp: generic_parse_statement, generic_parse_paren, layout_index_x_shift

// 4coder_fleury_colors.cpp:
// - Add Token_Array to ColorCtx
// - Call Long_Index_LookupBestNote rather than F4_Index_LookupNote in F4_GetColor

// 4coder_fleury_lang.h: Add more fields to F4_Language_PosContextData

// @optional
// 4coder_fleury_lang_list.h: Add C# to the language list
// 4coder_fleury_lang_cpp.cpp: Modify the F4_CPP_PosContext hook to work with my new position-context system
// (I should just write a new cpp parser at some point)

//- @4coder_lister_base
// The entire Lister system is a drop-in file, to use it just call Long_Lister_Run rather than run_lister.
// Most of the default commands call run_lister so "upgrading" all those commands to the new system requires
// a lot of work. I choose the simplest way of #define run_lister to Long_Lister_Run.

//~ @features 4coder Long Feature/Option List

//- @long_point_stack_intro

// 4coder provides a very basic point stack framework where commands can push new points to a single global stack.
// Commands that push are jump_to_definition(_at_cursor) and f4_search_for_definition__xxx.
// The only command that pops is jump_to_last_point.

// This layer implements a completely new and separate framework that is used in most new jump/goto commands
// - Rather than a single global stack, each view now has its own stack.
// - You can iterate over all the points or get a point at a specific index.
// - Each stack also has a current, top, and bottom point that you can read and modify.

// Some notable commands:
// - long_undo/redo_jump: jumps to the current point or the prev/next point if already there
// - long_push_new_jump: pushes the current position to the stack if the current point isn't already it

//- @long_history_intro

// 4coder never reset the dirty state of a buffer, except when saving it.
// This has been changed so that if you undo/redo a buffer to a saved state,
// It will set the buffer's dirty state to up-to-date

// Some notable commands:
// - Long_SaveFile: This is the new SaveFile hook that changes the current saved state for the buffer
// - long_undo/redo(_all_buffers): the same as their counterpart, but use the new history system
// - long_undo/redo_same_pos: undo/redo but doesn't change the cursor and camera position

//- @long_index_intro

// This is my biggest customization layer yet. It contains multiple helper functions for writing a parser,
// and using/lookup the generated index note. Originally, It started as an extended version for the fleury's
// index system. But after adding and writing most the code by hand, it has grown bigger and more complete
// than the fleury's one. I also have modified multiple fleury's files, so it's probably better to just
// completely rewrite and make it a simple drop-in file. For now, a short list of all the noteworthy features:

// - The Lookup Function
//   Long_Index_LookupBestNote is the most important function in this layer. This function will parse a series
//   of "selective path" and lookup the best-matched note. It has a proper handling for name collision and
//   local/nested notes. It'll search the declaration type, return type, the inheritance tree, or the imported
//   namespaces for a match. It also understands that 'this' means search in the base type, 'new' means searching
//   for constructor, and `new Type {...}` means object initializer in C# (hasn't implementd for in C/C++ yet).
//   All the index commands and render functions use this, including but not limited to: Long_Index_IndentBuffer,
//   Long_GoToDefinition, Long_SearchDefinition, Long_Scan_Note, Long_Index_DrawPosContext, and F4_GetColor.

// - The C# Parser
//   This is a mostly complete parser. It can parse (using) namespaces; classes, structs, enums, and tuples;
//   functions and function-like features like constructors, lambdas, getters/setters, and operators;
//   local/global declarations, type's fields, and function's arguments; generic type arguments and inheritance.
//   You can take a look at it in 4coder_long_lang_cs.cpp and 4coder_cs_lexer_test.cs for more details.

// - The Indent Function
//   Long_Index_IndentBuffer uses Code_Index_Nest just like the virtual whitespace system, so now anytime you
//   change the way the parser works, both of these will be updated correctly.
//   All the new indenting commands are just the same as their counterpart but use this new indenting system.

// - The Render System
//
// 1. Long_Index_DrawPosContext
//   Unlike F4_PosContext_Render, it will draw a function's return type, the variable's declaration type, and a type's
//   keyword (class, struct, enum, etc). When displaying a type, it will split all the type's members into 3
//   categories: TYPES, FUNCS, and DECLS, and display the number of entries for each one. You can cycle
//   between each category by calling the command long_switch_pos_context_option. Calling long_toggle_pos_context
//   for toggling the commands, and long_switch_pos_context_draw_position for changing the tooltip position.
//
// 2. Long_Index_DrawCodePeek
//   This is the same as F4_CodePeek_Render, but uses F4_Index_Note rather than Code_Index_Note and
//   calls Long_Index_LookupBestNote. It will prioritize def notes over prototype ones.

// - FINAL NOTE -
// When I first wrote this system, I had some experience parsing and modifying AST nodes. After reading
// how Ryan did his parser and indexer, I've learned a lot of new and useful techniques. Since then,
// I've been experimenting with my other projects (including this C# parser). If I have the time and
// energy in the future, I'll write a more detailed comment about the way this C# parser works and how
// I think about parsing now.

//- @long_search_intro

// All the new search, query, and list commands now use an improved query bar input handler:
// - Ctrl+E to center the view
// - Ctrl+V to append content from the clipboard to the bar's string
// - Ctrl+Backspace now deletes all the contents inside the query bar
// - Esc will abort and jump back to the old position before querying started
// - Shift+Esc will abort but don't reset the current position
//   (The Multi-Select system will return to the normal jump's highlight system)

// The current way I implement the new query bar is by copy-paste the same code, over and over again.
// The better way is to have a Long_QueryBar_HandleDefaultInput function that will handle all the basic
// input and return unhandled input to the caller.

// - Long_ISearch: Same as isearch, but with the ability to handle case-sensitive string
// - Long_Query_Replace: Same thing as Long_ISearch, but for query_replace
//   The user can now seek the next and previous match, and choose which one to replace
//   Should probably merge this with Long_ISearch
// - Long_ListAllLocations: list_all_locations__generic but can localize the search to the current buffer
//   It also remembers the current buffer and position before the search, so it can jump back when aborting
//   It also uses the new Multi-Select system, read @long_multi_select_intro for more info
// - Long_ListAllLines_InRange: Lists all the lines in the current buffer that satisfy a predicate function
//   It also uses the Multi-Select system like Long_ListAllLocations
//   I implemented this as a replacement for Alt+Shift in Visual Studio, but it's not as good as I wanted
//   because the Multi-Select system is still very basic

//- @long_lister_intro

// In general, I think the API design of 4coder is good but has several weaknesses. You can see some of
// them in the default lister wrapper API. In the future, I may write a more detailed comments about this.
// For now, here's a short list of all the improved features:
// - Long_Lister_Render:
//   - Show the number of items and the current index in the query bar
//   - Each item can have an optional header that is its location
//   - Each entry can now have a tooltip (document for commands, code peek for notes, etc)
// - Long_Lister_GetFilter:
//   - Filter tags (inclusive and exclusive): Originally, I added this feature so that I don't need
//     specific commands to list all functions or types, but this has become more useful than I thought
//     because now you can search for keybindings in the command lister and unique file path in the buffer
//     lister because those are just tags.
//   - Filter header (only inclusive right now)
// - Long_Lister_HandleKeyStroke:
//   - Ctrl+C to copy from and Ctrl+V to paste to the query bar
//   - Ctrl+Tick for toggling the current item's tooltip
//   - Alt+Tick for switching between different tooltip modes
// - Custom handlers:
//   - The default lister layer has multiple callback functions called handlers. I added multiple custom
//     handlers that work with the new filtered tags. Most of the new handlers are just the original but
//     call Long_Lister_FilterList rather than lister_update_filtered_list.
//   - Long_Lister_Backspace adds the ability to delete the entire text field with Ctrl+Backspace

// @important
// The new lister uses an internal data type (Long_Lister_Data) which is added by using Long_Lister_AddItem.
// The function will always append the extra data right after the allocated item and assign the user_data
// field to it (which means `node->user_data == node + 1`). If your custom code uses the same strategy
// then there will be conflict so keep it in mind.

// Fixed bugs:
// - Overdraw bug: https://github.com/4coder-community/4cc/commit/dc692dc0d4b4a125475573ebfcc07cfd60a85bb7
// - Render the wrong file bar: lister_render calls draw_file_bar rather than F4_DrawFileBar

// Index note tags (defined in Long_Note_PushTag)
// - declaration, constant, comment tag, TODO
// - Function tags: function, operator, lambda, constructor, getter, setter, [forward]
// - Type tags: type, namespace
// - Type optional tags: [forward], [union], [generic]
// - Declaration optional tags: [argument], [global], [local], [field], [property]

// Some notable listers:
// - long_history_lister: List all the edits to the buffer, undo or redo the selected entry
// - long_point_lister: List all the stored points in the current view's stack, jump to the selected one
// - long_jump_lister: view_jump_list_with_lister but uses the new lister
// - long_theme_lister: theme_lister but uses the new lister and hot theme reloading
// - long_interactive_switch_buffer: the default one but uses the new lister and pushes new points to the stack

//- @long_move_intro

// These commands use fleury's index or 4coder's code index layer to move to the next target
// - long_move_to_next/prev_function_and_type: moves to the next/prev function or type in the current buffer
//   It uses Long_Filter_FunctionAndType to filter out lambda functions and generic argument
// - long_move_up/down_token_occurrence: uses F4_Boundary_CursorToken from jack (link at the function)

// These commands move around by scanning for the surrounding scope like 4coder_scope_commands
// - long_select_prev/next_scope_current_level: if a scope is already selected, find its prev/next sibling.
//   Otherwise, find the first scope that starts before the cursor.
// - long_select_upper_scope: selects the upper/parent scope (this's also called the surrounding scope).
//   It's the same as select_surrounding_scope, just with a different name to match other scope commands.
// - long_select_lower_scope: finds the first scope that starts inside the current selected or surrounding scope.
//   I should change this so that it finds the nearest child scope instead of the first one.
// - long_select_surrounding_scope: selects the surrounding scope. If a scope is already selected,
//   switch between having the cursor and mark outside or inside that scope.

//- @long_multi_select_intro

// Long_SearchBuffer_MultiSelect is a makeshift function for implementing a multi-cursor-like system.
// It works by rendering the jump buffer with a custom render function (Long_Highlight_DrawList) that
// will draw a notepad-like cursor at each sticky jump location and have a custom navigation code to
// move the highlighted range around.

// Up/Down will move to the prev/next location and hold down shift while doing so to select multiple locations
// Left/Right to change the current selection size, and move all the cursors around
// Esc will undo all the changes and jump back to the old position, while Enter will commit all the edits
// Shift+Esc still keeps the current position and exits back to the normal jump highlighting code

// This is a quick and dirty way for stuff to work, so one of the main weaknesses is that you can't run any
// custom commands, which means you can't use basic navigation like move_up/down, copy/cut/paste, mark
// highlighting, etc. Each cursor can't be moved in arbitrary ways or has its own mark, and any new commands
// you want must be implemented in the Long_SearchBuffer_MultiSelect function.

// A better way to implement this is to parse the *keyboard* buffer like the macro system and store each cursor
// and mark position in a marker array. If you don't need cursors to be in multiple buffers, and ok with a
// notepad-like design for each cursor (meaning you can't have arbitrary mark position), then this's probably
// the best way to do it.

//- @long_commands

// - Long_KillBuffer: When you kill a buffer in the original command, 4coder will switch to the most recent buffer.
//   It's not that bad because killing a buffer isn't common. But what's common is killing the *search* buffer, and
//   it's very annoying when after a quick search the buffer that you get back isn't the one before the search.
//   This function will first check if the buffer you want to kill is the *search* buffer, then jump back to the
//   correct position afterward.
//   This is used by long_interactive_kill_buffer, long_kill_buffer, and long_kill_search_buffer.

// - long_select_current_line: This will move the cursor to the end of the current line and the mark to the
//   first non-whitespace character on the current line or the end of the previous line if it's already there.

// - long_reload_all_themes_default_folder: I added this command while customizing my theme file.
//   It will clear all the loaded themes and reload all the files in the default theme folder.
//   Then it will try to find the theme that was being used and select it. It'll do it by calling
//   Long_UpdateCurrentTheme in the theme lister and on startup.

// - long_load_project: I modified this command to work with the new reference library concept.
//   Each project file can now contain an array of reference paths (set inside the reference_paths variable)
//   When this command runs, it will recursively load all files in those paths as read-only and unimportant

//~ TODO MULTI CURSOR
// [ ] Across buffers
// [X] Add/Remove a cursor at the current location while in search mode
// [ ] Begin multi-cursor block that ignore comment lines
// [X] Remove all the old MultiSelect code
// [ ] Correct MC cursors while enabling virtual whitespace

//~ TODO QUERY/REPLACE
// [X] Compress the input code
// [X] Refactor isearch and query replace
// [X] Ctrl+Enter will append previous search query
// [X] Handle number input
// [X] Fade the replaced range when query replace
// [X] Fix error fading after replace the last match
// [X] Finish query replace after the final match
// [X] Interactively update the query highlights while typing when replace_in_range
// [X] Seamlessly switch between isearch and query replace
// [X] Fix incorrect starting position when Shift+Esc

//~ TODO TAB
// [ ] Basic system and commands/bindings
// [ ] Put *message* buffer to a separate tab
// [ ] Put  *search* buffer to a separate tab

//~ TODO INDEX

//- CODE
// [ ] Write a new cpp parser
// [ ] Rewrite the Index system into a simple drop-in file
// [ ] Add Index API for customizing the indentation and poscontext
// [ ] Handle function overloading

//- RENDER
// [ ] Autocompletion using PosContext or casey/jack's system
// [ ] String/Comment indenting as code
// [ ] Render #if block with annotation
// [ ] Render hex/bin/oct/dec number with thousands/byte separator

//- BUGS
// [ ] Fix global function macro color for variable
// [ ] `MyStruct arg = {}` in the argument list cause prototype function to be parsed incorectly

//~ TODO BUGS
// [ ] Fix undo/redo_all_buffers right after saving bug
// [ ] The current saved history gets overwritten by merging with the next modification
// [ ] Clipboard bug (again)
// [X] Fix hot-reload mc bindings bug
// [X] Fix ListAllLocations highlights the current range in *compilation*
// [X] Pass buffer id rather than the view id to Long_Mapping_GetMap in MC_ListAllLocations
// [X] Off-by-one selection count in MC_ListAllLocations

//~ @CONSIDER NEW SYSTEM
// [ ] LOC counter
// [ ] Virtual column

//~ @CONSIDER LISTER

//- SEARCH
// [ ] Replace all the wildcard searching in the query bar and lister with grep or glob
// [ ] Search for definitions like Hoogle
// [ ] Has a lister for important but rarely used commands
// [ ] Hotkeys for inserting and cycling through common tags

//- RENDER
// [ ] Put ... after a large item and scroll its content horizontally over time
// [ ] Has syntax highlight inside each item's contents
// [ ] Improve relative path display in the file lister
// [ ] Render multi-column grid like byp_qol

//~ @CONSIDER Interesting but low-priority stuff that may or may not be useful but deserves a look

//- IDENTIFIER (when the cursor is at then end of the identifier token)
// [ ] Search
// [ ] Replace
// [ ] List
// [ ] Peek
// [ ] Long_Boundary_CursorToken
// [ ] Token Occurrence Highlight

//- HOT RELOADING
// [ ] Hot-reload the project file and print the error message
// [ ] Hot-reload multi-cursor bindings
// [ ] Custom commands for each reloadable file

//- MISC
// [ ] Toggle comments ignore blank lines
// [ ] Move range selection up and down
// [ ] Modal auto-complete {} () [] on enter or typing
// [ ] Jump to location with relative path
// [ ] Code peek yank

//~ NOTE(long): @long_macros and default include
#define LONG_INDEX_INDENT_STATEMENT 1
#define LONG_INDEX_INLINE 1
#define LONG_INDEX_INSERT_QUEUE 1
#define LONG_INDEX_INDENT_PAREN 0

#define LONG_LISTER_OVERLOAD 1

#define LONG_ENABLE_INDEX_PROFILE 1
#define LONG_ENABLE_PROFILE 0

#if LONG_ENABLE_INDEX_PROFILE
#define Long_Index_ProfileScope(T, N) ProfileScope(T, N)
#define Long_Index_ProfileBlock(T, N) ProfileBlock(T, N)
#else
#define Long_Index_ProfileScope(...)
#define Long_Index_ProfileBlock(...)
#endif

//~ NOTE(rjf): Macros and pragmase stuff that have to be put here for various reasons
#include <string.h>
#include "4coder_default_include.cpp"
#pragma warning(disable : 4706)

//~ NOTE(rjf): @f4_headers
#include "4coder_fleury_ubiquitous.h"
#include "4coder_fleury_lang.h"
#include "4coder_fleury_index.h"
#include "4coder_fleury_colors.h"
#include "4coder_fleury_render_helpers.h"

//~ NOTE(long): @f4_optional_headers
#include "4coder_fleury_brace.h"
#include "4coder_fleury_recent_files.h"
#include "4coder_fleury_plot.h"
#include "4coder_fleury_calc.h"

//~ NOTE(long): @long_headers
#include "4coder_long_hooks.h"
#include "4coder_long_index.h"
#include "4coder_long_base_commands.h"
#include "4coder_long_lister.h"
#include "4coder_long_render.h"
#include "4coder_long_bindings.h"

#pragma warning(disable : 4456)

//~ NOTE(long): @byp_src
#include "plugins/4coder_multi_cursor.cpp"

//~ NOTE(rjf): @f4_src
#include "4coder_fleury_ubiquitous.cpp"
#include "4coder_fleury_lang.cpp"
#include "4coder_fleury_index.cpp"
#include "4coder_fleury_colors.cpp"
#include "4coder_fleury_render_helpers.cpp"

//~ NOTE(long): @f4_optional_src
#include "4coder_fleury_brace.cpp"
#include "4coder_fleury_recent_files.cpp"
#include "4coder_fleury_plot.cpp"
#include "4coder_fleury_calc.cpp"

#pragma warning(default : 4456)

//~ NOTE(long): @long_src
#include "4coder_long_hooks.cpp"
#include "4coder_long_index.cpp"
#include "4coder_long_base_commands.cpp"
#include "4coder_long_lister.cpp"
#include "4coder_long_render.cpp"
#include "4coder_long_bindings.cpp"

#if LONG_ENABLE_PROFILE
#include "4coder_profile_static_enable.cpp"
#else
#include "4coder_profile_static_disable.cpp"
#endif

//~ NOTE(rjf): 4coder Stuff
#include "generated/managed_id_metadata.cpp"

//~ NOTE(long): @long_custom_layer_initialization

void custom_layer_init(Application_Links* app)
{
    default_framework_init(app);
    MC_init(app);
    implicit_map_function = 0;
    global_frame_arena = make_arena(get_base_allocator_system());
    permanent_arena = make_arena(get_base_allocator_system());
    
    // NOTE(long): Set up hooks.
    {
        set_all_default_hooks(app);
        
        set_custom_hook(app, HookID_Tick,             Long_Tick);
        set_custom_hook(app, HookID_RenderCaller,     Long_Render);
        set_custom_hook(app, HookID_ViewEventHandler, long_view_input_handler);
        
        set_custom_hook(app, HookID_BeginBuffer,      Long_BeginBuffer);
        set_custom_hook(app, HookID_EndBuffer,        Long_EndBuffer);
        set_custom_hook(app, HookID_SaveFile,         Long_SaveFile);
        set_custom_hook(app, HookID_BufferEditRange,  Long_BufferEditRange);
        
        set_custom_hook(app, HookID_Layout,           Long_Layout);
        set_custom_hook(app, HookID_DeltaRule,        Long_DeltaRule);
        set_custom_hook_memory_size(app, HookID_DeltaRule, delta_ctx_size(sizeof(Vec2_f32)));
    }
    
    // NOTE(long): Set up bindings.
    {
        Thread_Context* tctx = get_thread_context(app);
        mapping_init(tctx, &framework_mapping);
        String_Const_u8 bindings_file = S8Lit("bindings.4coder");
        
        String_ID global_id = vars_save_string_lit("keys_global");
        String_ID   file_id = vars_save_string_lit("keys_file");
        String_ID   code_id = vars_save_string_lit("keys_code");
        
        if (!dynamic_binding_load_from_file(app, &framework_mapping, bindings_file))
            Long_Binding_SetupDefault(&framework_mapping, global_id, file_id, code_id);
        Long_Binding_SetupEssential(&framework_mapping, global_id, file_id, code_id);
        Long_Binding_MultiCursor();
    }
    
    // TODO(long): Improve the index and language layers.
    F4_Index_Initialize();
    F4_RegisterLanguages();
}

CUSTOM_COMMAND_SIG(long_startup)
CUSTOM_DOC("Long startup event")
{
    ProfileScope(app, "default startup");
    Scratch_Block scratch(app);
    
    User_Input input = get_current_input(app);
    if (!match_core_code(&input, CoreCode_Startup))
        return;
    
    // NOTE(long): default font.
    Face_ID default_font = get_face_id(app, 0);
    
    //- @COPYPASTA(long): default_4coder_initialize
    {
        load_themes_default_folder(app);
        
        // TODO(long): Better default message
#define M \
    "Welcome to " VERSION "\n" \
    "If you're new to 4coder there is a built in tutorial\n" \
    "Use the key combination [ X Alt ] (on mac [ X Control ])\n" \
    "Type in 'hms_demo_tutorial' and press enter\n" \
    "\n" \
    "Direct bug reports and feature requests to https://github.com/4coder-editor/4coder/issues\n" \
    "\n" \
    "Other questions and discussion can be directed to editor@4coder.net or 4coder.handmade.network\n" \
    "\n" \
    "The change log can be found in CHANGES.txt\n" \
    "\n"
        print_message(app, S8Lit(M));
#undef M
        
        // config loading
        {
            Face_Description description = get_face_description(app, 0);
            load_config_and_apply(app, &global_config_arena, description.parameters.pt_size, description.parameters.hinting);
            String8 font_name = get_global_face_description(app).font.file_name;
            Long_Print_Messagef(app, "Default Font: %.*s\n\n", string_expand(font_name));
        }
        
        // open command line files
        String_Const_u8 hot = push_hot_directory(app, scratch);
        String8Array files = input.event.core.file_names;
        
        for (i32 i = 0; i < files.count; i += 1)
        {
            Temp_Memory_Block temp(scratch);
            String8 filename = files.vals[i];
            String8 fullname = push_u8_stringf(scratch, "%.*s/%.*s", string_expand(hot), string_expand(filename));
            
            Buffer_ID new_buffer = create_buffer(app, fullname, BufferCreate_NeverNew|BufferCreate_MustAttachToFile);
            if (new_buffer == 0)
                create_buffer(app, filename, 0);
        }
    }
    
    //- NOTE(rjf): Open special buffers.
    Buffer_ID comp_buffer, left_buffer, right_buffer;
    {
        Buffer_Create_Flag special_flags = BufferCreate_NeverAttachToFile|BufferCreate_AlwaysNew;
        
        comp_buffer = create_buffer(app, S8Lit("*compilation*"), special_flags);
        buffer_set_setting(app, comp_buffer, BufferSetting_Unimportant, true);
        buffer_set_setting(app, comp_buffer, BufferSetting_ReadOnly, true);
        buffer_set_setting(app, comp_buffer, BufferSetting_Unkillable, true);
        
        Buffer_ID lego_buffer = create_buffer(app, S8Lit("*lego*"), special_flags);
        buffer_set_setting(app, lego_buffer, BufferSetting_Unimportant, true);
        buffer_set_setting(app, lego_buffer, BufferSetting_ReadOnly, true);
        
        Buffer_ID calc_buffer = create_buffer(app, S8Lit("*calc*"), special_flags);
        buffer_set_setting(app, calc_buffer, BufferSetting_Unimportant, true);
        
        left_buffer  = get_buffer_by_name(app, S8Lit( "*scratch*"), 0);
        right_buffer = get_buffer_by_name(app, S8Lit("*messages*"), 0);
    }
    
    //- NOTE(rjf): Initialize panels
    {
        // NOTE(rjf): Left Panel
        View_ID view = get_active_view(app, Access_Always);
        new_view_settings(app, view);
        view_set_buffer(app, view, left_buffer, 0);
        
        // NOTE(rjf): Bottom panel
        View_ID compilation_view = 0;
        {
            compilation_view = open_view(app, view, ViewSplit_Bottom);
            new_view_settings(app, compilation_view);
            Buffer_ID buffer = view_get_buffer(app, compilation_view, Access_Always);
            Face_Metrics metrics = get_face_metrics(app, default_font);
            view_set_split_pixel_size(app, compilation_view, (i32)(metrics.line_height*4.f));
            view_set_passive(app, compilation_view, true);
            global_compilation_view = compilation_view;
            view_set_buffer(app, compilation_view, comp_buffer, 0);
        }
        
        view_set_active(app, view);
        
        // NOTE(rjf): Right Panel
        open_panel_vsplit(app);
        View_ID right_view = get_active_view(app, Access_Always);
        view_set_buffer(app, right_view, right_buffer, 0);
        
        // NOTE(rjf): Restore Active to Left
        view_set_active(app, view);
    }
    
    //- NOTE(rjf): Auto-Load Project.
    {
        b32 auto_load = def_get_config_b32(vars_save_string_lit("automatically_load_project"));
        if (auto_load)
            long_load_project(app);
    }
    
    //- NOTE(long): Open most recent modified files on startup
    {
        u64 last_write[2] = {};
        Buffer_ID recent_buffers[2];
        
        String8 treat_as_code_string = def_get_config_string(scratch, vars_save_string_lit("treat_as_code"));
        String8Array extensions = parse_extension_line_to_extension_list(app, scratch, treat_as_code_string);
        
        for (Buffer_ID buffer = get_buffer_next(app, 0, 0); buffer; buffer = get_buffer_next(app, buffer, 0))
        {
            Temp_Memory_Block temp(scratch);
            String8 filename = push_buffer_file_name(app, scratch, buffer);
            
            i64 unimportant = 0, readonly = 0, is_code_file = 0;
            buffer_get_setting(app, buffer, BufferSetting_Unimportant, &unimportant);
            buffer_get_setting(app, buffer, BufferSetting_ReadOnly, &readonly);
            is_code_file = Long_Index_IsMatch(string_file_extension(filename), extensions.strings, extensions.count);
            
            if (filename.size && !unimportant && !readonly && is_code_file)
            {
                File_Attributes att = system_quick_file_attributes(scratch, filename);
                if (att.last_write_time > last_write[0])
                {
                    last_write[1] = last_write[0];
                    recent_buffers[1] = recent_buffers[0];
                    
                    last_write[0] = att.last_write_time;
                    recent_buffers[0] = buffer;
                }
                
                else if (att.last_write_time > last_write[1])
                {
                    last_write[1] = att.last_write_time;
                    recent_buffers[1] = buffer;
                }
            }
        }
        
        String8 left  = push_buffer_unique_name(app, scratch, recent_buffers[0]);
        String8 right = push_buffer_unique_name(app, scratch, recent_buffers[1]);
        Long_Print_Messagef(app, "Recent Files:\n  Left:  %.*s\n  Right: %.*s", string_expand(left), string_expand(right));
        
        u64 max_days_older = 7;
        u64 days_older = (last_write[0] - last_write[1]) / (24*60*60*10000000ULL);
        
        if (days_older >= max_days_older)
            Long_Print_Messagef(app, " (%llu days older than left)", days_older);
        print_message(app, S8Lit("\n\n"));
        
        View_ID view = get_active_view(app, 0);
        view_set_buffer(app, view, recent_buffers[0], 0);
        if (days_older < max_days_older)
        {
            view = get_next_view_looped_primary_panels(app, view, 0);
            view_set_buffer(app, view, recent_buffers[1], 0);
        }
    }
    
    //- NOTE(rjf): Initialize audio.
    def_audio_init();
    
    //- NOTE(rjf): Initialize stylish fonts.
    {
        String8 bin_path = system_get_path(scratch, SystemPath_Binary);
        
        // NOTE(rjf): Title font.
        {
            Face_Description desc = {0};
            {
                desc.font.file_name =  push_u8_stringf(scratch, "%.*sfonts/RobotoCondensed-Regular.ttf", string_expand(bin_path));
                desc.parameters.pt_size = 18;
                desc.parameters.bold = 0;
                desc.parameters.italic = 0;
                desc.parameters.hinting = 0;
            }
            
            global_styled_title_face = try_create_new_face(app, &desc);
            if (!global_styled_title_face)
                global_styled_title_face = default_font;
        }
        
        // NOTE(rjf): Label font.
        {
            Face_Description desc = {0};
            {
                desc.font.file_name =  push_u8_stringf(scratch, "%.*sfonts/RobotoCondensed-Regular.ttf", string_expand(bin_path));
                desc.parameters.pt_size = 10;
                desc.parameters.bold = 1;
                desc.parameters.italic = 1;
                desc.parameters.hinting = 0;
            }
            
            global_styled_label_face = try_create_new_face(app, &desc);
            if (!global_styled_label_face)
                global_styled_label_face = default_font;
        }
        
        // NOTE(rjf): Small code font.
        {
            Face_Description normal_code_desc = get_face_description(app, default_font);
            Face_Description desc = {0};
            {
                desc.font.file_name =  push_u8_stringf(scratch, "%.*sfonts/Inconsolata-Regular.ttf", string_expand(bin_path));
                desc.parameters.pt_size = normal_code_desc.parameters.pt_size - 1;
                desc.parameters.bold = 1;
                desc.parameters.italic = 1;
                desc.parameters.hinting = 0;
            }
            
            global_small_code_face = try_create_new_face(app, &desc);
            if (!global_small_code_face)
                global_small_code_face = default_font;
        }
        
        // NOTE(long): Set the *compilation* buffer font
        {
            set_fancy_compilation_buffer_font(app);
            long_toggle_compilation_expand(app);
            long_toggle_compilation_expand(app);
        }
    }
    
    //- NOTE(rjf): Prep virtual whitespace.
    {
        def_enable_virtual_whitespace = def_get_config_b32(vars_save_string_lit("enable_virtual_whitespace"));
        clear_all_layouts(app);
    }
}

#endif //FCODER_LONG_H

/*date = January 12th 2023 5:22 pm */

#ifndef FCODER_LONG_H
#define FCODER_LONG_H

//~ @readme Tran Thanh Long's 4coder Custom Layer

// I aim to structure my code as a set of isolated, drop-in files without modifying the base layer
// This often means copying and slightly adjusting existing functions (see COPYPASTA comments)
// The only modified file is 4coder_code_index.cpp (see @long_index_intro for more info)
// The same goes for Fleury’s layer — I only rely on and extend the language/index system
// Everything else is optional and provided as-is (@f4_optional_intro for more details)

//~ @features 4coder Long Feature/Option List

//- @long_render_intro

// - File Bars: Uses the global font and includes Ryan's "progress bar" idea
//   Displays file metadata (name, id, EOL mode, etc) at the start and cursor position at the end

// - Line Margin: Displays either absolute line numbers or relative offsets from the current line
//   Toggle visibility or switch mode with toggle_line_numbers and long_toggle_line_offset

// - Error Annotations: Inspired by qol_draw_compile_errors and F4_RenderErrorAnnotations
//   Highlights lines with errors in red and displays the message at the end of the line
//   Can also show config, binding, and project errors

// - Hex Colors: Highlight any 8-digit with a color interpreted from its value
//   Very helpful when changing the theme colors (open any theme file and see)

// - Digit Grouping: Colors every 2 (bin), 3 (decimal), or 4 (hex) digits alternately
//   Applies to numeric prefixes and postfixes as well

// - Highlight: There are currently 3 separate highlight systems:
//   - List: for highlighting a runtime list of ranges (push via Long_Highlight_Push)
//   - Range: for highlighting between the cursor and mark (notepad-style, isearch, query, etc.)
//   - Multi-Cursor: for highlighting all entries in the cursor list
// => I should probably merge these 3 into one system

// - Whitespace Highlight: Render whitespaces as grey circles
//   Could be done via draw_rectangle with roundness, but small circles look bad in 4coder
//   Instead, a font-dependent dot character is drawn at the correct size to fake the circle
//   Toggle per-view with toggle_show_whitespace, or globally with long_toggle_whitespace_all

// - Brace Rendering: Consits of 3 subsystems, inspired by 4coder_fleury_brace:
//   - Lines: thin vertical lines connecting the cursor’s surrounding open and close braces
//   - Annotation: small text after a close brace showing the code before the matching open brace
//   - Highlight: colors each surrounding brace pair differently
// - Paren Rendering: Only has paren highlighting out of the three brace features

// - Divider Comments: A smarter version of F4_RenderDividerComments
//   Renders the dividing line only up to the visible screen edge

// - Token Occurrence Highlight: Similar to Ryan's Cursor Identifier Highlight
//   A modified version of F4_RenderRangeHighlight, inlined directly into Long_Render_Buffer

// - Cursor Rendering: Supports two cursor styles:
//   - Original: The mark is visible and must be moved manually
//   - Notepad-like: The mark is hidden and stops snapping to the cursor when holding Shift
//   Both styles smoothly interpolate the cursor/mark and include a glow effect
//   In Original mode, a thin vertical line is drawn between the cursor and mark

//- @long_hud_intro

//- @f4_optional_files Calculator/Ploting/Recent-Files

//- @long_hot_reload

// Long_SaveFile: new SaveFile hook that updates the buffer’s saved state
// - Theme
// - Config
// - Bindings
// - Project

//- @long_auto_indent

//- @long_point_stack_intro

// 4coder provides a basic framework that uses a single global point stack
// This layer implements a completely new and separate system:
// - Instead of a single global stack, each view now has its own stack
// - You can iterate over all points or access a point by index
// - Each stack also tracks a current point that can be used for traversal

// Notable commands:
// - long_undo/redo_jump: jumps to the current point or the previous/next one if already there
// - long_push_new_jump: pushes the current position if it isn’t already the current point

//- @long_history_intro

// By default, 4coder only reset a buffer’s dirty state on save
// Now, undoing/redoing back to the saved state also clears the dirty flag

// Notable commands:
// - long_undo/redo(_all_buffers): same as the originals, but use the new history system
// - long_undo/redo_same_pos: undo/redo without changing the cursor or camera position

//- @long_search_intro

// All new search, query, and list commands use an improved query system that supports basic actions
// such as copy-paste, clear all, center view, and integrates better with the multi-cursor system

// Notable Functions:
// - Long_ISearch: same as isearch, but supports case-sensitive searches
// - Long_Query_Replace: same as query_replace, but can move to the next or previous match
// - Long_ListAllLocations: similar to list_all_locations__generic, but:
//   - Can limit the search to the current buffer
//   - Remembers the buffer/position before the search, allowing a jump back when aborted
//   - Uses the new Multi-Select system (see @long_multi_select_intro for more info)

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

//- @long_lister_intro

// In general, I think 4coder’s API design is good but has several weaknesses
// Many of them can be seen in the default lister wrapper API
// The new lister system improves the UI, allows items to have headers and tooltips,
// adds new filtering options, and handles basic commands (e.g. copy, paste, clear all, etc.)

// Notable listers:
// - long_history_lister: lists all edits to the buffer; allows undo/redo to the selected entry
// - long_point_lister: lists all the current view's stored points; jumps to the selected one
// - long_jump_lister: same as view_jump_list_with_lister, but uses the new lister
// - long_theme_lister: same as theme_lister, but supports hot theme reloading

// @important
// The entire Lister system is a drop-in file -> just call Long_Lister_Run instead of run_lister
// Most default commands use run_lister, so upgrading them is a bit of work
// I took the simpler approach: #define run_lister as Long_Lister_Run

// The new lister uses an internal data type (Long_Lister_Data), added via Long_Lister_AddItem
// This function appends the extra data right after the allocated item and points user_data to itself
// i.e. node->user_data == node + 1
// If your custom code uses the same layout strategy, it will conflict, so keep that in mind

// Index note tags (defined in Long_Note_PushTag)
// - Main tags: type, namespace, declaration, constant, comment tag, TODO
// - Function tags: function, operator, lambda, constructor, getter, setter, [forward]
// - Type optional tags: [forward], [union], [generic]
// - Declaration optional tags: [argument], [global], [local], [field], [property]

//- @long_move_intro

// Move commands:
// - long_move_up/down_token_occurrence: copies Jack’s F4_Boundary_CursorToken
// - long_move_next/prev_alpha_numeric_or_camel_boundary: like the defaults, but better case handling
// - long_move_next/prev_word: similar to the default commands, but better with comments/strings
// - long_move_to_next/prev_function_and_type: uses F4_Index to scan for named functions/types
// - long_move_to_next/prev_divider_comment: simplified version of _F4_Boundary_DividerComment

// Scope commands:
// - long_select_prev/next_scope_current_level: If a scope is selected, jump to its prev/next sibling
//   Otherwise, select the first scope before the cursor
// - long_select_upper_scope: selects the parent/surrounding scope
//   (same as select_surrounding_scope, renamed for consistency)
// - long_select_lower_scope: selects the first scope inside the current one
// - long_select_surrounding_scope: selects the surrounding scope
//   If a scope is already selected, toggles between inclusive and exclusive selection

//- @long_commands

// - Long_Buffer_Kill: When you kill a buffer in the original command, 4coder will switch to the most recent buffer.
//   It's not that bad because killing a buffer isn't common. But what's common is killing the *search* buffer, and
//   it's very annoying when after a quick search the buffer that you get back isn't the one before the search.
//   This function will first check if the buffer you want to kill is the *search* buffer, then jump back to the
//   correct position afterward.
//   This is used by long_interactive_kill_buffer, long_kill_buffer, and long_kill_search_buffer.

// - long_select_current_line: This will move the cursor to the end of the current line and the mark to the
//   first non-whitespace character on the current line or the end of the previous line if it's already there.

// - long_load_project: I modified this command to work with the new reference library concept.
//   Each project file can now contain an array of reference paths (set inside the reference_paths variable)
//   When this command runs, it will recursively load all files in those paths as read-only and unimportant


//~ TODO INDEX

//- CODE
// [ ] Refactor the C# parser
// [ ] Write a new cpp parser
// [ ] Rewrite the Index system into a simple drop-in file
// [ ] Add Index API for customizing the indentation and pos-context
// [ ] Handle function overloading
// [ ] Rewrite and optimize the lookup function

//- INDENTING
// [ ] Change `:` indenting for the ternary operator and label-in-the-midle `else LABEL: \n foo()`
// [ ] Indentless comments/strings

//- RENDER
// [ ] Autocompletion using Pos-Context or casey/jack's system
// [ ] String/Comment indenting as code
// [ ] Render #if block with annotation

//- BUGS
// [ ] Fix global function macro color for variable
// [ ] {} and () in the argument list cause prototype functions and pos-context to be parsed incorectly
// [ ] Parsing arary in the CPP parser

//~ TODO RENDER

//- Pos-Context
// [ ] Anchor it at the start position
// [ ] Display it above the current line when overflow or newline
// [ ] Add blank virtual lines at the tooltip position so there's no overlapped
// [ ] Handle line-wrapping when drawing at bottom

//- CODE
// [ ] Compress jump highlight and error highlight into one
// [ ] Change the margin style
// [ ] Slider
// [X] Long_Render_Context helper

//- Line Wrap/Overflow
// [ ] Weird wrap postion
// [ ] Incorrect movement (up/down/beg/end) between visual lines
// [ ] Overflow line width
// [ ] Overlap with line number margin

//~ TODO Face Commands
// [ ] Use the new query bar system
// [ ] Up/Down to change the face size
// [ ] Cap the input size
// [ ] Change other related faces (global_small_code_face)
// [ ] Collapse down into two fonts: UI and Code

//~ TODO MULTI CURSOR
// [ ] Across buffers
// [ ] Correct MC cursors while enabling virtual whitespace
// [ ] Copy in MC mode -> Exit and re-enter MC mode -> Paste in MC mode
// [ ] Special UI to know that MC mode is on (cursor color, cursor count, file bar, etc)
// [ ] Handle special commands (re: commented bindings in Long_Binding_MultiCursor)
//     [ ] Jupming commands
//     [ ] Query bar commands
//     [X] Lister commands
//     [ ] Others

//~ TODO BUGS
// [ ] Fix undo/redo_all_buffers right after saving bug
// [ ] undo/redo in MC mode
// [ ] The current saved history gets overwritten by merging with the next modification
// [ ] Move to the next/prev divider comment's line rather than position
// [ ] The cursor doesn't get snapped into view when overlaps with the file bar
// [ ] Fix incorrect pos-context rect after modifying string

//~ @CONSIDER LISTER

//- SEARCH
// [ ] Replace wildcard searching in the query bar and lister with fuzzy search (spaces only)
// [ ] Search for definitions like Hoogle
// [ ] Has a lister for important but rarely used commands
// [ ] Hotkeys for inserting and cycling through common tags
// [ ] Run commands while in lister

//- RENDER
// [ ] Put ... after overflow items
// [ ] Has syntax highlight inside each item's contents
// [ ] Improve relative path display in the file lister
// [ ] Render multi-column grid like byp_qol
// [ ] Exit lister with sub lister for dirty buffers

//~ @CONSIDER FPS HUD
// [ ] More detailed stats
// [ ] Pause-Resume
// [ ] Position (Top-Left, Top-Right, Bot-Left, Bot-Right)

//~ @CONSIDER NEW SYSTEM
// [ ] LOC counter
// [ ] Virtual column
// [ ] Jump system
// [ ] Lister + Query Bar: https://discord.com/channels/657067375681863699/657067375681863703/1363439846793679088

//~ @CONSIDER Interesting but low-priority stuff that may or may not be useful but deserves a look

//- IDENTIFIER (when the cursor is at then end of the identifier token)
// [ ] Search
// [ ] Replace
// [ ] List
// [ ] Peek
// [ ] Long_Boundary_CursorToken
// [ ] Token Occurrence Highlight

//- HOT RELOADING
// [ ] Hot-reload multi-cursor bindings
// [ ] Custom commands for each reloadable file

//- Quick Buffer
// [ ] Config
// [ ] Project
// [ ] Bindings
// [ ] 4coder folder
// [ ] Reference folders

//- MISC
// [ ] Move range selection up and down
// [ ] Modal auto-complete {} () [] on enter or typing
// [ ] Jump to location with relative path
// [ ] Code peek yank
// [ ] Greedy lookup the binding commands

//~ NOTE(long): @long_macros
#define LONG_INDEX_INDENT_STATEMENT 1
#define LONG_INDEX_INLINE 1
#define LONG_INDEX_INSERT_QUEUE 1
#define LONG_INDEX_INDENT_PAREN 1

#define LONG_LISTER_OVERLOAD 1

#define LONG_ENABLE_INDEX_PROFILE 0
#define LONG_ENABLE_PROFILE 0

#if LONG_ENABLE_INDEX_PROFILE
#define Long_Index_ProfileScope(T, N) ProfileScope(T, N)
#define Long_Index_ProfileBlock(T, N) ProfileBlock(T, N)
#else
#define Long_Index_ProfileScope(...)
#define Long_Index_ProfileBlock(...)
#endif

//~ NOTE(long): @default_headers
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "4coder_base_types.h"
#include "4coder_version.h"
#include "4coder_table.h"
#include "4coder_events.h"
#include "4coder_types.h"
#include "4coder_doc_content_types.h"
#include "4coder_default_colors.h"
#define DYNAMIC_LINK_API
#include "generated/custom_api.h"
#include "4coder_system_types.h"
#define DYNAMIC_LINK_API
#include "generated/system_api.h"
#if !defined(META_PASS)
#include "generated/command_metadata.h"
#endif

#include "4coder_profile.h"
#include "4coder_profile_inspect.h"

#include "4coder_token.h"
#include "generated/lexer_cpp.h"
#include "generated/4coder_fleury_lexer_jai.h"
#include "generated/lexer_cs.h"

#include "4coder_variables.h"
#include "4coder_audio.h"
#include "4coder_async_tasks.h"
#include "4coder_string_match.h"
#include "4coder_helper.h"
#include "4coder_delta_rule.h"
#include "4coder_layout_rule.h"
#include "4coder_code_index.h"
#include "4coder_draw.h"
#include "4coder_insertion.h"
#include "4coder_command_map.h"
#include "4coder_lister_base.h"
#include "4coder_clipboard.h"
#include "4coder_default_framework.h"
#include "4coder_config.h"
#include "4coder_auto_indent.h"
#include "4coder_search.h"
#include "4coder_build_commands.h"
#include "4coder_jumping.h"
#include "4coder_jump_sticky.h"
#include "4coder_jump_lister.h"
#include "4coder_project_commands.h"
#include "4coder_prj_v1.h"
#include "4coder_function_list.h"
#include "4coder_scope_commands.h"
#include "4coder_combined_write_commands.h"
#include "4coder_log_parser.h"
#include "4coder_tutorial.h"
#include "4coder_search_list.h"

//~ NOTE(rjf): @f4_headers
#include "4coder_fleury_ubiquitous.h"
#include "4coder_fleury_lang.h"
#include "4coder_fleury_index.h"
#include "4coder_fleury_colors.h"
#include "4coder_fleury_render_helpers.h"

//~ NOTE(long): @f4_optional_headers
#include "4coder_fleury_recent_files.h"
#include "4coder_fleury_plot.h"
#include "4coder_fleury_calc.h"

//~ NOTE(long): @long_headers
#include "4coder_long_hooks.h"
#include "4coder_long_render.h"
#include "4coder_long_index.h"
#include "4coder_long_base_commands.h"
#include "4coder_long_lister.h"
#include "4coder_long_bindings.h"

//~ NOTE(long): @default_src
#include "4coder_base_types.cpp"
#include "4coder_stringf.cpp"
#include "4coder_app_links_allocator.cpp"
#include "4coder_system_allocator.cpp"
#include "4coder_file.cpp"

#include "4coder_profile.cpp"
#if LONG_ENABLE_PROFILE
#include "4coder_profile_static_enable.cpp"
#else
#include "4coder_profile_static_disable.cpp"
#endif

#define DYNAMIC_LINK_API
#include "generated/custom_api.cpp"
#define DYNAMIC_LINK_API
#include "generated/system_api.cpp"
#include "4coder_system_helpers.cpp"
#include "4coder_layout.cpp"
#include "4coder_events.cpp"
#include "4coder_custom.cpp"
#include "4coder_log.cpp"
#include "4coder_hash_functions.cpp"
#include "4coder_table.cpp"
#include "4coder_codepoint_map.cpp"
#include "4coder_async_tasks.cpp"
#include "4coder_string_match.cpp"
#include "4coder_buffer_seek_constructors.cpp"

#include "4coder_command_map.cpp"
#include "4coder_default_map.cpp"
#include "4coder_mac_map.cpp"

#include "4coder_token.cpp"
#include "generated/lexer_cpp.cpp"
#include "generated/4coder_fleury_lexer_jai.cpp"
#include "generated/lexer_cs.cpp"

#include "4coder_default_framework_variables.cpp"
#include "4coder_default_colors.cpp"
#include "4coder_helper.cpp"
#include "4coder_delta_rule.cpp"
#include "4coder_layout_rule.cpp"
#include "4coder_code_index.cpp"
#include "4coder_fancy.cpp"
#include "4coder_draw.cpp"
#include "4coder_font_helper.cpp"
#include "4coder_config.cpp"
#include "4coder_dynamic_bindings.cpp"
#include "4coder_default_framework.cpp"
#include "4coder_clipboard.cpp"
#include "4coder_lister_base.cpp"
#include "4coder_base_commands.cpp"
#include "4coder_insertion.cpp"
#include "4coder_eol.cpp"
#include "4coder_lists.cpp"
#include "4coder_auto_indent.cpp"
#include "4coder_search.cpp"
#include "4coder_jumping.cpp"
#include "4coder_jump_sticky.cpp"
#include "4coder_jump_lister.cpp"
#include "4coder_code_index_listers.cpp"
#include "4coder_log_parser.cpp"
#include "4coder_keyboard_macro.cpp"
#include "4coder_cli_command.cpp"
#include "4coder_build_commands.cpp"
#include "4coder_project_commands.cpp"
#include "4coder_prj_v1.cpp"
#include "4coder_function_list.cpp"
#include "4coder_scope_commands.cpp"
#include "4coder_combined_write_commands.cpp"
#include "4coder_miblo_numbers.cpp"
#include "4coder_profile_inspect.cpp"
#include "4coder_tutorial.cpp"
#include "4coder_doc_content_types.cpp"
#include "4coder_doc_commands.cpp"
#include "4coder_docs.cpp"
#include "4coder_variables.cpp"
#include "4coder_audio.cpp"
#include "4coder_search_list.cpp"
#include "4coder_examples.cpp"
#include "4coder_default_hooks.cpp"

//~ NOTE(rjf): @f4_src
#include "4coder_fleury_ubiquitous.cpp"
#include "4coder_fleury_lang.cpp"
#include "4coder_fleury_index.cpp"
#include "4coder_fleury_colors.cpp"
#include "4coder_fleury_render_helpers.cpp"

//~ NOTE(long): @f4_optional_src
#include "4coder_fleury_recent_files.cpp"
#include "4coder_fleury_plot.cpp"
#include "4coder_fleury_calc.cpp"

//~ NOTE(long): @long_src
#include "plugins/4coder_multi_cursor.cpp"

#include "4coder_long_hooks.cpp"
#include "4coder_long_render.cpp"
#include "4coder_long_index.cpp"
#include "4coder_long_base_commands.cpp"
#include "4coder_long_lister.cpp"
#include "4coder_long_bindings.cpp"

//~ NOTE(rjf): 4coder Stuff
#include "generated/managed_id_metadata.cpp"

//~ NOTE(long): @long_custom_layer_initialization

function Face_ID Long_Font_TryCreate(Application_Links* app, String8 filename, Face_ID fallback, u32 size, b8 bold, b8 italic)
{
    Face_Description desc = {0};
    desc.font.file_name = filename;
    desc.parameters.pt_size = size;
    desc.parameters.bold = bold;
    desc.parameters.italic = italic;
    
    Face_ID result = try_create_new_face(app, &desc);
    if (!result)
        result = fallback;
    return result;
}

void custom_layer_init(Application_Links* app)
{
    default_framework_init(app);
    global_frame_arena = make_arena(get_base_allocator_system());
    permanent_arena = make_arena(get_base_allocator_system());
    
    // NOTE(long): Initialize BYP's MC
    MC_init(app);
    implicit_map_function = 0;
    
    // NOTE(long): Set up hooks.
    set_all_default_hooks(app);
    set_custom_hook(app, HookID_Tick,                    Long_Tick);
    set_custom_hook(app, HookID_RenderCaller,            Long_Render);
    set_custom_hook(app, HookID_WholeScreenRenderCaller, Long_WholeScreenRender);
    set_custom_hook(app, HookID_ViewEventHandler,        long_view_input_handler);
    set_custom_hook(app, HookID_BeginBuffer,             Long_BeginBuffer);
    set_custom_hook(app, HookID_EndBuffer,               Long_EndBuffer);
    set_custom_hook(app, HookID_SaveFile,                Long_SaveFile);
    set_custom_hook(app, HookID_BufferEditRange,         Long_BufferEditRange);
    set_custom_hook(app, HookID_DeltaRule,               Long_DeltaRule);
    set_custom_hook_memory_size(app, HookID_DeltaRule,   delta_ctx_size(sizeof(Vec2_f32)));
    
    // NOTE(long): Set up bindings
    mapping_init(get_thread_context(app), &framework_mapping);
    if (!dynamic_binding_load_from_file(app, &framework_mapping, S8Lit("bindings.4coder")))
        Long_Binding_SetupDefault(&framework_mapping);
    Long_Binding_SetupEssential(&framework_mapping);
    Long_Binding_MultiCursor();
    
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
    
    //- @COPYPASTA(long): default_4coder_initialize
    {
        load_themes_default_folder(app);
        
        // config loading
        String8 data = {};
        FILE* file = def_search_normal_fopen(scratch, "config.4coder", "rb");
        if (file)
        {
            data = dump_file_handle(scratch, file);
            fclose(file);
        } 
        Long_Config_ApplyFromData(app, data, 0, 0);
        
        // open command line files
        String8 hot = push_hot_directory(app, scratch);
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
    Buffer_ID comp_buffer, msg_buffer, left_buffer, right_buffer;
    {
        Buffer_Create_Flag special_flags = BufferCreate_NeverAttachToFile|BufferCreate_AlwaysNew;
        
        comp_buffer = create_buffer(app, S8Lit("*compilation*"), special_flags);
        buffer_set_setting(app, comp_buffer, BufferSetting_Unimportant, true);
        buffer_set_setting(app, comp_buffer, BufferSetting_ReadOnly, true);
        buffer_set_setting(app, comp_buffer, BufferSetting_Unkillable, true);
        
        Buffer_ID calc_buffer = create_buffer(app, S8Lit("*calc*"), special_flags);
        buffer_set_setting(app, calc_buffer, BufferSetting_Unimportant, true);
        
        msg_buffer  = get_buffer_by_name(app, S8Lit("*messages*"), 0);
        left_buffer = get_buffer_by_name(app, S8Lit( "*scratch*"), 0);
        right_buffer = calc_buffer;
    }
    
    //- NOTE(rjf): Initialize panels
    {
        // NOTE(long): Left Panel
        View_ID view = get_active_view(app, Access_Always);
        new_view_settings(app, view);
        view_set_buffer(app, view, left_buffer, 0);
        
        Face_Description description = get_buffer_face_description(app, comp_buffer);
        description.font.file_name = def_search_normal_full_path(scratch, S8Lit("fonts/Inconsolata-Regular.ttf"));
        
        // NOTE(long): Left Bottom Panel
        View_ID compilation_view = Long_View_Open(app, view, comp_buffer, ViewSplit_Bottom, 1);
        Face_ID special_face = try_create_new_face(app, &description);
        buffer_set_face(app, comp_buffer, special_face);
        
        // NOTE(long): Right Bottom Panel
        Long_View_Open(app, compilation_view, msg_buffer, ViewSplit_Right, 1);
        buffer_set_face(app, msg_buffer, special_face);
        
        // NOTE(long): Compilation Panel Split
        global_compilation_view = compilation_view;
        long_toggle_compilation_expand(app);
        long_toggle_compilation_expand(app);
        
        // NOTE(long): Right Panel
        Long_View_Open(app, view, right_buffer, ViewSplit_Right, 0);
        
        // NOTE(long): Restore Active to Left
        view_set_active(app, view);
    }
    
    //- NOTE(rjf): Auto-Load Project.
    b32 auto_load = def_get_config_b32_lit("automatically_load_project");
    if (auto_load)
        long_load_project(app);
    
    //- NOTE(long): Open most recent modified files on startup
    {
        u64 last_write[2] = {};
        Buffer_ID recent_buffers[2];
        
        String8 treat_as_code_string = def_get_config_str_lit(scratch, "treat_as_code");
        String8Array extensions = parse_extension_line_to_extension_list(app, scratch, treat_as_code_string);
        
        for (Buffer_ID buffer = get_buffer_next(app, 0, 0); buffer; buffer = get_buffer_next(app, buffer, 0))
        {
            Temp_Memory_Block temp(scratch);
            String8 filename = push_buffer_file_name(app, scratch, buffer);
            
            i64 unimportant = 0, readonly = 0;
            buffer_get_setting(app, buffer, BufferSetting_Unimportant, &unimportant);
            buffer_get_setting(app, buffer, BufferSetting_ReadOnly, &readonly);
            
            String8 ext = string_file_extension(filename);
            b32 is_code_file = Long_Index_IsMatch(ext, extensions.strings, extensions.count) &&
                !string_match(ext, S8Lit("4coder"));
            
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
        
        u64 max_days_older = 7;
        u64 days_older = (last_write[0] - last_write[1]) / (24*60*60*10000000ULL);
        
#if 0
        String8 left  = push_buffer_unique_name(app, scratch, recent_buffers[0]);
        String8 right = push_buffer_unique_name(app, scratch, recent_buffers[1]);
        Long_Print_Messagef(app, "Recent Files:\n  Left:  %.*s\n  Right: %.*s", string_expand(left), string_expand(right));
        
        if (days_older >= max_days_older)
            Long_Print_Messagef(app, " (%llu days older than left)", days_older);
        print_message(app, S8Lit("\n\n"));
#endif
        
        View_ID view = get_active_view(app, 0);
        view_set_buffer(app, view, recent_buffers[0], 0);
        if (days_older < max_days_older)
        {
            view = get_next_view_looped_primary_panels(app, view, 0);
            view_set_buffer(app, view, recent_buffers[1], 0);
        }
    }
    
    //- NOTE(long): Initialize Point Stack
    Long_View_ForEach(view, app, 0)
        Long_PointStack_PushCurrent(app, view);
    
    //- NOTE(rjf): Initialize audio.
    def_audio_init();
    
    //- NOTE(rjf): Initialize stylish fonts.
    {
        Face_ID default_font = get_face_id(app, 0);
        Face_Description default_desc = get_face_description(app, default_font);
        
        String8 binary = system_get_path(scratch, SystemPath_Binary);
        String8 roboto = push_u8_stringf(scratch, "%.*sfonts/RobotoCondensed-Regular.ttf", string_expand(binary));
        String8 incons = push_u8_stringf(scratch, "%.*sfonts/Inconsolata-Regular.ttf", string_expand(binary));
        
        // NOTE(long): calc/plot faces
        u32 pt_size = default_desc.parameters.pt_size;
        global_styled_title_face = Long_Font_TryCreate(app, roboto, default_font, pt_size + 3, 0, 0);
        global_styled_label_face = Long_Font_TryCreate(app, roboto, default_font, pt_size - 2, 1, 1);
        global_small_code_face   = Long_Font_TryCreate(app, incons, default_font, pt_size - 2, 1, 1);
    }
    
    //- NOTE(rjf): Prep virtual whitespace.
    {
        def_enable_virtual_whitespace = def_get_config_b32_lit("enable_virtual_whitespace");
        clear_all_layouts(app);
    }
}

#endif //FCODER_LONG_H

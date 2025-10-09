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

// @panel_render (Long_Render)
// - File Bars: Uses the global font and includes Ryan's "progress bar" idea
//   Displays file metadata (name, id, EOL mode, etc) at the start and cursor position at the end
//
// - Line Margin: Displays either absolute line numbers or relative offsets from the current line
//   Toggle visibility or switch mode with toggle_line_numbers and long_toggle_line_offset
//
// - Cursor-Mark Highlight: Renders a thin vertical line connecting the cursor and mark
//   Long_Highlight_CursorMark draws the line to the left of the text layout

// @buffer_render (Long_Render_Buffer)
// - Error Annotations: Inspired by qol_draw_compile_errors and F4_RenderErrorAnnotations
//   Highlights lines with errors in red and displays the message at the end of the line
//   Can also show config, binding, and project errors
//
// - Hex Colors: Highlight any 8-digit with a color interpreted from its value
//   Very helpful when changing the theme colors (open any theme file and see)
//
// - Digit Grouping: Colors every 2 (bin), 3 (decimal), or 4 (hex) digits alternately
//   Applies to numeric prefixes and postfixes as well
//
// - Highlight: There are currently 3 separate highlight systems:
//   - List: for highlighting a runtime list of ranges (push via Long_Highlight_Push)
//   - Range: for highlighting between the cursor and mark (notepad-style, isearch, query, etc.)
//   - Multi-Cursor: for highlighting all entries in the cursor list
// => I should probably merge these 3 into one system
//
// - Whitespace Highlight: Render whitespaces as grey circles
//   Could be done via draw_rectangle with roundness, but small circles look bad in 4coder
//   Instead, a font-dependent dot character is drawn at the correct size to fake the circle
//   Toggle per-view with toggle_show_whitespace, or globally with long_toggle_whitespace_all
//
// - Brace Rendering: Consits of 3 subsystems, inspired by 4coder_fleury_brace:
//   - Lines: thin vertical lines connecting the cursor’s surrounding open and close braces
//   - Annotation: small text after a close brace showing the code before the matching open brace
//   - Highlight: colors each surrounding brace pair differently
// - Paren Rendering: Only has paren highlighting out of the three brace features
//
// - Divider Comments: A smarter version of F4_RenderDividerComments
//   Renders the dividing line only up to the visible screen edge
//
// - Token Occurrence Highlight: Similar to Ryan's Cursor Identifier Highlight
//   A modified version of F4_RenderRangeHighlight, inlined directly into Long_Render_Buffer
//
// - Cursor Rendering: Supports two cursor styles:  and 
//   - Original (Long_Render_EmacsCursor): The mark is visible and must be moved manually
//   - Notepad-like (Long_Render_NotepadCursor): Mark hidden; no snapping while holding Shift
//   Both styles smoothly interpolate the cursor/mark and include a glow effect

// @screen_render (Long_WholeScreenRender)
// - Code Peek: Searches for the symbol under the cursor and displays its declaration
//   Rendered with full syntax highlighting at the bottom of the other half of the screen
//
// - Position-Context: Renders a helper tooltip showing extra info about the current symbol
//   For example, if the symbol is a function, the tooltip shows all of its arguments
//   Entries are pushed via F4_Language_PosContext_PushData (originally Ryan's idea)
//   Multiple configurable options => see @long_index_intro for details
//
// - Lister: An improved version of the default lister_render using the new lister system
//   - Each item can include a header, tooltip, and optional preview
//     (e.g. code peek for the index lister, docs for the command lister)
//   - Only renders visible items while displaying the total count and current index
//   - Occupies a portion of the current panel rather than filling it
//
// - FPS: Displays stats (FPS, frame time, frame index, etc) in the top-right corner
//   Tracks the last 10 frames (configurable with fps_history_depth)
//
// - Tooltip: Renders a global list of runtime tooltips
//   Each is defined by a range and color, added via Long_Render_PushTooltip

//- @f4_optional_intro

// Optional Fleury systems that I found useful:
// - 4coder_fleury_calc
// - 4coder_fleury_plot
// - 4coder_fleury_recent_files
//
// Useful Fleury base commands (see @f4_base_commands in 4coder_long_base_commands.cpp):
// - f4_open_project
// - f4_home_first_non_whitespace
// - f4_unindent

//- @long_hot_reload

// Long_SaveFile: A custom SaveFile hook that updates the buffer's saved/dirty state
// Additionally, it hot-reloads content based on the saved file's name:
// - theme-xxx.4coder: Updates the corresponding color table according to the theme name
// - config.4coder: Calls Long_Config_ApplyFromData to update config settings
// - project.4coder: Calls Long_Prj_Parse to update project settings
// - bindings.4coder: Calls Long_Binding_LoadData to update all command key bindings
// Any syntax errors in these files will be displayed (see @buffer_render for details)
// The new updated value will be printed to *messages*

//- @long_auto_indent (4coder_code_index.cpp)

// LONG_INDENT_STMNT: #define generic_parse_statement as Long_Index_ParseStmnt
// - Indents control-flow statements that can take a single statement (e.g. if, else, for)
// - Properly indents C# [Attributes]
// - Properly indents Metadesk statements
//
// LONG_INDENT_PAREN: By default, 4coder stops indenting inside parentheses
// Set this macro to true to enable it (see 4coder_code_index_test.cpp for examples)

//- @long_point_stack_intro

// 4coder provides a basic framework that uses a single global point stack
// This layer implements a completely new and separate system:
// - Instead of a single global stack, each view now has its own stack
// - You can iterate over all points or access a point by index
// - Each stack also tracks a current point that can be used for traversal
//
// Notable commands:
// - long_undo/redo_jump: jumps to the current point or the previous/next one if already there
// - long_push_new_jump: pushes the current position if it isn’t already the current point

//- @long_history_intro

// By default, 4coder only reset a buffer’s dirty state on save
// Now, undoing/redoing back to the saved state also clears the dirty flag
//
// Notable commands:
// - long_undo/redo(_all_buffers): same as the originals, but use the new history system
// - long_undo/redo_same_pos: undo/redo without changing the cursor or camera position

//- @long_search_intro

// All new search, query, and list commands use an improved system that supports basic actions
// such as copy-paste, clear all, center view, and integrates better with the multi-cursor system
//
// Notable Functions:
// - Long_ISearch: same as isearch, but supports case-sensitive searches
// - Long_Query_Replace: same as query_replace, but can move to the next or previous match
// - Long_ListAllLocations: similar to list_all_locations__generic, but:
//   - Can limit the search to the current buffer
//   - Remembers the buffer/position before the search, allowing a jump back when aborted
//   - Uses the new Multi-Select system (see @long_multi_select_intro for more info)
// - long_replace_in_range: Same as replace_in_range, but integrates better with multi-cursor
//   Highlights all matches in the range and shows a brief error fade if no match is found

//- @long_multi_select_intro

// I previously had a makeshift function to handle basic navigation and typing
// Later, I switched to BYP's excellent 4coder_multi_cursor plugin (see the file for more info)
// One thing I used to have with my previous function was the ability to replace across buffers
// This is harder to implement because BYP's MC plugin doesn't work across buffers
// Maybe adding a smooth transition to long_replace_all_buffers would be enough
//
// Notable commands:
// - Long_MC_DrawHighlights: Renders all cursors
// - Long_MC_ListAllLocations: Searches for a query and adds a new cursor for each match
// - long_mc_begin_multi_block: Like MC_begin_multi_block, but ignores comments and blank lines
// - long_mc_up/down_trail: Like MC_up/down_trail, but usuable by the main cursor in MC mode

//- @long_index_intro

// This is my largest customization layer so far, with helpers for writing a parser
// Originally started as an extended Fleury index system, but now larger and more complete
// Several Fleury files were modified, so it’s better rewritten as a simple drop-in file
// Here's a short list of noteworthy features follows:
//
// - Long_Index_LookupBestNote: The lookup function is the most important function in this layer
//   This function will parse a series of "selective path" and lookup the best-matched note
//   It resolves name collisions and can search the inheritance tree or imported namespaces
//   It understands that 'this' means search the base type, while 'new' is for constructors, etc
//   All the index commands and render functions use this, including but not limited to:
//   Long_Index_IndentBuffer, Long_Jump_Definition, and Long_Syntax_Highlight
//
// - The C# Parser: A complete and powerful parser that can handle:
//   - using directives and types, including classes, structs, enums, and tuples
//   - functions, constructors, lambdas, getters/setters, and operators
//   - declarations, type fields, function arguments, generic arguments, and inheritance
//   See 4coder_long_lang_cs.cpp and 4coder_cs_lexer_test.cs for examples and details
//
// - Long_Index_IndentBuffer: Uses Code_Index_Nest like the virtual whitespace system
//   Anytime the parser changes, both are updated correctly
//   All new indenting commands mirror their originals but use this new indenting system
//
// - The Render System
// 1. Long_Index_DrawPosContext: Similar to F4_PosContext_Render, but:
//   - Shows a declaration's base type (return type for functions, class/struct/etc. for types)
//   - For types (C# only, for now), splits into 3 categories: TYPES, FUNCS, and DECLS
//     - Cycles between categories with long_switch_pos_context_option
//     - Toggles on/off with long_toggle_pos_context
//     - Switches the tooltip's position with long_switch_pos_context_draw_position
// 2. Long_Index_DrawCodePeek: Similar to F4_CodePeek_Render, but:
//   - Uses F4_Index_Note rather than Code_Index_Note
//   - Calls Long_Index_LookupBestNote, which prioritizes definition notes over prototypes

// - @FINAL_NOTE -
// When I first wrote this system, I had some experience parsing and modifying AST nodes
// After studying how Ryan built his parser and indexer, I learned many new and useful techniques
// Since then, I've experimented with other projects, including this C# parser
// In the future, if I have time and energy, I'll write a more detailed comment on my experience

//- @long_lister_intro

// In general, I think 4coder’s API design is good but has several weaknesses
// Many of them can be seen in the default lister wrapper API
// The new lister system improves the UI, allows items to have headers and tooltips,
// adds new filtering options, and handles basic commands (e.g. copy, paste, clear all, etc.)
//
// Notable listers:
// - long_history_lister: lists all edits to the buffer; allows undo/redo to the selected entry
// - long_point_lister: lists all the current view's stored points; jumps to the selected one
// - long_jump_lister: same as view_jump_list_with_lister, but uses the new lister
// - long_theme_lister: same as theme_lister, but supports hot theme reloading

// @important
// The entire Lister system is a drop-in file -> just call Long_Lister_Run instead of run_lister
// Most default commands use run_lister, so upgrading them is a bit of work
// I took the simpler approach: #define run_lister as Long_Lister_Run
//
// The new lister uses an internal data type (Long_Lister_Data), added via Long_Lister_AddItem
// It appends the extra data right after the allocated item and points user_data to itself
// i.e. node->user_data == node + 1
// If your custom code uses the same layout strategy, it will conflict, so keep that in mind
//
// Index note tags (defined in Long_Note_PushTag)
// - Main tags: type, namespace, declaration, constant, comment tag, TODO
// - Function tags: function, operator, lambda, constructor, getter, setter, [forward]
// - Type optional tags: [forward], [union], [generic]
// - Declaration optional tags: [argument], [global], [local], [field], [property]

//- @long_move_intro

// Move commands:
// - long_move_up/down_token_occurrence: copies Jack’s F4_Boundary_CursorToken
// - long_move_next/prev_alpha_numeric_or_camel: like the defaults, but better case handling
// - long_move_next/prev_word: similar to the default commands, but better with comments/strings
// - long_move_to_next/prev_function_and_type: uses F4_Index to scan for named functions/types
// - long_move_to_next/prev_divider_comment: simplified version of _F4_Boundary_DividerComment
//
// Scope commands:
// - long_select_prev/next_scope_current_level: If a scope is selected, jump to its sibling
//   Otherwise, select the first scope before the cursor
// - long_select_upper_scope: selects the parent/surrounding scope
//   (same as select_surrounding_scope, renamed for consistency)
// - long_select_lower_scope: selects the first scope inside the current one
// - long_select_surrounding_scope: selects the surrounding scope
//   If a scope is already selected, toggles between inclusive and exclusive selection

//- @long_commands

// - Long_Buffer_Kill: Fixes 4coder's default buffer-kill behavior
//   Normally, killing a buffer switches to the most recent buffer
//   This is annoying when killing the search buffer, as you return to the wrong place
//   This function checks if the buffer being killed is the search buffer
//   If so, it jumps back to the correct position afterward
//   Used by long_interactive_kill_buffer, long_kill_buffer, and long_kill_search_buffer
//
// - long_load_project: Modified to work with the new reference library concept
//   Each project file can contain an array of reference paths in reference_paths
//   This command recursively loads all files in those paths as read-only and unimportant
//
// - long_macro_toggle_recording: Toggles macro recording
//   Much better than 2 different commands for starting and stopping recording
//
// - long_toggle_compilation_expand: Similar to f4_toggle_compilation_expand but centers the view
//   The expanded height is relative to the font's line height
// - long_toggle_panel_expand: Toggles the current panel's proportion between 0.625 and 0.5
// - long_toggle_panel_expand_big: Same as above, but between 0.75 and 0.625
//
// - long_paste_and_replace_range: This is my favorite command
//   Combines deleting the selected range and pasting into a single operation
//
// - long_autocomplete: Like f4_autocomplete_or_indent, but skips current input when in MC mode
//
// - long_toggle_comment_selection: Performs VS-style (un)commenting on the selected range
//   Comments each line with single-line comments, ignoring blank lines
//   Uses block comments if the selection doesn’t start/end at the first/last non-whitespace


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
//     [ ] Others

//~ TODO BUGS
// [ ] Fix undo/redo_all_buffers right after saving bug
// [ ] The current saved history gets overwritten by merging with the next modification
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

//- Buffer
// [ ] Quick open file
//     [ ] bindings.4coder
//     [ ]  project.4coder
//     [ ]   config.4coder
// [ ] Quick open folder
//     [ ] 4coder source code
//     [ ] Reference

//- MISC
// [ ] Move range selection up and down
// [ ] Modal auto-complete {} () [] on enter or typing
// [ ] Jump to location with relative path
// [ ] Code peek yank
// [ ] Greedy lookup the binding commands

//~ NOTE(long): @long_macros
#define LONG_INDENT_STMNT 1
#define LONG_INDEX_INLINE 1
#define LONG_INDEX_INSERT_QUEUE 1
#define LONG_INDENT_PAREN 0

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
#include "4coder_multi_cursor.cpp"

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

/* date = January 12th 2023 5:22 pm */

#ifndef FCODER_LONG_H
#define FCODER_LONG_H

//~ @readme Tran Thanh Long's 4coder Custom Layer

// In general, I really want to structure my code into a bunch of isolated drop-in files without changing
// any code from the base and fleury's layer. But it would mean that I need to copy a lot of functions to
// modify them slightly (I did do some of it, search for all the COPYPASTA comments). There are also multiple
// bugs in the original layers that I fixed. That's why a lot of code still depends on those modified files.
// Here's a list of them:

//- @long_init_files
// I never actually wrote my own custom_layer_init function to setup all the necessary hooks.
// I just modified the one from 4coder_fleury.cpp and included all my files there.
// I also put some additional includes and forward declarations in 4coder_default_include.cpp

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

//- @long_render_files

// 4coder_fleury_hooks.cpp
// - F4_RenderBuffer: Read the RENDER section in @long_index_intro for more details
// - F4_DrawFileBar: I redesign the file bar slightly, this is optional

// 4coder_fleury_cursor.cpp:
// - For some reason, F4_Cursor_RenderEmacsStyle doesn't use outline_thickness when render the cursor.
//   This's probably a bug, so I change the two C4_RenderCursorSymbolThingy calls.
// - Replace draw_highlight_range with Long_Highlight_DrawRange
//   This is for the "search for whitespace when enabling virtual whitespace" bug. More detail here:
//   https://discord.com/channels/657067375681863699/657067375681863703/1167466549339234384

//- @long_project_files
// 4coder_fleury_base_commands.cpp: f4_setup_new_project
// 4coder_project_commands.cpp: prj_open_files_pattern_filter__rec and load_project (@long_commands for more info)
// 4coder_project_commands.h: Add the PrjOpenFileFlag_ReadOnly flag
// 4coder_config.cpp: Call Long_UpdateCurrentTheme in load_config_and_apply (@long_commands for more info)

//- @4coder_lister_base
// Fix the lister overdraw bug:
// https://github.com/4coder-community/4cc/commit/dc692dc0d4b4a125475573ebfcc07cfd60a85bb7
// Fix lister_render calls draw_file_bar rather than F4_DrawFileBar
// Read @long_lister_intro for more details

//- @4coder_base_commands
// There's a bug in the redo command in 4coder_base_commands.cpp where it gets the wrong record info.
// I fixed it in the long_redo command (read @long_history_intro for more details as to what change)
// without touching the base file, change the base command directly if you want:
// https://discord.com/channels/657067375681863699/657067375681863703/1008430045548789840

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
// and using or looking up the generated index note. Originally, It started as an extended version for
// the fleury's index system. But after adding and writing most the code by hand, it has grown bigger and
// more complete than the fleury's one. I also have modified and added multiple new fields and flags to
// fleury's layer, so it's probably better to just completely rewrite that part and make it a simple drop-in
// file. Until then, here's a short list of all the noteworthy things:

// - The Lookup Function
//   Long_Index_LookupBestNote is the most important function in this layer. This function will parse a series
//   of path accessing and lookup the best-matched note. It has proper handling for name collision and
//   local/nested notes. It'll search the declaration type, return type, the inheritance tree, or the imported
//   namespaces for a match. It also understands that 'this' means search in the base type, 'new' means searching
//   for constructor, and `new Type {...}` means object initializer in C# (hasn't implementd for in C/C++ yet).
//   All the index commands and render functions use this, including but not limited to: Long_Index_IndentBuffer,
//   Long_GoToDefinition, _Long_SearchDefinition, Long_Scan_Note, Long_Index_DrawPosContext, and F4_GetColor.

// - The C# Parser
//   This is a mostly complete parser. It can parse (using) namespaces; classes, structs, enums, and tuples;
//   functions and function-like features like lambdas, getters/setters, and operators; local/global
//   declarations, type's fields, and function's arguments; generic type arguments and inheritance.
//   You can take a look at it in 4coder_long_lang_cs.cpp and 4coder_cs_lexer_test.cs for more details.

// - The Indent Function
//   Long_Index_IndentBuffer uses Code_Index_Nest just like the virtual whitespace system, so now anytime you
//   change the way the parser works, both of these will be updated correctly.
//   All the new indenting commands are just the same as their counterpart but use this new indenting system.

// - The Render System

// 1. Long_Index_DrawPosContext
//   Unlike F4_PosContext_Render, it will draw a function's return type, the variable's declaration type, and a type's
//   keyword (class, struct, enum, etc). When displaying a type, it will split all the type's members into 3
//   categories: TYPES, FUNCS, and DECLS, and display the number of entries for each one. You can cycle
//   between each category by calling the command long_switch_pos_context_option. Calling long_toggle_pos_context
//   for toggling the commands, and long_switch_pos_context_draw_position for changing the tooltip position.

// 2. Long_Index_DrawCodePeek
//   This is the same as F4_CodePeek_Render, but uses F4_Index_Note rather than Code_Index_Note and
//   calls Long_Index_LookupBestNote. It will prioritize def notes over prototype ones.

// - FINAL NOTE -
// When I first wrote this system, I had some experience parsing and modifying AST nodes. After reading
// how Ryan did his parser and indexer, I've learned a lot of new and useful techniques. Since then,
// I've been experimenting with my other projects (including this C# parser). If I have the time and
// energy in the future, I'll write a more detailed comment about the way the C# parser works and how
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

// In general, I think the API design of 4coder is good but has several weaknesses.
// You can see some of them in the default lister wrapper API.
// In the future, I may write a new wrapper layer with more detailed comments about these shortcomings.
// But for now, I just modify 4coder_lister_base.cpp:
// - lister_render:
//   - Show the number of items in the query bar
//   - Each item can have an optional header that is its location
// - lister_get_filtered:
//   - Filter tags (inclusive and exclusive): Originally, I added this feature so that I don't need
//     specific commands to list all functions or types, but this has become more useful than I thought
//     because now you can search for keybindings in the command lister and unique file path in the buffer
//     lister because those are just tags.
//   - Filter header (only inclusive right now)
// - run_lister:
//   - Ctrl+V to paste to the query bar
//   - Ctrl+Backspace to delete the entire bar's string

// Index Note Tags (declared in _Long_PushNoteTag)
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

// - Long_KillBuffer: When you kill a buffer in the original command, 4coder will switch to the most
//   recent buffer. This isn't that bad because killing a buffer isn't that common. But what's common is
//   killing the *search* buffer, and it's very annoying when after a quick search the buffer
//   that you get back isn't the one before you started the search. This function will first check if the
//   buffer you want to kill is the *search* buffer, then jump back to the correct position afterward.
//   This is used by long_interactive_kill_buffer, long_kill_buffer, and long_kill_search_buffer.

// - long_select_current_line: This will move the cursor to the end of the current line and the mark to the
//   first non-whitespace character on the current line or the end of the previous line if it's already there.

// - long_reload_all_themes_default_folder: I added this command while customizing my theme file.
//   It will clear all the loaded themes and reload all the files in the default theme folder.
//   Then it will try to find the theme that was being used and select it. It'll do it by calling
//   Long_UpdateCurrentTheme in the theme lister and on startup.

// - load_project: I modified this command to work with the new reference library concept.
//   Each project file can now contain an array of reference paths (set inside the reference_paths variable)
//   When this command runs, it will recursively load all files in those paths as read-only and unimportant

//~ TODO SEARCH
// [ ] Replace all the wildcard searching in the query bar and lister with grep
// [ ] Search for definitions like Hoogle
// [ ] Has a lister for important but rarely used commands
// [ ] Search and open the most recent modified file on startup
// [ ] Handle function overloading

//~ TODO RENDER

//- LISTER
// [ ] Put ... after a large item and scroll its content horizontally over time
// [ ] Has syntax highlight inside each item's contents

//- INDEX
// [ ] Autocompletion using PosContext or casey/jack's system
// [ ] String/Comment indenting as code

//- BUGS
// [ ] Fix unmatched error annotation locations

//~ TODO CODE/ARCHITECTURE

//- LONG
// [ ] Write my own init layer and hooks
// [ ] Write a new lister layer from scratch as a drop-in file

// [ ] Write a new cpp parser
// [ ] Rewrite the Index system into a simple drop-in file
// [ ] Add Index API for customizing the indentation and poscontext

// [ ] Merge all the default query bar code into a single function
// [ ] Merge Long_Isearch and Long_Query_User_String into one function

//- FLEURY
// [ ] Strip out *calc* buffer
// [ ] Strip out power mode
// [ ] Strip out *lego* buffer 
// [ ] Strip out unused f4 commands

#include "4coder_long_index.h"
#include "4coder_long_base_commands.h"

#if LONG_ENABLE_PROFILE
#include "4coder_profile_static_enable.cpp"
#else
#include "4coder_profile_static_disable.cpp"
#endif

#if LONG_ENABLE_INDEX_PROFILE
#define Long_Index_ProfileScope(T, N) ProfileScope(T, N)
#define Long_Index_ProfileBlock(T, N) ProfileBlock(T, N)
#else
#define Long_Index_ProfileScope(...)
#define Long_Index_ProfileBlock(...)
#endif

#elif !defined(FCODER_LONG_CPP)
#define FCODER_LONG_CPP

#include "4coder_long_index.cpp"
#include "4coder_long_base_commands.cpp"

#endif //FCODER_LONG_H

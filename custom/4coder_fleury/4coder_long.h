/* date = January 12th 2023 5:22 pm */

#ifndef FCODER_LONG_H
#define FCODER_LONG_H

//~ TODO MOVEMENT/SELECT
// [X] Undo/redo cursor movements/jump list/recent buffers
// [X] Have different jump history for each buffer
// [X] Improve jump history/list and move between files
// [X] Delete-then-paste in range
// [X] Copy current line
// [X] Cut current line
// [X] Select current line
// [X] Move between functions and types
// [X] Move from between start/end boundary of a token
// [X] Improve move between scopes
// [X] Movement binding
// [X] Polish all the move/scope commands
// [ ] Move camera view

//~ TODO SEARCH/LIST/QUERY
// [X] Complete all search commands and bindings
// [X] Jump list's view remembers the buffer before the search buffer
// [X] Filter and list all functions/types/globals/macros file/project-wide
// [X] Search commands based on key bindings
// [X] Kill/switch/view the jump buffer directly
// [X] Paste from clipboard to search bar
// [X] List and query multiple selections
// [X] Query replace forward and backward using (Page)Up/Down
// [ ] Replace wildcard with grep
// [ ] Search for definitions like Hoogle
// [ ] Find all references (using the index system)

//~ TODO LISTER
// [X] Have a concept of a header-content-tag
// [X] Search for headers, contents, tags with inclusive and exclusive logic
// [X] Polish the index lister (show parent as tag, pos/line number, the entire function/type signature)
// [X] Show file location in the file lister
// [X] Show the number of items
// [ ] Differentiate type/flag/parent tag (`function` vs `[forward]` vs '<MyParent>`)
// [ ] Show rare command lister
// [ ] Show description/footer of an item
// [ ] Resize font size to fit the element (has min size for large items and put ... after it)
// [ ] Scroll the content in each slot horizontally to show big items
// [ ] Highlight words inside each item's contents

//~ TODO RENDER
// [X] Fix single statement indentation
// [X] Highlight the current argument in the tooltip
// [X] Make the code peek system works with F4_Index_Note
// [X] Split PosContext entries into fields/functions/types and cap the number of entries
// [ ] Autocompletion using PosContext or casey/jack's system
// [ ] Unite all code peeking systems and bindings (fleury_code_peek, f4_autocomplete, word_complete_drop_down, and fleury_pos_context)
// [ ] Fix error annotation locations
// [ ] Expand macros in a popup window
// [ ] String/Comment indent as code
// [ ] Make auto_indent_buffer use virtual whitespace

//~ TODO LANGUAGE
// [X] Have syntax color for locals, fields, arguments
// [X] Render cpp pos context
// [X] Make a cs parser
// [ ] Rework cpp parser
// [ ] Custom language indentation
// [ ] Custom language position context
// [ ] Correct function/type overloading name

//~ TODO FILE/BUFFER
// [X] Create read-only buffers from a path
// [X] Open/Create matching files in the same panel
// [ ] Don't save/write a file if the file's content is the same
// [ ] Open most recent modified file on startup
// [ ] Open theme file

//~ TODO BUGS
// [X] Fix opening the same lister multiple times bug
// [X] Fix jump to buffer before search bug
// [X] Fix jump to whitespace when enabling virtual_whitespace bug
// [ ] Fix copy outside 4coder bug
// [ ] Fix switching to the build panel when the user accidentally kills it
// [ ] Fix undo/redo camera scroll bug

//~ TODO MISC
// [X] Reload all themes from the default folder
// [X] Complete key bindings
// [ ] Strip out *calc* buffer
// [ ] Strip out power mode
// [ ] Strip out *lego* buffer 
// [ ] Strip out unused f4 commands

#define clamp_loop(x, size) ((x) < 0 ? ((size) - 1) : ((x) % (size)))
function b32 F4_ARGBIsValid(ARGB_Color color);
function void Long_Highlight_DrawList(Application_Links *app, Buffer_ID buffer, Text_Layout_ID layout, f32 roundness);

#include "4coder_long_index.h"

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

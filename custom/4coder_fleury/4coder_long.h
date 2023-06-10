/* date = January 12th 2023 5:22 pm */

#ifndef FCODER_LONG_H
#define FCODER_LONG_H

//~ TODO MOVEMENT/SELECT
// [X] Undo/redo cursor movements/jump list/recent buffers
// [X] Improve jump history/list and move between files
// [X] Delete-then-paste in range
// [X] Copy current line
// [X] Cut current line
// [X] Select current line
// [ ] Multi selection
// [ ] Understand the macro, clipboard, and lego system
// [X] Move between functions and types
// [X] Move from between start/end token's boundary
// [ ] Movement binding
// [ ] Fix undo/redo camera scroll

//~ TODO SEARCH/LIST
// [X] Complete all search commands and bindings
// [X] Jump list's view doesn't switch previous buffer to the current one
// [X] Filter and list all functions/types/globals/macros file/project-wide
// [X] Search commands based on key bindings
// [ ] Find all references
// [ ] Find and look at crt/windows source code

//~ TODO RENDER
// [ ] Expand macros in a popup window
// [ ] String/Comment indent as code
// [X] Fix single statement indentation
// [ ] Make auto_indent_buffer use virtual whitespace
// [X] Highlight current argument in tooltip
// [ ] Fix error annotation locations
// [ ] Unite all code peeking systems and bindings (fleury_code_peek, f4_autocomplete, word_complete_drop_down, and fleury_pos_context)

//~ TODO LANGUAGE
// [ ] Custom language indentation
// [ ] Custom language position context
// [ ] Correct function overloading name
// [ ] Rework cpp parser

#include "4coder_long_index.h"

#elif !defined(FCODER_LONG_CPP)
#define FCODER_LONG_CPP

#include "4coder_long_index.cpp"
#include "4coder_long_base_commands.cpp"

#endif //FCODER_LONG_H

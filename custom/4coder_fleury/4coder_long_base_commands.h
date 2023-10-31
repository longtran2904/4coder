/* date = October 30th 2023 4:22 pm */

#ifndef FCODER_LONG_BASE_COMMANDS_H
#define FCODER_LONG_BASE_COMMANDS_H

#define clamp_loop(x, size) ((((x) % (size)) + (size)) % (size))

//- NOTE(long): Highlight functions
function void Long_Highlight_DrawList(Application_Links *app, Buffer_ID buffer, Text_Layout_ID layout, f32 roundness);
function b32  Long_Highlight_DrawRange(Application_Links *app, View_ID view, Buffer_ID buffer, Text_Layout_ID layout, f32 roundness);

// NOTE(long): Fleury's functions
function b32 F4_ARGBIsValid(ARGB_Color color);

#endif //4CODER_LONG_BASE_COMMANDS_H

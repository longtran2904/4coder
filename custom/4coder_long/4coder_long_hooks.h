/* date = October 1st 2024 7:37 pm */

#ifndef FCODER_LONG_HOOKS_H
#define FCODER_LONG_HOOKS_H

function void Long_Tick(Application_Links* app, Frame_Info frame);
function void Long_Render(Application_Links* app, Frame_Info frame, View_ID view);
function i32  Long_SaveFile(Application_Links* app, Buffer_ID buffer);
function i32  Long_EndBuffer(Application_Links* app, Buffer_ID buffer);

function DELTA_RULE_SIG(Long_DeltaRule);
function BUFFER_EDIT_RANGE_SIG(Long_BufferEditRange);

#endif //FCODER_LONG_HOOKS_H

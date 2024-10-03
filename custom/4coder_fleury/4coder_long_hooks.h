/* date = October 1st 2024 7:37 pm */

#ifndef FCODER_LONG_HOOKS_H
#define FCODER_LONG_HOOKS_H

//~ NOTE(long): Render Hooks

function void Long_DrawFileBar (Application_Links* app, View_ID view_id, Buffer_ID buffer, Face_ID face_id, Rect_f32 bar);
function void Long_RenderBuffer(Application_Links* app, View_ID view_id, Buffer_ID buffer, Face_ID face_id,
                                Text_Layout_ID text_layout_id, Rect_f32 rect, Frame_Info frame_info);
function void Long_Render(Application_Links* app, Frame_Info frame_info, View_ID view_id);

//~ NOTE(long): Tick Hooks

function void Long_Tick(Application_Links* app, Frame_Info frame_info);

//~ NOTE(long): Buffer Hooks

function i32 Long_SaveFile(Application_Links *app, Buffer_ID buffer_id);
function i32 Long_EndBuffer(Application_Links* app, Buffer_ID buffer_id);

//~ NOTE(long): F4 Clone Hooks
function DELTA_RULE_SIG(Long_DeltaRule);
function BUFFER_EDIT_RANGE_SIG(Long_BufferEditRange);

#endif //FCODER_LONG_HOOKS_H

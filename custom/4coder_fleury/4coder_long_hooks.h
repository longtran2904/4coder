/* date = October 1st 2024 7:37 pm */

#ifndef FCODER_LONG_HOOKS_H
#define FCODER_LONG_HOOKS_H

function void Long_RenderBuffer(Application_Links* app, View_ID view_id, Face_ID face_id,
                                Buffer_ID buffer, Text_Layout_ID text_layout_id,
                                Rect_f32 rect, Frame_Info frame_info);
function void Long_Render(Application_Links* app, Frame_Info frame_info, View_ID view_id);

#endif //FCODER_LONG_HOOKS_H

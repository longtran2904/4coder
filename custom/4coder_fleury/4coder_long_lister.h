/* date = March 3rd 2024 3:02 pm */

#ifndef FCODER_LONG_LISTER_H
#define FCODER_LONG_LISTER_H

enum Long_Lister_HeaderType
{
    Long_Header_None,
    Long_Header_Path,
    Long_Header_Location,
};

struct Long_Lister_Data
{
    Long_Lister_HeaderType type;
    Buffer_ID buffer;
    i64 pos;
    i64 user_index;
    String8 tooltip;
};

function void Long_Lister_AddItem(Application_Links* app, Lister* lister, String8 name, String8 tag,
                                  Buffer_ID buffer, i64 pos, u64 index = 0, String8 tooltip = {});
function void Long_Lister_AddBuffer(Application_Links* app, Lister* lister, String8 name, String8 tag, Buffer_ID buffer);
function f32  Long_Lister_GetHeight(Lister* lister, f32 line_height, f32 block_height, f32* out_list_y);

#define Long_Lister_IsResultValid(result) (!(result).canceled && (result).user_data)

enum Long_Lister_TooltipType
{
    Long_Tooltip_UnderItem  = 0x80000000,
    Long_Tooltip_NextToItem = 0x80000001,
    //Long_Tooltip_TopLayout  = 0x80000002,
    //Long_Tooltip_BotLayout  = 0x80000003,
};

// NOTE(long): Depending on the view, Long_Lister_Render may or may not run before Long_Index_DrawCodePeek.
// This will signal the order of execution to handle lister->set_vertical_focus_to_item properly.
#define LONG_LISTER_TOOLTIP_SPECIAL_VALUE 0xBADA55 // BADASS :)
global i32 long_lister_tooltip_peek = 0;

#endif //4CODER_LONG_LISTER_H

/* date = March 3rd 2024 3:02 pm */

#ifndef FCODER_LONG_LISTER_H
#define FCODER_LONG_LISTER_H

enum Long_Lister_HeaderType
{
    Long_Header_None,
    //Long_Header_Path,
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

enum Long_Lister_TooltipType
{
    Long_Tooltip_UnderItem  = 0x80000000,
    Long_Tooltip_NextToItem = 0x80000001,
    //Long_Tooltip_TopLayout  = 0x80000002,
    //Long_Tooltip_BotLayout  = 0x80000003,
};

global i32 long_lister_tooltip_peek = 0;

function void Long_Lister_AddItem(Application_Links* app, Lister* lister, String8 name, String8 tag,
                                  Buffer_ID buffer, i64 pos, u64 index = 0, String8 tooltip = {});
function void Long_Lister_AddBuffer(Application_Links* app, Lister* lister, String8 name, String8 tag, Buffer_ID buffer);

function Lister_Activation_Code Long_Lister_WriteString(Application_Links* app);
function void Long_Lister_Backspace(Application_Links* app);
function void Long_Lister_Backspace_Path(Application_Links* app);
function void Long_Lister_Navigate(Application_Links* app, View_ID view, Lister* lister, i32 delta);
function void Long_Lister_Navigate_NoSelect(Application_Links* app, View_ID view, Lister* lister, i32 delta);

function void    Long_Lister_Render(Application_Links* app, Frame_Info frame, View_ID view);
function void Long_Lister_RenderHUD(Long_Render_Context* ctx, Lister* lister, b32 show_file_bar);
function Lister_Result Long_Lister_Run(Application_Links* app, Lister* lister, b32 auto_select_first = 1);

#define Long_Lister_IsResultValid(result) (!(result).canceled && (result).user_data)

#endif //4CODER_LONG_LISTER_H

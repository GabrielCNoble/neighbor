#include "ui.h"
#include "in.h"
#include "r_draw.h"
#include <stdio.h>
#include <stdint.h>


void ui_Init()
{
    igCreateContext(NULL);
    ImGuiIO *io = igGetIO();

    unsigned char *pixels;
    int width;
    int height;
    ImFontAtlas_GetTexDataAsRGBA32(io->Fonts, &pixels, &width, &height, NULL);
}

void ui_Shutdown()
{

}

void ui_BeginFrame()
{
    ImGuiIO *io = igGetIO();
    struct r_view_t *view = r_GetViewPointer();
    io->DisplaySize.x = view->viewport.width;
    io->DisplaySize.y = view->viewport.height;
    igNewFrame();
}

void ui_EndFrame()
{
    igRender();
    ImDrawData *draw_data = igGetDrawData();

    for(uint32_t cmd_list_index = 0; cmd_list_index < draw_data->CmdListsCount; cmd_list_index++)
    {
        ImDrawList *cmd_list = draw_data->CmdLists[cmd_list_index];

//        for(uint32_T )
    }
}













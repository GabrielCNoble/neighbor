#include "ui.h"
#include "in.h"
#include "r_draw.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

uint32_t ui_vertice_count = 0;
struct r_i_vertex_t *ui_vertices = NULL;
uint32_t ui_indices_count = 0;
uint32_t *ui_indices = NULL;

mat4_t ui_projection_matrix;
mat4_t ui_inv_view_matrix;
struct r_texture_h ui_font_texture;

void ui_Init()
{
    igCreateContext(NULL);
    ImGuiIO *io = igGetIO();

    unsigned char *pixels;
    int width;
    int height;
    ImFontAtlas_GetTexDataAsRGBA32(io->Fonts, &pixels, &width, &height, NULL);
    
    struct r_texture_description_t texture_description = {};
    texture_description.extent.width = width;
    texture_description.extent.height = height;
    texture_description.extent.depth = 1;
    texture_description.image_type = VK_IMAGE_TYPE_2D;
    texture_description.format = VK_FORMAT_R8G8B8A8_UNORM;
    texture_description.mip_levels = 1;
    texture_description.sampler_params.mipmap_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    texture_description.sampler_params.addr_mode_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    texture_description.sampler_params.addr_mode_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    texture_description.sampler_params.addr_mode_w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    texture_description.sampler_params.min_filter = VK_FILTER_NEAREST;
    texture_description.sampler_params.mag_filter = VK_FILTER_NEAREST;
    
    ui_font_texture = r_CreateTexture(&texture_description);
    struct r_texture_t *texture = r_GetTexturePointer(ui_font_texture);
    r_FillImageChunk(texture->image, pixels, NULL);
    r_SetImageLayout(texture->image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void ui_Shutdown()
{

}

void ui_BeginFrame()
{
    ImGuiIO *io = igGetIO();
    struct r_view_t *view = r_GetViewPointer();
    int32_t mouse_x;
    int32_t mouse_y;
    
    io->DisplaySize.x = view->viewport.width;
    io->DisplaySize.y = view->viewport.height;
    io->DeltaTime = 1.0 / 60.0;
    io->MouseDown[0] = in_GetMouseState(IN_MOUSE_BUTTON_LEFT) & IN_INPUT_STATE_PRESSED;
    in_GetMousePos(&mouse_x, &mouse_y);
    
    io->MousePos.x = mouse_x;
    io->MousePos.y = mouse_y;
    
    mat4_t_ortho(&ui_projection_matrix, view->viewport.width, view->viewport.height, 0.0, 1.0);
    mat4_t_identity(&ui_inv_view_matrix);
    
    ui_inv_view_matrix.rows[1].y = -1.0;
    ui_inv_view_matrix.rows[3].x = -view->viewport.width / 2.0;
    ui_inv_view_matrix.rows[3].y = view->viewport.height / 2.0;
    
    igNewFrame();
}

void ui_EndFrame()
{
    igRender();
    ImDrawData *draw_data = igGetDrawData();
    struct r_view_t *view = r_GetViewPointer();

    if(draw_data->TotalVtxCount > ui_vertice_count)
    {
        ui_vertice_count = draw_data->TotalVtxCount;
        ui_vertices = realloc(ui_vertices, sizeof(struct r_i_vertex_t) * ui_vertice_count);
    }

    if(draw_data->TotalIdxCount > ui_indices_count)
    {
        ui_indices_count = draw_data->TotalIdxCount;
        ui_indices = realloc(ui_indices, sizeof(uint32_t ) * ui_indices_count);
    }

    r_i_BeginSubmission(&ui_inv_view_matrix, &ui_projection_matrix);
    r_i_SetTexture(ui_font_texture);
    r_i_SetDepthTest(VK_FALSE);
    for(uint32_t cmd_list_index = 0; cmd_list_index < draw_data->CmdListsCount; cmd_list_index++)
    {
        ImDrawList *cmd_list = draw_data->CmdLists[cmd_list_index];
        ImDrawIdx *indices = cmd_list->IdxBuffer.Data;
        ImDrawVert *verts = cmd_list->VtxBuffer.Data;

        uint32_t vertex_offset = 0;
        uint32_t index_offset = 0;

        for(uint32_t vert_index = 0; vert_index < cmd_list->VtxBuffer.Size; vert_index++, vertex_offset++)
        {
            ui_vertices[vertex_offset].position.x = verts[vert_index].pos.x;
            ui_vertices[vertex_offset].position.y = verts[vert_index].pos.y;
            ui_vertices[vertex_offset].position.z = 0.0;
            ui_vertices[vertex_offset].position.w = 1.0;
            
            ui_vertices[vertex_offset].tex_coords.x = verts[vert_index].uv.x;
            ui_vertices[vertex_offset].tex_coords.y = verts[vert_index].uv.y;
            ui_vertices[vertex_offset].tex_coords.z = 0.0;
            ui_vertices[vertex_offset].tex_coords.w = 0.0;

            ui_vertices[vertex_offset].color.x = (float)(verts[vert_index].col & 0xff) / 255.0;
            ui_vertices[vertex_offset].color.y = (float)((verts[vert_index].col >> 8) & 0xff) / 255.0;
            ui_vertices[vertex_offset].color.z = (float)((verts[vert_index].col >> 16) & 0xff) / 255.0;
            ui_vertices[vertex_offset].color.w = (float)((verts[vert_index].col >> 24) & 0xff) / 255.0;
        }
        
        vertex_offset = r_i_UploadVertices(ui_vertices, vertex_offset, 1);
        
        for(uint32_t index = 0; index < cmd_list->IdxBuffer.Size; index++, index_offset++)
        {
            ui_indices[index_offset] = indices[index];
        }
        
        index_offset = r_i_UploadIndices(ui_indices, index_offset, 1);
        
        for(uint32_t cmd_index = 0; cmd_index < cmd_list->CmdBuffer.Size; cmd_index++)
        {
            ImDrawCmd *draw_cmd = cmd_list->CmdBuffer.Data + cmd_index;
            r_i_SetScissor(draw_cmd->ClipRect.x, draw_cmd->ClipRect.y, 
                           draw_cmd->ClipRect.z - draw_cmd->ClipRect.x, draw_cmd->ClipRect.w - draw_cmd->ClipRect.y);
            r_i_DrawTris(vertex_offset, index_offset + draw_cmd->IdxOffset, draw_cmd->ElemCount, 1, 1);
        }
    }
    r_i_SetDepthTest(VK_TRUE);
    r_i_EndSubmission();
}













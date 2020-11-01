#include "ui.h"
#include "in.h"
#include "lib/dstuff/ds_mem.h"
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
mat4_t ui_view_projection_matrix;
struct r_texture_t *ui_font_texture;
extern struct r_shader_handle_t r_flat_vertex_shader;
extern struct r_shader_handle_t r_flat_fragment_shader;

void ui_Init()
{
    igCreateContext(NULL);
    ImGuiIO *io = igGetIO();
    io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io->ConfigWindowsResizeFromEdges = 1;
    unsigned char *pixels;
    int width;
    int height;
    ImFontAtlas_GetTexDataAsRGBA32(io->Fonts, &pixels, &width, &height, NULL);
    
    io->KeyMap[ImGuiKey_Tab] = SDL_SCANCODE_TAB;
    io->KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
    io->KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
    io->KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
    io->KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
    io->KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
    io->KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
    io->KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
    io->KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
    io->KeyMap[ImGuiKey_Insert] = SDL_SCANCODE_INSERT;
    io->KeyMap[ImGuiKey_Delete] = SDL_SCANCODE_DELETE;
    io->KeyMap[ImGuiKey_Backspace] = SDL_SCANCODE_BACKSPACE;
    io->KeyMap[ImGuiKey_Space] = SDL_SCANCODE_SPACE;
    io->KeyMap[ImGuiKey_Enter] = SDL_SCANCODE_RETURN;
    io->KeyMap[ImGuiKey_Escape] = SDL_SCANCODE_ESCAPE;
    io->KeyMap[ImGuiKey_KeyPadEnter] = SDL_SCANCODE_RETURN2;
    io->KeyMap[ImGuiKey_A] = SDL_SCANCODE_A;
    io->KeyMap[ImGuiKey_C] = SDL_SCANCODE_C;
    io->KeyMap[ImGuiKey_V] = SDL_SCANCODE_V;
    io->KeyMap[ImGuiKey_X] = SDL_SCANCODE_X;
    io->KeyMap[ImGuiKey_Y] = SDL_SCANCODE_Y;
    io->KeyMap[ImGuiKey_Z] = SDL_SCANCODE_Z;
    
    /*
    ImGuiKey_Tab,
    ImGuiKey_LeftArrow,
    ImGuiKey_RightArrow,
    ImGuiKey_UpArrow,
    ImGuiKey_DownArrow,
    ImGuiKey_PageUp,
    ImGuiKey_PageDown,
    ImGuiKey_Home,
    ImGuiKey_End,
    ImGuiKey_Insert,
    ImGuiKey_Delete,
    ImGuiKey_Backspace,
    ImGuiKey_Space,
    ImGuiKey_Enter,
    ImGuiKey_Escape,
    ImGuiKey_KeyPadEnter,
    ImGuiKey_A,
    ImGuiKey_C,
    ImGuiKey_V,
    ImGuiKey_X,
    ImGuiKey_Y,
    ImGuiKey_Z,
    */
    
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
    r_FillImageChunk(ui_font_texture->image, pixels, NULL);
    
    
//    union r_command_buffer_h command_buffer;
//    VkImageMemoryBarrier memory_barrier = {};
//    
//    memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
//    memory_barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
//    memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    
    
    
    
//    r_SetImageLayout(texture->image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    ImTextureID font_texture_id = (void *)ui_font_texture;
    ImFontAtlas_SetTexID(io->Fonts, font_texture_id);
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
    char *text_input;
    
    uint8_t *keyboard_state = in_GetKeyboardState();
    
    for(uint32_t scancode_index = SDL_SCANCODE_UNKNOWN; scancode_index < SDL_NUM_SCANCODES; scancode_index++)
    {
        io->KeysDown[scancode_index] = keyboard_state[scancode_index];
    }
        
    in_TextInput(io->WantTextInput);
    if(io->WantTextInput)
    {
        text_input = in_GetInputText();
        ImGuiIO_AddInputCharactersUTF8(io, text_input);
    }

    io->DisplaySize.x = view->viewport.width;
    io->DisplaySize.y = view->viewport.height;
    io->DeltaTime = 1.0 / 60.0;
    io->MouseDown[0] = in_GetMouseState(IN_MOUSE_BUTTON_LEFT) & IN_INPUT_STATE_PRESSED;
    io->MouseDown[1] = in_GetMouseState(IN_MOUSE_BUTTON_RIGHT) & IN_INPUT_STATE_PRESSED;
    io->MouseDown[2] = in_GetMouseState(IN_MOUSE_BUTTON_MIDDLE) & IN_INPUT_STATE_PRESSED;
    in_GetMousePos(&mouse_x, &mouse_y);
    
    io->MousePos.x = mouse_x;
    io->MousePos.y = mouse_y;
    
    mat4_t_ortho(&ui_projection_matrix, view->viewport.width, view->viewport.height, 0.0, 1.0);
    mat4_t_identity(&ui_inv_view_matrix);
    
    ui_inv_view_matrix.rows[1].y = -1.0;
    ui_inv_view_matrix.rows[3].x = -view->viewport.width / 2.0;
    ui_inv_view_matrix.rows[3].y = view->viewport.height / 2.0;
    
    mat4_t_mul(&ui_view_projection_matrix, &ui_inv_view_matrix, &ui_projection_matrix);
    
    igNewFrame();
    
//    igShowDemoWindow(NULL);
}

void ui_EndFrame()
{
    igRender();
    ImDrawData *draw_data = igGetDrawData();
    struct r_view_t *view = r_GetViewPointer();
    union r_command_buffer_h command_buffer;
//    struct r_begin_submission_info_t begin_info;
    struct r_draw_state_t draw_state = {};
    struct r_submit_info_t submit_info = {};

    if(draw_data->TotalVtxCount > ui_vertice_count)
    {
        ui_vertice_count = draw_data->TotalVtxCount;
        ui_vertices = mem_Realloc(ui_vertices, sizeof(struct r_i_vertex_t) * ui_vertice_count);
    }

    if(draw_data->TotalIdxCount > ui_indices_count)
    {
        ui_indices_count = draw_data->TotalIdxCount;
        ui_indices = mem_Realloc(ui_indices, sizeof(uint32_t ) * ui_indices_count);
    }
    
//    begin_info.inv_view_matrix = ui_inv_view_matrix;
//    begin_info.projection_matrix = ui_projection_matrix;
//    begin_info.framebuffer = r_GetBackbufferHandle();
//    begin_info.viewport = view->viewport;
//    begin_info.scissor = view->scissor;
    
    draw_state.scissor = view->scissor;
    draw_state.line_width = 1.0;
    draw_state.point_size = 1.0;
    draw_state.pipeline_state = &(struct r_pipeline_state_t)
    {
        .input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .rasterizer_state.cull_mode = VK_CULL_MODE_NONE,
        .rasterizer_state.polygon_mode = VK_POLYGON_MODE_FILL,
        .rasterizer_state.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depth_state.compare_op = VK_COMPARE_OP_LESS_OR_EQUAL,
        .depth_state.test_enable = VK_TRUE,
        .depth_state.write_enable = VK_TRUE,
        .vertex_shader = r_flat_vertex_shader,
        .fragment_shader = r_flat_fragment_shader,
    };
    
    command_buffer = r_AllocateCommandBuffer();
    r_BeginCommandBuffer(command_buffer, view);
    r_BeginRenderPass(command_buffer, &view->scissor, r_GetBackbufferHandle());
    
    r_SetDrawState(command_buffer, &draw_state);
    r_PushConstants(command_buffer, VK_SHADER_STAGE_VERTEX_BIT, sizeof(mat4_t), 0, &ui_view_projection_matrix);
    
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
        
        vertex_offset = r_FillTempVertices(ui_vertices, sizeof(struct r_i_vertex_t), vertex_offset);
        
        for(uint32_t index = 0; index < cmd_list->IdxBuffer.Size; index++, index_offset++)
        {
            ui_indices[index_offset] = indices[index];
        }
        
        index_offset = r_FillTempIndices(ui_indices, sizeof(uint32_t), index_offset);
        
        for(uint32_t cmd_index = 0; cmd_index < cmd_list->CmdBuffer.Size; cmd_index++)
        {
            ImDrawCmd *draw_cmd = cmd_list->CmdBuffer.Data + cmd_index;
            struct r_texture_t *texture = (struct r_texture_t *)draw_cmd->TextureId;
            
            draw_state.scissor.offset.x = draw_cmd->ClipRect.x;
            draw_state.scissor.offset.y = draw_cmd->ClipRect.y;
            draw_state.scissor.extent.width = draw_cmd->ClipRect.z - draw_cmd->ClipRect.x;
            draw_state.scissor.extent.height = draw_cmd->ClipRect.w - draw_cmd->ClipRect.y;
            
            draw_state.texture_state = &(struct r_texture_state_t)
            {
                .texture_count = 1,
                .textures = (struct r_texture_state_binding_t [])
                {
                    [0] = {.binding_index = 0, .texture = texture}
                }
            };
            
            r_SetDrawState(command_buffer, &draw_state);
            r_PushConstants(command_buffer, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(uint32_t), sizeof(mat4_t) + sizeof(float), &(uint32_t){1});

            r_DrawIndexed(command_buffer, index_offset + draw_cmd->IdxOffset, draw_cmd->ElemCount, vertex_offset);
        }
    }
    
    r_EndRenderPass(command_buffer);
    r_EndCommandBuffer(command_buffer);
    struct r_fence_h fence = r_AllocFence();
    r_SubmitCommandBuffers(1, &command_buffer, fence);
    r_vkWaitForFences(1, &fence, VK_TRUE, 0xffffffffffffffff);
}

uint32_t ui_MouseOverUi()
{
    ImGuiIO *io;
    io = igGetIO();
    return io->WantCaptureMouse;
}












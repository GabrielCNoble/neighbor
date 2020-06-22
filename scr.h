#ifndef SCR_H
#define SCR_H

#include <stdint.h>

enum SCR_ARGS_TYPE
{
    SCR_ARG_TYPE_UINT32 = 0,
    SCR_ARG_TYPE_FLOAT,
    SCR_ARG_TYPE_STRING,
};

union scr_value_t
{
    char *string_value;
    uint32_t int_value;
    float float_value;
};

struct scr_arg_t
{
    uint32_t type;
    union scr_value_t value;
};

struct scr_script_run_t
{
    char *function_name;
    uint32_t arg_count;
    struct scr_arg_t *args;
};

struct scr_script_t
{
    void (*prepare_function)(struct scr_script_t *script, void *data, struct scr_script_run_t *run_params);
    void *module;
    char *name;
};

struct scr_script_h
{
    uint32_t index;
};

#define SCR_INVALID_SCRIPT_INDEX 0xffffffff
#define SCR_SCRIPT_HANDLE(index) (struct scr_script_h){index}
#define SCR_INVALID_SCRIPT_HANDLE SCR_SCRIPT_HANDLE(SCR_INVALID_SCRIPT_INDEX)

struct scr_queued_script_t
{
    struct scr_script_h script;
    uint8_t data[64];
};


#ifdef __cplusplus
extern "C"
{
#endif

void scr_Init();

void scr_Shutdown();

struct scr_script_h scr_CreateScript(char *source, char *name, void (*prepare_function)(struct scr_script_t *script, void *data, struct scr_script_run_t *run_params));

void scr_DestroyScript(struct scr_script_h script_handle);

struct scr_script_t *scr_GetScriptPointer(struct scr_script_h script_handle);

void scr_UpdateScripts(float delta_time);

void scr_QueueScript(struct scr_script_h script_handle, void *data, uint32_t data_size);

void scr_RunScript(struct scr_script_h script_handle, void *data);

struct scr_script_arg_t *scr_AllocateScriptArgs(uint32_t count);

//void scr_RegisterObjectType(char *type, uint32_t byte_size)

#ifdef __cplusplus
}
#endif

#endif // SCR_H



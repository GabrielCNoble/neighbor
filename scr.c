#include "scr.h"
#include "lib/angelscript/include/angelscript.h"
#include "lib/dstuff/ds_stack_list.h"
#include "lib/dstuff/ds_list.h"
#include "lib/dstuff/ds_mem.h"
#include "lib/dstuff/ds_vector.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>


struct scr_string_t
{
    char *data;
    uint32_t length;
};

enum SCR_LIST_ITEM_TYPES
{
    SCR_LIST_ITEM_TYPE_INT = 0,
    SCR_LIST_ITEM_TYPE_FLOAT,
    SCR_LIST_ITEM_TYPE_STRING,
};

union scr_printf_arg_value_t
{
    uint32_t int_value;
    float float_value;
    struct scr_string_t *string_value;
};

struct scr_printf_arg_t
{
    uint32_t type;
    union scr_value_t value;
};

struct scr_printf_arg_list_t
{
    uint32_t length;
    struct scr_printf_arg_t *args;
};

void *scr_ConstructString();
void *scr_ConstructStringInit(void *other_index);
void scr_DestructString(void *this_index);
void *scr_StringOpAssign(void *this_index, void *other_index);

class scr_StringFactory : public asIStringFactory 
{
    public:
        
    struct stack_list_t *strings;
        
    scr_StringFactory(struct stack_list_t *strings)
    {
        this->strings = strings;
    };
    
    ~scr_StringFactory(){};
    
    const void *GetStringConstant(const char *data, asUINT length)
    {
        uint64_t string_index;
        struct scr_string_t *string;
        
        string_index = add_stack_list_element(strings, NULL);
        string = (struct scr_string_t *)get_stack_list_element(strings, string_index);
        
        if(string->length < length)
        {
            string->data = (char *)realloc(string->data, length + 1);
            string->length = length;
        }
        
        memcpy(string->data, data, length);
        string->data[string->length] = '\0';
        return (void *)(string_index + 1);
    }
    
    int ReleaseStringConstant(const void *str)
    {
        scr_DestructString((void *)str);
        return 0;
    }
    
    int GetRawStringData(const void *str, char *data, asUINT *length) const
    {
        uint64_t string_index = (uint64_t)str;
        struct scr_string_t *string = (struct scr_string_t *)get_stack_list_element((struct stack_list_t *)strings, string_index - 1);
        *length = string->length;
        if(data)
        {
            memcpy(data, string->data, string->length);
        }
        return 0;
    }
};



asIScriptEngine *scr_script_engine = NULL;
asIScriptContext *scr_script_context = NULL;
struct stack_list_t scr_scripts;
struct list_t scr_queued_scripts;
scr_StringFactory *scr_string_factory;
struct scr_arg_t *scr_args;
struct stack_list_t scr_script_strings;
struct stack_list_t scr_script_lists;

void *scr_ConstructString()
{
    uint64_t string_index = (uint64_t)add_stack_list_element(&scr_script_strings, NULL);
    return (void *)(string_index + 1);
}

struct scr_string_t *scr_GetStringPointer(uint64_t string_index)
{
    return (struct scr_string_t *)get_stack_list_element(&scr_script_strings, string_index - 1);
}

void *scr_ConstructStringInit(void *other_index)
{
    uint64_t string_index = (uint64_t)scr_ConstructString();
    return scr_StringOpAssign((void *)string_index, other_index);
}

void *scr_StringOpAssign(void *this_index, void *other_index)
{
    struct scr_string_t *this_string = scr_GetStringPointer((uint64_t)this_index);
    struct scr_string_t *other_string = scr_GetStringPointer((uint64_t)other_index);
    
    if(this_string->length < other_string->length)
    {
        this_string->length = other_string->length;
        this_string->data = (char *)realloc(this_string->data, other_string->length + 1);
    }
    
    if(other_string->length)
    {
        memcpy(this_string->data, other_string->data, other_string->length);
        this_string->data[this_string->length] = '\0';
    }
    
    return this_index;
}

void scr_DestructString(void *this_index)
{
    remove_stack_list_element(&scr_script_strings, (uint64_t)this_index - 1);
    /* nothing to do here for now... */
}





void *scr_ConstructPrintfArgs(asBYTE *init_buffer)
{
    uint64_t list_index;
    struct scr_printf_arg_list_t *list;
    uint32_t arg_count;
    
//    list_index = add_stack_list_element(&scr_script_lists, NULL);
//    list = (struct scr_printf_arg_list_t *)get_stack_list_element(&scr_script_lists, list_index);
    
    arg_count = *(asUINT *)init_buffer;
    
//    if(list->)
    
    
    
    return (void *)list_index + 1;
}

struct scr_list_t *scr_GetListPointer(void *list_index)
{
    return (struct scr_list_t *)get_stack_list_element(&scr_script_lists, (uint64_t)list_index - 1);
}

#ifdef __cplusplus
extern "C"
{
#endif


void scr_MessageCallback(const asSMessageInfo *info, void *param)
{
    char *type = "ERROR";
    switch(info->type)
    {
        case asMSGTYPE_WARNING:
            type = "WARNING";
        break;
        
        case asMSGTYPE_INFORMATION:
            type = "INFO";
        break;
    }
    
    printf("%s: %s (%d, %d) - %s\n", type, info->section, info->row, info->col, info->message);
}

void scr_Print(void *string_index)
{
    struct scr_string_t *string = scr_GetStringPointer((uint64_t)string_index);
    printf("%s\n", string->data);
}



uint32_t scr_Rand()
{
    return rand();
}

void scr_Init()
{
    scr_script_engine = asCreateScriptEngine();
    scr_script_context = scr_script_engine->CreateContext();
    scr_scripts = create_stack_list(sizeof(struct scr_script_t), 128);
    scr_queued_scripts = create_list(sizeof(struct scr_queued_script_t), 64);
    scr_script_strings = create_stack_list(sizeof(struct scr_string_t ), 128);
    scr_scripts = create_stack_list(sizeof(struct scr_script_t), 128);
    scr_string_factory = new scr_StringFactory(&scr_script_strings);
    scr_args = (struct scr_arg_t *)mem_Calloc(4096, sizeof(struct scr_arg_t));
    
    scr_script_engine->SetMessageCallback(asFUNCTION(scr_MessageCallback), 0, asCALL_CDECL);
    scr_script_engine->RegisterObjectType("string", sizeof(char *), asOBJ_REF | asOBJ_NOCOUNT);
    scr_script_engine->RegisterObjectBehaviour("string", asBEHAVE_FACTORY, "string @f()", asFUNCTION(scr_ConstructString), asCALL_CDECL);
    scr_script_engine->RegisterObjectBehaviour("string", asBEHAVE_FACTORY, "string @f(const string &in)", asFUNCTION(scr_ConstructStringInit), asCALL_CDECL);
    scr_script_engine->RegisterObjectMethod("string", "string &opAssign(const string &in)", asFUNCTION(scr_StringOpAssign), asCALL_CDECL_OBJFIRST);
    scr_script_engine->RegisterStringFactory("string", scr_string_factory);
    
    scr_script_engine->RegisterObjectType("vec2_t", sizeof(vec2_t), asOBJ_VALUE | asOBJ_POD);
    scr_script_engine->RegisterObjectProperty("vec2_t", "float x", asOFFSET(vec2_t, x));
    scr_script_engine->RegisterObjectProperty("vec2_t", "float y", asOFFSET(vec2_t, y));
    
    scr_script_engine->RegisterObjectType("vec3_t", sizeof(vec3_t), asOBJ_VALUE | asOBJ_POD);
    scr_script_engine->RegisterObjectProperty("vec3_t", "float x", asOFFSET(vec3_t, x));
    scr_script_engine->RegisterObjectProperty("vec3_t", "float y", asOFFSET(vec3_t, y));
    scr_script_engine->RegisterObjectProperty("vec3_t", "float z", asOFFSET(vec3_t, z));
    
    scr_script_engine->RegisterObjectType("vec4_t", sizeof(vec4_t), asOBJ_VALUE | asOBJ_POD);
    scr_script_engine->RegisterObjectProperty("vec4_t", "float x", asOFFSET(vec4_t, x));
    scr_script_engine->RegisterObjectProperty("vec4_t", "float y", asOFFSET(vec4_t, y));
    scr_script_engine->RegisterObjectProperty("vec4_t", "float z", asOFFSET(vec4_t, z));
    scr_script_engine->RegisterObjectProperty("vec4_t", "float w", asOFFSET(vec4_t, w));
    
    
    scr_script_engine->RegisterGlobalFunction("void print(string &in)", asFUNCTION(scr_Print), asCALL_CDECL);
    scr_script_engine->RegisterGlobalFunction("uint rand()", asFUNCTION(scr_Rand), asCALL_CDECL);
}

void scr_Shutdown()
{
    
}

struct scr_script_h scr_CreateScript(char *source, char *name, void (*prepare_function)(struct scr_script_t *script, void *data, struct scr_script_run_t *run_params))
{
    struct scr_script_t *script;
    struct scr_script_h script_handle = SCR_INVALID_SCRIPT_HANDLE;
    asIScriptModule *module;

    script_handle.index = add_stack_list_element(&scr_scripts, NULL);
    script = (struct scr_script_t *)get_stack_list_element(&scr_scripts, script_handle.index);
    script->prepare_function = prepare_function;
    script->name = strdup(name);
    module = scr_script_engine->GetModule(name, asGM_ALWAYS_CREATE);
    module->AddScriptSection(name, source);
    module->Build();
    script->module = module;
    
    return script_handle;
}

void scr_DestroyScript(struct scr_script_h script_handle)
{
    struct scr_script_t *script;
    script = scr_GetScriptPointer(script_handle);
    
    if(script)
    {
        script->prepare_function = NULL;
        remove_stack_list_element(&scr_scripts, script_handle.index);
    }
}

struct scr_script_t *scr_GetScriptPointer(struct scr_script_h script_handle)
{
    struct scr_script_t *script = NULL;
    
    script = (struct scr_script_t *)get_stack_list_element(&scr_scripts, script_handle.index);
    if(script && !script->prepare_function)
    {
        script = NULL;
    }
    
    return script;
}

void scr_UpdateScripts(float delta_time)
{
    struct scr_queued_script_t *script;
    
    for(uint32_t exec_index = 0; exec_index < scr_queued_scripts.cursor; exec_index++)
    {
        script = (struct scr_queued_script_t *)get_list_element(&scr_queued_scripts, exec_index);
        scr_RunScript(script->script, script->data);
    }
    
    scr_queued_scripts.cursor = 0;
}

void scr_QueueScript(struct scr_script_h script_handle, void *data, uint32_t data_size)
{
    struct scr_queued_script_t *script;
    script = (struct scr_queued_script_t *)get_list_element(&scr_queued_scripts, add_list_element(&scr_queued_scripts, NULL));
    
    script->script = script_handle;
    if(data && data_size)
    {
        memcpy(script->data, data, data_size);
    }
}

void scr_RunScript(struct scr_script_h script_handle, void *data)
{
    struct scr_script_t *script;
    struct scr_script_run_t run;
    asIScriptFunction *function;
    asIScriptModule *module;
    script = scr_GetScriptPointer(script_handle);
    if(script)
    {
        if(!script->prepare_function)
        {
            printf("scr_RunScript: script doesn't have a prepare function, so it won't get executed\n");
            return;
        }
        
        run.args = scr_args;
        run.arg_count = 0;
        run.function_name = NULL;
        script->prepare_function(script, data, &run);
        
        if(!run.function_name)
        {
            printf("scr_RunScript: script didn't provide a function name to be executed\n");
            return;
        }
        
        module = (asIScriptModule *)script->module;
        function = module->GetFunctionByName(run.function_name);
        scr_script_context->Prepare(function);
        for(uint32_t arg_index = 0; arg_index < run.arg_count; arg_index++)
        {
            struct scr_arg_t *arg = run.args + arg_index;
            
            switch(arg->type)
            {
                case SCR_ARG_TYPE_UINT32:
                    scr_script_context->SetArgDWord(arg_index, arg->value.int_value);
                break;
                
                case SCR_ARG_TYPE_FLOAT:
                    scr_script_context->SetArgFloat(arg_index, arg->value.float_value);
                break;
                
                case SCR_ARG_TYPE_STRING:   
                {
                    uint64_t string_index = (uint64_t)scr_ConstructString();
                    struct scr_string_t *string = scr_GetStringPointer(string_index);
                    string->data = arg->value.string_value;
                    string->length = strlen(string->data);
                    scr_script_context->SetArgObject(arg_index, (void *)string_index);
                }
                break;
            }
        }
        scr_script_context->Execute();
        scr_script_context->Unprepare();
    }
}

#ifdef __cplusplus
}
#endif













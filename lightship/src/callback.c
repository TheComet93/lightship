#include <stdlib.h>
#include <string.h>
#include "lightship/callback.h"
#include "util/linked_list.h"
#include "util/plugin.h"
#include "util/string.h"

char callback_register(struct list_t* cb_list,
                       struct plugin_t* plugin,
                       const char* name,
                       intptr_t exec)
{
    struct callback_t* callback;
    char* full_name;

    /* check if callback is already registered */
    full_name = cat_strings(3, plugin->info.name, ".", name);
    if(callback_get(cb_list, full_name))
    {
        free(full_name);
        return 0;
    }

    /* create callback and add to list */
    callback = (struct callback_t*)malloc(sizeof(struct callback_t));
    callback->name = full_name;
    callback->exec = exec;
    list_push(cb_list, callback);
    
    return 1;
}

char callback_unregister(struct list_t* cb_list,
                         struct plugin_t* plugin,
                         const char* name)
{
    char* full_name;
    
    /* remove callback from list */
    full_name = cat_strings(3, plugin->info.name, ".", name);
    {
        LIST_FOR_EACH_ERASE(cb_list, struct callback_t, callback)
        {
            if(strcmp(callback->name, name) == 0)
            {
                free(callback->name);
                free(callback);
                list_erase_node(cb_list, node);
                return 1;
            }
        }
    }

    return 0;
}

void callback_unregister_all(struct list_t* cb_list,
                             struct plugin_t* plugin)
{
    char* name = cat_strings(2, plugin->info.name, ".");
    int len = strlen(plugin->info.name) + 1;
    {
        LIST_FOR_EACH_ERASE(cb_list, struct callback_t, callback)
        {
            if(strncmp(callback->name, name, len) == 0)
            {
                free(callback->name);
                free(callback);
                list_erase_node(cb_list, node);
            }
        }
    }
}

intptr_t callback_get(struct list_t* cb_list, const char* name)
{
    LIST_FOR_EACH(cb_list, struct callback_t, callback)
    {
        if(strcmp(callback->name, name) == 0)
            return callback->exec;
    }
    return 0;
}

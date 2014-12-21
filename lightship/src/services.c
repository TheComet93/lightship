#include "lightship/services.h"
#include "util/memory.h"
#include "util/plugin.h"
#include "util/string.h"
#include <stdlib.h>
#include <string.h>

struct list_t g_services;

void services_init(void)
{
    list_init_list(&g_services);
}

char service_register(struct plugin_t* plugin,
                      const char* name,
                      intptr_t exec)
{
    struct service_t* service;
    char* full_name;

    /* check if service is already registered */
    full_name = cat_strings(3, plugin->info.name, ".", name);
    if(service_get(full_name))
    {
        FREE(full_name);
        return 0;
    }

    /* create service and add to list */
    service = (struct service_t*)MALLOC(sizeof(struct service_t));
    service->name = full_name;
    service->exec = exec;
    list_push(&g_services, service);
    
    return 1;
}

char service_unregister(struct plugin_t* plugin,
                        const char* name)
{
    char* full_name;
    char success = 0;
    
    /* remove service from list */
    full_name = cat_strings(3, plugin->info.name, ".", name);
    {
        LIST_FOR_EACH_ERASE(&g_services, struct service_t, service)
        {
            if(strcmp(service->name, full_name) == 0)
            {
                FREE(service->name);
                FREE(service);
                list_erase_node(&g_services, node);
                success = 1;
                break;
            }
        }
    }
    FREE(full_name);

    return success;
}

void service_unregister_all(struct plugin_t* plugin)
{
    char* name = cat_strings(2, plugin->info.name, ".");
    int len = strlen(plugin->info.name);
    {
        LIST_FOR_EACH_ERASE(&g_services, struct service_t, service)
        {
            if(strncmp(service->name, name, len) == 0)
            {
                FREE(service->name);
                FREE(service);
                list_erase_node(&g_services, node);
            }
        }
    }
    FREE(name);
}

intptr_t service_get(const char* name)
{
    LIST_FOR_EACH(&g_services, struct service_t, service)
    {
        if(strcmp(service->name, name) == 0)
            return service->exec;
    }
    return 0;
}

#include "util/config.h"
#include "util/events.h"
#include "util/plugin.h"
#include "util/string.h"
#include "util/memory.h"
#include <string.h>
#include <stdlib.h>

static struct list_t g_events;
EVENT_C(evt_log)
EVENT_C(evt_log_indent)
EVENT_C(evt_log_unindent)

/*!
 * @brief Unregisters any listeners that belong to the specified name_space from
 * the specified event.
 * @param event The event to unregister the listeners from.
 * @param name_space The name_space to search for.
 */
static void
event_unregister_all_listeners_of_name_space(struct event_t* event,
                                            const char* name_space);

/*!
 * @brief Returns the full name of the event using a plugin object and event name.
 * @note The returned string must be freed manually.
 */
static char*
event_get_full_name(const struct plugin_t* plugin, const char* name);

/*!
 * @brief Returns the name_space name of the event using a plugin object.
 * @note The returned string must be freed manually.
 */
static char*
event_get_name_space_name(const struct plugin_t* plugin);

/*!
 * @brief Frees an event object.
 * @note This does not remove it from the list.
 */
static void
event_free(struct event_t* event);

/*!
 * @brief Frees an event listener object.
 * @note This does not remove it from the list.
 */
static void
event_listener_free(struct event_listener_t* listener);

/*!
 * @brief Allocates and registers an event globally with the specified name.
 * @note No checks for duplicates are performed. This is an internal function.
 * @param[in] full_name The full name of the event.
 * @note The event object owns **full_name** after this call and will free it
 * when the event is destroyed.
 * @return The new event object.
 */
static struct event_t*
event_malloc_and_register(char* full_name);

void
events_init(void)
{
    char* name;

    list_init_list(&g_events);
    
    /* ----------------------------
     * Register built-in events 
     * --------------------------*/
    
    /* All logging events should be done through this event. */
    name = malloc_string(BUILTIN_NAMESPACE_NAME ".log");
    evt_log = event_malloc_and_register(name);
    name = malloc_string(BUILTIN_NAMESPACE_NAME ".log_indent");
    evt_log_indent = event_malloc_and_register(name);
    name = malloc_string(BUILTIN_NAMESPACE_NAME ".log_unindent");
    evt_log_unindent = event_malloc_and_register(name);
}

void
events_deinit(void)
{
    LIST_FOR_EACH_ERASE(&g_events, struct event_t, event)
    {
        event_free(event);
        list_erase_node(&g_events, node);
    }
}

struct event_t*
event_create(const struct plugin_t* plugin, const char* name)
{

    /* check for duplicate event names */
    char* full_name = event_get_full_name(plugin, name);
    if(event_get(full_name))
    {
        FREE(full_name);
        return NULL;
    }
    
    return event_malloc_and_register(full_name);
}

static struct event_t*
event_malloc_and_register(char* full_name)
{
    /* create new event and register to global list of events */
    struct event_t* event = (struct event_t*)MALLOC(sizeof(struct event_t));
    event->name = full_name;
    unordered_vector_init_vector(&event->listeners, sizeof(struct event_listener_t));
    list_push(&g_events, event);
    return event;
}

char
event_destroy(struct event_t* event_delete)
{
    LIST_FOR_EACH(&g_events, struct event_t, event)
    {
        if(event == event_delete)
        {
            event_free(event);
            list_erase_node(&g_events, node);
            return 1;
        }
    }
    return 0;
}

void
event_destroy_plugin_event(const struct plugin_t* plugin, const char* name)
{
    char* full_name = event_get_full_name(plugin, name);

    {
        LIST_FOR_EACH(&g_events, struct event_t, event)
        {
            if(strcmp(event->name, full_name) == 0)
            {
                event_free(event);
                list_erase_node(&g_events, node);
                break;
            }
        }
    }
    FREE(full_name);
}

void
event_destroy_all_plugin_events(const struct plugin_t* plugin)
{
    char* name_space = event_get_name_space_name(plugin);
    int len = strlen(name_space);
    LIST_FOR_EACH_ERASE(&g_events, struct event_t, event)
    {
        if(strncmp(event->name, name_space, len) == 0)
        {
            event_free(event);
            list_erase_node(&g_events, node);
        }
    }
    FREE(name_space);
}

struct event_t*
event_get(const char* full_name)
{
    LIST_FOR_EACH(&g_events, struct event_t, event)
    {
        if(strcmp(event->name, full_name) == 0)
            return event;
    }
    return NULL;
}

char
event_register_listener(const struct plugin_t* plugin,
                        const char* event_full_name,
                        event_callback_func callback)
{
    struct event_t* event;
    struct event_listener_t* new_listener;
    char* registering_name_space;
    
    /* get name space name - if NULL was specified as a plugin, make it builtin */
    if(plugin)
        registering_name_space = plugin->info.name;
    else
        registering_name_space = BUILTIN_NAMESPACE_NAME;
    
    /* make sure event exists */
    event = event_get(event_full_name);
    if(event == NULL)
        return 0;
    
    /* make sure plugin hasn't already registered to this event */
    if(plugin)
    {
        UNORDERED_VECTOR_FOR_EACH(&event->listeners, struct event_listener_t, listener)
        {
            if(strcmp(listener->name_space, registering_name_space) == 0)
                return 0;
        }
    }
    
    /* create event listener object */
    new_listener = (struct event_listener_t*)unordered_vector_push_emplace(&event->listeners);
    new_listener->exec = callback;
    /* create and copy string from plugin name */
    new_listener->name_space = cat_strings(2, registering_name_space, ".");
    
    return 1;
}

char
event_unregister_listener(const char* plugin_name, const char* event_name)
{
    struct event_t* event = event_get(event_name);
    if(event == NULL)
        return 0;
    
    {
        UNORDERED_VECTOR_FOR_EACH(&event->listeners, struct event_listener_t, listener)
        {
            if(strcmp(listener->name_space, plugin_name) == 0)
            {
                event_listener_free(listener);
                unordered_vector_erase_element(&event->listeners, listener);
                return 1;
            }
        }
    }
    
    return 0;
}

void
event_unregister_all_listeners(struct event_t* event)
{
    UNORDERED_VECTOR_FOR_EACH(&event->listeners, struct event_listener_t, listener)
    {
        event_listener_free(listener);
    }
    unordered_vector_clear_free(&event->listeners);
}

void
event_unregister_all_listeners_of_plugin(const struct plugin_t* plugin)
{
    /* 
     * For every listener in every event, search for any listener that belongs
     * to the specified plugin
     */
    char* name_space = event_get_name_space_name(plugin);
    {
        LIST_FOR_EACH(&g_events, struct event_t, event)
        {
            event_unregister_all_listeners_of_name_space(event, name_space);
        }
    }
    FREE(name_space);
}

static void
event_unregister_all_listeners_of_name_space(struct event_t* event,
                                            const char* name_space)
{
    int len = strlen(name_space);
    {
        UNORDERED_VECTOR_FOR_EACH(&event->listeners, struct event_listener_t, listener)
        {
            if(strncmp(listener->name_space, name_space, len) == 0)
            {
                event_listener_free(listener);
                UNORDERED_VECTOR_ERASE_IN_FOR_LOOP(&event->listeners, struct event_listener_t, listener);
            }
        }
    }
}

static char*
event_get_full_name(const struct plugin_t* plugin, const char* name)
{
    return cat_strings(3, plugin->info.name, ".", name);
}

static char*
event_get_name_space_name(const struct plugin_t* plugin)
{
    return cat_strings(2, plugin->info.name, ".");
}

static void
event_free(struct event_t* event)
{
    event_unregister_all_listeners(event);
    FREE(event->name);
    unordered_vector_clear_free(&event->listeners);
    FREE(event);
}

static void
event_listener_free(struct event_listener_t* listener)
{
    FREE(listener->name_space);
}

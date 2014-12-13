#include <util/pstdint.h>

/* these must be implemented by the plugin */
struct plugin_t;
typedef void (*plugin_start_func)(struct plugin_t*);
typedef void (*plugin_stop_func)(void);

/*!
 * @brief Programming language the plugin was written in.
 */
typedef enum plugin_programming_language_t
{
    PLUGIN_PROGRAMMING_LANGUAGE_UNSET,
    PLUGIN_PROGRAMMING_LANGUAGE_C,
    PLUGIN_PROGRAMMING_LANGUAGE_CPP
} plugin_programming_language_t;

/*!
 * @brief API version information of the plugin.
 */
typedef struct plugin_api_version_t
{
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
} plugin_api_version_t;

/*!
 * @brief Information about the plugin.
 */
typedef struct plugin_info_t
{
    char* name;
    char* category;
    char* author;
    char* description;
    char* website;
    plugin_programming_language_t language;
    plugin_api_version_t version;
} plugin_info_t;

/* host service functions */
typedef void (*plugin_get_by_name_func)(const char*);
typedef void (*plugin_load_func)(struct plugin_info_t*, plugin_programming_language_t);
typedef void (*plugin_unload_func)(struct plugin_t*);

typedef struct host_services_t
{
    plugin_get_by_name_func plugin_get_by_name;
    plugin_load_func plugin_load;
    plugin_unload_func plugin_unload;
} host_services_t;

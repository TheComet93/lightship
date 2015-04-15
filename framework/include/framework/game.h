#include "framework/config.h"
#include "util/map.h"
#include "util/linked_list.h"

struct net_connection_t;
struct glob_t;

typedef enum game_network_role_e
{
    GAME_CLIENT = 1,
    GAME_HOST = 2
} game_network_role_e;

typedef enum game_state_e
{
    GAME_STATE_TERMINATED,
    GAME_STATE_PAUSED,
    GAME_STATE_RUNNING
} game_state_e;

struct framework_glob_events_t
{
    struct event_t* tick;
    struct event_t* render;
    struct event_t* loop_stats;
    
    struct event_t* log;
    struct event_t* log_indent;
    struct event_t* log_unindent;
};

struct framework_glob_log_t
{
    char indent_level;
};

struct game_t
{
    game_state_e state;
    char* name;
    game_network_role_e network_role;
    struct net_connection_t* connection;
    struct framework_glob_events_t event;
    struct framework_glob_log_t log;
    struct list_t plugins;      /* list of active plugins used by this game */
    struct map_t services;      /* maps service names to active services used by this game */
    struct map_t events;        /* maps event names to active events used by this game */
    struct map_t global_data;   /* maps hashed plugin names to glob structs used by this game */
};

FRAMEWORK_PUBLIC_API struct game_t*
game_create(const char* name, game_network_role_e net_role);

FRAMEWORK_PUBLIC_API void
game_destroy(struct game_t* game);

FRAMEWORK_PUBLIC_API char
game_connect(struct game_t* game, const char* address);

FRAMEWORK_PUBLIC_API void
game_disconnect(struct game_t* game);

FRAMEWORK_PUBLIC_API void
game_run(struct game_t* game);

FRAMEWORK_PUBLIC_API void
game_pause(struct game_t* game);

FRAMEWORK_PUBLIC_API void
game_stop(struct game_t* game);

FRAMEWORK_PUBLIC_API void
games_run_all(void);

void
game_dispatch_stats(uint32_t render_fps, uint32_t tick_fps);

void
game_dispatch_render(void);

void
game_dispatch_tick(void);

#define game_add_global(game, hash, glob) map_insert(&(game)->global_data, hash, glob)
#define game_get_global(game, hash) map_find(&(game)->global_data, hash)
#define game_remove_global(game, hash) map_erase(&(game)->global_data, hash)
#ifndef LIGHTSHIP_FRAMEWORK_GAME_H
#define LIGHTSHIP_FRAMEWORK_GAME_H

#include "framework/config.h"
#include "framework/game.h"
#include "framework/se_api.h"
#include "util/ptree.h"
#include "util/linked_list.h"
#include "util/bst_vector.h"

C_HEADER_BEGIN

struct net_connection_t;
struct context_t;
struct plugin_t;

SERVICE(game_start_wrapper);
SERVICE(game_pause_wrapper);
SERVICE(game_exit_wrapper);

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

struct framework_events_t
{
	struct event_t* start;
	struct event_t* pause;
	struct event_t* stop;

	struct event_t* tick;
	struct event_t* render;
	struct event_t* stats;

	struct event_t* log;
	struct event_t* log_indent;
	struct event_t* log_unindent;

	struct event_t* event_created;
	struct event_t* event_destroyed;
	struct event_t* service_created;
	struct event_t* service_destroyed;
};

struct framework_services_t
{
	struct service_t* start;
	struct service_t* pause;
	struct service_t* stop;
};

struct framework_log_t
{
	char indent_level;
};

struct game_t
{
	game_state_e state;
	char* name;
	struct ptree_t* settings;            /* yaml settings */

	game_network_role_e network_role;
	struct net_connection_t* connection;

	struct plugin_t* core;               /* core plugin providing core functionality */
	struct framework_events_t event;     /* core plugin events are conveniently accessible here */
	struct framework_services_t service; /* core plugin services are conveniently accessible here */
	struct framework_log_t log;

	struct list_t plugins;      /* list of active plugins used by this game */
	struct ptree_t services;    /* service directory of this game */
	struct ptree_t events;      /* event directory of this game */

	struct bstv_t context_store;  /* maps hashed plugin names to context structs used by this game */
};

FRAMEWORK_PUBLIC_API void
game_init(void);

FRAMEWORK_PUBLIC_API void
game_deinit(void);

FRAMEWORK_PUBLIC_API struct game_t*
game_create(const char* name,
			const char* settings_yml_file,
			game_network_role_e net_role);

FRAMEWORK_PUBLIC_API void
game_destroy(struct game_t* game);

FRAMEWORK_PUBLIC_API char
game_connect(struct game_t* game, const char* address);

FRAMEWORK_PUBLIC_API void
game_disconnect(struct game_t* game);

FRAMEWORK_PUBLIC_API void
game_start(struct game_t* game);

FRAMEWORK_PUBLIC_API void
game_pause(struct game_t* game);

FRAMEWORK_PUBLIC_API void
game_exit(struct game_t* game);

FRAMEWORK_PUBLIC_API void
games_run_all(void);

void
game_dispatch_stats(uint32_t render_fps, uint32_t tick_fps);

void
game_dispatch_render(void);

void
game_dispatch_tick(void);

#define game_add_to_context_store(game, hash, context) bstv_insert(&(game)->context_store, hash, context)
#define game_get_from_context_store(game, hash) bstv_find(&(game)->context_store, hash)
#define game_remove_from_context_store(game, hash) bstv_erase(&(game)->context_store, hash)

C_HEADER_END

#endif /* LIGHTSHIP_FRAMEWORK_GAME_H */

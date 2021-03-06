#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "framework/plugin_manager.h"
#include "framework/services.h"
#include "framework/events.h"
#include "framework/plugin.h"
#include "framework/game.h"
#include "util/config.h"
#include "util/linked_list.h"
#include "util/unordered_vector.h"
#include "util/string.h"
#include "util/module_loader.h"
#include "util/dir.h"
#include "util/memory.h"
#include "util/yaml.h"
#include "framework/log.h"

/*!
 * @brief Evaluates whether the specified file is an acceptable plugin to load
 * based on the specified info and criteria.
 * @param[in] info The requested plugin to try and match.
 * @param[in] file The file to test.
 * @param[in] criteria The criteria to use.
 * @return Returns 1 if successful, 0 if otherwise.
 */
static char
plugin_version_acceptable(const struct plugin_info_t* info,
						  const char* file,
						  const plugin_search_criteria_e criteria);

/*!
 * @brief Scans the plugin directory for a suitable plugin to load.
 * @param[in] game Game object to find plugins fore.
 * @param[in] info The requested plugin to try and match.
 * @param[in] criteria The criteria to use.
 * @return Returns the full file name and relative path if a plugin was
 * matched. Returns NULL on failure.
 */
static char*
find_plugin(struct game_t* game,
			const struct plugin_info_t* info,
			const plugin_search_criteria_e criteria);

/* ------------------------------------------------------------------------- */
char
plugin_manager_init(struct game_t* game)
{
	/* init game's plugin container - this keeps track of all of the loaded
	 * plugins */
	list_init_list(&game->plugins);

	/* init core plugin */
	game->core = plugin_create(game,
			"lightship core",
			"core",
			"TheComet",
			"Provides essential events and services to the game object",
			"https://github.com/TheComet93/lightship"
	);
	if(!game->core)
		return 0;

	return 1;
}

/* ------------------------------------------------------------------------- */
void
plugin_manager_deinit(struct game_t* game)
{
	/* unload all plugins */
	LIST_FOR_EACH_ERASE_R(&game->plugins, struct plugin_t, plugin)
		/* NOTE this erases the plugin object from the linked list */
		plugin_unload(game, plugin);
	LIST_END_EACH

	/* destroy core plugin if it exists */
	if(game->core)
		plugin_destroy(game->core);
}

/* ------------------------------------------------------------------------- */
struct plugin_t*
plugin_load(struct game_t* game,
			const struct plugin_info_t* plugin_info,
			const plugin_search_criteria_e criteria)
{
	/* will contain the file name of the plugin if it is found. Must be free_string()'d */
	char* filename = NULL;
	/* will hold the handle of the loaded module, if successful */
	void* handle = NULL;
	/* functions to extract from the loaded module */
	plugin_init_func init_func;
	plugin_start_func start_func;
	plugin_stop_func stop_func;
	plugin_deinit_func deinit_func;
	/* will reference the allocated plugin object returned by plugin_init() */
	struct plugin_t* plugin = NULL;
	/* buffer for holding the various version strings that will be constructed */
	char version_str[sizeof(int)*27+1];

	/*
	 * if anything fails, the program will break from this for loop and clean
	 * up any allocated memory before returning NULL. If it succeeds, the
	 * plugin is returned.
	 */
	for(;;)
	{
		/* make sure not already loaded */
		if(plugin_get_by_name(game, plugin_info->name))
		{
			llog(LOG_ERROR, game, NULL, "plugin \"%s\" already loaded", plugin_info->name);
			break;
		}

		/* find a suitable file to match the criteria */
		filename = find_plugin(game, plugin_info, criteria);
		if(!filename)
		{
			llog(LOG_ERROR, game, NULL, "Error searching for plugin \"%s\": "
				"Unable to find a file matching the critera", plugin_info->name);
			break;
		}

		/* try to load library */
		handle = module_open(filename);
		if(!handle)
			break;

		/* get plugin initialise function */
		*(void**)(&init_func) = module_sym(handle, "plugin_init");
		if(!init_func)
			break;

		/* get plugin start function */
		*(void**)(&start_func) = module_sym(handle, "plugin_start");
		if(!start_func)
			break;

		/* get plugin exit function */
		*(void**)(&stop_func) = module_sym(handle, "plugin_stop");
		if(!stop_func)
			break;

		/* get plugin deinit function */
		*(void**)(&deinit_func) = module_sym(handle, "plugin_deinit");
		if(!deinit_func)
			break;

		/* initialise the plugin */
		plugin = init_func(game);
		if(!plugin)
		{
			llog(LOG_ERROR, game, NULL, "Error initialising plugin: \"plugin_init\" returned NULL");
			break;
		}

		/* get the version string the plugin claims to be */
		plugin_get_version_string(version_str, &plugin->info);

		/* ensure the plugin claims to be the same version as its filename */
		if(!plugin_version_acceptable(&plugin->info, filename, PLUGIN_VERSION_EXACT))
		{
			llog(LOG_ERROR, game, NULL,
							"Error: plugin claims to be version %s",
							", but the filename is \"%s\"",
							version_str, filename
			);
			break;
		}

		/*
		 * If the program has reached this point, it means the plugin has
		 * successfully been loaded and passed basic verification. Copy all
		 * of the relevant data into the plugin struct, push it into the list
		 * of loaded plugins and return the plugin object.
		 */

		plugin->handle = handle;
		plugin->init = init_func;
		plugin->start = start_func;
		plugin->stop = stop_func;
		plugin->deinit = deinit_func;
		list_push(&game->plugins, plugin);

		/* print info about loaded plugin */
		llog(LOG_INFO, game, NULL,
			"loaded plugin \"%s\", version %s", plugin->info.name, version_str
		);

		free_string(filename);
		return plugin;
	}

	/*
	 * If the program reaches this point, it means something went wrong with
	 * loading the plugin. Clean up...
	 */
	if(filename)
		free_string(filename);
	if(handle)
		module_close(handle);
	if(plugin)
		plugin_destroy(plugin);

	return NULL;
}

/* ------------------------------------------------------------------------- */
char
load_plugins_from_yaml(struct game_t* game, const struct ptree_t* plugins_node)
{
	struct plugin_info_t target;
	struct unordered_vector_t new_plugins;
	char index_key_str[sizeof(int)*8+1];
	uint32_t index_key;
	char success = 1;

	/* holds a list of successfully loaded plugins that haven't been started */
	unordered_vector_init_vector(&new_plugins, sizeof(struct plugin_t*));

	/* load all plugins listed in the plugins node */
	index_key = 0;
	YAML_FOR_EACH(plugins_node, ".", key, value)

		struct plugin_t* plugin;
		struct ptree_t* plugin_node;
		plugin_search_criteria_e criteria;
		const char* version_str;
		const char* policy_str;
		const char* is_optional_str;

		/*
		 * The plugins need to be loaded in the order in which they are listed.
		 * This is achieved by incrementing an index value and getting the
		 * value associated with the index. This works because the plugins
		 * are stored in a yaml list.
		 */
		sprintf(index_key_str, "%d", index_key++);
		plugin_node = yaml_get_node(plugins_node, index_key_str);
		if(!plugin_node)
		{
			llog(LOG_ERROR, game, NULL, "Failed to get value in list for index"
				" %d. Are you sure you're using yaml lists correctly?",
				index_key_str);
			success = 0;
			continue;
		}

		/* extract information from tree */
		if(!(target.name = (char*)yaml_get_value(plugin_node, "name")))
		{
			llog(LOG_ERROR, game, NULL, "Key \"name\" isn't defined for plugin");
			continue;
		}
		if(!(version_str = (char*)yaml_get_value(plugin_node, "version")))
		{
			llog(LOG_ERROR, game, NULL, "Key \"version\" isn't defined for plugin");
			continue;
		}
		if(!(policy_str = (char*)yaml_get_value(plugin_node, "version_policy")))
		{
			llog(LOG_WARNING, game, NULL, "Key \"version_policy\" isn't defined for plugin. Using default \"minimum\"");
			policy_str = "minimum"; /* default */
		}
		if(!(is_optional_str = yaml_get_value(plugin_node, "optional")))
		{
			is_optional_str = "false";
		}

		/*
		 * Time to fill out the plugin info struct with the gathered info
		 */

		/* convert version string */
		if(!plugin_extract_version_from_string(version_str,
											&target.version.major,
											&target.version.minor,
											&target.version.patch))
		{
			llog(LOG_ERROR, game, NULL, "Version string of plugin \"%s\" is "
				"invalid. Should be major.minor.patch", target.name);
			continue;
		}

		/* determine version policy */
		if(strncmp("minimum", policy_str, 7) == 0)
			criteria = PLUGIN_VERSION_MINIMUM;
		else if(strncmp("exact", policy_str, 5) == 0)
			criteria = PLUGIN_VERSION_EXACT;
		else
		{
			llog(LOG_ERROR, game, NULL, "Invalid policy \"%s\". Need either "
				"\"minimum\" or \"exact\"", policy_str);
			continue;
		}

		/* load plugin, and add to the list of loaded plugins */
		plugin = plugin_load(game, &target, criteria);
		if(!plugin)
		{
			/* this plugin was not optional, success cannot be achieved */
			if(!yaml_string_to_bool(is_optional_str))
			{
				success = 0;
				goto load_plugins_from_yaml_break;
			}

			continue;
		}

		unordered_vector_push(&new_plugins, &plugin);

	YAML_END_EACH
	load_plugins_from_yaml_break:

	/* start loaded plugins */
	if(success)
	{
		UNORDERED_VECTOR_FOR_EACH(&new_plugins, struct plugin_t*, pluginp)
			/*
			* If any of the plugins fail to start, abort starting further plugins
			* and return an error.
			*/
			if(!plugin_start(game, *pluginp))
			{
				llog(LOG_ERROR, game, NULL, "Failed to start plugin \"%s\"",
						(*pluginp)->info.name);
				success = 0;
				break;
			}
		UNORDERED_VECTOR_END_EACH
	}

	/* clean up */
	unordered_vector_clear_free(&new_plugins);
	return success;
}

/* ------------------------------------------------------------------------- */
void
plugin_unload(struct game_t* game, struct plugin_t* plugin)
{
	void* module_handle;
	llog(LOG_INFO, game, NULL, "unloading plugin \"%s\"", plugin->info.name);

	/* stop the plugin */
	plugin_stop(plugin);

	/* TODO notify everything that this plugin is about to be unloaded */

	/* unregister all services and events registered by this plugin */
	/* TODO once the new functions can be called, insert them here
	event_destroy_all_plugin_events(plugin);
	event_unregister_all_listeners_of_plugin(plugin); */

	/*
	 * NOTE The plugin object becomes invalid as soon as plugin_deinit() is
	 * called. That is why the module handle must first be extracted before
	 * stopping the plugin, so we can unload the module safely.
	 * NOTE Erasing an element from the linked list does not require the plugin
	 * object to be valid, as it does not dereference it.
	 */
	module_handle = plugin->handle;
	plugin_deinit(plugin);
	module_close(module_handle);
	list_erase_element(&game->plugins, plugin);
}

/* ------------------------------------------------------------------------- */
struct plugin_t*
plugin_get_by_name(struct game_t* game, const char* name)
{
	LIST_FOR_EACH(&game->plugins, struct plugin_t, plugin)
		if(strcmp(name, plugin->info.name) == 0)
			return plugin;
	LIST_END_EACH

	return NULL;
}

/* ------------------------------------------------------------------------- */
static char
plugin_version_acceptable(const struct plugin_info_t* info,
						  const char* file,
						  const plugin_search_criteria_e criteria)
{
	uint32_t major, minor, patch;
	if(!plugin_extract_version_from_string(file, &major, &minor, &patch))
		return 0;

	/* compare version info with desired info */
	switch(criteria)
	{
		case PLUGIN_VERSION_EXACT:
			if(major == info->version.major &&
				minor == info->version.minor &&
				patch == info->version.patch)
				return 1;
			break;

		case PLUGIN_VERSION_MINIMUM:
			if(major > info->version.major)
				return 1;
			if(major == info->version.major && minor > info->version.minor)
				return 1;
			if(major == info->version.major &&
				minor == info->version.minor &&
				patch >= info->version.patch)
				return 1;
		default:
			break;
	}

	return 0;
}

/* ------------------------------------------------------------------------- */
static char*
find_plugin(struct game_t* game,
			const struct plugin_info_t* info,
			const plugin_search_criteria_e criteria)
{
	/* local variables */
	struct list_t* list;
	static const char* crit_info[] = {"minimum version ", "exact version "};
	char* file_found = NULL;

	llog(LOG_INFO, game, NULL, "looking for plugin \"%s\", %s %d.%d.%d",
			info->name,
			crit_info[criteria],
			info->version.major,
			info->version.minor,
			info->version.patch);

	/*
	 * Get list of files in various plugin directories.
	 */
	list = list_create();
#if defined(LIGHTSHIP_PLATFORM_WINDOWS)
	get_directory_listing(list, "plugins\\");
#else
	get_directory_listing(list, "plugins/");
#endif

	/* search for plugin file name matching criteria */
	LIST_FOR_EACH(list, char, name)
		if(!file_found &&
			strstr(name, info->name) &&
			plugin_version_acceptable(info, name, criteria))
		{
			file_found = name;
		}
		else
		{
			/*
			 * get_directory_listing() allocates the strings it pushes into
			 * the linked list, and it is up to us to free them.
			 */
			free_string(name);
		}
	LIST_END_EACH

	/* free list of directories */
	list_destroy(list);

	return file_found;
}

#include "plugin_python/config.h"   /* configurations for this plugin */
#include "plugin_python/glob.h"
#include "plugin_python/services.h" /* plugin services */
#include "plugin_python/events.h"   /* plugin events */
#include "plugin_python/py_interp.h"
#include "framework/plugin.h"	    /* plugin api */

/* ------------------------------------------------------------------------- */
PLUGIN_PYTHON_PUBLIC_API PLUGIN_INIT()
{
	struct plugin_t* plugin;

	/* init global data */
	glob_create(game);

	/* create plugin object - host requires this */
	/* plugin information can be changed in the file "CMakeLists.txt" */
	plugin = plugin_create(game,
			PLUGIN_NAME,
			PLUGIN_CATEGORY,
			PLUGIN_AUTHOR,
			PLUGIN_DESCRIPTION,
			PLUGIN_WEBSITE);

	/* add the plugin to the glob struct for later access */
	get_global(game)->plugin = plugin;

	/* set other info */
	plugin_set_programming_language(plugin,
			PLUGIN_PROGRAMMING_LANGUAGE_C);
	plugin_set_version(plugin,
			PLUGIN_VERSION_MAJOR,
			PLUGIN_VERSION_MINOR,
			PLUGIN_VERSION_PATCH);

	register_services(plugin);
	register_events(plugin);

	return plugin;
}

/* ------------------------------------------------------------------------- */
PLUGIN_PYTHON_PUBLIC_API PLUGIN_START()
{
	struct glob_t* g = get_global(game);

	/* hook in to services and events */
	if(!get_required_services(g->plugin))
		return PLUGIN_FAILURE;
	get_optional_services(g->plugin);
	register_event_listeners(g->plugin);

	if(!init_python(g))
		return PLUGIN_FAILURE;

	return PLUGIN_SUCCESS;
}

/* ------------------------------------------------------------------------- */
PLUGIN_PYTHON_PUBLIC_API PLUGIN_STOP()
{
	struct glob_t* g = get_global(game);

	deinit_python(g);
}

/* ------------------------------------------------------------------------- */
PLUGIN_PYTHON_PUBLIC_API PLUGIN_DEINIT()
{
	plugin_destroy(get_global(game)->plugin);
	glob_destroy(game);
}
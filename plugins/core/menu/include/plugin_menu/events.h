#include "plugin_manager/event_api.h"
#include "util/pstdint.h"

/* -----------------------------------------------------
 * All events this plugin emits
 * ---------------------------------------------------*/
 
EVENT_H1(evt_button_clicked, uint32_t)

struct plugin_t;

void
register_events(struct plugin_t* plugin);

void
register_event_listeners(struct plugin_t* plugin);

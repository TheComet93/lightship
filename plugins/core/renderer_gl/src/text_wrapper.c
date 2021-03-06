#include "plugin_renderer_gl/context.h"
#include "plugin_renderer_gl/text_wrapper.h"
#include "plugin_renderer_gl/text.h"
#include "plugin_renderer_gl/text_manager.h"
#include "framework/game.h"
#include "framework/plugin.h"
#include "framework/services.h"

static struct bstv_t g_text_groups;
static struct bstv_t g_texts;
static uint32_t guid = 1;

/* ------------------------------------------------------------------------- */
char
text_wrapper_init(void)
{
	bstv_init_bstv(&g_texts);

	return 1;
}

/* ------------------------------------------------------------------------- */
void
text_wrapper_deinit(void)
{
	/* text objects are cleaned up automatically when all groups get destroyed */

	bstv_clear_free(&g_texts);
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
const struct bstv_t*
text_group_get_all(void)
{
	return &g_text_groups;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
SERVICE(text_group_create_wrapper)
{
	EXTRACT_ARGUMENT_PTR(0, file_name, const char*);
	EXTRACT_ARGUMENT(1, char_size, uint32_t, uint32_t);
	struct context_t* context = get_context(service->plugin->game);

	RETURN(text_group_create(context, file_name, char_size), uint32_t);

}

/* ------------------------------------------------------------------------- */
SERVICE(text_group_destroy_wrapper)
{
	EXTRACT_ARGUMENT(0, id, uint32_t, uint32_t);

	text_group_destroy(id);
}

/* ------------------------------------------------------------------------- */
SERVICE(text_group_load_character_set_wrapper)
{
	EXTRACT_ARGUMENT(0, id, uint32_t, uint32_t);
	EXTRACT_ARGUMENT_PTR(1, characters, wchar_t*);
	struct context_t* context = get_context(service->plugin->game);

	text_group_load_character_set(context, id, characters);
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

SERVICE(text_create_wrapper)
{
	EXTRACT_ARGUMENT(0, group_id, uint32_t, uint32_t);
	EXTRACT_ARGUMENT(1, centered, char, char);
	EXTRACT_ARGUMENT(2, x, float, GLfloat);
	EXTRACT_ARGUMENT(3, y, float, GLfloat);
	EXTRACT_ARGUMENT_PTR(4, string, wchar_t*);
	struct context_t* context = get_context(service->plugin->game);

	struct text_group_t* group = text_group_get(group_id);
	struct text_t* text = text_create(context, group, centered, x, y, string);
	uint32_t text_id = guid++;
	bstv_insert(&g_texts, text_id, text);

	RETURN(text_id, uint32_t);
}

/* ------------------------------------------------------------------------- */
SERVICE(text_destroy_wrapper)
{
	EXTRACT_ARGUMENT(0, text_id, uint32_t, uint32_t);

	struct text_t* text = bstv_erase(&g_texts, text_id);
	if(text)
		text_destroy(text);
}

/* ------------------------------------------------------------------------- */
SERVICE(text_set_centered_wrapper)
{
	EXTRACT_ARGUMENT(0, text_id, uint32_t, uint32_t);
	EXTRACT_ARGUMENT(1, is_centered, char, char);

	struct text_t* text = bstv_find(&g_texts, text_id);
	if(!text)
		return;

	text_set_centered(text, is_centered);
}

/* ------------------------------------------------------------------------- */
SERVICE(text_set_position_wrapper)
{
}

/* ------------------------------------------------------------------------- */
SERVICE(text_set_string_wrapper)
{
}

/* ------------------------------------------------------------------------- */
SERVICE(text_show_wrapper)
{
	EXTRACT_ARGUMENT(0, text_id, uint32_t, uint32_t);
	struct text_t* text = bstv_find(&g_texts, text_id);
	if(!text)
		return;

	text_show(text);
}

/* ------------------------------------------------------------------------- */
SERVICE(text_hide_wrapper)
{
	EXTRACT_ARGUMENT(0, text_id, uint32_t, uint32_t);
	struct text_t* text = bstv_find(&g_texts, text_id);
	if(!text)
		return;

	text_hide(text);
}

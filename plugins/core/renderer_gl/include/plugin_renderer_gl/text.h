#include "util/ordered_vector.h"
#include "util/service_api.h"
#include <GL/glew.h>
#include <wchar.h>

struct text_group_t;

struct text_t
{
    /* reference to group this text object belongs to */
    struct text_group_t* group;
    /* mesh data of this text instance */
    struct ordered_vector_t vertex_buffer;
    struct ordered_vector_t index_buffer;
    wchar_t* string;
    GLfloat x;
    GLfloat y;
    char is_centered;
    char visible;
};

/*!
 * @brief Adds a new text string to the static vertex buffer.
 * 
 * The static buffer takes longer to re-generate, but is very cheap to render
 * once generated.
 * @param font The font to use for the text string.
 * @param x The x-coordinate in GL screen space of the first letter.
 * @param y The y-coordinate in GL screen space of the top of the first letter.
 * @param str The wchar_t* string to add. If NULL is specified, the static
 * buffer is re-generated but nothing is added.
 * @return Returns the id for the string being added. This can be used to later
 * delete the string from the static buffer.
 */
struct text_t*
text_create(struct text_group_t* text_group, char centered, GLfloat x, GLfloat y, const wchar_t* str);

/*!
 * @brief Destroys a text string from the static vertex buffer.
 * 
 * The static buffer takes longer to re-generate, but is very cheap to render
 * once generated.
 * @param font The font the vertex buffer belongs to.
 * @param id The unique identifier returned by text_add_static_string().
 */
void
text_destroy(struct text_t* text);

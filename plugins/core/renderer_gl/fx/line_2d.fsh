#version 330 core

/* ----------------------------------------------------------------------------
 * vertex input data
 * ------------------------------------------------------------------------- */

in vec3 line_colour;

/* ----------------------------------------------------------------------------
 * output data
 * ------------------------------------------------------------------------- */

out vec3 colour;

/* ----------------------------------------------------------------------------
 * fragment shader main
 * ------------------------------------------------------------------------- */

void main()
{
    colour = line_colour;
}


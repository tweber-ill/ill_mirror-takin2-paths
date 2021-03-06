/**
 * paths rendering widget, fragment shader
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-2021
 * @license GPLv3, see 'LICENSE' file
 *
 * References:
 *   - http://doc.qt.io/qt-5/qopenglwidget.html#details
 *   - http://code.qt.io/cgit/qt/qtbase.git/tree/examples/opengl/threadedqopenglwidget
 */

#version ${GLSL_VERSION}

// colour
in vec4 frag_col;
out vec4 out_col;

// fragment coordinates
in vec2 frag_coords;

// current cursor position
uniform bool cursor_active = true;
uniform vec2 cursor_coords = vec2(0.5, 0.5);


void main()
{
	out_col = frag_col;

	if(cursor_active && length(frag_coords - cursor_coords) < 0.01)
		out_col = vec4(1, 0, 0, 1);
}

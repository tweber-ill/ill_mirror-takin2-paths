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
#define t_real float


// ----------------------------------------------------------------------------
// inputs to fragment shader (output from vertex shader)
// ----------------------------------------------------------------------------
in VertexOut
{
	vec4 col;
	vec2 coords;
} frag_in;
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// outputs from fragment shader
// ----------------------------------------------------------------------------
out vec4 frag_out_col;
// ----------------------------------------------------------------------------


// current cursor position
uniform bool cursor_active = true;
uniform vec2 cursor_coords = vec2(0.5, 0.5);


void main()
{
	frag_out_col = frag_in.col;

	if(cursor_active && length(frag_in.coords - cursor_coords) < 0.01)
		frag_out_col = vec4(1, 0, 0, 1);
}

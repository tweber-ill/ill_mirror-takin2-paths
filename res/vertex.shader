/**
 * paths rendering widget, vertex shader
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

const t_real pi = ${PI};



// ----------------------------------------------------------------------------
// inputs to vertex shader
// ----------------------------------------------------------------------------
in vec4 vertex;
in vec4 normal;
in vec4 vertex_col;
in vec2 tex_coords;
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// outputs from vertex shader
// ----------------------------------------------------------------------------
out VertexOut
{
	vec4 col;
	vec2 coords;

	vec4 pos;
	vec4 norm;
} vertex_out;
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// transformations
// ----------------------------------------------------------------------------
uniform mat4 trafos_proj = mat4(1.);
uniform mat4 trafos_cam = mat4(1.);
uniform mat4 trafos_cam_inv = mat4(1.);
uniform mat4 trafos_obj = mat4(1.);
// ----------------------------------------------------------------------------


void main()
{
	vec4 objPos = trafos_obj * vertex;
	vec4 objNorm = normalize(trafos_obj * normal);
	gl_Position = trafos_proj * trafos_cam * objPos;

	vertex_out.pos = objPos;
	vertex_out.norm = objNorm;
	vertex_out.col = vertex_col;
	//vertex_out.col.a = 1;

	vertex_out.coords = tex_coords;
}

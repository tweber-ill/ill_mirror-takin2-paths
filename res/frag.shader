/**
 * paths rendering widget, fragment shader
 * @author Tobias Weber <tweber@ill.fr>
 * @date 2021
 * @license GPLv3, see 'LICENSE' file
 *
 * References:
 *   - http://doc.qt.io/qt-5/qopenglwidget.html#details
 *   - http://code.qt.io/cgit/qt/qtbase.git/tree/examples/opengl/threadedqopenglwidget
 */

#version ${GLSL_VERSION}


in vec4 fragcol;
out vec4 outcol;


void main()
{
	outcol = fragcol;
}

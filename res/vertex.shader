/**
 * paths rendering widget, vertex shader
 * @author Tobias Weber <tweber@ill.fr>
 * @date 2021
 * @license GPLv3, see 'LICENSE' file
 *
 * References:
 *   - http://doc.qt.io/qt-5/qopenglwidget.html#details
 *   - http://code.qt.io/cgit/qt/qtbase.git/tree/examples/opengl/threadedqopenglwidget
 */

#version ${GLSL_VERSION}


const float pi = ${PI};

in vec4 vertex;
in vec4 normal;
in vec4 vertex_col;
in vec2 tex_coords;

out vec4 frag_col;
out vec2 frag_coords;


// ----------------------------------------------------------------------------
// transformations
// ----------------------------------------------------------------------------
uniform mat4 proj = mat4(1.);
uniform mat4 cam = mat4(1.);
uniform mat4 cam_inv = mat4(1.);
uniform mat4 obj = mat4(1.);
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// lighting
// ----------------------------------------------------------------------------
uniform vec4 const_col = vec4(1, 1, 1, 1);
uniform vec3 lightpos[] = vec3[]( vec3(5, 5, 5), vec3(0, 0, 0), vec3(0, 0, 0), vec3(0, 0, 0) );
uniform int activelights = 1;	// how many lights to use?

float g_diffuse = 1.;
float g_specular = 0.25;
float g_shininess = 1.;
float g_ambient = 0.2;
// ----------------------------------------------------------------------------


/**
 * reflect a vector on a surface with normal n => subtract the projection vector twice: 1 - 2*|n><n|
 */
mat3 reflect(vec3 n)
{
	mat3 refl = mat3(1.) - 2.*outerProduct(n, n);

	// have both vectors point away from the surface
	return -refl;
}


/**
 * position of the camera
 */
vec3 get_campos()
{
	vec4 trans = -vec4(cam[3].xyz, 0);
	return (cam_inv*trans).xyz;
}


/**
 * phong lighting model
 * @see https://en.wikipedia.org/wiki/Phong_reflection_model
 */
float lighting(vec4 objVert, vec4 objNorm)
{
	float I_diff = 0.;
	float I_spec = 0.;


	vec3 dirToCam;
	// only used for specular lighting
	if(g_specular > 0.) dirToCam = normalize(get_campos() - objVert.xyz);


	// iterate (active) light sources
	for(int lightidx=0; lightidx<min(lightpos.length(), activelights); ++lightidx)
	{
		// diffuse lighting
		vec3 dirLight = normalize(lightpos[lightidx]-objVert.xyz);

		if(g_diffuse > 0.)
		{
			float I_diff_inc = g_diffuse * dot(objNorm.xyz, dirLight);
			if(I_diff_inc < 0.) I_diff_inc = 0.;
			I_diff += I_diff_inc;
		}


		// specular lighting
		if(g_specular > 0.)
		{
			if(dot(dirToCam, objNorm.xyz) > 0.)
			{
				vec3 dirLightRefl = reflect(objNorm.xyz) * dirLight;

				float val = dot(dirToCam, dirLightRefl);
				if(val > 0.)
				{
					float I_spec_inc = g_specular * pow(val, g_shininess);
					if(I_spec_inc < 0.) I_spec_inc = 0.;
					I_spec += I_spec_inc;
				}
			}
		}
	}


	// ambient lighting
	float I_amb = g_ambient;


	// total intensity
	return I_diff + I_spec + I_amb;
}


void main()
{
	vec4 objPos = obj * vertex;
	vec4 objNorm = normalize(obj * normal);
	gl_Position = proj * cam * objPos;

	float I = lighting(objPos, objNorm);
	frag_col.rgb = vertex_col.rgb * I;
	//frag_col.a = 1;
	frag_col *= const_col;

	frag_coords = tex_coords;
}

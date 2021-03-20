/**
 * paths rendering widget
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-2021
 * @license GPLv3, see 'LICENSE' file
 *
 * References:
 *   - http://doc.qt.io/qt-5/qopenglwidget.html#details
 *   - http://code.qt.io/cgit/qt/qtbase.git/tree/examples/opengl/threadedqopenglwidget
 *   - http://doc.qt.io/qt-5/qopengltexture.html
 */

#include "PathsRenderer.h"

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QSurfaceFormat>
#include <QtGui/QPainter>
#include <QtGui/QGuiApplication>
#include <QtCore/QtGlobal>

#include <iostream>
#include <boost/scope_exit.hpp>

#include "tlibs2/libs/file.h"


#define OBJNAME_COORD_CROSS "coord_cross"
#define OBJNAME_FLOOR_PLANE "floor"
#define MAX_LIGHTS 4	// max. number allowed in shader
#define TIMER_FPS 60



PathsRenderer::PathsRenderer(QWidget *pParent) : QOpenGLWidget(pParent)
{
	connect(&m_timer, &QTimer::timeout,
		this, static_cast<void (PathsRenderer::*)()>(&PathsRenderer::tick));
	m_timer.start(std::chrono::milliseconds(1000 / TIMER_FPS));

	UpdateCam();
	setMouseTracking(true);
	setFocusPolicy(Qt::StrongFocus);
}


PathsRenderer::~PathsRenderer()
{
	setMouseTracking(false);
	m_timer.stop();

	Clear();

	// delete gl objects within current gl context
	m_pShaders.reset();
}


/**
 * renderer versions and driver descriptions
 */
std::tuple<std::string, std::string, std::string, std::string> PathsRenderer::GetGlDescr() const
{
	return std::make_tuple(m_strGlVer, m_strGlShaderVer, m_strGlVendor, m_strGlRenderer);
}


/**
 * clear instrument scene
 */
void PathsRenderer::Clear()
{
	makeCurrent();
	BOOST_SCOPE_EXIT(this_) { this_->doneCurrent(); } BOOST_SCOPE_EXIT_END

	QMutexLocker _locker{&m_mutexObj};
	for(auto &[obj_name, obj] : m_objs)
		DeleteObject(obj);

	m_objs.clear();
}


/**
 * create a 3d representation of the instrument and walls
 */
void PathsRenderer::LoadInstrument(const InstrumentSpace& instrspace)
{
	Clear();

	// upper and lower floor plane
	// the lower floor plane just serves to hide clipping artefacts
	std::string lowerFloor = "lower " OBJNAME_FLOOR_PLANE;
	AddFloorPlane(OBJNAME_FLOOR_PLANE, instrspace.GetFloorLenX(), instrspace.GetFloorLenY());
	AddFloorPlane(lowerFloor, instrspace.GetFloorLenX(), instrspace.GetFloorLenY());
	m_objs[lowerFloor].m_mat(2,3) = -0.01;

	// instrument
	const auto& instr = instrspace.GetInstrument();
	const auto& mono = instr.GetMonochromator();
	const auto& sample = instr.GetSample();
	const auto& ana = instr.GetAnalyser();

	for(const auto& axis : {mono, sample, ana})
	{
		t_mat_gl matAxis = tl2::convert<t_mat_gl>(axis.GetTrafo());

		for(const auto& comp : axis.GetComps())
		{
			auto [_verts, _norms, _uvs, _matGeo] = comp->GetTriangles();
			t_mat_gl matGeo = tl2::convert<t_mat_gl>(_matGeo);
			t_mat_gl mat = matAxis * matGeo;

			auto verts = tl2::convert<t_vec3_gl>(_verts);
			auto norms = tl2::convert<t_vec3_gl>(_norms);
			auto uvs = tl2::convert<t_vec3_gl>(_uvs);
			auto cols = tl2::convert<t_vec3_gl>(comp->GetColour());

			AddTriangleObject(comp->GetId(), verts, norms, uvs,
				cols[0], cols[1], cols[2], 1);
			m_objs[comp->GetId()].m_mat = mat;
		}
	}

	// walls
	for(const auto& wall : instrspace.GetWalls())
	{
		auto [_verts, _norms, _uvs, _mat] = wall->GetTriangles();
		t_mat_gl mat = tl2::convert<t_mat_gl>(_mat);

		auto verts = tl2::convert<t_vec3_gl>(_verts);
		auto norms = tl2::convert<t_vec3_gl>(_norms);
		auto uvs = tl2::convert<t_vec3_gl>(_uvs);
		auto cols = tl2::convert<t_vec3_gl>(wall->GetColour());

		AddTriangleObject(wall->GetId(), verts, norms, uvs,
			cols[0], cols[1], cols[2], 1);
		m_objs[wall->GetId()].m_mat = mat;
	}
}


QPointF PathsRenderer::GlToScreenCoords(const t_vec_gl& vec4, bool *pVisible) const
{
	auto [ vecPersp, vec ] =
		tl2::hom_to_screen_coords<t_mat_gl, t_vec_gl>
			(vec4, m_matCam, m_matPerspective, m_matViewport, true);

	// position not visible -> return a point outside the viewport
	if(vecPersp[2] > 1.)
	{
		if(pVisible) *pVisible = false;
		return QPointF(-1*m_screenDims[0], -1*m_screenDims[1]);
	}

	if(pVisible) *pVisible = true;
	return QPointF(vec[0], vec[1]);
}


/**
 * delete an object
 */
void PathsRenderer::DeleteObject(PathsObj& obj)
{
	obj.m_pvertexbuf.reset();
	obj.m_pnormalsbuf.reset();
	obj.m_pcolourbuf.reset();
	obj.m_puvbuf.reset();

	if(qgl_funcs* pGl = get_gl_functions(this); pGl)
	{
		pGl->glDeleteVertexArrays(1, &obj.m_vertexarr);
		LOGGLERR(pGl)
	}
}


/**
 * delete an object by name
 */
void PathsRenderer::DeleteObject(const std::string& obj_name)
{
	QMutexLocker _locker{&m_mutexObj};
	auto iter = m_objs.find(obj_name);

	if(iter != m_objs.end())
	{
		DeleteObject(iter->second);
		m_objs.erase(iter);
	}
}


/**
 * add a polygon-based object
 */
void PathsRenderer::AddTriangleObject(const std::string& obj_name,
	const std::vector<t_vec3_gl>& triag_verts,
	const std::vector<t_vec3_gl>& triag_norms, const std::vector<t_vec3_gl>& triag_uvs,
	t_real_gl r, t_real_gl g, t_real_gl b, t_real_gl a)
{
	auto [boundingSpherePos, boundingSphereRad] = tl2::bounding_sphere<t_vec3_gl>(triag_verts);
	auto col = tl2::create<t_vec_gl>({r,g,b,a});

	QMutexLocker _locker{&m_mutexObj};

	PathsObj obj;
	create_triangle_object(this, obj,
		triag_verts, triag_verts, triag_norms, triag_uvs, col,
		false, m_attrVertex, m_attrVertexNorm, m_attrVertexCol, m_attrTexCoords);

	obj.m_mat = tl2::hom_translation<t_mat_gl, t_real_gl>(0., 0., 0.);
	obj.m_boundingSpherePos = std::move(boundingSpherePos);
	obj.m_boundingSphereRad = boundingSphereRad;

	m_objs.insert(std::make_pair(obj_name, std::move(obj)));
}


/**
 * add the floor plane
 */
void PathsRenderer::AddFloorPlane(const std::string& obj_name, t_real_gl len_x, t_real_gl len_y)
{
	auto norm = tl2::create<t_vec3_gl>({0, 0, 1});
	auto plane = tl2::create_plane<t_mat_gl, t_vec3_gl>(norm, 0.5*len_x, 0.5*len_y);
	auto [verts, norms, uvs] = tl2::create_triangles<t_vec3_gl>(plane);

	AddTriangleObject(obj_name, verts, norms, uvs, 0.5,0.5,0.5,1);
	m_objs[obj_name].m_cull = false;
}


void PathsRenderer::UpdateCam()
{
	t_mat_gl matCamTrans = m_matCamTrans;
	matCamTrans(2,3) = 0.;
	t_mat_gl matCamTrans_inv = matCamTrans;
	matCamTrans_inv(0,3) = -matCamTrans(0,3);
	matCamTrans_inv(1,3) = -matCamTrans(1,3);
	matCamTrans_inv(2,3) = -matCamTrans(2,3);

	const t_vec_gl vecCamDir[2] =
	{
		tl2::create<t_vec_gl>({1., 0., 0., 0.}),
		tl2::create<t_vec_gl>({0. ,0., 1., 0.})
	};

	m_matCamRot = tl2::hom_rotation<t_mat_gl, t_vec_gl>(vecCamDir[0], m_theta/180.*tl2::pi<t_real_gl>, 0);
	m_matCamRot *= tl2::hom_rotation<t_mat_gl, t_vec_gl>(vecCamDir[1], m_phi/180.*tl2::pi<t_real_gl>, 0);

	m_matCam = tl2::unit<t_mat_gl>();
	m_matCam *= m_matCamTrans;
	m_matCam(2,3) /= m_zoom;
	m_matCam *= matCamTrans_inv * m_matCamRot * matCamTrans;
	std::tie(m_matCam_inv, std::ignore) = tl2::inv<t_mat_gl>(m_matCam);

	m_pickerNeedsUpdate = true;
	update();
}


/**
 * @brief centre camera around a given object
 */
void PathsRenderer::CentreCam(const std::string& objid)
{
	if(auto iter = m_objs.find(objid); iter!=m_objs.end())
	{
		PathsObj& obj = iter->second;
		m_matCamTrans(0,3) = -obj.m_mat(0,3);
		m_matCamTrans(1,3) = -obj.m_mat(1,3);
		//m_matCamTrans(2,3) = -bj.m_mat(2,3);

		UpdateCam();
	}
}


void PathsRenderer::SetLight(std::size_t idx, const t_vec3_gl& pos)
{
	if(m_lights.size() < idx+1)
		m_lights.resize(idx+1);

	m_lights[idx] = pos;
	m_lightsNeedUpdate = true;
}


void PathsRenderer::UpdateLights()
{
	auto *pGl = GetGlFunctions();
	if(!pGl)
		return;

	int num_lights = std::min(MAX_LIGHTS, static_cast<int>(m_lights.size()));
	t_real_gl pos[num_lights * 3];

	for(int i=0; i<num_lights; ++i)
	{
		pos[i*3 + 0] = m_lights[i][0];
		pos[i*3 + 1] = m_lights[i][1];
		pos[i*3 + 2] = m_lights[i][2];
	}

	// bind shaders
	m_pShaders->bind();
	BOOST_SCOPE_EXIT(m_pShaders) { m_pShaders->release(); } BOOST_SCOPE_EXIT_END
	LOGGLERR(pGl);

	m_pShaders->setUniformValueArray(m_uniLightPos, pos, num_lights, 3);
	m_pShaders->setUniformValue(m_uniNumActiveLights, num_lights);

	m_lightsNeedUpdate = false;
}


void PathsRenderer::EnablePicker(bool b)
{
	m_pickerEnabled = b;
}


void PathsRenderer::UpdatePicker()
{
	if(!m_initialised || !m_pickerEnabled)
		return;

	// picker ray
	auto [org, dir] = tl2::hom_line_from_screen_coords<t_mat_gl, t_vec_gl>(
		m_posMouse.x(), m_posMouse.y(), 0., 1., m_matCam_inv,
		m_matPerspective_inv, m_matViewport_inv, &m_matViewport, true);
	t_vec3_gl org3 = tl2::create<t_vec3_gl>({org[0], org[1], org[2]});
	t_vec3_gl dir3 = tl2::create<t_vec3_gl>({dir[0], dir[1], dir[2]});


	// intersection with unit sphere around origin
	bool hasSphereInters = false;
	t_vec_gl vecClosestSphereInters = tl2::create<t_vec_gl>({0,0,0,0});

	auto intersUnitSphere =
		tl2::intersect_line_sphere<t_vec3_gl, std::vector>(org3, dir3,
			tl2::create<t_vec3_gl>({0,0,0}), t_real_gl(m_pickerSphereRadius));
	for(const auto& result : intersUnitSphere)
	{
		t_vec_gl vecInters4 = tl2::create<t_vec_gl>({result[0], result[1], result[2], 1});

		if(!hasSphereInters)
		{	// first intersection
			vecClosestSphereInters = vecInters4;
			hasSphereInters = true;
		}
		else
		{	// test if next intersection is closer...
			t_vec_gl oldPosTrafo = m_matCam * vecClosestSphereInters;
			t_vec_gl newPosTrafo = m_matCam * vecInters4;

			// ... it is closer.
			if(tl2::norm(newPosTrafo) < tl2::norm(oldPosTrafo))
				vecClosestSphereInters = vecInters4;
		}
	}


	// intersection with geometry
	bool hasInters = false;
	m_curObj = "";
	m_curActive = false;
	t_vec_gl vecClosestInters = tl2::create<t_vec_gl>({0,0,0,0});

	QMutexLocker _locker{&m_mutexObj};

	for(const auto& [obj_name, obj] : m_objs)
	{
		if(obj.m_type != GlRenderObjType::TRIANGLES || !obj.m_visible)
			continue;


		const t_mat_gl& matTrafo = obj.m_mat;

		// scaling factor, TODO: maximum factor for non-uniform scaling
		auto scale = std::cbrt(std::abs(tl2::det(matTrafo)));

		// intersection with bounding sphere?
		auto boundingInters =
			tl2::intersect_line_sphere<t_vec3_gl, std::vector>(org3, dir3,
				matTrafo * obj.m_boundingSpherePos, scale*obj.m_boundingSphereRad);
		if(boundingInters.size() == 0)
			continue;


		// test actual polygons for intersection
		for(std::size_t startidx=0; startidx+2<obj.m_triangles.size(); startidx+=3)
		{
			std::vector<t_vec3_gl> poly{ {
				obj.m_triangles[startidx+0],
				obj.m_triangles[startidx+1],
				obj.m_triangles[startidx+2]
			} };

			std::vector<t_vec3_gl> polyuv{ {
				obj.m_uvs[startidx+0],
				obj.m_uvs[startidx+1],
				obj.m_uvs[startidx+2]
			} };

			auto [vecInters, bInters, lamInters] =
				tl2::intersect_line_poly<t_vec3_gl, t_mat_gl>(org3, dir3, poly, matTrafo);

			if(bInters)
			{
				t_vec_gl vecInters4 = tl2::create<t_vec_gl>({vecInters[0], vecInters[1], vecInters[2], 1});

				// intersection with floor plane
				if(obj_name == OBJNAME_FLOOR_PLANE)
				{
					auto uv = tl2::poly_uv<t_mat_gl, t_vec3_gl>
					(poly[0], poly[1], poly[2], polyuv[0], polyuv[1], polyuv[2], vecInters);

					// save intersections with base plane for drawing walls
					m_cursor[0] = uv[0];
					m_cursor[1] = uv[1];
					m_curActive = true;

					emit FloorPlaneCoordsChanged(vecInters4[0], vecInters4[1]);

					SetLight(0, tl2::create<t_vec3_gl>({vecInters4[0], vecInters4[1], 10}));
				}

				// intersection with other objects
				bool updateUV = false;

				if(!hasInters)
				{	// first intersection
					vecClosestInters = vecInters4;
					m_curObj = obj_name;
					hasInters = true;
					updateUV = true;
				}
				else
				{	// test if next intersection is closer...
					t_vec_gl oldPosTrafo = m_matCam * vecClosestInters;
					t_vec_gl newPosTrafo = m_matCam * vecInters4;

					if(tl2::norm(newPosTrafo) < tl2::norm(oldPosTrafo))
					{	// ...it is closer
						vecClosestInters = vecInters4;
						m_curObj = obj_name;

						updateUV = true;
					}
				}

				if(updateUV)
				{
				}
			}
		}
	}

	m_pickerNeedsUpdate = false;
	t_vec3_gl vecClosestInters3 = tl2::create<t_vec3_gl>({vecClosestInters[0], vecClosestInters[1], vecClosestInters[2]});
	t_vec3_gl vecClosestSphereInters3 = tl2::create<t_vec3_gl>({vecClosestSphereInters[0], vecClosestSphereInters[1], vecClosestSphereInters[2]});

	emit PickerIntersection(hasInters ? &vecClosestInters3 : nullptr,
		m_curObj, hasSphereInters ? &vecClosestSphereInters3 : nullptr);
}


void PathsRenderer::tick()
{
	tick(std::chrono::milliseconds(1000 / 60));
}


void PathsRenderer::tick(const std::chrono::milliseconds& ms)
{
	t_real_gl move_scale = t_real_gl(ms.count()) / t_real_gl(75);

	t_vec_gl xdir = tl2::row<t_mat_gl, t_vec_gl>(m_matCamRot, 0);
	t_vec_gl ydir = tl2::row<t_mat_gl, t_vec_gl>(m_matCamRot, 1);
	t_vec_gl zdir = tl2::row<t_mat_gl, t_vec_gl>(m_matCamRot, 2);

	t_vec_gl xinc = xdir * move_scale *
		(t_real_gl(m_arrowDown[0]) - t_real_gl(m_arrowDown[1]));
	t_vec_gl yinc = ydir * move_scale *
		(t_real_gl(m_pageDown[0]) - t_real_gl(m_pageDown[1]));
	t_vec_gl zinc = zdir * move_scale *
		(t_real_gl(m_arrowDown[2]) - t_real_gl(m_arrowDown[3]));

	m_matCamTrans(0,3) += xinc[0] + yinc[0] + zinc[0];
	m_matCamTrans(1,3) += xinc[1] + yinc[1] + zinc[1];
	m_matCamTrans(2,3) += xinc[2] + yinc[2] + zinc[2];

	UpdateCam();
}


void PathsRenderer::initializeGL()
{
	m_initialised = false;

	// --------------------------------------------------------------------
	// shaders
	// --------------------------------------------------------------------
	auto [frag_ok, strFragShader] = tl2::load_file<std::string>("res/frag.shader");
	auto [vertex_ok, strVertexShader] = tl2::load_file<std::string>("res/vertex.shader");

	if(!frag_ok || !vertex_ok)
	{
		std::cerr << "Fragment or vertex shader could not be loaded." << std::endl;
		return;
	}
	// --------------------------------------------------------------------


	// set glsl version and constants
	const std::string strGlsl = std::to_string(_GLSL_MAJ_VER*100 + _GLSL_MIN_VER*10);
	std::string strPi = std::to_string(tl2::pi<t_real_gl>);			// locale-dependent !
	algo::replace_all(strPi, std::string(","), std::string("."));	// ensure decimal point

	for(std::string* strSrc : { &strFragShader, &strVertexShader })
	{
		algo::replace_all(*strSrc, std::string("${GLSL_VERSION}"), strGlsl);
		algo::replace_all(*strSrc, std::string("${PI}"), strPi);
	}


	// get gl functions
	auto *pGl = get_gl_functions(this);
	if(!pGl) return;

	m_strGlVer = (char*)pGl->glGetString(GL_VERSION);
	m_strGlShaderVer = (char*)pGl->glGetString(GL_SHADING_LANGUAGE_VERSION);
	m_strGlVendor = (char*)pGl->glGetString(GL_VENDOR);
	m_strGlRenderer = (char*)pGl->glGetString(GL_RENDERER);
	LOGGLERR(pGl);


	static QMutex shadermutex;
	shadermutex.lock();
	BOOST_SCOPE_EXIT(&shadermutex) { shadermutex.unlock(); } BOOST_SCOPE_EXIT_END

	// shader compiler/linker error handler
	auto shader_err = [this](const char* err) -> void
	{
		std::cerr << err << std::endl;

		std::string strLog = m_pShaders->log().toStdString();
		if(strLog.size())
			std::cerr << "Shader log: " << strLog << std::endl;
	};

	// compile & link shaders
	m_pShaders = std::make_shared<QOpenGLShaderProgram>(this);

	if(!m_pShaders->addShaderFromSourceCode(QOpenGLShader::Fragment, strFragShader.c_str()))
	{
		shader_err("Cannot compile fragment shader.");
		return;
	}

	if(!m_pShaders->addShaderFromSourceCode(QOpenGLShader::Vertex, strVertexShader.c_str()))
	{
		shader_err("Cannot compile vertex shader.");
		return;
	}

	if(!m_pShaders->link())
	{
		shader_err("Cannot link shaders.");
		return;
	}


	// get attribute handles from shaders
	m_attrVertex = m_pShaders->attributeLocation("vertex");
	m_attrVertexNorm = m_pShaders->attributeLocation("normal");
	m_attrVertexCol = m_pShaders->attributeLocation("vertex_col");
	m_attrTexCoords = m_pShaders->attributeLocation("tex_coords");

	// get uniform handles from shaders
	m_uniMatrixCam = m_pShaders->uniformLocation("cam");
	m_uniMatrixCamInv = m_pShaders->uniformLocation("cam_inv");
	m_uniMatrixProj = m_pShaders->uniformLocation("proj");
	m_uniMatrixObj = m_pShaders->uniformLocation("obj");

	m_uniConstCol = m_pShaders->uniformLocation("const_col");
	m_uniLightPos = m_pShaders->uniformLocation("lightpos");
	m_uniNumActiveLights = m_pShaders->uniformLocation("activelights");

	m_uniCursorActive = m_pShaders->uniformLocation("cursor_active");
	m_uniCursorCoords = m_pShaders->uniformLocation("cursor_coords");
	LOGGLERR(pGl);

	m_initialised = true;
	emit AfterGLInitialisation();
}


void PathsRenderer::resizeGL(int w, int h)
{
	m_screenDims[0] = w;
	m_screenDims[1] = h;

	m_perspectiveNeedsUpdate = true;
	m_viewportNeedsUpdate = true;
}


qgl_funcs* PathsRenderer::GetGlFunctions()
{
	if(!m_initialised)
		return nullptr;
	if(auto *pContext = ((QOpenGLWidget*)this)->context(); !pContext)
		return nullptr;
	return get_gl_functions(this);
}


void PathsRenderer::UpdatePerspective()
{
	auto *pGl = GetGlFunctions();
	if(!pGl)
		return;

	// projection
	const t_real_gl nearPlane = 0.1;
	const t_real_gl farPlane = 1000.;

	if(m_perspectiveProjection)
		m_matPerspective = tl2::hom_perspective<t_mat_gl, t_real_gl>(
			nearPlane, farPlane, tl2::pi<t_real_gl>*0.5, t_real_gl(m_screenDims[1])/t_real_gl(m_screenDims[0]));
	else
		m_matPerspective = tl2::hom_ortho<t_mat_gl, t_real_gl>(nearPlane, farPlane, -10., 10., -10., 10.);

	std::tie(m_matPerspective_inv, std::ignore) = tl2::inv<t_mat_gl>(m_matPerspective);

	// bind shaders
	m_pShaders->bind();
	BOOST_SCOPE_EXIT(m_pShaders) { m_pShaders->release(); } BOOST_SCOPE_EXIT_END
	LOGGLERR(pGl);

	// set matrices
	m_pShaders->setUniformValue(m_uniMatrixProj, m_matPerspective);
	LOGGLERR(pGl);

	m_perspectiveNeedsUpdate = false;
}


void PathsRenderer::UpdateViewport()
{
	auto *pGl = GetGlFunctions();
	if(!pGl)
		return;

	// viewport
	m_matViewport = tl2::hom_viewport<t_mat_gl, t_real_gl>(m_screenDims[0], m_screenDims[1], 0., 1.);
	std::tie(m_matViewport_inv, std::ignore) = tl2::inv<t_mat_gl>(m_matViewport);

	pGl->glViewport(0, 0, m_screenDims[0], m_screenDims[1]);
	pGl->glDepthRange(0, 1);
	LOGGLERR(pGl);

	m_viewportNeedsUpdate = false;
}


void PathsRenderer::paintGL()
{
	if(!m_initialised)
		return;

	QMutexLocker _locker{&m_mutexObj};

	if(auto *pContext = context(); !pContext) return;
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	// gl painting
	{
		if(m_pickerNeedsUpdate)
			UpdatePicker();

		BOOST_SCOPE_EXIT(&painter) { painter.endNativePainting(); } BOOST_SCOPE_EXIT_END
		auto *pGl = get_gl_functions(this);
		painter.beginNativePainting();
		DoPaintGL(pGl);
	}

	// qt painting
	{
		DoPaintQt(painter);
	}
}


/**
 * pure gl drawing
 */
void PathsRenderer::DoPaintGL(qgl_funcs *pGl)
{
	// default options
	pGl->glCullFace(GL_BACK);
	pGl->glFrontFace(GL_CCW);
	pGl->glEnable(GL_CULL_FACE);

	pGl->glDisable(GL_BLEND);
	//pGl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	pGl->glEnable(GL_MULTISAMPLE);
	pGl->glEnable(GL_LINE_SMOOTH);
	pGl->glEnable(GL_POLYGON_SMOOTH);
	pGl->glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	pGl->glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

	// clear
	pGl->glClearColor(1., 1., 1., 1.);
	pGl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	pGl->glEnable(GL_DEPTH_TEST);

	if(m_perspectiveNeedsUpdate)
		UpdatePerspective();
	if(m_viewportNeedsUpdate)
		UpdateViewport();
	if(m_lightsNeedUpdate)
		UpdateLights();

	// bind shaders
	m_pShaders->bind();
	BOOST_SCOPE_EXIT(m_pShaders) { m_pShaders->release(); } BOOST_SCOPE_EXIT_END
	LOGGLERR(pGl);

	// set cam and projection matrices
	m_pShaders->setUniformValue(m_uniMatrixCam, m_matCam);
	m_pShaders->setUniformValue(m_uniMatrixCamInv, m_matCam_inv);

	// cursor
	m_pShaders->setUniformValue(m_uniCursorCoords, m_cursor[0], m_cursor[1]);

	auto colOverride = tl2::create<t_vec_gl>({1,1,1,1});

	// render triangle geometry
	for(const auto& [obj_name, obj] : m_objs)
	{
		if(!obj.m_visible)
			continue;

		// set override color to white
		m_pShaders->setUniformValue(m_uniConstCol, colOverride);

		if(obj.m_cull)
			pGl->glEnable(GL_CULL_FACE);
		else
			pGl->glDisable(GL_CULL_FACE);

		// cursor only active on base plane
		m_pShaders->setUniformValue(m_uniCursorActive, obj_name==OBJNAME_FLOOR_PLANE && m_curActive);

		// set object matrix
		m_pShaders->setUniformValue(m_uniMatrixObj, obj.m_mat);

		// main vertex array object
		pGl->glBindVertexArray(obj.m_vertexarr);


		pGl->glEnableVertexAttribArray(m_attrVertex);
		if(obj.m_type == GlRenderObjType::TRIANGLES)
		{
			pGl->glEnableVertexAttribArray(m_attrVertexNorm);
			pGl->glEnableVertexAttribArray(m_attrTexCoords);
		}
		pGl->glEnableVertexAttribArray(m_attrVertexCol);
		BOOST_SCOPE_EXIT(pGl, &m_attrVertex, &m_attrVertexNorm, &m_attrVertexCol, &m_attrTexCoords)
		{
			pGl->glDisableVertexAttribArray(m_attrTexCoords);
			pGl->glDisableVertexAttribArray(m_attrVertexCol);
			pGl->glDisableVertexAttribArray(m_attrVertexNorm);
			pGl->glDisableVertexAttribArray(m_attrVertex);
		}
		BOOST_SCOPE_EXIT_END
		LOGGLERR(pGl);


		if(obj.m_type == GlRenderObjType::TRIANGLES)
			pGl->glDrawArrays(GL_TRIANGLES, 0, obj.m_triangles.size());
		else if(obj.m_type == GlRenderObjType::LINES)
			pGl->glDrawArrays(GL_LINES, 0, obj.m_vertices.size());
		else
			std::cerr << "Unknown plot object type." << std::endl;

		LOGGLERR(pGl);
	}

	pGl->glDisable(GL_CULL_FACE);
	pGl->glDisable(GL_DEPTH_TEST);
}


/**
 * directly draw on a qpainter
 */
void PathsRenderer::DoPaintQt(QPainter &painter)
{
	QFont fontOrig = painter.font();
	QPen penOrig = painter.pen();
	QBrush brushOrig = painter.brush();


	// draw tooltip
	if(auto curObj = m_objs.find(m_curObj); curObj != m_objs.end())
	{
		const auto& obj = curObj->second;

		if(obj.m_visible)
		{
			QString label = curObj->first.c_str();
			//t_vec3_gl posLabel3d = obj.m_mat * obj.m_labelPos;
			//auto posLabel2d = GlToScreenCoords(tl2::create<t_vec_gl>({posLabel3d[0], posLabel3d[1], posLabel3d[2], 1.}));

			QFont fontLabel = fontOrig;
			QPen penLabel = penOrig;
			QBrush brushLabel = brushOrig;

			fontLabel.setStyleStrategy(QFont::StyleStrategy(QFont::PreferAntialias | QFont::PreferQuality));
			fontLabel.setWeight(QFont::Normal);
			penLabel.setColor(QColor(0, 0, 0, 255));
			brushLabel.setColor(QColor(255, 255, 255, 127));
			brushLabel.setStyle(Qt::SolidPattern);
			painter.setFont(fontLabel);
			painter.setPen(penLabel);
			painter.setBrush(brushLabel);

			QRect boundingRect = painter.fontMetrics().boundingRect(label);
			boundingRect.setWidth(boundingRect.width() * 1.5);
			boundingRect.setHeight(boundingRect.height() * 2);
			boundingRect.translate(m_posMouse.x()+16, m_posMouse.y()+24);

			painter.drawRoundedRect(boundingRect, 8., 8.);
			painter.drawText(boundingRect, Qt::AlignCenter | Qt::AlignVCenter, label);
		}
	}


	// restore original styles
	painter.setFont(fontOrig);
	painter.setPen(penOrig);
	painter.setBrush(brushOrig);
}


void PathsRenderer::keyPressEvent(QKeyEvent *pEvt)
{
	switch(pEvt->key())
	{
		case Qt::Key_Left:
			m_arrowDown[0] = 1;
			pEvt->accept();
			break;
		case Qt::Key_Right:
			m_arrowDown[1] = 1;
			pEvt->accept();
			break;
		case Qt::Key_Up:
			m_arrowDown[2] = 1;
			pEvt->accept();
			break;
		case Qt::Key_Down:
			m_arrowDown[3] = 1;
			pEvt->accept();
			break;
		case Qt::Key_PageUp:
		case Qt::Key_Comma:
			m_pageDown[0] = 1;
			pEvt->accept();
			break;
		case Qt::Key_PageDown:
		case Qt::Key_Period:
			m_pageDown[1] = 1;
			pEvt->accept();
			break;
		default:
			QOpenGLWidget::keyPressEvent(pEvt);
			break;
	}
}


void PathsRenderer::keyReleaseEvent(QKeyEvent *pEvt)
{
	switch(pEvt->key())
	{
		case Qt::Key_Left:
			m_arrowDown[0] = 0;
			pEvt->accept();
			break;
		case Qt::Key_Right:
			m_arrowDown[1] = 0;
			pEvt->accept();
			break;
		case Qt::Key_Up:
			m_arrowDown[2] = 0;
			pEvt->accept();
			break;
		case Qt::Key_Down:
			m_arrowDown[3] = 0;
			pEvt->accept();
			break;
		case Qt::Key_PageUp:
		case Qt::Key_Comma:
			m_pageDown[0] = 0;
			pEvt->accept();
			break;
		case Qt::Key_PageDown:
		case Qt::Key_Period:
			m_pageDown[1] = 0;
			pEvt->accept();
			break;
		default:
			QOpenGLWidget::keyReleaseEvent(pEvt);
			break;
	}
}


void PathsRenderer::mouseMoveEvent(QMouseEvent *pEvt)
{
#if QT_VERSION >= 0x060000
	m_posMouse = pEvt->position();
#else
	m_posMouse = pEvt->localPos();
#endif

	if(m_inRotation)
	{
		auto diff = (m_posMouse - m_posMouseRotationStart);
		m_phi = diff.x() + m_phi_saved;
		m_theta = diff.y() + m_theta_saved;

		UpdateCam();
	}

	UpdatePicker();
	update();

	// an object is being dragged
	if(m_draggedObj != "")
	{
		emit ObjectDragged(m_draggedObj, m_cursor[0], m_cursor[1]);
	}

	m_mouseMovedBetweenDownAndUp = true;
	pEvt->accept();
}


void PathsRenderer::mousePressEvent(QMouseEvent *pEvt)
{
	m_mouseMovedBetweenDownAndUp = false;

	if(pEvt->buttons() & Qt::LeftButton) m_mouseDown[0] = 1;
	if(pEvt->buttons() & Qt::MiddleButton) m_mouseDown[1] = 1;
	if(pEvt->buttons() & Qt::RightButton) m_mouseDown[2] = 1;

	// left mouse button pressed
	if(m_mouseDown[0])
	{
		m_draggedObj = m_curObj;
	}

	// middle mouse button pressed
	if(m_mouseDown[1])
	{
		// reset zoom
		m_zoom = 1;
		UpdateCam();
	}

	// right mouse button pressed
	if(m_mouseDown[2])
	{
		// begin rotation
		if(!m_inRotation)
		{
			m_posMouseRotationStart = m_posMouse;
			m_inRotation = true;
		}
	}

	pEvt->accept();
}


void PathsRenderer::mouseReleaseEvent(QMouseEvent *pEvt)
{
	bool mouseDownOld[] = { m_mouseDown[0], m_mouseDown[1], m_mouseDown[2] };

	if((pEvt->buttons() & Qt::LeftButton) == 0) m_mouseDown[0] = 0;
	if((pEvt->buttons() & Qt::MiddleButton) == 0) m_mouseDown[1] = 0;
	if((pEvt->buttons() & Qt::RightButton) == 0) m_mouseDown[2] = 0;

	// left mouse button released
	if(!m_mouseDown[0])
	{
		m_draggedObj = "";
	}

	// right mouse button released
	if(!m_mouseDown[2])
	{
		// end rotation
		if(m_inRotation)
		{
			auto diff = (m_posMouse - m_posMouseRotationStart);
			m_phi_saved += diff.x();
			m_theta_saved += diff.y();

			m_inRotation = false;
		}
	}

	pEvt->accept();

	// only emit click if moving the mouse (i.e. rotationg the scene) was not the primary intent
	if(!m_mouseMovedBetweenDownAndUp)
	{
		bool mouseClicked[] = {
			!m_mouseDown[0] && mouseDownOld[0],
			!m_mouseDown[1] && mouseDownOld[1],
			!m_mouseDown[2] && mouseDownOld[2] };
		if(mouseClicked[0] || mouseClicked[1] || mouseClicked[2])
			emit ObjectClicked(m_curObj, mouseClicked[0], mouseClicked[1], mouseClicked[2]);
	}
}


void PathsRenderer::wheelEvent(QWheelEvent *pEvt)
{
	const t_real_gl degrees = pEvt->angleDelta().y() / 8.;

	// zoom
	m_zoom *= std::pow(2., degrees/64.);
	UpdateCam();

	pEvt->accept();
}


void PathsRenderer::paintEvent(QPaintEvent* pEvt)
{
	QOpenGLWidget::paintEvent(pEvt);
}

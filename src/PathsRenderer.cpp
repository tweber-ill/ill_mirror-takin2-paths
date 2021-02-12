/**
 * paths rendering widget
 * @author Tobias Weber <tweber@ill.fr>
 * @date 2021
 * @license GPLv3, see 'LICENSE' file
 *
 * References:
 *   - http://doc.qt.io/qt-5/qopenglwidget.html#details
 *   - http://code.qt.io/cgit/qt/qtbase.git/tree/examples/opengl/threadedqopenglwidget
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
#include <boost/preprocessor/stringize.hpp>
#include <boost/algorithm/string/replace.hpp>
namespace algo = boost::algorithm;

#include "tlibs2/libs/file.h"


#pragma message("Compiling for GL version " BOOST_PP_STRINGIZE(_GL_MAJ_VER) "." BOOST_PP_STRINGIZE(_GL_MIN_VER) " and GLSL version " BOOST_PP_STRINGIZE(_GLSL_MAJ_VER) BOOST_PP_STRINGIZE(_GLSL_MIN_VER) "0.")


// ----------------------------------------------------------------------------
void set_gl_format(bool bCore, int iMajorVer, int iMinorVer, int iSamples)
{
	QSurfaceFormat surf = QSurfaceFormat::defaultFormat();

	surf.setRenderableType(QSurfaceFormat::OpenGL);
	if(bCore)
		surf.setProfile(QSurfaceFormat::CoreProfile);
	else
		surf.setProfile(QSurfaceFormat::CompatibilityProfile);

	if(iMajorVer > 0 && iMinorVer > 0)
		surf.setVersion(iMajorVer, iMinorVer);

	surf.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
	if(iSamples > 0)
		surf.setSamples(iSamples);	// multisampling

	QSurfaceFormat::setDefaultFormat(surf);
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// GL plot implementation

PathsRenderer::PathsRenderer(QWidget *pParent) : QOpenGLWidget(pParent)
{
	qRegisterMetaType<std::size_t>("std::size_t");

	if constexpr(m_usetimer)
	{
		connect(&m_timer, &QTimer::timeout,
			this, static_cast<void (PathsRenderer::*)()>(&PathsRenderer::tick));
		m_timer.start(std::chrono::milliseconds(1000 / 60));
	}

	UpdateCam();

	connect(this, &QOpenGLWidget::aboutToCompose, this, &PathsRenderer::beforeComposing);
	connect(this, &QOpenGLWidget::frameSwapped, this, &PathsRenderer::afterComposing);
	connect(this, &QOpenGLWidget::aboutToResize, this, &PathsRenderer::beforeResizing);
	connect(this, &QOpenGLWidget::resized, this, &PathsRenderer::afterResizing);

	//setUpdateBehavior(QOpenGLWidget::PartialUpdate);
	setMouseTracking(true);
}


PathsRenderer::~PathsRenderer()
{
	setMouseTracking(false);

	if constexpr(m_usetimer)
		m_timer.stop();

	makeCurrent();
	BOOST_SCOPE_EXIT(this_) { this_->doneCurrent(); } BOOST_SCOPE_EXIT_END

	// delete gl objects within current gl context
	m_pShaders.reset();

	qgl_funcs* pGl = GetGlFunctions();
	for(auto &obj : m_objs)
	{
		obj.m_pvertexbuf.reset();
		obj.m_pnormalsbuf.reset();
		obj.m_pcolorbuf.reset();
		if(pGl) pGl->glDeleteVertexArrays(1, &obj.m_vertexarr);
	}

	m_objs.clear();
	LOGGLERR(pGl)
}


qgl_funcs* PathsRenderer::GetGlFunctions(QOpenGLWidget *pWidget)
{
	if(!pWidget) pWidget = (QOpenGLWidget*) this;
	qgl_funcs *pGl = nullptr;

	if constexpr(std::is_same_v<qgl_funcs, QOpenGLFunctions>)
		pGl = (qgl_funcs*)pWidget->context()->functions();
	else
		pGl = (qgl_funcs*)pWidget->context()->versionFunctions<qgl_funcs>();

	if(!pGl)
		std::cerr << "No suitable GL interface found." << std::endl;

	return pGl;
}


QPointF PathsRenderer::GlToScreenCoords(const t_vec_gl& vec4, bool *pVisible)
{
	auto [ vecPersp, vec ] =
		tl2::hom_to_screen_coords<t_mat_gl, t_vec_gl>
			(vec4, m_matCam, m_matPerspective, m_matViewport, true);

	// position not visible -> return a point outside the viewport
	if(vecPersp[2] > 1.)
	{
		if(pVisible) *pVisible = false;
		return QPointF(-1*m_iScreenDims[0], -1*m_iScreenDims[1]);
	}

	if(pVisible) *pVisible = true;
	return QPointF(vec[0], vec[1]);
}


t_mat_gl PathsRenderer::GetArrowMatrix(const t_vec_gl& vecTo, t_real_gl postscale, const t_vec_gl& vecPostTrans,
	const t_vec_gl& vecFrom, t_real_gl prescale, const t_vec_gl& vecPreTrans)
{
	t_mat_gl mat = tl2::unit<t_mat_gl>(4);

	mat *= tl2::hom_translation<t_mat_gl>(vecPreTrans[0], vecPreTrans[1], vecPreTrans[2]);
	mat *= tl2::hom_scaling<t_mat_gl>(prescale, prescale, prescale);

	mat *= tl2::rotation<t_mat_gl, t_vec_gl>(vecFrom, vecTo);

	mat *= tl2::hom_scaling<t_mat_gl>(postscale, postscale, postscale);
	mat *= tl2::hom_translation<t_mat_gl>(vecPostTrans[0], vecPostTrans[1], vecPostTrans[2]);

	return mat;
}


PathsRendererObj PathsRenderer::CreateTriangleObject(const std::vector<t_vec3_gl>& verts,
	const std::vector<t_vec3_gl>& triagverts, const std::vector<t_vec3_gl>& norms,
	const t_vec_gl& color, bool bUseVertsAsNorm)
{
	// TODO: move context to calling thread
	makeCurrent();
	BOOST_SCOPE_EXIT(this_) { this_->doneCurrent(); } BOOST_SCOPE_EXIT_END


	qgl_funcs* pGl = GetGlFunctions();

	GLint attrVertex = m_attrVertex;
	GLint attrVertexNormal = m_attrVertexNorm;
	GLint attrVertexColor = m_attrVertexCol;

	PathsRendererObj obj;
	obj.m_type = PathsRendererObjType::TRIANGLES;
	obj.m_color = color;

	// flatten vertex array into raw float array
	auto to_float_array = [](const std::vector<t_vec3_gl>& verts, int iRepeat=1, int iElems=3, bool bNorm=false)
		-> std::vector<t_real_gl>
	{
		std::vector<t_real_gl> vecRet;
		vecRet.reserve(iRepeat*verts.size()*iElems);

		for(const t_vec3_gl& vert : verts)
		{
			t_real_gl norm = bNorm ? tl2::norm<t_vec3_gl>(vert) : 1;

			for(int i=0; i<iRepeat; ++i)
				for(int iElem=0; iElem<iElems; ++iElem)
					vecRet.push_back(vert[iElem] / norm);
		}

		return vecRet;
	};

	// main vertex array object
	pGl->glGenVertexArrays(1, &obj.m_vertexarr);
	pGl->glBindVertexArray(obj.m_vertexarr);

	{	// vertices
		obj.m_pvertexbuf = std::make_shared<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);

		if(!obj.m_pvertexbuf->create())
			std::cerr << "Cannot create vertex buffer." << std::endl;
		if(!obj.m_pvertexbuf->bind())
			std::cerr << "Cannot bind vertex buffer." << std::endl;
		BOOST_SCOPE_EXIT(&obj) { obj.m_pvertexbuf->release(); } BOOST_SCOPE_EXIT_END

		auto vecVerts = to_float_array(triagverts, 1,3, false);
		obj.m_pvertexbuf->allocate(vecVerts.data(), vecVerts.size()*sizeof(typename decltype(vecVerts)::value_type));
		pGl->glVertexAttribPointer(attrVertex, 3, GL_FLOAT, 0, 0, nullptr);
	}

	{	// normals
		obj.m_pnormalsbuf = std::make_shared<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);

		obj.m_pnormalsbuf->create();
		obj.m_pnormalsbuf->bind();
		BOOST_SCOPE_EXIT(&obj) { obj.m_pnormalsbuf->release(); } BOOST_SCOPE_EXIT_END

		auto vecNorms = bUseVertsAsNorm ? to_float_array(triagverts, 1,3, true) : to_float_array(norms, 3,3, false);
		obj.m_pnormalsbuf->allocate(vecNorms.data(), vecNorms.size()*sizeof(typename decltype(vecNorms)::value_type));
		pGl->glVertexAttribPointer(attrVertexNormal, 3, GL_FLOAT, 0, 0, nullptr);
	}

	{	// colors
		obj.m_pcolorbuf = std::make_shared<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);

		obj.m_pcolorbuf->create();
		obj.m_pcolorbuf->bind();
		BOOST_SCOPE_EXIT(&obj) { obj.m_pcolorbuf->release(); } BOOST_SCOPE_EXIT_END

		std::vector<t_real_gl> vecCols;
		vecCols.reserve(4*triagverts.size());
		for(std::size_t iVert=0; iVert<triagverts.size(); ++iVert)
		{
			for(int icol=0; icol<obj.m_color.size(); ++icol)
				vecCols.push_back(obj.m_color[icol]);
		}

		obj.m_pcolorbuf->allocate(vecCols.data(), vecCols.size()*sizeof(typename decltype(vecCols)::value_type));
		pGl->glVertexAttribPointer(attrVertexColor, 4, GL_FLOAT, 0, 0, nullptr);
	}


	obj.m_vertices = std::move(verts);
	obj.m_triangles = std::move(triagverts);
	LOGGLERR(pGl)

	return obj;
}


PathsRendererObj PathsRenderer::CreateLineObject(const std::vector<t_vec3_gl>& verts, const t_vec_gl& color)
{
	// TODO: move context to calling thread
	makeCurrent();
	BOOST_SCOPE_EXIT(this_) { this_->doneCurrent(); } BOOST_SCOPE_EXIT_END


	qgl_funcs* pGl = GetGlFunctions();
	GLint attrVertex = m_attrVertex;
	GLint attrVertexColor = m_attrVertexCol;

	PathsRendererObj obj;
	obj.m_type = PathsRendererObjType::LINES;
	obj.m_color = color;

	// flatten vertex array into raw float array
	auto to_float_array = [](const std::vector<t_vec3_gl>& verts, int iElems=3) -> std::vector<t_real_gl>
	{
		std::vector<t_real_gl> vecRet;
		vecRet.reserve(verts.size()*iElems);

		for(const t_vec3_gl& vert : verts)
		{
			for(int iElem=0; iElem<iElems; ++iElem)
				vecRet.push_back(vert[iElem]);
		}

		return vecRet;
	};

	// main vertex array object
	pGl->glGenVertexArrays(1, &obj.m_vertexarr);
	pGl->glBindVertexArray(obj.m_vertexarr);

	{	// vertices
		obj.m_pvertexbuf = std::make_shared<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);

		obj.m_pvertexbuf->create();
		obj.m_pvertexbuf->bind();
		BOOST_SCOPE_EXIT(&obj) { obj.m_pvertexbuf->release(); } BOOST_SCOPE_EXIT_END

		auto vecVerts = to_float_array(verts, 3);
		obj.m_pvertexbuf->allocate(vecVerts.data(), vecVerts.size()*sizeof(typename decltype(vecVerts)::value_type));
		pGl->glVertexAttribPointer(attrVertex, 3, GL_FLOAT, 0, 0, nullptr);
	}

	{	// colors
		obj.m_pcolorbuf = std::make_shared<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);

		obj.m_pcolorbuf->create();
		obj.m_pcolorbuf->bind();
		BOOST_SCOPE_EXIT(&obj) { obj.m_pcolorbuf->release(); } BOOST_SCOPE_EXIT_END

		std::vector<t_real_gl> vecCols;
		vecCols.reserve(4*verts.size());
		for(std::size_t iVert=0; iVert<verts.size(); ++iVert)
		{
			for(int icol=0; icol<obj.m_color.size(); ++icol)
				vecCols.push_back(obj.m_color[icol]);
		}

		obj.m_pcolorbuf->allocate(vecCols.data(), vecCols.size()*sizeof(typename decltype(vecCols)::value_type));
		pGl->glVertexAttribPointer(attrVertexColor, 4, GL_FLOAT, 0, 0, nullptr);
	}


	obj.m_vertices = std::move(verts);
	LOGGLERR(pGl)

	return obj;
}


void PathsRenderer::SetObjectMatrix(std::size_t idx, const t_mat_gl& mat)
{
	if(idx >= m_objs.size()) return;
	m_objs[idx].m_mat = mat;
}


void PathsRenderer::SetObjectCol(std::size_t idx, t_real_gl r, t_real_gl g, t_real_gl b, t_real_gl a)
{
	if(idx >= m_objs.size()) return;
	m_objs[idx].m_color = tl2::create<t_vec_gl>({r,g,b,a});
}


void PathsRenderer::SetObjectLabel(std::size_t idx, const std::string& label)
{
	if(idx >= m_objs.size()) return;
	m_objs[idx].m_label = label;
}

const std::string& PathsRenderer::GetObjectLabel(std::size_t idx) const
{
	static const std::string empty{};
	if(idx >= m_objs.size()) return empty;

	return m_objs[idx].m_label;
}


void PathsRenderer::SetObjectDataString(std::size_t idx, const std::string& data)
{
	if(idx >= m_objs.size()) return;
	m_objs[idx].m_datastr = data;
}

const std::string& PathsRenderer::GetObjectDataString(std::size_t idx) const
{
	static const std::string empty{};
	if(idx >= m_objs.size()) return empty;

	return m_objs[idx].m_datastr;
}


void PathsRenderer::SetObjectVisible(std::size_t idx, bool visible)
{
	if(idx >= m_objs.size()) return;
	m_objs[idx].m_visible = visible;
}


void PathsRenderer::SetObjectHighlight(std::size_t idx, bool highlight)
{
	if(idx >= m_objs.size()) return;
	m_objs[idx].m_highlighted = highlight;
}


bool PathsRenderer::GetObjectHighlight(std::size_t idx) const
{
	if(idx >= m_objs.size()) return 0;
	return m_objs[idx].m_highlighted;
}


void PathsRenderer::RemoveObject(std::size_t idx)
{
	m_objs[idx].m_valid = false;

	m_objs[idx].m_pvertexbuf.reset();
	m_objs[idx].m_pnormalsbuf.reset();
	m_objs[idx].m_pcolorbuf.reset();

	m_objs[idx].m_vertices.clear();
	m_objs[idx].m_triangles.clear();

	// TODO: remove if object has no follow-up indices
}


std::size_t PathsRenderer::AddLinkedObject(std::size_t linkTo,
	t_real_gl x, t_real_gl y, t_real_gl z,
	t_real_gl r, t_real_gl g, t_real_gl b, t_real_gl a)
{
	PathsRendererObj obj;
	obj.linkedObj = linkTo;
	obj.m_mat = tl2::hom_translation<t_mat_gl>(x, y, z);
	obj.m_color = tl2::create<t_vec_gl>({r, g, b, a});

	QMutexLocker _locker{&m_mutexObj};
	m_objs.emplace_back(std::move(obj));

	return m_objs.size()-1;		// object handle
}


std::size_t PathsRenderer::AddSphere(t_real_gl rad, t_real_gl x, t_real_gl y, t_real_gl z,
	t_real_gl r, t_real_gl g, t_real_gl b, t_real_gl a)
{
	constexpr int numsubdivs = 2;

	auto solid = tl2::create_icosahedron<t_vec3_gl>(1);
	auto [triagverts, norms, uvs] = tl2::spherify<t_vec3_gl>(
		tl2::subdivide_triangles<t_vec3_gl>(
			tl2::create_triangles<t_vec3_gl>(solid), numsubdivs), rad);
	auto [boundingSpherePos, boundingSphereRad] = tl2::bounding_sphere<t_vec3_gl>(triagverts);

	QMutexLocker _locker{&m_mutexObj};

	auto obj = CreateTriangleObject(std::get<0>(solid), triagverts, norms, tl2::create<t_vec_gl>({r,g,b,a}), true);
	obj.m_mat = tl2::hom_translation<t_mat_gl>(x, y, z);
	obj.m_boundingSpherePos = std::move(boundingSpherePos);
	obj.m_boundingSphereRad = boundingSphereRad;
	//obj.m_boundingSphereRad = rad;
	m_objs.emplace_back(std::move(obj));

	return m_objs.size()-1;		// object handle
}


std::size_t PathsRenderer::AddCylinder(t_real_gl rad, t_real_gl h,
	t_real_gl x, t_real_gl y, t_real_gl z,
	t_real_gl r, t_real_gl g, t_real_gl b, t_real_gl a)
{
	auto solid = tl2::create_cylinder<t_vec3_gl>(rad, h, true);
	auto [triagverts, norms, uvs] = tl2::create_triangles<t_vec3_gl>(solid);
	auto [boundingSpherePos, boundingSphereRad] = tl2::bounding_sphere<t_vec3_gl>(triagverts);

	QMutexLocker _locker{&m_mutexObj};

	auto obj = CreateTriangleObject(std::get<0>(solid), triagverts, norms, tl2::create<t_vec_gl>({r,g,b,a}), false);
	obj.m_mat = tl2::hom_translation<t_mat_gl>(x, y, z);
	obj.m_boundingSpherePos = std::move(boundingSpherePos);
	obj.m_boundingSphereRad = boundingSphereRad;
	m_objs.emplace_back(std::move(obj));

	return m_objs.size()-1;		// object handle
}


std::size_t PathsRenderer::AddCone(t_real_gl rad, t_real_gl h,
	t_real_gl x, t_real_gl y, t_real_gl z,
	t_real_gl r, t_real_gl g, t_real_gl b, t_real_gl a)
{
	auto solid = tl2::create_cone<t_vec3_gl>(rad, h);
	auto [triagverts, norms, uvs] = tl2::create_triangles<t_vec3_gl>(solid);
	auto [boundingSpherePos, boundingSphereRad] = tl2::bounding_sphere<t_vec3_gl>(triagverts);

	QMutexLocker _locker{&m_mutexObj};

	auto obj = CreateTriangleObject(std::get<0>(solid), triagverts, norms, tl2::create<t_vec_gl>({r,g,b,a}), false);
	obj.m_mat = tl2::hom_translation<t_mat_gl>(x, y, z);
	obj.m_boundingSpherePos = std::move(boundingSpherePos);
	obj.m_boundingSphereRad = boundingSphereRad;
	m_objs.emplace_back(std::move(obj));

	return m_objs.size()-1;		// object handle
}


std::size_t PathsRenderer::AddArrow(t_real_gl rad, t_real_gl h,
	t_real_gl x, t_real_gl y, t_real_gl z,
	t_real_gl r, t_real_gl g, t_real_gl b, t_real_gl a)
{
	auto solid = tl2::create_cylinder<t_vec3_gl>(rad, h, 2, 32, rad, rad*1.5);
	auto [triagverts, norms, uvs] = tl2::create_triangles<t_vec3_gl>(solid);
	auto [boundingSpherePos, boundingSphereRad] = tl2::bounding_sphere<t_vec3_gl>(triagverts);

	QMutexLocker _locker{&m_mutexObj};

	auto obj = CreateTriangleObject(std::get<0>(solid), triagverts, norms, tl2::create<t_vec_gl>({r,g,b,a}), false);
	obj.m_mat = GetArrowMatrix(tl2::create<t_vec_gl>({1,0,0}), 1., tl2::create<t_vec_gl>({x,y,z}), tl2::create<t_vec_gl>({0,0,1}));
	obj.m_boundingSpherePos = std::move(boundingSpherePos);
	obj.m_boundingSphereRad = boundingSphereRad;
	obj.m_labelPos = tl2::create<t_vec3_gl>({0., 0., 0.75});
	m_objs.emplace_back(std::move(obj));

	return m_objs.size()-1;		// object handle
}


std::size_t PathsRenderer::AddTriangleObject(const std::vector<t_vec3_gl>& triag_verts,
	const std::vector<t_vec3_gl>& triag_norms,
	t_real_gl r, t_real_gl g, t_real_gl b, t_real_gl a)
{
	auto [boundingSpherePos, boundingSphereRad] = tl2::bounding_sphere<t_vec3_gl>(triag_verts);

	QMutexLocker _locker{&m_mutexObj};

	auto obj = CreateTriangleObject(triag_verts, triag_verts, triag_norms, tl2::create<t_vec_gl>({r,g,b,a}), false);
	obj.m_mat = tl2::hom_translation<t_mat_gl>(0., 0., 0.);
	obj.m_boundingSpherePos = std::move(boundingSpherePos);
	obj.m_boundingSphereRad = boundingSphereRad;
	obj.m_labelPos = tl2::create<t_vec3_gl>({0., 0., 0.75});
	m_objs.emplace_back(std::move(obj));

	return m_objs.size()-1;		// object handle
}


std::size_t PathsRenderer::AddCoordinateCross(t_real_gl min, t_real_gl max)
{
	auto col = tl2::create<t_vec_gl>({0,0,0,1});
	auto verts = std::vector<t_vec3_gl>
	{{
		tl2::create<t_vec3_gl>({min,0,0}), tl2::create<t_vec3_gl>({max,0,0}),
		tl2::create<t_vec3_gl>({0,min,0}), tl2::create<t_vec3_gl>({0,max,0}),
		tl2::create<t_vec3_gl>({0,0,min}), tl2::create<t_vec3_gl>({0,0,max}),
	}};

	QMutexLocker _locker{&m_mutexObj};

	auto obj = CreateLineObject(verts, col);
	obj.m_invariant = true;
	m_objs.emplace_back(std::move(obj));

	return m_objs.size()-1;		// object handle
}



void PathsRenderer::UpdateCam()
{
	m_matCam = m_matCamBase;
	m_matCam(2,3) /= m_zoom;
	m_matCam *= m_matCamRot;
	std::tie(m_matCam_inv, std::ignore) = tl2::inv<t_mat_gl>(m_matCam);

	m_bPickerNeedsUpdate = true;
	RequestPlotUpdate();
}


/**
 * request a plot update
 */
void PathsRenderer::RequestPlotUpdate()
{
	QMetaObject::invokeMethod((QOpenGLWidget*)this,
		static_cast<void (QOpenGLWidget::*)()>(&QOpenGLWidget::update),
		Qt::ConnectionType::QueuedConnection);
}


void PathsRenderer::SetLight(std::size_t idx, const t_vec3_gl& pos)
{
	if(m_lights.size() < idx+1)
		m_lights.resize(idx+1);

	m_lights[idx] = pos;
	m_bLightsNeedUpdate = true;
}


void PathsRenderer::UpdateLights()
{
	constexpr int MAX_LIGHTS = 4;	// max. number allowed in shader

	int num_lights = std::min(MAX_LIGHTS, static_cast<int>(m_lights.size()));
	t_real_gl pos[num_lights * 3];

	for(int i=0; i<num_lights; ++i)
	{
		pos[i*3 + 0] = m_lights[i][0];
		pos[i*3 + 1] = m_lights[i][1];
		pos[i*3 + 2] = m_lights[i][2];
	}

	m_pShaders->setUniformValueArray(m_uniLightPos, pos, num_lights, 3);
	m_pShaders->setUniformValue(m_uniNumActiveLights, num_lights);

	m_bLightsNeedUpdate = false;
}


void PathsRenderer::EnablePicker(bool b)
{
	m_bPickerEnabled = b;
}


void PathsRenderer::UpdatePicker()
{
	if(!m_bInitialised || !m_bPlatformSupported || !m_bPickerEnabled) return;

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
	t_vec_gl vecClosestInters = tl2::create<t_vec_gl>({0,0,0,0});
	std::size_t objInters = 0xffffffff;


	QMutexLocker _locker{&m_mutexObj};

	for(std::size_t curObj=0; curObj<m_objs.size(); ++curObj)
	{
		const auto& obj = m_objs[curObj];
		const PathsRendererObj *linkedObj = &obj;
		if(obj.linkedObj)
			linkedObj = &m_objs[*obj.linkedObj];

		if(linkedObj->m_type != PathsRendererObjType::TRIANGLES || !obj.m_visible || !obj.m_valid)
			continue;


		t_mat_gl matTrafo = obj.m_mat;

		// scaling factor, TODO: maximum factor for non-uniform scaling
		auto scale = std::cbrt(std::abs(tl2::det(matTrafo)));

		// intersection with bounding sphere?
		auto boundingInters =
			tl2::intersect_line_sphere<t_vec3_gl, std::vector>(org3, dir3,
				matTrafo * linkedObj->m_boundingSpherePos, scale*linkedObj->m_boundingSphereRad);
		if(boundingInters.size() == 0)
			continue;


		// test actual polygons for intersection
		for(std::size_t startidx=0; startidx+2<linkedObj->m_triangles.size(); startidx+=3)
		{
			std::vector<t_vec3_gl> poly{ {
				linkedObj->m_triangles[startidx+0],
				linkedObj->m_triangles[startidx+1],
				linkedObj->m_triangles[startidx+2]
			} };

			auto [vecInters, bInters, lamInters] =
				tl2::intersect_line_poly<t_vec3_gl, t_mat_gl>(org3, dir3, poly, matTrafo);

			if(bInters)
			{
				t_vec_gl vecInters4 = tl2::create<t_vec_gl>({vecInters[0], vecInters[1], vecInters[2], 1});

				if(!hasInters)
				{	// first intersection
					vecClosestInters = vecInters4;
					objInters = curObj;
					hasInters = true;
				}
				else
				{	// test if next intersection is closer...
					t_vec_gl oldPosTrafo = m_matCam * vecClosestInters;
					t_vec_gl newPosTrafo = m_matCam * vecInters4;

					if(tl2::norm(newPosTrafo) < tl2::norm(oldPosTrafo))
					{	// ...it is closer
						vecClosestInters = vecInters4;
						objInters = curObj;
					}
				}
			}
		}
	}

	m_bPickerNeedsUpdate = false;
	t_vec3_gl vecClosestInters3 = tl2::create<t_vec3_gl>({vecClosestInters[0], vecClosestInters[1], vecClosestInters[2]});
	t_vec3_gl vecClosestSphereInters3 = tl2::create<t_vec3_gl>({vecClosestSphereInters[0], vecClosestSphereInters[1], vecClosestSphereInters[2]});
	emit PickerIntersection(hasInters ? &vecClosestInters3 : nullptr, objInters,
		hasSphereInters ? &vecClosestSphereInters3 : nullptr);
}



void PathsRenderer::tick()
{
	tick(std::chrono::milliseconds(1000 / 60));
}


void PathsRenderer::tick(const std::chrono::milliseconds& ms)
{
	// TODO
	UpdateCam();
}


/**
 * pure gl drawing
 */
void PathsRenderer::DoPaintGL(qgl_funcs *pGl)
{
	if(!pGl)
		return;

	// clear
	pGl->glClearColor(1., 1., 1., 1.);
	pGl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	pGl->glEnable(GL_DEPTH_TEST);


	// bind shaders
	m_pShaders->bind();
	BOOST_SCOPE_EXIT(m_pShaders) { m_pShaders->release(); } BOOST_SCOPE_EXIT_END
	LOGGLERR(pGl);

	if(m_bLightsNeedUpdate) UpdateLights();

	// set cam matrix
	m_pShaders->setUniformValue(m_uniMatrixCam, m_matCam);
	m_pShaders->setUniformValue(m_uniMatrixCamInv, m_matCam_inv);


	auto colOverride = tl2::create<t_vec_gl>({1,1,1,1});
	auto colHighlight = tl2::create<t_vec_gl>({1,1,1,1});

	// render triangle geometry
	for(const auto& obj : m_objs)
	{
		const PathsRendererObj *linkedObj = &obj;
		if(obj.linkedObj)
		{
			// get linked object
			linkedObj = &m_objs[*obj.linkedObj];

			// override constant color for linked object
			if(obj.m_highlighted)
				m_pShaders->setUniformValue(m_uniConstCol, colHighlight);
			else
				m_pShaders->setUniformValue(m_uniConstCol, obj.m_color);
		}
		else
		{
			// set override color to white for non-linked objects
			m_pShaders->setUniformValue(m_uniConstCol, colOverride);
		}

		if(!obj.m_visible || !obj.m_valid) continue;

		m_pShaders->setUniformValue(m_uniMatrixObj, obj.m_mat);

		// main vertex array object
		pGl->glBindVertexArray(linkedObj->m_vertexarr);


		pGl->glEnableVertexAttribArray(m_attrVertex);
		if(linkedObj->m_type == PathsRendererObjType::TRIANGLES)
			pGl->glEnableVertexAttribArray(m_attrVertexNorm);
		pGl->glEnableVertexAttribArray(m_attrVertexCol);
		BOOST_SCOPE_EXIT(pGl, &m_attrVertex, &m_attrVertexNorm, &m_attrVertexCol)
		{
			pGl->glDisableVertexAttribArray(m_attrVertexCol);
			pGl->glDisableVertexAttribArray(m_attrVertexNorm);
			pGl->glDisableVertexAttribArray(m_attrVertex);
		}
		BOOST_SCOPE_EXIT_END
		LOGGLERR(pGl);


		if(linkedObj->m_type == PathsRendererObjType::TRIANGLES)
			pGl->glDrawArrays(GL_TRIANGLES, 0, linkedObj->m_triangles.size());
		else if(linkedObj->m_type == PathsRendererObjType::LINES)
			pGl->glDrawArrays(GL_LINES, 0, linkedObj->m_vertices.size());
		else
			std::cerr << "Unknown plot object type." << std::endl;

		LOGGLERR(pGl);
	}

	pGl->glDisable(GL_DEPTH_TEST);
}


/**
 * directly draw on a qpainter
 */
void PathsRenderer::DoPaintNonGL(QPainter &painter)
{
	QFont fontOrig = painter.font();
	QPen penOrig = painter.pen();

	QPen penLabel(Qt::black);
	painter.setPen(penLabel);


	// coordinate labels
	painter.drawText(GlToScreenCoords(tl2::create<t_vec_gl>({0.,0.,0.,1.})), "0");
	for(t_real_gl f=-std::floor(m_CoordMax); f<=std::floor(m_CoordMax); f+=0.5)
	{
		if(tl2::equals<t_real_gl>(f, 0))
			continue;

		std::ostringstream ostrF;
		ostrF << f;
		painter.drawText(GlToScreenCoords(tl2::create<t_vec_gl>({f,0.,0.,1.})), ostrF.str().c_str());
		painter.drawText(GlToScreenCoords(tl2::create<t_vec_gl>({0.,f,0.,1.})), ostrF.str().c_str());
		painter.drawText(GlToScreenCoords(tl2::create<t_vec_gl>({0.,0.,f,1.})), ostrF.str().c_str());
	}

	painter.drawText(GlToScreenCoords(tl2::create<t_vec_gl>({m_CoordMax*t_real_gl(1.2), 0., 0., 1.})), "x");
	painter.drawText(GlToScreenCoords(tl2::create<t_vec_gl>({0., m_CoordMax*t_real_gl(1.2), 0., 1.})), "y");
	painter.drawText(GlToScreenCoords(tl2::create<t_vec_gl>({0., 0., m_CoordMax*t_real_gl(1.2), 1.})), "z");


	// render object labels
	for(const auto& obj : m_objs)
	{
		if(!obj.m_visible || !obj.m_valid) continue;

		if(obj.m_label != "")
		{
			t_vec3_gl posLabel3d = obj.m_mat * obj.m_labelPos;
			auto posLabel2d = GlToScreenCoords(tl2::create<t_vec_gl>({posLabel3d[0], posLabel3d[1], posLabel3d[2], 1.}));

			QFont fontLabel = fontOrig;
			QPen penLabel = penOrig;

			fontLabel.setStyleStrategy(QFont::StyleStrategy(/*QFont::OpenGLCompatible |*/ QFont::PreferAntialias | QFont::PreferQuality));
			fontLabel.setWeight(QFont::Medium);
			//penLabel.setColor(QColor(int((1.-obj.m_color[0])*255.), int((1.-obj.m_color[1])*255.), int((1.-obj.m_color[2])*255.), int(obj.m_color[3]*255.)));
			penLabel.setColor(QColor(0,0,0,255));
			painter.setFont(fontLabel);
			painter.setPen(penLabel);
			painter.drawText(posLabel2d, obj.m_label.c_str());

			fontLabel.setWeight(QFont::Normal);
			penLabel.setColor(QColor(int(obj.m_color[0]*255.), int(obj.m_color[1]*255.), int(obj.m_color[2]*255.), int(obj.m_color[3]*255.)));
			painter.setFont(fontLabel);
			painter.setPen(penLabel);
			painter.drawText(posLabel2d, obj.m_label.c_str());
		}
	}

	// restore original styles
	painter.setFont(fontOrig);
	painter.setPen(penOrig);
}




void PathsRenderer::initializeGL()
{
	m_bInitialised = false;

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


	// GL functions
	auto *pGl = GetGlFunctions();
	if(!pGl) return;

	m_strGlVer = (char*)pGl->glGetString(GL_VERSION);
	m_strGlShaderVer = (char*)pGl->glGetString(GL_SHADING_LANGUAGE_VERSION);
	m_strGlVendor = (char*)pGl->glGetString(GL_VENDOR);
	m_strGlRenderer = (char*)pGl->glGetString(GL_RENDERER);
	LOGGLERR(pGl);


	// shaders
	{
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

			std::exit(-1);
		};

		// compile & link shaders
		m_pShaders = std::make_shared<QOpenGLShaderProgram>(this);

		if(!m_pShaders->addShaderFromSourceCode(QOpenGLShader::Fragment, strFragShader.c_str()))
			shader_err("Cannot compile fragment shader.");
		if(!m_pShaders->addShaderFromSourceCode(QOpenGLShader::Vertex, strVertexShader.c_str()))
			shader_err("Cannot compile vertex shader.");

		if(!m_pShaders->link())
			shader_err("Cannot link shaders.");

		m_uniMatrixCam = m_pShaders->uniformLocation("cam");
		m_uniMatrixCamInv = m_pShaders->uniformLocation("cam_inv");
		m_uniMatrixProj = m_pShaders->uniformLocation("proj");
		m_uniMatrixObj = m_pShaders->uniformLocation("obj");
		m_uniConstCol = m_pShaders->uniformLocation("constcol");
		m_uniLightPos = m_pShaders->uniformLocation("lightpos");
		m_uniNumActiveLights = m_pShaders->uniformLocation("activelights");
		m_attrVertex = m_pShaders->attributeLocation("vertex");
		m_attrVertexNorm = m_pShaders->attributeLocation("normal");
		m_attrVertexCol = m_pShaders->attributeLocation("vertexcol");
	}
	LOGGLERR(pGl);


	// 3d objects
	AddCoordinateCross(-m_CoordMax, m_CoordMax);


	// options
	pGl->glCullFace(GL_BACK);
	pGl->glEnable(GL_CULL_FACE);

	pGl->glEnable(GL_BLEND);
	pGl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	pGl->glEnable(GL_MULTISAMPLE);
	pGl->glEnable(GL_LINE_SMOOTH);
	pGl->glEnable(GL_POLYGON_SMOOTH);
	pGl->glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	pGl->glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

	m_bInitialised = true;


	if(IsInitialised())
		emit AfterGLInitialisation();
	else
		emit GLInitialisationFailed();
}


void PathsRenderer::resizeGL(int w, int h)
{
	m_iScreenDims[0] = w;
	m_iScreenDims[1] = h;
	m_bWantsResize = true;

	if(!m_bPlatformSupported || !m_bInitialised) return;

	if(auto *pContext = ((QOpenGLWidget*)this)->context(); !pContext)
		return;
	auto *pGl = GetGlFunctions();
	if(!pGl)
		return;

	m_matViewport = tl2::hom_viewport<t_mat_gl>(w, h, 0., 1.);
	std::tie(m_matViewport_inv, std::ignore) = tl2::inv<t_mat_gl>(m_matViewport);

	m_matPerspective = tl2::hom_perspective<t_mat_gl>(0.01, 100., tl2::pi<t_real_gl>*0.5, t_real_gl(h)/t_real_gl(w));
	std::tie(m_matPerspective_inv, std::ignore) = tl2::inv<t_mat_gl>(m_matPerspective);

	pGl->glViewport(0, 0, w, h);
	pGl->glDepthRange(0, 1);

	// bind shaders
	m_pShaders->bind();
	BOOST_SCOPE_EXIT(m_pShaders) { m_pShaders->release(); } BOOST_SCOPE_EXIT_END
	LOGGLERR(pGl);

	// set matrices
	m_pShaders->setUniformValue(m_uniMatrixCam, m_matCam);
	m_pShaders->setUniformValue(m_uniMatrixCamInv, m_matCam_inv);
	m_pShaders->setUniformValue(m_uniMatrixProj, m_matPerspective);
	LOGGLERR(pGl);

	m_bWantsResize = false;
}


void PathsRenderer::paintGL()
{
	if(!m_bPlatformSupported || !m_bInitialised) return;
	QMutexLocker _locker{&m_mutexObj};

	if(auto *pContext = context(); !pContext) return;
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	// gl painting
	{
		BOOST_SCOPE_EXIT(&painter) { painter.endNativePainting(); } BOOST_SCOPE_EXIT_END

		if(m_bPickerNeedsUpdate) UpdatePicker();

		auto *pGl = GetGlFunctions();
		painter.beginNativePainting();
		DoPaintGL(pGl);
	}

	// qt painting
	DoPaintNonGL(painter);
}


void PathsRenderer::mouseMoveEvent(QMouseEvent *pEvt)
{
	m_posMouse = pEvt->localPos();

	if(m_bInRotation)
	{
		auto diff = (m_posMouse - m_posMouseRotationStart);
		t_real_gl phi = diff.x() + m_phi_saved;
		t_real_gl theta = diff.y() + m_theta_saved;

		m_matCamRot = tl2::rotation<t_mat_gl, t_vec_gl>(m_vecCamX, theta/180.*tl2::pi<t_real_gl>, 0);
		m_matCamRot *= tl2::rotation<t_mat_gl, t_vec_gl>(m_vecCamY, phi/180.*tl2::pi<t_real_gl>, 0);

		UpdateCam();
	}
	else
	{
		// also automatically done in UpdateCam
		m_bPickerNeedsUpdate = true;
		RequestPlotUpdate();
	}

	m_mouseMovedBetweenDownAndUp = 1;
	pEvt->accept();
}


void PathsRenderer::mousePressEvent(QMouseEvent *pEvt)
{
	m_mouseMovedBetweenDownAndUp = 0;

	if(pEvt->buttons() & Qt::LeftButton) m_mouseDown[0] = 1;
	if(pEvt->buttons() & Qt::MiddleButton) m_mouseDown[1] = 1;
	if(pEvt->buttons() & Qt::RightButton) m_mouseDown[2] = 1;

	if(m_mouseDown[1])
	{
		// reset zoom
		m_zoom = 1;
		UpdateCam();
	}
	if(m_mouseDown[2])
	{
		// begin rotation
		if(!m_bInRotation)
		{
			m_posMouseRotationStart = m_posMouse;
			m_bInRotation = true;
		}
	}

	pEvt->accept();
	emit MouseDown(m_mouseDown[0], m_mouseDown[1], m_mouseDown[2]);
}


void PathsRenderer::mouseReleaseEvent(QMouseEvent *pEvt)
{
	bool mouseDownOld[] = { m_mouseDown[0], m_mouseDown[1], m_mouseDown[2] };

	if((pEvt->buttons() & Qt::LeftButton) == 0) m_mouseDown[0] = 0;
	if((pEvt->buttons() & Qt::MiddleButton) == 0) m_mouseDown[1] = 0;
	if((pEvt->buttons() & Qt::RightButton) == 0) m_mouseDown[2] = 0;

	if(!m_mouseDown[2])
	{
		// end rotation
		if(m_bInRotation)
		{
			auto diff = (m_posMouse - m_posMouseRotationStart);
			m_phi_saved += diff.x();
			m_theta_saved += diff.y();

			m_bInRotation = false;
		}
	}

	pEvt->accept();
	emit MouseUp(!m_mouseDown[0], !m_mouseDown[1], !m_mouseDown[2]);

	// only emit click if moving the mouse (i.e. rotationg the scene) was not the primary intent
	if(!m_mouseMovedBetweenDownAndUp)
	{
		bool mouseClicked[] = { !m_mouseDown[0] && mouseDownOld[0],
			!m_mouseDown[1] && mouseDownOld[1],
			!m_mouseDown[2] && mouseDownOld[2] };
		if(mouseClicked[0] || mouseClicked[1] || mouseClicked[2])
			emit MouseClick(mouseClicked[0], mouseClicked[1], mouseClicked[2]);
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


/**
 * main thread wants to compose -> wait for sub-threads to be finished
 */
void PathsRenderer::beforeComposing()
{
}


/**
 * main thread has composed -> sub-threads can be unblocked
 */
void PathsRenderer::afterComposing()
{
}


/**
 * main thread wants to resize -> wait for sub-threads to be finished
 */
void PathsRenderer::beforeResizing()
{
}


/**
 * main thread has resized -> sub-threads can be unblocked
 */
void PathsRenderer::afterResizing()
{
}

// ----------------------------------------------------------------------------

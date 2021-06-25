/**
 * paths rendering widget
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-2021
 * @license GPLv3, see 'LICENSE' file
 *
 * References:
 *   - http://doc.qt.io/qt-5/qopenglwidget.html#details
 *   - http://code.qt.io/cgit/qt/qtbase.git/tree/examples/opengl/threadedqopenglwidget
 */

#ifndef __PATHS_WIDGET_H__
#define __PATHS_WIDGET_H__

#include <unordered_map>

#include <QtCore/QTimer>
#include <QtWidgets/QDialog>
#include <QtGui/QMouseEvent>

#include "tlibs2/libs/maths.h"
#include "tlibs2/libs/qt/gl.h"

#include "src/core/InstrumentSpace.h"

using t_real_gl = tl2::t_real_gl;
using t_vec2_gl = tl2::t_vec2_gl;
using t_vec3_gl = tl2::t_vec3_gl;
using t_vec_gl = tl2::t_vec_gl;
using t_mat_gl = tl2::t_mat_gl;



/**
 * rendering object structure
 */
struct PathsObj : public tl2::GlRenderObj
{
	t_mat_gl m_mat = tl2::unit<t_mat_gl>();

	bool m_visible = true;		// object shown?
	bool m_highlighted = false;	// object highlighted?
	bool m_cull = true;		// object faces culled?

	t_vec3_gl m_boundingSpherePos = tl2::create<t_vec3_gl>({ 0., 0., 0. });
	t_real_gl m_boundingSphereRad = 0.;
};



/**
 * rendering widget
 */
class PathsRenderer : public QOpenGLWidget
{ Q_OBJECT

public:
	PathsRenderer(QWidget *pParent = nullptr);
	virtual ~PathsRenderer();

	void Clear();
	void LoadInstrument(const InstrumentSpace& instr);
	void AddWall(const Geometry& geo, bool update_scene=true);

	// receivers for instrument (space) update signals
	void UpdateInstrumentSpace(const InstrumentSpace& instr);
	void UpdateInstrument(const Instrument& instr);


protected:
	virtual void paintEvent(QPaintEvent*) override;
	virtual void initializeGL() override;
	virtual void paintGL() override;
	virtual void resizeGL(int w, int h) override;

	virtual void mouseMoveEvent(QMouseEvent *pEvt) override;
	virtual void mousePressEvent(QMouseEvent *Evt) override;
	virtual void mouseReleaseEvent(QMouseEvent *Evt) override;
	virtual void wheelEvent(QWheelEvent *pEvt) override;
	virtual void keyPressEvent(QKeyEvent *pEvt) override;
	virtual void keyReleaseEvent(QKeyEvent *pEvt) override;


private:
#if QT_VERSION >= 0x060000
	t_qt_mutex m_mutexObj;
#else
	t_qt_mutex m_mutexObj{QMutex::Recursive};
#endif

	bool m_mouseMovedBetweenDownAndUp = false;
	bool m_mouseDown[3] = { 0, 0, 0 };
	bool m_perspectiveProjection = true;
	bool m_arrowDown[4] = { 0, 0, 0, 0 };	// l, r, u, d
	bool m_pageDown[2] = { 0, 0 };

protected slots:
	void tick();


public slots:
	void EnablePicker(bool b);
	void SetPerspectiveProjection(bool b);
	void SetCamViewingAngle(t_real_gl angle);
	void SetCamPosition(const t_vec3_gl& pos);
	void SetCamRotation(const t_vec2_gl& rot);

	t_real_gl GetCamViewingAngle() const { return m_camViewingAngle; }
	bool GetPerspectiveProjection() const { return m_perspectiveProjection; }
	t_vec3_gl GetCamPosition() const;
	t_vec2_gl GetCamRotation() const;

	void EnableTimer(bool enable=true);

signals:
	void AfterGLInitialisation();

	void ObjectClicked(const std::string& obj, bool left, bool mid, bool right);
	void ObjectDragged(bool drag_start, const std::string& obj, t_real_gl x_start, t_real_gl y_start, t_real_gl x, t_real_gl y);
	void FloorPlaneCoordsChanged(t_real_gl x, t_real_gl y);
	void PickerIntersection(const t_vec3_gl* pos, std::string obj_name, const t_vec3_gl* posSphere);
	void CamPositionChanged(t_real_gl x, t_real_gl y, t_real_gl z);
	void CamRotationChanged(t_real_gl phi, t_real_gl theta);

protected:
	// ------------------------------------------------------------------------
	// shader interface
	// ------------------------------------------------------------------------
	std::shared_ptr<QOpenGLShaderProgram> m_pShaders{};

	// vertex attributes
	GLint m_attrVertex = -1;
	GLint m_attrVertexNorm = -1;
	GLint m_attrVertexCol = -1;
	GLint m_attrTexCoords = -1;

	// lighting
	GLint m_uniConstCol = -1;
	GLint m_uniLightPos = -1;
	GLint m_uniNumActiveLights = -1;

	// matrices
	GLint m_uniMatrixProj = -1;
	GLint m_uniMatrixCam = -1;
	GLint m_uniMatrixCamInv = -1;
	GLint m_uniMatrixObj = -1;

	// cursor
	GLint m_uniCursorActive = -1;
	GLint m_uniCursorCoords = -1;
	// ------------------------------------------------------------------------

	// version identifiers
	std::string m_strGlVer, m_strGlShaderVer, m_strGlVendor, m_strGlRenderer;

	// cursor uv coordinates and object under cursor
	GLfloat m_cursorUV[2] = {0., 0.};
	GLfloat m_cursor[2] = {0., 0.};
	GLfloat m_dragstartcursor[2] = {0., 0.};
	std::string m_curObj, m_draggedObj;
	bool m_curActive = false;

	// matrices
	t_mat_gl m_matPerspective = tl2::unit<t_mat_gl>();
	t_mat_gl m_matPerspective_inv = tl2::unit<t_mat_gl>();
	t_mat_gl m_matViewport = tl2::unit<t_mat_gl>();
	t_mat_gl m_matViewport_inv = tl2::unit<t_mat_gl>();
	t_mat_gl m_matCam = tl2::unit<t_mat_gl>();
	t_mat_gl m_matCam_inv = tl2::unit<t_mat_gl>();
	t_mat_gl m_matCamRot = tl2::unit<t_mat_gl>();
	t_mat_gl m_matCamTrans = tl2::create<t_mat_gl>
		({ 1,0,0,0, 0,1,0,0, 0,0,1,-15, 0,0,0,1 });

	t_real_gl m_camViewingAngle = tl2::pi<t_real_gl>*t_real_gl(0.5);
	t_real_gl m_phi = 0, m_theta = 0;
	t_real_gl m_phi_saved = 0, m_theta_saved = 0;
	t_real_gl m_zoom = 1.;

	std::atomic<bool> m_initialised = false;
	std::atomic<bool> m_pickerEnabled = true;
	std::atomic<bool> m_pickerNeedsUpdate = false;
	std::atomic<bool> m_lightsNeedUpdate = false;
	std::atomic<bool> m_perspectiveNeedsUpdate = false;
	std::atomic<bool> m_viewportNeedsUpdate = false;
	std::atomic<int> m_screenDims[2] = { 800, 600 };
	t_real_gl m_pickerSphereRadius = 1;

	std::vector<t_vec3_gl> m_lights;
	std::unordered_map<std::string, PathsObj> m_objs;

	QPointF m_posMouse{};
	QPointF m_posMouseRotationStart{}, m_posMouseRotationEnd{};
	bool m_inRotation = false;

	QTimer m_timer{};


protected:
	qgl_funcs* GetGlFunctions();

	void UpdateCam();
	void UpdatePicker();
	void UpdateLights();
	void UpdatePerspective();
	void UpdateViewport();

	void DoPaintGL(qgl_funcs *pGL);
	void DoPaintQt(QPainter &painter);

	void tick(const std::chrono::milliseconds& ms);


public:
	std::tuple<std::string, std::string, std::string, std::string> GetGlDescr() const;
	bool IsInitialised() const { return m_initialised; }

	QPointF GlToScreenCoords(const t_vec_gl& vec, bool *pVisible=nullptr) const;
	void SetPickerSphereRadius(t_real_gl rad) { m_pickerSphereRadius = rad; }

	void DeleteObject(PathsObj& obj);
	void DeleteObject(const std::string& obj_name);

	void AddTriangleObject(const std::string& obj_name,
		const std::vector<t_vec3_gl>& triag_verts,
		const std::vector<t_vec3_gl>& triag_norms, const std::vector<t_vec3_gl>& triag_uvs,
		t_real_gl r=0, t_real_gl g=0, t_real_gl b=0, t_real_gl a=1);

	void AddFloorPlane(const std::string& obj_name, t_real_gl len_x=10, t_real_gl len_y=10);

	void SetLight(std::size_t idx, const t_vec3_gl& pos);
	void CentreCam(const std::string& obj);
};


#endif

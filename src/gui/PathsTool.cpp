/**
 * TAS path tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "PathsTool.h"

#include <QtCore/QDir>
#include <QtCore/QLoggingCategory>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
namespace pt = boost::property_tree;

#include "tlibs2/libs/maths.h"
#include "tlibs2/libs/str.h"
#include "tlibs2/libs/file.h"
#include "tlibs2/libs/algos.h"
#include "tlibs2/libs/helper.h"


#define MAX_RECENT_FILES 16
#define PROG_TITLE "Triple-Axis Paths Tool"


// ----------------------------------------------------------------------------
/**
 * event signalling that the crystal UB matrix needs an update
 */
void PathsTool::UpdateUB()
{
	m_UB = tl2::UB_matrix<t_mat, t_vec>(m_B, m_plane_rlu[0], m_plane_rlu[1], m_plane_rlu[2]);

	/*using namespace tl2_ops;
	std::cout << "vec1: " << m_plane_rlu[0] << std::endl;
	std::cout << "vec2: " << m_plane_rlu[1] << std::endl;
	std::cout << "vec3: " << m_plane_rlu[2] << std::endl;
	std::cout << "B matrix: " << m_B << std::endl;
	std::cout << "UB matrix: " << m_UB << std::endl;*/

	if(m_xtalInfos)
		m_xtalInfos->GetWidget()->SetUB(m_B, m_UB);
}


/**
 * the window is being shown
 */
void PathsTool::showEvent(QShowEvent *evt)
{
	m_renderer->EnableTimer(true);
	QMainWindow::showEvent(evt);
}


/**
 * the window is being hidden
 */
void PathsTool::hideEvent(QHideEvent *evt)
{
	m_renderer->EnableTimer(false);
	QMainWindow::hideEvent(evt);
}


/**
 * the window is being closed
 */
void PathsTool::closeEvent(QCloseEvent *evt)
{
	// save window size, position, and state
	m_sett.setValue("geo", saveGeometry());
	m_sett.setValue("state", saveState());

	// remove superfluous entries and save the recent files list
	while(m_recentFiles.size() > MAX_RECENT_FILES)
		m_recentFiles.pop_front();
	m_sett.setValue("recent_files", m_recentFiles);

	QMainWindow::closeEvent(evt);
}


/**
 * File -> New
 */
void PathsTool::NewFile()
{
	SetCurrentFile("");

	m_instrspace.Clear();

	if(m_dlgGeoBrowser)
		m_dlgGeoBrowser->UpdateGeoTree(m_instrspace);
	if(m_renderer)
		m_renderer->LoadInstrument(m_instrspace);
}


/**
 * File -> Open
 */
void PathsTool::OpenFile()
{
	QString dirLast = m_sett.value("cur_dir", "~/").toString();

	QString filename = QFileDialog::getOpenFileName(
		this, "Open File", dirLast, "Paths Files (*.taspaths)");
	if(filename=="" || !QFile::exists(filename))
		return;

	if(OpenFile(filename))
		m_sett.setValue("cur_dir", QFileInfo(filename).path());
}


/**
 * File -> Save
 */
void PathsTool::SaveFile()
{
	if(m_curFile == "")
		SaveFileAs();
	else
		SaveFile(m_curFile);
}


/**
 * File -> Save As
 */
void PathsTool::SaveFileAs()
{
	QString dirLast = m_sett.value("cur_dir", "~/").toString();

	QString filename = QFileDialog::getSaveFileName(
		this, "Save File", dirLast, "Paths Files (*.taspaths)");
	if(filename=="")
		return;

	if(SaveFile(filename))
		m_sett.setValue("cur_dir", QFileInfo(filename).path());
}


/**
 * load file
 */
bool PathsTool::OpenFile(const QString &file)
{
	try
	{
		NewFile();

		// load instrument definition file
		if(auto [instrok, msg] = 
			InstrumentSpace::load(file.toStdString(), m_instrspace); !instrok)
		{
			QMessageBox::critical(this, "Error", msg.c_str());
			return false;
		}
		else
		{
			std::ostringstream ostr;
			ostr << "Loaded \"" << QFileInfo{file}.fileName().toStdString() << "\" "
				<< "dated " << msg << ".";
			m_statusbar->showMessage(ostr.str().c_str());
		}

		SetCurrentFile(file);
		AddRecentFile(file);

		if(m_dlgGeoBrowser)
			m_dlgGeoBrowser->UpdateGeoTree(m_instrspace);
		if(m_renderer)
			m_renderer->LoadInstrument(m_instrspace);

		// update slot for instrument space (e.g. walls) changes
		m_instrspace.AddUpdateSlot(
			[this](const InstrumentSpace& instrspace)
			{
				if(m_renderer)
					m_renderer->UpdateInstrumentSpace(instrspace);
			});

		// update slot for instrument movements
		m_instrspace.GetInstrument().AddUpdateSlot(
			[this](const Instrument& instr)
			{
				// get scattering angles
				t_real monoScAngle = m_instrspace.GetInstrument().GetMonochromator().GetAxisAngleOut();
				t_real sampleScAngle = m_instrspace.GetInstrument().GetSample().GetAxisAngleOut();
				t_real anaScAngle = m_instrspace.GetInstrument().GetAnalyser().GetAxisAngleOut();
				m_tasProperties->GetWidget()->SetMonoScatteringAngle(monoScAngle*t_real{180}/tl2::pi<t_real>);
				m_tasProperties->GetWidget()->SetSampleScatteringAngle(sampleScAngle*t_real{180}/tl2::pi<t_real>);
				m_tasProperties->GetWidget()->SetAnaScatteringAngle(anaScAngle*t_real{180}/tl2::pi<t_real>);

				// get crystal angles
				t_real monoXtalAngle = m_instrspace.GetInstrument().GetMonochromator().GetAxisAngleInternal();
				t_real sampleXtalAngle = m_instrspace.GetInstrument().GetSample().GetAxisAngleInternal();
				t_real anaXtalAngle = m_instrspace.GetInstrument().GetAnalyser().GetAxisAngleInternal();
				m_tasProperties->GetWidget()->SetMonoCrystalAngle(monoXtalAngle*t_real{180}/tl2::pi<t_real>);
				m_tasProperties->GetWidget()->SetSampleCrystalAngle(sampleXtalAngle*t_real{180}/tl2::pi<t_real>);
				m_tasProperties->GetWidget()->SetAnaCrystalAngle(anaXtalAngle*t_real{180}/tl2::pi<t_real>);

				// get crystal coordinates corresponding to current instrument position
				t_real ki = tl2::calc_tas_k<t_real>(monoXtalAngle, m_dspacings[0]);
				t_real kf = tl2::calc_tas_k<t_real>(anaXtalAngle, m_dspacings[1]);
				t_real Q = tl2::calc_tas_Q_len<t_real>(ki, kf, sampleScAngle);
				t_real E = tl2::calc_tas_E<t_real>(ki, kf);

				auto Qrlu = tl2::calc_tas_hkl<t_mat, t_vec, t_real>(
					m_B, ki, kf, Q, sampleXtalAngle,
					m_plane_rlu[0], m_plane_rlu[2],
					m_sensesCCW[1], g_a3_offs);

				SetInstrumentStatus(Qrlu, E, m_instrspace.CheckCollision2D());

				if(this->m_dlgConfigSpace)
					this->m_dlgConfigSpace->UpdateInstrument(instr, m_sensesCCW);

				if(this->m_renderer)
					this->m_renderer->UpdateInstrument(instr);
			});

		m_instrspace.GetInstrument().EmitUpdate();
	}
	catch(const std::exception& ex)
	{
		QMessageBox::critical(this, "Error",
			QString{"Instrument configuration error: "} + ex.what() + QString{"."});
		return false;
	}
	return true;
}


/**
 * save file
 */
bool PathsTool::SaveFile(const QString &file)
{
	if(file=="")
		return false;

	pt::ptree prop = m_instrspace.Save();

	// set format and version
	prop.put(FILE_BASENAME "ident", PROG_IDENT);
	prop.put(FILE_BASENAME "timestamp", tl2::var_to_str(tl2::epoch<t_real>()));

	std::ofstream ofstr{file.toStdString()};
	if(!ofstr)
	{
		QMessageBox::critical(this, "Error", "Could not save file.");
		return false;
	}

	ofstr.precision(g_prec);
	pt::write_xml(ofstr, prop, pt::xml_writer_make_settings('\t', 1, std::string{"utf-8"}));

	SetCurrentFile(file);
	AddRecentFile(file);
	return true;
}


/**
 * adds a file to the recent files menu
 */
void PathsTool::AddRecentFile(const QString &file)
{
	for(const auto &recentfile : m_recentFiles)
	{
		// file already in list?
		if(recentfile == file)
			return;
	}

	m_recentFiles.push_back(file);
	RebuildRecentFiles();
}


/**
 * remember current file and set window title
 */
void PathsTool::SetCurrentFile(const QString &file)
{
	static const QString title(PROG_TITLE);
	m_curFile = file;

	if(m_curFile == "")
		this->setWindowTitle(title);
	else
		this->setWindowTitle(title + " -- " + m_curFile);
}


/**
 * sets the recent file menu
 */
void PathsTool::SetRecentFiles(const QStringList &files)
{
	m_recentFiles = files;
	RebuildRecentFiles();
}


/**
 * creates the "recent files" sub-menu
 */
void PathsTool::RebuildRecentFiles()
{
	m_menuOpenRecent->clear();

	std::size_t num_recent_files = 0;
	for(auto iter = m_recentFiles.rbegin(); iter != m_recentFiles.rend(); ++iter)
	{
		QString filename = *iter;
		auto *acFile = new QAction(QIcon::fromTheme("document"), filename, m_menubar);

		connect(acFile, &QAction::triggered, [this, filename]() { this->OpenFile(filename); });
		m_menuOpenRecent->addAction(acFile);

		if(++num_recent_files >= MAX_RECENT_FILES)
			break;
	}
}


/**
 * go to crystal coordinates
 */
void PathsTool::GotoCoordinates(t_real h, t_real k, t_real l, t_real ki, t_real kf)
{
	std::optional<t_real> a1 = tl2::calc_tas_a1<t_real>(ki, m_dspacings[0]);
	std::optional<t_real> a5 = tl2::calc_tas_a1<t_real>(kf, m_dspacings[1]);

	if(!a1)
	{
		QMessageBox::critical(this, "Error", "Invalid monochromator angle.");
		return;
	}

	if(!a5)
	{
		QMessageBox::critical(this, "Error", "Invalid analyser angle.");
		return;
	}

	*a1 *= m_sensesCCW[0];
	*a5 *= m_sensesCCW[2];

	t_vec Q = tl2::create<t_vec>({h, k, l});
	auto [ok, a3, a4, dist] = calc_tas_a3a4<t_mat, t_vec, t_real>(
		m_B, ki, kf, Q,
		m_plane_rlu[0], m_plane_rlu[2],
		m_sensesCCW[1], g_a3_offs);

	if(!ok)
	{
		QMessageBox::critical(this, "Error", "Invalid scattering angles.");
		return;
	}

	// set scattering angles
	m_instrspace.GetInstrument().GetMonochromator().SetAxisAngleOut(t_real{2} * *a1);
	m_instrspace.GetInstrument().GetSample().SetAxisAngleOut(a4);
	m_instrspace.GetInstrument().GetAnalyser().SetAxisAngleOut(t_real{2} * *a5);

	// set crystal angles
	m_instrspace.GetInstrument().GetMonochromator().SetAxisAngleInternal(*a1);
	m_instrspace.GetInstrument().GetSample().SetAxisAngleInternal(a3);
	m_instrspace.GetInstrument().GetAnalyser().SetAxisAngleInternal(*a5);
}


/**
 * set the instrument angles to the specified ones
 * (angles have to be positive as scattering senses are applied in the function)
 */
void PathsTool::GotoAngles(std::optional<t_real> a1,
	std::optional<t_real> a3, std::optional<t_real> a4,
	std::optional<t_real> a5)
{
	// set mono angle
	if(a1)
	{
		*a1 *= m_sensesCCW[0];
		m_instrspace.GetInstrument().GetMonochromator(). SetAxisAngleOut(t_real{2} * *a1);
		m_instrspace.GetInstrument().GetMonochromator().SetAxisAngleInternal(*a1);
	}

	// set sample crystal angle
	if(a3)
	{
		*a3 *= m_sensesCCW[1];
		m_instrspace.GetInstrument().GetSample().SetAxisAngleInternal(*a3);
	}

	// set sample scattering angle
	if(a4)
	{
		*a4 *= m_sensesCCW[1];
		m_instrspace.GetInstrument().GetSample().SetAxisAngleOut(*a4);
	}

	// set ana angle
	if(a5)
	{
		*a5 *= m_sensesCCW[2];
		m_instrspace.GetInstrument().GetAnalyser().SetAxisAngleOut(t_real{2} * *a5);
		m_instrspace.GetInstrument().GetAnalyser().SetAxisAngleInternal(*a5);
	}

}


/**
 * called after the plotter has initialised
 */
void PathsTool::AfterGLInitialisation()
{
	// GL device info
	std::tie(m_gl_ver, m_gl_shader_ver, m_gl_vendor, m_gl_renderer)
		= m_renderer->GetGlDescr();

	// get viewing angle
	t_real viewingAngle = m_renderer ? m_renderer->GetCamViewingAngle() : tl2::pi<t_real>*0.5;
	m_camProperties->GetWidget()->SetViewingAngle(viewingAngle*t_real{180}/tl2::pi<t_real>);

	// get perspective projection flag
	bool persp = m_renderer ? m_renderer->GetPerspectiveProjection() : true;
	m_camProperties->GetWidget()->SetPerspectiveProj(persp);

	// get camera position
	t_vec3_gl campos = m_renderer ? m_renderer->GetCamPosition() : tl2::zero<t_vec3_gl>(3);
	m_camProperties->GetWidget()->SetCamPosition(t_real(campos[0]), t_real(campos[1]), t_real(campos[2]));

	// get camera rotation
	t_vec2_gl camrot = m_renderer ? m_renderer->GetCamRotation() : tl2::zero<t_vec2_gl>(2);
	m_camProperties->GetWidget()->SetCamRotation(
		t_real(camrot[0])*t_real{180}/tl2::pi<t_real>, 
		t_real(camrot[1])*t_real{180}/tl2::pi<t_real>);

	// load an initial instrument definition
	if(std::string instrfile = find_resource(m_initialInstrFile); !instrfile.empty())
	{
		if(OpenFile(instrfile.c_str()))
			m_renderer->LoadInstrument(m_instrspace);
	}
}


/**
 * mouse coordinates on base plane
 */
void PathsTool::CursorCoordsChanged(t_real_gl x, t_real_gl y)
{
	m_mouseX = x;
	m_mouseY = y;
	UpdateStatusLabel();
}


/**
 * mouse is over an object
 */
void PathsTool::PickerIntersection(const t_vec3_gl* pos, std::string obj_name, const t_vec3_gl* posSphere)
{
	m_curObj = obj_name;
	UpdateStatusLabel();
}


/**
 * clicked on an object
 */
void PathsTool::ObjectClicked(const std::string& obj, bool left, bool middle, bool right)
{
	if(middle || right)
		m_renderer->CentreCam(obj);
}


/**
 * dragging an object
 */
void PathsTool::ObjectDragged(bool drag_start, const std::string& obj, 
	t_real_gl x_start, t_real_gl y_start, t_real_gl x, t_real_gl y)
{
	/*std::cout << "Dragging " << obj 
		<< " from (" << x_start << ", " << y_start << ")"
		<< " to (" << x << ", " << y << ")." << std::endl;*/

	m_instrspace.DragObject(drag_start, obj, x_start, y_start, x, y);
}


void PathsTool::UpdateStatusLabel()
{
	std::ostringstream ostr;
	ostr.precision(g_prec_gui);
	ostr << std::fixed << std::showpos 
		<< "Cursor: (" << m_mouseX << ", " << m_mouseY << ") m";
	if(m_curObj != "")
		ostr << ", object: " << m_curObj;
	ostr << ".";
	m_labelStatus->setText(ostr.str().c_str());
}


void PathsTool::SetInstrumentStatus(const std::optional<t_vec>& Qopt, t_real E, bool colliding)
{
	using namespace tl2_ops;

	std::ostringstream ostr;
	ostr.precision(g_prec_gui);
	//ostr << "Position: ";
	if(Qopt)
	{
		t_vec Q = *Qopt;
		tl2::set_eps_0<t_vec>(Q, g_eps_gui);
		ostr << std::fixed << "Q = (" << Q << ") rlu, ";
	}
	else
		ostr << "Q invalid, ";

	tl2::set_eps_0<t_real>(E, g_eps_gui);
	ostr << std::fixed << "E = " << E << " meV, ";

	if(colliding)
		ostr << "collision detected!";
	else
		ostr << "no collision.";

	m_labelCollisionStatus->setText(ostr.str().c_str());
}


/**
 * create UI
 */
PathsTool::PathsTool(QWidget* pParent) : QMainWindow{pParent}
{
	setWindowTitle(PROG_TITLE);

	// restore settings
	SettingsDlg::ReadSettings(&m_sett);


	// --------------------------------------------------------------------
	// rendering widget
	// --------------------------------------------------------------------
	// set gl surface format
	m_renderer->setFormat(tl2::gl_format(true, _GL_MAJ_VER, _GL_MIN_VER,
		m_multisamples, m_renderer->format()));

	auto plotpanel = new QWidget(this);

	connect(m_renderer.get(), &PathsRenderer::FloorPlaneCoordsChanged, this, &PathsTool::CursorCoordsChanged);
	connect(m_renderer.get(), &PathsRenderer::PickerIntersection, this, &PathsTool::PickerIntersection);
	connect(m_renderer.get(), &PathsRenderer::ObjectClicked, this, &PathsTool::ObjectClicked);
	connect(m_renderer.get(), &PathsRenderer::ObjectDragged, this, &PathsTool::ObjectDragged);
	connect(m_renderer.get(), &PathsRenderer::AfterGLInitialisation, this, &PathsTool::AfterGLInitialisation);

	// camera position
	connect(m_renderer.get(), &PathsRenderer::CamPositionChanged,
		[this](t_real_gl _x, t_real_gl _y, t_real_gl _z) -> void
		{
			t_real x = t_real(_x);
			t_real y = t_real(_y);
			t_real z = t_real(_z);

			if(m_camProperties)
				m_camProperties->GetWidget()->SetCamPosition(x, y, z);
		});

	// camera rotation
	connect(m_renderer.get(), &PathsRenderer::CamRotationChanged,
		[this](t_real_gl _phi, t_real_gl _theta) -> void
		{
			t_real phi = t_real(_phi);
			t_real theta = t_real(_theta);

			if(m_camProperties)
				m_camProperties->GetWidget()->SetCamRotation(
					phi*t_real{180}/tl2::pi<t_real>, 
					theta*t_real{180}/tl2::pi<t_real>);
		});

	auto pGrid = new QGridLayout(plotpanel);
	pGrid->setSpacing(4);
	pGrid->setContentsMargins(4,4,4,4);

	pGrid->addWidget(m_renderer.get(), 0,0,1,4);

	setCentralWidget(plotpanel);
	// --------------------------------------------------------------------


	// --------------------------------------------------------------------
	// dock widgets
	// --------------------------------------------------------------------
	setDockOptions(QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks | QMainWindow::VerticalTabs);

	m_tasProperties = std::make_shared<TASPropertiesDockWidget>(this);
	m_xtalProperties = std::make_shared<XtalPropertiesDockWidget>(this);
	m_xtalInfos = std::make_shared<XtalInfoDockWidget>(this);
	m_pathProperties = std::make_shared<PathPropertiesDockWidget>(this);
	m_camProperties = std::make_shared<CamPropertiesDockWidget>(this);

	addDockWidget(Qt::LeftDockWidgetArea, m_tasProperties.get());
	addDockWidget(Qt::LeftDockWidgetArea, m_xtalProperties.get());
	addDockWidget(Qt::LeftDockWidgetArea, m_xtalInfos.get());
	addDockWidget(Qt::RightDockWidgetArea, m_pathProperties.get());
	addDockWidget(Qt::RightDockWidgetArea, m_camProperties.get());

	auto* taswidget = m_tasProperties->GetWidget().get();
	auto* xtalwidget = m_xtalProperties->GetWidget().get();
	auto* pathwidget = m_pathProperties->GetWidget().get();
	auto* camwidget = m_camProperties->GetWidget().get();

	// scattering angles
	connect(taswidget, &TASPropertiesWidget::MonoScatteringAngleChanged,
		[this](t_real angle) -> void
		{
			m_instrspace.GetInstrument().GetMonochromator().
				SetAxisAngleOut(angle/t_real{180}*tl2::pi<t_real>);
		});
	connect(taswidget, &TASPropertiesWidget::SampleScatteringAngleChanged,
		[this](t_real angle) -> void
		{
			m_instrspace.GetInstrument().GetSample().
				SetAxisAngleOut(angle/t_real{180}*tl2::pi<t_real>);
		});
	connect(taswidget, &TASPropertiesWidget::AnaScatteringAngleChanged,
		[this](t_real angle) -> void
		{
			m_instrspace.GetInstrument().GetAnalyser().
				SetAxisAngleOut(angle/t_real{180}*tl2::pi<t_real>);
		});

	// crystal angles
	connect(taswidget, &TASPropertiesWidget::MonoCrystalAngleChanged,
		[this](t_real angle) -> void
		{
			m_instrspace.GetInstrument().GetMonochromator().
				SetAxisAngleInternal(angle/t_real{180}*tl2::pi<t_real>);
		});
	connect(taswidget, &TASPropertiesWidget::SampleCrystalAngleChanged,
		[this](t_real angle) -> void
		{
			m_instrspace.GetInstrument().GetSample().
				SetAxisAngleInternal(angle/t_real{180}*tl2::pi<t_real>);
		});
	connect(taswidget, &TASPropertiesWidget::AnaCrystalAngleChanged,
		[this](t_real angle) -> void
		{
			m_instrspace.GetInstrument().GetAnalyser().
				SetAxisAngleInternal(angle/t_real{180}*tl2::pi<t_real>);
		});

	// d spacings
	connect(taswidget, &TASPropertiesWidget::DSpacingsChanged,
		[this](t_real dmono, t_real dana) -> void
		{
			m_dspacings[0] = dmono;
			m_dspacings[1] = dana;
		});

	// scattering senses
	connect(taswidget, &TASPropertiesWidget::ScatteringSensesChanged,
		[this](bool monoccw, bool sampleccw, bool anaccw) -> void
		{
			m_sensesCCW[0] = (monoccw ? 1 : -1);
			m_sensesCCW[1] = (sampleccw ? 1 : -1);
			m_sensesCCW[2] = (anaccw ? 1 : -1);
		});

	// camera viewing angle
	connect(camwidget, &CamPropertiesWidget::ViewingAngleChanged,
		[this](t_real angle) -> void
		{
			if(m_renderer)
				m_renderer->SetCamViewingAngle(angle/t_real{180}*tl2::pi<t_real>);
		});

	// camera projection
	connect(camwidget, &CamPropertiesWidget::PerspectiveProjChanged,
		[this](bool persp) -> void
		{
			if(m_renderer)
				m_renderer->SetPerspectiveProjection(persp);
		});

	// camera position
	connect(camwidget, &CamPropertiesWidget::CamPositionChanged,
		[this](t_real _x, t_real _y, t_real _z) -> void
		{
			t_real_gl x = t_real_gl(_x);
			t_real_gl y = t_real_gl(_y);
			t_real_gl z = t_real_gl(_z);

			if(m_renderer)
				m_renderer->SetCamPosition(tl2::create<t_vec3_gl>({x, y, z}));
		});

	// camera rotation
	connect(camwidget, &CamPropertiesWidget::CamRotationChanged,
		[this](t_real _phi, t_real _theta) -> void
		{
			t_real_gl phi = t_real_gl(_phi);
			t_real_gl theta = t_real_gl(_theta);

			if(m_renderer)
				m_renderer->SetCamRotation(tl2::create<t_vec2_gl>(
					{phi/t_real_gl{180}*tl2::pi<t_real_gl>, 
					theta/t_real_gl{180}*tl2::pi<t_real_gl>}));
		});

	// lattice constants and angles
	connect(xtalwidget, &XtalPropertiesWidget::LatticeChanged,
		[this](t_real a, t_real b, t_real c, t_real alpha, t_real beta, t_real gamma) -> void
		{
			m_B = tl2::B_matrix<t_mat>(a, b, c, alpha, beta, gamma);
			m_plane_rlu[2] = tl2::cross<t_mat, t_vec>(m_B, m_plane_rlu[0], m_plane_rlu[1]);

			UpdateUB();
		});

	connect(xtalwidget, &XtalPropertiesWidget::PlaneChanged,
		[this](t_real vec1_x, t_real vec1_y, t_real vec1_z, t_real vec2_x, t_real vec2_y, t_real vec2_z) -> void
		{
			m_plane_rlu[0] = tl2::create<t_vec>({vec1_x, vec1_y, vec1_z});
			m_plane_rlu[1] = tl2::create<t_vec>({vec2_x, vec2_y, vec2_z});
			m_plane_rlu[2] = tl2::cross<t_mat, t_vec>(m_B, m_plane_rlu[0], m_plane_rlu[1]);

			UpdateUB();
		});

	// goto coordinates
	connect(pathwidget, &PathPropertiesWidget::Goto, this, &PathsTool::GotoCoordinates);
	// --------------------------------------------------------------------


	// --------------------------------------------------------------------
	// menu bar
	// --------------------------------------------------------------------
	m_menubar = new QMenuBar(this);


	// file menu
	QMenu *menuFile = new QMenu("File", m_menubar);

	QAction *actionNew = new QAction(QIcon::fromTheme("document-new"), "New", menuFile);
	QAction *actionOpen = new QAction(QIcon::fromTheme("document-open"), "Open...", menuFile);
	QAction *actionSave = new QAction(QIcon::fromTheme("document-save"), "Save", menuFile);
	QAction *actionSaveAs = new QAction(QIcon::fromTheme("document-save-as"), "Save As...", menuFile);
	QAction *actionSettings = new QAction(QIcon::fromTheme("preferences-system"), "Settings", menuFile);
	QAction *actionQuit = new QAction(QIcon::fromTheme("application-exit"), "Quit", menuFile);

	// shortcuts
	actionNew->setShortcut(QKeySequence::New);
	actionOpen->setShortcut(QKeySequence::Open);
	actionSave->setShortcut(QKeySequence::Save);
	actionSaveAs->setShortcut(QKeySequence::SaveAs);
	actionSettings->setShortcut(QKeySequence::Preferences);
	actionQuit->setShortcut(QKeySequence::Quit);

	m_menuOpenRecent = new QMenu("Open Recent", menuFile);
	m_menuOpenRecent->setIcon(QIcon::fromTheme("document-open-recent"));

	actionSettings->setMenuRole(QAction::PreferencesRole);
	actionQuit->setMenuRole(QAction::QuitRole);

	connect(actionNew, &QAction::triggered, this, [this]() { this->NewFile(); });
	connect(actionOpen, &QAction::triggered, this, [this]() { this->OpenFile(); });
	connect(actionSave, &QAction::triggered, this, [this]() { this->SaveFile(); });
	connect(actionSaveAs, &QAction::triggered, this, [this]() { this->SaveFileAs(); });
	connect(actionQuit, &QAction::triggered, this, &PathsTool::close);

	connect(actionSettings, &QAction::triggered, this, [this]()
	{
		if(!this->m_dlgSettings)
			this->m_dlgSettings = std::make_shared<SettingsDlg>(this, &m_sett);

		m_dlgSettings->show();
		m_dlgSettings->raise();
		m_dlgSettings->activateWindow();
	});

	menuFile->addAction(actionNew);
	menuFile->addSeparator();
	menuFile->addAction(actionOpen);
	menuFile->addMenu(m_menuOpenRecent);
	menuFile->addSeparator();
	menuFile->addAction(actionSave);
	menuFile->addAction(actionSaveAs);
	menuFile->addSeparator();
	menuFile->addAction(actionSettings);
	menuFile->addSeparator();
	menuFile->addAction(actionQuit);


	// view menu
	QMenu *menuView = new QMenu("View", m_menubar);

	menuView->addAction(m_tasProperties->toggleViewAction());
	menuView->addAction(m_xtalProperties->toggleViewAction());
	menuView->addAction(m_xtalInfos->toggleViewAction());
	menuView->addAction(m_pathProperties->toggleViewAction());
	menuView->addAction(m_camProperties->toggleViewAction());
	//menuView->addSeparator();
	//menuView->addAction(acPersp);


	// geometry menu
	QMenu *menuGeo = new QMenu("Geometry", m_menubar);

	QAction *actionAddCuboidWall = new QAction("Add Wall", menuGeo);
	QAction *actionAddCylindricalWall = new QAction("Add Pillar", menuGeo);
	QAction *actionGeoBrowser = new QAction("Geometries Browser...", menuGeo);

	connect(actionAddCuboidWall, &QAction::triggered, this, [this]()
	{
		auto wall = std::make_shared<BoxGeometry>();
		wall->SetHeight(4.);
		wall->SetDepth(0.5);
		wall->SetCentre(tl2::create<t_vec>({0, 0, wall->GetHeight()*0.5}));
		wall->SetLength(4.);
		wall->UpdateTrafo();

		static std::size_t wallcnt = 1;
		std::ostringstream ostrId;
		ostrId << "new wall " << wallcnt++;

		m_instrspace.AddWall(std::vector<std::shared_ptr<Geometry>>{{wall}}, ostrId.str());

		if(m_dlgGeoBrowser)
			m_dlgGeoBrowser->UpdateGeoTree(m_instrspace);

		if(m_renderer)
			m_renderer->AddWall(*wall, true);
	});

	connect(actionAddCylindricalWall, &QAction::triggered, this, [this]()
	{
		auto wall = std::make_shared<CylinderGeometry>();
		wall->SetHeight(4.);
		wall->SetCentre(tl2::create<t_vec>({0, 0, wall->GetHeight()*0.5}));
		wall->SetRadius(0.5);
		wall->UpdateTrafo();

		static std::size_t wallcnt = 1;
		std::ostringstream ostrId;
		ostrId << "new pillar " << wallcnt++;

		m_instrspace.AddWall(std::vector<std::shared_ptr<Geometry>>{{wall}}, ostrId.str());

		if(m_dlgGeoBrowser)
			m_dlgGeoBrowser->UpdateGeoTree(m_instrspace);

		if(m_renderer)
			m_renderer->AddWall(*wall, true);
	});

	connect(actionGeoBrowser, &QAction::triggered, this, [this]()
	{
		if(!this->m_dlgGeoBrowser)
		{
			this->m_dlgGeoBrowser = std::make_shared<GeometriesBrowser>(this, &m_sett);
			this->m_dlgGeoBrowser->UpdateGeoTree(this->m_instrspace);
		}

		m_dlgGeoBrowser->show();
		m_dlgGeoBrowser->raise();
		m_dlgGeoBrowser->activateWindow();
	});

	menuGeo->addAction(actionAddCuboidWall);
	menuGeo->addAction(actionAddCylindricalWall);
	menuGeo->addSeparator();
	menuGeo->addAction(actionGeoBrowser);



	// calculate menu
	QMenu *menuCalc = new QMenu("Calculation", m_menubar);

	QAction *actionConfigSpace = new QAction("Configuration Space...", menuCalc);

	connect(actionConfigSpace, &QAction::triggered, this, [this]()
	{
		if(!this->m_dlgConfigSpace)
		{
			this->m_dlgConfigSpace = std::make_shared<ConfigSpaceDlg>(this, &m_sett);
			this->m_dlgConfigSpace->SetPathsBuilder(&this->m_pathsbuilder);

			this->connect(this->m_dlgConfigSpace.get(), &ConfigSpaceDlg::GotoAngles, this, &PathsTool::GotoAngles);
		}

		m_dlgConfigSpace->show();
		m_dlgConfigSpace->raise();
		m_dlgConfigSpace->activateWindow();
	});

	menuCalc->addAction(actionConfigSpace);


	// tools menu
	QMenu *menuTools = new QMenu("Tools", m_menubar);

	fs::path hullpath = fs::path(g_apppath) / fs::path("hull");
	fs::path linespath = fs::path(g_apppath) / fs::path("lines");

	std::size_t num_tools = 0;
	if(fs::exists(hullpath))
	{
		QAction *acHullTool = new QAction("Convex Hull...", menuTools);
		menuTools->addAction(acHullTool);
		++num_tools;

		connect(acHullTool, &QAction::triggered, this, [hullpath]()
		{
			std::system((hullpath.string() + "&").c_str());
		});
	}

	if(fs::exists(linespath))
	{
		QAction *acLinesTool = new QAction("Line Segments...", menuTools);
		menuTools->addAction(acLinesTool);
		++num_tools;

		connect(acLinesTool, &QAction::triggered, this, [linespath]()
		{
			std::system((linespath.string() + "&").c_str());
		});
	}


	// help menu
	QMenu *menuHelp = new QMenu("Help", m_menubar);

	QAction *actionAboutQt = new QAction(QIcon::fromTheme("help-about"), "About Qt Libraries...", menuHelp);
	QAction *actionAboutGl = new QAction(QIcon::fromTheme("help-about"), "About Renderer...", menuHelp);
	QAction *actionAbout = new QAction(QIcon::fromTheme("help-about"), "About TAS-Paths...", menuHelp);

	actionAboutQt->setMenuRole(QAction::AboutQtRole);
	actionAbout->setMenuRole(QAction::AboutRole);

	connect(actionAboutQt, &QAction::triggered, this, []() { qApp->aboutQt(); });

	connect(actionAboutGl, &QAction::triggered, this, [this]()
	{
		std::ostringstream ostrInfo;
		ostrInfo << "Rendering using the following device:\n\n";
		ostrInfo << "GL Vendor: " << m_gl_vendor << "\n";
		ostrInfo << "GL Renderer: " << m_gl_renderer << "\n";
		ostrInfo << "GL Version: " << m_gl_ver << "\n";
		ostrInfo << "GL Shader Version: " << m_gl_shader_ver << "\n";
		QMessageBox::information(this, "About Renderer", ostrInfo.str().c_str());
	});

	connect(actionAbout, &QAction::triggered, this, [this]()
	{
		if(!this->m_dlgAbout)
			this->m_dlgAbout = std::make_shared<AboutDlg>(this, &m_sett);

		m_dlgAbout->show();
		m_dlgAbout->raise();
		m_dlgAbout->activateWindow();
	});

	menuHelp->addAction(actionAboutQt);
	menuHelp->addAction(actionAboutGl);
	menuHelp->addSeparator();
	menuHelp->addAction(actionAbout);


	// menu bar
	m_menubar->addMenu(menuFile);
	m_menubar->addMenu(menuView);
	m_menubar->addMenu(menuGeo);
	m_menubar->addMenu(menuCalc);
	if(num_tools)
		m_menubar->addMenu(menuTools);
	m_menubar->addMenu(menuHelp);
	//m_menubar->setNativeMenuBar(0);
	setMenuBar(m_menubar);
	// --------------------------------------------------------------------


	// --------------------------------------------------------------------
	// status bar
	// --------------------------------------------------------------------
	m_labelStatus = new QLabel(this);
	m_labelStatus->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_labelStatus->setFrameStyle(int(QFrame::Sunken) | int(QFrame::Panel));
	m_labelStatus->setLineWidth(1);

	m_labelCollisionStatus = new QLabel(this);
	m_labelCollisionStatus->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_labelCollisionStatus->setFrameStyle(int(QFrame::Sunken) | int(QFrame::Panel));
	m_labelCollisionStatus->setLineWidth(1);

	m_statusbar = new QStatusBar(this);
	m_statusbar->addPermanentWidget(m_labelCollisionStatus);
	m_statusbar->addPermanentWidget(m_labelStatus);
	setStatusBar(m_statusbar);
	// --------------------------------------------------------------------


	// --------------------------------------------------------------------
	// restory window size, position, and state
	// --------------------------------------------------------------------
	if(m_sett.contains("geo"))
		restoreGeometry(m_sett.value("geo").toByteArray());
	else
		resize(1200, 800);

	if(m_sett.contains("state"))
		restoreState(m_sett.value("state").toByteArray());

	// recent files
	if(m_sett.contains("recent_files"))
		SetRecentFiles(m_sett.value("recent_files").toStringList());
	// --------------------------------------------------------------------


	// --------------------------------------------------------------------
	// initialisations
	// --------------------------------------------------------------------
	m_pathsbuilder.SetMaxNumThreads(g_maxnum_threads);
	m_pathsbuilder.SetEpsilon(g_eps);
	m_pathsbuilder.SetAngularEpsilon(g_eps_angular);
	m_pathsbuilder.SetInstrumentSpace(&this->m_instrspace);
	m_pathsbuilder.SetScatteringSenses(this->m_sensesCCW);
	m_pathsbuilder.AddProgressSlot([](bool start, bool end, t_real progress, const std::string& message)
	{
		//std::cout << "Progress: " << int(progress*100.) << " \%." << std::endl;
		return true;
	});

	UpdateUB();
	// --------------------------------------------------------------------
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
/**
 * main
 */
int main(int argc, char** argv)
{
	try
	{
		// qt log handler
		QLoggingCategory::setFilterRules("*=true\n*.debug=false\n*.info=false\n");
		qInstallMessageHandler([](QtMsgType ty, const QMessageLogContext& ctx, const QString& log) -> void
		{
			auto get_msg_type = [](const QtMsgType& _ty) -> std::string
			{
				switch(_ty)
				{
					case QtDebugMsg: return "debug";
					case QtWarningMsg: return "warning";
					case QtCriticalMsg: return "critical error";
					case QtFatalMsg: return "fatal error";
					case QtInfoMsg: return "info";
					default: return "<n/a>";
				}
			};

			auto get_str = [](const char* pc) -> std::string
			{
				if(!pc) return "<n/a>";
				return std::string{"\""} + std::string{pc} + std::string{"\""};
			};

			std::cerr << "Qt " << get_msg_type(ty);
			if(ctx.function)
			{
				std::cerr << " in "
					<< "file " << get_str(ctx.file) << ", "
					<< "function " << get_str(ctx.function) << ", "
					<< "line " << ctx.line;
			}
			std::cerr << ": " << log.toStdString() << std::endl;
		});

		// default gl surface format
		tl2::set_gl_format(true, _GL_MAJ_VER, _GL_MIN_VER, 8);
		tl2::set_locales();

		// set maximum number of threads
		g_maxnum_threads = std::max<unsigned int>(1, std::thread::hardware_concurrency()/2);

		//QApplication::setAttribute(Qt::AA_NativeWindows, true);
		QApplication::setAttribute(Qt::AA_DontUseNativeMenuBar, true);
		QApplication::addLibraryPath(QDir::currentPath() + QDir::separator() + "Qt_Plugins");

		auto app = std::make_unique<QApplication>(argc, argv);
		g_apppath = app->applicationDirPath().toStdString();
		app->addLibraryPath(app->applicationDirPath() + QDir::separator() + ".." +
			QDir::separator() + "Libraries" + QDir::separator() + "Qt_Plugins");
		std::cout << "Application binary path: " << g_apppath << "." << std::endl;

		auto mainwnd = std::make_unique<PathsTool>(nullptr);
		if(argc > 1)
			mainwnd->SetInitialInstrumentFile(argv[1]);
		mainwnd->show();
		mainwnd->raise();
		mainwnd->activateWindow();

		return app->exec();
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return -1;
	}

	return 0;
}
// ----------------------------------------------------------------------------

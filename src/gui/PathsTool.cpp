/**
 * TAS path tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "PathsTool.h"

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
#define PROG_TITLE "Triple-Axis Path Calculator"


/**
 * event signalling that the crystal UB matrix needs an update
 */
void PathsTool::UpdateUB()
{
	m_tascalc.UpdateUB();

	if(m_xtalInfos)
		m_xtalInfos->GetWidget()->SetUB(m_tascalc.GetB(), m_tascalc.GetUB());
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
 * File -> Export Path
 */
bool PathsTool::ExportPath(PathsExporterFormat fmt)
{
	std::shared_ptr<PathsExporterBase> exporter;

	QString dirLast = m_sett.value("cur_dir", "~/").toString();
	QString filename = QFileDialog::getSaveFileName(
		this, "Export Path", dirLast, "Text Files (*.txt)");
	if(filename == "")
		return false;

	switch(fmt)
	{
		case PathsExporterFormat::RAW:
			exporter = std::make_shared<PathsExporterRaw>(filename.toStdString());
			break;
		case PathsExporterFormat::NOMAD:
			exporter = std::make_shared<PathsExporterNomad>(filename.toStdString());
			break;
		case PathsExporterFormat::NICOS:
			exporter = std::make_shared<PathsExporterNicos>(filename.toStdString());
			break;
	}

	if(!exporter)
	{
		QMessageBox::critical(this, "Error", "No path is available.");
		return false;
	}

	if(!m_pathsbuilder.AcceptExporter(exporter.get(), m_pathvertices, true))
	{
		QMessageBox::critical(this, "Error", "path could not be exported.");
		return false;
	}

	m_sett.setValue("cur_dir", QFileInfo(filename).path());
	return true;
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

			SetTmpStatus(ostr.str());
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

				auto [Qrlu, E] = m_tascalc.GetQE(monoXtalAngle, anaXtalAngle,
					sampleXtalAngle, sampleScAngle);

				SetInstrumentStatus(Qrlu, E,
					m_instrspace.CheckAngularLimits(),
					m_instrspace.CheckCollision2D());

				if(this->m_dlgConfigSpace)
					this->m_dlgConfigSpace->UpdateInstrument(
						instr, m_tascalc.GetScatteringSenses());

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
void PathsTool::GotoCoordinates(
	t_real h, t_real k, t_real l,
	t_real ki, t_real kf,
	bool only_set_target)
{
	TasAngles angles = m_tascalc.GetAngles(h, k, l, ki, kf);
	
	if(!angles.mono_ok)
	{
		QMessageBox::critical(this, "Error", "Invalid monochromator angle.");
		return;
	}

	if(!angles.ana_ok)
	{
		QMessageBox::critical(this, "Error", "Invalid analyser angle.");
		return;
	}

	if(!angles.sample_ok)
	{
		QMessageBox::critical(this, "Error", "Invalid scattering angles.");
		return;
	}

	// set target coordinate angles
	if(only_set_target)
	{
		if(!m_pathProperties)
			return;

		auto pathwidget = m_pathProperties->GetWidget();
		if(!pathwidget)
			return;

		const t_real *sensesCCW = m_tascalc.GetScatteringSenses();
		t_real a2_abs = angles.monoXtalAngle * 2. * sensesCCW[0];
		t_real a4_abs = angles.sampleScatteringAngle * sensesCCW[1];

		pathwidget->SetTarget(
			a2_abs / tl2::pi<t_real> * 180.,
			a4_abs / tl2::pi<t_real> * 180.);
	}

	// set instrument angles
	else
	{
		// set scattering angles
		m_instrspace.GetInstrument().GetMonochromator().SetAxisAngleOut(
			t_real{2} * angles.monoXtalAngle);
		m_instrspace.GetInstrument().GetSample().SetAxisAngleOut(
			angles.sampleScatteringAngle);
		m_instrspace.GetInstrument().GetAnalyser().SetAxisAngleOut(
			t_real{2} * angles.anaXtalAngle);

		// set crystal angles
		m_instrspace.GetInstrument().GetMonochromator().SetAxisAngleInternal(
			angles.monoXtalAngle);
		m_instrspace.GetInstrument().GetSample().SetAxisAngleInternal(
			angles.sampleXtalAngle);
		m_instrspace.GetInstrument().GetAnalyser().SetAxisAngleInternal(
			angles.anaXtalAngle);

		m_tascalc.SetKfix(kf, true);
	}
}


/**
 * set the instrument angles to the specified ones
 * (angles have to be positive as scattering senses are applied in the function)
 */
void PathsTool::GotoAngles(std::optional<t_real> a1,
	std::optional<t_real> a3, std::optional<t_real> a4,
	std::optional<t_real> a5, bool only_set_target)
{
	// set target coordinate angles
	if(only_set_target && a1 && a4)
	{
		if(!m_pathProperties)
			return;
		auto pathwidget = m_pathProperties->GetWidget();
		if(!pathwidget)
			return;

		t_real _a2 = *a1 * 2.;
		t_real _a4 = *a4;

		pathwidget->SetTarget(
			_a2 / tl2::pi<t_real> * 180.,
			_a4 / tl2::pi<t_real> * 180.);
	}

	// set instrument angles
	else
	{
		const t_real *sensesCCW = m_tascalc.GetScatteringSenses();

		// set mono angle
		if(a1)
		{
			*a1 *= sensesCCW[0];
			m_instrspace.GetInstrument().GetMonochromator(). SetAxisAngleOut(t_real{2} * *a1);
			m_instrspace.GetInstrument().GetMonochromator().SetAxisAngleInternal(*a1);
		}

		// set sample crystal angle
		if(a3)
		{
			*a3 *= sensesCCW[1];
			m_instrspace.GetInstrument().GetSample().SetAxisAngleInternal(*a3);
		}

		// set sample scattering angle
		if(a4)
		{
			*a4 *= sensesCCW[1];
			m_instrspace.GetInstrument().GetSample().SetAxisAngleOut(*a4);
		}

		// set ana angle
		if(a5)
		{
			*a5 *= sensesCCW[2];
			m_instrspace.GetInstrument().GetAnalyser().SetAxisAngleOut(t_real{2} * *a5);
			m_instrspace.GetInstrument().GetAnalyser().SetAxisAngleInternal(*a5);
		}
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
	if(!m_renderer)
		return;

	// show context menu for object
	if(right && obj != "")
	{
		m_curContextObj = obj;

		QPoint pos = m_renderer->GetMousePosition(true);
		m_contextMenuObj->popup(pos);
	}

	// centre scene around object
	if(middle)
	{
		m_renderer->CentreCam(obj);
	}
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


/**
 * set temporary status message
 */
void PathsTool::SetTmpStatus(const std::string& msg)
{
	if(!m_statusbar)
		return;

	// show message for 2 seconds
	m_statusbar->showMessage(msg.c_str(), 2000);
}


/**
 * update permanent status message
 */
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


/**
 * set permanent instrumetn status message
 */
void PathsTool::SetInstrumentStatus(const std::optional<t_vec>& Qopt, t_real E,
	bool in_angular_limits, bool colliding)
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

	if(!in_angular_limits)
		ostr << "invalid angles, ";

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
	m_coordProperties = std::make_shared<CoordPropertiesDockWidget>(this);
	m_pathProperties = std::make_shared<PathPropertiesDockWidget>(this);
	m_camProperties = std::make_shared<CamPropertiesDockWidget>(this);

	addDockWidget(Qt::LeftDockWidgetArea, m_tasProperties.get());
	addDockWidget(Qt::LeftDockWidgetArea, m_xtalProperties.get());
	addDockWidget(Qt::RightDockWidgetArea, m_xtalInfos.get());
	addDockWidget(Qt::RightDockWidgetArea, m_coordProperties.get());
	addDockWidget(Qt::RightDockWidgetArea, m_pathProperties.get());
	addDockWidget(Qt::RightDockWidgetArea, m_camProperties.get());

	auto* taswidget = m_tasProperties->GetWidget().get();
	auto* xtalwidget = m_xtalProperties->GetWidget().get();
	auto* coordwidget = m_coordProperties->GetWidget().get();
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
			m_tascalc.SetMonochromatorD(dmono);
			m_tascalc.SetAnalyserD(dana);
		});

	// scattering senses
	connect(taswidget, &TASPropertiesWidget::ScatteringSensesChanged,
		[this](bool monoccw, bool sampleccw, bool anaccw) -> void
		{
			m_tascalc.SetScatteringSenses(monoccw, sampleccw, anaccw);
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
			m_tascalc.SetSampleLatticeConstants(a, b, c);
			m_tascalc.SetSampleLatticeAngles(alpha, beta, gamma, true);

			m_tascalc.UpdateB();
			UpdateUB();
		});

	connect(xtalwidget, &XtalPropertiesWidget::PlaneChanged,
		[this](t_real vec1_x, t_real vec1_y, t_real vec1_z, t_real vec2_x, t_real vec2_y, t_real vec2_z) -> void
		{
			m_tascalc.SetSampleScatteringPlane(
				vec1_x, vec1_y, vec1_z,
				vec2_x, vec2_y, vec2_z);

			UpdateUB();
		});

	// goto coordinates
	connect(coordwidget, &CoordPropertiesWidget::GotoCoordinates,
		this, &PathsTool::GotoCoordinates);

	// goto angles
	connect(pathwidget, &PathPropertiesWidget::GotoAngles,
		[this](t_real a2, t_real a4)
		{
			a2 = a2 / 180. * tl2::pi<t_real>;
			a4 = a4 / 180. * tl2::pi<t_real>;

			this->GotoAngles(a2/2., std::nullopt, a4, std::nullopt, false);
		});

	// target angles have changed
	connect(pathwidget, &PathPropertiesWidget::TargetChanged,
		[this](t_real a2, t_real a4)
		{
			//std::cout << "target angles: " << a2 << ", " << a4 << std::endl;
			const t_real *sensesCCW = m_tascalc.GetScatteringSenses();

			a2 = a2 / 180. * tl2::pi<t_real> * sensesCCW[0];
			a4 = a4 / 180. * tl2::pi<t_real> * sensesCCW[1];

			m_targetMonoScatteringAngle = a2;
			m_targetSampleScatteringAngle = a4;

			if(this->m_dlgConfigSpace)
				this->m_dlgConfigSpace->UpdateTarget(a2, a4, sensesCCW);
		});


	// calculate path mesh
	connect(pathwidget, &PathPropertiesWidget::CalculatePathMesh,
		this, &PathsTool::CalculatePathMesh);

	// calculate path
	connect(pathwidget, &PathPropertiesWidget::CalculatePath,
		this, &PathsTool::CalculatePath);

	// a new path has been calculated
	connect(this, &PathsTool::PathAvailable,
		pathwidget, &PathPropertiesWidget::PathAvailable);

	// a new path vertex has been chosen on the path slider
	connect(pathwidget, &PathPropertiesWidget::TrackPath,
		this, &PathsTool::TrackPath);
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

	// export menu
	QMenu *menuExportPath = new QMenu("Export Path", m_menubar);

	QAction *acExportRaw = new QAction("To Raw...", menuExportPath);
	QAction *acExportNomad = new QAction("To Nomad...", menuExportPath);
	QAction *acExportNicos = new QAction("To Nicos...", menuExportPath);

	menuExportPath->addAction(acExportRaw);
	menuExportPath->addAction(acExportNomad);
	menuExportPath->addAction(acExportNicos);

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

	connect(acExportRaw, &QAction::triggered, [this]() -> void
	{
		ExportPath(PathsExporterFormat::RAW);
	});

	connect(acExportNomad, &QAction::triggered, [this]() -> void
	{
		ExportPath(PathsExporterFormat::NOMAD);
	});

	connect(acExportNicos, &QAction::triggered, [this]() -> void
	{
		ExportPath(PathsExporterFormat::NICOS);
	});


	menuFile->addAction(actionNew);
	menuFile->addSeparator();
	menuFile->addAction(actionOpen);
	menuFile->addMenu(m_menuOpenRecent);
	menuFile->addSeparator();
	menuFile->addAction(actionSave);
	menuFile->addAction(actionSaveAs);
	menuFile->addMenu(menuExportPath);
	menuFile->addSeparator();
	menuFile->addAction(actionSettings);
	menuFile->addSeparator();
	menuFile->addAction(actionQuit);


	// view menu
	QMenu *menuView = new QMenu("View", m_menubar);

	menuView->addAction(m_tasProperties->toggleViewAction());
	menuView->addAction(m_xtalProperties->toggleViewAction());
	menuView->addAction(m_xtalInfos->toggleViewAction());
	menuView->addAction(m_coordProperties->toggleViewAction());
	menuView->addAction(m_pathProperties->toggleViewAction());
	menuView->addAction(m_camProperties->toggleViewAction());
	//menuView->addSeparator();
	//menuView->addAction(acPersp);


	// geometry menu
	QMenu *menuGeo = new QMenu("Geometry", m_menubar);

	QAction *actionAddCuboidWall = new QAction("Add Wall", menuGeo);
	QAction *actionAddCylindricalWall = new QAction("Add Pillar", menuGeo);
	QAction *actionGeoBrowser = new QAction("Geometries Browser...", menuGeo);

	connect(actionAddCuboidWall, &QAction::triggered, this, &PathsTool::AddWall);
	connect(actionAddCylindricalWall, &QAction::triggered, this, &PathsTool::AddPillar);

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

			this->connect(this->m_dlgConfigSpace.get(), &ConfigSpaceDlg::GotoAngles,
				this, &PathsTool::GotoAngles);
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
	fs::path polypath = fs::path(g_apppath) / fs::path("poly");

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

	if(fs::exists(polypath))
	{
		QAction *acPolyTool = new QAction("Polygon...", menuTools);
		menuTools->addAction(acPolyTool);
		++num_tools;

		connect(acPolyTool, &QAction::triggered, this, [polypath]()
		{
			std::system((polypath.string() + "&").c_str());
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
	// context menu
	// --------------------------------------------------------------------
	m_contextMenuObj = new QMenu(this);
	m_contextMenuObj->addAction("Delete Object", this, &PathsTool::DeleteCurrentObject);
	// --------------------------------------------------------------------


	// --------------------------------------------------------------------
	// status bar
	// --------------------------------------------------------------------
	m_progress = new QProgressBar(this);
	m_progress->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_progress->setMinimum(0);
	m_progress->setMaximum(1000);

	m_labelStatus = new QLabel(this);
	m_labelStatus->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_labelStatus->setFrameStyle(int(QFrame::Sunken) | int(QFrame::Panel));
	m_labelStatus->setLineWidth(1);

	m_labelCollisionStatus = new QLabel(this);
	m_labelCollisionStatus->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_labelCollisionStatus->setFrameStyle(int(QFrame::Sunken) | int(QFrame::Panel));
	m_labelCollisionStatus->setLineWidth(1);

	m_statusbar = new QStatusBar(this);
	m_statusbar->addPermanentWidget(m_progress);
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
	// TODO: add to settings
	m_tascalc.SetSampleAngleOffset(g_a3_offs);

	m_pathsbuilder.SetMaxNumThreads(g_maxnum_threads);
	m_pathsbuilder.SetEpsilon(g_eps);
	m_pathsbuilder.SetAngularEpsilon(g_eps_angular);
	m_pathsbuilder.SetInstrumentSpace(&this->m_instrspace);
	m_pathsbuilder.SetTasCalculator(&m_tascalc);
	m_pathsbuilder.AddProgressSlot(
		[this](bool start, bool end, t_real progress, const std::string& message)
		{
			//std::cout << "Progress: " << int(progress*100.) << " \%." << std::endl;
			if(!m_progress)
				return true;

			m_progress->setValue((int)(progress * m_progress->maximum()));
			//m_progress->setFormat((std::string("%p% -- ") + message).c_str());
			return true;
		});

	UpdateUB();
	// --------------------------------------------------------------------
}


/**
 * add a wall to the instrument space
 */
void PathsTool::AddWall()
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

	// add wall to instrument space
	m_instrspace.AddWall(std::vector<std::shared_ptr<Geometry>>{{wall}}, ostrId.str());

	// update object browser tree
	if(m_dlgGeoBrowser)
		m_dlgGeoBrowser->UpdateGeoTree(m_instrspace);

	// add a 3d representation of the wall
	if(m_renderer)
		m_renderer->AddWall(*wall, true);
}


/**
 * add a pillar to the instrument space
 */
void PathsTool::AddPillar()
{
	auto wall = std::make_shared<CylinderGeometry>();
	wall->SetHeight(4.);
	wall->SetCentre(tl2::create<t_vec>({0, 0, wall->GetHeight()*0.5}));
	wall->SetRadius(0.5);
	wall->UpdateTrafo();

	static std::size_t wallcnt = 1;
	std::ostringstream ostrId;
	ostrId << "new pillar " << wallcnt++;

	// add pillar to instrument space
	m_instrspace.AddWall(std::vector<std::shared_ptr<Geometry>>{{wall}}, ostrId.str());

	// update object browser tree
	if(m_dlgGeoBrowser)
		m_dlgGeoBrowser->UpdateGeoTree(m_instrspace);

	// add a 3d representation of the pillar
	if(m_renderer)
		m_renderer->AddWall(*wall, true);
}


/**
 * delete 3d object under the cursor
 */
void PathsTool::DeleteCurrentObject()
{
	if(m_curContextObj == "")
		return;

	// remove object from instrument space
	if(m_instrspace.DeleteObject(m_curContextObj))
	{
		// update object browser tree
		if(m_dlgGeoBrowser)
			m_dlgGeoBrowser->UpdateGeoTree(m_instrspace);

		// remove 3d representation of object
		if(m_renderer)
			m_renderer->DeleteObject(m_curContextObj);
	}
	else
	{
		QMessageBox::warning(this, "Warning",
			QString("Object \"") + m_curContextObj.c_str() + QString("\" cannot be deleted."));
	}
}


/**
 * calculate the mesh of possible paths
 */
void PathsTool::CalculatePathMesh()
{
}


/**
 * calculate the path from the current to the target position
 */
void PathsTool::CalculatePath()
{
	m_pathvertices.clear();

	// get the scattering angles
	const Instrument& instr = m_instrspace.GetInstrument();
	t_real curMonoScatteringAngle = instr.GetMonochromator().GetAxisAngleOut();
	t_real curSampleScatteringAngle = instr.GetSample().GetAxisAngleOut();

	// adjust scattering senses
	const t_real* sensesCCW = m_tascalc.GetScatteringSenses();

	curMonoScatteringAngle *= sensesCCW[0];
	curSampleScatteringAngle *= sensesCCW[1];
	t_real targetMonoScatteringAngle = m_targetMonoScatteringAngle * sensesCCW[0];
	t_real targetSampleScatteringAngle = m_targetSampleScatteringAngle * sensesCCW[1];

	// find path from current to target position
	InstrumentPath path = m_pathsbuilder.FindPath(
		curMonoScatteringAngle, curSampleScatteringAngle,
		targetMonoScatteringAngle, targetSampleScatteringAngle);

	if(!path.ok)
	{
		QMessageBox::critical(this, "Error", "No path could be found.");
		return;
	}

	// get the vertices on the path
	m_pathvertices = m_pathsbuilder.GetPathVertices(path, true, false);
	emit PathAvailable(m_pathvertices.size());
}


/**
 * move the instrument to a position on the path
 */
void PathsTool::TrackPath(std::size_t idx)
{
	if(idx >= m_pathvertices.size())
		return;

	const t_vec& vert = m_pathvertices[idx];
	GotoAngles(vert[1]*0.5, std::nullopt, vert[0], std::nullopt, false);
}

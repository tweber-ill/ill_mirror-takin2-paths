/**
 * TAS path tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "PathsTool.h"
#include "Settings.h"

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
#include "src/libs/ptree.h"


#define MAX_RECENT_FILES 16
#define PROG_TITLE "TAS Path Tool"
#define PROG_IDENT "takin_paths"
#define FILE_BASENAME "paths."



// ----------------------------------------------------------------------------
void PathsTool::UpdateUB()
{
	m_UB = tl2::UB_matrix<t_mat, t_vec>(m_B, m_plane_rlu[0], m_plane_rlu[1], m_plane_rlu[2]);

	using namespace tl2_ops;
	std::cout << "B matrix: " << m_B << std::endl;
	std::cout << "UB matrix: " << m_UB << std::endl;
}


void PathsTool::showEvent(QShowEvent *evt)
{
	m_renderer->EnableTimer(true);
	QMainWindow::showEvent(evt);
}


void PathsTool::hideEvent(QHideEvent *evt)
{
	m_renderer->EnableTimer(false);
	QMainWindow::hideEvent(evt);
}


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
	m_renderer->LoadInstrument(m_instrspace);
}


/**
 * File -> Open
 */
void PathsTool::OpenFile()
{
	QString dirLast = m_sett.value("cur_dir", "").toString();

	QString filename = QFileDialog::getOpenFileName(this, "Open File", dirLast, "Paths Files (*.paths)");
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
	QString dirLast = m_sett.value("cur_dir", "").toString();

	QString filename = QFileDialog::getSaveFileName(this, "Save File", dirLast, "Paths Files (*.paths)");
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
		if(file=="" || !QFile::exists(file))
			return false;


		// load xml
		pt::ptree prop;
		std::ifstream ifstr{file.toStdString()};

		if(!ifstr)
		{
			QMessageBox::critical(this, "Error", "Could not load file.");
			return false;
		}

		// check format and version
		pt::read_xml(ifstr, prop);
		if(auto opt = prop.get_optional<std::string>(FILE_BASENAME "ident");
			!opt || *opt != PROG_IDENT)
		{
			QMessageBox::critical(this, "Error", "Not a recognised file format. Ignoring.");
			return false;
		}

		if(auto optTime = prop.get_optional<t_real>(FILE_BASENAME "timestamp"); optTime)
			std::cout << "Loading file \"" << file.toStdString()
				<< "\" dated " << tl2::epoch_to_str(*optTime) << "." << std::endl;


		// get variables from config file
		std::unordered_map<std::string, std::string> propvars;

		if(auto vars = prop.get_child_optional(FILE_BASENAME "variables"); vars)
		{
			// iterate variables
			for(const auto &var : *vars)
			{
				const auto& key = var.first;
				std::string val = var.second.get<std::string>("<xmlattr>.value", "");
				//std::cout << key << " = " << val << std::endl;

				propvars.insert(std::make_pair(key, val));
			}

			replace_ptree_values(prop, propvars);
		}


		// load instrument definition
		if(auto instr = prop.get_child_optional(FILE_BASENAME "instrument_space"); instr)
		{
			if(!m_instrspace.Load(*instr))
			{
				QMessageBox::critical(this, "Error", "Instrument configuration could not be loaded.");
				return false;
			}
		}
		else
		{
			QMessageBox::critical(this, "Error", "No instrument definition found.");
			return false;
		}


		SetCurrentFile(file);
		AddRecentFile(file);

		m_renderer->LoadInstrument(m_instrspace);
		m_instrspace.GetInstrument().AddUpdateSlot(
			[this](const Instrument& instr)->void
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

				if(m_renderer)
					m_renderer->UpdateInstrument(instr);
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

	pt::ptree prop;

	// set format and version
	prop.put(FILE_BASENAME "ident", PROG_IDENT);
	prop.put(FILE_BASENAME "timestamp", tl2::var_to_str(tl2::epoch<t_real>()));

	std::ofstream ofstr{file.toStdString()};
	if(!ofstr)
	{
		QMessageBox::critical(this, "Error", "Could not save file.");
		return false;
	}

	ofstr.precision(6);
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
	// TODO
	t_real a3_offs = tl2::pi<t_real>*0.5;

	t_real a1 = m_sensesCCW[0] * tl2::calc_tas_a1<t_real>(ki, m_dspacings[0]);
	t_real a5 = m_sensesCCW[2] * tl2::calc_tas_a1<t_real>(kf, m_dspacings[1]);

	t_vec Q = tl2::create<t_vec>({h, k, l});
	auto [a3, a4, dist] = calc_tas_a3a4<t_mat, t_vec, t_real>(
		m_B, ki, kf, Q,
		m_plane_rlu[0], m_plane_rlu[2], m_sensesCCW[1], a3_offs);

	// set scattering angles
	m_instrspace.GetInstrument().GetMonochromator().
		SetAxisAngleOut(t_real{2}*a1);
	m_instrspace.GetInstrument().GetSample().
		SetAxisAngleOut(a4);
	m_instrspace.GetInstrument().GetAnalyser().
		SetAxisAngleOut(t_real{2}*a5);

	// set crystal angles
	m_instrspace.GetInstrument().GetMonochromator().
		SetAxisAngleInternal(a1);
	m_instrspace.GetInstrument().GetSample().
		SetAxisAngleInternal(a3);
	m_instrspace.GetInstrument().GetAnalyser().
		SetAxisAngleInternal(a5);
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
		OpenFile(instrfile.c_str());
	m_renderer->LoadInstrument(m_instrspace);
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
	std::cout << "Clicked on " << obj << "." << std::endl;
	m_renderer->CentreCam(obj);
}


/**
 * dragging an object
 */
void PathsTool::ObjectDragged(const std::string& obj, t_real_gl x, t_real_gl y)
{
	std::cout << "Dragging " << obj << " to (" << x << ", " << y << ")." << std::endl;
}


void PathsTool::UpdateStatusLabel()
{
	std::ostringstream ostr;
	ostr.precision(4);
	ostr << "x = " << m_mouseX << " m, y = " << m_mouseY << " m";
	if(m_curObj != "")
		ostr << ", object: " << m_curObj;
	m_labelStatus->setText(ostr.str().c_str());
}


/**
 * create UI
 */
PathsTool::PathsTool(QWidget* pParent) : QMainWindow{pParent}
{
	setWindowTitle(PROG_TITLE);


	// --------------------------------------------------------------------
	// rendering widget
	// --------------------------------------------------------------------
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
	m_pathProperties = std::make_shared<PathPropertiesDockWidget>(this);
	m_camProperties = std::make_shared<CamPropertiesDockWidget>(this);

	addDockWidget(Qt::LeftDockWidgetArea, m_tasProperties.get());
	addDockWidget(Qt::LeftDockWidgetArea, m_xtalProperties.get());
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

	QAction *acNew = new QAction(QIcon::fromTheme("document-new"), "New", menuFile);
	QAction *acOpen = new QAction(QIcon::fromTheme("document-open"), "Open...", menuFile);
	QAction *acSave = new QAction(QIcon::fromTheme("document-save"), "Save", menuFile);
	QAction *acSaveAs = new QAction(QIcon::fromTheme("document-save-as"), "Save As...", menuFile);
	QAction *acQuit = new QAction(QIcon::fromTheme("application-exit"), "Quit", menuFile);

	// shortcuts
	acNew->setShortcut(QKeySequence::New);
	acOpen->setShortcut(QKeySequence::Open);
	acSave->setShortcut(QKeySequence::Save);
	acSaveAs->setShortcut(QKeySequence::SaveAs);
	acQuit->setShortcut(QKeySequence::Quit);

	m_menuOpenRecent = new QMenu("Open Recent", menuFile);
	m_menuOpenRecent->setIcon(QIcon::fromTheme("document-open-recent"));

	acQuit->setMenuRole(QAction::QuitRole);


	connect(acNew, &QAction::triggered, this, [this]() { this->NewFile(); });
	connect(acOpen, &QAction::triggered, this, [this]() { this->OpenFile(); });
	connect(acSave, &QAction::triggered, this, [this]() { this->SaveFile(); });
	connect(acSaveAs, &QAction::triggered, this, [this]() { this->SaveFileAs(); });
	connect(acQuit, &QAction::triggered, this, &PathsTool::close);

	menuFile->addAction(acNew);
	menuFile->addSeparator();
	menuFile->addAction(acOpen);
	menuFile->addMenu(m_menuOpenRecent);
	menuFile->addSeparator();
	menuFile->addAction(acSave);
	menuFile->addAction(acSaveAs);
	menuFile->addSeparator();
	menuFile->addAction(acQuit);


	// view menu
	QMenu *menuView = new QMenu("View", m_menubar);

	/*QAction *acPersp = new QAction("Perspective Projection", menuView);
	acPersp->setCheckable(true);
	acPersp->setChecked(true);

	connect(acPersp, &QAction::toggled, this, [this](bool b)
	{
		if(m_renderer)
			m_renderer->SetPerspectiveProjection(b);
	});*/

	menuView->addAction(m_tasProperties->toggleViewAction());
	menuView->addAction(m_xtalProperties->toggleViewAction());
	menuView->addAction(m_pathProperties->toggleViewAction());
	menuView->addAction(m_camProperties->toggleViewAction());
	//menuView->addSeparator();
	//menuView->addAction(acPersp);


	// help menu
	QMenu *menuHelp = new QMenu("Help", m_menubar);

	QAction *actionAboutQt = new QAction(QIcon::fromTheme("help-about"), "About Qt Libraries...", menuHelp);
	QAction *actionAboutGl = new QAction(QIcon::fromTheme("help-about"), "About Renderer...", menuHelp);
	QAction *actionAbout = new QAction(QIcon::fromTheme("help-about"), "About Program...", menuHelp);

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
		if(!this->m_dspacingslgAbout)
			this->m_dspacingslgAbout = std::make_shared<AboutDlg>(this);

		m_dspacingslgAbout->show();
		m_dspacingslgAbout->activateWindow();
	});

	menuHelp->addAction(actionAboutQt);
	menuHelp->addAction(actionAboutGl);
	menuHelp->addSeparator();
	menuHelp->addAction(actionAbout);


	// menu bar
	m_menubar->addMenu(menuFile);
	m_menubar->addMenu(menuView);
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

	m_statusbar = new QStatusBar(this);
	m_statusbar->addPermanentWidget(m_labelStatus);
	setStatusBar(m_statusbar);
	// --------------------------------------------------------------------


	// --------------------------------------------------------------------
	// restory window size, position, and state
	// --------------------------------------------------------------------
	if(m_sett.contains("geo"))
		restoreGeometry(m_sett.value("geo").toByteArray());
	else
		resize(800, 600);

	if(m_sett.contains("state"))
		restoreState(m_sett.value("state").toByteArray());

	// recent files
	if(m_sett.contains("recent_files"))
		SetRecentFiles(m_sett.value("recent_files").toStringList());
	// --------------------------------------------------------------------


	// --------------------------------------------------------------------
	// initialisations
	// --------------------------------------------------------------------
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

	tl2::set_gl_format(1, _GL_MAJ_VER, _GL_MIN_VER, 8);
	tl2::set_locales();

	//QApplication::setAttribute(Qt::AA_NativeWindows, true);
	QApplication::setAttribute(Qt::AA_DontUseNativeMenuBar, true);
	QApplication::addLibraryPath(QDir::currentPath() + QDir::separator() + "qtplugins");
	auto app = std::make_unique<QApplication>(argc, argv);
	app->addLibraryPath(app->applicationDirPath() + QDir::separator() + "qtplugins");
	g_apppath = app->applicationDirPath().toStdString();
	std::cout << "Application binary path: " << g_apppath << "." << std::endl;

	auto dlg = std::make_unique<PathsTool>(nullptr);
	if(argc > 1)
		dlg->SetInitialInstrumentFile(argv[1]);
	dlg->show();
	return app->exec();
}
// ----------------------------------------------------------------------------
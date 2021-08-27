/**
 * angular configuration space dialog
 * @author Tobias Weber <tweber@ill.fr>
 * @date may-2021
 * @license GPLv3, see 'LICENSE' file
 *
 * References:
 *   - https://www.qcustomplot.com/documentation/classQCustomPlot.html
 *   - https://www.qcustomplot.com/documentation/classQCPColorMap.html
 *   - https://www.qcustomplot.com/documentation/classQCPGraph.html
 *   - https://www.qcustomplot.com/documentation/classQCPCurve.html
 */

#include "ConfigSpace.h"

#include <QtCore/QMetaObject>
#include <QtCore/QThread>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMenu>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QFileDialog>

#include "Settings.h"


ConfigSpaceDlg::ConfigSpaceDlg(QWidget* parent, QSettings *sett)
	: QDialog{parent}, m_sett{sett}
{
	setWindowTitle("Angular Configuration Space");

	// restore dialog geometry
	if(m_sett && m_sett->contains("configspace/geo"))
		restoreGeometry(m_sett->value("configspace/geo").toByteArray());
	else
		resize(800, 600);

	// plotter
	m_plot = std::make_shared<QCustomPlot>(this);
	m_plot->xAxis->setLabel("2θ_S (deg)");
	m_plot->yAxis->setLabel("2θ_M (deg)");
	m_plot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_plot->setInteraction(QCP::iSelectPlottablesBeyondAxisRect, false);

	// wall contours
	m_colourMap = new QCPColorMap(m_plot->xAxis, m_plot->yAxis);
	m_colourMap->setGradient(QCPColorGradient::gpJet);
	m_colourMap->setDataRange(QCPRange{0, 1});
	m_colourMap->setDataScaleType(QCPAxis::stLinear);
	m_colourMap->setInterpolate(false);
	m_colourMap->setAntialiased(false);

	// instrument position plot
	{
		m_instrposplot = m_plot->addGraph();
		m_instrposplot->setLineStyle(QCPGraph::lsNone);
		m_instrposplot->setAntialiased(true);

		QPen instrpen = m_instrposplot->pen();
		instrpen.setColor(QColor::fromRgbF(1., 0., 0.));
		m_instrposplot->setPen(instrpen);

		QBrush instrbrush = m_instrposplot->brush();
		instrbrush.setColor(QColor::fromRgbF(1., 0., 0.));
		instrbrush.setStyle(Qt::SolidPattern);
		m_instrposplot->setBrush(instrbrush);

		QCPScatterStyle scatterstyle(QCPScatterStyle::ssCircle, 8);
		scatterstyle.setPen(instrpen);
		scatterstyle.setBrush(instrbrush);
		m_instrposplot->setScatterStyle(scatterstyle);
	}

	// target position plot
	{
		m_targetposplot = m_plot->addGraph();
		m_targetposplot->setLineStyle(QCPGraph::lsNone);
		m_targetposplot->setAntialiased(true);

		QPen instrpen = m_targetposplot->pen();
		instrpen.setColor(QColor::fromRgbF(0., 1., 0.));
		m_targetposplot->setPen(instrpen);

		QBrush instrbrush = m_targetposplot->brush();
		instrbrush.setColor(QColor::fromRgbF(0., 1., 0.));
		instrbrush.setStyle(Qt::SolidPattern);
		m_targetposplot->setBrush(instrbrush);

		QCPScatterStyle scatterstyle(QCPScatterStyle::ssCircle, 8);
		scatterstyle.setPen(instrpen);
		scatterstyle.setBrush(instrbrush);
		m_targetposplot->setScatterStyle(scatterstyle);
	}

	UpdatePlotRanges();

	// status label
	m_status = new QLabel(this);
	m_status->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_status->setFrameStyle(QFrame::Sunken);
	m_status->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

	// spin boxes
	m_spinDelta2ThS = new QDoubleSpinBox(this);
	m_spinDelta2ThM = new QDoubleSpinBox(this);

	m_spinDelta2ThS->setPrefix("Δθ_S = ");
	m_spinDelta2ThS->setSuffix(" deg");
	m_spinDelta2ThS->setValue(g_a4_delta / tl2::pi<t_real>*180.);
	m_spinDelta2ThS->setMinimum(0.001);
	m_spinDelta2ThS->setMaximum(180.);
	m_spinDelta2ThS->setSingleStep(0.1);

	m_spinDelta2ThM->setPrefix("Δθ_M = ");
	m_spinDelta2ThM->setSuffix(" deg");
	m_spinDelta2ThM->setValue(g_a2_delta / tl2::pi<t_real>*180.);
	m_spinDelta2ThM->setMinimum(0.001);
	m_spinDelta2ThM->setMaximum(180.);
	m_spinDelta2ThM->setSingleStep(0.1);

	// buttons
	QPushButton *btnCalc = new QPushButton("Calculate", this);
	QPushButton *btnSave = new QPushButton("Save Figure...", this);
	QPushButton *btnClose = new QPushButton("OK", this);

	// grid
	auto grid = new QGridLayout(this);
	grid->setSpacing(4);
	grid->setContentsMargins(12, 12, 12, 12);
	int y = 0;
	grid->addWidget(m_plot.get(), y++, 0, 1, 5);
	grid->addWidget(m_spinDelta2ThS, y, 0, 1, 1);
	grid->addWidget(m_spinDelta2ThM, y, 1, 1, 1);
	grid->addWidget(btnCalc, y, 2, 1, 1);
	grid->addWidget(btnSave, y, 3, 1, 1);
	grid->addWidget(btnClose, y++, 4, 1, 1);
	grid->addWidget(m_status, y++, 0, 1, 5);


	// ------------------------------------------------------------------------
	// menu
	// ------------------------------------------------------------------------
	QMenu *menuFile = new QMenu("File", this);
	QMenu *menuOptions = new QMenu("Options", this);
	QMenu *menuCalc = new QMenu("Calculate", this);
	QMenu *menuView = new QMenu("View", this);

	QAction *acSavePDF = new QAction("Save Figure...", menuFile);
	menuFile->addAction(acSavePDF);

	QAction *acSaveLines = new QAction("Save Contour Lines...", menuFile);
	menuFile->addAction(acSaveLines);

	QAction *acSaveGraph = new QAction("Save Voronoi Graph...", menuFile);
	menuFile->addAction(acSaveGraph);

	QMenu *menuExportPath = new QMenu("Export Path", this);

	QAction *acExportRaw = new QAction("To Raw...", menuFile);
	menuExportPath->addAction(acExportRaw);

	QAction *acExportNomad = new QAction("To Nomad...", menuFile);
	menuExportPath->addAction(acExportNomad);

	QAction *acExportNicos = new QAction("To Nicos...", menuFile);
	menuExportPath->addAction(acExportNicos);

	menuFile->addMenu(menuExportPath);
	menuFile->addSeparator();

	QAction *acQuit = new QAction(QIcon::fromTheme("application-exit"), "Quit", menuFile);
	acQuit->setShortcut(QKeySequence::Quit);
	acQuit->setMenuRole(QAction::QuitRole);
	menuFile->addAction(acQuit);

	QAction *acSimplifyContour = new QAction("Simplify Contour", menuView);
	acSimplifyContour->setCheckable(true);
	acSimplifyContour->setChecked(m_simplifycontour);
	menuOptions->addAction(acSimplifyContour);

	QAction *acGroupLines = new QAction("Group Line Segments", menuView);
	acGroupLines->setCheckable(true);
	acGroupLines->setChecked(m_grouplines);
	menuOptions->addAction(acGroupLines);

	QAction *acSplitContour = new QAction("Split Contour into Convex Regions", menuView);
	acSplitContour->setCheckable(true);
	acSplitContour->setChecked(m_splitcontour);
	menuOptions->addAction(acSplitContour);

	QAction *acCalcVoro = new QAction("Calculate Voronoi Diagram", menuView);
	acCalcVoro->setCheckable(true);
	acCalcVoro->setChecked(m_calcvoronoi);
	menuOptions->addAction(acCalcVoro);

	QAction *acSubdivPath = new QAction("Subdivide Path", menuView);
	acSubdivPath->setCheckable(true);
	acSubdivPath->setChecked(m_subdivide_path);
	menuOptions->addAction(acSubdivPath);

	QAction *acAutocalcPath = new QAction("Automatically Calculate Path", menuView);
	acAutocalcPath->setCheckable(true);
	acAutocalcPath->setChecked(m_autocalcpath);
	menuOptions->addSeparator();
	menuOptions->addAction(acAutocalcPath);

	QAction *acMoveTarget = new QAction("Move Target Point", menuView);
	acMoveTarget->setCheckable(true);
	acMoveTarget->setChecked(m_movetarget);
	menuOptions->addAction(acMoveTarget);

	QAction *acCalcMesh = new QAction("Calculate Path Mesh", menuView);
	menuCalc->addAction(acCalcMesh);

	QAction *acCalcPath = new QAction("Calculate Path", menuView);
	menuCalc->addAction(acCalcPath);

	QAction *acEnableZoom = new QAction("Enable Zoom", menuView);
	acEnableZoom->setCheckable(true);
	acEnableZoom->setChecked(!m_moveInstr);
	menuView->addAction(acEnableZoom);

	QAction *acResetZoom = new QAction("Reset Zoom", menuView);
	menuView->addAction(acResetZoom);

	auto* menuBar = new QMenuBar(this);
	menuBar->addMenu(menuFile);
	menuBar->addMenu(menuOptions);
	menuBar->addMenu(menuCalc);
	menuBar->addMenu(menuView);
	grid->setMenuBar(menuBar);
	// ------------------------------------------------------------------------


	// ------------------------------------------------------------------------
	// output functions
	// ------------------------------------------------------------------------
	// export obstacle line segments
	auto saveLines = [this]()
	{
		if(!this->m_pathsbuilder)
			return;

		QString dirLast = this->m_sett->value("configspace/cur_dir", "~/").toString();

		QString filename = QFileDialog::getSaveFileName(
			this, "Save Line Segments", dirLast, "XML Files (*.xml)");
		if(filename=="")
			return;

		std::ofstream ofstr{filename.toStdString()};
		if(this->m_pathsbuilder->SaveToLinesTool(ofstr))
			this->m_sett->setValue("configspace/cur_dir", QFileInfo(filename).path());
	};


	// export figure as pdf file
	auto savePDF = [this]()
	{
		QString dirLast = this->m_sett->value("configspace/cur_dir", "~/").toString();
		QString filename = QFileDialog::getSaveFileName(
			this, "Save PDF Figure", dirLast, "PDF Files (*.pdf)");
		if(filename=="")
			return;

		if(this->m_plot->savePdf(filename))
			this->m_sett->setValue("configspace/cur_dir", QFileInfo(filename).path());
	};


	// export voronoi graph as dot file
	auto saveGraph = [this]()
	{
		if(!this->m_pathsbuilder)
			return;

		QString dirLast = this->m_sett->value("configspace/cur_dir", "~/").toString();
		QString filename = QFileDialog::getSaveFileName(
			this, "Save DOT Graph", dirLast, "DOT Files (*.dot)");
		if(filename=="")
			return;

		std::ofstream ofstr(filename.toStdString());
		bool ok = geo::print_graph(this->m_pathsbuilder->
			GetVoronoiResults().GetVoronoiGraph(), ofstr);
		ofstr << std::endl;

		if(ok)
			this->m_sett->setValue("configspace/cur_dir", QFileInfo(filename).path());
	};

	// export the current path to an external format
	auto exportPath = [this](PathsExporterFormat fmt) -> bool
	{
		std::shared_ptr<PathsExporterBase> exporter;

		QString dirLast = this->m_sett->value("configspace/cur_dir", "~/").toString();
		QString filename = QFileDialog::getSaveFileName(
			this, "Export Path", dirLast, "Text Files (*.txt)");
		if(filename=="")
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

		if(!this->m_pathsbuilder || !exporter)
		{
			QMessageBox::critical(this, "Error", "No path is available.");
			return false;
		}

		if(!m_pathsbuilder->AcceptExporter(exporter.get(), m_pathvertices))
		{
			QMessageBox::critical(this, "Error", "path could not be exported.");
			return false;
		}

		this->m_sett->setValue("configspace/cur_dir", QFileInfo(filename).path());
		return true;
	};
	// ------------------------------------------------------------------------


	// ------------------------------------------------------------------------
	// connections
	// ------------------------------------------------------------------------
	connect(m_plot.get(), &QCustomPlot::mousePress,
	[this](QMouseEvent* evt)
	{
		if(!this->m_plot || !m_moveInstr)
			return;

		const t_real _a4 = this->m_plot->xAxis->pixelToCoord(evt->x());
		const t_real _a2 = this->m_plot->yAxis->pixelToCoord(evt->y());

		std::optional<t_real> a1 = _a2 * t_real(0.5) / t_real(180) * tl2::pi<t_real>;
		std::optional<t_real> a4 = _a4 / t_real(180) * tl2::pi<t_real>;

		// move instrument
		this->EmitGotoAngles(a1, std::nullopt, a4, std::nullopt);
	});

	connect(m_plot.get(), &QCustomPlot::mouseMove,
	[this](QMouseEvent* evt)
	{
		if(!this->m_plot)
			return;

		const int x = evt->x();
		const int y = evt->y();
		const t_real _a4 = this->m_plot->xAxis->pixelToCoord(x);
		const t_real _a2 = this->m_plot->yAxis->pixelToCoord(y);

		// move instrument
		if(m_moveInstr && (evt->buttons() & Qt::LeftButton))
		{
			std::optional<t_real> a1 = _a2 * t_real(0.5) / t_real(180) * tl2::pi<t_real>;
			std::optional<t_real> a4 = _a4 / t_real(180) * tl2::pi<t_real>;

			this->EmitGotoAngles(a1, std::nullopt, a4, std::nullopt);
		}

		// set status
		std::ostringstream ostr;
		ostr.precision(g_prec_gui);

		// show angular coordinates
		ostr << "2θ_S = " << _a4 << " deg, 2θ_M = " << _a2 << " deg.";

		// show pixel coordinates
		if(m_pathsbuilder)
		{
			t_vec2 pix = m_pathsbuilder->AngleToPixel(_a4, _a2);
			ostr <<" Pixel: (" << (int)pix[0] << ", " << (int)pix[1] << ").";
		}

		m_status->setText(ostr.str().c_str());
	});

	connect(acSimplifyContour, &QAction::toggled, [this](bool simplify)->void
	{ m_simplifycontour = simplify; });

	connect(acGroupLines, &QAction::toggled, [this](bool group)->void
	{ m_grouplines = group; });

	connect(acSplitContour, &QAction::toggled, [this](bool split)->void
	{ m_splitcontour = split; });

	connect(acCalcVoro, &QAction::toggled, [this](bool calc)->void
	{ m_calcvoronoi = calc; });

	connect(acSubdivPath, &QAction::toggled, [this](bool subdiv)->void
	{ m_subdivide_path = subdiv; });

	connect(acAutocalcPath, &QAction::toggled, [this](bool calc)->void
	{ m_autocalcpath = calc; });

	connect(acMoveTarget, &QAction::toggled, [this](bool b)->void
	{ m_movetarget = b; });

	connect(acEnableZoom, &QAction::toggled, [this](bool enableZoom)->void
	{ this->SetInstrumentMovable(!enableZoom); });

	connect(acResetZoom, &QAction::triggered, [this]()->void
	{
		m_plot->rescaleAxes();
		m_plot->replot();
	});

	connect(acExportRaw, &QAction::triggered, this, [exportPath]()
	{
		exportPath(PathsExporterFormat::RAW);
	});

	connect(acExportNomad, &QAction::triggered, this, [exportPath]()
	{
		exportPath(PathsExporterFormat::NOMAD);
	});

	connect(acExportNicos, &QAction::triggered, this, [exportPath]()
	{
		exportPath(PathsExporterFormat::NICOS);
	});

	connect(acSaveLines, &QAction::triggered, this, saveLines);
	connect(acSavePDF, &QAction::triggered, this, savePDF);
	connect(acSaveGraph, &QAction::triggered, this, saveGraph);
	connect(btnSave, &QPushButton::clicked, savePDF);
	connect(acCalcMesh, &QAction::triggered, this, &ConfigSpaceDlg::CalculatePathMesh);
	connect(acCalcPath, &QAction::triggered, this, &ConfigSpaceDlg::CalculatePath);
	connect(btnCalc, &QPushButton::clicked, this, &ConfigSpaceDlg::CalculatePathMesh);
	connect(btnClose, &QPushButton::clicked, this, &ConfigSpaceDlg::accept);
	connect(acQuit, &QAction::triggered, this, &ConfigSpaceDlg::accept);
	// ------------------------------------------------------------------------


	SetInstrumentMovable(m_moveInstr);
}


ConfigSpaceDlg::~ConfigSpaceDlg()
{
	UnsetPathsBuilder();
}


void ConfigSpaceDlg::accept()
{
	if(m_sett)
		m_sett->setValue("configspace/geo", saveGeometry());
	QDialog::accept();
}


void ConfigSpaceDlg::UpdatePlotRanges()
{
	if(m_plot)
	{
		m_plot->xAxis->setRange(
			m_starta4/tl2::pi<t_real>*180.,
			m_enda4/tl2::pi<t_real>*180.);

		m_plot->yAxis->setRange(
			m_starta2/tl2::pi<t_real>*180.,
			m_enda2/tl2::pi<t_real>*180.);
	}

	if(m_colourMap)
	{
		m_colourMap->data()->setRange(
			QCPRange{m_starta4/tl2::pi<t_real>*180., m_enda4/tl2::pi<t_real>*180.}, 
			QCPRange{m_starta2/tl2::pi<t_real>*180., m_enda2/tl2::pi<t_real>*180.});
	}
}


/**
 * update the current instrument position indicator if the instrument has moved
 */
void ConfigSpaceDlg::UpdateInstrument(const Instrument& instr, const t_real* sensesCCW)
{
	m_curMonoScatteringAngle = instr.GetMonochromator().GetAxisAngleOut();
	m_curSampleScatteringAngle = instr.GetSample().GetAxisAngleOut();
	//m_curAnaScatteringAngle = instr.GetAnalyser().GetAxisAngleOut();

	if(sensesCCW)
	{
		m_curMonoScatteringAngle *= sensesCCW[0];
		m_curSampleScatteringAngle *= sensesCCW[1];
		//m_anaScatteringAngle *= sensesCCW[2];
	}

	QVector<t_real> x, y;
	x << m_curSampleScatteringAngle / tl2::pi<t_real> * t_real(180);
	y << m_curMonoScatteringAngle / tl2::pi<t_real> * t_real(180);

	m_instrposplot->setData(x, y);
	m_plot->replot();

	if(m_autocalcpath)
		CalculatePath();
}


/**
 * update the current target position indicator
 */
void ConfigSpaceDlg::UpdateTarget(t_real monoScAngle, t_real sampleScAngle, const t_real* sensesCCW)
{
	m_targetMonoScatteringAngle = monoScAngle;
	m_targetSampleScatteringAngle = sampleScAngle;

	if(sensesCCW)
	{
		m_targetMonoScatteringAngle *= sensesCCW[0];
		m_targetSampleScatteringAngle *= sensesCCW[1];
		//m_targetAnaScatteringAngle *= sensesCCW[2];
	}

	QVector<t_real> x, y;
	x << m_targetSampleScatteringAngle / tl2::pi<t_real> * t_real(180);
	y << m_targetMonoScatteringAngle / tl2::pi<t_real> * t_real(180);

	m_targetposplot->setData(x, y);
	m_plot->replot();

	if(m_autocalcpath)
		CalculatePath();
}


/**
 * either move instrument by clicking in the plot or enable plot zoom mode
 */
void ConfigSpaceDlg::SetInstrumentMovable(bool moveInstr)
{
	m_moveInstr = moveInstr;

	if(!m_plot)
		return;

	if(m_moveInstr)
	{
		m_plot->setSelectionRectMode(QCP::srmNone);
		m_plot->setInteraction(QCP::iRangeZoom, false);
		m_plot->setInteraction(QCP::iRangeDrag, false);
	}
	else
	{
		m_plot->setSelectionRectMode(QCP::srmZoom);
		m_plot->setInteraction(QCP::iRangeZoom, true);
		m_plot->setInteraction(QCP::iRangeDrag, true);
	}
}


/**
 * set instrument angles to the specified ones
 */
void ConfigSpaceDlg::EmitGotoAngles(std::optional<t_real> a1,
	std::optional<t_real> a3, std::optional<t_real> a4,
	std::optional<t_real> a5)
{
	if(m_autocalcpath)
		CalculatePath();

	emit GotoAngles(a1, a3, a4, a5, m_movetarget);
}


/**
 * calculate the mesh of possible instrument paths
 */
void ConfigSpaceDlg::CalculatePathMesh()
{
	if(!m_pathsbuilder || !m_pathsbuilder->GetInstrumentSpace())
		return;

	const auto& instr = m_pathsbuilder->GetInstrumentSpace()->GetInstrument();

	// plot angular steps
	t_real da2 = m_spinDelta2ThM->value() / 180. * tl2::pi<t_real>;
	t_real da4 = m_spinDelta2ThS->value() / 180. * tl2::pi<t_real>;

	// get the angular limits from the instrument model
	m_starta2 = instr.GetMonochromator().GetAxisAngleOutLowerLimit();
	m_enda2 = instr.GetMonochromator().GetAxisAngleOutUpperLimit();
	m_starta4 = instr.GetSample().GetAxisAngleOutLowerLimit();
	m_enda4 = instr.GetSample().GetAxisAngleOutUpperLimit();

	// angular padding
	t_real padding = 4;
	m_starta2 -= padding * da2;
	m_enda2 += padding * da2;
	m_starta4 -= padding * da4;
	m_enda4 += padding * da4;

	UpdatePlotRanges();

	m_status->setText("Clearing old paths.");
	m_pathsbuilder->Clear();

	m_status->setText("Calculating configuration space.");
	if(!m_pathsbuilder->CalculateConfigSpace(da2, da4, 
		m_starta2, m_enda2, m_starta4, m_enda4))
	{
		m_status->setText("Error: Configuration space calculation failed.");
		return;
	}

	m_status->setText("Calculating obstacle contour lines.");
	if(!m_pathsbuilder->CalculateWallContours(m_simplifycontour, m_splitcontour))
	{
		m_status->setText("Error: Obstacle contour lines calculation failed.");
		return;
	}

	m_status->setText("Calculating line segments.");
	if(!m_pathsbuilder->CalculateLineSegments())
	{
		m_status->setText("Error: Line segment calculation failed.");
		return;
	}

	if(m_calcvoronoi)
	{
		m_status->setText("Calculating Voronoi regions.");
		if(!m_pathsbuilder->CalculateVoronoi(m_grouplines))
		{
			m_status->setText("Error: Voronoi regions calculation failed.");
			return;
		}
	}

	m_status->setText("Calculation finished.");
	RedrawVoronoiPlot();
}


/**
 * calculate the instrument path from the current to the target position
 */
void ConfigSpaceDlg::CalculatePath()
{
	m_pathvertices.clear();

	if(!m_pathsbuilder)
		return;

	// find path from current to target position
	InstrumentPath path = m_pathsbuilder->FindPath(
		m_curMonoScatteringAngle, m_curSampleScatteringAngle,
		m_targetMonoScatteringAngle, m_targetSampleScatteringAngle);

	if(!path.ok)
	{
		m_status->setText("Error: No path could be found.");
		if(!m_autocalcpath)
			QMessageBox::critical(this, "Error", "No path could be found.");
		return;
	}

	// get the vertices on the path
	m_pathvertices = m_pathsbuilder->GetPathVertices(path, m_subdivide_path, true);

	RedrawPathPlot();
}


/**
 * associate a PathsBuilder and register the progress dialog callback
 */
void ConfigSpaceDlg::SetPathsBuilder(PathsBuilder* builder)
{
	UnsetPathsBuilder();

	m_pathsbuilder = builder;
	m_pathsbuilderslot = m_pathsbuilder->AddProgressSlot(
		[this](bool start, bool end, t_real progress, const std::string& message) -> bool
		{
			if(this->thread() == QThread::currentThread())
			{
				return this->PathsBuilderProgress(start, end, progress, message);
			}
			else
			{
				// alternate call via meta object when coming from another thread
				bool ok = true;
				QMetaObject::invokeMethod(this, "PathsBuilderProgress", Qt::QueuedConnection,
					//Q_RETURN_ARG(bool, ok),
					Q_ARG(bool, start),
					Q_ARG(bool, end),
					Q_ARG(t_real, progress),
					Q_ARG(const std::string&, message)
				);
				return ok;
			}
		});
}


/**
 * disassociate a PathsBuilder and unregister the progress dialog callback
 */
void ConfigSpaceDlg::UnsetPathsBuilder()
{
	if(m_pathsbuilder)
	{
		m_pathsbuilderslot.disconnect();
		m_pathsbuilder = nullptr;
	}
}


void ConfigSpaceDlg::ClearVoronoiPlotCurves()
{
	// TODO: optimise, as this takes too much time
	for(auto* plot : m_vorocurves)
	{
		m_plot->removePlottable(plot);
		//delete plot;
	}

	m_vorocurves.clear();
}


void ConfigSpaceDlg::ClearPathPlotCurve()
{
	m_plot->removePlottable(m_pathcurve);
	//delete m_pathcurve;
}


void ConfigSpaceDlg::AddVoronoiPlotCurve(const QVector<t_real>& x, const QVector<t_real>& y,
	t_real width, QColor colour)
{
	QCPCurve *voroplot = new QCPCurve(m_plot->xAxis, m_plot->yAxis);
	voroplot->setLineStyle(QCPCurve::lsLine);
	voroplot->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssNone, 1));
	voroplot->setAntialiased(true);

	QPen voropen = voroplot->pen();
	voropen.setColor(colour);
	voropen.setWidthF(width);
	voroplot->setPen(voropen);

	voroplot->setData(x, y);
	m_vorocurves.push_back(voroplot);
}


void ConfigSpaceDlg::SetPathPlotCurve(const QVector<t_real>& x, const QVector<t_real>& y,
	t_real width, QColor colour)
{
	m_pathcurve = new QCPCurve(m_plot->xAxis, m_plot->yAxis);
	m_pathcurve->setLineStyle(QCPCurve::lsLine);
	m_pathcurve->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssNone, 1));
	m_pathcurve->setAntialiased(true);

	QPen voropen = m_pathcurve->pen();
	voropen.setColor(colour);
	voropen.setWidthF(width);
	m_pathcurve->setPen(voropen);

	m_pathcurve->setData(x, y);
}


/**
 * redraw the path mesh
 */
void ConfigSpaceDlg::RedrawVoronoiPlot()
{
	ClearVoronoiPlotCurves();

	// draw wall image
	const auto& img = m_pathsbuilder->GetImage();
	const std::size_t width = img.GetWidth();
	const std::size_t height = img.GetHeight();

	m_colourMap->data()->setSize(width, height);

	for(std::size_t y=0; y<height; ++y)
	{
		for(std::size_t x=0; x<width; ++x)
		{
			using t_pixel = typename std::decay_t<decltype(img)>::value_type;
			t_pixel pixel_val = img.GetPixel(x, y);

			// val > 0 => colliding
			t_real val = std::lerp(t_real(0), t_real(1), 
				t_real(pixel_val)/t_real(std::numeric_limits<t_pixel>::max()));
			m_colourMap->data()->setCell(x, y, val);
		}
	}


	// draw wall contours
	const auto& contours = m_pathsbuilder->GetWallContours(true);

	for(const auto& contour : contours)
		for(const auto& vec : contour)
			m_colourMap->data()->setCell(vec[0], vec[1], 0.5);


	// draw linear voronoi edges
	const t_real edge_eps = m_pathsbuilder->GetVoronoiEdgeEpsilon();

	for(const auto& edge : m_pathsbuilder->GetVoronoiResults().GetLinearEdges())
	{
		const auto& line = std::get<1>(edge);

		QVector<t_real> vecx, vecy;
		vecx.reserve((std::size_t)std::ceil(1./edge_eps));
		vecy.reserve((std::size_t)std::ceil(1./edge_eps));

		for(t_real param = 0.; param <= 1.; param += edge_eps)
		{
			t_vec2 point = std::get<0>(line) + param * (std::get<1>(line) - std::get<0>(line));
			int x = int(point[0]);
			int y = int(point[1]);

			if(x>=0 && y>=0 && std::size_t(x)<width && std::size_t(y)<height)
			{
				//m_colourMap->data()->setCell(x, y, 0.25);
				t_vec2 angles = m_pathsbuilder->PixelToAngle(point[0], point[1], true);

				vecx << angles[0];
				vecy << angles[1];
			}
		}

		AddVoronoiPlotCurve(vecx, vecy);
	}


	// draw parabolic voronoi edges
	for(const auto& edge : m_pathsbuilder->GetVoronoiResults().GetParabolicEdges())
	{
		const auto& points = std::get<1>(edge);

		QVector<t_real> vecx, vecy;
		vecx.reserve(points.size());
		vecy.reserve(points.size());

		for(const auto& point : points)
		{
			int x = int(point[0]);
			int y = int(point[1]);

			if(x>=0 && y>=0 && std::size_t(x)<width && std::size_t(y)<height)
			{
				//m_colourMap->data()->setCell(x, y, 0.25);
				t_vec2 angles = m_pathsbuilder->PixelToAngle(point[0], point[1], true);

				vecx << angles[0];
				vecy << angles[1];
			}
		}

		AddVoronoiPlotCurve(vecx, vecy);
	}


	// replot
	m_plot->rescaleAxes();
	m_plot->replot();
}


/**
 * redraw the current instrument path
 */
void ConfigSpaceDlg::RedrawPathPlot()
{
	ClearPathPlotCurve();


	// draw path curve
	QVector<t_real> pathx, pathy;
	pathx.reserve(m_pathvertices.size());
	pathy.reserve(m_pathvertices.size());

	for(const t_vec2& pathvert : m_pathvertices)
	{
		pathx << pathvert[0];
		pathy << pathvert[1];
	}

	SetPathPlotCurve(pathx, pathy, 4., QColor::fromRgbF(0.9, 0.9, 0.));


	// replot
	m_plot->rescaleAxes();
	m_plot->replot();
}


/**
 * display a progress dialog
 */
bool ConfigSpaceDlg::PathsBuilderProgress(bool start, bool end, t_real progress, const std::string& message)
{
	static const int max_progress = 1000;

	if(!m_progress)
		m_progress = std::make_unique<QProgressDialog>(this);

	if(start)
	{
		m_progress->setWindowModality(Qt::WindowModal);
		m_progress->setLabelText(message.c_str());
		m_progress->setMinimum(0);
		m_progress->setMaximum(max_progress);
		m_progress->setValue(0);
		m_progress->setAutoReset(false);
		m_progress->setMinimumDuration(1000);
	}

	m_progress->setValue(int(progress*max_progress));
	bool ok = !m_progress->wasCanceled();

	RedrawVoronoiPlot();

	if(end)
	{
		m_progress->reset();
		//m_progress.reset();
	}

	return ok;
}

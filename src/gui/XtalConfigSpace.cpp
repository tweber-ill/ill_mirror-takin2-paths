/**
 * xtal configuration space
 * @author Tobias Weber <tweber@ill.fr>
 * @date aug-2021
 * @license GPLv3, see 'LICENSE' file
 *
 * References:
 *   - https://www.qcustomplot.com/documentation/classQCustomPlot.html
 *   - https://www.qcustomplot.com/documentation/classQCPColorMap.html
 *   - https://www.qcustomplot.com/documentation/classQCPGraph.html
 *   - https://www.qcustomplot.com/documentation/classQCPCurve.html
 */

#include "XtalConfigSpace.h"

#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMenu>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QFileDialog>

#include "Settings.h"


XtalConfigSpaceDlg::XtalConfigSpaceDlg(QWidget* parent, QSettings *sett)
	: QDialog{parent}, m_sett{sett}
{
	setWindowTitle("Crystal Configuration Space");

	// restore dialog geometry
	if(m_sett && m_sett->contains("xtalconfigspace/geo"))
		restoreGeometry(m_sett->value("xtalconfigspace/geo").toByteArray());
	else
		resize(800, 600);

	// plotter
	m_plot = std::make_shared<QCustomPlot>(this);
	m_plot->xAxis->setLabel("x * Orientation Vector 1");
	m_plot->yAxis->setLabel("y * Orientation Vector 2");
	m_plot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_plot->setInteraction(QCP::iSelectPlottablesBeyondAxisRect, false);

	// wall contours
	m_colourMap = new QCPColorMap(m_plot->xAxis, m_plot->yAxis);
	m_colourMap->setGradient(QCPColorGradient::gpJet);
	m_colourMap->setDataRange(QCPRange{0, 1});
	m_colourMap->setDataScaleType(QCPAxis::stLinear);
	m_colourMap->setInterpolate(false);
	m_colourMap->setAntialiased(false);

	// status label
	m_status = new QLabel(this);
	m_status->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_status->setFrameStyle(QFrame::Sunken);
	m_status->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

	// spin boxes
	m_spinVec1Start = new QDoubleSpinBox(this);
	m_spinVec1End = new QDoubleSpinBox(this);

	m_spinVec1Start->setPrefix("x_start = ");
	m_spinVec1Start->setValue(0.0);
	m_spinVec1Start->setSingleStep(0.1);
	m_spinVec1End->setPrefix("x_end = ");
	m_spinVec1End->setValue(1.0);
	m_spinVec1End->setSingleStep(0.1);

	m_spinVec2Start = new QDoubleSpinBox(this);
	m_spinVec2End = new QDoubleSpinBox(this);

	m_spinVec2Start->setPrefix("y_start = ");
	m_spinVec2Start->setValue(0.0);
	m_spinVec2Start->setSingleStep(0.1);
	m_spinVec2End->setPrefix("y_end = ");
	m_spinVec2End->setValue(1.0);
	m_spinVec2End->setSingleStep(0.1);

	UpdatePlotRanges();

	// buttons
	QPushButton *btnCalc = new QPushButton("Calculate", this);
	QPushButton *btnSave = new QPushButton("Save Figure...", this);
	QPushButton *btnClose = new QPushButton("OK", this);

	// grid
	auto grid = new QGridLayout(this);
	grid->setSpacing(4);
	grid->setContentsMargins(12, 12, 12, 12);
	int y = 0;
	grid->addWidget(m_plot.get(), y++, 0, 1, 4);
	grid->addWidget(m_spinVec1Start, y, 0, 1, 1);
	grid->addWidget(m_spinVec1End, y, 1, 1, 1);
	grid->addWidget(m_spinVec2Start, y, 2, 1, 1);
	grid->addWidget(m_spinVec2End, y++, 3, 1, 1);
	grid->addWidget(btnCalc, y, 1, 1, 1);
	grid->addWidget(btnSave, y, 2, 1, 1);
	grid->addWidget(btnClose, y++, 3, 1, 1);
	grid->addWidget(m_status, y++, 0, 1, 4);


	// ------------------------------------------------------------------------
	// menu
	// ------------------------------------------------------------------------
	QMenu *menuFile = new QMenu("File", this);
	QMenu *menuView = new QMenu("View", this);

	QAction *acSavePDF = new QAction("Save Figure...", menuFile);
	menuFile->addAction(acSavePDF);
	menuFile->addSeparator();

	QAction *acQuit = new QAction(QIcon::fromTheme("application-exit"), "Quit", menuFile);
	acQuit->setShortcut(QKeySequence::Quit);
	acQuit->setMenuRole(QAction::QuitRole);
	menuFile->addAction(acQuit);

	QAction *acEnableZoom = new QAction("Enable Zoom", menuView);
	acEnableZoom->setCheckable(true);
	acEnableZoom->setChecked(!m_moveInstr);
	menuView->addAction(acEnableZoom);

	QAction *acResetZoom = new QAction("Reset Zoom", menuView);
	menuView->addAction(acResetZoom);

	auto* menuBar = new QMenuBar(this);
	menuBar->addMenu(menuFile);
	menuBar->addMenu(menuView);
	grid->setMenuBar(menuBar);
	// ------------------------------------------------------------------------


	// ------------------------------------------------------------------------
	// output functions
	// ------------------------------------------------------------------------
	// export figure as pdf file
	auto savePDF = [this]()
	{
		QString dirLast = this->m_sett->value("xtalconfigspace/cur_dir", "~/").toString();
		QString filename = QFileDialog::getSaveFileName(
			this, "Save PDF Figure", dirLast, "PDF Files (*.pdf)");
		if(filename=="")
			return;

		if(this->m_plot->savePdf(filename))
			this->m_sett->setValue("xtalconfigspace/cur_dir", QFileInfo(filename).path());
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

		const t_real x = this->m_plot->xAxis->pixelToCoord(evt->x());
		const t_real y = this->m_plot->yAxis->pixelToCoord(evt->y());

		// move instrument
		//this->EmitGotoAngles(a1, std::nullopt, a4, std::nullopt);
	});

	connect(m_plot.get(), &QCustomPlot::mouseMove,
	[this](QMouseEvent* evt)
	{
		if(!this->m_plot)
			return;

		const int _x = evt->x();
		const int _y = evt->y();
		const t_real x = this->m_plot->xAxis->pixelToCoord(_x);
		const t_real y = this->m_plot->yAxis->pixelToCoord(_y);

		// move instrument
		if(m_moveInstr && (evt->buttons() & Qt::LeftButton))
		{
			//this->EmitGotoAngles(a1, std::nullopt, a4, std::nullopt);
		}

		// set status
		std::ostringstream ostr;
		ostr.precision(g_prec_gui);

		// show coordinates
		ostr << "x = " << x << ", y = " << y << ".";

		m_status->setText(ostr.str().c_str());
	});


	// connections
	connect(acEnableZoom, &QAction::toggled, [this](bool enableZoom)->void
	{ this->SetInstrumentMovable(!enableZoom); });

	connect(acResetZoom, &QAction::triggered, [this]()->void
	{
		m_plot->rescaleAxes();
		m_plot->replot();
	});

	connect(acSavePDF, &QAction::triggered, this, savePDF);
	connect(btnSave, &QPushButton::clicked, savePDF);
	connect(btnCalc, &QPushButton::clicked, this, &XtalConfigSpaceDlg::Calculate);
	connect(btnClose, &QPushButton::clicked, this, &XtalConfigSpaceDlg::accept);
	connect(acQuit, &QAction::triggered, this, &XtalConfigSpaceDlg::accept);
	// ------------------------------------------------------------------------


	SetInstrumentMovable(m_moveInstr);
}


XtalConfigSpaceDlg::~XtalConfigSpaceDlg()
{
}


void XtalConfigSpaceDlg::accept()
{
	if(m_sett)
		m_sett->setValue("xtalconfigspace/geo", saveGeometry());
	QDialog::accept();
}


/**
 * either move instrument by clicking in the plot or enable plot zoom mode
 */
void XtalConfigSpaceDlg::SetInstrumentMovable(bool moveInstr)
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


void XtalConfigSpaceDlg::UpdatePlotRanges()
{
	// ranges
	t_real vec1start = m_spinVec1Start->value();
	t_real vec1end = m_spinVec1End->value();
	t_real vec2start = m_spinVec2Start->value();
	t_real vec2end = m_spinVec2End->value();

	if(m_plot)
	{
		m_plot->xAxis->setRange(vec1start, vec1end);
		m_plot->yAxis->setRange(vec2start, vec2end);
	}

	if(m_colourMap)
	{
		m_colourMap->data()->setRange(
			QCPRange{vec1start, vec1end}, 
			QCPRange{vec2start, vec2end});
	}
}


/**
 * calculate the obstacles
 */
void XtalConfigSpaceDlg::Calculate()
{
	if(!m_instrspace || !m_tascalc)
		return;

	// orientation vectors
	const t_vec& vec1 = m_tascalc->GetSampleScatteringPlane(0);
	const t_vec& vec2 = m_tascalc->GetSampleScatteringPlane(1);

	// ranges
	t_real vec1start = m_spinVec1Start->value();
	t_real vec1end = m_spinVec1End->value();
	t_real vec2start = m_spinVec2Start->value();
	t_real vec2end = m_spinVec2End->value();
}

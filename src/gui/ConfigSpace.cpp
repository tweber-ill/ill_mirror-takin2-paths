/**
 * TAS paths tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date may-2021
 * @license GPLv3, see 'LICENSE' file
 *
 * References:
 *   - https://www.qcustomplot.com/documentation/classQCPColorMap.html
 */

#include "ConfigSpace.h"
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QProgressDialog>
#include <QtWidgets/QFileDialog>

#include "Settings.h"


ConfigSpaceDlg::ConfigSpaceDlg(QWidget* parent, QSettings *sett)
	: QDialog{parent}, m_sett{sett}
{
	setWindowTitle("Configuration Space");

	// restore dialog geometry
	if(m_sett && m_sett->contains("configspace/geo"))
		restoreGeometry(m_sett->value("configspace/geo").toByteArray());
	else
		resize(800, 600);

	// plotter
	m_plot = std::make_shared<QCustomPlot>(this);
	m_plot->setSelectionRectMode(QCP::srmZoom);
	m_plot->setInteraction(QCP::Interaction(int(QCP::iRangeZoom) | int(QCP::iRangeDrag)));
	m_plot->xAxis->setLabel("2θ_S (deg)");
	m_plot->xAxis->setRange(0., 180.);
	m_plot->yAxis->setLabel("2θ_M (deg)");
	m_plot->yAxis->setRange(0., 180.);
	m_plot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	m_colourMap = new QCPColorMap(m_plot->xAxis, m_plot->yAxis);
	m_colourMap->setGradient(QCPColorGradient::gpJet);
	m_colourMap->setDataRange(QCPRange{0, 1});
	m_colourMap->setDataScaleType(QCPAxis::stLinear);
	m_colourMap->data()->setRange(QCPRange{0., 180.}, QCPRange{0., 180.});
	m_colourMap->setInterpolate(false);

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
	m_spinDelta2ThS->setValue(0.5);
	m_spinDelta2ThS->setMinimum(0.001);
	m_spinDelta2ThS->setMaximum(180.);
	m_spinDelta2ThS->setSingleStep(0.1);

	m_spinDelta2ThM->setPrefix("Δθ_M = ");
	m_spinDelta2ThM->setSuffix(" deg");
	m_spinDelta2ThM->setValue(0.5);
	m_spinDelta2ThM->setMinimum(0.001);
	m_spinDelta2ThM->setMaximum(180.);
	m_spinDelta2ThM->setSingleStep(0.1);

	// buttons
	QPushButton *btnCalc = new QPushButton("Calculate", this);
	QPushButton *btnSave = new QPushButton("Save PDF...", this);
	QPushButton *btnClose = new QPushButton("Close", this);

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

	// connections
	connect(m_plot.get(), &QCustomPlot::mousePress,
		[this](QMouseEvent* evt)
		{
			if(!this->m_plot)
				return;
			const t_real _a4 = this->m_plot->xAxis->pixelToCoord(evt->x());
			const t_real _a2 = this->m_plot->yAxis->pixelToCoord(evt->y());

			std::optional<t_real> a1 = _a2 * t_real(0.5) / t_real(180) * tl2::pi<t_real>;
			std::optional<t_real> a4 = _a4 / t_real(180) * tl2::pi<t_real>;
			this->EmitGotoAngles(a1, std::nullopt, a4, std::nullopt);
		});

	connect(m_plot.get(), &QCustomPlot::mouseMove, 
		[this](QMouseEvent* evt)
		{
			if(!this->m_plot)
				return;
			const t_real a4 = this->m_plot->xAxis->pixelToCoord(evt->x());
			const t_real a2 = this->m_plot->yAxis->pixelToCoord(evt->y());

			std::ostringstream ostr;
			ostr.precision(g_prec_gui);
			ostr << "2θ_S = " << a4 << " deg, 2θ_M = " << a2 << " deg.";
			m_status->setText(ostr.str().c_str());
		});

	connect(btnSave, &QPushButton::clicked, 
		[this]()
		{
			QString dirLast = this->m_sett->value("configspace/cur_dir", "~/").toString();

			QString filename = QFileDialog::getSaveFileName(
				this, "Save File", dirLast, "PDF Files (*.pdf)");
			if(filename=="")
				return;

			if(this->m_plot->savePdf(filename))
				this->m_sett->setValue("configspace/cur_dir", QFileInfo(filename).path());
		});

	connect(btnCalc, &QPushButton::clicked, this, &ConfigSpaceDlg::Calculate);
	connect(btnClose, &QPushButton::clicked, this, &ConfigSpaceDlg::accept);
}


ConfigSpaceDlg::~ConfigSpaceDlg()
{
}


void ConfigSpaceDlg::accept()
{
	if(m_sett)
		m_sett->setValue("configspace/geo", saveGeometry());
	QDialog::accept();
}


/**
 * set instrument angles to the specified ones
 */
void ConfigSpaceDlg::EmitGotoAngles(std::optional<t_real> a1,
	std::optional<t_real> a3, std::optional<t_real> a4,
	std::optional<t_real> a5)
{
	emit GotoAngles(a1, a3, a4, a5);
}


void ConfigSpaceDlg::Calculate()
{
	if(!m_pathsbuilder)
		return;

	t_real da2 = m_spinDelta2ThM->value();
	t_real da4 = m_spinDelta2ThS->value();


	// calculate configuration space
	m_pathsbuilder->CalculateConfigSpace(da2, da4);
	const auto& img = m_pathsbuilder->GetImage();

	m_colourMap->data()->setSize(img.GetWidth(), img.GetHeight());

	for(std::size_t y=0; y<img.GetHeight(); ++y)
	{
		for(std::size_t x=0; x<img.GetWidth(); ++x)
		{
			bool colliding = img.GetPixel(x, y) > 0;
			m_colourMap->data()->setCell(x, y, colliding ? 1. : 0.);
		}
	}


	// calculate contour lines
	m_pathsbuilder->CalculateWallContours();
	const auto& contours = m_pathsbuilder->GetWallContours();

	for(const auto& contour : contours)
		for(const auto& vec : contour)
			m_colourMap->data()->setCell(vec[0], vec[1], 0.5);


	m_plot->rescaleAxes();
	m_plot->replot();
}

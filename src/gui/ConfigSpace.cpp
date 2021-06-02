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

#include <iostream>
#include <thread>
#include <future>
#include <cmath>
#include <cstdint>

#include "mingw_hacks.h"
#include <boost/asio.hpp>
namespace asio = boost::asio;

#include "tlibs2/libs/maths.h"
#include "Settings.h"

using t_task = std::packaged_task<void()>;
using t_taskptr = std::shared_ptr<t_task>;


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
	// angles and ranges
	t_real a6 = m_instrspace->GetInstrument().GetAnalyser().GetAxisAngleOut();

	t_real da4 = m_spinDelta2ThS->value() / 180. * tl2::pi<t_real>;
	t_real starta4 = 0.;
	t_real enda4 = tl2::pi<t_real>;

	t_real da2 = m_spinDelta2ThM->value() / 180. * tl2::pi<t_real>;
	t_real starta2 = 0.;
	t_real enda2 = tl2::pi<t_real>;

	// include scattering senses
	if(m_sensesCCW)
	{
		da4 *= m_sensesCCW[1];
		starta4 *= m_sensesCCW[1];
		enda4 *= m_sensesCCW[1];

		da2 *= m_sensesCCW[0];
		starta2 *= m_sensesCCW[0];
		enda2 *= m_sensesCCW[0];
	}

	// create colour map and image
	std::size_t img_w = (enda4-starta4) / da4;
	std::size_t img_h = (enda2-starta2) / da2;
	m_colourMap->data()->setSize(img_w, img_h);
	m_img.Init(img_w, img_h);

	// create thread pool
	unsigned int num_threads = std::max<unsigned int>(
		1, std::thread::hardware_concurrency()/2);
	asio::thread_pool pool(num_threads);

	std::vector<t_taskptr> tasks;
	tasks.reserve(img_h);

	// set image pixels
	for(std::size_t img_row=0; img_row<img_h; ++img_row)
	{
		t_real a2 = std::lerp(starta2, enda2, t_real(img_row)/t_real(img_h));

		auto task = [this, img_w, img_row, starta4, enda4, a2, a6]()
		{
			InstrumentSpace instrspace_cpy = *this->m_instrspace;

			for(std::size_t img_col=0; img_col<img_w; ++img_col)
			{
				t_real a4 = std::lerp(starta4, enda4, t_real(img_col)/t_real(img_w));
				t_real a3 = a4 * 0.5;

				// set scattering angles
				instrspace_cpy.GetInstrument().GetMonochromator().SetAxisAngleOut(a2);
				instrspace_cpy.GetInstrument().GetSample().SetAxisAngleOut(a4);
				instrspace_cpy.GetInstrument().GetAnalyser().SetAxisAngleOut(a6);

				// set crystal angles
				instrspace_cpy.GetInstrument().GetMonochromator().SetAxisAngleInternal(0.5 * a2);
				instrspace_cpy.GetInstrument().GetSample().SetAxisAngleInternal(a3);
				instrspace_cpy.GetInstrument().GetAnalyser().SetAxisAngleInternal(0.5 * a6);

				// set plot and image value
				bool colliding = instrspace_cpy.CheckCollision2D();
				m_colourMap->data()->setCell(img_col, img_row, colliding ? 1. : 0.);
				m_img.SetPixel(img_col, img_row, colliding ? 255 : 0);
			}
		};

		t_taskptr taskptr = std::make_shared<t_task>(task);
		tasks.push_back(taskptr);
		asio::post(pool, [taskptr]() { (*taskptr)(); });
	}

	// progress dialog
	QProgressDialog progress(this);
	progress.setWindowModality(Qt::WindowModal);
	progress.setLabelText(QString{"Calculating configuration space in %1 threads..."}.arg(num_threads));
	progress.setMinimum(0);
	progress.setMaximum(tasks.size());
	progress.setValue(0);

	// get results
	for(std::size_t taskidx=0; taskidx<tasks.size(); ++taskidx)
	{
		if(progress.wasCanceled())
		{
			pool.stop();
			break;
		}

		tasks[taskidx]->get_future().get();
		progress.setValue(taskidx+1);
		m_plot->replot();
	}

	pool.join();
	m_plot->rescaleAxes();
	m_plot->replot();


	// calculate contour lines
	using t_contourvec = tl2::vec<int, std::vector>;
	auto contours = geo::trace_boundary<t_contourvec, decltype(m_img)>(m_img);
	m_status->setText(QString("%1 contour lines found.").arg(contours.size()));

	// draw contour lines
	for(const auto& contour : contours)
		for(const t_contourvec& vec : contour)
			m_colourMap->data()->setCell(vec[0], vec[1], 0.5);
	m_plot->replot();
}

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

#include <iostream>
#include <thread>
#include <future>
#include <cmath>
#include <cstdint>

#include <boost/asio.hpp>
namespace asio = boost::asio;

#include "tlibs2/libs/maths.h"
#include "src/core/types.h"

using t_task = std::packaged_task<void()>;
using t_taskptr = std::shared_ptr<t_task>;


ConfigSpaceDlg::ConfigSpaceDlg(QWidget* parent, QSettings *sett)
	: QDialog{parent}, m_sett{sett}
{
	setWindowTitle("Configuration Space");

	// restore dialog geometry
	if(m_sett->contains("configspace/geo"))
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

	m_colourMap = new QCPColorMap(m_plot->xAxis, m_plot->yAxis);
	m_colourMap->setGradient(QCPColorGradient::gpGrayscale);
	m_colourMap->setDataRange(QCPRange{0, 1});
	m_colourMap->setDataScaleType(QCPAxis::stLinear);
	m_colourMap->data()->setRange(QCPRange{0., 180.}, QCPRange{0., 180.});
	m_colourMap->setInterpolate(false);

	connect(m_plot.get(), &QCustomPlot::mouseMove, this, &ConfigSpaceDlg::PlotMouseMove);

	// grid
	auto grid = new QGridLayout(this);
	grid->setSpacing(4);
	grid->setContentsMargins(16, 16, 16, 16);
	grid->addWidget(m_plot.get(), 0, 0, 1, 1);
}


ConfigSpaceDlg::~ConfigSpaceDlg()
{
}


void ConfigSpaceDlg::PlotMouseMove(QMouseEvent* evt)
{
	const t_real x = m_plot->xAxis->pixelToCoord(evt->x());
	const t_real y = m_plot->yAxis->pixelToCoord(evt->y());

	//std::cout << "(" << x << ", " << y << ")" << std::endl;
}


void ConfigSpaceDlg::accept()
{
	if(m_sett)
		m_sett->setValue("configspace/geo", saveGeometry());
}


void ConfigSpaceDlg::Calculate()
{
	// angles and ranges
	t_real a6 = 83.957 / 180. * tl2::pi<t_real>;

	t_real da2 = 0.5 / 180. * tl2::pi<t_real>;
	t_real starta2 = 0.;
	t_real enda2 = tl2::pi<t_real>;

	t_real da4 = -0.5 / 180. * tl2::pi<t_real>;
	t_real starta4 = 0.;
	t_real enda4 = -tl2::pi<t_real>;

	std::size_t img_w = (enda4-starta4) / da4;
	std::size_t img_h = (enda2-starta2) / da2;

	m_colourMap->data()->setSize(img_w, img_h);

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

				t_real val = instrspace_cpy.CheckCollision2D() ? 0. : 1.;
				m_colourMap->data()->setCell(img_col, img_row, val);
			}
		};

		t_taskptr taskptr = std::make_shared<t_task>(task);
		tasks.push_back(taskptr);
		asio::post(pool, [taskptr]() { (*taskptr)(); });
	}

	for(std::size_t taskidx=0; taskidx<tasks.size(); ++taskidx)
	{
		tasks[taskidx]->get_future().get();

		std::cout << "Task " << taskidx+1 << " of " 
			<< tasks.size() << " finished." << std::endl;
	}

	pool.join();
	m_plot->rescaleAxes();
	m_plot->replot();
}

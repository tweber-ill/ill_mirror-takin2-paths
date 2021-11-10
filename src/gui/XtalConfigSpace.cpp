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
 *
 * ----------------------------------------------------------------------------
 * TAS-Paths (part of the Takin software suite)
 * Copyright (C) 2021  Tobias WEBER (Institut Laue-Langevin (ILL),
 *                     Grenoble, France).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * ----------------------------------------------------------------------------
 */

#include "XtalConfigSpace.h"

#include <QtGui/QClipboard>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMenu>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QProgressDialog>

#include "settings_variables.h"

#include "src/core/mingw_hacks.h"
#include <boost/asio.hpp>
namespace asio = boost::asio;

using t_task = std::packaged_task<void()>;
using t_taskptr = std::shared_ptr<t_task>;


XtalConfigSpaceDlg::XtalConfigSpaceDlg(QWidget* parent, QSettings *sett)
	: QDialog{parent}, m_sett{sett}
{
	setWindowTitle("Crystal Configuration Space");
	setSizeGripEnabled(true);

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
	m_colourMap->setGradient(QCPColorGradient::gpJet /*gpGrayscale*/);
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
	m_spinVec2Start = new QDoubleSpinBox(this);
	m_spinVec2End = new QDoubleSpinBox(this);
	m_spinVec1Delta = new QDoubleSpinBox(this);
	m_spinVec2Delta = new QDoubleSpinBox(this);
	m_spinE = new QDoubleSpinBox(this);

	m_spinVec1Start->setPrefix("x_start = ");
	m_spinVec1Start->setMinimum(-999.);
	m_spinVec1Start->setMaximum(999.);
	m_spinVec1Start->setValue(-1.0);
	m_spinVec1Start->setSingleStep(0.1);
	m_spinVec1End->setPrefix("x_end = ");
	m_spinVec1End->setMinimum(-999.);
	m_spinVec1End->setMaximum(999.);
	m_spinVec1End->setValue(1.0);
	m_spinVec1End->setSingleStep(0.1);
	m_spinVec1Delta->setPrefix("Δx = ");
	m_spinVec1Delta->setDecimals(4);
	m_spinVec1Delta->setMinimum(0.0001);
	m_spinVec1Delta->setMaximum(999.);
	m_spinVec1Delta->setValue(0.05);
	m_spinVec1Delta->setSingleStep(0.01);

	m_spinVec2Start->setPrefix("y_start = ");
	m_spinVec2Start->setMinimum(-999);
	m_spinVec2Start->setMaximum(999);
	m_spinVec2Start->setValue(-1.0);
	m_spinVec2Start->setSingleStep(0.1);
	m_spinVec2End->setPrefix("y_end = ");
	m_spinVec2End->setMinimum(-999);
	m_spinVec2End->setMaximum(999);
	m_spinVec2End->setValue(1.0);
	m_spinVec2End->setSingleStep(0.1);
	m_spinVec2Delta->setPrefix("Δy = ");
	m_spinVec2Delta->setDecimals(4);
	m_spinVec2Delta->setMinimum(0.0001);
	m_spinVec2Delta->setMaximum(999.);
	m_spinVec2Delta->setValue(0.05);
	m_spinVec2Delta->setSingleStep(0.01);

	m_spinE->setPrefix("E = ");
	m_spinE->setSuffix(" meV");
	m_spinE->setMinimum(-999.);
	m_spinE->setMaximum(999.);
	m_spinE->setValue(0.);
	m_spinE->setSingleStep(0.1);

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
	grid->addWidget(m_spinVec1Delta, y, 0, 1, 1);
	grid->addWidget(m_spinVec2Delta, y, 1, 1, 1);
	grid->addWidget(m_spinE, y++, 3, 1, 1);
	grid->addWidget(btnCalc, y, 1, 1, 1);
	grid->addWidget(btnSave, y, 2, 1, 1);
	grid->addWidget(btnClose, y++, 3, 1, 1);
	grid->addWidget(m_status, y++, 0, 1, 4);


	// ------------------------------------------------------------------------
	// menu
	// ------------------------------------------------------------------------
	QMenu *menuFile = new QMenu("File", this);
	QMenu *menuEdit = new QMenu("Edit", this);
	QMenu *menuView = new QMenu("View", this);

	QAction *acSavePDF = new QAction("Save Figure...", menuFile);
	acSavePDF->setShortcut(QKeySequence::Save);
	menuFile->addAction(acSavePDF);
	menuFile->addSeparator();

	QAction *acQuit = new QAction(QIcon::fromTheme("window-close"), "Close", menuFile);
	acQuit->setShortcut(QKeySequence::Close);
	menuFile->addAction(acQuit);

	QAction *acCopy = new QAction("Copy Figure", menuEdit);
	acCopy->setShortcut(QKeySequence::Copy);
	menuEdit->addAction(acCopy);
	
	QAction *acEnableZoom = new QAction("Enable Zoom", menuView);
	acEnableZoom->setCheckable(true);
	acEnableZoom->setChecked(!m_moveInstr);
	menuView->addAction(acEnableZoom);

	QAction *acResetZoom = new QAction("Reset Zoom", menuView);
	menuView->addAction(acResetZoom);

	auto* menuBar = new QMenuBar(this);
	menuBar->addMenu(menuFile);
	menuBar->addMenu(menuEdit);
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

	// copy figure to clipboard
	auto copyFigure = [this]()
	{
		QClipboard *clp = QGuiApplication::clipboard();
		if(!m_plot || !clp)
			return;

		QPixmap pix = m_plot->toPixmap();
		QImage img = pix.toImage();
		clp->setImage(img);
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

		// coordinates
		const t_real x = this->m_plot->xAxis->pixelToCoord(evt->pos().x());
		const t_real y = this->m_plot->yAxis->pixelToCoord(evt->pos().y());

		auto [Q, ki, kf] = GetQkikf(x, y);

		// move instrument
		emit GotoCoordinates(Q[0], Q[1], Q[2], ki, kf);
	});

	connect(m_plot.get(), &QCustomPlot::mouseMove,
	[this](QMouseEvent* evt)
	{
		if(!this->m_plot)
			return;

		// coordinates
		const int _x = evt->pos().x();
		const int _y = evt->pos().y();

		const t_real x = this->m_plot->xAxis->pixelToCoord(_x);
		const t_real y = this->m_plot->yAxis->pixelToCoord(_y);

		// crystal coordinates
		auto [Q, ki, kf] = GetQkikf(x, y);

		// move instrument
		if(m_moveInstr && (evt->buttons() & Qt::LeftButton))
		{
			emit GotoCoordinates(Q[0], Q[1], Q[2], ki, kf);
		}

		// set status
		std::ostringstream ostr;
		ostr.precision(g_prec_gui);

		// show coordinates
		ostr << "x = " << x << ", y = " << y << ";";
		ostr << " Q = (" << Q[0] << ", " << Q[1] << ", " << Q[2] << ")"
			<< "; ki = " << ki << ", kf = " << kf << ".";

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
	connect(acCopy, &QAction::triggered, this, copyFigure);
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
 * redraw plot
 */
void XtalConfigSpaceDlg::RedrawPlot()
{
	// print scattering plane vectors as axis labels
	if(m_tascalc)
	{
		const t_vec& vec1 = m_tascalc->GetSampleScatteringPlane(0);
		const t_vec& vec2 = m_tascalc->GetSampleScatteringPlane(1);

		std::ostringstream xlabel, ylabel;
		xlabel << "x * [" << vec1[0] << ", " << vec1[1] << ", " << vec1[2] << "]";
		ylabel << "y * [" << vec2[0] << ", " << vec2[1] << ", " << vec2[2] << "]";

		m_plot->xAxis->setLabel(xlabel.str().c_str());
		m_plot->yAxis->setLabel(ylabel.str().c_str());
	}

	UpdatePlotRanges();

	// draw wall image
	const std::size_t width = m_img.GetWidth();
	const std::size_t height = m_img.GetHeight();

	m_colourMap->data()->setSize(width, height);

	for(std::size_t y=0; y<height; ++y)
	{
		for(std::size_t x=0; x<width; ++x)
		{
			using t_pixel = typename std::decay_t<decltype(m_img)>::value_type;
			t_pixel pixel_val = m_img.GetPixel(x, y);

			// val > 0 => colliding
			t_real val = std::lerp(t_real(0), t_real(1),
				t_real(pixel_val)/t_real(std::numeric_limits<t_pixel>::max()));
			m_colourMap->data()->setCell(x, y, val);
		}
	}

	// replot
	m_plot->rescaleAxes();
	m_plot->replot();
}


/**
 * calculate crystal coordinates from graph position
 */
std::tuple<t_vec, t_real, t_real>
XtalConfigSpaceDlg::GetQkikf(t_real x, t_real y) const
{
	// orientation vectors
	const t_vec& vec1 = m_tascalc->GetSampleScatteringPlane(0);
	const t_vec& vec2 = m_tascalc->GetSampleScatteringPlane(1);

	// momentum
	t_vec Q = x*vec1 + y*vec2;

	// fixed energy
	t_real E = m_spinE->value();

	// wavenumbers
	t_real ki, kf;
	auto [kfix, fixed_kf] = m_tascalc->GetKfix();

	if(fixed_kf)
	{
		kf = kfix;
		ki = tl2::calc_tas_ki(kf, E);
	}
	else
	{
		ki = kfix;
		kf = tl2::calc_tas_ki(ki, E);
	}

	return std::make_tuple(Q, ki, kf);
}


/**
 * calculate the obstacle representations in the crystal configuration space
 */
void XtalConfigSpaceDlg::Calculate()
{
	if(!m_instrspace || !m_tascalc)
		return;

	// fixed energy
	t_real E = m_spinE->value();

	// ranges
	t_real vec1start = m_spinVec1Start->value();
	t_real vec1end = m_spinVec1End->value();
	t_real vec2start = m_spinVec2Start->value();
	t_real vec2end = m_spinVec2End->value();
	t_real vec1step = m_spinVec1Delta->value();
	t_real vec2step = m_spinVec2Delta->value();

	// create colour map and image
	std::size_t img_w = (vec1end-vec1start) / vec1step;
	std::size_t img_h = (vec2end-vec2start) / vec2step;

	m_img.Init(img_w, img_h);

	// create thread pool
	asio::thread_pool pool(g_maxnum_threads);

	std::vector<t_taskptr> tasks;
	tasks.reserve(img_h);

	// set image pixels
	for(std::size_t img_row=0; img_row<img_h; ++img_row)
	{
		t_real yparam = std::lerp(vec2start, vec2end, img_row / t_real(img_h));

		auto task = [this, img_w, img_row, vec1start, vec1end, yparam, E]()
		{
			InstrumentSpace instrspace_cpy = *this->m_instrspace;

			for(std::size_t img_col=0; img_col<img_w; ++img_col)
			{
				t_real xparam = std::lerp(vec1start, vec1end, img_col / t_real(img_w));

				// crystal coordinates
				auto [Q, ki, kf] = GetQkikf(xparam, yparam);

				TasAngles angles = m_tascalc->GetAngles(Q[0], Q[1], Q[2], E);
				if(angles.mono_ok && angles.ana_ok && angles.sample_ok)
				{
					// set scattering angles
					instrspace_cpy.GetInstrument().GetMonochromator().
						SetAxisAngleOut(angles.monoXtalAngle * t_real{2});
					instrspace_cpy.GetInstrument().GetSample().
						SetAxisAngleOut(angles.sampleScatteringAngle);
					instrspace_cpy.GetInstrument().GetAnalyser().
						SetAxisAngleOut(angles.anaXtalAngle * t_real{2});

					// set crystal angles
					instrspace_cpy.GetInstrument().GetMonochromator().
						SetAxisAngleInternal(angles.monoXtalAngle);
					instrspace_cpy.GetInstrument().GetSample().
						SetAxisAngleInternal(angles.sampleXtalAngle);
					instrspace_cpy.GetInstrument().GetAnalyser().
						SetAxisAngleInternal(angles.anaXtalAngle);
				}
				else
				{
					m_img.SetPixel(img_col, img_row, 0xe0);
					continue;
				}

				// set image value
				bool angle_ok = instrspace_cpy.CheckAngularLimits();

				if(!angle_ok)
				{
					m_img.SetPixel(img_col, img_row, 0xf0);
				}
				else
				{
					bool colliding = instrspace_cpy.CheckCollision2D();
					m_img.SetPixel(img_col, img_row, colliding ? 0xff : 0x00);
				}
			}
		};

		t_taskptr taskptr = std::make_shared<t_task>(task);
		tasks.push_back(taskptr);
		asio::post(pool, [taskptr]() { (*taskptr)(); });
	}

	std::size_t num_tasks = tasks.size();


	// get results
	auto progress = std::make_unique<QProgressDialog>(this);
	progress->setWindowModality(Qt::WindowModal);
	progress->setLabelText(
		QString("Calculating configuration space in %1 threads...")
			.arg(g_maxnum_threads));
	progress->setAutoReset(true);
	progress->setAutoClose(true);
	progress->setMinimumDuration(1000);
	progress->setMinimum(0);
	progress->setMaximum(num_tasks);

	for(std::size_t taskidx=0; taskidx<num_tasks; ++taskidx)
	{
		progress->setValue(taskidx);

		if(progress->wasCanceled())
		{
			pool.stop();
			break;
		}

		tasks[taskidx]->get_future().get();
		RedrawPlot();
	}

	pool.join();
	progress->setValue(num_tasks);
	RedrawPlot();
}

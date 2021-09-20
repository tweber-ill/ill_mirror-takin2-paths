/**
 * path properties dock widget
 * @author Tobias Weber <tweber@ill.fr>
 * @date apr-2021
 * @license GPLv3, see 'LICENSE' file
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

#include "PathProperties.h"
#include "../Settings.h"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QPushButton>


// --------------------------------------------------------------------------------
// properties widget
// --------------------------------------------------------------------------------
PathPropertiesWidget::PathPropertiesWidget(QWidget *parent)
	: QWidget{parent}
{
	for(std::size_t i=0; i<m_num_coord_elems; ++i)
	{
		m_spinFinish[i] = new QDoubleSpinBox(this);

		m_spinFinish[i]->setMinimum(-180);
		m_spinFinish[i]->setMaximum(180);
		m_spinFinish[i]->setSingleStep(0.1);
		m_spinFinish[i]->setDecimals(g_prec_gui);
		m_spinFinish[i]->setValue(0);
		m_spinFinish[i]->setSuffix("°");
	}

	for(std::size_t i=1; i<m_num_coord_elems; ++i)
		QWidget::setTabOrder(m_spinFinish[i-1], m_spinFinish[i]);


	// default values
	m_spinFinish[0]->setValue(90.);
	m_spinFinish[1]->setValue(90.);

	QPushButton *btnGotoFinish = new QPushButton("Jump to Target Angles", this);
	QPushButton *btnCalcMesh = new QPushButton("Calculate Path Mesh", this);
	QPushButton *btnCalcPath = new QPushButton("Calculate Path", this);
	m_sliderPath = new QSlider(Qt::Horizontal, this);
	m_btnGo = new QPushButton("Go", this);
	m_btnGo->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	const char* labels[] = {"Monochromator:", "Sample:"};

	auto *groupFinish = new QGroupBox("Target Scattering Angles", this);
	{
		auto *layoutFinish = new QGridLayout(groupFinish);
		layoutFinish->setHorizontalSpacing(2);
		layoutFinish->setVerticalSpacing(2);
		layoutFinish->setContentsMargins(4,4,4,4);

		int y = 0;
		for(std::size_t i=0; i<m_num_coord_elems; ++i)
		{
			layoutFinish->addWidget(new QLabel(labels[i], this), y, 0, 1, 1);
			layoutFinish->addWidget(m_spinFinish[i], y++, 1, 1, 1);
		}

		layoutFinish->addWidget(btnGotoFinish, y++, 0, 1, 2);
	}

	auto *groupPath = new QGroupBox("Path Tracking", this);
	{
		auto *layoutPath = new QGridLayout(groupPath);
		layoutPath->setHorizontalSpacing(2);
		layoutPath->setVerticalSpacing(2);
		layoutPath->setContentsMargins(4,4,4,4);

		int y = 0;
		layoutPath->addWidget(btnCalcMesh, y++, 0, 1, 3);
		layoutPath->addWidget(btnCalcPath, y++, 0, 1, 3);
		layoutPath->addWidget(m_sliderPath, y, 0, 1, 2);
		layoutPath->addWidget(m_btnGo, y++, 2, 1, 1);
	}

	auto *grid = new QGridLayout(this);
	grid->setHorizontalSpacing(2);
	grid->setVerticalSpacing(2);
	grid->setContentsMargins(4,4,4,4);

	int y = 0;
	grid->addWidget(groupFinish, y++, 0, 1, 1);
	grid->addWidget(groupPath, y++, 0, 1, 1);
	grid->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding), y++, 0, 1, 1);

	for(std::size_t i=0; i<m_num_coord_elems; ++i)
	{
		// target angles
		connect(m_spinFinish[i],
			static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			[this, i](t_real val) -> void
			{
				std::size_t j = (i + 1) % m_num_coord_elems;

				t_real coords[m_num_coord_elems];
				coords[i] = val;
				coords[j] = m_spinFinish[j]->value();

				//std::cout << coords[0] << ", " << coords[1] << std::endl;
				emit TargetChanged(coords[0], coords[1]);
			});
	}

	// go to target angles
	connect(btnGotoFinish, &QPushButton::clicked,
		[this]() -> void
		{
			t_real a2 = m_spinFinish[0]->value();
			t_real a4 = m_spinFinish[1]->value();

			emit GotoAngles(a2, a4);
		});

	// calculate path mesh
	connect(btnCalcMesh, &QPushButton::clicked,
		[this]() -> void
		{
			emit CalculatePathMesh();
		});

	// calculate path
	connect(btnCalcPath, &QPushButton::clicked,
		[this]() -> void
		{
			emit CalculatePath();
		});

	// path tracking slider value has changed
	connect(m_sliderPath, &QSlider::valueChanged,
		[this](int value) -> void
		{
			emit TrackPath((std::size_t)value);
		});

	// path tracking timer
	connect(&m_pathTrackTimer, &QTimer::timeout,
		this, static_cast<void (PathPropertiesWidget::*)()>(
			&PathPropertiesWidget::trackerTick));

	// start path tracking
	connect(m_btnGo, &QPushButton::clicked,
		[this]() -> void
		{
			// start the timer if it's not already running
			if(!m_pathTrackTimer.isActive())
			{
				m_pathTrackTimer.start(
					std::chrono::milliseconds(1000 / g_pathtracker_fps));
				m_btnGo->setText("Stop");
			}
			// otherwise stop it
			else
			{
				m_pathTrackTimer.stop();
				m_btnGo->setText("Go");
			}
		});
}


/**
 * timer tick to track along current path
 */
void PathPropertiesWidget::trackerTick()
{
	int val = m_sliderPath->value();
	int max = m_sliderPath->maximum();
	if(val >= max)
	{
		m_pathTrackTimer.stop();
		m_btnGo->setText("Go");
	}
	else
	{
		m_sliderPath->setValue(val+1);
	}
}


PathPropertiesWidget::~PathPropertiesWidget()
{
}


/**
 * set the target angular coordinates
 */
void PathPropertiesWidget::SetTarget(t_real a2, t_real a4)
{
	this->blockSignals(true);

	m_spinFinish[0]->setValue(a2);
	m_spinFinish[1]->setValue(a4);

	this->blockSignals(false);
	emit TargetChanged(a2, a4);
}


/**
 * a new path has been calculated
 */
void PathPropertiesWidget::PathAvailable(std::size_t numVertices)
{
	if(!m_sliderPath || numVertices == 0)
		return;

	m_sliderPath->setMinimum(0);
	m_sliderPath->setMaximum(numVertices-1);
	m_sliderPath->setValue(0);
}
// --------------------------------------------------------------------------------



// --------------------------------------------------------------------------------
// properties dock widget
// --------------------------------------------------------------------------------
PathPropertiesDockWidget::PathPropertiesDockWidget(QWidget *parent)
	: QDockWidget{parent},
		m_widget{std::make_shared<PathPropertiesWidget>(this)}
{
	setObjectName("PathPropertiesDockWidget");
	setWindowTitle("Path Properties");

	setWidget(m_widget.get());
}


PathPropertiesDockWidget::~PathPropertiesDockWidget()
{
}
// --------------------------------------------------------------------------------

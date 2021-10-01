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
	m_btnCalcPath = new QPushButton("Calculate Path", this);
	m_sliderPath = new QSlider(Qt::Horizontal, this);
	m_sliderPath->setToolTip("Path tracking.");
	m_btnGo = new QToolButton(this);
	SetGoButtonText(true);
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
		layoutPath->addWidget(m_btnCalcPath, y++, 0, 1, 3);
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
	connect(m_btnCalcPath, &QPushButton::clicked,
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
			&PathPropertiesWidget::TrackerTick));

	// start path tracking
	connect(m_btnGo, &QPushButton::clicked,
		[this]() -> void
		{
			// start the timer if it's not already running
			if(!m_pathTrackTimer.isActive())
			{
				m_pathTrackTimer.start(
					std::chrono::milliseconds(1000 / g_pathtracker_fps));
				SetGoButtonText(false);
			}
			// otherwise stop it
			else
			{
				m_pathTrackTimer.stop();
				SetGoButtonText(true);
			}
		});
}


/**
 * set text and icon of the "go" button
 */
void PathPropertiesWidget::SetGoButtonText(bool start)
{
	if(start)
	{
		// set start icon
		QIcon iconStart = QIcon::fromTheme("media-playback-start");
		m_btnGo->setIcon(iconStart);
		if(iconStart.isNull())
			m_btnGo->setText("Go");
		else
			m_btnGo->setText("");

		m_btnGo->setToolTip("Start path tracking.");
	}
	else
	{
		// set stop icon
		QIcon iconStop = QIcon::fromTheme("media-playback-stop");
		m_btnGo->setIcon(iconStop);
		if(iconStop.isNull())
			m_btnGo->setText("Stop");
		else
			m_btnGo->setText("");

		m_btnGo->setToolTip("Stop path tracking.");
	}
}


/**
 * timer tick to track along current path
 */
void PathPropertiesWidget::TrackerTick()
{
	int val = m_sliderPath->value();
	int max = m_sliderPath->maximum();
	if(val >= max)
	{
		m_pathTrackTimer.stop();
		SetGoButtonText(true);
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
	if(!m_sliderPath)
		return;

	// no path available
	if(numVertices == 0)
	{
		m_btnGo->setEnabled(false);
		m_sliderPath->setEnabled(false);
		return;
	}

	m_btnGo->setEnabled(true);
	m_sliderPath->setEnabled(true);
	m_sliderPath->setMinimum(0);
	m_sliderPath->setMaximum(numVertices-1);
	m_sliderPath->setValue(0);
}


/**
 * a path mesh has been (in)validated
 */
void PathPropertiesWidget::PathMeshValid(bool valid)
{
	m_btnCalcPath->setEnabled(valid);

	if(!valid)
		PathAvailable(0);
}


/**
 * save the dock widget's settings
 */
boost::property_tree::ptree PathPropertiesWidget::Save() const
{
	boost::property_tree::ptree prop;

	// path coordinate
	prop.put<t_real>("target_2thM", m_spinFinish[0]->value());
	prop.put<t_real>("target_2thS", m_spinFinish[1]->value());

	return prop;
}


/**
 * load the dock widget's settings
 */
bool PathPropertiesWidget::Load(const boost::property_tree::ptree& prop)
{
	// old values
	t_real target_2thM = m_spinFinish[0]->value();
	t_real target_2thS = m_spinFinish[1]->value();

	// path coordinate
	if(auto opt = prop.get_optional<t_real>("target_2thM"); opt)
		target_2thM = *opt;
	if(auto opt = prop.get_optional<t_real>("target_2thS"); opt)
		target_2thS = *opt;

	// set new values
	SetTarget(target_2thM, target_2thS);

	// emit changes
	emit TargetChanged(target_2thM, target_2thS);

	return true;
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

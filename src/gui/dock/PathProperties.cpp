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
#include "../settings_variables.h"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QPushButton>



// --------------------------------------------------------------------------------
// properties widget
// --------------------------------------------------------------------------------
#define CALC_MESH_TITLE "Update Path &Mesh"
//#define CALC_PATH_TITLE "Calculate &Path"


PathPropertiesWidget::PathPropertiesWidget(QWidget *parent)
	: QWidget{parent}
{
	const QString target_comp[] = { "monochromator/analyser", "sample" };
	const QString target_angle[] = { "Θm/a", "Θs" };

	for(std::size_t i=0; i<m_num_coord_elems; ++i)
	{
		m_spinFinish[i] = new QDoubleSpinBox(this);

		m_spinFinish[i]->setMinimum(-180);
		m_spinFinish[i]->setMaximum(180);
		m_spinFinish[i]->setSingleStep(0.1);
		m_spinFinish[i]->setDecimals(g_prec_gui);
		m_spinFinish[i]->setValue(0);
		m_spinFinish[i]->setSuffix("°");
		m_spinFinish[i]->setToolTip(
			QString("Target %1 scattering angle %2 in units of [deg].").
				arg(target_comp[i]).arg(target_angle[i]));
	}

	for(std::size_t i=1; i<m_num_coord_elems; ++i)
		QWidget::setTabOrder(m_spinFinish[i-1], m_spinFinish[i]);

	// default values
	m_spinFinish[0]->setValue(90.);
	m_spinFinish[1]->setValue(90.);

	QPushButton *btnGotoFinish = new QPushButton("Jump to Target Angles", this);
	btnGotoFinish->setToolTip("Set the current instrument position to the given target angles.");

	m_btnCalcMesh = new QPushButton(CALC_MESH_TITLE, this);
	m_btnCalcMesh->setToolTip("Calculate the mesh of possible paths used for pathfinding.");
	//m_btnCalcMesh->setShortcut(Qt::ALT | Qt::Key_M);

	//m_btnCalcPath = new QPushButton(CALC_PATH_TITLE, this);
	//m_btnCalcPath->setToolTip("Calculate the actual path from the current to the target instrument position.");
	//m_btnCalcPath->setShortcut(Qt::ALT | Qt::Key_P);

	m_sliderPath = new QSlider(Qt::Horizontal, this);
	m_sliderPath->setToolTip("Path tracking.");

	m_btnGo = new QToolButton(this);
	SetGoButtonText(true);
	m_btnGo->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);


	// TODO: change the label "monochromator" to "analyser" for ki=const mode
	const char* labels[] = {"Mono./Ana.:", "Sample:"};

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
		layoutPath->setVerticalSpacing(/*2*/ 8); // to prevent track-bar clipping
		layoutPath->setContentsMargins(4,4,4,4);

		int y = 0;
		layoutPath->addWidget(m_btnCalcMesh, y, 0, 1, 2);
		//layoutPath->addWidget(m_btnCalcPath, y, 0, 1, 2);
		layoutPath->addWidget(m_btnGo, y++, 2, 2, 1);
		layoutPath->addWidget(m_sliderPath, y, 0, 1, 2);
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

				emit TargetChanged(coords[0], coords[1]);
			});
	}

	// go to target angles
	connect(btnGotoFinish, &QPushButton::clicked, [this]()
	{
		t_real a2 = m_spinFinish[0]->value();
		t_real a4 = m_spinFinish[1]->value();

		emit GotoAngles(a2, a4);
	});

	// calculate path mesh
	connect(m_btnCalcMesh, &QPushButton::clicked, [this]()
	{
		emit CalculatePathMesh();
	});

	// calculate path
	/*connect(m_btnCalcPath, &QPushButton::clicked, [this]()
	{
		emit CalculatePath();
	});*/

	// path tracking slider value has changed
	connect(m_sliderPath, &QSlider::valueChanged, [this](int value)
	{
		emit TrackPath((std::size_t)value);
	});

	// path tracking timer
	connect(&m_pathTrackTimer, &QTimer::timeout,
		this, static_cast<void (PathPropertiesWidget::*)()>(
			&PathPropertiesWidget::TrackerTick));

	// start path tracking
	connect(m_btnGo, &QPushButton::clicked, [this]()
	{
		// start the timer if it's not already running
		if(!m_pathTrackTimer.isActive())
		{
			SetGoButtonText(false);
			m_sliderPath->setValue(0);

			m_pathTrackTimer.start(
				std::chrono::milliseconds(1000 / g_pathtracker_fps));
		}
		// otherwise stop it
		else
		{
			m_pathTrackTimer.stop();

			SetGoButtonText(true);
		}
	});


	// palette for flashing the mesh button
	m_paletteBtnNormal = m_btnCalcMesh->palette();
	m_paletteBtnFlash = m_paletteBtnNormal;
	if(g_theme.toLower() != "macintosh")  // themes with hard-coded colours
	{
		m_paletteBtnFlash.setColor(m_btnCalcMesh->backgroundRole(), QColor(0, 0, 195));
		m_paletteBtnFlash.setColor(m_btnCalcMesh->foregroundRole(), QColor(255, 255, 255));
	}

	// mesh button flashing timer
	/*connect(&m_meshButtonFlashTimer, &QTimer::timeout,
		this, static_cast<void (PathPropertiesWidget::*)()>(
			&PathPropertiesWidget::MeshButtonFlashTick));*/

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
			m_btnGo->setText(" Go ");
		else
			m_btnGo->setText("    ");

		m_btnGo->setToolTip("Start path tracking from the current to the target instrument position.");
	}
	else
	{
		// set stop icon
		QIcon iconStop = QIcon::fromTheme("media-playback-stop");
		m_btnGo->setIcon(iconStop);
		if(iconStop.isNull())
			m_btnGo->setText("Stop");
		else
			m_btnGo->setText("    ");

		m_btnGo->setToolTip("Stop path tracking.");
	}
}


/**
 * timer tick to track along current path
 */
void PathPropertiesWidget::TrackerTick()
{
	if(!m_sliderPath)
		return;

	int max = m_sliderPath->maximum();
	int val = m_sliderPath->value();

	// advance path
	int next_val = std::min(val+1, max);
	m_sliderPath->setValue(next_val);

	// end of tracking reached?
	if(next_val >= max)
	{
		m_sliderPath->setValue(max);

		m_pathTrackTimer.stop();
		SetGoButtonText(true);
	}
}


/**
 * timer tick to flash mesh calculation button
 */
/*void PathPropertiesWidget::MeshButtonFlashTick()
{
	if(!m_btnCalcMesh)
		return;

	static bool toggle = false;

	if(toggle)
	{
		m_btnCalcMesh->setPalette(m_paletteBtnNormal);
	}
	else
	{
		m_btnCalcMesh->setPalette(m_paletteBtnFlash);
	}

	toggle = !toggle;
}*/


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
 * a path mesh calculation has been started or is finishing
 */
void PathPropertiesWidget::PathMeshCalculation(CalculationState state, t_real progress)
{
	switch(state)
	{
		case CalculationState::STARTED:
		{
			PathMeshValid(false);
			m_btnCalcMesh->setEnabled(false);
			m_btnCalcMesh->setText("STAND BY");
			break;
		}

		case CalculationState::RUNNING:
		{
			QString txt = QString("RUNNING: %1%").arg(int(progress*100.));
			m_btnCalcMesh->setText(txt);
			break;
		}

		case CalculationState::FAILED:
		case CalculationState::SUCCEEDED:
		{
			m_btnCalcMesh->setText(CALC_MESH_TITLE);
			m_btnCalcMesh->setEnabled(true);
			break;
		}

		default:
			break;
	}
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
	}
	else
	{
		m_btnGo->setEnabled(true);
		m_sliderPath->setEnabled(true);
		m_sliderPath->setMinimum(0);
		m_sliderPath->setMaximum(numVertices-1);
		m_sliderPath->setValue(0);
	}
}


/**
 * a path mesh has been (in)validated
 */
void PathPropertiesWidget::PathMeshValid(bool valid)
{
	//m_btnCalcPath->setEnabled(valid);

	if(valid)
	{
		//m_meshButtonFlashTimer.stop();
		if(m_btnCalcMesh)
			m_btnCalcMesh->setPalette(m_paletteBtnNormal);
	}
	else
	{
		PathAvailable(0);

		if(m_btnCalcMesh)
			m_btnCalcMesh->setPalette(m_paletteBtnFlash);

		/*if(g_allow_gui_flashing && !m_meshButtonFlashTimer.isActive())
			m_meshButtonFlashTimer.start(
				std::chrono::milliseconds(1000));*/
	}
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

/**
 * TAS properties dock widget
 * @author Tobias Weber <tweber@ill.fr>
 * @date mar-2021
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

#include "TASProperties.h"
#include "../settings_variables.h"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QGroupBox>


// --------------------------------------------------------------------------------
// properties widget
// --------------------------------------------------------------------------------
TASPropertiesWidget::TASPropertiesWidget(QWidget *parent)
	: QWidget{parent}
{
	m_spinMonoScAngle = new QDoubleSpinBox(this);
	m_spinSampleScAngle = new QDoubleSpinBox(this);
	m_spinAnaScAngle = new QDoubleSpinBox(this);

	m_spinMonoXtalAngle = new QDoubleSpinBox(this);
	m_spinSampleXtalAngle = new QDoubleSpinBox(this);
	m_spinAnaXtalAngle = new QDoubleSpinBox(this);

	m_spinMonoD = new QDoubleSpinBox(this);
	m_spinAnaD = new QDoubleSpinBox(this);

	QPushButton *btnTarget = new QPushButton("Set Current Angles as Target", this);
	btnTarget->setToolTip("Set the current instrument position as the target position for pathfinding.");

	m_checkScatteringSense[0] = new QCheckBox(this);
	m_checkScatteringSense[1] = new QCheckBox(this);
	m_checkScatteringSense[2] = new QCheckBox(this);

	m_checkScatteringSense[0]->setText("Mono.");
	m_checkScatteringSense[1]->setText("Sample");
	m_checkScatteringSense[2]->setText("Analyser");
	m_checkScatteringSense[0]->setToolTip("Move the monochromator scattering angle in the counterclockwise direction.");
	m_checkScatteringSense[1]->setToolTip("Move the sample scattering angle in the counterclockwise direction.");
	m_checkScatteringSense[2]->setToolTip("Move the analyser scattering angle in the counterclockwise direction.");

	m_checkScatteringSense[0]->setChecked(true);
	m_checkScatteringSense[1]->setChecked(false);
	m_checkScatteringSense[2]->setChecked(true);

	for(QDoubleSpinBox *spin : {m_spinMonoScAngle, m_spinSampleScAngle, m_spinAnaScAngle})
	{
		spin->setMinimum(-180);
		spin->setMaximum(180);
		spin->setDecimals(g_prec_gui);
		spin->setValue(90);
		spin->setSingleStep(1);
		spin->setSuffix("°");
	}

	for(QDoubleSpinBox *spin : {m_spinMonoXtalAngle, m_spinSampleXtalAngle, m_spinAnaXtalAngle})
	{
		spin->setMinimum(-360);
		spin->setMaximum(360);
		spin->setDecimals(g_prec_gui);
		spin->setValue(90);
		spin->setSingleStep(1);
		spin->setSuffix("°");
	}

	for(QDoubleSpinBox *spin : {m_spinMonoD, m_spinAnaD})
	{
		spin->setMinimum(0);
		spin->setMaximum(999);
		spin->setDecimals(g_prec_gui);
		spin->setValue(3.355);
		spin->setSingleStep(0.1);
		spin->setSuffix(" Å");
	}

	auto *groupScatterAngles = new QGroupBox("Scattering Angles", this);
	{
		auto *layoutScatterAngles = new QGridLayout(groupScatterAngles);
		layoutScatterAngles->setHorizontalSpacing(2);
		layoutScatterAngles->setVerticalSpacing(2);
		layoutScatterAngles->setContentsMargins(4,4,4,4);

		int y = 0;
		layoutScatterAngles->addWidget(new QLabel("Monochromator:", this), y, 0, 1, 1);
		layoutScatterAngles->addWidget(m_spinMonoScAngle, y++, 1, 1, 1);
		layoutScatterAngles->addWidget(new QLabel("Sample:", this), y, 0, 1, 1);
		layoutScatterAngles->addWidget(m_spinSampleScAngle, y++, 1, 1, 1);
		layoutScatterAngles->addWidget(new QLabel("Analyser:", this), y, 0, 1, 1);
		layoutScatterAngles->addWidget(m_spinAnaScAngle, y++, 1, 1, 1);
	}

	auto *groupXtalAngles = new QGroupBox("Crystal Angles", this);
	{
		auto *layoutXtalAngles = new QGridLayout(groupXtalAngles);
		layoutXtalAngles->setHorizontalSpacing(2);
		layoutXtalAngles->setVerticalSpacing(2);
		layoutXtalAngles->setContentsMargins(4,4,4,4);

		int y = 0;
		layoutXtalAngles->addWidget(new QLabel("Monochromator:", this), y, 0, 1, 1);
		layoutXtalAngles->addWidget(m_spinMonoXtalAngle, y++, 1, 1, 1);
		layoutXtalAngles->addWidget(new QLabel("Sample:", this), y, 0, 1, 1);
		layoutXtalAngles->addWidget(m_spinSampleXtalAngle, y++, 1, 1, 1);
		layoutXtalAngles->addWidget(new QLabel("Analyser:", this), y, 0, 1, 1);
		layoutXtalAngles->addWidget(m_spinAnaXtalAngle, y++, 1, 1, 1);
	}

	auto *groupD = new QGroupBox("d Spacings", this);
	{
		auto *layoutScatter = new QGridLayout(groupD);
		layoutScatter->setHorizontalSpacing(2);
		layoutScatter->setVerticalSpacing(2);
		layoutScatter->setContentsMargins(4,4,4,4);

		int y = 0;
		layoutScatter->addWidget(new QLabel("Monochromator:", this), y, 0, 1, 1);
		layoutScatter->addWidget(m_spinMonoD, y++, 1, 1, 1);
		layoutScatter->addWidget(new QLabel("Analyser:", this), y, 0, 1, 1);
		layoutScatter->addWidget(m_spinAnaD, y++, 1, 1, 1);
	}

	auto *groupSenses = new QGroupBox("Scattering Senses", this);
	{
		auto *layoutScatter = new QGridLayout(groupSenses);
		layoutScatter->setHorizontalSpacing(2);
		layoutScatter->setVerticalSpacing(2);
		layoutScatter->setContentsMargins(4,4,4,4);

		int x = 0;
		int y = 0;
		for(int comp=0; comp<3; ++comp)
			layoutScatter->addWidget(
				m_checkScatteringSense[comp], y, x++, 1, 1);
	}

	/*auto *groupOptions = new QGroupBox("Options", this);
	{
		auto *layoutScatter = new QGridLayout(groupOptions);
		layoutScatter->setHorizontalSpacing(2);
		layoutScatter->setVerticalSpacing(2);
		layoutScatter->setContentsMargins(4,4,4,4);

		int y = 0;
		layoutScatter->addWidget(btnTarget, y++, 0, 1, 1);
	}*/

	auto *grid = new QGridLayout(this);
	grid->setHorizontalSpacing(2);
	grid->setVerticalSpacing(2);
	grid->setContentsMargins(4,4,4,4);

	int y = 0;
	grid->addWidget(groupScatterAngles, y++, 0, 1, 1);
	grid->addWidget(groupXtalAngles, y++, 0, 1, 1);
	grid->addWidget(groupD, y++, 0, 1, 1);
	grid->addWidget(groupSenses, y++, 0, 1, 1);
	//grid->addWidget(groupOptions, y++, 0, 1, 1);
	grid->addWidget(btnTarget, y++, 0, 1, 1);
	grid->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding), y++, 0, 1, 1);

	connect(m_spinMonoScAngle,
		static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		this, &TASPropertiesWidget::MonoScatteringAngleChanged);
	connect(m_spinSampleScAngle,
		static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		this, &TASPropertiesWidget::SampleScatteringAngleChanged);
	connect(m_spinAnaScAngle,
		static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		this, &TASPropertiesWidget::AnaScatteringAngleChanged);

	connect(m_spinMonoXtalAngle,
		static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		this, &TASPropertiesWidget::MonoCrystalAngleChanged);
	connect(m_spinSampleXtalAngle,
		static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		this, &TASPropertiesWidget::SampleCrystalAngleChanged);
	connect(m_spinAnaXtalAngle,
		static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		this, &TASPropertiesWidget::AnaCrystalAngleChanged);

	// d spacings
	connect(m_spinMonoD,
		static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		[this](t_real dmono) -> void
		{
			t_real dana = m_spinAnaD->value();
			emit DSpacingsChanged(dmono, dana);
		});
	connect(m_spinAnaD,
		static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		[this](t_real dana) -> void
		{
			t_real dmono = m_spinMonoD->value();
			emit DSpacingsChanged(dmono, dana);
		});

	// scattering senses
	for(std::size_t i=0; i<3; ++i)
	{
		connect(m_checkScatteringSense[i], &QCheckBox::stateChanged,
			[this, i](int state) -> void
			{
				bool senses[3];
				for(std::size_t j=0; j<3; ++j)
				{
					if(i == j)
						senses[j] = (state == Qt::Checked);
					else
						senses[j] = m_checkScatteringSense[j]->isChecked();
				}
				emit ScatteringSensesChanged(senses[0], senses[1], senses[2]);
			});
	}

	// set current angles as target angles
	connect(btnTarget, &QPushButton::clicked,
		[this]() -> void
		{
			t_real a1 = m_spinMonoXtalAngle->value();
			t_real a2 = m_spinMonoScAngle->value();
			t_real a3 = m_spinSampleXtalAngle->value();
			t_real a4 = m_spinSampleScAngle->value();
			t_real a5 = m_spinAnaXtalAngle->value();
			t_real a6 = m_spinAnaScAngle->value();

			emit GotoAngles(a1, a2, a3, a4, a5, a6, true);
		});
}


TASPropertiesWidget::~TASPropertiesWidget()
{
}


void TASPropertiesWidget::SetMonoScatteringAngle(t_real angle)
{
	m_spinMonoScAngle->setValue(angle);
}

void TASPropertiesWidget::SetSampleScatteringAngle(t_real angle)
{
	m_spinSampleScAngle->setValue(angle);
}

void TASPropertiesWidget::SetAnaScatteringAngle(t_real angle)
{
	m_spinAnaScAngle->setValue(angle);
}


void TASPropertiesWidget::SetMonoCrystalAngle(t_real angle)
{
	m_spinMonoXtalAngle->setValue(angle);
}

void TASPropertiesWidget::SetSampleCrystalAngle(t_real angle)
{
	m_spinSampleXtalAngle->setValue(angle);
}

void TASPropertiesWidget::SetAnaCrystalAngle(t_real angle)
{
	m_spinAnaXtalAngle->setValue(angle);
}


/**
 * set all angles
 */
void TASPropertiesWidget::SetAngles(t_real a1, t_real a2, t_real a3, t_real a4, t_real a5, t_real a6)
{
	SetMonoScatteringAngle(a2);
	SetSampleScatteringAngle(a4);
	SetAnaScatteringAngle(a6);

	SetMonoCrystalAngle(a1);
	SetSampleCrystalAngle(a3);
	SetAnaCrystalAngle(a5);
}


t_real TASPropertiesWidget::GetMonoScatteringAngle() const
{
	return m_spinMonoScAngle->value();
}

t_real TASPropertiesWidget::GetSampleScatteringAngle() const
{
	return m_spinSampleScAngle->value();
}

t_real TASPropertiesWidget::GetAnaScatteringAngle() const
{
	return m_spinAnaScAngle->value();
}

t_real TASPropertiesWidget::GetMonoCrystalAngle() const
{
	return m_spinMonoXtalAngle->value();
}

t_real TASPropertiesWidget::GetSampleCrystalAngle() const
{
	return m_spinSampleXtalAngle->value();
}

t_real TASPropertiesWidget::GetAnaCrystalAngle() const
{
	return m_spinAnaXtalAngle->value();
}


void TASPropertiesWidget::SetDSpacings(t_real dmono, t_real dana)
{
	m_spinMonoD->setValue(dmono);
	m_spinAnaD->setValue(dana);
}


void TASPropertiesWidget::SetScatteringSenses(bool monoccw, bool sampleccw, bool anaccw)
{
	m_checkScatteringSense[0]->setChecked(monoccw);
	m_checkScatteringSense[1]->setChecked(sampleccw);
	m_checkScatteringSense[2]->setChecked(anaccw);
}


/**
 * save the dock widget's settings
 */
boost::property_tree::ptree TASPropertiesWidget::Save() const
{
	boost::property_tree::ptree prop;

	// scattering angles
	prop.put<t_real>("2thM", m_spinMonoScAngle->value());
	prop.put<t_real>("2thS", m_spinSampleScAngle->value());
	prop.put<t_real>("2thA", m_spinAnaScAngle->value());

	// crystal angles
	prop.put<t_real>("thM", m_spinMonoXtalAngle->value());
	prop.put<t_real>("thS", m_spinSampleXtalAngle->value());
	prop.put<t_real>("thA", m_spinAnaXtalAngle->value());

	// scattering senses
	prop.put<int>("sense_mono", m_checkScatteringSense[0]->isChecked());
	prop.put<int>("sense_sample", m_checkScatteringSense[1]->isChecked());
	prop.put<int>("sense_ana", m_checkScatteringSense[2]->isChecked());

	// d spacings
	prop.put<t_real>("dM", m_spinMonoD->value());
	prop.put<t_real>("dA", m_spinAnaD->value());

	return prop;
}


/**
 * load the dock widget's settings
 */
bool TASPropertiesWidget::Load(const boost::property_tree::ptree& prop)
{
	// old scattering angles
	t_real _2thM = m_spinMonoScAngle->value();
	t_real _2thS = m_spinSampleScAngle->value();
	t_real _2thA = m_spinAnaScAngle->value();

	// old crystal angles
	t_real _thM = m_spinMonoXtalAngle->value();
	t_real _thS = m_spinSampleXtalAngle->value();
	t_real _thA = m_spinAnaXtalAngle->value();

	// old scattering senses
	bool scM = m_checkScatteringSense[0]->isChecked();
	bool scS = m_checkScatteringSense[1]->isChecked();
	bool scA = m_checkScatteringSense[2]->isChecked();

	// old d spacings
	t_real dM = m_spinMonoD->value();
	t_real dA = m_spinAnaD->value();

	// scattering angles
	if(auto opt = prop.get_optional<t_real>("2thM"); opt)
		_2thM = *opt;
	if(auto opt = prop.get_optional<t_real>("2thS"); opt)
		_2thS = *opt;
	if(auto opt = prop.get_optional<t_real>("2thA"); opt)
		_2thA = *opt;

	// crystal angles
	if(auto opt = prop.get_optional<t_real>("thM"); opt)
		_thM = *opt;
	if(auto opt = prop.get_optional<t_real>("thS"); opt)
		_thS = *opt;
	if(auto opt = prop.get_optional<t_real>("thA"); opt)
		_thA = *opt;

	// scattering senses
	if(auto opt = prop.get_optional<int>("sense_mono"); opt)
		scM = (*opt != 0);
	if(auto opt = prop.get_optional<int>("sense_sample"); opt)
		scS = (*opt != 0);
	if(auto opt = prop.get_optional<int>("sense_ana"); opt)
		scA = (*opt != 0);

	// d spacings
	if(auto opt = prop.get_optional<t_real>("dM"); opt)
		dM = *opt;
	if(auto opt = prop.get_optional<t_real>("dA"); opt)
		dA = *opt;

	// set new values
	SetAngles(_thM, _2thM, _thS, _2thS, _thA, _2thA);
	SetScatteringSenses(scM, scS, scA);
	SetDSpacings(dM, dA);

	// emit changes
	//emit AnglesChanged(_thM, _2thM, _thS, _2thS, _thA, _2thA);
	emit MonoScatteringAngleChanged(_2thM);
	emit SampleScatteringAngleChanged(_2thS);
	emit AnaScatteringAngleChanged(_2thA);
	emit MonoCrystalAngleChanged(_thM);
	emit SampleCrystalAngleChanged(_thS);
	emit AnaCrystalAngleChanged(_thA);
	emit ScatteringSensesChanged(scM, scS, scA);
	emit DSpacingsChanged(dM, dA);

	return true;
}
// --------------------------------------------------------------------------------



// --------------------------------------------------------------------------------
// properties dock widget
// --------------------------------------------------------------------------------
TASPropertiesDockWidget::TASPropertiesDockWidget(QWidget *parent)
	: QDockWidget{parent},
		m_widget{std::make_shared<TASPropertiesWidget>(this)}
{
	setObjectName("TASPropertiesDockWidget");
	setWindowTitle("Instrument Properties");

	setWidget(m_widget.get());
}


TASPropertiesDockWidget::~TASPropertiesDockWidget()
{
}
// --------------------------------------------------------------------------------

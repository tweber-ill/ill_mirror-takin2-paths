/**
 * xtal position properties dock widget
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

#include "CoordProperties.h"
#include "../settings_variables.h"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QPushButton>


// --------------------------------------------------------------------------------
// properties widget
// --------------------------------------------------------------------------------
CoordPropertiesWidget::CoordPropertiesWidget(QWidget *parent)
	: QWidget{parent}
{
	for(std::size_t i=0; i<m_num_coord_elems; ++i)
	{
		m_spinCoords[i] = new QDoubleSpinBox(this);

		m_spinCoords[i]->setMinimum(-999);
		m_spinCoords[i]->setMaximum(999);
		m_spinCoords[i]->setSingleStep(0.1);
		m_spinCoords[i]->setDecimals(g_prec_gui);
		m_spinCoords[i]->setValue(0);

		if(i==3 || i==4)
			m_spinCoords[i]->setSuffix(" Å⁻¹");
		else if(i==5)
			m_spinCoords[i]->setSuffix(" meV");
		else
			m_spinCoords[i]->setSuffix(" rlu");
	}

	for(std::size_t i=1; i<m_num_coord_elems; ++i)
		QWidget::setTabOrder(m_spinCoords[i-1], m_spinCoords[i]);


	m_checkKfFixed = new QCheckBox(this);
	m_checkKfFixed->setText("Keep kf = const.");
	m_checkKfFixed->setToolTip("Select the energy transfer by keeping either ki or kf fixed.");

	// default values
	m_spinCoords[0]->setValue(1);
	m_spinCoords[3]->setValue(1.4);
	m_spinCoords[4]->setValue(1.4);
	m_spinCoords[5]->setValue(0);
	m_checkKfFixed->setChecked(true);

	QPushButton *btnGoto = new QPushButton("Jump to Coordinates", this);
	QPushButton *btnTarget = new QPushButton("Set Target Angles", this);

	const char* labels[] = {
		"Momentum (h):", 
		"Momentum (k):", 
		"Momentum (l):", 
		"Initial k (ki):", 
		"Final k (kf):", 
		"Energy (E):"
	};

	auto *groupCoords = new QGroupBox("Crystal Coordinates", this);
	{
		auto *layoutStart = new QGridLayout(groupCoords);
		layoutStart->setHorizontalSpacing(2);
		layoutStart->setVerticalSpacing(2);
		layoutStart->setContentsMargins(4,4,4,4);

		int y = 0;
		for(std::size_t i=0; i<m_num_coord_elems; ++i)
		{
			layoutStart->addWidget(new QLabel(labels[i], this), y, 0, 1, 1);
			layoutStart->addWidget(m_spinCoords[i], y++, 1, 1, 1);
		}

		layoutStart->addWidget(m_checkKfFixed, y++, 0, 1, 2);
		layoutStart->addWidget(btnGoto, y++, 0, 1, 2);
		layoutStart->addWidget(btnTarget, y++, 0, 1, 2);
	}

	auto *grid = new QGridLayout(this);
	grid->setHorizontalSpacing(2);
	grid->setVerticalSpacing(2);
	grid->setContentsMargins(4,4,4,4);

	int y = 0;
	grid->addWidget(groupCoords, y++, 0, 1, 1);
	grid->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding), y++, 0, 1, 1);

	for(std::size_t i=0; i<m_num_coord_elems-1; ++i)
	{
		// coordinates
		connect(m_spinCoords[i],
			static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			[this, i](t_real val) -> void
			{
				t_real coords[m_num_coord_elems-1];
				for(std::size_t j=0; j<m_num_coord_elems-1; ++j)
				{
					if(j == i)
						coords[j] = val;
					else
						coords[j] = m_spinCoords[j]->value();
				}

				t_real E = tl2::calc_tas_E<t_real>(coords[3], coords[4]);
				m_spinCoords[5]->blockSignals(true);
				m_spinCoords[5]->setValue(E);
				m_spinCoords[5]->blockSignals(false);

				emit CoordinatesChanged(coords[0], coords[1], coords[2], coords[3], coords[4]);
			});
	}

	// energy
	connect(m_spinCoords[5],
		static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		[this](t_real E) -> void
		{
			if(m_checkKfFixed->isChecked())
			{
				t_real kf = m_spinCoords[4]->value();
				t_real ki = tl2::calc_tas_ki<t_real>(kf, E);

				// set ki
				m_spinCoords[3]->blockSignals(true);
				m_spinCoords[3]->setValue(ki);
				m_spinCoords[3]->blockSignals(false);
			}
			else
			{
				t_real ki = m_spinCoords[3]->value();
				t_real kf = tl2::calc_tas_kf<t_real>(ki, E);

				// set kf
				m_spinCoords[4]->blockSignals(true);
				m_spinCoords[4]->setValue(kf);
				m_spinCoords[4]->blockSignals(false);
			}
		});

	// go to crystal coordinates
	connect(btnGoto, &QPushButton::clicked,
		[this]() -> void
		{
			t_real h = m_spinCoords[0]->value();
			t_real k = m_spinCoords[1]->value();
			t_real l = m_spinCoords[2]->value();
			t_real ki = m_spinCoords[3]->value();
			t_real kf = m_spinCoords[4]->value();

			emit GotoCoordinates(h, k, l, ki, kf, false);
		});

	// only set target angles
	connect(btnTarget, &QPushButton::clicked,
		[this]() -> void
		{
			t_real h = m_spinCoords[0]->value();
			t_real k = m_spinCoords[1]->value();
			t_real l = m_spinCoords[2]->value();
			t_real ki = m_spinCoords[3]->value();
			t_real kf = m_spinCoords[4]->value();

			emit GotoCoordinates(h, k, l, ki, kf, true);
		});

	// kf == const?
	// TODO: maybe move this to TASProperties
	connect(m_checkKfFixed, &QCheckBox::stateChanged,
		[this](int state) -> void
		{
			emit KfConstModeChanged(state == Qt::Checked);
		});
}


CoordPropertiesWidget::~CoordPropertiesWidget()
{
}


void CoordPropertiesWidget::SetCoordinates(
	t_real h, t_real k, t_real l, t_real ki, t_real kf)
{
	this->blockSignals(true);

	m_spinCoords[0]->setValue(h);
	m_spinCoords[1]->setValue(k);
	m_spinCoords[2]->setValue(l);
	m_spinCoords[3]->setValue(ki);
	m_spinCoords[4]->setValue(kf);
	m_spinCoords[5]->setValue(tl2::calc_tas_E<t_real>(ki, kf));

	this->blockSignals(false);
}


void CoordPropertiesWidget::SetKfConstMode(bool kf_const)
{
	this->blockSignals(true);

	m_checkKfFixed->setChecked(kf_const);

	this->blockSignals(false);
}


/**
 * save the dock widget's settings
 */
boost::property_tree::ptree CoordPropertiesWidget::Save() const
{
	boost::property_tree::ptree prop;

	// crystal coordinates
	prop.put<t_real>("h", m_spinCoords[0]->value());
	prop.put<t_real>("k", m_spinCoords[1]->value());
	prop.put<t_real>("l", m_spinCoords[2]->value());
	prop.put<t_real>("ki", m_spinCoords[3]->value());
	prop.put<t_real>("kf", m_spinCoords[4]->value());
	prop.put<t_real>("E", m_spinCoords[5]->value());

	// fixed wave vector
	prop.put<int>("kf_fixed", m_checkKfFixed->isChecked());

	return prop;
}


/**
 * load the dock widget's settings
 */
bool CoordPropertiesWidget::Load(const boost::property_tree::ptree& prop)
{
	// get current coordinate values
	t_real h = m_spinCoords[0]->value();
	t_real k = m_spinCoords[1]->value();
	t_real l = m_spinCoords[2]->value();
	t_real ki = m_spinCoords[3]->value();
	t_real kf = m_spinCoords[4]->value();

	bool kf_fixed = true;

	// new coordinates
	if(auto opt = prop.get_optional<t_real>("h"); opt)
		h = *opt;
	if(auto opt = prop.get_optional<t_real>("k"); opt)
		k = *opt;
	if(auto opt = prop.get_optional<t_real>("l"); opt)
		l = *opt;
	if(auto opt = prop.get_optional<t_real>("ki"); opt)
		ki = *opt;
	if(auto opt = prop.get_optional<t_real>("kf"); opt)
		kf = *opt;

	// fixed wave vector
	if(auto opt = prop.get_optional<int>("kf_fixed"); opt)
		m_checkKfFixed->setChecked(*opt != 0);

	// set new coordinates
	SetCoordinates(h, k, l, ki, kf);

	// set kf=const mode
	SetKfConstMode(kf_fixed);

	// set kf=const mode
	emit KfConstModeChanged(kf_fixed);

	// emit changes
	emit CoordinatesChanged(h, k, l, ki, kf);

	return true;
}
// --------------------------------------------------------------------------------



// --------------------------------------------------------------------------------
// properties dock widget
// --------------------------------------------------------------------------------
CoordPropertiesDockWidget::CoordPropertiesDockWidget(QWidget *parent)
	: QDockWidget{parent},
		m_widget{std::make_shared<CoordPropertiesWidget>(this)}
{
	setObjectName("CoordPropertiesDockWidget");
	setWindowTitle("Crystal Coordinates");

	setWidget(m_widget.get());
}


CoordPropertiesDockWidget::~CoordPropertiesDockWidget()
{
}
// --------------------------------------------------------------------------------

/**
 * xtal position properties dock widget
 * @author Tobias Weber <tweber@ill.fr>
 * @date apr-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "CoordProperties.h"
#include "../Settings.h"

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

	// default values
	m_spinCoords[0]->setValue(1);
	m_spinCoords[3]->setValue(1.4);
	m_spinCoords[4]->setValue(1.4);
	m_spinCoords[5]->setValue(0);
	m_checkKfFixed->setChecked(true);

	QPushButton *btnGoto = new QPushButton("Go to Coordinates", this);
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

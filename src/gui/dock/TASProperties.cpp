/**
 * TAS properties dock widget
 * @author Tobias Weber <tweber@ill.fr>
 * @date mar-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "TASProperties.h"
#include "../Settings.h"

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

	m_checkScatteringSense[0] = new QCheckBox(this);
	m_checkScatteringSense[1] = new QCheckBox(this);
	m_checkScatteringSense[2] = new QCheckBox(this);

	m_checkScatteringSense[0]->setText("Monochromator ccw");
	m_checkScatteringSense[1]->setText("Sample ccw");
	m_checkScatteringSense[2]->setText("Analyser ccw");

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

		int y = 0;
		layoutScatter->addWidget(m_checkScatteringSense[0], y++, 0, 1, 2);
		layoutScatter->addWidget(m_checkScatteringSense[1], y++, 0, 1, 2);
		layoutScatter->addWidget(m_checkScatteringSense[2], y++, 0, 1, 2);
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

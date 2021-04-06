/**
 * TAS properties dock widget
 * @author Tobias Weber <tweber@ill.fr>
 * @date mar-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "TASProperties.h"

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

	for(QDoubleSpinBox *spin : {m_spinMonoScAngle, m_spinSampleScAngle, m_spinAnaScAngle})
	{
		spin->setMinimum(-180);
		spin->setMaximum(180);
		spin->setDecimals(3);
	}

	for(QDoubleSpinBox *spin : {m_spinMonoXtalAngle, m_spinSampleXtalAngle, m_spinAnaXtalAngle})
	{
		spin->setMinimum(-360);
		spin->setMaximum(360);
		spin->setDecimals(3);
	}

	auto *groupScatterAngles = new QGroupBox("Scattering Angles", this);
	{
		auto *layoutScatterAngles = new QGridLayout(groupScatterAngles);
		layoutScatterAngles->setHorizontalSpacing(4);
		layoutScatterAngles->setVerticalSpacing(4);
		layoutScatterAngles->setContentsMargins(6,6,6,6);

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
		layoutXtalAngles->setHorizontalSpacing(4);
		layoutXtalAngles->setVerticalSpacing(4);
		layoutXtalAngles->setContentsMargins(6,6,6,6);

		int y = 0;
		layoutXtalAngles->addWidget(new QLabel("Monochromator:", this), y, 0, 1, 1);
		layoutXtalAngles->addWidget(m_spinMonoXtalAngle, y++, 1, 1, 1);
		layoutXtalAngles->addWidget(new QLabel("Sample:", this), y, 0, 1, 1);
		layoutXtalAngles->addWidget(m_spinSampleXtalAngle, y++, 1, 1, 1);
		layoutXtalAngles->addWidget(new QLabel("Analyser:", this), y, 0, 1, 1);
		layoutXtalAngles->addWidget(m_spinAnaXtalAngle, y++, 1, 1, 1);
	}

	auto *grid = new QGridLayout(this);
	grid->setHorizontalSpacing(4);
	grid->setVerticalSpacing(4);
	grid->setContentsMargins(6,6,6,6);

	int y = 0;
	grid->addWidget(groupScatterAngles, y++, 0, 1, 1);
	grid->addWidget(groupXtalAngles, y++, 0, 1, 1);
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

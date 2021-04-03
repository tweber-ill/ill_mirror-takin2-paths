/**
 * perperties dock widget
 * @author Tobias Weber <tweber@ill.fr>
 * @date mar-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "Properties.h"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>


// --------------------------------------------------------------------------------
// properties widget
// --------------------------------------------------------------------------------
TASPropertiesWidget::TASPropertiesWidget(QWidget *parent)
	: QWidget{parent}
{
	m_spinMonoScAngle = new QDoubleSpinBox(this);
	m_spinSampleScAngle = new QDoubleSpinBox(this);
	m_spinAnaScAngle = new QDoubleSpinBox(this);

	for(QDoubleSpinBox *spin : {m_spinMonoScAngle, m_spinSampleScAngle, m_spinAnaScAngle})
	{
		spin->setMinimum(-180);
		spin->setMaximum(180);
		spin->setDecimals(2);
	}

	auto *grid = new QGridLayout(this);
	int y = 0;
	grid->addWidget(new QLabel("Mono. Sc. Angle:", this), y, 0, 1, 1);
	grid->addWidget(m_spinMonoScAngle, y++, 1, 1, 1);
	grid->addWidget(new QLabel("Sample. Sc. Angle:", this), y, 0, 1, 1);
	grid->addWidget(m_spinSampleScAngle, y++, 1, 1, 1);
	grid->addWidget(new QLabel("Ana. Sc. Angle:", this), y, 0, 1, 1);
	grid->addWidget(m_spinAnaScAngle, y++, 1, 1, 1);

	grid->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding), y++, 0, 1, 2);

	connect(m_spinMonoScAngle, 
		static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		this, &TASPropertiesWidget::MonoScatteringAngleChanged);
	connect(m_spinSampleScAngle, 
		static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		this, &TASPropertiesWidget::SampleScatteringAngleChanged);
	connect(m_spinAnaScAngle, 
		static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		this, &TASPropertiesWidget::AnaScatteringAngleChanged);
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


TASPropertiesWidget::~TASPropertiesWidget()
{
}
// --------------------------------------------------------------------------------



// --------------------------------------------------------------------------------
// properties dock widget
// --------------------------------------------------------------------------------
TASPropertiesDockWidget::TASPropertiesDockWidget(QWidget *parent)
	: QDockWidget{parent}, 
		m_widget{std::make_shared<TASPropertiesWidget>(this)}
{
	setObjectName("PropertiesDockWidget");
	setWindowTitle("Instrument Properties");

	setWidget(m_widget.get());
}


TASPropertiesDockWidget::~TASPropertiesDockWidget()
{
}
// --------------------------------------------------------------------------------

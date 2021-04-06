/**
 * camera properties dock widget
 * @author Tobias Weber <tweber@ill.fr>
 * @date apr-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "CamProperties.h"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QGroupBox>


// --------------------------------------------------------------------------------
// properties widget
// --------------------------------------------------------------------------------
CamPropertiesWidget::CamPropertiesWidget(QWidget *parent)
	: QWidget{parent}
{
	m_spinViewingAngle = new QDoubleSpinBox(this);
	m_spinViewingAngle->setMinimum(1);
	m_spinViewingAngle->setMaximum(179);

	m_checkPerspectiveProj = new QCheckBox(this);
	m_checkPerspectiveProj->setText("Perspective Projection");
	m_checkPerspectiveProj->setChecked(true);

	for(int pos=0; pos<3; ++pos)
	{
		m_spinPos[pos] = new QDoubleSpinBox(this);
		m_spinPos[pos]->setMinimum(-100);
		m_spinPos[pos]->setMaximum(+100);
		m_spinPos[pos]->setDecimals(3);
	}

	for(int rot=0; rot<2; ++rot)
	{
		m_spinRot[rot] = new QDoubleSpinBox(this);
		m_spinRot[rot]->setMinimum(-180);
		m_spinRot[rot]->setMaximum(+180);
		m_spinRot[rot]->setDecimals(2);
	}

	auto *groupProj = new QGroupBox("Projection", this);
	{
		auto *layoutProj = new QGridLayout(groupProj);
		layoutProj->setHorizontalSpacing(4);
		layoutProj->setVerticalSpacing(4);
		layoutProj->setContentsMargins(6,6,6,6);

		int y = 0;
		layoutProj->addWidget(new QLabel("Viewing Angle:", this), y, 0, 1, 1);
		layoutProj->addWidget(m_spinViewingAngle, y++, 1, 1, 1);
		layoutProj->addWidget(m_checkPerspectiveProj, y++, 0, 1, 2);
	}

	auto *groupVecs = new QGroupBox("Vectors", this);
	{
		auto *layoutVecs = new QGridLayout(groupVecs);
		layoutVecs->setHorizontalSpacing(4);
		layoutVecs->setVerticalSpacing(4);
		layoutVecs->setContentsMargins(6,6,6,6);

		int y = 0;
		layoutVecs->addWidget(new QLabel("Position x:", this), y, 0, 1, 1);
		layoutVecs->addWidget(m_spinPos[0], y++, 1, 1, 1);
		layoutVecs->addWidget(new QLabel("Position y:", this), y, 0, 1, 1);
		layoutVecs->addWidget(m_spinPos[1], y++, 1, 1, 1);
		layoutVecs->addWidget(new QLabel("Position z:", this), y, 0, 1, 1);
		layoutVecs->addWidget(m_spinPos[2], y++, 1, 1, 1);

		layoutVecs->addWidget(new QLabel("Rotation φ:", this), y, 0, 1, 1);
		layoutVecs->addWidget(m_spinRot[0], y++, 1, 1, 1);
		layoutVecs->addWidget(new QLabel("Rotation θ:", this), y, 0, 1, 1);
		layoutVecs->addWidget(m_spinRot[1], y++, 1, 1, 1);
	}

	auto *grid = new QGridLayout(this);
	grid->setHorizontalSpacing(4);
	grid->setVerticalSpacing(4);
	grid->setContentsMargins(6,6,6,6);

	int y = 0;
	grid->addWidget(groupProj, y++, 0, 1, 1);
	grid->addWidget(groupVecs, y++, 0, 1, 1);
	grid->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding), y++, 0, 1, 1);

	// viewing angle
	connect(m_spinViewingAngle, 
		static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		this, &CamPropertiesWidget::ViewingAngleChanged);

	// perspective projection flag
	connect(m_checkPerspectiveProj, &QCheckBox::stateChanged,
		[this](int state) -> void
		{
			emit PerspectiveProjChanged(state == Qt::Checked);
		});

	// position
	connect(m_spinPos[0], 
		static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		[this](t_real x) -> void
		{
			t_real y = m_spinPos[1]->value();
			t_real z = m_spinPos[2]->value();

			emit CamPositionChanged(x, y, z);
		});
	connect(m_spinPos[1], 
		static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		[this](t_real y) -> void
		{
			t_real x = m_spinPos[0]->value();
			t_real z = m_spinPos[2]->value();

			emit CamPositionChanged(x, y, z);
		});
	connect(m_spinPos[2], 
		static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		[this](t_real z) -> void
		{
			t_real x = m_spinPos[0]->value();
			t_real y = m_spinPos[1]->value();

			emit CamPositionChanged(x, y, z);
		});

	// rotation
	connect(m_spinRot[0], 
		static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		[this](t_real phi) -> void
		{
			t_real theta = m_spinRot[1]->value();
			emit CamRotationChanged(phi, theta);
		});
	connect(m_spinRot[1], 
		static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		[this](t_real theta) -> void
		{
			t_real phi = m_spinRot[0]->value();
			emit CamRotationChanged(phi, theta);
		});
}


CamPropertiesWidget::~CamPropertiesWidget()
{
}


void CamPropertiesWidget::SetViewingAngle(t_real angle)
{
	m_spinViewingAngle->setValue(angle);
}

void CamPropertiesWidget::SetPerspectiveProj(bool proj)
{
	m_checkPerspectiveProj->setChecked(proj);
}

void CamPropertiesWidget::SetCamPosition(t_real x, t_real y, t_real z)
{
	this->blockSignals(true);
	if(m_spinPos[0]) m_spinPos[0]->setValue(x);
	if(m_spinPos[1]) m_spinPos[1]->setValue(y);
	if(m_spinPos[2]) m_spinPos[2]->setValue(z);
	this->blockSignals(false);
}

void CamPropertiesWidget::SetCamRotation(t_real phi, t_real theta)
{
	this->blockSignals(true);
	if(m_spinRot[0]) m_spinRot[0]->setValue(phi);
	if(m_spinRot[1]) m_spinRot[1]->setValue(theta);
	this->blockSignals(false);
}
// --------------------------------------------------------------------------------



// --------------------------------------------------------------------------------
// properties dock widget
// --------------------------------------------------------------------------------
CamPropertiesDockWidget::CamPropertiesDockWidget(QWidget *parent)
	: QDockWidget{parent}, 
		m_widget{std::make_shared<CamPropertiesWidget>(this)}
{
	setObjectName("CamPropertiesDockWidget");
	setWindowTitle("Camera Properties");

	setWidget(m_widget.get());
}


CamPropertiesDockWidget::~CamPropertiesDockWidget()
{
}
// --------------------------------------------------------------------------------

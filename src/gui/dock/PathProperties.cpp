/**
 * path properties dock widget
 * @author Tobias Weber <tweber@ill.fr>
 * @date apr-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "PathProperties.h"
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

	QPushButton *btnGotoFinish = new QPushButton("Go to Target Angles", this);

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

	auto *grid = new QGridLayout(this);
	grid->setHorizontalSpacing(2);
	grid->setVerticalSpacing(2);
	grid->setContentsMargins(4,4,4,4);

	int y = 0;
	grid->addWidget(groupFinish, y++, 0, 1, 1);
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
}


PathPropertiesWidget::~PathPropertiesWidget()
{
}


void PathPropertiesWidget::SetTarget(t_real a2, t_real a4)
{
	this->blockSignals(true);

	m_spinFinish[0]->setValue(a2);
	m_spinFinish[1]->setValue(a4);

	this->blockSignals(false);
	emit TargetChanged(a2, a4);
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

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
		m_spinStart[i] = new QDoubleSpinBox(this);
		m_spinFinish[i] = new QDoubleSpinBox(this);

		m_spinStart[i]->setMinimum(-180);
		m_spinStart[i]->setMaximum(180);
		m_spinStart[i]->setSingleStep(0.1);
		m_spinStart[i]->setDecimals(g_prec_gui);
		m_spinStart[i]->setValue(0);
		m_spinStart[i]->setSuffix("°");

		m_spinFinish[i]->setMinimum(-180);
		m_spinFinish[i]->setMaximum(180);
		m_spinFinish[i]->setSingleStep(0.1);
		m_spinFinish[i]->setDecimals(g_prec_gui);
		m_spinFinish[i]->setValue(0);
		m_spinFinish[i]->setSuffix("°");
	}

	for(std::size_t i=1; i<m_num_coord_elems; ++i)
		QWidget::setTabOrder(m_spinStart[i-1], m_spinStart[i]);
	for(std::size_t i=1; i<m_num_coord_elems; ++i)
		QWidget::setTabOrder(m_spinFinish[i-1], m_spinFinish[i]);


	// default values
	m_spinStart[0]->setValue(90.);
	m_spinStart[1]->setValue(90.);

	m_spinFinish[0]->setValue(90.);
	m_spinFinish[1]->setValue(90.);

	QPushButton *btnGotoStart = new QPushButton("Go to Start Angles", this);
	QPushButton *btnGotoFinish = new QPushButton("Go to Finish Angles", this);

	const char* labels[] = {"Monochromator:", "Sample:"};

	auto *groupStart = new QGroupBox("Start Scattering Angles", this);
	{
		auto *layoutStart = new QGridLayout(groupStart);
		layoutStart->setHorizontalSpacing(2);
		layoutStart->setVerticalSpacing(2);
		layoutStart->setContentsMargins(4,4,4,4);

		int y = 0;
		for(std::size_t i=0; i<m_num_coord_elems; ++i)
		{
			layoutStart->addWidget(new QLabel(labels[i], this), y, 0, 1, 1);
			layoutStart->addWidget(m_spinStart[i], y++, 1, 1, 1);
		}

		layoutStart->addWidget(btnGotoStart, y++, 0, 1, 2);
	}

	auto *groupFinish = new QGroupBox("Finish Scattering Angles", this);
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
	grid->addWidget(groupStart, y++, 0, 1, 1);
	grid->addWidget(groupFinish, y++, 0, 1, 1);
	grid->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding), y++, 0, 1, 1);

	for(std::size_t i=0; i<m_num_coord_elems-1; ++i)
	{
		// start angles
		connect(m_spinStart[i],
			static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			[this, i](t_real val) -> void
			{
				t_real coords[m_num_coord_elems-1];
				for(std::size_t j=0; j<m_num_coord_elems-1; ++j)
				{
					if(j == i)
						coords[j] = val;
					else
						coords[j] = m_spinStart[j]->value();
				}

				emit StartChanged(coords[0], coords[1]);
			});

		// finish angles
		connect(m_spinFinish[i],
			static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			[this, i](t_real val) -> void
			{
				t_real coords[m_num_coord_elems-1];
				for(std::size_t j=0; j<m_num_coord_elems-1; ++j)
				{
					if(j == i)
						coords[j] = val;
					else
						coords[j] = m_spinFinish[j]->value();
				}

				emit FinishChanged(coords[0], coords[1]);
			});
	}


	// go to start angles
	connect(btnGotoStart, &QPushButton::clicked,
		[this]() -> void
		{
			t_real a2 = m_spinStart[0]->value();
			t_real a4 = m_spinStart[1]->value();

			emit GotoAngles(a2, a4);
		});

	// go to finish angles
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


void PathPropertiesWidget::SetStart(t_real a2, t_real a4)
{
	this->blockSignals(true);

	m_spinStart[0]->setValue(a2);
	m_spinStart[1]->setValue(a4);

	this->blockSignals(false);
}


void PathPropertiesWidget::SetFinish(t_real a2, t_real a4)
{
	this->blockSignals(true);

	m_spinFinish[0]->setValue(a2);
	m_spinFinish[1]->setValue(a4);

	this->blockSignals(false);
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

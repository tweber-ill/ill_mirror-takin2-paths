/**
 * path properties dock widget
 * @author Tobias Weber <tweber@ill.fr>
 * @date apr-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "PathProperties.h"
#include "Settings.h"

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

		m_spinStart[i]->setMinimum(-999);
		m_spinStart[i]->setMaximum(999);
		m_spinStart[i]->setSingleStep(0.1);
		m_spinStart[i]->setDecimals(g_prec_gui);
		m_spinStart[i]->setValue(0);

		if(i==3 || i==4)
			m_spinStart[i]->setSuffix(" Å⁻¹");
		else if(i==5)
			m_spinStart[i]->setSuffix(" meV");
		else
			m_spinStart[i]->setSuffix(" rlu");

		m_spinFinish[i]->setMinimum(-999);
		m_spinFinish[i]->setMaximum(999);
		m_spinFinish[i]->setSingleStep(0.1);
		m_spinFinish[i]->setDecimals(g_prec_gui);
		m_spinFinish[i]->setValue(0);

		if(i==3 || i==4)
			m_spinFinish[i]->setSuffix(" Å⁻¹");
		else if(i==5)
			m_spinFinish[i]->setSuffix(" meV");
		else
			m_spinFinish[i]->setSuffix(" rlu");
	}

	for(std::size_t i=1; i<m_num_coord_elems; ++i)
		QWidget::setTabOrder(m_spinStart[i-1], m_spinStart[i]);
	for(std::size_t i=1; i<m_num_coord_elems; ++i)
		QWidget::setTabOrder(m_spinFinish[i-1], m_spinFinish[i]);


	m_checkStartKfFixed = new QCheckBox(this);
	m_checkFinishKfFixed = new QCheckBox(this);
	m_checkStartKfFixed->setText("Keep kf = const.");
	m_checkFinishKfFixed->setText("Keep kf = const.");

	// default values
	m_spinStart[0]->setValue(1);
	m_spinStart[3]->setValue(1.4);
	m_spinStart[4]->setValue(1.4);
	m_spinStart[5]->setValue(0);
	m_checkStartKfFixed->setChecked(true);

	m_spinFinish[0]->setValue(1);
	m_spinFinish[3]->setValue(1.4);
	m_spinFinish[4]->setValue(1.4);
	m_spinFinish[5]->setValue(0);
	m_checkFinishKfFixed->setChecked(true);

	QPushButton *btnGotoStart = new QPushButton("Go to Start Coordinate", this);
	QPushButton *btnGotoFinish = new QPushButton("Go to Finish Coordinate", this);

	const char* labels[] = {"h:", "k:", "l:", "ki:", "kf:", "E:"};

	auto *groupStart = new QGroupBox("Start Coordinate", this);
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

		layoutStart->addWidget(m_checkStartKfFixed, y++, 0, 1, 2);
		layoutStart->addWidget(btnGotoStart, y++, 0, 1, 2);
	}

	auto *groupFinish = new QGroupBox("Finish Coordinate", this);
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

		layoutFinish->addWidget(m_checkFinishKfFixed, y++, 0, 1, 2);
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
		// start coordinates
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

				t_real E = tl2::calc_tas_E<t_real>(coords[3], coords[4]);
				m_spinStart[5]->blockSignals(true);
				m_spinStart[5]->setValue(E);
				m_spinStart[5]->blockSignals(false);

				emit StartChanged(coords[0], coords[1], coords[2], coords[3], coords[4]);
			});

		// finish coordinates
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

				t_real E = tl2::calc_tas_E<t_real>(coords[3], coords[4]);
				m_spinFinish[5]->blockSignals(true);
				m_spinFinish[5]->setValue(E);
				m_spinFinish[5]->blockSignals(false);

				emit FinishChanged(coords[0], coords[1], coords[2], coords[3], coords[4]);
			});
	}

	// start energy
	connect(m_spinStart[5],
		static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		[this](t_real E) -> void
		{
			if(m_checkStartKfFixed->isChecked())
			{
				t_real kf = m_spinStart[4]->value();
				t_real ki = tl2::calc_tas_ki<t_real>(kf, E);

				// set ki
				m_spinStart[3]->blockSignals(true);
				m_spinStart[3]->setValue(ki);
				m_spinStart[3]->blockSignals(false);
			}
			else
			{
				t_real ki = m_spinStart[3]->value();
				t_real kf = tl2::calc_tas_kf<t_real>(ki, E);

				// set kf
				m_spinStart[4]->blockSignals(true);
				m_spinStart[4]->setValue(kf);
				m_spinStart[4]->blockSignals(false);
			}
		});

	// finish energy
	connect(m_spinFinish[5],
		static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		[this](t_real E) -> void
		{
			if(m_checkFinishKfFixed->isChecked())
			{
				t_real kf = m_spinFinish[4]->value();
				t_real ki = tl2::calc_tas_ki<t_real>(kf, E);

				// set ki
				m_spinFinish[3]->blockSignals(true);
				m_spinFinish[3]->setValue(ki);
				m_spinFinish[3]->blockSignals(false);
			}
			else
			{
				t_real ki = m_spinFinish[3]->value();
				t_real kf = tl2::calc_tas_kf<t_real>(ki, E);

				// set kf
				m_spinFinish[4]->blockSignals(true);
				m_spinFinish[4]->setValue(kf);
				m_spinFinish[4]->blockSignals(false);
			}
		});

	// go to start coordinate
	connect(btnGotoStart, &QPushButton::clicked,
		[this]() -> void
		{
			t_real h = m_spinStart[0]->value();
			t_real k = m_spinStart[1]->value();
			t_real l = m_spinStart[2]->value();
			t_real ki = m_spinStart[3]->value();
			t_real kf = m_spinStart[4]->value();

			emit Goto(h, k, l, ki, kf);
		});

	// go to finish coordinate
	connect(btnGotoFinish, &QPushButton::clicked,
		[this]() -> void
		{
			t_real h = m_spinFinish[0]->value();
			t_real k = m_spinFinish[1]->value();
			t_real l = m_spinFinish[2]->value();
			t_real ki = m_spinFinish[3]->value();
			t_real kf = m_spinFinish[4]->value();

			emit Goto(h, k, l, ki, kf);
		});
}


PathPropertiesWidget::~PathPropertiesWidget()
{
}


void PathPropertiesWidget::SetStart(t_real h, t_real k, t_real l, t_real ki, t_real kf)
{
	this->blockSignals(true);

	m_spinStart[0]->setValue(h);
	m_spinStart[1]->setValue(k);
	m_spinStart[2]->setValue(l);
	m_spinStart[3]->setValue(ki);
	m_spinStart[4]->setValue(kf);
	m_spinStart[5]->setValue(tl2::calc_tas_E<t_real>(ki, kf));

	this->blockSignals(false);
}


void PathPropertiesWidget::SetFinish(t_real h, t_real k, t_real l, t_real ki, t_real kf)
{
	this->blockSignals(true);

	m_spinFinish[0]->setValue(h);
	m_spinFinish[1]->setValue(k);
	m_spinFinish[2]->setValue(l);
	m_spinFinish[3]->setValue(ki);
	m_spinFinish[4]->setValue(kf);
	m_spinFinish[5]->setValue(tl2::calc_tas_E<t_real>(ki, kf));

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

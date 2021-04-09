/**
 * crystal properties dock widget
 * @author Tobias Weber <tweber@ill.fr>
 * @date apr-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "XtalProperties.h"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QFrame>


// --------------------------------------------------------------------------------
// properties widget
// --------------------------------------------------------------------------------
XtalPropertiesWidget::XtalPropertiesWidget(QWidget *parent)
	: QWidget{parent}
{
	for(std::size_t i=0; i<m_num_lattice_elems; ++i)
	{
		m_spinLatticeConsts[i] = new QDoubleSpinBox(this);
		m_spinLatticeAngles[i] = new QDoubleSpinBox(this);

		m_spinLatticeConsts[i]->setMinimum(0);
		m_spinLatticeConsts[i]->setMaximum(999);
		m_spinLatticeConsts[i]->setSingleStep(0.1);
		m_spinLatticeConsts[i]->setDecimals(3);
		m_spinLatticeConsts[i]->setValue(5);
		m_spinLatticeConsts[i]->setSuffix(" Å");

		m_spinLatticeAngles[i]->setMinimum(0);
		m_spinLatticeAngles[i]->setMaximum(180);
		m_spinLatticeAngles[i]->setDecimals(2);
		m_spinLatticeAngles[i]->setValue(90);
		m_spinLatticeAngles[i]->setSuffix("°");
	}

	for(std::size_t i=0; i<m_num_plane_elems; ++i)
	{
		m_spinPlane[i] = new QDoubleSpinBox(this);

		m_spinPlane[i]->setMinimum(-999);
		m_spinPlane[i]->setMaximum(999);
		m_spinPlane[i]->setDecimals(2);
		m_spinPlane[i]->setValue((i==0 || i==4) ? 1 : 0);
		m_spinPlane[i]->setSuffix(" rlu");
	}

	auto *groupLattice = new QGroupBox("Lattice", this);
	{
		auto *layoutLattice = new QGridLayout(groupLattice);
		layoutLattice->setHorizontalSpacing(2);
		layoutLattice->setVerticalSpacing(2);
		layoutLattice->setContentsMargins(4,4,4,4);

		int y = 0;
		layoutLattice->addWidget(new QLabel("Constant a:", this), y, 0, 1, 1);
		layoutLattice->addWidget(m_spinLatticeConsts[0], y++, 1, 1, 1);
		layoutLattice->addWidget(new QLabel("Constant b:", this), y, 0, 1, 1);
		layoutLattice->addWidget(m_spinLatticeConsts[1], y++, 1, 1, 1);
		layoutLattice->addWidget(new QLabel("Constant c:", this), y, 0, 1, 1);
		layoutLattice->addWidget(m_spinLatticeConsts[2], y++, 1, 1, 1);

		QFrame *separator = new QFrame(this);
		separator->setFrameStyle(QFrame::HLine);
		layoutLattice->addWidget(separator, y++, 0, 1, 2);

		layoutLattice->addWidget(new QLabel("Angle α:", this), y, 0, 1, 1);
		layoutLattice->addWidget(m_spinLatticeAngles[0], y++, 1, 1, 1);
		layoutLattice->addWidget(new QLabel("Angle β:", this), y, 0, 1, 1);
		layoutLattice->addWidget(m_spinLatticeAngles[1], y++, 1, 1, 1);
		layoutLattice->addWidget(new QLabel("Angle γ:", this), y, 0, 1, 1);
		layoutLattice->addWidget(m_spinLatticeAngles[2], y++, 1, 1, 1);
	}

	auto *groupPlane = new QGroupBox("Scattering Plane", this);
	{
		auto *layoutPlane = new QGridLayout(groupPlane);
		layoutPlane->setHorizontalSpacing(2);
		layoutPlane->setVerticalSpacing(2);
		layoutPlane->setContentsMargins(4,4,4,4);

		const char* labels[] = {
			"Vector 1, x:", "Vector 1, y:", "Vector 1, z:",
			"Vector 2, x:", "Vector 2, y:", "Vector 2, z:"
		};
		int y = 0;
		for(std::size_t i=0; i<m_num_plane_elems; ++i)
		{
			layoutPlane->addWidget(new QLabel(labels[i], this), y, 0, 1, 1);
			layoutPlane->addWidget(m_spinPlane[i], y++, 1, 1, 1);

			if(i == 2)
			{
				QFrame *separator = new QFrame(this);
				separator->setFrameStyle(QFrame::HLine);
				layoutPlane->addWidget(separator, y++, 0, 1, 2);
			}
		}
	}

	auto *grid = new QGridLayout(this);
	grid->setHorizontalSpacing(2);
	grid->setVerticalSpacing(2);
	grid->setContentsMargins(4,4,4,4);

	int y = 0;
	grid->addWidget(groupLattice, y++, 0, 1, 1);
	grid->addWidget(groupPlane, y++, 0, 1, 1);
	grid->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding), y++, 0, 1, 1);

	for(std::size_t i=0; i<m_num_lattice_elems; ++i)
	{
		// lattice constants
		connect(m_spinLatticeConsts[i],
			static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			[this, i](t_real val) -> void
			{
				t_real vals[m_num_lattice_elems];
				for(std::size_t j=0; j<m_num_lattice_elems; ++j)
				{
					if(j == i)
						vals[j] = val;
					else
						vals[j] = m_spinLatticeConsts[j]->value();
				}

				t_real alpha = m_spinLatticeAngles[0]->value() / t_real(180)*tl2::pi<t_real>;
				t_real beta = m_spinLatticeAngles[1]->value() / t_real(180)*tl2::pi<t_real>;
				t_real gamma = m_spinLatticeAngles[2]->value() / t_real(180)*tl2::pi<t_real>;

				emit LatticeChanged(vals[0], vals[1], vals[2], alpha, beta, gamma);
			});

		// lattice angles
		connect(m_spinLatticeAngles[i],
			static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			[this, i](t_real angle) -> void
			{
				t_real angles[m_num_lattice_elems];
				for(std::size_t j=0; j<m_num_lattice_elems; ++j)
				{
					if(j == i)
						angles[j] = angle / t_real(180)*tl2::pi<t_real>;
					else
						angles[j] = m_spinLatticeAngles[j]->value() / t_real(180)*tl2::pi<t_real>;
				}

				t_real a = m_spinLatticeConsts[0]->value();
				t_real b = m_spinLatticeConsts[1]->value();
				t_real c = m_spinLatticeConsts[2]->value();

				emit LatticeChanged(a, b, c, angles[0], angles[1], angles[2]);
			});
	}

	// plane vectors
	for(std::size_t i=0; i<m_num_plane_elems; ++i)
	{
		connect(m_spinPlane[i],
			static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			[this, i](t_real val) -> void
			{
				t_real vals[m_num_plane_elems];
				for(std::size_t j=0; j<m_num_plane_elems; ++j)
				{
					if(j == i)
						vals[j] = val;
					else
						vals[j] = m_spinPlane[j]->value();
				}

				emit PlaneChanged(vals[0], vals[1], vals[2], vals[3], vals[4], vals[5]);
			});
	}
}


XtalPropertiesWidget::~XtalPropertiesWidget()
{
}


void XtalPropertiesWidget::SetLattice(
	t_real a, t_real b, t_real c,
	t_real alpha, t_real beta, t_real gamma)
{
	this->blockSignals(true);

	m_spinLatticeConsts[0]->setValue(a);
	m_spinLatticeConsts[1]->setValue(b);
	m_spinLatticeConsts[2]->setValue(c);

	m_spinLatticeAngles[0]->setValue(alpha);
	m_spinLatticeAngles[1]->setValue(beta);
	m_spinLatticeAngles[2]->setValue(gamma);

	this->blockSignals(false);
}


void XtalPropertiesWidget::SetPlane(
	t_real vec1_x, t_real vec1_y, t_real vec1_z,
	t_real vec2_x, t_real vec2_y, t_real vec2_z)
{
	this->blockSignals(true);

	m_spinPlane[0]->setValue(vec1_x);
	m_spinPlane[1]->setValue(vec1_y);
	m_spinPlane[2]->setValue(vec1_z);

	m_spinPlane[3]->setValue(vec2_x);
	m_spinPlane[4]->setValue(vec2_y);
	m_spinPlane[5]->setValue(vec2_z);

	this->blockSignals(false);
}
// --------------------------------------------------------------------------------



// --------------------------------------------------------------------------------
// properties dock widget
// --------------------------------------------------------------------------------
XtalPropertiesDockWidget::XtalPropertiesDockWidget(QWidget *parent)
	: QDockWidget{parent}, 
		m_widget{std::make_shared<XtalPropertiesWidget>(this)}
{
	setObjectName("XtalPropertiesDockWidget");
	setWindowTitle("Crystal Definition");

	setWidget(m_widget.get());
}


XtalPropertiesDockWidget::~XtalPropertiesDockWidget()
{
}
// --------------------------------------------------------------------------------

/**
 * crystal properties dock widget
 * @author Tobias Weber <tweber@ill.fr>
 * @date apr-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __XTAL_PROP_WIDGET_H__
#define __XTAL_PROP_WIDGET_H__

#include <memory>
#include <QtWidgets/QWidget>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QDoubleSpinBox>

#include "src/core/types.h"


class XtalPropertiesWidget : public QWidget
{Q_OBJECT
public:
	XtalPropertiesWidget(QWidget *parent=nullptr);
	virtual ~XtalPropertiesWidget();

public slots:
	void SetLattice(t_real a, t_real b, t_real c, t_real alpha, t_real beta, t_real gamma);
	void SetPlane(t_real vec1_x, t_real vec1_y, t_real vec1_z, t_real vec2_x, t_real vec2_y, t_real vec2_z);

signals:
	void LatticeChanged(t_real a, t_real b, t_real c, t_real alpha, t_real beta, t_real gamma);
	void PlaneChanged(t_real vec1_x, t_real vec1_y, t_real vec1_z, t_real vec2_x, t_real vec2_y, t_real vec2_z);

private:
	// crystal lattice constants
	QDoubleSpinBox *m_spinLatticeConsts[3]{nullptr, nullptr, nullptr};
	// crystal lattice angles
	QDoubleSpinBox *m_spinLatticeAngles[3]{nullptr, nullptr, nullptr};
	// scattering plane vectors
	QDoubleSpinBox *m_spinPlane[6]{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
};


class XtalPropertiesDockWidget : public QDockWidget
{
public:
	XtalPropertiesDockWidget(QWidget *parent=nullptr);
	virtual ~XtalPropertiesDockWidget();

	std::shared_ptr<XtalPropertiesWidget> GetWidget() { return m_widget; }

private:
    std::shared_ptr<XtalPropertiesWidget> m_widget;
};


#endif

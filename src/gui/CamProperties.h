/**
 * camera properties dock widget
 * @author Tobias Weber <tweber@ill.fr>
 * @date apr-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __CAM_PROP_WIDGET_H__
#define __CAM_PROP_WIDGET_H__

#include <memory>
#include <QtWidgets/QWidget>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QCheckBox>

#include "src/core/types.h"


class CamPropertiesWidget : public QWidget
{Q_OBJECT
public:
	CamPropertiesWidget(QWidget *parent=nullptr);
	virtual ~CamPropertiesWidget();

public slots:
	void SetViewingAngle(t_real angle);
	void SetPerspectiveProj(bool persp);
	void SetCamPosition(t_real x, t_real y, t_real z);

signals:
	void ViewingAngleChanged(t_real angle);
	void PerspectiveProjChanged(bool persp);
	void CamPositionChanged(t_real x, t_real y, t_real z);

private:
	QDoubleSpinBox *m_spinViewingAngle{nullptr};
	QCheckBox *m_checkPerspectiveProj{nullptr};

	QDoubleSpinBox *m_spinPos[3]{nullptr, nullptr, nullptr};
};


class CamPropertiesDockWidget : public QDockWidget
{
public:
	CamPropertiesDockWidget(QWidget *parent=nullptr);
	virtual ~CamPropertiesDockWidget();

	std::shared_ptr<CamPropertiesWidget> GetWidget() { return m_widget; }

private:
    std::shared_ptr<CamPropertiesWidget> m_widget;
};


#endif

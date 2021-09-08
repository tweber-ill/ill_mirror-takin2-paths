/**
 * camera properties dock widget
 * @author Tobias Weber <tweber@ill.fr>
 * @date apr-2021
 * @license GPLv3, see 'LICENSE' file
 *
 * ----------------------------------------------------------------------------
 * TAS-Paths (part of the Takin software suite)
 * Copyright (C) 2021  Tobias WEBER (Institut Laue-Langevin (ILL), 
 *                     Grenoble, France).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * ----------------------------------------------------------------------------
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

	CamPropertiesWidget(const CamPropertiesWidget&) = delete;
	CamPropertiesWidget& operator=(const CamPropertiesWidget&) = delete;


public slots:
	void SetViewingAngle(t_real angle);
	void SetPerspectiveProj(bool persp);
	void SetCamPosition(t_real x, t_real y, t_real z);
	void SetCamRotation(t_real phi, t_real theta);


signals:
	void ViewingAngleChanged(t_real angle);
	void PerspectiveProjChanged(bool persp);
	void CamPositionChanged(t_real x, t_real y, t_real z);
	void CamRotationChanged(t_real phi, t_real theta);


private:
	QDoubleSpinBox *m_spinViewingAngle{nullptr};
	QCheckBox *m_checkPerspectiveProj{nullptr};

	QDoubleSpinBox *m_spinPos[3]{nullptr, nullptr, nullptr};
	QDoubleSpinBox *m_spinRot[2]{nullptr, nullptr};
};



class CamPropertiesDockWidget : public QDockWidget
{
public:
	CamPropertiesDockWidget(QWidget *parent=nullptr);
	virtual ~CamPropertiesDockWidget();

	CamPropertiesDockWidget(const CamPropertiesDockWidget&) = delete;
	const CamPropertiesDockWidget& operator=(const CamPropertiesDockWidget&) = delete;

	std::shared_ptr<CamPropertiesWidget> GetWidget() { return m_widget; }


private:
    std::shared_ptr<CamPropertiesWidget> m_widget;
};


#endif

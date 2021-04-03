/**
 * perperties dock widget
 * @author Tobias Weber <tweber@ill.fr>
 * @date mar-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __PROP_WIDGET_H__
#define __PROP_WIDGET_H__

#include <memory>
#include <QtWidgets/QWidget>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QDoubleSpinBox>

#include "src/core/types.h"


class TASPropertiesWidget : public QWidget
{Q_OBJECT
public:
	TASPropertiesWidget(QWidget *parent=nullptr);
	virtual ~TASPropertiesWidget();

public slots:
	void SetMonoScatteringAngle(t_real angle);
	void SetSampleScatteringAngle(t_real angle);
	void SetAnaScatteringAngle(t_real angle);

signals:
	void MonoScatteringAngleChanged(t_real angle);
	void SampleScatteringAngleChanged(t_real angle);
	void AnaScatteringAngleChanged(t_real angle);	

private:
	QDoubleSpinBox *m_spinMonoScAngle{nullptr};
	QDoubleSpinBox *m_spinSampleScAngle{nullptr};
	QDoubleSpinBox *m_spinAnaScAngle{nullptr};
};


class TASPropertiesDockWidget : public QDockWidget
{
public:
	TASPropertiesDockWidget(QWidget *parent=nullptr);
	virtual ~TASPropertiesDockWidget();

	std::shared_ptr<TASPropertiesWidget> GetWidget() { return m_widget; }

private:
    std::shared_ptr<TASPropertiesWidget> m_widget;
};


#endif

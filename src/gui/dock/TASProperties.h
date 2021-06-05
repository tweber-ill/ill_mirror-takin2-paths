/**
 * TAS properties dock widget
 * @author Tobias Weber <tweber@ill.fr>
 * @date mar-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __TAS_PROP_WIDGET_H__
#define __TAS_PROP_WIDGET_H__

#include <memory>
#include <QtWidgets/QWidget>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QCheckBox>

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

	void SetMonoCrystalAngle(t_real angle);
	void SetSampleCrystalAngle(t_real angle);
	void SetAnaCrystalAngle(t_real angle);

	void SetDSpacings(t_real dmono, t_real dana);
	void SetScatteringSenses(bool monoccw, bool sampleccw, bool anaccw);

signals:
	void MonoScatteringAngleChanged(t_real angle);
	void SampleScatteringAngleChanged(t_real angle);
	void AnaScatteringAngleChanged(t_real angle);	

	void MonoCrystalAngleChanged(t_real angle);
	void SampleCrystalAngleChanged(t_real angle);
	void AnaCrystalAngleChanged(t_real angle);	

	void DSpacingsChanged(t_real dmono, t_real dana);
	void ScatteringSensesChanged(bool monoccw, bool sampleccw, bool anaccw);

private:
	// scattering angles
	QDoubleSpinBox *m_spinMonoScAngle{nullptr};
	QDoubleSpinBox *m_spinSampleScAngle{nullptr};
	QDoubleSpinBox *m_spinAnaScAngle{nullptr};

	// crystal angles
	QDoubleSpinBox *m_spinMonoXtalAngle{nullptr};
	QDoubleSpinBox *m_spinSampleXtalAngle{nullptr};
	QDoubleSpinBox *m_spinAnaXtalAngle{nullptr};

	// d spacings
	QDoubleSpinBox *m_spinMonoD{nullptr};
	QDoubleSpinBox *m_spinAnaD{nullptr};

	// scattering senses
	QCheckBox *m_checkScatteringSense[3]{nullptr, nullptr, nullptr};
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
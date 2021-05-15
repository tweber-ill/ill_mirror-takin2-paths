/**
 * TAS paths tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date may-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __TAKIN_PATHS_CFGSPACE_H__
#define __TAKIN_PATHS_CFGSPACE_H__

#include <QtWidgets/QDialog>
#include <QtCore/QSettings>
#include <QtWidgets/QDoubleSpinBox>

#include <memory>

#include "src/core/types.h"
#include "src/core/Instrument.h"
#include "qcustomplot/qcustomplot.h"


class ConfigSpaceDlg : public QDialog
{
public:
	ConfigSpaceDlg(QWidget* parent = nullptr, QSettings *sett = nullptr);
	virtual ~ConfigSpaceDlg();

	void SetInstrumentSpace(const InstrumentSpace* instr) { m_instrspace = instr; }
	void SetScatteringSenses(const t_real *senses) { m_sensesCCW = senses; }
	void Calculate();

protected:
	virtual void accept() override;

private:
	QSettings *m_sett{nullptr};

	std::shared_ptr<QCustomPlot> m_plot;
	QCPColorMap* m_colourMap{};
	QDoubleSpinBox *m_spinDelta2ThS{}, *m_spinDelta2ThM{};

	const InstrumentSpace *m_instrspace{};
	const t_real* m_sensesCCW{nullptr};
};


#endif

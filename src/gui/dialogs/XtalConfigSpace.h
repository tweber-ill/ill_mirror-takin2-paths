/**
 * xtal configuration space
 * @author Tobias Weber <tweber@ill.fr>
 * @date aug-2021
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

#ifndef __TAKIN_PATHS_XTAL_CFGSPACE_H__
#define __TAKIN_PATHS_XTAL_CFGSPACE_H__

#include <QtCore/QSettings>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDoubleSpinBox>

#include <cstdint>
#include <memory>

#include "qcp_wrapper.h"

#include "src/libs/img.h"
#include "src/core/InstrumentSpace.h"
#include "src/core/TasCalculator.h"


class XtalConfigSpaceDlg : public QDialog
{ Q_OBJECT
public:
	XtalConfigSpaceDlg(QWidget* parent = nullptr, QSettings *sett = nullptr);
	virtual ~XtalConfigSpaceDlg();

	XtalConfigSpaceDlg(const XtalConfigSpaceDlg&) = delete;
	const XtalConfigSpaceDlg& operator=(const XtalConfigSpaceDlg&) = delete;

	void UpdatePlotRanges();
	void Calculate();

	// ------------------------------------------------------------------------
	// input instrument
	// ------------------------------------------------------------------------
	void SetInstrumentSpace(const InstrumentSpace* instr) { m_instrspace = instr; }
	const InstrumentSpace* GetInstrumentSpace() const { return m_instrspace; }

	void SetTasCalculator(const TasCalculator* tascalc) { m_tascalc = tascalc; }
	const TasCalculator* GetTasCalculator() const { return m_tascalc; }
	// ------------------------------------------------------------------------


protected:
	virtual void accept() override;

	void RedrawPlot();

	// either move instrument by clicking in the plot or enable plot zoom mode
	void SetInstrumentMovable(bool moveInstr);

	// calculate crystal coordinates from graph position
	std::tuple<t_vec, t_real, t_real> GetQkikf(t_real x, t_real y) const;


private:
	QSettings *m_sett{nullptr};

	// plot curves
	std::shared_ptr<QCustomPlot> m_plot{};
	QCPColorMap* m_colourMap{};

	QLabel *m_status{};
	QDoubleSpinBox *m_spinVec1Start{}, *m_spinVec1End{}, *m_spinVec1Delta{};
	QDoubleSpinBox *m_spinVec2Start{}, *m_spinVec2End{}, *m_spinVec2Delta{};
	QDoubleSpinBox *m_spinE{};

	const InstrumentSpace *m_instrspace{};
	const TasCalculator *m_tascalc{};

	geo::Image<std::uint8_t> m_img{};
	bool m_moveInstr = true;


signals:
	void GotoCoordinates(t_real h, t_real k, t_real l, t_real ki, t_real kf);
};

#endif

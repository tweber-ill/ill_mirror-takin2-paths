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

#include <cstdint>
#include <memory>

#include "src/core/PathsBuilder.h"
#include "qcustomplot/qcustomplot.h"


class ConfigSpaceDlg : public QDialog
{ Q_OBJECT
public:
	ConfigSpaceDlg(QWidget* parent = nullptr, QSettings *sett = nullptr);
	virtual ~ConfigSpaceDlg();

	void SetPathsBuilder(PathsBuilder* builder) { m_pathsbuilder = builder; }
	void Calculate();

	void EmitGotoAngles(std::optional<t_real> a1,
		std::optional<t_real> a3, std::optional<t_real> a4,
		std::optional<t_real> a5);

signals:
	void GotoAngles(std::optional<t_real> a1,
		std::optional<t_real> a3, std::optional<t_real> a4,
		std::optional<t_real> a5);

protected:
	virtual void accept() override;

private:
	QSettings *m_sett{nullptr};

	geo::Image<std::uint8_t> m_img;
	std::shared_ptr<QCustomPlot> m_plot;
	QCPColorMap* m_colourMap{};

	QLabel *m_status{};
	QDoubleSpinBox *m_spinDelta2ThS{}, *m_spinDelta2ThM{};

	PathsBuilder *m_pathsbuilder{};
};


#endif

/**
 * angular configuration space dialog
 * @author Tobias Weber <tweber@ill.fr>
 * @date may-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __TAKIN_PATHS_CFGSPACE_H__
#define __TAKIN_PATHS_CFGSPACE_H__

#include <QtCore/QSettings>
#include <QtWidgets/QDialog>
#include <QtWidgets/QProgressDialog>
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

	void SetPathsBuilder(PathsBuilder* builder);
	void UnsetPathsBuilder();

	void CalculatePathMesh();
	void CalculatePath();

	void UpdatePlotRanges();

	// receivers for instrument (space) and target position update signals
	void UpdateInstrument(const Instrument& instr, const t_real* sensesCCW = nullptr);
	void UpdateTarget(t_real monoScAngle, t_real sampleScAngle, const t_real* sensesCCW = nullptr);

	void EmitGotoAngles(std::optional<t_real> a1,
		std::optional<t_real> a3, std::optional<t_real> a4,
		std::optional<t_real> a5);


protected:
	virtual void accept() override;

	// voronoi / path mesh plot curves
	void ClearVoronoiPlotCurves();
	void AddVoronoiPlotCurve(const QVector<t_real>& x, const QVector<t_real>& y,
		t_real width = 1., QColor colour = QColor::fromRgbF(1., 1., 1.));
	void RedrawVoronoiPlot();

	// path plot curve
	void ClearPathPlotCurve();
	void SetPathPlotCurve(const QVector<t_real>& x, const QVector<t_real>& y,
		t_real width = 1., QColor colour = QColor::fromRgbF(1., 1., 1.));
	void RedrawPathPlot();

	// either move instrument by clicking in the plot or enable plot zoom mode
	void SetInstrumentMovable(bool moveInstr);


private:
	QSettings *m_sett{nullptr};
	std::unique_ptr<QProgressDialog> m_progress{};

	// start and target positions
	t_real m_starta2 = 0.;
	t_real m_enda2 = tl2::pi<t_real>;
	t_real m_starta4 = -tl2::pi<t_real>;
	t_real m_enda4 = tl2::pi<t_real>;

	// plot curves
	std::shared_ptr<QCustomPlot> m_plot;
	QCPColorMap* m_colourMap{};
	std::vector<QCPCurve*> m_vorocurves{};
	QCPCurve* m_pathcurve = nullptr;
	std::vector<t_vec2> m_pathvertices;

	// current instrument position
	t_real m_curMonoScatteringAngle{};
	t_real m_curSampleScatteringAngle{};
	QCPGraph *m_instrposplot{};

	// target instrument position
	t_real m_targetMonoScatteringAngle{};
	t_real m_targetSampleScatteringAngle{};
	QCPGraph *m_targetposplot{};

	QLabel *m_status{};
	QDoubleSpinBox *m_spinDelta2ThS{}, *m_spinDelta2ThM{};

	PathsBuilder *m_pathsbuilder{};
	boost::signals2::connection m_pathsbuilderslot{};

	// options
	bool m_grouplines = false;
	bool m_simplifycontour = true;
	bool m_splitcontour = false;
	bool m_calcvoronoi = true;
	bool m_autocalcpath = true;
	bool m_subdivide_path = false;
	bool m_movetarget = false;

	bool m_moveInstr = true;


signals:
	void GotoAngles(std::optional<t_real> a1,
		std::optional<t_real> a3, std::optional<t_real> a4,
		std::optional<t_real> a5, bool move_target);


protected slots:
	bool PathsBuilderProgress(bool start, bool end, t_real progress, const std::string& message);
};


#endif

/**
 * TAS paths tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date may-2021
 * @license GPLv3, see 'LICENSE' file
 * 
 * References:
 *   - https://www.qcustomplot.com/documentation/classQCPColorMap.html
 */

#include "ConfigSpace.h"

#include <QtWidgets/QGridLayout>


ConfigSpaceDlg::ConfigSpaceDlg(QWidget* parent, QSettings *sett)
	: QDialog{parent}, m_sett{sett}
{
	setWindowTitle("Configuration Space");

	// restore dialog geometry
	if(m_sett->contains("configspace/geo"))
		restoreGeometry(m_sett->value("configspace/geo").toByteArray());
	else
		resize(800, 600);

	// plotter
	m_plot = std::make_shared<QCustomPlot>(this);
	m_plot->setSelectionRectMode(QCP::srmZoom);
	m_plot->setInteraction(QCP::Interaction(int(QCP::iRangeZoom) | int(QCP::iRangeDrag)));
	m_plot->replot();

	// grid
	auto grid = new QGridLayout(this);
	grid->setSpacing(4);
	grid->setContentsMargins(16, 16, 16, 16);
	grid->addWidget(m_plot.get(), 0, 0, 1, 1);
}


ConfigSpaceDlg::~ConfigSpaceDlg()
{
}


void ConfigSpaceDlg::accept()
{
	if(m_sett)
		m_sett->setValue("configspace/geo", saveGeometry());
}

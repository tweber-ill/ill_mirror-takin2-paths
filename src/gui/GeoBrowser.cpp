/**
 * geometries browser
 * @author Tobias Weber <tweber@ill.fr>
 * @date jun-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "GeoBrowser.h"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QDialogButtonBox>


GeometriesBrowser::GeometriesBrowser(QWidget* parent, QSettings *sett)
	: QDialog{parent}, m_sett{sett}
{
	setWindowTitle("Geometries Browser");


	m_geotree = new QTreeWidget(this);
	m_geotree->headerItem()->setText(0, "Instrument Space");

 	QSizePolicy sptree(QSizePolicy::Preferred, QSizePolicy::Expanding, QSizePolicy::DefaultType);
    sptree.setHorizontalStretch(1);
    sptree.setVerticalStretch(1);
	m_geotree->setSizePolicy(sptree);


	auto *geosettings = new QWidget(this);

 	QSizePolicy spsettings(QSizePolicy::Preferred, QSizePolicy::Expanding, QSizePolicy::Frame);
    spsettings.setHorizontalStretch(2);
    spsettings.setVerticalStretch(1);
	geosettings->setSizePolicy(spsettings);


	// splitter
	m_splitter = new QSplitter(Qt::Horizontal, this);
	m_splitter->addWidget(m_geotree);
	m_splitter->addWidget(geosettings);

	QDialogButtonBox *buttons = new QDialogButtonBox(this);
	buttons->setStandardButtons(QDialogButtonBox::Ok);


	// grid layout
	auto grid = new QGridLayout(this);
	grid->setSpacing(4);
	grid->setContentsMargins(12, 12, 12, 12);

	int y = 0;
	grid->addWidget(m_splitter, y++, 0, 1, 1);
	grid->addWidget(buttons, y++, 0, 1, 1);


	// restore settings
	if(m_sett)
	{
		if(m_sett->contains("geobrowser/geo"))
			restoreGeometry(m_sett->value("geobrowser/geo").toByteArray());
		if(m_sett->contains("geobrowser/splitter"))
			m_splitter->restoreState(m_sett->value("geobrowser/splitter").toByteArray());
	}


	// connections
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
}


GeometriesBrowser::~GeometriesBrowser()
{
}


void GeometriesBrowser::accept()
{
	if(m_sett)
	{
		m_sett->setValue("geobrowser/geo", saveGeometry());
		m_sett->setValue("geobrowser/splitter", m_splitter->saveState());
	}
	QDialog::accept();
}


void GeometriesBrowser::UpdateGeoTree(const InstrumentSpace& instrspace)
{
	m_geotree->clear();


	// walls
	auto* wallsitem = new QTreeWidgetItem(m_geotree);
	wallsitem->setText(0, "Walls");

	for(const auto& wall : instrspace.GetWalls())
	{
		auto* wallitem = new QTreeWidgetItem(wallsitem);
		wallitem->setText(0, wall->GetId().c_str());
	}

	m_geotree->expandItem(wallsitem);


	// instrument
	auto* instritem = new QTreeWidgetItem(m_geotree);
	instritem->setText(0, "Instrument");

	for(const Axis& axis : {
		instrspace.GetInstrument().GetMonochromator(), 
		instrspace.GetInstrument().GetSample(), 
		instrspace.GetInstrument().GetAnalyser() })
	{
		auto* axisitem = new QTreeWidgetItem(instritem);
		axisitem->setText(0, axis.GetId().c_str());

		std::vector<std::string> axislabels{{
			"Relative Incoming Axis", 
			"Relative Internal Axis",
			"Relative Outgoing Axis"
		}};

		std::vector<AxisAngle> axisangles = {{
			AxisAngle::INCOMING,
			AxisAngle::INTERNAL,
			AxisAngle::OUTGOING
		}};

		for(int i=0; i<3; ++i)
		{
			auto* axissubitem = new QTreeWidgetItem(axisitem);
			axissubitem->setText(0, axislabels[i].c_str());

			for(const auto& comp : axis.GetComps(axisangles[i]))
			{
				auto* compitem = new QTreeWidgetItem(axissubitem);
				compitem->setText(0, comp->GetId().c_str());
			}
		}

		//m_geotree->expandItem(axisitem);
	}

	m_geotree->expandItem(instritem);
}

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
#include <QtWidgets/QHeaderView>


GeometriesBrowser::GeometriesBrowser(QWidget* parent, QSettings *sett)
	: QDialog{parent}, m_sett{sett}
{
	setWindowTitle("Geometries Browser");


	// geometry object tree
	m_geotree = new QTreeWidget(this);
	m_geotree->headerItem()->setText(0, "Instrument Space");

 	QSizePolicy sptree(QSizePolicy::Preferred, QSizePolicy::Expanding, QSizePolicy::DefaultType);
    sptree.setHorizontalStretch(1);
    sptree.setVerticalStretch(1);
	m_geotree->setSizePolicy(sptree);


	// geometry settings table
	m_geosettings = new QTableWidget(this);
	m_geosettings->setShowGrid(true);
	m_geosettings->setSortingEnabled(true);
	m_geosettings->setMouseTracking(true);
	m_geosettings->setSelectionBehavior(QTableWidget::SelectItems);
	m_geosettings->setSelectionMode(QTableWidget::SingleSelection);
	m_geosettings->horizontalHeader()->setDefaultSectionSize(200);
	m_geosettings->verticalHeader()->setDefaultSectionSize(32);
	m_geosettings->verticalHeader()->setVisible(false);
	m_geosettings->setColumnCount(2);
	m_geosettings->setColumnWidth(0, 200);
	m_geosettings->setColumnWidth(1, 200);
	m_geosettings->setHorizontalHeaderItem(0, new QTableWidgetItem{"Key"});
	m_geosettings->setHorizontalHeaderItem(1, new QTableWidgetItem{"Value"});

 	QSizePolicy spsettings(QSizePolicy::Preferred, QSizePolicy::Expanding, QSizePolicy::Frame);
    spsettings.setHorizontalStretch(2);
    spsettings.setVerticalStretch(1);
	m_geosettings->setSizePolicy(spsettings);


	// splitter
	m_splitter = new QSplitter(Qt::Horizontal, this);
	m_splitter->addWidget(m_geotree);
	m_splitter->addWidget(m_geosettings);

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
	connect(buttons, &QDialogButtonBox::accepted, this, &GeometriesBrowser::accept);
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
/**
 * geometries browser
 * @author Tobias Weber <tweber@ill.fr>
 * @date jun-2021
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

#include "GeoBrowser.h"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMessageBox>

#include <boost/scope_exit.hpp>

#include "src/gui/settings_variables.h"
#include "tlibs2/libs/expr.h"
#include "tlibs2/libs/str.h"


// columns in the settings table
#define GEOBROWSER_SETTINGS_KEY 0
#define GEOBROWSER_SETTINGS_TYPE 1
#define GEOBROWSER_SETTINGS_VALUE 2


/**
 * create a geometries browser dialog
 */
GeometriesBrowser::GeometriesBrowser(QWidget* parent, QSettings *sett)
	: QDialog{parent}, m_sett{sett}
{
	setWindowTitle("Object Browser");
	setSizeGripEnabled(true);


	// geometry object tree
	m_geotree = new QTreeWidget(this);
	m_geotree->headerItem()->setText(0, "Instrument Space");

	QSizePolicy sptree(QSizePolicy::Preferred,
		QSizePolicy::Expanding, QSizePolicy::DefaultType);
	sptree.setHorizontalStretch(1);
	sptree.setVerticalStretch(1);
	m_geotree->setSizePolicy(sptree);
	m_geotree->setContextMenuPolicy(Qt::CustomContextMenu);


	// tree context menu
	m_contextMenuGeoTree = new QMenu(m_geotree);
	QAction *actionRenObj = new QAction(
		QIcon::fromTheme("edit-find-replace"),
		"Rename Object");
	QAction *actionDelObj = new QAction(
		QIcon::fromTheme("edit-delete"),
		"Delete Object");

	m_contextMenuGeoTree->addAction(actionRenObj);
	m_contextMenuGeoTree->addAction(actionDelObj);

	connect(actionRenObj, &QAction::triggered,
		this, &GeometriesBrowser::RenameCurrentGeoTreeObject);
	connect(actionDelObj, &QAction::triggered,
		this, &GeometriesBrowser::DeleteCurrentGeoTreeObject);


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
	m_geosettings->setColumnCount(3);
	m_geosettings->setColumnWidth(0, 150);
	m_geosettings->setColumnWidth(1, 75);
	m_geosettings->setColumnWidth(2, 150);
	m_geosettings->setHorizontalHeaderItem(
		GEOBROWSER_SETTINGS_KEY, new QTableWidgetItem{"Key"});
	m_geosettings->setHorizontalHeaderItem(
		GEOBROWSER_SETTINGS_TYPE, new QTableWidgetItem{"Type"});
	m_geosettings->setHorizontalHeaderItem(
		GEOBROWSER_SETTINGS_VALUE, new QTableWidgetItem{"Value"});

	QSizePolicy spsettings(QSizePolicy::Preferred,
		QSizePolicy::Expanding, QSizePolicy::Frame);
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
		// restore dialog geometry
		if(m_sett->contains("geobrowser/geo"))
			restoreGeometry(m_sett->value("geobrowser/geo").toByteArray());
		else
			resize(600, 400);

		// restore splitter position
		if(m_sett->contains("geobrowser/splitter"))
			m_splitter->restoreState(m_sett->value("geobrowser/splitter").toByteArray());

		// restore settings table column widths
		if(m_sett->contains("geobrowser/settings_col0_width"))
			m_geosettings->setColumnWidth(0, m_sett->value("geobrowser/settings_col0_width").toInt());
		if(m_sett->contains("geobrowser/settings_col1_width"))
			m_geosettings->setColumnWidth(1, m_sett->value("geobrowser/settings_col1_width").toInt());
		if(m_sett->contains("geobrowser/settings_col2_width"))
			m_geosettings->setColumnWidth(2, m_sett->value("geobrowser/settings_col2_width").toInt());
	}


	// connections
	connect(buttons, &QDialogButtonBox::accepted,
		this, &GeometriesBrowser::accept);
	connect(buttons, &QDialogButtonBox::rejected,
		this, &GeometriesBrowser::reject);

	connect(m_geotree, &QTreeWidget::customContextMenuRequested,
		this, &GeometriesBrowser::ShowGeoTreeContextMenu);
	connect(m_geotree, &QTreeWidget::itemChanged,
		this, &GeometriesBrowser::GeoTreeItemChanged);
	connect(m_geotree, &QTreeWidget::currentItemChanged,
		this, &GeometriesBrowser::GeoTreeCurrentItemChanged);

	connect(m_geosettings, &QTableWidget::itemChanged,
		this, &GeometriesBrowser::GeoSettingsItemChanged);
}


/**
 * destroy the geometries browser dialog
 */
GeometriesBrowser::~GeometriesBrowser()
{
}


/**
 * show the context menu for the geometry object tree
 */
void GeometriesBrowser::ShowGeoTreeContextMenu(const QPoint& pt)
{
	m_curContextItem = m_geotree->itemAt(pt);
	if(!m_curContextItem)
		return;

	auto ptGlob = m_geotree->mapToGlobal(pt);
	ptGlob.setX(ptGlob.x() + 8);
	ptGlob.setY(ptGlob.y() + m_contextMenuGeoTree->sizeHint().height()/2 + 8);
	m_contextMenuGeoTree->popup(ptGlob);
}


/**
 * rename an object in the instrument space
 */
void GeometriesBrowser::RenameCurrentGeoTreeObject()
{
	if(!m_curContextItem)
		return;

	m_geotree->editItem(m_curContextItem);
}


/**
 * delete an object from the instrument space
 */
void GeometriesBrowser::DeleteCurrentGeoTreeObject()
{
	if(!m_curContextItem)
		return;

	std::string id = m_curContextItem->text(0).toStdString();
	emit SignalDeleteObject(id);
}


/**
 * an object has been renamed
 */
void GeometriesBrowser::GeoTreeItemChanged(QTreeWidgetItem *item, int col)
{
	if(!item)
		return;

	std::string newid = item->text(col).toStdString();
	std::string oldid = item->data(col, Qt::UserRole).toString().toStdString();

	if(oldid == "" || newid == "" || oldid == newid)
		return;

	//std::cout << "renaming from " << oldid << " to " << newid << std::endl;
	item->setData(col, Qt::UserRole, QString(newid.c_str()));
	emit SignalRenameObject(oldid, newid);

	// re-select item
	m_curObject = newid;
}


/**
 * an object has been selected
 */
void GeometriesBrowser::GeoTreeCurrentItemChanged(QTreeWidgetItem *item, QTreeWidgetItem */*previtem*/)
{
	if(!item || !m_instrspace || !m_geosettings)
		return;

	std::string itemid = item->text(0).toStdString();
	if(itemid == "")
		return;

	m_curObject = itemid;

	// ignore programmatic settings changes
	m_ignoresettingschanges = true;
	GeometriesBrowser* pThis = this;
	BOOST_SCOPE_EXIT(pThis)
	{
		pThis->m_ignoresettingschanges = false;
	} BOOST_SCOPE_EXIT_END


	// get the geometry object properties and insert them in the table
	const auto& props = m_instrspace->GetProperties(m_curObject);

	m_geosettings->clearContents();
	m_geosettings->setRowCount(props.size());
	bool sorting_enabled = m_geosettings->isSortingEnabled();
	m_geosettings->setSortingEnabled(false);

	// iterate object properties
	for(std::size_t row=0; row<props.size(); ++row)
	{
		const auto& prop = props[row];

		// key
		auto* itemKey = new QTableWidgetItem(prop.key.c_str());
		itemKey->setFlags(itemKey->flags() & ~Qt::ItemIsEditable);
		m_geosettings->setItem(row, GEOBROWSER_SETTINGS_KEY, itemKey);

		// real value
		if(std::holds_alternative<t_real>(prop.value))
		{
			t_real val = std::get<t_real>(prop.value);

			std::ostringstream ostr;
			ostr.precision(g_prec);
			ostr << val;

			// type
			auto* itemType = new QTableWidgetItem("real");
			itemType->setFlags(itemType->flags() & ~Qt::ItemIsEditable);
			m_geosettings->setItem(row, GEOBROWSER_SETTINGS_TYPE, itemType);

			// value
			m_geosettings->setItem(row, GEOBROWSER_SETTINGS_VALUE,
				new QTableWidgetItem(ostr.str().c_str()));
		}

		// integer value
		else if(std::holds_alternative<t_int>(prop.value))
		{
			t_int val = std::get<t_int>(prop.value);

			std::ostringstream ostr;
			ostr.precision(g_prec);
			ostr << val;

			// type
			auto* itemType = new QTableWidgetItem("integer");
			itemType->setFlags(itemType->flags() & ~Qt::ItemIsEditable);
			m_geosettings->setItem(row, GEOBROWSER_SETTINGS_TYPE, itemType);

			// value
			m_geosettings->setItem(row, GEOBROWSER_SETTINGS_VALUE,
				new QTableWidgetItem(ostr.str().c_str()));
		}

		// string value
		else if(std::holds_alternative<std::string>(prop.value))
		{
			const std::string& val = std::get<std::string>(prop.value);

			// type
			auto* itemType = new QTableWidgetItem("string");
			itemType->setFlags(itemType->flags() & ~Qt::ItemIsEditable);
			m_geosettings->setItem(row, GEOBROWSER_SETTINGS_TYPE, itemType);

			// value
			m_geosettings->setItem(row, GEOBROWSER_SETTINGS_VALUE,
				new QTableWidgetItem(val.c_str()));
		}

		// vector value
		else if(std::holds_alternative<t_vec>(prop.value))
		{
			const t_vec& val = std::get<t_vec>(prop.value);

			std::ostringstream ostr;
			ostr.precision(g_prec);

			using namespace tl2_ops;
			ostr << val;

			/*for(std::size_t i=0; i<val.size(); ++i)
			{
				ostr << val[i];
				if(i != val.size()-1)
					ostr << ", ";
			}*/

			// type
			auto* itemType = new QTableWidgetItem("vector");
			itemType->setFlags(itemType->flags() & ~Qt::ItemIsEditable);
			m_geosettings->setItem(row, GEOBROWSER_SETTINGS_TYPE, itemType);

			// value
			m_geosettings->setItem(row, GEOBROWSER_SETTINGS_VALUE,
				new QTableWidgetItem(ostr.str().c_str()));
		}
	}

	m_geosettings->setSortingEnabled(sorting_enabled);
}


/**
 * an entry in the settings table has been changed
 */
void GeometriesBrowser::GeoSettingsItemChanged(QTableWidgetItem *item)
{
	if(!item || m_curObject == "" || m_ignoresettingschanges)
		return;

	try
	{
		int row = m_geosettings->row(item);

		QTableWidgetItem* itemKey = m_geosettings->item(row, GEOBROWSER_SETTINGS_KEY);
		QTableWidgetItem* itemType = m_geosettings->item(row, GEOBROWSER_SETTINGS_TYPE);
		QTableWidgetItem* itemVal = m_geosettings->item(row, GEOBROWSER_SETTINGS_VALUE);

		if(!itemKey || !itemType || !itemVal)
			return;

		std::string ty = itemType->text().toStdString();
		std::string val = itemVal->text().toStdString();

		ObjectProperty prop;
		prop.key = itemKey->text().toStdString();

		// real value
		if(ty == "real")
		{
			// parse the expression to yield a real value
			tl2::ExprParser<t_real> parser;
			if(!parser.parse(val))
				throw std::logic_error("Could not parse real expression.");
			prop.value = parser.eval();
		}

		// integer value
		else if(ty == "integer")
		{
			// parse the expression to yield a real value
			tl2::ExprParser<t_int> parser;
			if(!parser.parse(val))
				throw std::logic_error("Could not parse integer expression.");
			prop.value = parser.eval();
		}

		// string value
		else if(ty == "string")
		{
			prop.value = val;
		}

		// vector value
		else if(ty == "vector")
		{
			// get the components of the vector
			std::vector<std::string> tokens;
			tl2::get_tokens<std::string, std::string>(val, ";,", tokens);

			t_vec vec = tl2::create<t_vec>(tokens.size());
			for(std::size_t tokidx=0; tokidx<tokens.size(); ++tokidx)
			{
				// parse the vector component expression to yield a real value
				tl2::ExprParser<t_real> parser;
				if(!parser.parse(tokens[tokidx]))
					throw std::logic_error("Could not parse vector expression.");
				vec[tokidx] = parser.eval();
			}

			prop.value = vec;
		}

		emit SignalChangeObjectProperty(m_curObject, prop);
	}
	catch(const std::exception& ex)
	{
		QMessageBox::critical(this, "Error", ex.what());
	}
}


/**
 * close the dialog
 */
void GeometriesBrowser::accept()
{
	if(m_sett)
	{
		// save dialog geometry
		m_sett->setValue("geobrowser/geo", saveGeometry());
		m_sett->setValue("geobrowser/splitter", m_splitter->saveState());

		// save settings table column widths
		if(m_geosettings)
		{
			m_sett->setValue("geobrowser/settings_col0_width", m_geosettings->columnWidth(0));
			m_sett->setValue("geobrowser/settings_col1_width", m_geosettings->columnWidth(1));
			m_sett->setValue("geobrowser/settings_col2_width", m_geosettings->columnWidth(2));
		}
	}

	QDialog::accept();
}


/**
 * set an instrument space and refresh the information in the geometries object tree
 */
void GeometriesBrowser::UpdateGeoTree(const InstrumentSpace& instrspace)
{
	// remember the current instrument space
	m_instrspace = &instrspace;

	// clear old tree
	m_geotree->clear();


	// walls
	auto* wallsitem = new QTreeWidgetItem(m_geotree);
	wallsitem->setText(0, "Walls");

	for(const auto& wall : instrspace.GetWalls())
	{
		auto* wallitem = new QTreeWidgetItem(wallsitem);
		wallitem->setFlags(wallitem->flags() | Qt::ItemIsEditable);
		wallitem->setText(0, wall->GetId().c_str());
		wallitem->setData(0, Qt::UserRole, QString(wall->GetId().c_str()));
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


/**
 * select an object in the geometries tree
 */
void GeometriesBrowser::SelectObject(const std::string& obj)
{
	QList<QTreeWidgetItem*> items = m_geotree->findItems(obj.c_str(), Qt::MatchRecursive, 0);

	if(items.size())
	{
		m_geotree->setCurrentItem(items[0], QItemSelectionModel::Select);
	}
	else
	{
		QMessageBox::warning(this, "Warning",
			QString("Object \"") + obj.c_str() + QString("\" was not found."));
	}
}

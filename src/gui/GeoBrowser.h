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

#ifndef __TAKIN_PATHS_GEOBROWSER_H__
#define __TAKIN_PATHS_GEOBROWSER_H__


#include <QtCore/QSettings>
#include <QtWidgets/QDialog>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QMenu>

#include "src/core/InstrumentSpace.h"


class GeometriesBrowser : public QDialog
{ Q_OBJECT
public:
	GeometriesBrowser(QWidget* parent = nullptr, QSettings *sett = nullptr);
	virtual ~GeometriesBrowser();

	GeometriesBrowser(const GeometriesBrowser&) = delete;
	const GeometriesBrowser& operator=(const GeometriesBrowser&) = delete;

	void UpdateGeoTree(const InstrumentSpace& instrspace);
	void SelectObject(const std::string& obj);


protected:
	virtual void accept() override;

	void ShowGeoTreeContextMenu(const QPoint& pt);
	void RenameCurrentGeoTreeObject();
	void DeleteCurrentGeoTreeObject();
	void GeoTreeItemChanged(QTreeWidgetItem *item, int col);
	void GeoTreeCurrentItemChanged(QTreeWidgetItem *item, QTreeWidgetItem *previtem);

	void GeoSettingsItemChanged(QTableWidgetItem *item);


private:
	const InstrumentSpace* m_instrspace{nullptr};
	QSettings *m_sett{nullptr};

	QTreeWidget *m_geotree{nullptr};
	QTableWidget *m_geosettings{nullptr};
	QSplitter *m_splitter{nullptr};

	QMenu *m_contextMenuGeoTree{nullptr};
	QTreeWidgetItem *m_curContextItem{nullptr};

	// currently selected geometry object
	std::string m_curObject{};
	bool m_ignoresettingschanges{false};


signals:
	void SignalDeleteObject(const std::string& id);
	void SignalRenameObject(const std::string& oldId, const std::string& newId);

	void SignalChangeObjectProperty(const std::string& id, const GeometryProperty& prop);
};


#endif

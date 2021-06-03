/**
 * geometries browser
 * @author Tobias Weber <tweber@ill.fr>
 * @date jun-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __TAKIN_PATHS_GEOBROWSER_H__
#define __TAKIN_PATHS_GEOBROWSER_H__


#include <QtCore/QSettings>
#include <QtWidgets/QDialog>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTableWidget>

#include "src/core/Instrument.h"


class GeometriesBrowser : public QDialog
{
public:
	GeometriesBrowser(QWidget* parent = nullptr, QSettings *sett = nullptr);
	virtual ~GeometriesBrowser();

	void UpdateGeoTree(const InstrumentSpace& instrspace);

protected:
	virtual void accept() override;

private:
	QSettings *m_sett{nullptr};

	QTreeWidget *m_geotree{nullptr};
	QTableWidget *m_geosettings{nullptr};
	QSplitter *m_splitter{nullptr};
};


#endif

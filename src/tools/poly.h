/**
 * polygon splitting and kernel calculation test program
 * @author Tobias Weber <tweber@ill.fr>
 * @date 11-nov-2020
 * @note Forked on 4-aug-2021 from my privately developed "geo" project (https://github.com/t-weber/geo).
 * @license GPLv3, see 'LICENSE' file
 *
 * ----------------------------------------------------------------------------
 * TAS-Paths (part of the Takin software suite)
 * Copyright (C) 2021       Tobias WEBER (Institut Laue-Langevin (ILL),
 *                          Grenoble, France).
 * "geo" project
 * Copyright (C) 2020-2021  Tobias WEBER (privately developed).
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

#ifndef __POLY_GUI_H__
#define __POLY_GUI_H__

#include <QtWidgets/QDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QMenu>
#include <QtCore/QSettings>

#include <memory>
#include <vector>

#include "src/libs/voronoi_lines.h"
#include "src/libs/lines.h"

#include "tlibs2/libs/qt/recent.h"

#include "vertex.h"
#include "about.h"
#include "settings_variables.h"

#ifdef TASPATHS_TOOLS_STANDALONE
	#include "src/gui/dialogs/Settings.h"
	#define GeoSettingsDlg SettingsDlg<g_settingsvariables_tools.size(), &g_settingsvariables_tools>
#endif


class PolyView : public QGraphicsView
{ Q_OBJECT
public:
	using t_vec = ::t_vec2;
	using t_mat = ::t_mat22;

public:
	PolyView(QGraphicsScene *scene=nullptr, QWidget *parent=nullptr);
	virtual ~PolyView();

	PolyView(PolyView&) = delete;
	const PolyView& operator=(const PolyView&) const = delete;

	void AddVertex(const QPointF& pos);
	void ClearVertices();

	const std::vector<Vertex*>& GetVertexElems() const { return m_elems_vertices; }
	std::vector<Vertex*>& GetVertexElems() { return m_elems_vertices; }

	void UpdateAll();
	void UpdateEdges();
	void UpdateSplitPolygon();
	void UpdateKer();

	void SetSortVertices(bool b);
	bool GetSortVertices() const { return m_sortvertices; }

	void SetCalcSplitPolygon(bool b);
	bool GetCalcSplitPolygon() const { return m_splitpolygon; }

	void SetCalcKernel(bool b);
	bool GetCalcKernel() const { return m_calckernel; }

protected:
	virtual void mousePressEvent(QMouseEvent *evt) override;
	virtual void mouseReleaseEvent(QMouseEvent *evt) override;
	virtual void mouseMoveEvent(QMouseEvent *evt) override;

	virtual void drawBackground(QPainter*, const QRectF&) override;
	virtual void drawForeground(QPainter*, const QRectF&) override;

	virtual void resizeEvent(QResizeEvent *evt) override;

private:
	QGraphicsScene *m_scene = nullptr;

	std::vector<Vertex*> m_elems_vertices{};
	std::vector<QGraphicsItem*> m_elems_edges{}, m_elems_ker{}, m_elems_split{};

	bool m_dragging = false;

	std::vector<t_vec> m_vertices{};

	bool m_sortvertices = true;
	bool m_splitpolygon = true;
	bool m_calckernel = true;

signals:
	void SignalMouseCoordinates(double x, double y);
	void SignalError(const QString& err);
};



class PolyWnd : public QDialog
{ Q_OBJECT
public:
	using QDialog::QDialog;

	PolyWnd(QWidget* pParent = nullptr);
	virtual ~PolyWnd();

	PolyWnd(PolyWnd&) = delete;
	const PolyWnd& operator=(const PolyWnd&) const = delete;

	void SetStatusMessage(const QString& msg);

protected:
	virtual void closeEvent(QCloseEvent *) override;
	void SetCurrentFile(const QString &file);

private:
	QSettings m_sett{};

	// recently opened files
	QMenu *m_menuOpenRecent{ nullptr };
	tl2::RecentFiles m_recent{};
	// function to call for the recent file menu items
	std::function<bool(const QString& filename)> m_open_func
		= [this](const QString& filename) -> bool
	{
		return this->OpenFile(filename);
	};

#ifdef TASPATHS_TOOLS_STANDALONE
	std::shared_ptr<GeoSettingsDlg> m_dlgSettings{};
#endif
	std::shared_ptr<GeoAboutDlg> m_dlgAbout{};

	std::shared_ptr<QGraphicsScene> m_scene{};
	std::shared_ptr<PolyView> m_view{};
	std::shared_ptr<QLabel> m_statusLabel{};

protected slots:
	// File -> New
	void NewFile();

	// File -> Open
	void OpenFile();
	bool OpenFile(const QString& filename);

	// File -> Save
	void SaveFile();
	bool SaveFile(const QString& filename);

	// File -> Save As
	void SaveFileAs();
};


#endif

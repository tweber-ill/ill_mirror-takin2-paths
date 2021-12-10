/**
 * convex hull test program
 * @author Tobias Weber <tweber@ill.fr>
 * @date 15-Aug-2020
 * @note Forked on 19-apr-2021 from my privately developed "geo" project (https://github.com/t-weber/geo).
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

#ifndef __HULL_GUI_H__
#define __HULL_GUI_H__

#include <QtCore/QSettings>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QDialog>
#include <QtWidgets/QMenu>
#include <QtWidgets/QLabel>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QMenu>

#include <memory>
#include <unordered_set>
#include <vector>

#include "vertex.h"
#include "about.h"
#include "Settings.h"
#include "src/gui/Recent.h"

#define GeoSettingsDlg SettingsDlg


enum class HullCalculationMethod
{
	QHULL,
	CONTOUR,
	ITERATIVE,
	RECURSIVE,
};


enum class DelaunayCalculationMethod
{
	QHULL,
	ITERATIVE,
	PARABOLIC,
};


enum class SpanCalculationMethod
{
	KRUSKAL,
	BOOST,
};



class HullScene : public QGraphicsScene
{ Q_OBJECT
public:
	using t_vec = ::t_vec2;
	using t_vec_dyn = ::t_vec;
	using t_mat = ::t_mat22;

public:
	HullScene(QWidget* parent);
	virtual ~HullScene();

	HullScene(HullScene&) = delete;
	const HullScene& operator=(const HullScene&) const = delete;

public:
	void SetCalculateHull(bool b);
	void SetCalculateVoronoiVertices(bool b);
	void SetCalculateVoronoiRegions(bool b);
	void SetCalculateDelaunay(bool b);
	void SetCalculateKruskal(bool b);

	bool GetCalculateHull() const { return m_calchull; }
	bool GetCalculateVoronoiVertices() const { return m_calcvoronoivertices; }
	bool GetCalculateVoronoiRegions() const { return m_calcvoronoiregions; }
	bool GetCalculateDelaunay() const { return m_calcdelaunay; }
	bool GetCalculateKruskal() const { return m_calckruskal; }

	void SetHullCalculationMethod(HullCalculationMethod m);
	void SetDelaunayCalculationMethod(DelaunayCalculationMethod m);
	void SetSpanCalculationMethod(SpanCalculationMethod m);

	void AddVertex(const QPointF& pos);
	void ClearVertices();
	const std::unordered_set<Vertex*>& GetVertices() const { return m_vertices; }
	std::unordered_set<Vertex*>& GetVertices() { return m_vertices; }

	void UpdateAll();
	void UpdateHull();
	void UpdateDelaunay();

private:
	QWidget *m_parent = nullptr;

	std::unordered_set<Vertex*> m_vertices{};
	std::unordered_set<QGraphicsItem*> m_hull{};
	std::unordered_set<QGraphicsItem*> m_voronoi{};
	std::unordered_set<QGraphicsItem*> m_delaunay{};

	bool m_calchull = true;
	bool m_calcvoronoivertices = false;
	bool m_calcvoronoiregions = true;
	bool m_calcdelaunay = true;
	bool m_calckruskal = false;

	HullCalculationMethod m_hullcalculationmethod = HullCalculationMethod::QHULL;
	DelaunayCalculationMethod m_delaunaycalculationmethod = DelaunayCalculationMethod::QHULL;
	SpanCalculationMethod m_spancalculationmethod = SpanCalculationMethod::KRUSKAL;
};



class HullView : public QGraphicsView
{ Q_OBJECT
public:
	HullView(HullScene *scene=nullptr, QWidget *parent=nullptr);
	virtual ~HullView();

	HullView(HullView&) = delete;
	const HullView& operator=(const HullView&) const = delete;

	void UpdateAll();

protected:
	virtual void mousePressEvent(QMouseEvent *evt) override;
	virtual void mouseReleaseEvent(QMouseEvent *evt) override;
	virtual void mouseMoveEvent(QMouseEvent *evt) override;
	virtual void wheelEvent(QWheelEvent *evt) override;

	virtual void resizeEvent(QResizeEvent *evt) override;

private:
	HullScene *m_scene = nullptr;
	bool m_dragging = false;

signals:
	void SignalMouseCoordinates(t_real x, t_real y);
};



/**
 * convex hull calculation dialog
 */
class HullDlg : public QDialog
{ Q_OBJECT
public:
	using t_vec = ::t_vec;
	using t_mat = ::t_mat;

public:
	HullDlg(QWidget* pParent = nullptr);
	HullDlg(const HullDlg& other) = delete;
	virtual ~HullDlg() = default;

protected:
	std::shared_ptr<QTableWidget> m_tab{};
	std::shared_ptr<QPlainTextEdit> m_editResults{};
	std::shared_ptr<QCheckBox> m_checkDelaunay{};
	std::shared_ptr<QMenu> m_contextMenuTab{};

protected:
	virtual void accept() override;

protected:
	void AddTabItem(int row = -1);
	void DelTabItem();
	void MoveTabItemUp();
	void MoveTabItemDown();

	void TableCellChanged(int rowNew, int colNew, int rowOld, int colOld);
	void ShowTableContextMenu(const QPoint& pt);

	void SetDim(int dim);
	void CalculateHull();

private:
	std::vector<int> GetSelectedRows(bool sort_reversed = false) const;

private:
	QSettings m_sett{};

	int m_iCursorRow{-1};

};



/**
 * main window
 */
class HullWnd : public QMainWindow
{ Q_OBJECT
public:
	using QMainWindow::QMainWindow;

	HullWnd(QWidget* pParent = nullptr);
	virtual ~HullWnd();

	HullWnd(HullWnd&) = delete;
	const HullWnd& operator=(const HullWnd&) const = delete;

	void SetStatusMessage(const QString& msg);

protected:
	virtual void closeEvent(QCloseEvent *) override;
	void SetCurrentFile(const QString &file);

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

private:
	QSettings m_sett{};

	// recently opened files
	QMenu *m_menuOpenRecent{ nullptr };
	RecentFiles m_recent{};
	// function to call for the recent file menu items
	std::function<bool(const QString& filename)> m_open_func
		= [this](const QString& filename) -> bool
	{
		return this->OpenFile(filename);
	};

	std::shared_ptr<GeoAboutDlg> m_dlgAbout{};
	std::shared_ptr<GeoSettingsDlg> m_dlgSettings{};
	std::shared_ptr<HullDlg> m_hulldlg{};
	std::shared_ptr<HullScene> m_scene{};
	std::shared_ptr<HullView> m_view{};
	std::shared_ptr<QLabel> m_statusLabel{};
};

#endif

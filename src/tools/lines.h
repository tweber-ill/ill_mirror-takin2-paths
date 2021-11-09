/**
 * line intersection test program
 * @author Tobias Weber <tweber@ill.fr>
 * @date November-2020 - October-2021
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

#ifndef __LINES_GUI_H__
#define __LINES_GUI_H__

#include <QtCore/QSettings>
#include <QtGui/QImage>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QLabel>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QMenu>

#include <memory>
#include <vector>

#include "src/libs/lines.h"
#include "src/libs/graphs.h"
#include "src/libs/trapezoid.h"

#include "vertex.h"
#include "info.h"
#include "about.h"
#include "Settings.h"
#include "src/gui/Recent.h"

#define GeoSettingsDlg SettingsDlg


enum class IntersectionCalculationMethod
{
	DIRECT,
	SWEEP,
};


enum class VoronoiCalculationMethod
{
	BOOSTPOLY,
	CGAL,
	OVD,
};


class LinesScene : public QGraphicsScene
{ Q_OBJECT
public:
	using t_vec = ::t_vec2;
	using t_mat = ::t_mat22;
	using t_graph = geo::AdjacencyList<t_real>;

public:
	LinesScene(QWidget *parent=nullptr);
	virtual ~LinesScene();

	LinesScene(LinesScene&) = delete;
	const LinesScene& operator=(const LinesScene&) const = delete;

public:
	bool GetCalculateIntersections() const { return m_calcinters; }
	void SetCalculateIntersections(bool);

	bool GetCalculateTrapezoids() const { return m_calctrapezoids; }
	void SetCalculateTrapezoids(bool);

	bool GetCalculateVoro() const { return m_calcvoro; }
	void SetCalculateVoro(bool);

	bool GetCalculateVoroVertex() const { return m_calcvorovertex; }
	void SetCalculateVoroVertex(bool);

	bool GetStopOnInters() const { return m_stoponinters; }
	void SetStopOnInters(bool);

	bool GetRemoveVerticesInRegions() const { return m_removeverticesinregions; }
	void SetRemoveVerticesInRegions(bool);

	bool GetGroupLines() const { return m_grouplines; }
	void SetGroupLines(bool);

	void AddVertex(const QPointF& pos);
	void AddRegion(std::vector<t_vec>&& region);
	void AddGroup(std::pair<std::size_t, std::size_t>&& group);

	void ClearVertices();
	void ClearRegions();
	void ClearGroups();

	const std::vector<std::vector<t_vec>>& GetRegions() const { return m_regions; }
	void MakeRegionsFromGroups();

	const std::vector<Vertex*>& GetVertexElems() const { return m_elems_vertices; }
	std::vector<Vertex*>& GetVertexElems() { return m_elems_vertices; }

	void UpdateAll();
	void UpdateLines();
	void UpdateIntersections();
	void UpdateTrapezoids();

	void SetIntersectionCalculationMethod(IntersectionCalculationMethod m);
	void SetVoronoiCalculationMethod(VoronoiCalculationMethod m);

	void CreateVoroImage(int width, int height);
	void UpdateVoroImage(const QTransform& trafoSceneToVP);
	const QImage* GetVoroImage() const { return m_elem_voro; }

	void UpdateVoro();
	const t_graph& GetVoroGraph() const { return m_vorograph; }

	// infos
	std::size_t GetNumLines() const { return m_numLines; }
	std::size_t GetNumIntersections() const { return m_numIntersections; }
	std::size_t GetNumTrapezoids() const { return m_numTrapezoids; }
	std::size_t GetNumVoronoiVertices() const { return m_numVoronoiVertices; }
	std::size_t GetNumVoronoiLinearEdges() const { return m_numVoronoiLinearEdges; }
	std::size_t GetNumVoronoiParabolicEdges() const { return m_numVoronoiLParabolicEdges; }

private:
	QWidget *m_parent = nullptr;

	std::vector<Vertex*> m_elems_vertices{};
	std::vector<QGraphicsItem*> m_elems_lines{};
	std::vector<QGraphicsItem*> m_elems_inters{};
	std::vector<QGraphicsItem*> m_elems_trap{};
	std::vector<QGraphicsItem*> m_elems_voro{};
	QImage *m_elem_voro{nullptr};

	std::vector<std::pair<t_vec, t_vec>> m_lines{};
	std::vector<std::vector<t_vec>> m_regions{};
	std::vector<std::pair<std::size_t, std::size_t>> m_vertexgroups{};
	std::vector<std::pair<std::size_t, std::size_t>> m_linegroups{};

	t_graph m_vorograph{};
	bool m_calcinters = true;
	bool m_calcvoro = true;
	bool m_calcvorovertex = false;
	bool m_stoponinters = true;
	bool m_grouplines = false;
	bool m_removeverticesinregions = false;

	IntersectionCalculationMethod m_intersectioncalculationmethod
		= IntersectionCalculationMethod::SWEEP;
	VoronoiCalculationMethod m_voronoicalculationmethod
		= VoronoiCalculationMethod::BOOSTPOLY;
	bool m_calctrapezoids = false;

	// infos
	std::size_t m_numLines{};
	std::size_t m_numIntersections{};
	std::size_t m_numTrapezoids{};
	std::size_t m_numVoronoiVertices{};
	std::size_t m_numVoronoiLinearEdges{};
	std::size_t m_numVoronoiLParabolicEdges{};

private:
	std::size_t GetClosestLineIdx(const t_vec& pt) const;

signals:
	void CalculationFinished();
};


class LinesView : public QGraphicsView
{ Q_OBJECT
public:
	LinesView(LinesScene *scene=nullptr, QWidget *parent=nullptr);
	virtual ~LinesView();

	LinesView(LinesView&) = delete;
	const LinesView& operator=(const LinesView&) const = delete;

	void UpdateAll();

protected:
	virtual void mousePressEvent(QMouseEvent *evt) override;
	virtual void mouseReleaseEvent(QMouseEvent *evt) override;
	virtual void mouseMoveEvent(QMouseEvent *evt) override;
	virtual void wheelEvent(QWheelEvent *evt) override;

	virtual void resizeEvent(QResizeEvent *evt) override;

private:
	LinesScene *m_scene = nullptr;
	bool m_dragging = false;

protected:
	virtual void drawBackground(QPainter*, const QRectF&) override;

signals:
	void SignalMouseCoordinates(
		t_real scenex, t_real sceney, t_real vpx, t_real vpy,
		const std::vector<std::size_t>& cursor_regions,
		const std::vector<std::size_t>& vert_indices);
};


class LinesWnd : public QMainWindow
{ Q_OBJECT
public:
	using QMainWindow::QMainWindow;

	LinesWnd(QWidget* pParent = nullptr);
	~LinesWnd();

	void SetStatusMessage(const QString& msg);

protected:
	virtual void closeEvent(QCloseEvent *) override;
	void SetCurrentFile(const QString &file);

private:
	QSettings m_sett{"geo_tools", "lines"};

	// recently opened files
	QMenu *m_menuOpenRecent{ nullptr };
	RecentFiles m_recent{};
        // function to call for the recent file menu items
	std::function<bool(const QString& filename)> m_open_func
		= [this](const QString& filename) -> bool
	{
		return this->OpenFile(filename);
	};

	std::shared_ptr<GeoInfoDlg> m_dlgInfo{};
	std::shared_ptr<GeoAboutDlg> m_dlgAbout{};
	std::shared_ptr<GeoSettingsDlg> m_dlgSettings{};

	std::shared_ptr<LinesScene> m_scene{};
	std::shared_ptr<LinesView> m_view{};
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

	void UpdateInfos();
};


#endif

/**
 * line intersection test program
 * @author Tobias Weber <tweber@ill.fr>
 * @date 11-Nov-2020
 * @note Forked on 19-apr-2021 from my privately developed "geo" project (https://github.com/t-weber/geo).
 * @license see 'LICENSE' file
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

#include <memory>
#include <vector>

#include "src/libs/lines.h"
#include "src/libs/graphs.h"
#include "src/libs/trapezoid.h"

#include "about.h"
#include "vertex.h"


enum class IntersectionCalculationMethod
{
	DIRECT,
	SWEEP,
};


enum class VoronoiCalculationMethod
{
	BOOSTPOLY,
	OVD,
};



class LinesScene : public QGraphicsScene
{
public:
	using t_vec = tl2::vec<t_real, std::vector>;
	using t_mat = tl2::mat<t_real, std::vector>;
	using t_graph = geo::AdjacencyList<t_real>;

	static const constexpr t_real g_eps = 1e-5;

public:
	LinesScene(QWidget *parent=nullptr);
	virtual ~LinesScene();

	LinesScene(LinesScene&) = delete;
	const LinesScene& operator=(const LinesScene&) const = delete;

public:
	bool GetCalculateTrapezoids() const { return m_calctrapezoids; }
	void SetCalculateTrapezoids(bool);

	bool GetCalculateVoro() const { return m_calcvoro; }
	void SetCalculateVoro(bool);

	bool GetCalculateVoroVertex() const { return m_calcvorovertex; }
	void SetCalculateVoroVertex(bool);

	bool GetStopOnInters() const { return m_stoponinters; }
	void SetStopOnInters(bool);

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
	bool m_calcvoro = true;
	bool m_calcvorovertex = false;
	bool m_stoponinters = true;

	IntersectionCalculationMethod m_intersectioncalculationmethod
		= IntersectionCalculationMethod::SWEEP;
	VoronoiCalculationMethod m_voronoicalculationmethod
		= VoronoiCalculationMethod::BOOSTPOLY;
	bool m_calctrapezoids = false;

private:
	std::size_t GetClosestLineIdx(const t_vec& pt) const;
};



class LinesView : public QGraphicsView
{Q_OBJECT
public:
	LinesView(LinesScene *scene=nullptr, QWidget *parent=nullptr);
	virtual ~LinesView();

	LinesView(LinesView&) = delete;
	const LinesView& operator=(const LinesView&) const = delete;

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
		const std::vector<std::size_t>& cursor_regions);
};



class LinesWnd : public QMainWindow
{
public:
	using QMainWindow::QMainWindow;

	LinesWnd(QWidget* pParent = nullptr);
	~LinesWnd();

	void SetStatusMessage(const QString& msg);

private:
	virtual void closeEvent(QCloseEvent *) override;

private:
	QSettings m_sett{"geo_tools", "lines"};
	std::shared_ptr<AboutDlg> m_dlgAbout;

	std::shared_ptr<LinesScene> m_scene;
	std::shared_ptr<LinesView> m_view;
	std::shared_ptr<QLabel> m_statusLabel;
};


#endif

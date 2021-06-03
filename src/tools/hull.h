/**
 * convex hull test program
 * @author Tobias Weber <tweber@ill.fr>
 * @date 15-Aug-2020
 * @note Forked on 19-apr-2021 from my privately developed "geo" project (https://github.com/t-weber/geo).
 * @license see 'LICENSE' file
 */

#ifndef __HULL_GUI_H__
#define __HULL_GUI_H__


#include <QMainWindow>
#include <QDialog>
#include <QMenu>
#include <QLabel>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QTableWidget>
#include <QToolButton>
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QSettings>

#include <memory>
#include <unordered_set>
#include <vector>

#include "about.h"


class Vertex : public QGraphicsItem
{
public:
	Vertex(const QPointF& pos, double rad = 15.);
	virtual ~Vertex();

	virtual QRectF boundingRect() const override;
	virtual void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;

private:
	double m_rad = 15.;
};


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
{
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
{Q_OBJECT
public:
	HullView(HullScene *scene=nullptr, QWidget *parent=nullptr);
	virtual ~HullView();

	HullView(HullView&) = delete;
	const HullView& operator=(const HullView&) const = delete;

protected:
	virtual void mousePressEvent(QMouseEvent *evt) override;
	virtual void mouseReleaseEvent(QMouseEvent *evt) override;
	virtual void mouseMoveEvent(QMouseEvent *evt) override;

	virtual void resizeEvent(QResizeEvent *evt) override;

private:
	HullScene *m_scene = nullptr;
	bool m_dragging = false;

signals:
	void SignalMouseCoordinates(double x, double y);
};



/**
 * convex hull calculation dialog
 */
class HullDlg : public QDialog
{
public:
	HullDlg(QWidget* pParent = nullptr);
	virtual ~HullDlg() = default;

protected:
	QTableWidget *m_tab{};
	QPlainTextEdit *m_editResults{};
	QCheckBox *m_checkDelaunay{};
	QMenu *m_contextMenuTab{};

protected:
	virtual void closeEvent(QCloseEvent *) override;

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
	QSettings m_sett{"geo_tools", "hull"};

	int m_iCursorRow{-1};

};



/**
 * main window
 */
class HullWnd : public QMainWindow
{
public:
	using QMainWindow::QMainWindow;

	HullWnd(QWidget* pParent = nullptr);
	~HullWnd();

	void SetStatusMessage(const QString& msg);

protected:
	virtual void closeEvent(QCloseEvent *) override;

private:
	QSettings m_sett{"geo_tools", "hull"};

	std::shared_ptr<AboutDlg> m_dlgAbout;
	std::shared_ptr<HullDlg> m_hulldlg;
	std::shared_ptr<HullScene> m_scene;
	std::shared_ptr<HullView> m_view;
	std::shared_ptr<QLabel> m_statusLabel;
};

#endif

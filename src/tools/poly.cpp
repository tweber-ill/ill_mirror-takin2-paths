/**
 * polygon splitting and kernel calculation test program
 * @author Tobias Weber <tweber@ill.fr>
 * @date 11-nov-2020
 * @note Forked on 4-aug-2021 from my privately developed "geo" project (https://github.com/t-weber/geo).
 * @license see 'LICENSE' file
 */

#include "poly.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QLabel>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtGui/QMouseEvent>
#include <QtSvg/QSvgGenerator>

#include <locale>
#include <memory>
#include <array>
#include <vector>
#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
namespace ptree = boost::property_tree;

#include "tlibs2/libs/helper.h"



// ----------------------------------------------------------------------------

PolyView::PolyView(QGraphicsScene *scene, QWidget *parent) : QGraphicsView(scene, parent),
	m_scene{scene}
{
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

	setInteractive(true);
	setMouseTracking(true);

	setBackgroundBrush(QBrush{QColor::fromRgbF(0.95, 0.95, 0.95, 1.)});
}


PolyView::~PolyView()
{
}


void PolyView::resizeEvent(QResizeEvent *evt)
{
	QPointF pt1{mapToScene(QPoint{0,0})};
	QPointF pt2{mapToScene(QPoint{evt->size().width(), evt->size().height()})};

	const double padding = 16;

	// include bounds given by vertices
	for(const Vertex* vertex : m_elems_vertices)
	{
		QPointF vertexpos = vertex->scenePos();

		if(vertexpos.x() < pt1.x())
			pt1.setX(vertexpos.x() -  padding);
		if(vertexpos.x() > pt2.x())
			pt2.setX(vertexpos.x() +  padding);
		if(vertexpos.y() < pt1.y())
			pt1.setY(vertexpos.y() -  padding);
		if(vertexpos.y() > pt2.y())
			pt2.setY(vertexpos.y() +  padding);
	}

	setSceneRect(QRectF{pt1, pt2});
}



void PolyView::AddVertex(const QPointF& pos)
{
	Vertex *vertex = new Vertex{pos};
	m_elems_vertices.push_back(vertex);
	m_scene->addItem(vertex);
}


void PolyView::mousePressEvent(QMouseEvent *evt)
{
	QPoint posVP = evt->pos();
	QPointF posScene = mapToScene(posVP);

	QList<QGraphicsItem*> items = this->items(posVP);
	QGraphicsItem* item = nullptr;
	bool item_is_vertex = false;

	for(int itemidx=0; itemidx<items.size(); ++itemidx)
	{
		item = items[itemidx];
		auto iter = std::find(m_elems_vertices.begin(), m_elems_vertices.end(), static_cast<Vertex*>(item));
		item_is_vertex = (iter != m_elems_vertices.end());
		if(item_is_vertex)
			break;
	}

	// only select vertices
	if(!item_is_vertex)
		item = nullptr;


	if(evt->button() == Qt::LeftButton)
	{
		// if no vertex is at this position, create a new one
		if(!item)
		{
			AddVertex(posScene);
			m_dragging = true;
			UpdateAll();
		}

		else
		{
			// vertex is being dragged
			if(item_is_vertex)
			{
				m_dragging = true;
			}
		}
	}
	else if(evt->button() == Qt::RightButton)
	{
		// if a vertex is at this position, remove it
		if(item && item_is_vertex)
		{
			auto iter = std::find(m_elems_vertices.begin(), m_elems_vertices.end(), static_cast<Vertex*>(item));
			if(iter != m_elems_vertices.end())
			{
				m_scene->removeItem(item);
				delete item;
				iter = m_elems_vertices.erase(iter);
				UpdateAll();
			}
		}
	}

	QGraphicsView::mousePressEvent(evt);
}


void PolyView::mouseReleaseEvent(QMouseEvent *evt)
{
	if(evt->button() == Qt::LeftButton)
		m_dragging = false;

	UpdateAll();
	QGraphicsView::mouseReleaseEvent(evt);
}


void PolyView::mouseMoveEvent(QMouseEvent *evt)
{
	QGraphicsView::mouseMoveEvent(evt);

	if(m_dragging)
	{
		QResizeEvent evt{size(), size()};
		resizeEvent(&evt);
		UpdateAll();
	}

	QPoint posVP = evt->pos();
	QPointF posScene = mapToScene(posVP);
	emit SignalMouseCoordinates(posScene.x(), posScene.y());
}


void PolyView::ClearVertices()
{
	for(Vertex* vertex : m_elems_vertices)
	{
		m_scene->removeItem(vertex);
		delete vertex;
	}
	m_elems_vertices.clear();

	UpdateAll();
}


/**
 * calculate everything
 */
void PolyView::UpdateAll()
{
	// get vertices
	m_vertices.clear();
	m_vertices.reserve(m_elems_vertices.size());
	std::transform(m_elems_vertices.begin(), m_elems_vertices.end(), std::back_inserter(m_vertices),
		[](const Vertex* vert) -> t_vec { return tl2::create<t_vec>({vert->x(), vert->y()}); } );

	if(m_sortvertices)
		std::tie(m_vertices, std::ignore) = geo::sort_vertices_by_angle<t_vec>(m_vertices);

	UpdateEdges();
	UpdateSplitPolygon();
	UpdateKer();
}


/**
 * draw the polygon edges
 */
void PolyView::UpdateEdges()
{
	// remove previous edges
	for(QGraphicsItem* item : m_elems_edges)
	{
		m_scene->removeItem(item);
		delete item;
	}
	m_elems_edges.clear();


	QPen penEdge;
	penEdge.setStyle(Qt::SolidLine);
	penEdge.setWidthF(2.);
	penEdge.setColor(QColor::fromRgbF(0., 0., 1.));

	const std::size_t num_vertices = m_vertices.size();
	m_elems_edges.reserve(num_vertices);

	for(std::size_t vertidx = 0; vertidx < num_vertices; ++vertidx)
	{
		std::size_t vertidx2 = (vertidx+1) % num_vertices;

		const t_vec& vertex1 = m_vertices[vertidx];
		const t_vec& vertex2 = m_vertices[vertidx2];

		QLineF line{QPointF{vertex1[0], vertex1[1]}, QPointF{vertex2[0], vertex2[1]}};
		QGraphicsItem *item = m_scene->addLine(line, penEdge);
		m_elems_edges.push_back(item);
	}
}


/**
 * split the polygon into convex regions
 */
void PolyView::UpdateSplitPolygon()
{
	try
	{
		// remove previous split poly
		for(QGraphicsItem* item : m_elems_split)
		{
			m_scene->removeItem(item);
			delete item;
		}
		m_elems_split.clear();

		if(!m_splitpolygon)
			return;

		auto splitpolys = geo::convex_split<t_vec>(m_vertices, g_eps);

		// already convex?
		if(splitpolys.size() == 0)
			return;

		QPen penKer;
		penKer.setStyle(Qt::SolidLine);
		penKer.setWidthF(2.);
		penKer.setColor(QColor::fromRgbF(0., 0., 1., 1.));

		QBrush brushKer;
		brushKer.setColor(QColor::fromRgbF(0., 0., 1., 0.1));
		brushKer.setStyle(Qt::SolidPattern);

		for(const auto& splitpoly : splitpolys)
		{
			QPolygonF poly;
			for(std::size_t vertidx = 0; vertidx < splitpoly.size(); ++vertidx)
			{
				const t_vec& vertex1 = splitpoly[vertidx];
				poly << QPointF(vertex1[0], vertex1[1]);
			}

			QGraphicsItem *item = m_scene->addPolygon(poly, penKer, brushKer);
			m_elems_split.push_back(item);
		}
	}
	catch(const std::exception& ex)
	{
		std::ostringstream ostrErr;
		ostrErr << "Error: " << ex.what();

		std::cerr << ostrErr.str() << std::endl;
		emit SignalError(ostrErr.str().c_str());
	}
}


/**
 * calculate the kernel of the polygon
 */
void PolyView::UpdateKer()
{
	// remove previous vis poly
	for(QGraphicsItem* item : m_elems_ker)
	{
		m_scene->removeItem(item);
		delete item;
	}
	m_elems_ker.clear();

	if(!m_calckernel)
		return;

	std::vector<t_vec> verts_reversed;
	verts_reversed.reserve(m_vertices.size());
	for(auto iter=m_vertices.rbegin(); iter!=m_vertices.rend(); ++iter)
		verts_reversed.push_back(*iter);

	auto kerpoly = geo::calc_ker<t_vec>(m_vertices, g_eps);
	auto kerpoly_reversed = geo::calc_ker<t_vec>(verts_reversed, g_eps);

	// in case the vertices were inserted in reversed order
	if(kerpoly_reversed.size() > kerpoly.size())
		kerpoly = kerpoly_reversed;


	QPen penKer;
	penKer.setStyle(Qt::SolidLine);
	penKer.setWidthF(2.);
	penKer.setColor(QColor::fromRgbF(1., 0., 0., 1.));

	QBrush brushKer;
	brushKer.setColor(QColor::fromRgbF(1., 0., 0., 0.1));
	brushKer.setStyle(Qt::SolidPattern);

	QPolygonF poly;
	for(std::size_t vertidx = 0; vertidx < kerpoly.size(); ++vertidx)
	{
		const t_vec& vertex1 = kerpoly[vertidx];
		poly << QPointF(vertex1[0], vertex1[1]);
	}

	QGraphicsItem *item = m_scene->addPolygon(poly, penKer, brushKer);
	m_elems_ker.push_back(item);
}


void PolyView::SetSortVertices(bool b)
{
	m_sortvertices = b;
	UpdateAll();
}


void PolyView::SetCalcSplitPolygon(bool b)
{
	m_splitpolygon = b;
	UpdateAll();
}


void PolyView::SetCalcKernel(bool b)
{
	m_calckernel = b;
	UpdateAll();
}

// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------

PolyWnd::PolyWnd(QWidget* pParent) : QMainWindow{pParent},
	m_scene{new QGraphicsScene{this}},
	m_view{new PolyView{m_scene.get(), this}},
	m_statusLabel{std::make_shared<QLabel>(this)}
{
	// ------------------------------------------------------------------------
	// restore settings
	SettingsDlg::ReadSettings(&m_sett);

	if(m_sett.contains("wnd_geo"))
	{
		QByteArray arr{m_sett.value("wnd_geo").toByteArray()};
		this->restoreGeometry(arr);
	}
	else
	{
		resize(1024, 768);
	}
	if(m_sett.contains("wnd_state"))
	{
		QByteArray arr{m_sett.value("wnd_state").toByteArray()};
		this->restoreState(arr);
	}

	m_view->SetSortVertices(
		m_sett.value("sort_vertices", m_view->GetSortVertices()).toBool());
	// ------------------------------------------------------------------------


	m_view->setRenderHints(QPainter::Antialiasing);

	setWindowTitle("Polygons");
	setCentralWidget(m_view.get());

	QStatusBar *statusBar = new QStatusBar{this};
	statusBar->addPermanentWidget(m_statusLabel.get(), 1);
	setStatusBar(statusBar);


	// menu actions
	QAction *actionNew = new QAction{QIcon::fromTheme("document-new"), "New", this};
	connect(actionNew, &QAction::triggered, [this]() { m_view->ClearVertices(); });

	QAction *actionLoad = new QAction{QIcon::fromTheme("document-open"), "Open...", this};
	connect(actionLoad, &QAction::triggered, [this]()
	{
		if(QString file = QFileDialog::getOpenFileName(this, "Load Data", "",
			"XML Files (*.xml);;All Files (* *.*)"); file!="")
		{
			std::ifstream ifstr(file.toStdString());
			if(!ifstr)
			{
				QMessageBox::critical(this, "Error", "File could not be opened for loading.");
				return;
			}

			m_view->ClearVertices();

			ptree::ptree prop{};
			ptree::read_xml(ifstr, prop);

			std::size_t vertidx = 0;
			while(true)
			{
				std::ostringstream ostrVert;
				ostrVert << "vis2d.vertices." << vertidx;

				auto vertprop = prop.get_child_optional(ostrVert.str());
				if(!vertprop)
					break;

				auto vertx = vertprop->get_optional<t_real>("<xmlattr>.x");
				auto verty = vertprop->get_optional<t_real>("<xmlattr>.y");

				if(!vertx || !verty)
					break;

				m_view->AddVertex(QPointF{*vertx, *verty});

				++vertidx;
			}

			if(vertidx > 0)
				m_view->UpdateAll();
			else
				QMessageBox::warning(this, "Warning", "File contains no data.");
		}
	});

	QAction *actionSaveAs = new QAction{QIcon::fromTheme("document-save-as"), "Save as...", this};
	connect(actionSaveAs, &QAction::triggered, [this]()
	{
		if(QString file = QFileDialog::getSaveFileName(this, "Save Data", "",
			"XML Files (*.xml);;All Files (* *.*)"); file!="")
		{
			std::ofstream ofstr(file.toStdString());
			if(!ofstr)
			{
				QMessageBox::critical(this, "Error", "File could not be opened for saving.");
				return;
			}

			ptree::ptree prop{};

			std::size_t vertidx = 0;
			for(const Vertex* vertex : m_view->GetVertexElems())
			{
				QPointF vertexpos = vertex->scenePos();

				std::ostringstream ostrX, ostrY;
				ostrX << "vis2d.vertices." << vertidx << ".<xmlattr>.x";
				ostrY << "vis2d.vertices." << vertidx << ".<xmlattr>.y";

				prop.put<t_real>(ostrX.str(), vertexpos.x());
				prop.put<t_real>(ostrY.str(), vertexpos.y());

				++vertidx;
			}

			ptree::write_xml(ofstr, prop, ptree::xml_writer_make_settings('\t', 1, std::string{"utf-8"}));
		}
	});

	QAction *actionExportSvg = new QAction{QIcon::fromTheme("image-x-generic"), "Export SVG...", this};
	connect(actionExportSvg, &QAction::triggered, [this]()
	{
		if(QString file = QFileDialog::getSaveFileName(this, "Export SVG", "",
			"SVG Files (*.svg);;All Files (* *.*)"); file!="")
		{
			QSvgGenerator svggen;
			svggen.setSize(QSize{width(), height()});
			svggen.setFileName(file);

			QPainter paint(&svggen);
			m_scene->render(&paint);
		}
	});

	QAction *actionSettings = new QAction(QIcon::fromTheme("preferences-system"), "Settings...", this);
	actionSettings->setMenuRole(QAction::PreferencesRole);
	connect(actionSettings, &QAction::triggered, this, [this]()
	{
		if(!this->m_dlgSettings)
			this->m_dlgSettings = std::make_shared<SettingsDlg>(this, &m_sett);

		m_dlgSettings->show();
		m_dlgSettings->raise();
		m_dlgSettings->activateWindow();
	});

	QAction *actionQuit = new QAction{QIcon::fromTheme("application-exit"), "Quit", this};
	actionQuit->setMenuRole(QAction::QuitRole);
	connect(actionQuit, &QAction::triggered, [this]() { this->close(); });


	QAction *actionZoomIn = new QAction{QIcon::fromTheme("zoom-in"), "Zoom in", this};
	connect(actionZoomIn, &QAction::triggered, [this]()
	{
		if(!m_view)
			return;
		m_view->scale(2., 2.);
	});

	QAction *actionZoomOut = new QAction{QIcon::fromTheme("zoom-out"), "Zoom out", this};
	connect(actionZoomOut, &QAction::triggered, [this]()
	{
		if(!m_view)
			return;
		m_view->scale(0.5, 0.5);
	});

	QAction *actionIncreaseVertexSize = new QAction{"Increase Vertex Size", this};
	connect(actionIncreaseVertexSize, &QAction::triggered, [this]()
	{
		if(!m_scene)
			return;

		for(Vertex* vert : m_view->GetVertexElems())
		{
			t_real rad = vert->GetRadius();
			rad *= t_real(2.);
			vert->SetRadius(rad);
		}

		m_scene->update(QRectF());
	});

	QAction *actionDecreaseVertexSize = new QAction{"Decrease Vertex Size", this};
	connect(actionDecreaseVertexSize, &QAction::triggered, [this]()
	{
		if(!m_scene)
			return;

		for(Vertex* vert : m_view->GetVertexElems())
		{
			t_real rad = vert->GetRadius();
			rad *= t_real(0.5);
			vert->SetRadius(rad);
		}

		m_scene->update(QRectF());
	});


	QAction *actionSort = new QAction{"Sort Vertices", this};
	actionSort->setCheckable(true);
	actionSort->setChecked(m_view->GetSortVertices());
	connect(actionSort, &QAction::toggled, [this](bool b) { m_view->SetSortVertices(b); });

	QAction *actionSplit = new QAction{"Convex Regions", this};
	actionSplit->setCheckable(true);
	actionSplit->setChecked(m_view->GetCalcSplitPolygon());
	connect(actionSplit, &QAction::toggled, [this](bool b) { m_view->SetCalcSplitPolygon(b); });

	QAction *actionKer = new QAction{"Visibility Kernel", this};
	actionKer->setCheckable(true);
	actionKer->setChecked(m_view->GetCalcKernel());
	connect(actionKer, &QAction::toggled, [this](bool b) { m_view->SetCalcKernel(b); });


	QAction *actionAboutQt = new QAction(QIcon::fromTheme("help-about"), "About Qt Libraries...", this);
	QAction *actionAbout = new QAction(QIcon::fromTheme("help-about"), "About Program...", this);

	actionAboutQt->setMenuRole(QAction::AboutQtRole);
	actionAbout->setMenuRole(QAction::AboutRole);

	connect(actionAboutQt, &QAction::triggered, this, []() { qApp->aboutQt(); });

	connect(actionAbout, &QAction::triggered, this, [this]()
	{
		if(!this->m_dlgAbout)
			this->m_dlgAbout = std::make_shared<AboutDlg>(this, &m_sett);

		m_dlgAbout->show();
		m_dlgAbout->raise();
		m_dlgAbout->activateWindow();
	});


	// shortcuts
	actionNew->setShortcut(QKeySequence::New);
	actionLoad->setShortcut(QKeySequence::Open);
	//actionSave->setShortcut(QKeySequence::Save);
	actionSaveAs->setShortcut(QKeySequence::SaveAs);
	actionSettings->setShortcut(QKeySequence::Preferences);
	actionQuit->setShortcut(QKeySequence::Quit);
	actionZoomIn->setShortcut(QKeySequence::ZoomIn);
	actionZoomOut->setShortcut(QKeySequence::ZoomOut);


	// menu
	QMenu *menuFile = new QMenu{"File", this};
	QMenu *menuView = new QMenu{"View", this};
	QMenu *menuCalc = new QMenu{"Calculate", this};
	QMenu *menuHelp = new QMenu("Help", this);

	menuFile->addAction(actionNew);
	menuFile->addSeparator();
	menuFile->addAction(actionLoad);
	menuFile->addAction(actionSaveAs);
	menuFile->addSeparator();
	menuFile->addAction(actionExportSvg);
	menuFile->addSeparator();
	menuFile->addAction(actionSettings);
	menuFile->addSeparator();
	menuFile->addAction(actionQuit);

	menuView->addAction(actionZoomIn);
	menuView->addAction(actionZoomOut);
	menuView->addSeparator();
	menuView->addAction(actionIncreaseVertexSize);
	menuView->addAction(actionDecreaseVertexSize);

	menuCalc->addAction(actionSort);
	menuCalc->addSeparator();
	menuCalc->addAction(actionSplit);
	menuCalc->addAction(actionKer);

	menuHelp->addAction(actionAboutQt);
	menuHelp->addSeparator();
	menuHelp->addAction(actionAbout);


	// menu bar
	QMenuBar *menuBar = new QMenuBar{this};
	menuBar->setNativeMenuBar(false);
	menuBar->addMenu(menuFile);
	menuBar->addMenu(menuView);
	menuBar->addMenu(menuCalc);
	menuBar->addMenu(menuHelp);
	setMenuBar(menuBar);


	// connections
	connect(m_view.get(), &PolyView::SignalMouseCoordinates, [this](double x, double y) -> void
	{
		SetStatusMessage(QString("x=%1, y=%2.").arg(x, 5).arg(y, 5));
	});

	connect(m_view.get(), &PolyView::SignalError, this, &PolyWnd::SetStatusMessage);


	SetStatusMessage("Ready.");
}


void PolyWnd::SetStatusMessage(const QString& msg)
{
	m_statusLabel->setText(msg);
}


void PolyWnd::closeEvent(QCloseEvent *e)
{
	// ------------------------------------------------------------------------
	// save settings
	QByteArray geo{this->saveGeometry()}, state{this->saveState()};
	m_sett.setValue("wnd_geo", geo);
	m_sett.setValue("wnd_state", state);
	m_sett.setValue("sort_vertices", m_view->GetSortVertices());
	// ------------------------------------------------------------------------

	QMainWindow::closeEvent(e);
}


PolyWnd::~PolyWnd()
{
}

// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------

int main(int argc, char** argv)
{
	try
	{
		auto app = std::make_unique<QApplication>(argc, argv);
		app->setOrganizationName("tw");
		app->setApplicationName("poly");
		tl2::set_locales();

		auto vis = std::make_unique<PolyWnd>();
		vis->show();

		return app->exec();
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;
	}

	return -1;
}

// ----------------------------------------------------------------------------

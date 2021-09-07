/**
 * path properties dock widget
 * @author Tobias Weber <tweber@ill.fr>
 * @date apr-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __PATH_PROP_WIDGET_H__
#define __PATH_PROP_WIDGET_H__

#include <memory>
#include <QtWidgets/QWidget>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QSlider>

#include "src/core/types.h"


class PathPropertiesWidget : public QWidget
{Q_OBJECT
public:
	PathPropertiesWidget(QWidget *parent=nullptr);
	virtual ~PathPropertiesWidget();

	PathPropertiesWidget(const PathPropertiesWidget&) = delete;
	const PathPropertiesWidget& operator=(const PathPropertiesWidget&) = delete;


private:
	// number of coordinate elements
	static constexpr std::size_t m_num_coord_elems = 2;

	// path target (a2, a4) coordinates
	QDoubleSpinBox *m_spinFinish[m_num_coord_elems]{nullptr, nullptr};
	QSlider *m_sliderPath = nullptr;


public slots:
	void SetTarget(t_real a2, t_real a4);
	void PathAvailable(std::size_t numVertices);


signals:
	void TargetChanged(t_real a2, t_real a4);
	void GotoAngles(t_real a2, t_real a4);
	void CalculatePathMesh();
	void CalculatePath();
	void TrackPath(std::size_t);
};



class PathPropertiesDockWidget : public QDockWidget
{
public:
	PathPropertiesDockWidget(QWidget *parent=nullptr);
	virtual ~PathPropertiesDockWidget();

	PathPropertiesDockWidget(const PathPropertiesDockWidget&) = delete;
	const PathPropertiesDockWidget& operator=(const PathPropertiesDockWidget&) = delete;

	std::shared_ptr<PathPropertiesWidget> GetWidget() { return m_widget; }


private:
    std::shared_ptr<PathPropertiesWidget> m_widget;
};


#endif

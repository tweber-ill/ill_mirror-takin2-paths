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

#include "src/core/types.h"


class PathPropertiesWidget : public QWidget
{Q_OBJECT
public:
	PathPropertiesWidget(QWidget *parent=nullptr);
	virtual ~PathPropertiesWidget();

public slots:
	void SetStart(t_real a2, t_real a4);
	void SetFinish(t_real a2, t_real a4);

signals:
	void StartChanged(t_real a2, t_real a4);
	void FinishChanged(t_real a2, t_real a4);
	void GotoAngles(t_real a2, t_real a4);

private:
	// number of coordinate elements
	static constexpr std::size_t m_num_coord_elems = 2;

	// path start and finish (a2, a4) coordinates
	QDoubleSpinBox *m_spinStart[m_num_coord_elems]{nullptr, nullptr};
	QDoubleSpinBox *m_spinFinish[m_num_coord_elems]{nullptr, nullptr};
};


class PathPropertiesDockWidget : public QDockWidget
{
public:
	PathPropertiesDockWidget(QWidget *parent=nullptr);
	virtual ~PathPropertiesDockWidget();

	std::shared_ptr<PathPropertiesWidget> GetWidget() { return m_widget; }

private:
    std::shared_ptr<PathPropertiesWidget> m_widget;
};


#endif

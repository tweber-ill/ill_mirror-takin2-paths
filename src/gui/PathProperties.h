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

#include "src/core/types.h"


class PathPropertiesWidget : public QWidget
{Q_OBJECT
public:
	PathPropertiesWidget(QWidget *parent=nullptr);
	virtual ~PathPropertiesWidget();

public slots:
	void SetStart(t_real h, t_real k, t_real l, t_real ki, t_real kf);
	void SetFinish(t_real h, t_real k, t_real l, t_real ki, t_real kf);

signals:
	void StartChanged(t_real h, t_real k, t_real l, t_real ki, t_real kf);
	void FinishChanged(t_real h, t_real k, t_real l, t_real ki, t_real kf);
	void Goto(t_real h, t_real k, t_real l, t_real ki, t_real kf);

private:
	// number of coordinate elements
	static constexpr std::size_t m_num_coord_elems = 5;

	// path start (h, k, l, ki, kf) coordinates
	QDoubleSpinBox *m_spinStart[m_num_coord_elems]
		{nullptr, nullptr, nullptr, nullptr, nullptr};

	// path finish (h, k, l, ki, kf) coordinates
	QDoubleSpinBox *m_spinFinish[m_num_coord_elems]
		{nullptr, nullptr, nullptr, nullptr, nullptr};
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

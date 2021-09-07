/**
 * xtal position properties dock widget
 * @author Tobias Weber <tweber@ill.fr>
 * @date apr-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __COORD_PROP_WIDGET_H__
#define __COORD_PROP_WIDGET_H__

#include <memory>
#include <QtWidgets/QWidget>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QCheckBox>

#include "src/core/types.h"


class CoordPropertiesWidget : public QWidget
{Q_OBJECT
public:
	CoordPropertiesWidget(QWidget *parent=nullptr);
	virtual ~CoordPropertiesWidget();

	CoordPropertiesWidget(const CoordPropertiesWidget&) = delete;
        CoordPropertiesWidget& operator=(const CoordPropertiesWidget&) = delete;


public slots:
	void SetCoordinates(t_real h, t_real k, t_real l, t_real ki, t_real kf);


signals:
	void CoordinatesChanged(t_real h, t_real k, t_real l, t_real ki, t_real kf);
	void GotoCoordinates(t_real h, t_real k, t_real l, 
		t_real ki, t_real kf, bool only_set_target);


private:
	// number of coordinate elements
	static constexpr std::size_t m_num_coord_elems = 6;

	// (h, k, l, ki, kf, E) coordinates
	QDoubleSpinBox *m_spinCoords[m_num_coord_elems]
		{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
	QCheckBox *m_checkKfFixed{nullptr};
};



class CoordPropertiesDockWidget : public QDockWidget
{
public:
	CoordPropertiesDockWidget(QWidget *parent=nullptr);
	virtual ~CoordPropertiesDockWidget();

	CoordPropertiesDockWidget(const CoordPropertiesDockWidget&) = delete;
	const CoordPropertiesDockWidget& operator=(const CoordPropertiesDockWidget&) = delete;

	std::shared_ptr<CoordPropertiesWidget> GetWidget()
	{ return m_widget; }


private:
    std::shared_ptr<CoordPropertiesWidget> m_widget;
};


#endif

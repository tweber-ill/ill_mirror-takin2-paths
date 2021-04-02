/**
 * perperties dock widget
 * @author Tobias Weber <tweber@ill.fr>
 * @date mar-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __PROP_WIDGET_H__
#define __PROP_WIDGET_H__

#include <memory>
#include <QtWidgets/QWidget>
#include <QtWidgets/QDockWidget>


class PropertiesWidget : public QWidget
{
public:
	PropertiesWidget(QWidget* parent=nullptr);
	virtual ~PropertiesWidget();
};


class PropertiesDockWidget : public QDockWidget
{
public:
	PropertiesDockWidget(QWidget *parent=nullptr);
	virtual ~PropertiesDockWidget();

private:
    std::unique_ptr<PropertiesWidget> m_widget;
};


#endif

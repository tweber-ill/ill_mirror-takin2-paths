/**
 * perperties dock widget
 * @author Tobias Weber <tweber@ill.fr>
 * @date mar-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "Properties.h"


// --------------------------------------------------------------------------------
// properties widget
// --------------------------------------------------------------------------------
PropertiesWidget::PropertiesWidget(QWidget *parent)
	: QWidget{parent}
{
}


PropertiesWidget::~PropertiesWidget()
{
}
// --------------------------------------------------------------------------------



// --------------------------------------------------------------------------------
// properties dock widget
// --------------------------------------------------------------------------------
PropertiesDockWidget::PropertiesDockWidget(QWidget *parent)
	: QDockWidget{parent}, 
		m_widget{std::make_unique<PropertiesWidget>(this)}
{
	setObjectName("PropertiesDockWidget");
	setWindowTitle("Properties");

	setWidget(m_widget.get());
}


PropertiesDockWidget::~PropertiesDockWidget()
{
}
// --------------------------------------------------------------------------------

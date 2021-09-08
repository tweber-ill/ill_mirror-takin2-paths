/**
 * draggable vertex for the gui
 * @author Tobias Weber <tweber@ill.fr>
 * @date 11-Nov-2020
 * @note Forked on 19-apr-2021 from my privately developed "geo" project (https://github.com/t-weber/geo).
 * @license GPLv3, see 'LICENSE' file
 *
 * ----------------------------------------------------------------------------
 * TAS-Paths (part of the Takin software suite) and private "Geo" project
 * Copyright (C) 2021  Tobias WEBER (Institut Laue-Langevin (ILL), 
 *                     Grenoble, France).
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

#ifndef __GUI_VERTEX_H__
#define __GUI_VERTEX_H__

#include <QtWidgets/QGraphicsItem>
#include <QtGui/QPainter>

#include "../core/types.h"


class Vertex : public QGraphicsItem
{
public:
	Vertex(const QPointF& pos, t_real rad = 16.);
	virtual ~Vertex();

	virtual QRectF boundingRect() const override;
	virtual void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;

	t_real GetRadius() const;
	void SetRadius(t_real rad);

private:
	t_real m_rad = 16.;
};


#endif

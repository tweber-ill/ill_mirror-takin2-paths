/**
 * draggable vertex for the gui
 * @author Tobias Weber <tweber@ill.fr>
 * @date 11-Nov-2020
 * @note Forked on 19-apr-2021 from my privately developed "geo" project (https://github.com/t-weber/geo).
 * @license see 'LICENSE' file
 */

#ifndef __GUI_VERTEX_H__
#define __GUI_VERTEX_H__

#include <QtWidgets/QGraphicsItem>
#include <QtGui/QPainter>

using t_real = double;


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

#include "edge.h"
#include <math.h>

#include <iostream>

Edge::Edge(const Gradient &grad,
           const Vertex &min_y,
           const Vertex &max_y,
           int index)
{
    m_ystart = (int)ceil(min_y.pos.y);
    m_yend = (int)ceil(max_y.pos.y);

    float ydist = max_y.pos.y - min_y.pos.y;
    float xdist = max_y.pos.x - min_y.pos.x;

    float yprestep = m_ystart - min_y.pos.y;

    m_xstep = xdist / ydist;
    m_x = min_y.pos.x + yprestep * m_xstep;

    float xprestep = m_x - min_y.pos.x;

    m_bary = grad.bary(index) +
             (grad.barystep_x() * xprestep) +
             (grad.barystep_y() * yprestep);


    m_barystep = grad.barystep_y() +
                 (grad.barystep_x() * m_xstep);

}

void Edge::step()
{
    m_x += m_xstep;
    m_bary += m_barystep;
}


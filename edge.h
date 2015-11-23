#ifndef EDGE_H
#define EDGE_H

#include "gradient.h"
#include "vertex.h"
#include <glm/glm.hpp>

class Edge
{
public:
    Edge(const Gradient &grad,
         const Vertex &min_y,
         const Vertex &max_y,
         int index);
    void step();
    int ystart() const {return m_ystart;}
    int yend() const {return m_yend;}
    float x() const {return m_x;}
    glm::vec3 color() const {return m_color;}

private:
    float m_x;
    float m_xstep;
    int m_ystart;
    int m_yend;

    glm::vec3 m_color;
    glm::vec3 m_colorstep;

};

#endif // EDGE_H

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
    glm::vec3 bary() const {return m_bary;}

private:
    float m_x;
    float m_xstep;
    int m_ystart;
    int m_yend;

    glm::vec3 m_bary;
    glm::vec3 m_barystep;

};

#endif // EDGE_H

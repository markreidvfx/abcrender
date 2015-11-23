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
    float one_overz() const {return m_one_overz;}
    glm::vec2 uv() const {return m_uv;}
private:
    float m_x;
    float m_xstep;
    int m_ystart;
    int m_yend;

    glm::vec3 m_color;
    glm::vec3 m_colorstep;

    float m_one_overz;
    float m_one_overz_step;

    glm::vec2 m_uv;
    glm::vec2 m_uvstep;

};

#endif // EDGE_H

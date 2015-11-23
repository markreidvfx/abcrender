#ifndef GRADIENT_H
#define GRADIENT_H

#include "vertex.h"
#include <glm/glm.hpp>

class Gradient
{
public:
    Gradient(const Vertex &min_y,
             const Vertex &mid_y,
             const Vertex &max_y);

    glm::vec3 bary(int index) const { return m_bary[index];}
    glm::vec3 barystep_x() const { return m_barystep_x; }
    glm::vec3 barystep_y() const { return m_barystep_y; }

    glm::vec2 uv[3];
    float depth[3];
    float one_over_z[3];

private:
    glm::vec3 m_barystep_x;
    glm::vec3 m_barystep_y;
    glm::vec3 m_bary[3];

    float calc_xstep(const glm::vec3 &values,
                     const Vertex& min,
                     const Vertex& mid,
                     const Vertex& max,
                     float inv_dx);
    float calc_ystep(const glm::vec3 &values,
                     const Vertex& min,
                     const Vertex& mid,
                     const Vertex& max,
                     float inv_dy);
};

#endif // GRADIENT_H

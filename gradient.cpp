#include "gradient.h"

Gradient::Gradient(const Vertex &min_y,
                   const Vertex &mid_y,
                   const Vertex &max_y)
{
    float inv_dx = 1.0f / (
              ((mid_y.pos.x - max_y.pos.x) * (min_y.pos.y - max_y.pos.y)) -
              ((min_y.pos.x - max_y.pos.x) * (mid_y.pos.y - max_y.pos.y))
              );

    float inv_dy = -inv_dx;

    m_colors[0] = glm::vec3(1,0,0);
    m_colors[1] = glm::vec3(0,1,0);
    m_colors[2] = glm::vec3(0,0,1);

    depth[0] = min_y.pos.z;
    depth[1] = mid_y.pos.z;
    depth[2] = max_y.pos.z;

    one_over_z[0] = 1.0f / min_y.pos.w;
    one_over_z[1] = 1.0f / mid_y.pos.w;
    one_over_z[2] = 1.0f / max_y.pos.w;

    uv[0] = min_y.uv * one_over_z[0];
    uv[1] = mid_y.uv * one_over_z[1];
    uv[2] = max_y.uv * one_over_z[2];

    for (int i =0; i < 3; i++) {
        glm::vec3 values(m_colors[0][i],
                         m_colors[1][i],
                         m_colors[2][i]);

        m_colorstep_x[i] = calc_xstep(values, min_y, mid_y, max_y, inv_dx);
        m_colorstep_y[i] = calc_ystep(values, min_y, mid_y, max_y, inv_dy);
    }

}

float Gradient::calc_xstep(const glm::vec3 &values,
                           const Vertex& min,
                           const Vertex& mid,
                           const Vertex& max,
                           float inv_dx)
{
    return (((values[1] - values[2]) * (min.pos.y - max.pos.y)) -
            ((values[0] - values[2]) * (mid.pos.y - max.pos.y))) * inv_dx;
}

float Gradient::calc_ystep(const glm::vec3 &values,
                           const Vertex& min,
                           const Vertex& mid,
                           const Vertex& max,
                           float inv_dy)
{
    return (((values[1] - values[2]) * (min.pos.x - max.pos.x)) -
            ((values[0] - values[2]) * (mid.pos.x - max.pos.x))) * inv_dy;
}

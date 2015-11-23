#include "rendercontext.h"
#include <iostream>
#include <glm/gtx/string_cast.hpp>
#include <string>
#include <cfloat>

#define FILL_DEPTH FLT_MAX

RenderContext::RenderContext(int width, int height)
{
    resize(width, height);
    texture = NULL;
}

void RenderContext::resize(int width, int height)
{
    data.resize(width * height *4);
    depth.resize(width * height);
    m_width = width;
    m_height = height;
    clear();
}

void RenderContext::clear()
{
    std::fill(data.begin(), data.end(), 0);
    std::fill(depth.begin(), depth.end(), FILL_DEPTH);
}

glm::vec4 RenderContext::get_pixel_linear(float x, float y) const
{
    glm::vec4 color;

    int px = (int)(x); //floor
    int py = (int)(y); //floor

    glm::vec4 c[4];
    c[0] = get_pixel(px + 0, py + 0);
    c[1] = get_pixel(px + 1, py + 0);
    c[2] = get_pixel(px + 0, py + 1);
    c[3] = get_pixel(px + 1, py + 1);

    float fx = x - px;
    float fy = y - py;
    float fx1 = 1.0f - fx;
    float fy1 = 1.0f - fy;

    float w[4];

    w[0] = fx1 * fy1;
    w[1] = fx  * fy1;
    w[2] = fx1 * fy;
    w[3] = fx  * fy;

    for (int i = 0; i < 4; i++) {
        color[i] = (c[0][i] * w[0] ) +
                   (c[1][i] * w[1] ) +
                   (c[2][i] * w[2] ) +
                   (c[3][i] * w[3] );
    }
    //color.a = 1;

    return color;
}

glm::vec4 RenderContext::get_pixel(int x, int y) const
{
    glm::vec4 color;
    if (y < 0 || y >= m_height)
        return color;

    if (x < 0 || x >= m_width)
        return color;

    int index = (x + (y * m_width)) * 4;

    if (!(data[index + 3] > 0))
        return color;

    color.r = data[index    ];
    color.g = data[index + 1];
    color.b = data[index + 2];
    color.a = data[index + 3];

    //memcpy((void*)&data[index], (void*)&color[0], sizeof(glm::vec4));

    return color;
}

float RenderContext::get_depth(int x, int y) const
{
    float value = FILL_DEPTH;
    if (y < 0 || y >= m_height)
        return value;

    if (x < 0 || x >= m_width)
        return value;

    int index = x + (y * m_width);
    return depth[index];
}

void RenderContext::draw_pixel(int x, int y, const glm::vec4 &color)
{
    if (y < 0 || y >= m_height)
        return;

    if (x < 0 || x >= m_width)
        return;

    int index = (x + (y * m_width)) * 4;

    data[index    ] = color.r;
    data[index + 1] = color.g;
    data[index + 2] = color.b;
    data[index + 3] = color.a;


}

void RenderContext::draw_depth(int x, int y, float value)
{
    if (y < 0 || y >= m_height)
        return;

    if (x < 0 || x >= m_width)
        return;

    int index = x + (y * m_width);
    depth[index] = value;
}

void RenderContext::draw_triangle(const Vertex &v1, const Vertex &v2, const Vertex &v3)
{
    const Vertex *min = &v1;
    const Vertex *mid = &v2;
    const Vertex *max = &v3;

    // cull back facing polygons
    if (min->area_x2(*max, *mid) <= 0) {
        return;
    }

    // Sort triangles in y
    if(max->pos.y < mid->pos.y) {
        const Vertex *temp = max;
        max = mid;
        mid = temp;
    }

    if(mid->pos.y < min->pos.y) {
        const Vertex *temp = mid;
        mid = min;
        min = temp;
    }

    if(max->pos.y < mid->pos.y) {
        const Vertex *temp = max;
        max = mid;
        mid = temp;
    }

    //std::cerr << "min " << glm::to_string(min->pos) << "\n";
    //std::cerr << "mid " << glm::to_string(mid.pos) << "\n";
    //std::cerr << "max " << glm::to_string(max.pos) << "\n";

    scan_triangle(*min, *mid, *max,
                  min->area_x2(*max, *mid) >= 0);
}

void RenderContext::scan_triangle(const Vertex &min_y,
                                  const Vertex &mid_y,
                                  const Vertex &max_y,
                                  bool handedness)
{
    Gradient grad(min_y, mid_y, max_y);
    Edge top_bottom(grad, min_y, max_y, 0);
    Edge top_middle(grad, min_y, mid_y, 0);
    Edge middle_bottom(grad, mid_y, max_y, 1);

    scan_edge(grad, top_bottom, top_middle, handedness);
    scan_edge(grad, top_bottom, middle_bottom, handedness);

}

void RenderContext::scan_edge(const Gradient &grad,
                              Edge &a,
                              Edge &b,
                              bool handedness)
{
    Edge *left = &a;
    Edge *right = &b;

    if (handedness) {
        Edge *temp = left;
        left = right;
        right = temp;
    }

    int ystart = b.ystart();
    int yend = b.yend();

    for (int y = ystart; y < yend; y++) {
        draw_scanline(grad, *left, *right, y);
        left->step();
        right->step();
    }
}

void RenderContext::draw_scanline(const Gradient &grad,
                                  const Edge &left,
                                  const Edge &right,
                                  float y)
{
    int xmin = (int)ceil(left.x());
    int xmax = (int)ceil(right.x());

    float xprestep = (float)xmin - (float)left.x();

    glm::vec3 bary_step = grad.colorstep_x();
    glm::vec3 bary = left.color() + (grad.colorstep_x() * xprestep);

    for(int x = xmin; x < xmax; x++) {
        float depth = (grad.depth[0] * bary.x) +
                      (grad.depth[1] * bary.y) +
                      (grad.depth[2] * bary.z);

        if (depth > get_depth(x, y))
            continue;

        float one_over_z = (grad.one_over_z[0] * bary.x) +
                           (grad.one_over_z[1] * bary.y) +
                           (grad.one_over_z[2] * bary.z);

        float z = 1.0f/one_over_z;

        /*
        glm::vec3 normal = (grad.vtx[0].normal * bary.x) +
                           (grad.vtx[1].normal * bary.y) +
                           (grad.vtx[2].normal * bary.z);

        glm::vec3 light_dir(0,0,1);
        float light_amt = glm::length(glm::dot(normal, light_dir)) * 0.9f + 0.1f;
        */
        glm::vec2 uv = (grad.uv[0] * bary.x) +
                       (grad.uv[1] * bary.y) +
                       (grad.uv[2] * bary.z);

        uv *= z;
        glm::vec4 c(1,1,1,1);
        if (texture)
            c = texture->get_pixel_linear(uv.x * ((float)texture->width()-1),
                                          uv.y * ((float)texture->height()-1));
        /*
        for (int i= 0; i < 3; i++) {
            c[i] = c[i] * light_amt;
        }
        */

        draw_pixel(x, y, c);
        draw_depth(x, y, depth);


        bary += bary_step;
    }
}

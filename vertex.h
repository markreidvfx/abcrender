#ifndef VERTEX_H
#define VERTEX_H

#include <glm/glm.hpp>

struct Vertex
{
    Vertex() : pos(0), color(0) {}
    Vertex(const glm::vec4 &p, const glm::vec4 &c) : pos(p), color(c) {}
    glm::vec4 pos;
    glm::vec4 color;
    glm::vec3 world_pos;
    glm::vec2 uv;
    glm::vec3 normal;

    float area_x2(const Vertex &b, const Vertex &c) const;
};

#endif // VERTEX_H

#ifndef ABCRENDER_H
#define ABCRENDER_H
#include "rendercontext.h"
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreAbstract/All.h>
#include <Alembic/AbcCoreHDF5/All.h>
#include <Alembic/AbcCoreFactory/All.h>
#include <Alembic/Abc/ErrorHandler.h>
#include <Alembic/Abc/All.h>
#include <Alembic/Abc/ISchema.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/string_cast.hpp>

#include <iostream>
#include <string>
#include <vector>

using namespace Alembic::AbcGeom;
namespace AbcF = ::Alembic::AbcCoreFactory;

int abcrender(const std::string &abc_path,
              const std::string &dest_path,
              const std::string &image_path,
              const std::string &texture_path,
              int start_frame,
              int end_frame,
              int width=1920,
              int height=1080);

class ABCRender
{
public:
    ABCRender(const std::string &abc_path);
    std::vector<IPolyMesh> mesh_list;
    std::vector<ICamera> camera_list;
    void render(RenderContext &ctx, int frame);
    void draw_mesh(RenderContext &ctx,
                   const IPolyMesh &mesh,
                   double seconds);

    void read_uvs(const IPolyMeshSchema::Sample& m_sample,
                  const IPolyMeshSchema &m_schema,
                  std::vector<glm::vec2> &uvs);

    void read_normals(const IPolyMeshSchema::Sample& m_sample,
                      const IPolyMeshSchema &m_schema,
                      std::vector<glm::vec3> &normals);

    void create_normals(const IPolyMeshSchema::Sample& m_sample,
                        const IPolyMeshSchema &m_schema,
                        std::vector<glm::vec3> &normals);

private:
    IArchive m_archive;
    glm::mat4 m_view_matrix;
    glm::mat4 m_projection_matrix;
    glm::mat4 m_screen_matrix;
};

#endif // ABCRENDER_H

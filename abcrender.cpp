#include "abcrender.h"
#include <stdio.h>
#include <Magick++.h>

static void accumXform( M44d &xf, const IObject obj, chrono_t seconds )
{
    if (IXform::matches( obj.getHeader()))  {
        IXform x(obj, kWrapExisting);
        XformSample xs;
        ISampleSelector sel(seconds);
        x.getSchema().get(xs, sel);
        xf *= xs.getMatrix();
    }
}


static M44d get_final_matrix( const IObject &iObj, chrono_t seconds )
{
    M44d xf;
    xf.makeIdentity();
    IObject parent = iObj.getParent();

    while (parent) {
        accumXform(xf, parent, seconds);
        parent = parent.getParent();
    }

    return xf;
}

static glm::mat4 get_camera_projection_matrix(const ICamera &camera,
                                              double width,
                                              double height,
                                              double seconds)
{
    ICameraSchema schema = camera.getSchema();
    CameraSample sample = schema.getValue(seconds);

    double near = sample.getNearClippingPlane();
    double far = sample.getFarClippingPlane();

    double fovy = 2.0 * glm::degrees( atan( sample.getVerticalAperture() * 10.0f /
                                       (2.0f * sample.getFocalLength() ) ) );

    glm::mat4 persp_matrix = glm::perspective(glm::radians(fovy), width/height, near, far);
    return persp_matrix;
}

static void read_object(IObject object, std::vector<IPolyMesh> &mesh_list, std::vector<ICamera> &camera_list)
{
    const size_t child_count = object.getNumChildren();
    for (size_t i = 0; i < child_count; ++i) {
        const ObjectHeader& child_header = object.getChildHeader(i);
        if (IPolyMesh::matches(child_header)) {
            mesh_list.push_back(IPolyMesh(object, child_header.getName()));
        } else if (ICamera::matches(child_header)) {
            camera_list.push_back(ICamera(object, child_header.getName()));
        }
        read_object(object.getChild(i), mesh_list, camera_list);
    }

}

ABCRender::ABCRender(const std::string &abc_path)
{
    AbcF::IFactory factory;
    factory.setPolicy(Abc::ErrorHandler::kQuietNoopPolicy);
    AbcF::IFactory::CoreType coreType;
    m_archive = factory.getArchive(abc_path, coreType);
    read_object(m_archive.getTop(), mesh_list, camera_list);
}

void ABCRender::render(RenderContext &ctx, int frame)
{
    int width = ctx.width();
    int height = ctx.height();
    double seconds = frame / 24.0;

    const ICamera &camera= camera_list[0];

    M44d xf = get_final_matrix(camera, seconds);
    m_view_matrix = glm::inverse(glm::make_mat4(&xf[0][0]));
    m_projection_matrix = get_camera_projection_matrix(camera, width, height, seconds);

    m_screen_matrix = glm::mat4();
    m_screen_matrix = glm::scale(m_screen_matrix, glm::vec3(width/2.0f, height/2.0f, 1.0f));
    m_screen_matrix = glm::translate(m_screen_matrix, glm::vec3(1.0, 1.0, 0));

    for (int i= 0; i < mesh_list.size(); i++) {
        draw_mesh(ctx, mesh_list[i], seconds);
    }
}

void ABCRender::read_uvs(const IPolyMeshSchema::Sample& m_sample,
                         const IPolyMeshSchema &m_schema,
                         std::vector<glm::vec2> &uvs) {
    size_t face_indices = m_sample.getFaceIndices()->size();
    IV2fGeomParam uv_param = m_schema.getUVsParam();
    if (!uv_param.valid()) {
        uvs.resize(face_indices);
        return;
    }

    IV2fGeomParam::Sample uv_sample(uv_param.getIndexedValue());
    if (!uv_sample.valid()){
        uvs.resize(face_indices);
        return;
    }

    const V2f *uv = uv_sample.getVals()->get();
    const size_t uv_count = uv_sample.getVals()->size();
    //cerr << "     reading uvs " << uv_count << "\n";

    if (uv_param.isIndexed()) {
        UInt32ArraySamplePtr indices_obj = uv_sample.getIndices();
        int indices_count = indices_obj->size();
        //cerr << "     mesh has indexed uvs = " << indices_count << "\n";

        for (size_t i = 0; i < indices_count; i++) {
             glm::uint32_t index = indices_obj->get()[i];
             const Alembic::AbcGeom::V2f v = uv[index];
             uvs.push_back(glm::vec2(v.x, v.y));
         }

    } else {

        for (size_t i = 0; i < uv_count; ++i) {
             const Alembic::AbcGeom::V2f v = uv[i];
             uvs.push_back(glm::vec2(v.x, v.y));
        }
    }
}

void ABCRender::read_normals(const IPolyMeshSchema::Sample& m_sample,
                             const IPolyMeshSchema &m_schema,
                             std::vector<glm::vec3> &normals)
{
    IN3fGeomParam normal_param = m_schema.getNormalsParam();
    size_t face_indices = m_sample.getFaceIndices()->size();

    if (!normal_param.valid()) {
        create_normals(m_sample, m_schema, normals);
        return;
    }

    IN3fGeomParam::Sample normal_sample(normal_param.getIndexedValue());
    if (!normal_sample.valid()) {
        create_normals(m_sample, m_schema, normals);
        return;
    }

    const N3f* n = normal_sample.getVals()->get();
    const size_t normal_count = normal_sample.getVals()->size();

    //cerr << "     reading normals " << normal_count << "\n";

    if (normal_param.isIndexed()) {
        UInt32ArraySamplePtr indices_obj = normal_sample.getIndices();
        int indices_count = indices_obj->size();
       // cerr << "   mesh has indexed normals = " << indices_count << "\n";

        for (size_t i = 0; i < indices_count; i++) {
             glm::uint32_t index = indices_obj->get()[i];
             const Alembic::AbcGeom::N3f v = n[index];
             normals.push_back(glm::vec3(v.x, v.y, v.z));
         }
     } else {

        for (size_t i = 0; i < normal_count; ++i) {
            const Alembic::AbcGeom::N3f v = n[i];
            normals.push_back(glm::vec3(v.x, v.y, v.z));
        }
    }

}

struct SimpleVertex{
    glm::vec3 vertex;
    bool operator<(const SimpleVertex that) const{
                    return memcmp((void*)this, (void*)&that, sizeof(SimpleVertex))>0;
    };
};

void ABCRender::create_normals(const IPolyMeshSchema::Sample& m_sample,
                                const IPolyMeshSchema &m_schema,
                                std::vector<glm::vec3> &normals)
{
    const P3fArraySamplePtr &positions = m_sample.getPositions();
    const Int32ArraySamplePtr &faceIndices = m_sample.getFaceIndices();
    const Int32ArraySamplePtr &faceCounts = m_sample.getFaceCounts();

    std::map< SimpleVertex, std::vector< std::pair < unsigned int , glm::vec3 > >  > vertex_map;
    std::map< SimpleVertex, std::vector< std::pair< unsigned int , glm::vec3 > > >::iterator it;

    //cacluate normals
    unsigned int cur_index = 0;
    for(int i =0; i < faceCounts->size(); i++) {
        int face_size = faceCounts->get()[i];

        // degenerated faces
        if (face_size < 3) {
            for(int j=0; j< face_size; j++) {
                normals.push_back(glm::vec3(0,0,0));
            }
            cur_index += face_size;
            continue;
        }

        glm::vec3 poly[3];

        // get just the first normal
        for(int j =0; j < 3; j++) {
            unsigned int index = (unsigned int)(*faceIndices)[cur_index + j];
            const Alembic::AbcGeom::V3f p = positions->get()[index];
            poly[j] = glm::vec3(p.x, p.y, p.z);
        }

        // caculate the normal
        glm::vec3 ab = poly[1] - poly[0];
        glm::vec3 ac = poly[2] - poly[0];
        glm::vec3 wn = glm::normalize(glm::cross(ac, ab));

        for(int j=0; j< face_size; j++) {
            unsigned int index = (unsigned int)(*faceIndices)[cur_index + j];
            const Alembic::AbcGeom::V3f p = positions->get()[index];
            SimpleVertex vert = {glm::vec3(p.x, p.y, p.z)};
            vertex_map[vert].push_back( std::pair< unsigned int ,
                                        glm::vec3>(cur_index + j, wn) );
        }

        cur_index += face_size;
    }

    // finally average the normals
    normals.resize(cur_index);
    for (it = vertex_map.begin(); it != vertex_map.end(); it++) {
        glm::vec3 a;
        for (int i = 0; i < it->second.size(); i ++) {
             a +=  it->second[i].second;
        }

        a /= it->second.size();
        for (int i = 0; i < it->second.size(); i ++) {
            normals[it->second[i].first] = a;
        }
    }
}

void ABCRender::draw_mesh(RenderContext &ctx,
                          const IPolyMesh &mesh,
                          double seconds)
{
    IPolyMeshSchema schema = mesh.getSchema();

    ISampleSelector sel(seconds );
    IPolyMeshSchema::Sample sampler;
    schema.get(sampler, sel);

    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;

    read_uvs(sampler, schema, uvs);
    read_normals(sampler, schema, normals);

    const P3fArraySamplePtr &positions = sampler.getPositions();
    const Int32ArraySamplePtr &faceIndices = sampler.getFaceIndices();
    const Int32ArraySamplePtr &faceCounts = sampler.getFaceCounts();

    unsigned int cur_index = 0;

    std::pair<unsigned int, unsigned int> *p;
    std::pair<unsigned int, unsigned int> face_indices[3];
    Vertex polygon[3];

    M44d xf = get_final_matrix(mesh, seconds);
    glm::mat4 model_matrix = glm::make_mat4(&xf[0][0]);

    glm::mat4 mat =  m_screen_matrix * m_projection_matrix * m_view_matrix * model_matrix;

    for(size_t i =0; i < faceCounts->size(); i++) {
        int face_size = faceCounts->get()[i];
        p = &face_indices[0];
        p->first = cur_index;
        p->second = (unsigned int)(*faceIndices)[p->first];

        for (int j = 1; j < face_size -1; j++) {
            p = &face_indices[1];
            p->first = cur_index + j;
            p->second = (unsigned int)(*faceIndices)[p->first];

            p = &face_indices[2];
            p->first = cur_index + j + 1;
            p->second = (unsigned int)(*faceIndices)[p->first];
            for (int k = 0; k < 3; k++) {
                p = &face_indices[k];
                const Alembic::AbcGeom::V3f vert = positions->get()[p->second];
                glm::vec4 pos(vert.x, vert.y, vert.z, 1.0);
                polygon[k].pos = mat * pos;
                polygon[k].uv = uvs[p->first];
                polygon[k].normal = normals[p->first];

                polygon[k].pos.x /= polygon[k].pos.w;
                polygon[k].pos.y /= polygon[k].pos.w;
                polygon[k].pos.z /= polygon[k].pos.w;

            }
            ctx.draw_triangle(polygon[0], polygon[1], polygon[2]);
        }

        cur_index += face_size;
    }

}

int format_string(const std::string &s, std::string &result, int frame)
{
    char buffer[200];
    int cx;
    cx = snprintf(buffer, 200, s.c_str(), frame);

    if (cx < 0) {
        std::cerr << "error formatting string: " << s << std::endl;
        result = s;
        return cx;
    }

    result = buffer;
    return cx;
}

int abcrender(const std::string &abc_path,
              const std::string &dest_path,
              const std::string &image_path,
              const std::string &texture_path,
              int start_frame,
              int end_frame)
{
    ABCRender renderer(abc_path);

    if (renderer.camera_list.empty()) {
        std::cerr << "no cameras found" << std::endl;
        return -1;
    }

    if (renderer.mesh_list.empty()) {
        std::cerr << "no mesh found" << std::endl;
        return -1;
    }

    int width = 1920;
    int height = 1080;
    RenderContext ctx(width, height);
    RenderContext texture(0, 0);
    //std::string texture_path = "/Users/mark/Dev/SimpleRaster-build/checker.png";
    //texture_path = "/Users/mark/Projects/Samples/RU_029_001/projection_template_guide.tif";
    if (!texture_path.empty()) {
        Magick::Image texture_image(texture_path);
        float tex_width = texture_image.size().width();
        float tex_height = texture_image.size().height();
        texture.resize(tex_width, tex_height);
        texture_image.flip();
        texture_image.write(0, 0, tex_width, tex_height, "RGBA",  Magick::FloatPixel, &texture.data[0]);
        ctx.texture = &texture;
    }

    std::chrono::time_point<std::chrono::system_clock> start;
    std::chrono::duration<double> elapsed_seconds;
    //std::string bg_formating_string = "/Users/mark/Projects/Samples/RU_029_001/RU_029_001/RU_029_001.%04d.dpx";

    for (int i= start_frame; i < end_frame + 1; i++) {
        start = std::chrono::system_clock::now();
        std::string out_image_path;

        //std::cerr << out_image_path << std::endl;

        renderer.render(ctx, i);

        elapsed_seconds = std::chrono::system_clock::now() - start;
        std::cerr << "frame " << i << " render in " << elapsed_seconds.count() << " secs \n";

        start = std::chrono::system_clock::now();

        Magick::Image rendered_image;
        Magick::Image image;

        //rendered_image.verbose(true);
        //image.verbose(true);
        rendered_image.read(width, height, "RGBA", Magick::FloatPixel, &ctx.data[0]);
        //rendered_image.resize("50%");
        rendered_image.flip();

        std::string bg_path;
        if (!(image_path.empty()) && !(format_string(image_path, bg_path, i) < 0)) {
            image.read(bg_path);
            image.strip();
            image.attribute("colorspace", "srgb");
            image.composite(rendered_image, Magick::CenterGravity, Magick::OverCompositeOp);
        } else {
            image = rendered_image;
        }

        format_string(dest_path, out_image_path, i);

        image.depth(8);
        image.write(out_image_path);

        elapsed_seconds = std::chrono::system_clock::now()-start;
        std::cerr << "image " << i << " written in " << elapsed_seconds.count() << " secs \n";

        //break;
        ctx.clear();
    }


    return 0;
}

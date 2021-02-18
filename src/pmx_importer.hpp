#pragma once
#include <Godot.hpp>
#include <gen/ArrayMesh.hpp>

#include "parser.h"

namespace godot {
  class PMXImporter : public Object {
    GODOT_CLASS(PMXImporter, Object)
  private:
    String model_name_local;
    String model_name_universal;
    String comment_local;
    String comment_universal;
    Array mesh_array;
    PoolVector3Array positions;
    PoolVector3Array normals;
    PoolVector2Array uvs;
    PoolIntArray bones;
    PoolRealArray weights;
    PoolIntArray triangles;
    PoolStringArray textures;
    Array materials;
    ArrayMesh *mesh;

    int parseBoneWeights(const pmx_vertex_t &vertex, uint_fast32_t i);
  public:
    static void _register_methods();

    PMXImporter();
    ~PMXImporter();

    int modelInfoCB(pmx_model_info_t *model);
    int vertexCB(struct pmx_parse_state *state, int_fast32_t count);
    int triangleCB(struct pmx_parse_state *state, int_fast32_t count);
    int textureCB(struct pmx_parse_state *state, int_fast32_t count);
    int materialCB(struct pmx_parse_state *state, int_fast32_t count);

    int parse(String filename);
    void _init();

    Variant get_mesh();
    void set_mesh(Variant mesh_var);
  };
}

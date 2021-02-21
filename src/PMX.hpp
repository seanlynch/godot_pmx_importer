#pragma once
#include <vector>

#include <Godot.hpp>
#include <Reference.hpp>

#include "parser.h"

namespace godot {
  class Vertex : private pmx_vertex_t {
  };

  class Material : private pmx_material_t {
  };

  class PMX : public Object {
    GODOT_CLASS(PMX, Object)
  private:
    String model_name_local;
    String model_name_universal;
    String comment_local;
    String comment_universal;
    std::vector<Vertex> vertices;
    std::vector<godot_int> indices;
    PoolStringArray textures;
    Array materials;
    Array bones;

    int parseBoneWeights(const pmx_vertex_t &vertex, uint_fast32_t i);
    Array getSurfaceArrays(int_fast32_t start, int_fast32_t end);
  public:
    static void _register_methods();
    void _init();

    int modelInfoCB(pmx_model_info_t *model);
    int vertexCB(struct pmx_parse_state *state, int_fast32_t count);
    int triangleCB(struct pmx_parse_state *state, int_fast32_t count);
    int textureCB(struct pmx_parse_state *state, int_fast32_t count);
    int materialCB(struct pmx_parse_state *state, int_fast32_t count);
    int boneCB(struct pmx_parse_state *state, int_fast32_t count);
    int morphCB(struct pmx_parse_state *state, int_fast32_t count);

    int parse(PoolByteArray data);
  };
}

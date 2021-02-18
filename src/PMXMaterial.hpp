#pragma once
#include <Godot.hpp>
#include <Reference.hpp>

extern "C" struct pmx_parse_state;

namespace godot {
  class PMXMaterial : public Reference {
    GODOT_CLASS(PMXMaterial, Reference)
    friend class PMX;

  private:
    int parse(struct pmx_parse_state *state);

  public:
    static void _register_methods();
    void _init();

    // String name_local;
    // String name_universal;
    // Color diffuse;
    // Color specular;
    // godot_real specularity;
    // Color ambient;
    // godot_int flags;
    // Color edge_color;
    // godot_real edge_scale;
    // godot_int texture;
    // godot_int environment;
    // godot_int environment_blend_mode;
    // godot_int toon_type;
    // godot_int toon;
    // String metadata;
    // godot_int index_count;
  };
}

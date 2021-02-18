#include <stdio.h>

#include "PMXMaterial.hpp"
#include "parser.h"

using namespace godot;

void PMXMaterial::_init() {
  std::cerr << "PMXMaterial::_init()" << std::endl;
}

void PMXMaterial::_register_methods() {
  std::cerr << "PMXMaterial::register_methods()" << std::endl;
  // register_property<PMXMaterial, String>("name_local", &PMXMaterial::name_local, "");
  // register_property<PMXMaterial, String>("name_universal", &PMXMaterial::name_universal, "");
  // register_property<PMXMaterial, Color>("diffuse", &PMXMaterial::diffuse, {});
  // register_property<PMXMaterial, Color>("specular", &PMXMaterial::specular, {});
  // register_property<PMXMaterial, godot_real>("specularity", &PMXMaterial::specularity, 0.0);
  // register_property<PMXMaterial, Color>("ambient", &PMXMaterial::ambient, {});
  // register_property<PMXMaterial, godot_int>("flags", &PMXMaterial::flags, 0);
  // register_property<PMXMaterial, Color>("edge_color", &PMXMaterial::edge_color, {});
  // register_property<PMXMaterial, godot_real>("edge_scale", &PMXMaterial::edge_scale, 0.0);
  // register_property<PMXMaterial, godot_int>("texture", &PMXMaterial::texture, 0);
  // register_property<PMXMaterial, godot_int>("environment", &PMXMaterial::environment, 0);
  // register_property<PMXMaterial, godot_int>("environment_blend_mode", &PMXMaterial::environment_blend_mode, 0);
  // register_property<PMXMaterial, godot_int>("toon_type", &PMXMaterial::toon_type, 0);
  // register_property<PMXMaterial, godot_int>("toon", &PMXMaterial::toon, 0);
  // register_property<PMXMaterial, String>("metadata", &PMXMaterial::metadata, "");
  // register_property<PMXMaterial, godot_int>("index_count", &PMXMaterial::index_count, 0);
}

int PMXMaterial::parse(struct pmx_parse_state *state) {
  pmx_material_t material;
  int ret = pmx_parser_next_material(state, &material);
  if (ret != 0) return ret;
  // name_local = material.name_local;
  // name_universal = material.name_universal;
  // diffuse = Color(material.diffuse[0], material.diffuse[1], material.diffuse[2], material.diffuse[3]);
  // specular = Color(material.specular[0], material.specular[1], material.specular[2]);
  // specularity = material.specularity;
  // edge_color = Color(material.edge_color[0], material.edge_color[1], material.edge_color[2], material.edge_color[3]);
  // texture = material.texture;
  // environment = material.environment;
  // environment_blend_mode = material.environment_blend_mode;
  // toon_type = material.toon_type;
  // toon = material.toon;
  // metadata = material.metadata;
  // index_count = material.index_count;
  return ret;
}

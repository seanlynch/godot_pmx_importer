#include <array>

#include "PMX.hpp"

using namespace godot;

static inline Vector2 vec2(const float v[2]) { return Vector2(v[0], v[1]); }
static inline Vector3 vec3(const float v[3]) { return Vector3(v[0], v[1], v[2]); }

int PMX::modelInfoCB(pmx_model_info_t *model) {
  model_name_local = model->model_name_local;
  model_name_universal = model->model_name_universal;
  comment_local = model->comment_local;
  comment_universal = model->comment_universal;
  return 0;
}

int PMX::parseBoneWeights(const pmx_vertex_t &vertex, uint_fast32_t i) {
  int b0 = vertex.bones[0];
  int b1 = vertex.bones[1];
  int b2 = vertex.bones[2];
  int b3 = vertex.bones[3];
  float w0 = vertex.bones[0];
  float w1 = vertex.bones[1];
  float w2 = vertex.bones[2];
  float w3 = vertex.bones[3];
  float renorm;

  if (b0 < 0) {
    b0 = 0;
    w0 = 0.0;
  };

  if (b1 < 0) {
    b1 = 0;
    w1 = 0.0;
  }

  if (b2 < 0) {
    b2 = 0;
    w2 = 0.0;
  }

  if (b3 < 0) {
    b3 = 0;
    w3 = 0.0;
  }

  switch (vertex.deform_type) {
  case bdef1:
  case bdef2:
    break;
  case bdef4:
    /* Make the weights sum to 1 by renormalizing */
    renorm = w0 + w1 + w2 + w3;
    if (renorm != 0.0 && renorm != 1.0) {
      w0 /= renorm;
      w1 /= renorm;
      w2 /= renorm;
      w3 /= renorm;
    }
    break;
  case sdef:
    sdef_vertices.push_back(i);
    sdef_data.push_back(vec3(vertex.c));
    sdef_data.push_back(vec3(vertex.r0));
    sdef_data.push_back(vec3(vertex.r1));
    break;
  case qdef:
    std::cerr << "Unhandled " << pmx_deform_type_string(vertex.deform_type) << " weights" << std::endl;
    break;
  }

  godot_int index = i * 4;
  bone_indices.set(index, b0);
  bone_weights.set(index, w0);
  index++;
  bone_indices.set(index, b1);
  bone_weights.set(index, w1);
  index++;
  bone_indices.set(index, b2);
  bone_weights.set(index, w2);
  index++;
  bone_indices.set(index, b3);
  bone_weights.set(index, w3);
  return 0;
}

int PMX::vertexCB(struct pmx_parse_state *state, int_fast32_t count) {
  pmx_vertex_t vertex;
  int ret = 0;

  vertices.resize(count);
  for (auto& it : vertices) {
    ret = pmx_parser_next_vertex(state, &vertex);
    if (ret != 0) break;
  }

  return ret;
}

int PMX::triangleCB(struct pmx_parse_state *state, int_fast32_t count) {
  int ret = 0;
  int32_t triangle[3];

  triangles.resize(count * 3);
  int j = 0;
  for (int i = 0; i < count; i++) {
    ret = pmx_parser_next_triangle_int32(state, triangle);
    if (ret != 0) break;

    triangles[j++] = triangle[0];
    triangles[j++] = triangle[1];
    triangles[j++] = triangle[2];
  }

  return ret;
}

int PMX::textureCB(struct pmx_parse_state *state, int_fast32_t count) {
  char buf[256];
  int ret;

  textures.resize(count);
  for (int i = 0; i < count; i++) {
    ret = pmx_parser_next_texture(state, buf, sizeof buf);
    if (ret != 0) break;
    textures.set(i, buf);
  }

  return ret;
}

Dictionary PMX::getSurfaceArrays(int_fast32_t start, int_fast32_t count) {
  PoolVector3Array positions, normals;
  PoolVector2Array uvs;
  PoolIntArray bones;
  PoolRealArray weights;

  positions.resize(count);
  normals.resize(count);
  uvs.resize(count);

  
  for (const auto& it : indices) {
  }
}

int PMX::materialCB(struct pmx_parse_state *state, int_fast32_t count) {
  int ret = 0;
  godot_int j, k = 0;
  pmx_material_t m;

  printf("Got %ld materials\n", count);
  for (int i = 0; i < count; i++) {
    ret = pmx_parser_next_material(state, &m);
    if (ret != 0) break;

    
    PoolIntArray slice;
    slice.resize(m.index_count);
    for (j = 0; j < m.index_count; j++) {
      slice.set(j, triangles[k++]);
    }

    printf("Material \"%s\" (\"%s\")\n", m.name_local, m.name_universal);
    materials.push_back(Dictionary::make("name_local", m.name_local,
					 "name_universal", m.name_universal,
					 "diffuse", Color(m.diffuse[0], m.diffuse[1], m.diffuse[2], m.diffuse[3]),
					 "specular", Color(m.specular[0], m.specular[1], m.specular[2]),
					 "specularity", m.specularity,
					 "edge_color", Color(m.edge_color[0], m.edge_color[1], m.edge_color[2], m.edge_color[3]),
					 "edge_scale", m.edge_scale,
					 "texture", m.texture,
					 "environment", m.environment,
					 "environment_blend_mode", m.environment_blend_mode,
					 "toon_internal", m.toon_type == m.internal_ref,
					 "toon", m.toon,
					 "metadata", m.metadata,
					 "index_count", m.index_count,
					 "indices", slice
					 ));
    printf("Done\n");
  }

  return ret;
}

static Dictionary parse_bone_ik(const pmx_bone_t &b) {
  auto ik = Dictionary::make("target", b.ik.target,
			     "loop_count", b.ik.loop_count,
			     "limit_radian", b.ik.limit_radian);
  Array links;
  links.resize(b.ik.link_count);
  for (int i = 0; i < b.ik.link_count; i++) {
    auto &link = b.ik.ik_links[i];
    auto link_dict = Dictionary::make("bone", link.bone,
				      "has_limits", link.has_limits);
    if (link.has_limits) {
      Array limits;
      limits.resize(2);
      limits[0] = vec3(link.limit_min);
      limits[1] = vec3(link.limit_max);
      link_dict["limits"] = limits;
    }
    links[i] = link_dict;
  }
  return ik;
}

int PMX::boneCB(struct pmx_parse_state *state, int_fast32_t count) {
  int ret = 0;
  pmx_bone_t b;

  printf("Got %ld bones\n", count);
  for (int i = 0; i < count; i++) {
    ret = pmx_parser_next_bone(state, &b);
    if (ret != 0) break;
    auto dict = Dictionary::make("name_local", b.name_local,
				 "name_universal", b.name_universal,
				 "position", vec3(b.position),
				 "parent", b.parent,
				 "layer", b.layer,
				 "flags", b.flags);
    if (b.flags & PMX_BONE_FLAG_INDEXED_TAIL_POS) dict["tail_bone"] = b.tail_bone;
    else dict["tail_position"] = vec3(b.tail_position);
    if (b.flags & (PMX_BONE_FLAG_INHERIT_ROTATION | PMX_BONE_FLAG_INHERIT_TRANSLATION)) {
      dict["inherit"] = Dictionary::make("parent", b.inherit.parent,
					 "weight", b.inherit.weight);
    }
    if (b.flags & PMX_BONE_FLAG_FIXED_AXIS) dict["fixed_axis"] = vec3(b.fixed_axis);
    if (b.flags & PMX_BONE_FLAG_LOCAL_COORD) {
      dict["local_coord"] = Dictionary::make("x_axis", vec3(b.local_coord.x_axis),
					     "z_axis", vec3(b.local_coord.z_axis));
    }
    if (b.flags & PMX_BONE_FLAG_EXTERNAL_PARENT_DEFORM) dict["external_parent"] = b.external_parent;
    if (b.flags & PMX_BONE_FLAG_IK) dict["ik"] = parse_bone_ik(b);
    bones.push_back(dict);
  }
  return ret;
}

extern "C" int pmx_importer_model_info_cb(pmx_model_info_t *model, void *userdata) {
  PMX *obj = static_cast<PMX *>(userdata);
  return obj->modelInfoCB(model);
}

extern "C" int pmx_importer_vertex_cb(struct pmx_parse_state *state, int_fast32_t count, void *userdata) {
  return static_cast<PMX *>(userdata)->vertexCB(state, count);
}

extern "C" int pmx_importer_triangle_cb(struct pmx_parse_state *state, int_fast32_t count, void *userdata) {
  return static_cast<PMX *>(userdata)->triangleCB(state, count);
}

extern "C" int pmx_importer_texture_cb(struct pmx_parse_state *state, int_fast32_t count, void *userdata) {
  return static_cast<PMX *>(userdata)->textureCB(state, count);
}

extern "C" int pmx_importer_material_cb(struct pmx_parse_state *state, int_fast32_t count, void *userdata) {
  return static_cast<PMX *>(userdata)->materialCB(state, count);
}

extern "C" int pmx_importer_bone_cb(struct pmx_parse_state *state, int_fast32_t count, void *userdata) {
  return static_cast<PMX *>(userdata)->boneCB(state, count);
}

extern "C" int pmx_importer_morph_cb(struct pmx_parse_state *state, int_fast32_t count, void *userdata) {
  return static_cast<PMX *>(userdata)->morphCB(state, count);
}

int PMX::parse(PoolByteArray data) {
  static const pmx_parser_callbacks_t parser_callbacks =
    {
     .model_info_cb = pmx_importer_model_info_cb,
     .vertex_cb = pmx_importer_vertex_cb,
     .triangle_cb = pmx_importer_triangle_cb,
     .texture_cb = pmx_importer_texture_cb,
     .material_cb = pmx_importer_material_cb,
     .bone_cb = pmx_importer_bone_cb,
     .morph_cb = pmx_importer_morph_cb
    };

  return pmx_parser_parse(data.read().ptr(), data.size(), &parser_callbacks, static_cast<void *>(this));
}

void PMX::_init() {
}

void PMX::_register_methods() {
  std::cerr << "PMX::_register_methods()" << std::endl;
  register_method("parse", &PMX::parse);
  register_property<PMX, String>("model_name_local", &PMX::model_name_local, "");
  register_property<PMX, String>("model_name_universal", &PMX::model_name_universal, "");
  register_property<PMX, String>("comment_local", &PMX::comment_local, "");
  register_property<PMX, String>("comment_universal", &PMX::comment_universal, "");
  register_property<PMX, PoolVector3Array>("positions", &PMX::positions, {});
  register_property<PMX, PoolVector3Array>("normals", &PMX::normals, {});
  register_property<PMX, PoolVector2Array>("uvs", &PMX::uvs, {});
  register_property<PMX, PoolIntArray>("bone_indices", &PMX::bone_indices, {});
  register_property<PMX, PoolRealArray>("bone_weights", &PMX::bone_weights, {});
  register_property<PMX, PoolIntArray>("sdef_vertices", &PMX::bone_indices, {});
  register_property<PMX, PoolVector3Array>("sdef_data", &PMX::sdef_data, {});
  register_property<PMX, PoolIntArray>("triangles", &PMX::triangles, {});
  register_property<PMX, PoolStringArray>("textures", &PMX::textures, {});
  register_property<PMX, Array>("materials", &PMX::materials, {});
  register_property<PMX, Array>("bones", &PMX::bones, {});
}

extern "C" void GDN_EXPORT godot_gdnative_init(godot_gdnative_init_options *o) {
    godot::Godot::gdnative_init(o);
}

extern "C" void GDN_EXPORT godot_gdnative_terminate(godot_gdnative_terminate_options *o) {
    godot::Godot::gdnative_terminate(o);
}

extern "C" void GDN_EXPORT godot_nativescript_init(void *handle) {
    godot::Godot::nativescript_init(handle);

    godot::register_class<godot::PMX>();
}

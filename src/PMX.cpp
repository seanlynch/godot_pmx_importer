#include <Ref.hpp>

#include "PMX.hpp"
#include "PMXMaterial.hpp"

using namespace godot;

int PMX::modelInfoCB(pmx_model_info_t *model) {
  model_name_local = model->model_name_local;
  model_name_universal = model->model_name_universal;
  comment_local = model->comment_local;
  comment_universal = model->comment_universal;
  return 0;
}

int PMX::parseBoneWeights(const pmx_vertex_t &vertex, uint_fast32_t i) {
  godot_int index = i * 4;
  const int_fast32_t (&vbones)[4] = vertex.deform.non_sdef.bones;
  const float (&vweights)[4] = vertex.deform.non_sdef.weights;
  float renorm;

  switch (vertex.deform_type) {
  case bdef1:
    bones.set(index, vbones[0]);
    weights.set(index++, 1.0);
    bones.set(index, -1);
    weights.set(index++, 0.0);
    bones.set(index, -1);
    weights.set(index++, 0.0);
    bones.set(index, -1);
    weights.set(index, 0.0);
    break;
  case bdef2:
    bones.set(index, vbones[0]);
    weights.set(index++, vweights[0]);
    bones.set(index, vbones[1]);
    weights.set(index++, 1.0 - vweights[0]);
    bones.set(index, -1);
    weights.set(index++, 0.0);
    bones.set(index, -1);
    weights.set(index, 0.0);
    break;
  case bdef4:
    /* Make the weights sum to 1 by renormalizing */
    renorm = 1.0 / (vweights[0] + vweights[1] + vweights[2] + vweights[3]);
    bones.set(index, vbones[0]);
    weights.set(index++, vweights[0] * renorm);
    bones.set(index, vbones[1]);
    weights.set(index++, vweights[1] * renorm);
    bones.set(index, bones[2]);
    weights.set(index++, vweights[2] * renorm);
    bones.set(index, bones[3]);
    weights.set(index, vweights[3] * renorm);
    break;
  case qdef:
  case sdef:
    std::cerr << "Unhandled " << pmx_deform_type_string(vertex.deform_type) << " weights" << std::endl;
    break;
  }
  return 0;
}

int PMX::vertexCB(struct pmx_parse_state *state, int_fast32_t vertex_count) {
  pmx_vertex_t vertex;
  int ret = 0;

  /* Reserve space for the vertex data */
  positions.resize(vertex_count);
  normals.resize(vertex_count);
  uvs.resize(vertex_count);
  bones.resize(vertex_count * 4);
  weights.resize(vertex_count * 4);

  for (int i = 0; i < vertex_count; i++) {
    ret = pmx_parser_next_vertex(state, &vertex);
    if (ret != 0) break;
    positions.set(i, Vector3(vertex.position[0], vertex.position[1], vertex.position[2]));
    normals.set(i, Vector3(vertex.normal[0], vertex.normal[1], vertex.normal[2]));
    uvs.set(i, Vector2(vertex.uv[0], vertex.uv[1]));
    parseBoneWeights(vertex, i);
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

    /* Reverse the winding direction */
    triangles.set(j++, triangle[2]);
    triangles.set(j++, triangle[1]);
    triangles.set(j++, triangle[0]);
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

int PMX::materialCB(struct pmx_parse_state *state, int_fast32_t count) {
  int ret = 0;
  const char *str;
  int_fast32_t j = 0;

  printf("Got %ld materials\n", count);
  for (int i = 0; i < count; i++) {
    std::cerr << "Material " << i << std::endl;
    Ref<PMXMaterial> material;
    material.instance();
    ret = material->parse(state);
    if (ret != 0) break;
    materials.push_back(material);
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

int PMX::parse(String filename) {
  const pmx_parser_callbacks_t parser_callbacks =
    {
     .model_info_cb = pmx_importer_model_info_cb,
     .vertex_cb = pmx_importer_vertex_cb,
     .triangle_cb = pmx_importer_triangle_cb,
     .texture_cb = pmx_importer_texture_cb,
     .material_cb = pmx_importer_material_cb
    };

  return pmx_parser_parse(filename.utf8().get_data(), &parser_callbacks, static_cast<void *>(this));
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
  register_property<PMX, PoolIntArray>("triangles", &PMX::triangles, {});
  register_property<PMX, Array>("materials", &PMX::materials, {});
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
    godot::register_class<godot::PMXMaterial>();
}

#include "pmx_importer.hpp"

using namespace godot;

PMXImporter::PMXImporter() {}


PMXImporter::~PMXImporter() {}


int PMXImporter::modelInfoCB(pmx_model_info_t *model) {
  model_name_local = model->model_name_local;
  model_name_universal = model->model_name_universal;
  comment_local = model->comment_local;
  comment_universal = model->comment_universal;
  mesh = nullptr;
  return 0;
}

int PMXImporter::parseBoneWeights(const pmx_vertex_t &vertex, uint_fast32_t i) {
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

int PMXImporter::vertexCB(struct pmx_parse_state *state, int_fast32_t vertex_count) {
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

int PMXImporter::triangleCB(struct pmx_parse_state *state, int_fast32_t count) {
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

int PMXImporter::textureCB(struct pmx_parse_state *state, int_fast32_t count) {
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


int PMXImporter::materialCB(struct pmx_parse_state *state, int_fast32_t count) {
  int ret;
  pmx_material_t material;
  const char *str;
  int_fast32_t j = 0;
  Array arrays;

  mesh = new ArrayMesh;
  arrays.resize(ArrayMesh::ARRAY_MAX);
  arrays[ArrayMesh::ARRAY_VERTEX] = positions;
  arrays[ArrayMesh::ARRAY_NORMAL] = normals;
  arrays[ArrayMesh::ARRAY_TEX_UV] = uvs;
  arrays[ArrayMesh::ARRAY_BONES] = bones;
  arrays[ArrayMesh::ARRAY_WEIGHTS] = weights;

  printf("Got %ld materials\n", count);
  for (int i = 0; i < count; i++) {
    ret = pmx_parser_next_material(state, &material);
    if (ret != 0) break;
    switch (material.environment_blend_mode) {
    case material.disabled:
      str = "disabled";
      break;
    case material.multiplicative:
      str = "multiplicative";
      break;
    case material.additive:
      str = "additive";
      break;
    case material.addl_vec4:
      str = "addl_vec4";
      break;
    default:
      std::cerr << "Invalid environment blend mode " << material.environment_blend_mode << std::endl;
    }

    int_fast32_t index_count = material.triangle_count * 3;

    // The slice method hasn't made it into the GDNative headers yet
    Array slice;
    slice.resize(index_count);
    for (i = 0; i < index_count; i++) {
      slice[i] = triangles[j++];
    }

    // Add just the indices that correspond to this material
    arrays[ArrayMesh::ARRAY_INDEX] = slice;
    mesh->add_surface_from_arrays(ArrayMesh::PRIMITIVE_TRIANGLES, arrays);

    materials.push_back(Dictionary::make("name_local", material.name_local,
					 "name_universal", material.name_universal,
					 "diffuse", Color(material.diffuse[0], material.diffuse[1], material.diffuse[2], material.diffuse[3]),
					 "specular", Color(material.specular[0], material.specular[1], material.specular[2]),
					 "specularity", material.specularity,
					 "edge_color", Color(material.edge_color[0], material.edge_color[1], material.edge_color[2], material.edge_color[3]),
					 "texture", material.texture,
					 "environment", material.environment,

					 "environment_blend_mode", str,
					 "toon_internal", material.toon_type == material.internal_ref,
					 "toon", material.toon,
					 "metadata", material.metadata,
					 "triangle_count", material.triangle_count
					 ));
  }

  return 0;
}

extern "C" int pmx_importer_model_info_cb(pmx_model_info_t *model, void *userdata) {
  PMXImporter *obj = static_cast<PMXImporter *>(userdata);
  return obj->modelInfoCB(model);
}

extern "C" int pmx_importer_vertex_cb(struct pmx_parse_state *state, int_fast32_t count, void *userdata) {
  return static_cast<PMXImporter *>(userdata)->vertexCB(state, count);
}

extern "C" int pmx_importer_triangle_cb(struct pmx_parse_state *state, int_fast32_t count, void *userdata) {
  return static_cast<PMXImporter *>(userdata)->triangleCB(state, count);
}

extern "C" int pmx_importer_texture_cb(struct pmx_parse_state *state, int_fast32_t count, void *userdata) {
  return static_cast<PMXImporter *>(userdata)->textureCB(state, count);
}


extern "C" int pmx_importer_material_cb(struct pmx_parse_state *state, int_fast32_t count, void *userdata) {
  return static_cast<PMXImporter *>(userdata)->materialCB(state, count);
}

int PMXImporter::parse(String filename) {
  const pmx_parser_callbacks_t parser_callbacks =
    {
     .model_info_cb = pmx_importer_model_info_cb,
     .vertex_cb = pmx_importer_vertex_cb,
     .triangle_cb = pmx_importer_triangle_cb,
     .texture_cb = pmx_importer_texture_cb,
     .material_cb = pmx_importer_material_cb
    };

  char *filename_cstr = filename.alloc_c_string();
  int ret = pmx_parser_parse(filename_cstr, &parser_callbacks, static_cast<void *>(this));
  godot_free(filename_cstr);
  return ret;
}

void PMXImporter::_init() {
}


void PMXImporter::_register_methods() {
  register_method("parse", &PMXImporter::parse);
  register_property<PMXImporter, String>("model_name_local", &PMXImporter::model_name_local, "");
  register_property<PMXImporter, String>("model_name_universal", &PMXImporter::model_name_universal, "");
  register_property<PMXImporter, String>("comment_local", &PMXImporter::comment_local, "");
  register_property<PMXImporter, String>("comment_universal", &PMXImporter::comment_universal, "");
  register_property<PMXImporter, PoolVector3Array>("positions", &PMXImporter::positions, {});
  register_property<PMXImporter, PoolVector3Array>("normals", &PMXImporter::normals, {});
  register_property<PMXImporter, PoolVector2Array>("uvs", &PMXImporter::uvs, {});
  register_property<PMXImporter, PoolIntArray>("triangles", &PMXImporter::triangles, {});
  //register_property<PMXImporter, ArrayMesh*>("mesh", &PMXImporter::mesh, nullptr);
}

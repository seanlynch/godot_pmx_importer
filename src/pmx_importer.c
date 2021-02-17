#include <gdnative_api_struct.gen.h>

#include <stdint.h>
#include <stdio.h>

#include "parser.h"

static const godot_gdnative_core_api_struct *api = NULL;
static const godot_gdnative_ext_nativescript_api_struct *nativescript_api = NULL;

typedef struct {
  godot_variant model_name_local;
  godot_variant model_name_universal;
  godot_variant comment_local;
  godot_variant comment_universal;
  godot_variant vertex_count;
  godot_variant triangle_count;
  godot_variant positions;
  godot_variant normals;
  godot_variant uvs;
  godot_variant triangles;
  godot_variant bones;
  godot_variant weights;
  godot_variant textures;
  godot_variant materials;
} pmx_importer_userdata_t;

static void *pmx_constructor(godot_object *obj, void *method_data) {
  pmx_importer_userdata_t *userdata = api->godot_alloc(sizeof *userdata);
  api->godot_variant_new_nil(&userdata->positions);
  api->godot_variant_new_nil(&userdata->normals);
  api->godot_variant_new_nil(&userdata->uvs);
  api->godot_variant_new_nil(&userdata->triangles);
  api->godot_variant_new_nil(&userdata->bones);
  api->godot_variant_new_nil(&userdata->weights);
  api->godot_variant_new_nil(&userdata->textures);
  api->godot_variant_new_nil(&userdata->materials);
  return userdata;
}

static void pmx_destructor(godot_object *obj, void *method_data, void *userdata_void) {
  pmx_importer_userdata_t *userdata = userdata_void;
  api->godot_variant_destroy(&userdata->model_name_local);
  api->godot_variant_destroy(&userdata->model_name_universal);
  api->godot_variant_destroy(&userdata->comment_local);
  api->godot_variant_destroy(&userdata->comment_universal);
  api->godot_variant_destroy(&userdata->positions);
  api->godot_variant_destroy(&userdata->normals);
  api->godot_variant_destroy(&userdata->uvs);
  api->godot_variant_destroy(&userdata->triangles);
  api->godot_variant_destroy(&userdata->bones);
  api->godot_variant_destroy(&userdata->weights);
  api->godot_variant_destroy(&userdata->textures);
  api->godot_variant_destroy(&userdata->materials);
  api->godot_free(userdata);
}

static void var_set_str(godot_variant *var, const char *str) {
  godot_string gstr = api->godot_string_chars_to_utf8(str);
  api->godot_variant_new_string(var, &gstr);
  api->godot_string_destroy(&gstr);
}

static int pmx_importer_model_info_cb(pmx_model_info_t *model, void *userdata_void) {
  pmx_importer_userdata_t *userdata = userdata_void;
  godot_string str;
  var_set_str(&userdata->model_name_local, model->model_name_local);
  var_set_str(&userdata->model_name_universal, model->model_name_universal);
  var_set_str(&userdata->comment_local, model->comment_local);
  var_set_str(&userdata->comment_universal, model->comment_universal);
  return 0;
}

static void pmx_importer_parse_bone_weights(pmx_vertex_t *vertex, uint_fast32_t i, godot_pool_int_array *barr, godot_pool_real_array *warr) {
  godot_int index = i * 4;
  int_fast32_t *bones = vertex->deform.non_sdef.bones;
  float *weights = vertex->deform.non_sdef.weights;
  float renorm;

  switch (vertex->deform_type) {
  case bdef1:
    api->godot_pool_int_array_set(barr, i, bones[0]);
    api->godot_pool_real_array_set(warr, i++, 1.0);
    api->godot_pool_int_array_set(barr, i, -1);
    api->godot_pool_real_array_set(warr, i++, 0.0);
    api->godot_pool_int_array_set(barr, i, -1);
    api->godot_pool_real_array_set(warr, i++, 0.0);
    api->godot_pool_int_array_set(barr, i, -1);
    api->godot_pool_real_array_set(warr, i, 0.0);
    break;
  case bdef2:
    api->godot_pool_int_array_set(barr, i, bones[0]);
    api->godot_pool_real_array_set(warr, i++, weights[0]);
    api->godot_pool_int_array_set(barr, i, bones[1]);
    api->godot_pool_real_array_set(warr, i++, 1.0-weights[0]);
    api->godot_pool_int_array_set(barr, i, -1);
    api->godot_pool_real_array_set(warr, i++, 0.0);
    api->godot_pool_int_array_set(barr, i, -1);
    api->godot_pool_real_array_set(warr, i, 0.0);
    break;
  case bdef4:
    /* Make the weights sum to 1 by renormalizing */
    renorm = 1.0 / (weights[0] + weights[1] + weights[2] + weights[3]);
    api->godot_pool_int_array_set(barr, i, bones[0]);
    api->godot_pool_real_array_set(warr, i++, weights[0] * renorm);
    api->godot_pool_int_array_set(barr, i, bones[1]);
    api->godot_pool_real_array_set(warr, i++, weights[1] * renorm);
    api->godot_pool_int_array_set(barr, i, bones[2]);
    api->godot_pool_real_array_set(warr, i++, weights[2] * renorm);
    api->godot_pool_int_array_set(barr, i, bones[3]);
    api->godot_pool_real_array_set(warr, i, weights[3] * renorm);
    break;
  case qdef:
  case sdef:
    fprintf(stderr, "Unhandled %s weights\n", pmx_deform_type_string(vertex->deform_type));
    break;
  }
}

static int pmx_importer_vertex_cb(struct pmx_parse_state *state, int_fast32_t vertex_count, void *userdata_void) {
  pmx_importer_userdata_t *userdata = userdata_void;
  pmx_vertex_t vertex;
  int ret = 0;
  godot_vector3 vec3;
  godot_vector2 vec2;
  godot_pool_vector3_array positions, normals;
  godot_pool_vector2_array uvs;
  godot_pool_int_array bones;
  godot_pool_real_array weights;

  /* Reserve space for the vertex data */
  api->godot_pool_vector3_array_new(&positions);
  api->godot_pool_vector3_array_resize(&positions, vertex_count);
  api->godot_pool_vector3_array_new(&normals);
  api->godot_pool_vector3_array_resize(&normals, vertex_count);
  api->godot_pool_vector2_array_new(&uvs);
  api->godot_pool_vector2_array_resize(&uvs, vertex_count);
  api->godot_pool_int_array_new(&bones);
  api->godot_pool_int_array_resize(&bones, vertex_count * 4);
  api->godot_pool_real_array_new(&weights);
  api->godot_pool_real_array_resize(&weights, vertex_count * 4);
  for (int i = 0; i < vertex_count; i++) {
    ret = pmx_parser_next_vertex(state, &vertex);
    if (ret != 0) break;
    api->godot_vector3_new(&vec3, vertex.position[0], vertex.position[1], vertex.position[2]);
    api->godot_pool_vector3_array_set(&positions, i, &vec3);
    api->godot_vector3_new(&vec3, vertex.normal[0], vertex.normal[1], vertex.normal[2]);
    api->godot_pool_vector3_array_set(&normals, i, &vec3);
    api->godot_vector2_new(&vec2, vertex.uv[0], vertex.uv[1]);
    api->godot_pool_vector2_array_set(&uvs, i, &vec2);
    pmx_importer_parse_bone_weights(&vertex, i, &bones, &weights);
  }

  api->godot_variant_new_pool_vector3_array(&userdata->positions, &positions);
  api->godot_pool_vector3_array_destroy(&positions);
  api->godot_variant_new_pool_vector3_array(&userdata->normals, &normals);
  api->godot_pool_vector3_array_destroy(&normals);
  api->godot_variant_new_pool_vector2_array(&userdata->uvs, &uvs);
  api->godot_pool_vector2_array_destroy(&uvs);
  api->godot_variant_new_pool_int_array(&userdata->bones, &bones);
  api->godot_pool_int_array_destroy(&bones);
  api->godot_variant_new_pool_real_array(&userdata->weights, &weights);
  api->godot_pool_real_array_destroy(&weights);
  return ret;
}

static int pmx_importer_triangle_cb(struct pmx_parse_state *state, int_fast32_t triangle_count, void *userdata_void) {
  pmx_importer_userdata_t *userdata = userdata_void;
  int ret = 0;
  int32_t triangle[3];
  godot_pool_int_array triangles;

  api->godot_pool_int_array_new(&triangles);
  api->godot_pool_int_array_resize(&triangles, triangle_count * 3);
  int j = 0;
  for (int i = 0; i < triangle_count; i++) {
    ret = pmx_parser_next_triangle_int32(state, triangle);
    if (ret != 0) break;

    /* Reverse the winding direction */
    api->godot_pool_int_array_set(&triangles, j++, triangle[2]);
    api->godot_pool_int_array_set(&triangles, j++, triangle[1]);
    api->godot_pool_int_array_set(&triangles, j++, triangle[0]);
  }

  api->godot_variant_new_pool_int_array(&userdata->triangles, &triangles);
  api->godot_pool_int_array_destroy(&triangles);
  return ret;
}

static int pmx_importer_texture_cb(struct pmx_parse_state *state, int_fast32_t texture_count, void *userdata_void) {
  pmx_importer_userdata_t *userdata = userdata_void;
  godot_string name;
  godot_pool_string_array textures;
  char buf[256];
  int ret;

  api->godot_pool_string_array_new(&textures);
  api->godot_pool_string_array_resize(&textures, texture_count);
  for (int i = 0; i < texture_count; i++) {
    ret = pmx_parser_next_texture(state, buf, sizeof buf);
    if (ret != 0) break;
    name = api->godot_string_chars_to_utf8(buf);
    api->godot_pool_string_array_set(&textures, i, &name);
    api->godot_string_destroy(&name);
  }

  api->godot_variant_new_pool_string_array(&userdata->textures, &textures);
  api->godot_pool_string_array_destroy(&textures);
  return ret;
}


static void dict_set(godot_dictionary *dict, const char *key, godot_variant *value) {
  godot_variant kvar;
  godot_string str;

  str = api->godot_string_chars_to_utf8(key);
  api->godot_variant_new_string(&kvar, &str);
  api->godot_string_destroy(&str);
  api->godot_dictionary_set(dict, &kvar, value);
  api->godot_variant_destroy(&kvar);
}


static void dict_set_str(godot_dictionary *dict, const char *key, const char *value) {
  godot_variant vvar;
  godot_string str;

  str = api->godot_string_chars_to_utf8(value);
  api->godot_variant_new_string(&vvar, &str);
  api->godot_string_destroy(&str);
  dict_set(dict, key, &vvar);
  api->godot_variant_destroy(&vvar);
}


static void dict_set_rgb(godot_dictionary *dict, const char *key, const float rgb[3]) {
  godot_variant vvar;
  godot_string str;
  godot_color color;

  api->godot_color_new_rgb(&color, rgb[0], rgb[1], rgb[2]);
  api->godot_variant_new_color(&vvar, &color);
  dict_set(dict, key, &vvar);
  api->godot_variant_destroy(&vvar);
}


static void dict_set_rgba(godot_dictionary *dict, const char *key, const float rgba[4]) {
  godot_variant vvar;
  godot_string str;
  godot_color color;

  api->godot_color_new_rgba(&color, rgba[0], rgba[1], rgba[2], rgba[3]);
  api->godot_variant_new_color(&vvar, &color);
  dict_set(dict, key, &vvar);
  api->godot_variant_destroy(&vvar);
}


static void dict_set_int(godot_dictionary *dict, const char *key, godot_int value) {
  godot_variant vvar;
  godot_string str;
  godot_color color;

  api->godot_variant_new_int(&vvar, value);
  dict_set(dict, key, &vvar);
  api->godot_variant_destroy(&vvar);
}


static int pmx_importer_material_cb(struct pmx_parse_state *state, int_fast32_t count, void *userdata_void) {
  pmx_importer_userdata_t *userdata = userdata_void;
  int result;
  pmx_material_t material;
  godot_dictionary dict;
  godot_variant var;
  godot_array arr;
  const char *str;

  printf("Got %ld materials\n", count);
  api->godot_array_new(&arr);
  api->godot_dictionary_new(&dict);
  for (int i = 0; i < count; i++) {
    result = pmx_parser_next_material(state, &material);
    if (result != 0) {
      api->godot_array_destroy(&arr);
      return result;
    }

    dict_set_str(&dict, "name_local", material.name_local);
    dict_set_str(&dict, "name_universal", material.name_universal);
    dict_set_rgba(&dict, "diffuse", material.diffuse);
    dict_set_rgb(&dict, "specular", material.specular);
    api->godot_variant_new_real(&var, material.specularity);
    dict_set(&dict, "specularity", &var);
    api->godot_variant_destroy(&var);
    dict_set_rgba(&dict, "edge_color", material.edge_color);
    dict_set_int(&dict, "texture", material.texture);
    dict_set_int(&dict, "environment", material.environment);

    switch (material.environment_blend_mode) {
    case disabled:
      str = "disabled";
      break;
    case multiplicative:
      str = "multiplicative";
      break;
    case additive:
      str = "additive";
      break;
    case addl_vec4:
      str = "addl_vec4";
      break;
    default:
      fprintf(stderr, "Invalid environment blend mode %d\n", material.environment_blend_mode);
    }

    dict_set_str(&dict, "environment_blend_mode", str);

    api->godot_variant_new_bool(&var, material.toon_type == internal_ref);
    dict_set(&dict, "toon_internal", &var);
    dict_set_int(&dict, "toon", material.toon);
    dict_set_str(&dict, "metadata", material.metadata);
    dict_set_int(&dict, "triangle_count", material.triangle_count);

    api->godot_variant_new_dictionary(&var, &dict);
    api->godot_array_push_back(&arr, &var);
    api->godot_variant_destroy(&var);
  }

  api->godot_variant_new_array(&userdata->materials, &arr);
  api->godot_array_destroy(&arr);
  return 0;
}

static const pmx_parser_callbacks_t parser_callbacks =
  {
   .model_info_cb = pmx_importer_model_info_cb,
   .vertex_cb = pmx_importer_vertex_cb,
   .triangle_cb = pmx_importer_triangle_cb,
   .texture_cb = pmx_importer_texture_cb,
   .material_cb = pmx_importer_material_cb
  };

static godot_variant pmx_parse(godot_object *obj, void *method_data, void *userdata_void, int num_args, godot_variant **args) {
  pmx_importer_userdata_t *userdata = userdata_void;
  godot_variant ret;
  if (num_args != 1) {
    fprintf(stderr, "Wrong number of arguments %d\n", num_args);
    api->godot_variant_new_int(&ret, -1);
    return ret;
  }

  godot_string filename_string = api->godot_variant_as_string(args[0]);
  godot_char_string filename_utf8 = api->godot_string_utf8(&filename_string);
  const char *filename = api->godot_char_string_get_data(&filename_utf8);
  printf("parse filename \"%s\"\n", filename);
  int result = pmx_parser_parse(filename, &parser_callbacks, userdata);
  api->godot_variant_new_int(&ret, result);
  return ret;
}

#define PMX_GETTER(name)                                                       \
  static godot_variant pmx_get_##name(godot_object *obj, void *method_data,    \
                                      void *userdata_void, int num_args,       \
                                      godot_variant **args) {                  \
    pmx_importer_userdata_t *userdata = userdata_void;                         \
    return userdata->name;                                                     \
  }

PMX_GETTER(model_name_local);
PMX_GETTER(model_name_universal);
PMX_GETTER(comment_local);
PMX_GETTER(comment_universal);
PMX_GETTER(positions);
PMX_GETTER(normals);
PMX_GETTER(uvs);
PMX_GETTER(triangles);
PMX_GETTER(textures);
PMX_GETTER(materials);

/** Library entry point **/
void GDN_EXPORT godot_gdnative_init(godot_gdnative_init_options *o) {
  api = o->api_struct;

  // Now find our extensions.
  for (int i = 0; i < api->num_extensions; i++) {
    switch (api->extensions[i]->type) {
    case GDNATIVE_EXT_NATIVESCRIPT: {
      nativescript_api = (godot_gdnative_ext_nativescript_api_struct *)api->extensions[i];
    }; break;
    default: break;
    }
  }
}

void GDN_EXPORT godot_gdnative_terminate(godot_gdnative_terminate_options *o) {
}

#define PMX_METHOD(name)                                                       \
  method.method = &pmx_##name;                                                 \
  nativescript_api->godot_nativescript_register_method(desc, "PMX", #name,     \
                                                       attr, method)

void GDN_EXPORT godot_nativescript_init(void *desc) {
  godot_instance_create_func create_func = { .create_func = &pmx_constructor, .method_data = 0, .free_func = 0 };
  godot_instance_destroy_func destroy_func = { .destroy_func = &pmx_destructor, .method_data = 0, .free_func = 0 };
  nativescript_api->godot_nativescript_register_class(desc, "PMX", "Object", create_func, destroy_func);

  godot_method_attributes attr = { .rpc_type = GODOT_METHOD_RPC_MODE_DISABLED };
  godot_instance_method method;
  method.method_data = 0;
  method.free_func = 0;

  PMX_METHOD(parse);
  PMX_METHOD(get_model_name_local);
  PMX_METHOD(get_model_name_universal);
  PMX_METHOD(get_comment_local);
  PMX_METHOD(get_comment_universal);
  PMX_METHOD(get_positions);
  PMX_METHOD(get_normals);
  PMX_METHOD(get_uvs);
  PMX_METHOD(get_triangles);
  PMX_METHOD(get_textures);
  PMX_METHOD(get_materials);
}

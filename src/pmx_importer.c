#include <gdnative_api_struct.gen.h>

#include <stdint.h>
#include <stdio.h>

#include "parser.h"

static const godot_gdnative_core_api_struct *api = NULL;
static const godot_gdnative_ext_nativescript_api_struct *nativescript_api = NULL;

typedef struct {
  godot_string model_name_local;
  godot_string model_name_universal;
  godot_string comment_local;
  godot_string comment_universal;
  int_fast32_t vertex_count;
  int_fast32_t triangle_count;
  godot_pool_vector3_array positions;
  godot_pool_vector3_array normals;
  godot_pool_vector2_array uvs;
  godot_pool_int_array triangles;
  godot_pool_int_array bones;
  godot_pool_real_array weights;
  godot_array textures;
  godot_array materials;
} pmx_importer_userdata_t;

static void *pmx_constructor(godot_object *obj, void *method_data) {
  pmx_importer_userdata_t *userdata = api->godot_alloc(sizeof *userdata);
  api->godot_pool_vector3_array_new(&userdata->positions);
  api->godot_pool_vector3_array_new(&userdata->normals);
  api->godot_pool_vector2_array_new(&userdata->uvs);
  api->godot_pool_int_array_new(&userdata->triangles);
  api->godot_pool_int_array_new(&userdata->bones);
  api->godot_pool_real_array_new(&userdata->weights);
  api->godot_array_new(&userdata->textures);
  api->godot_array_new(&userdata->materials);
  return userdata;
}

static void pmx_destructor(godot_object *obj, void *method_data, void *userdata_void) {
  pmx_importer_userdata_t *userdata = userdata_void;
  api->godot_string_destroy(&userdata->model_name_local);
  api->godot_string_destroy(&userdata->model_name_universal);
  api->godot_string_destroy(&userdata->comment_local);
  api->godot_string_destroy(&userdata->comment_universal);
  api->godot_pool_vector3_array_destroy(&userdata->positions);
  api->godot_pool_vector3_array_destroy(&userdata->normals);
  api->godot_pool_vector2_array_destroy(&userdata->uvs);
  api->godot_pool_int_array_destroy(&userdata->triangles);
  api->godot_pool_int_array_destroy(&userdata->bones);
  api->godot_pool_real_array_destroy(&userdata->weights);
  api->godot_array_destroy(&userdata->textures);
  api->godot_array_destroy(&userdata->materials);
  api->godot_free(userdata);
}

static int pmx_importer_model_info_cb(pmx_model_info_t *model, void *userdata_void) {
  pmx_importer_userdata_t *userdata = userdata_void;
  userdata->model_name_local = api->godot_string_chars_to_utf8(model->model_name_local);
  userdata->model_name_universal = api->godot_string_chars_to_utf8(model->model_name_universal);
  userdata->comment_local = api->godot_string_chars_to_utf8(model->comment_local);
  userdata->comment_universal = api->godot_string_chars_to_utf8(model->comment_universal);
  switch (model->text_encoding) {
  case utf16le:
    printf("Encoding for \"%s\" is utf16le\n", model->model_name_local);
    break;
  case utf8:
    printf("Encoding for \"%s\" is utf8\n", model->model_name_local);
    break;
  }
  return 0;
}

static void pmx_importer_parse_bone_weights(pmx_importer_userdata_t *userdata, pmx_vertex_t *vertex, uint_fast32_t i) {
  godot_int index = i * 4;
  int_fast32_t *bones = vertex->deform.non_sdef.bones;
  float *weights = vertex->deform.non_sdef.weights;
  float renorm;
  switch (vertex->deform_type) {
  case bdef1:
    api->godot_pool_int_array_set(&userdata->bones, i, bones[0]);
    api->godot_pool_real_array_set(&userdata->weights, i++, 1.0);
    api->godot_pool_int_array_set(&userdata->bones, i, -1);
    api->godot_pool_real_array_set(&userdata->weights, i++, 0.0);
    api->godot_pool_int_array_set(&userdata->bones, i, -1);
    api->godot_pool_real_array_set(&userdata->weights, i++, 0.0);
    api->godot_pool_int_array_set(&userdata->bones, i, -1);
    api->godot_pool_real_array_set(&userdata->weights, i, 0.0);
    break;
  case bdef2:
    api->godot_pool_int_array_set(&userdata->bones, i, bones[0]);
    api->godot_pool_real_array_set(&userdata->weights, i++, weights[0]);
    api->godot_pool_int_array_set(&userdata->bones, i, bones[1]);
    api->godot_pool_real_array_set(&userdata->weights, i++, 1.0-weights[0]);
    api->godot_pool_int_array_set(&userdata->bones, i, -1);
    api->godot_pool_real_array_set(&userdata->weights, i++, 0.0);
    api->godot_pool_int_array_set(&userdata->bones, i, -1);
    api->godot_pool_real_array_set(&userdata->weights, i, 0.0);
    break;
  case bdef4:
    /* Make the weights sum to 1 by renormalizing */
    renorm = 1.0 / (weights[0] + weights[1] + weights[2] + weights[3]);
    api->godot_pool_int_array_set(&userdata->bones, i, bones[0]);
    api->godot_pool_real_array_set(&userdata->weights, i++, weights[0] * renorm);
    api->godot_pool_int_array_set(&userdata->bones, i, bones[1]);
    api->godot_pool_real_array_set(&userdata->weights, i++, weights[1] * renorm);
    api->godot_pool_int_array_set(&userdata->bones, i, bones[2]);
    api->godot_pool_real_array_set(&userdata->weights, i++, weights[2] * renorm);
    api->godot_pool_int_array_set(&userdata->bones, i, bones[3]);
    api->godot_pool_real_array_set(&userdata->weights, i, weights[3] * renorm);
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
  int ret;
  godot_vector3 vec3;
  godot_vector2 vec2;

  userdata->vertex_count = vertex_count;
  printf("Got %ld vertices\n", vertex_count);

  /* Reserve space for the vertex data */
  api->godot_pool_vector3_array_resize(&userdata->positions, vertex_count);
  api->godot_pool_vector3_array_resize(&userdata->normals, vertex_count);
  api->godot_pool_vector2_array_resize(&userdata->uvs, vertex_count);
  api->godot_pool_int_array_resize(&userdata->bones, vertex_count * 4);
  api->godot_pool_real_array_resize(&userdata->weights, vertex_count * 4);
  for (int i = 0; i < vertex_count; i++) {
    ret = pmx_parser_next_vertex(state, &vertex);
    if (ret != 0) return ret;
    api->godot_vector3_new(&vec3, vertex.position[0], vertex.position[1], vertex.position[2]);
    api->godot_pool_vector3_array_set(&userdata->positions, i, &vec3);
    api->godot_vector3_new(&vec3, vertex.normal[0], vertex.normal[1], vertex.normal[2]);
    api->godot_pool_vector3_array_set(&userdata->normals, i, &vec3);
    api->godot_vector2_new(&vec2, vertex.uv[0], vertex.uv[1]);
    api->godot_pool_vector2_array_set(&userdata->uvs, i, &vec2);
    pmx_importer_parse_bone_weights(userdata, &vertex, i);
  }
  return 0;
}

static int pmx_importer_triangle_cb(struct pmx_parse_state *state, int_fast32_t triangle_count, void *userdata_void) {
  pmx_importer_userdata_t *userdata = userdata_void;
  int ret;
  int32_t triangle[3];

  userdata->triangle_count = triangle_count;
  printf("Got %ld triangles\n", triangle_count);
  api->godot_pool_int_array_resize(&userdata->triangles, triangle_count * 3);
  int j = 0;
  for (int i = 0; i < triangle_count; i++) {
    ret = pmx_parser_next_triangle_int32(state, triangle);
    if (ret != 0) return ret;
    if (triangle[0] > userdata->vertex_count || triangle[1] > userdata->vertex_count || triangle[2] > userdata->vertex_count || triangle[0] < 0 || triangle[1] < 0 || triangle[2] < 0) {
      fprintf(stderr, "Bad index");
      return -1;
    }

    /* Reverse the winding direction */
    api->godot_pool_int_array_set(&userdata->triangles, j++, triangle[2]);
    api->godot_pool_int_array_set(&userdata->triangles, j++, triangle[1]);
    api->godot_pool_int_array_set(&userdata->triangles, j++, triangle[0]);
  }
  return 0;
}

static int pmx_importer_texture_cb(struct pmx_parse_state *state, int_fast32_t texture_count, void *userdata_void) {
  pmx_importer_userdata_t *userdata = userdata_void;
  godot_string name;
  godot_variant name_variant;
  char buf[256];
  int result;
  for (int i = 0; i < texture_count; i++) {
    result = pmx_parser_next_texture(state, buf, sizeof buf);
    if (result != 0) {
      return result;
    }
    name = api->godot_string_chars_to_utf8(buf);
    api->godot_variant_new_string(&name_variant, &name);
    api->godot_array_push_back(&userdata->textures, &name_variant);
    api->godot_variant_destroy(&name_variant);
    api->godot_string_destroy(&name);
  }
  return 0;
}

static int pmx_importer_material_cb(struct pmx_parse_state *state, int_fast32_t count, void *userdata_void) {
  pmx_importer_userdata_t *userdata = userdata_void;
  godot_string name;
  godot_variant name_variant;
  int result;
  pmx_material_t material;

  printf("Got %ld materials\n", count);
  for (int i = 0; i < count; i++) {
    result = pmx_parser_next_material(state, &material);
    if (result != 0) return result;
    printf("Got material %s\n", material.name_local);
  }
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

godot_variant pmx_parse(godot_object *obj, void *method_data, void *userdata_void, int num_args, godot_variant **args) {
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

godot_variant pmx_get_model_name_local(godot_object *obj, void *method_data,
                                void *userdata_void, int num_args,
                                godot_variant **args) {
  pmx_importer_userdata_t *userdata = userdata_void;
  godot_variant ret;

  api->godot_variant_new_string(&ret, &userdata->model_name_local);
  return ret;
}

godot_variant pmx_get_model_name_universal(godot_object *obj, void *method_data,
                                void *userdata_void, int num_args,
                                godot_variant **args) {
  pmx_importer_userdata_t *userdata = userdata_void;
  godot_variant ret;

  api->godot_variant_new_string(&ret, &userdata->model_name_universal);
  return ret;
}

godot_variant pmx_get_comment_local(godot_object *obj, void *method_data,
				    void *userdata_void, int num_args,
				    godot_variant **args) {
  pmx_importer_userdata_t *userdata = userdata_void;
  godot_variant ret;

  api->godot_variant_new_string(&ret, &userdata->comment_local);
  return ret;
}

godot_variant pmx_get_comment_universal(godot_object *obj, void *method_data,
					void *userdata_void, int num_args,
					godot_variant **args) {
  pmx_importer_userdata_t *userdata = userdata_void;
  godot_variant ret;

  api->godot_variant_new_string(&ret, &userdata->comment_universal);
  return ret;
}

godot_variant pmx_get_positions(godot_object *obj, void *method_data,
                                void *userdata_void, int num_args,
                                godot_variant **args) {
  pmx_importer_userdata_t *userdata = userdata_void;
  godot_variant ret;

  api->godot_variant_new_pool_vector3_array(&ret, &userdata->positions);
  return ret;
}


godot_variant pmx_get_normals(godot_object *obj, void *method_data,
                              void *userdata_void, int num_args,
                              godot_variant **args) {
  pmx_importer_userdata_t *userdata = userdata_void;
  godot_variant ret;

  api->godot_variant_new_pool_vector3_array(&ret, &userdata->normals);
  return ret;
}


godot_variant pmx_get_uvs(godot_object *obj, void *method_data,
                          void *userdata_void, int num_args,
                          godot_variant **args) {
  pmx_importer_userdata_t *userdata = userdata_void;
  godot_variant ret;

  api->godot_variant_new_pool_vector2_array(&ret, &userdata->uvs);
  return ret;
}


godot_variant pmx_get_triangles(godot_object *obj, void *method_data,
                                void *userdata_void, int num_args,
                                godot_variant **args) {
  pmx_importer_userdata_t *userdata = userdata_void;
  godot_variant ret;

  api->godot_variant_new_pool_int_array(&ret, &userdata->triangles);
  return ret;
}


godot_variant pmx_get_textures(godot_object *obj, void *method_data,
			       void *userdata_void, int num_args,
			       godot_variant **args) {
  pmx_importer_userdata_t *userdata = userdata_void;
  godot_variant ret;

  api->godot_variant_new_array(&ret, &userdata->textures);
  return ret;
}


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

void GDN_EXPORT godot_nativescript_init(void *desc) {
  godot_instance_create_func create_func = { .create_func = &pmx_constructor, .method_data = 0, .free_func = 0 };
  godot_instance_destroy_func destroy_func = { .destroy_func = &pmx_destructor, .method_data = 0, .free_func = 0 };
  nativescript_api->godot_nativescript_register_class(desc, "PMX", "Object", create_func, destroy_func);

  godot_method_attributes attr = { .rpc_type = GODOT_METHOD_RPC_MODE_DISABLED };
  godot_instance_method method;
  method.method_data = 0;
  method.free_func = 0;

  method.method = &pmx_parse;
  nativescript_api->godot_nativescript_register_method(desc, "PMX", "parse", attr, method);
  method.method = &pmx_get_model_name_local;
  nativescript_api->godot_nativescript_register_method(desc, "PMX", "get_model_name_local", attr, method);
  method.method = &pmx_get_model_name_universal;
  nativescript_api->godot_nativescript_register_method(desc, "PMX", "get_model_name_universal", attr, method);
  method.method = &pmx_get_comment_local;
  nativescript_api->godot_nativescript_register_method(desc, "PMX", "get_comment_local", attr, method);
  method.method = &pmx_get_comment_universal;
  nativescript_api->godot_nativescript_register_method(desc, "PMX", "get_comment_universal", attr, method);
  method.method = &pmx_get_positions;
  nativescript_api->godot_nativescript_register_method(desc, "PMX", "get_positions", attr, method);
  method.method = &pmx_get_normals;
  nativescript_api->godot_nativescript_register_method(desc, "PMX", "get_normals", attr, method);
  method.method = &pmx_get_uvs;
  nativescript_api->godot_nativescript_register_method(desc, "PMX", "get_uvs", attr, method);
  method.method = &pmx_get_triangles;
  nativescript_api->godot_nativescript_register_method(desc, "PMX", "get_triangles", attr, method);
  method.method = &pmx_get_textures;
  nativescript_api->godot_nativescript_register_method(desc, "PMX", "get_textures", attr, method);
}

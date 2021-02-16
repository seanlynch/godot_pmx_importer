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
} pmx_importer_userdata_t;

static void *pmx_constructor(godot_object *obj, void *method_data) {
  pmx_importer_userdata_t *userdata = api->godot_alloc(sizeof *userdata);
  api->godot_pool_vector3_array_new(&userdata->positions);
  api->godot_pool_vector3_array_new(&userdata->normals);
  api->godot_pool_vector2_array_new(&userdata->uvs);
  api->godot_pool_int_array_new(&userdata->triangles);
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
  api->godot_free(userdata);
}

static int pmx_importer_model_info_cb(pmx_model_info_t *model, void *userdata_void) {
  pmx_importer_userdata_t *userdata = userdata_void;
  userdata->model_name_local = api->godot_string_chars_to_utf8(model->model_name_local);
  userdata->model_name_universal = api->godot_string_chars_to_utf8(model->model_name_universal);
  userdata->comment_local = api->godot_string_chars_to_utf8(model->comment_local);
  userdata->comment_universal = api->godot_string_chars_to_utf8(model->comment_universal);
  return 0;
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
  api->godot_pool_vector3_array_new(&userdata->positions);
  api->godot_pool_vector3_array_resize(&userdata->positions, vertex_count);
  api->godot_pool_vector3_array_new(&userdata->normals);
  api->godot_pool_vector3_array_resize(&userdata->normals, vertex_count);
  api->godot_pool_vector2_array_new(&userdata->uvs);
  api->godot_pool_vector2_array_resize(&userdata->uvs, vertex_count);
  for (int i = 0; i < vertex_count; i++) {
    ret = pmx_parser_next_vertex(state, &vertex);
    if (ret != 0) return ret;
    api->godot_vector3_new(&vec3, vertex.position[0], vertex.position[1], vertex.position[2]);
    api->godot_pool_vector3_array_set(&userdata->positions, i, &vec3);
    api->godot_vector3_new(&vec3, vertex.normal[0], vertex.normal[1], vertex.normal[2]);
    api->godot_pool_vector3_array_set(&userdata->normals, i, &vec3);
    api->godot_vector2_new(&vec2, vertex.uv[0], vertex.uv[1]);
    api->godot_pool_vector2_array_set(&userdata->uvs, i, &vec2);
  }
  /* TODO bone deformations */
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
    api->godot_pool_int_array_set(&userdata->triangles, j++, triangle[2]);
    api->godot_pool_int_array_set(&userdata->triangles, j++, triangle[1]);
    api->godot_pool_int_array_set(&userdata->triangles, j++, triangle[0]);
  }
  return 0;
}

static const pmx_parser_callbacks_t parser_callbacks =
  {
   .model_info_cb = pmx_importer_model_info_cb,
   .vertex_cb = pmx_importer_vertex_cb,
   .triangle_cb = pmx_importer_triangle_cb
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
}

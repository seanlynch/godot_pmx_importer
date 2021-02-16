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
  uint_fast32_t vertex_count;
  godot_pool_real_array positions;
  godot_pool_real_array normals;
  godot_pool_real_array uvs;
} pmx_importer_userdata_t;

static int pmx_importer_pre_vertex_cb(pmx_model_info_t *model, void *userdata_void) {
  pmx_importer_userdata_t *userdata = userdata_void;
  userdata->model_name_local = api->godot_string_chars_to_utf8(model->model_name_local);
  userdata->model_name_universal = api->godot_string_chars_to_utf8(model->model_name_universal);
  userdata->comment_local = api->godot_string_chars_to_utf8(model->comment_local);
  userdata->comment_universal = api->godot_string_chars_to_utf8(model->comment_universal);
  userdata->vertex_count = model->vertex_count;
  printf("Got %ld vertices\n", userdata->vertex_count);
  /* Reserve space for the vertex data */
  api->godot_pool_real_array_new(&userdata->positions);
  api->godot_pool_real_array_resize(&userdata->positions, model->vertex_count * 3);
  api->godot_pool_real_array_new(&userdata->normals);
  api->godot_pool_real_array_resize(&userdata->normals, model->vertex_count * 3);
  api->godot_pool_real_array_new(&userdata->uvs);
  api->godot_pool_real_array_resize(&userdata->uvs, model->vertex_count * 2);
  return 0;
}

static int pmx_importer_vertex_cb(pmx_vertex_t *vertex, void *userdata_void) {
  pmx_importer_userdata_t *userdata = userdata_void;
  api->godot_pool_real_array_append(&userdata->positions, vertex->position[0]);
  api->godot_pool_real_array_append(&userdata->positions, vertex->position[1]);
  api->godot_pool_real_array_append(&userdata->positions, vertex->position[2]);
  api->godot_pool_real_array_append(&userdata->normals, vertex->normal[0]);
  api->godot_pool_real_array_append(&userdata->normals, vertex->normal[1]);
  api->godot_pool_real_array_append(&userdata->normals, vertex->normal[2]);
  api->godot_pool_real_array_append(&userdata->uvs, vertex->uv[0]);
  api->godot_pool_real_array_append(&userdata->uvs, vertex->uv[1]);
  /* TODO bone deformations */
  return 0;
}


static const pmx_parser_callbacks_t parser_callbacks =
  {
   .pre_vertex_cb = pmx_importer_pre_vertex_cb,
   .vertex_cb = pmx_importer_vertex_cb,
   .triangle_cb = pmx_importer_triangle_cb
  };

void *pmx_constructor(godot_object *obj, void *method_data) {
  pmx_importer_userdata_t *userdata = api->godot_alloc(sizeof *userdata);
  return userdata;
}

void pmx_destructor(godot_object *obj, void *method_data, void *userdata) {
  /* TODO free strings & arrays */
  api->godot_free(userdata);
}

godot_variant pmx_parse(godot_object *obj, void *method_data, pmx_importer_userdata_t *userdata, int num_args, godot_variant **args) {
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

/** Library de-initialization **/
void GDN_EXPORT godot_gdnative_terminate(godot_gdnative_terminate_options *o) {
}

/** Script entry (Registering all the classes and stuff) **/
void GDN_EXPORT godot_nativescript_init(void *desc) {
  printf("nativescript init\n");

  godot_instance_create_func create_func = {
					    .create_func = &pmx_constructor,
					    .method_data = 0,
					    .free_func   = 0
  };

  godot_instance_destroy_func destroy_func = {
					      .destroy_func = &pmx_destructor,
					      .method_data  = 0,
					      .free_func    = 0
  };

  nativescript_api->godot_nativescript_register_class(desc, "PMX", "Object", create_func, destroy_func);

  {
    godot_instance_method method = {
				    .method = &pmx_parse,
				    .method_data = 0,
				    .free_func = 0
    };

    godot_method_attributes attr = {
				    .rpc_type = GODOT_METHOD_RPC_MODE_DISABLED
    };

    nativescript_api->godot_nativescript_register_method(desc, "PMX", "parse", attr, method);
  }
}

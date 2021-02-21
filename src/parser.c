#include <assert.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "parser.h"
#include "utf.h"


typedef struct pmx_parse_state {
  const uint8_t *data;
  size_t offset;
  size_t size;
  const pmx_parser_callbacks_t *callbacks;
  pmx_model_info_t model;
  void *userdata;
  jmp_buf env;
} pmx_parse_state_t;


static void parse_error(pmx_parse_state_t *state, int result, char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fprintf(stderr, "\n");
  longjmp(state->env, -1);
}


static void pmx_state_init(pmx_parse_state_t *state, const uint8_t *data, size_t size, const pmx_parser_callbacks_t *callbacks, void *userdata) {
  bzero(state, sizeof *state);
  state->data = data;
  state->size = size;
  state->callbacks = callbacks;
  state->userdata = userdata;
}


static void check_size(pmx_parse_state_t *state, size_t needed) {
  if (state->offset + needed > state->size) {
    parse_error(state, -1, "Not enough data. Needed %zd, got %zd", needed, state->size - state->offset);
  }
}


static const uint8_t *parse_bytes(pmx_parse_state_t *state, size_t length) {
  check_size(state, length);
  const uint8_t *ptr = &state->data[state->offset];
  state->offset += length;
  return ptr;
}


static void copy_bytes(pmx_parse_state_t *state, char *dest, size_t length) {
  const uint8_t *src = parse_bytes(state, length);
  memcpy(dest, src, length);
}


#define PARSER(name, ftype, rtype)                                             \
  static rtype parse_##name(pmx_parse_state_t *state) {			\
    check_size(state, sizeof(ftype));                                          \
    rtype result = *(ftype *)(&state->data[state->offset]);                    \
    state->offset += sizeof(ftype);                                            \
    return result;                                                             \
  }


#define RPARSER(name, ftype, rtype, repeat)                                    \
  static void parse_##name(pmx_parse_state_t *state, rtype result[repeat]) {       \
    check_size(state, sizeof(ftype) * repeat);                                 \
    for (int i = 0; i < repeat; i++) {                                         \
      result[i] = *(ftype *)(&state->data[state->offset]);                     \
      state->offset += sizeof(ftype);                                          \
    }                                                                          \
  }


#define IPARSER(name, rtype, parser1, parser2, parser4)                        \
  static rtype parse_##name(pmx_parse_state_t *state) {                        \
    switch (state->model.name##_size) {                                        \
    case 1:                                                                    \
      return parse_##parser1(state);                                           \
    case 2:                                                                    \
      return parse_##parser2(state);                                           \
    case 4:                                                                    \
      return parse_##parser4(state);                                           \
    default:                                                                   \
      parse_error(state, -1, "Unsupported index size %d for " #name,               \
                  state->model.name##_size);                                   \
      return 0;                                                                \
    }                                                                          \
  }


PARSER(int, int32_t, int_fast32_t);
PARSER(uint, uint32_t, uint_fast32_t);
PARSER(float, float, float);
PARSER(byte, int8_t, int_fast8_t);
PARSER(ubyte, uint8_t, uint_fast8_t);
PARSER(short, int16_t, int_fast16_t);
PARSER(ushort, uint16_t, uint_fast16_t);

RPARSER(vec2, float, float, 2);
RPARSER(vec3, float, float, 3);
RPARSER(vec4, float, float, 4);

IPARSER(bone_index, int_fast32_t, byte, short, int);
IPARSER(texture_index, int_fast32_t, byte, short, int);


static size_t parse_text(pmx_parse_state_t *state, char *buf, size_t bufsize) {
  uint_fast32_t length = parse_int(state);
  if (state->model.text_encoding == utf16le) {
    ssize_t decoded = utf16le_to_utf8(parse_bytes(state, length), length, buf, bufsize);
    if (decoded == -1) parse_error(state, -1, "Buffer wasn't long enough.");
    return decoded;
  } else {
    if (bufsize < length + 1) parse_error(state, -1, "Buffer isn't long enough. Wanted %zd got $zd", length + 1, bufsize);
    copy_bytes(state, buf, length);
    buf[length] = '\0';
    return length;
  }
}


static void parse_skip(pmx_parse_state_t *state, size_t bytes) {
  check_size(state, bytes);
  state->offset += bytes;
}


#define PARSE_TEXT(buf)                                                        \
  parse_text(state, buf, sizeof(buf))


const char *pmx_deform_type_string(pmx_deform_type_t t) {
  switch(t) {
  case bdef1:
    return "bdef1";
  case bdef2:
    return "bdef2";
  case bdef4:
    return "bdef4";
  case sdef:
    return "sdef";
  case qdef:
    return "qdef";
  }
}

static void format_vec2(char *buf, size_t size, float v[2]) {
  snprintf(buf, size, "%f %f", v[0], v[1]);
}

static void format_vec3(char *buf, size_t size, float v[3]) {
  snprintf(buf, size, "%f %f %f", v[0], v[1], v[2]);
}

static void print_vertex(pmx_vertex_t *vertex) {
  char buf[40];
  format_vec3(buf, sizeof buf, vertex->position);
  printf("Position: %s\n", buf);
  format_vec3(buf, sizeof buf, vertex->normal);
  printf("Normal: %s\n", buf);
  format_vec2(buf, sizeof buf, vertex->uv);
  printf("UV: %s\n", buf);
  printf("Deformation type: %s\n", pmx_deform_type_string(vertex->deform_type));
}

int pmx_parser_next_vertex(pmx_parse_state_t *state, pmx_vertex_t *vertex) {
  /* Avoid unwinding the stack past user code */
  int ret = setjmp(state->env);
  if (ret != 0) {
    return ret;
  }
  parse_vec3(state, vertex->position);
  parse_vec3(state, vertex->normal);
  parse_vec2(state, vertex->uv);
  parse_skip(state, state->model.addl_vec4_count * 16);
  vertex->deform_type = parse_byte(state);
  switch(vertex->deform_type) {
  case bdef1:
    vertex->bones[0] = parse_bone_index(state);
    vertex->bones[1] = 0;
    vertex->bones[2] = 0;
    vertex->bones[3] = 0;
    vertex->weights[0] = 1.0;
    vertex->weights[1] = 0.0;
    vertex->weights[2] = 0.0;
    vertex->weights[3] = 0.0;
    break;
  case bdef2:
    vertex->bones[0] = parse_bone_index(state);
    vertex->bones[1] = parse_bone_index(state);
    vertex->bones[2] = 0;
    vertex->bones[3] = 0;
    vertex->weights[0] = parse_float(state);
    vertex->weights[1] = 0.0;
    vertex->weights[2] = 0.0;
    vertex->weights[3] = 0.0;
    break;
  case bdef4:
  case qdef:
    vertex->bones[0] = parse_bone_index(state);
    vertex->bones[1] = parse_bone_index(state);
    vertex->bones[2] = parse_bone_index(state);
    vertex->bones[3] = parse_bone_index(state);
    vertex->weights[0] = parse_float(state);
    vertex->weights[1] = parse_float(state);
    vertex->weights[2] = parse_float(state);
    vertex->weights[3] = parse_float(state);
    break;
  case sdef:
    vertex->bones[0] = parse_bone_index(state);
    vertex->bones[1] = parse_bone_index(state);
    vertex->bones[2] = 0;
    vertex->bones[3] = 0;
    vertex->weights[0] = parse_float(state);
    vertex->weights[1] = 1.0 - vertex->weights[0];
    vertex->weights[2] = 0.0;
    vertex->weights[3] = 0.0;
    parse_vec3(state, vertex->c);
    parse_vec3(state, vertex->r0);
    parse_vec3(state, vertex->r1);
    break;
  }
  vertex->outline_scale = parse_float(state);
  return 0;
}


/* For convenience, define triangle parsers for uint8, uint16, and int32 */
#define TRIANGLE_PARSER(T)                                                     \
  int pmx_parser_next_triangle_##T(pmx_parse_state_t *state, T##_t *buf) {     \
    int ret = setjmp(state->env);                                              \
    if (ret != 0) {                                                            \
      return ret;                                                              \
    }                                                                          \
    size_t size = state->model.vertex_index_size;                              \
    check_size(state, size * 3);                                               \
    size_t offset = state->offset;                                             \
    switch (size) {                                                            \
    case 1:                                                                    \
      buf[0] = *(uint8_t *)(&state->data[offset]);                             \
      offset += size;                                                          \
      buf[1] = *(uint8_t *)(&state->data[offset]);                             \
      offset += size;                                                          \
      buf[2] = *(uint8_t *)(&state->data[offset]);                             \
      offset += size;                                                          \
      break;                                                                   \
    case 2:                                                                    \
      buf[0] = *(uint16_t *)(&state->data[offset]);                            \
      offset += size;                                                          \
      buf[1] = *(uint16_t *)(&state->data[offset]);                            \
      offset += size;                                                          \
      buf[2] = *(uint16_t *)(&state->data[offset]);                            \
      offset += size;                                                          \
      break;                                                                   \
    case 4:                                                                    \
      buf[0] = *(int32_t *)(&state->data[offset]);                             \
      offset += size;                                                          \
      buf[1] = *(int32_t *)(&state->data[offset]);                             \
      offset += size;                                                          \
      buf[2] = *(int32_t *)(&state->data[offset]);                             \
      offset += size;                                                          \
      break;                                                                   \
    }                                                                          \
    state->offset = offset;                                                    \
    return 0;                                                                  \
  }


TRIANGLE_PARSER(uint8);
TRIANGLE_PARSER(uint16);
TRIANGLE_PARSER(int32);


int pmx_parser_next_texture(pmx_parse_state_t *state, char *buf, size_t bufsize) {
  int ret = setjmp(state->env);
  if (ret != 0) return ret;

  parse_text(state, buf, bufsize);
  return 0;
}


int pmx_parser_next_material(pmx_parse_state_t *state, pmx_material_t *material) {
  int ret = setjmp(state->env);
  if (ret != 0) return ret;

  PARSE_TEXT(material->name_local);
  PARSE_TEXT(material->name_universal);
  parse_vec4(state, material->diffuse);
  parse_vec3(state, material->specular);
  material->specularity = parse_float(state);
  parse_vec3(state, material->ambient);
  material->flags = parse_byte(state);
  parse_vec4(state, material->edge_color);
  material->edge_scale = parse_float(state);
  material->texture = parse_texture_index(state);
  material->environment = parse_texture_index(state);
  material->environment_blend_mode = parse_byte(state);
  material->toon_type = parse_byte(state);
  switch (material->toon_type) {
  case texture_ref:
    material->toon = parse_texture_index(state);
    break;
  case internal_ref:
    material->toon = parse_byte(state);
    break;
  default:
    parse_error(state, -1, "Invalid toon reference type %d", material->toon_type);
  }
  PARSE_TEXT(material->metadata);
  int_fast32_t index_count = parse_int(state);
  if (index_count %3 != 0) parse_error(state, -1, "Index count %d is not a multiple of 3", index_count);
  material->index_count = index_count;
  return 0;
}

static void parse_ik_link(pmx_parse_state_t *state, pmx_bone_ik_link_t *link) {
  link->bone = parse_bone_index(state);
  link->has_limits = parse_byte(state) == 1;
  if (link->has_limits) {
    parse_vec3(state, link->limit_min);
    parse_vec3(state, link->limit_max);
  }
}
int pmx_parser_next_bone(pmx_parse_state_t *state, pmx_bone_t *bone) {
  int ret = setjmp(state->env);
  if (ret != 0) return ret;

  PARSE_TEXT(bone->name_local);
  PARSE_TEXT(bone->name_universal);
  parse_vec3(state, bone->position);
  bone->parent = parse_bone_index(state);
  bone->layer = parse_int(state);
  bone->flags = parse_ushort(state);

  if (bone->flags & PMX_BONE_FLAG_INDEXED_TAIL_POS) {
    bone->tail_bone = parse_bone_index(state);
  } else {
    bone->tail_bone = -1;
    parse_vec3(state, bone->tail_position);
  }

  if (bone->flags & (PMX_BONE_FLAG_INHERIT_ROTATION | PMX_BONE_FLAG_INHERIT_TRANSLATION)) {
    bone->inherit.parent = parse_bone_index(state);
    bone->inherit.weight = parse_float(state);
  } else {
    bone->inherit.parent = -1;
  }

  if (bone->flags & PMX_BONE_FLAG_FIXED_AXIS) parse_vec3(state, bone->fixed_axis);
  if (bone->flags & PMX_BONE_FLAG_LOCAL_COORD) {
    parse_vec3(state, bone->local_coord.x_axis);
    parse_vec3(state, bone->local_coord.z_axis);
  }

  if (bone->flags & PMX_BONE_FLAG_EXTERNAL_PARENT_DEFORM) {
    bone->external_parent = parse_bone_index(state);
  } else {
    bone->external_parent = -1;
  }

  if (bone->flags & PMX_BONE_FLAG_IK) {
    bone->ik.target = parse_bone_index(state);
    bone->ik.loop_count = parse_int(state);
    bone->ik.limit_radian = parse_float(state);
    int_fast32_t link_count = parse_int(state);

    if (link_count > 0) {
      if (link_count <= PMX_MAX_IK_LINKS) {
	bone->ik.link_count = link_count;
      } else {
	fprintf(stderr, "IK link count %ld is greater than PMX_MAX_IK_LINKS\n", link_count);
	bone->ik.link_count = PMX_MAX_IK_LINKS;
      }

      for (int i = 0; i < bone->ik.link_count; i++) {
	parse_ik_link(state, &bone->ik.ik_links[i]);
      }

      /* Parse any links above the limit */
      for (int i = bone->ik.link_count; i < link_count; i++) {
	pmx_bone_ik_link_t dummy;
	parse_ik_link(state, &dummy);
      }
    } else {
      bone->ik.link_count = 0;
    }
  }
  return 0;
}


#define CHECK_ISIZE(x) if (state->model.x != 1 && state->model.x != 2 && state->model.x != 4) parse_error(state, -1, "Invalid index size for " #x, state->model.x)


static int pmx_parser_parse_header(pmx_parse_state_t *state) {
  if (memcmp(parse_bytes(state, 4), "PMX ", 4)) parse_error(state, -1, "Bad signature");
  float version = parse_float(state);
  if (version != 2.0 && version != 2.1) parse_error(state, -1, "Unsupported version %f", version);
  int global_count = parse_byte(state);
  if (global_count != 8) parse_error(state, -1, "Unexpected number of globals. Expected 8 got %d", global_count);

  state->model.text_encoding = parse_byte(state);
  state->model.addl_vec4_count = parse_byte(state);
  if (state->model.addl_vec4_count > 4 || state->model.addl_vec4_count < 0) {
    parse_error(state, -1, "Invalid addl_vec4_count %d", state->model.addl_vec4_count);
  }
  state->model.vertex_index_size = parse_byte(state);
  CHECK_ISIZE(vertex_index_size);
  state->model.texture_index_size = parse_byte(state);
  CHECK_ISIZE(texture_index_size);
  state->model.material_index_size = parse_byte(state);
  CHECK_ISIZE(material_index_size);
  state->model.bone_index_size = parse_byte(state);
  CHECK_ISIZE(bone_index_size);
  state->model.morph_index_size = parse_byte(state);
  CHECK_ISIZE(morph_index_size);
  state->model.rigidbody_index_size = parse_byte(state);
  CHECK_ISIZE(rigidbody_index_size);

  PARSE_TEXT(state->model.model_name_local);
  PARSE_TEXT(state->model.model_name_universal);
  PARSE_TEXT(state->model.comment_local);
  PARSE_TEXT(state->model.comment_universal);
  return state->callbacks->model_info_cb(&state->model, state->userdata);
}


int pmx_parser_parse(const uint8_t *data, size_t size, const pmx_parser_callbacks_t *callbacks, void *userdata) {
  pmx_parse_state_t state;
  char text[128];
  pmx_state_init(&state, data, size, callbacks, userdata);
  int ret = setjmp(state.env);
  if (ret != 0) return ret;

  ret = pmx_parser_parse_header(&state);
  if (ret != 0) return ret;

  /* vertices */
  int_fast32_t count = parse_int(&state);
  ret = state.callbacks->vertex_cb(&state, count, state.userdata);
  if (ret != 0) return ret;

  ret = setjmp(state.env);
  if (ret != 0) return ret;

  /* triangles */
  count = parse_int(&state);
  if (count % 3 != 0) parse_error(&state, -1, "Index count %ld is not a multiple of 3", count);
  ret = state.callbacks->triangle_cb(&state, count / 3, state.userdata);
  if (ret != 0) return ret;

  ret = setjmp(state.env);
  if (ret != 0) return ret;

  /* textures */
  count = parse_int(&state);
  ret = state.callbacks->texture_cb(&state, count, state.userdata);
  if (ret != 0) return ret;

  ret = setjmp(state.env);
  if (ret != 0) return ret;

  /* materials */
  count = parse_int(&state);
  ret = state.callbacks->material_cb(&state, count, state.userdata);
  if (ret != 0) return ret;

  ret = setjmp(state.env);
  if (ret != 0) return ret;

  /* bones */
  count = parse_int(&state);
  ret = state.callbacks->bone_cb(&state, count, state.userdata);
  if (ret != 0) return ret;

  return 0;
}

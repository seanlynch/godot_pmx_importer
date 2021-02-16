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


typedef struct {
  char data[4096];
  size_t offset;
  size_t size;
  int fd;
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


static void pmx_state_init(pmx_parse_state_t *state, const pmx_parser_callbacks_t *callbacks, void *userdata) {
  bzero(state, sizeof *state);
  state->callbacks = callbacks;
  state->userdata = userdata;
}


static void fill_buffer(pmx_parse_state_t *state) {
  if (state->offset != state->size) {
    assert(state->offset < state->size);
    state->size -= state->offset;
    memmove(state->data, &state->data[state->offset], state->size);
  } else {
    state->size = 0;
  }
  state->offset = 0;
  ssize_t bytes_read = read(state->fd, &state->data[state->size], sizeof (state->data) - state->size);
  if (bytes_read <= 0) {
    parse_error(state, -1, "Failed to read any more bytes");
  }
  state->size += bytes_read;
  printf("Read %zd bytes\n", bytes_read);
  assert(state->size <= sizeof (state->data));
}


static void check_size(pmx_parse_state_t *state, size_t needed) {
  if (state->offset + needed > state->size) {
    fill_buffer(state);
  }
}


static char *parse_bytes(pmx_parse_state_t *state, size_t length) {
  check_size(state, length);
  char *ptr = &state->data[state->offset];
  state->offset += length;
  return ptr;
}


static void copy_bytes(pmx_parse_state_t *state, char *dest, size_t length) {
  char *src = parse_bytes(state, length);
  memcpy(dest, src, length);
}


typedef struct {
  float version;
  pmx_encoding_t text_encoding;
  int addl_vec4_count;
  int vertex_index_size;
  int texture_index_size;
  int material_index_size;
  int bone_index_size;
  int morph_index_size;
  int rigidbody_index_size;
} model_info_t;


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
PARSER(short, int16_t, int_fast16_t);
PARSER(ushort, uint16_t, uint_fast16_t);

RPARSER(vec2, float, float, 2);
RPARSER(vec3, float, float, 3);
RPARSER(vec4, float, float, 4);

IPARSER(bone_index, int_fast32_t, byte, short, int);


static size_t parse_text(pmx_parse_state_t *state, char *buf, size_t bufsize) {
  uint_fast32_t length = parse_int(state);
  printf("Text length: %lu\n", length);
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
  parse_text(&state, buf, sizeof(buf))


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


static void parse_vertices(pmx_parse_state_t *state) {
  int vertex_count = parse_int(state);
  pmx_vertex_t vertex;
  state->callbacks->pre_vertex_cb(&state->model, state->userdata);
  for (int i = 0; i < vertex_count; i++) {
    parse_vec3(state, vertex.position);
    parse_vec3(state, vertex.normal);
    parse_vec2(state, vertex.uv);
    parse_skip(state, state->model.addl_vec4_count * 16);
    vertex.deform_type = parse_byte(state);
    switch(vertex.deform_type) {
    case bdef1:
      vertex.deform.non_sdef.bones[0] = parse_bone_index(state);
      break;
    case bdef2:
      vertex.deform.non_sdef.bones[0] = parse_bone_index(state);
      vertex.deform.non_sdef.bones[1] = parse_bone_index(state);
      vertex.deform.non_sdef.weights[0] = parse_float(state);
      break;
    case bdef4:
    case qdef:
      vertex.deform.non_sdef.bones[0] = parse_bone_index(state);
      vertex.deform.non_sdef.bones[1] = parse_bone_index(state);
      vertex.deform.non_sdef.bones[2] = parse_bone_index(state);
      vertex.deform.non_sdef.bones[3] = parse_bone_index(state);
      vertex.deform.non_sdef.weights[0] = parse_float(state);
      vertex.deform.non_sdef.weights[1] = parse_float(state);
      vertex.deform.non_sdef.weights[2] = parse_float(state);
      vertex.deform.non_sdef.weights[3] = parse_float(state);
      break;
    case sdef:
      vertex.deform.sdef.bones[0] = parse_bone_index(state);
      vertex.deform.sdef.bones[1] = parse_bone_index(state);
      vertex.deform.sdef.weight = parse_float(state);
      parse_vec3(state, vertex.deform.sdef.c);
      parse_vec3(state, vertex.deform.sdef.r0);
      parse_vec3(state, vertex.deform.sdef.r1);
      break;
    }
    vertex.outline_scale = parse_float(state);
    state->callbacks->vertex_cb(&vertex, state->userdata);
  }
}


static void vertex_cb(pmx_vertex_t *vertex, void *userdata) {
  print_vertex(vertex);
}


int pmx_parser_parse(const char *filename, const pmx_parser_callbacks_t *callbacks, void *userdata) {
  pmx_parse_state_t state;
  char text[128];
  pmx_state_init(&state, callbacks, userdata);
  int ret = setjmp(state.env);
  if (ret != 0) {
    return ret;
  }

  state.fd = open(filename, 0, O_RDONLY);

  if (memcmp(parse_bytes(&state, 4), "PMX ", 4)) parse_error(&state, -1, "Bad signature");
  float version = parse_float(&state);
  printf("Got version %f\n", version);
  if (version != 2.0 && version != 2.1) parse_error(&state, -1, "Unsupported version %f", version);
  int global_count = parse_byte(&state);
  if (global_count != 8) parse_error(&state, -1, "Unexpected number of globals. Expected 8 got %d", global_count);

  state.model.text_encoding = parse_byte(&state);
  state.model.addl_vec4_count = parse_byte(&state);
  state.model.vertex_index_size = parse_byte(&state);
  state.model.texture_index_size = parse_byte(&state);
  state.model.material_index_size = parse_byte(&state);
  state.model.bone_index_size = parse_byte(&state);
  state.model.morph_index_size = parse_byte(&state);
  state.model.rigidbody_index_size = parse_byte(&state);

  PARSE_TEXT(state.model.model_name_local);
  PARSE_TEXT(state.model.model_name_universal);
  PARSE_TEXT(state.model.comment_local);
  PARSE_TEXT(state.model.comment_universal);
  parse_vertices(&state);
  return 0;
}

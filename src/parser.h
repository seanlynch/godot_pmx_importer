#pragma once
#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>

typedef enum { utf16le = 0, utf8 = 1 } pmx_encoding_t;

#define PMX_STRING_MAX 256

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
  char model_name_local[PMX_STRING_MAX];
  char model_name_universal[PMX_STRING_MAX];
  char comment_local[PMX_STRING_MAX];
  char comment_universal[PMX_STRING_MAX];
} pmx_model_info_t;

typedef enum {
  bdef1 = 0,
  bdef2 = 1,
  bdef4 = 2,
  sdef = 3,
  qdef = 4
} pmx_deform_type_t;

typedef struct {
  float position[3];
  float normal[3];
  float uv[2];
  float addl_vec4s[4][4];
  pmx_deform_type_t deform_type;
  union {
    struct {
      int_fast32_t bones[4];
      float weights[4];
    } non_sdef;
    struct {
      int_fast32_t bones[2];
      float weight;
      float c[3];
      float r0[3];
      float r1[3];
    } sdef;
  } deform;
  float outline_scale;
} pmx_vertex_t;

struct pmx_parse_state;

typedef struct {
  int (*model_info_cb)(pmx_model_info_t *model, void *userdata);
  int (*vertex_cb)(struct pmx_parse_state *parse_state, int_fast32_t vertex_count, void *userdata);
  int (*triangle_cb)(struct pmx_parse_state *parse_state, int_fast32_t triangle_count, void *userdata);
} pmx_parser_callbacks_t;

int pmx_parser_parse(const char *filename,
                     const pmx_parser_callbacks_t *callbacks, void *userdata);
int pmx_parser_next_vertex(struct pmx_parse_state *state, pmx_vertex_t *vertex);
int pmx_parser_next_triangle_uint8(struct pmx_parse_state *state, uint8_t *buf);
int pmx_parser_next_triangle_uint16(struct pmx_parse_state *state, uint16_t *buf);
int pmx_parser_next_triangle_int32(struct pmx_parse_state *state, int32_t *buf);

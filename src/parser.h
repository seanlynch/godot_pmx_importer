#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <setjmp.h>
#include <stdbool.h>
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
  int_fast32_t bones[4];
  float weights[4];
  float c[3];
  float r0[3];
  float r1[3];
  float outline_scale;
} pmx_vertex_t;

typedef struct {
  char name_local[PMX_STRING_MAX];
  char name_universal[PMX_STRING_MAX];
  float diffuse[4];
  float specular[3];
  float specularity;
  float ambient[3];
  uint8_t flags;
  float edge_color[4];
  float edge_scale;
  int_fast32_t texture;
  int_fast32_t environment;
  enum { disabled=0, multiplicative=1, additive=2, addl_vec4=3 } environment_blend_mode;
  enum { texture_ref=0, internal_ref=1 } toon_type;
  int_fast32_t toon;  /* Meaning depends on toon_type */
  char metadata[PMX_STRING_MAX];
  int_fast32_t index_count;  /* Number of indices (triangles * 3) to apply this material to */
} pmx_material_t;

#define PMX_BONE_FLAG_INDEXED_TAIL_POS 1
#define PMX_BONE_FLAG_ROTATABLE (1<<1)
#define PMX_BONE_FLAG_TRANSLATABLE (1<<2)
#define PMX_BONE_FLAG_VISIBLE (1<<3)
#define PMX_BONE_FLAG_ENABLED (1<<4)
#define PMX_BONE_FLAG_IK (1<<5)
#define PMX_BONE_FLAG_INHERIT_ROTATION (1<<8)
#define PMX_BONE_FLAG_INHERIT_TRANSLATION (1<<9)
#define PMX_BONE_FLAG_FIXED_AXIS (1<<10)
#define PMX_BONE_FLAG_LOCAL_COORD (1<<11)
#define PMX_BONE_FLAG_PHYSICS_AFTER_DEFORM (1<<12)
#define PMX_BONE_FLAG_EXTERNAL_PARENT_DEFORM (1<<13)

#define PMX_MAX_IK_LINKS 64

typedef struct {
  int_fast32_t bone;
  bool has_limits;
  float limit_min[3], limit_max[3];
} pmx_bone_ik_link_t;

typedef struct {
  char name_local[PMX_STRING_MAX];
  char name_universal[PMX_STRING_MAX];
  float position[3];
  int_fast32_t parent;
  int_fast32_t layer;
  int_fast32_t flags;
  float tail_position[3];
  int_fast32_t tail_bone;
  struct { int_fast32_t parent; float weight; } inherit;
  float fixed_axis[3];
  struct { float x_axis[3], z_axis[3]; } local_coord;
  int_fast32_t external_parent;
  struct {
    int_fast32_t target;
    int_fast32_t loop_count;
    float limit_radian;
    int link_count;
    pmx_bone_ik_link_t ik_links[PMX_MAX_IK_LINKS];
  } ik;
} pmx_bone_t;


struct pmx_parse_state;

typedef struct {
  int (*model_info_cb)(pmx_model_info_t *model, void *userdata);
  int (*vertex_cb)(struct pmx_parse_state *parse_state, int_fast32_t vertex_count, void *userdata);
  int (*triangle_cb)(struct pmx_parse_state *parse_state, int_fast32_t triangle_count, void *userdata);
  int (*texture_cb)(struct pmx_parse_state *parse_state, int_fast32_t texture_count, void *userdata);
  int (*material_cb)(struct pmx_parse_state *parse_state, int_fast32_t material_count, void *userdata);
  int (*bone_cb)(struct pmx_parse_state *parse_state, int_fast32_t bone_count, void *userdata);
} pmx_parser_callbacks_t;

const char *pmx_deform_type_string(pmx_deform_type_t t);

int pmx_parser_parse(const uint8_t *data, size_t size,
                     const pmx_parser_callbacks_t *callbacks, void *userdata);
int pmx_parser_next_vertex(struct pmx_parse_state *state, pmx_vertex_t *vertex);
int pmx_parser_next_triangle_uint8(struct pmx_parse_state *state, uint8_t *buf);
int pmx_parser_next_triangle_uint16(struct pmx_parse_state *state, uint16_t *buf);
int pmx_parser_next_triangle_int32(struct pmx_parse_state *state, int32_t *buf);
int pmx_parser_next_texture(struct pmx_parse_state *state, char *buf,
                            size_t bufsize);
int pmx_parser_next_material(struct pmx_parse_state *state, pmx_material_t *material);
int pmx_parser_next_bone(struct pmx_parse_state *state, pmx_bone_t *bone);

#ifdef __cplusplus
}
#endif

// #pragma once

// #include <stdbool.h>
// #include <stddef.h>
// #include <stdint.h>
// #include <exception>
// #include <variant>

// #include "core/string/ustring.h"

// namespace PMX {

// class ParseError : public std::exception {
// public:
// 	ParseError(const String m) :
// 			message(m) {}
// 	~ParseError() {}

// 	String message;
// };

// typedef float Vec2[2];
// typedef float Vec3[3];
// typedef float Vec4[4];

// enum class Encoding { UTF16LE = 0,
// 	UTF8 = 1 };

// enum class BoneFlags { IndexedTailPos = 0x1,
// 	Rotatable = 0x2,
// 	Translatable = 0x4,
// 	Visible = 0x8,
// 	Enabled = 0x10,
// 	IK = 0x20,
// 	InheritRotation = 0x100,
// 	InheritTranslation = 0x200,
// 	FixedAxis = 0x400,
// 	LocalCoord = 0x800,
// 	PhysicsAfterDeform = 0x1000,
// 	ExternalParentDeform = 0x2000 };

// struct ModelInfo {
// 	float version;
// 	Encoding encoding;
// 	int addl_vec4_count;
// 	int vertex_index_size;
// 	int texture_index_size;
// 	int material_index_size;
// 	int bone_index_size;
// 	int morph_index_size;
// 	int rigidbody_index_size;
// 	String model_name_local, model_name_universal, comment_local, comment_universal;
// };

// enum class DeformType {
// 	BDEF1 = 0,
// 	BDEF2 = 1,
// 	BDEF4 = 2,
// 	SDEF = 3,
// 	QDEF = 4
// };

// struct Vertex {
// 	Vec3 position, normal;
// 	;
// 	Vec2 uv;
// 	Vec4 addl_vec4s[4];
// 	DeformType deform_type;
// 	int_fast32_t bones[4];
// 	float weights[4];
// 	Vec3 c, r0, r1;
// 	float outline_scale;
// };

// struct Material {
// 	String name_local, name_universal;
// 	Vec4 diffuse[4];
// 	Vec3 specular;
// 	float specularity;
// 	Vec3 ambient;
// 	int_fast8_t flags;
// 	Vec4 edge_color;
// 	float edge_scale;
// 	int_fast32_t texture;
// 	int_fast32_t environment;
// 	enum { disabled = 0,
// 		multiplicative = 1,
// 		additive = 2,
// 		addl_vec4 = 3 } environment_blend_mode;
// 	enum { texture_ref = 0,
// 		internal_ref = 1 } toon_type;
// 	int_fast32_t toon; /* Meaning depends on toon_type */
// 	String metadata;
// 	int_fast32_t index_count; /* Number of indices (triangles * 3) to apply this material to */
// };

// struct IKLink {
// 	int_fast32_t bone;
// 	bool has_limits;
// 	Vec3 limit_min, limit_max;
// };

// struct IK {
// 	int_fast32_t target;
// 	int_fast32_t loop_count;
// 	float limit_radian;
// 	int link_count;
// };

// struct Bone {
// 	String name_local, name_universal;
// 	Vec3 position;
// 	int_fast32_t parent;
// 	int_fast32_t layer;
// 	BoneFlags flags;
// 	Vec3 tail_position;
// 	int_fast32_t tail_bone;
// 	struct {
// 		int_fast32_t parent;
// 		float weight;
// 	} inherit;
// 	Vec3 fixed_axis;
// 	struct {
// 		Vec3 x_axis, z_axis;
// 	} local_coord;
// 	int_fast32_t external_parent;
// 	IK ik;
// };

// enum class MorphType {
// 	group = 0,
// 	vertex = 1,
// 	bone = 2,
// 	uv = 3,
// 	uv_ext1 = 4,
// 	uv_ext2 = 5,
// 	uv_ext3 = 6,
// 	uv_ext4 = 7,
// 	material = 8,
// 	flip = 9,
// 	impulse = 10
// };

// struct GroupOffset {
// 	int_fast32_t morph;
// 	float weight;
// };

// struct VertexOffset {
// 	int_fast32_t vertex;
// 	float translation[3];
// };

// struct BoneOffset {
// 	int_fast32_t bone;
// 	float translation[3];
// 	float rotation[4];
// };

// struct UVOffset {
// 	int_fast32_t vertex;
// 	Vec4 data;
// };

// enum class BlendMode { multiplicative = 0,
// 	additive = 1 };

// struct MaterialOffset {
// 	int_fast32_t material;
// 	BlendMode blend_mode;
// 	Vec4 diffuse;
// 	Vec3 specular;
// 	float specularity;
// 	Vec3 ambient;
// 	Vec4 edge_color;
// 	float edge_size;
// 	Vec4 texture_tint;
// 	Vec4 environment_tint;
// 	Vec4 toon_tint;
// };

// struct FlipOffset {
// 	int_fast32_t morph;
// 	float weight;
// };

// struct ImpulseOffset {
// 	int_fast32_t rigidbody;
// 	int_fast32_t local_flag;
// 	Vec3 speed, torque;
// };

// typedef std::variant<GroupOffset, VertexOffset, BoneOffset, UVOffset, MaterialOffset, FlipOffset, ImpulseOffset> MorphOffset;

// struct Morph {
// 	String name_local, name_universal;
// 	int_fast32_t panel_type;
// 	MorphType morph_type;
// };

// class Visitor {
// public:
// 	virtual void modelInfo(ModelInfo &model) = 0;
// 	virtual void vertexCount(int_fast32_t count);
// 	virtual void processVertex(int_fast32_t i, const Vertex &vertex) = 0;
// 	virtual void triangleCount(int_fast32_t count);
// 	virtual void processTriangle(int_fast32_t i, int_fast32_t a, int_fast32_t b, int_fast32_t c) = 0;
// 	virtual void textureCount(int_fast32_t count);
// 	virtual void processTexture(int_fast32_t i, const String &texture);
// 	virtual void materialCount(int_fast32_t count);
// 	virtual void processMaterial(int_fast32_t i, const Material &material) = 0;
// 	virtual void boneCount(int_fast32_t count);
// 	virtual void processBone(int_fast32_t i, const Bone &bone) = 0;
// 	virtual void processIKLink(int_fast32_t i, const IKLink &link) = 0;
// 	virtual void morphCount(int_fast32_t count);
// 	virtual void processMorph(int_fast32_t i, const MorphOffset &offset);
// };

// void parse(const uint8_t *data, size_t size, Visitor &visitor);

// } // namespace PMX

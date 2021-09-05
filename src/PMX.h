#pragma once

#include "core/io/resource.h"
#include "core/templates/local_vector.h"
#include "core/templates/safe_refcount.h"

#include "parser.h"
class PMXModel : public Resource {
	GDCLASS(PMXModel, Resource);
};

typedef enum {
	bdef1 = 0,
	bdef2 = 1,
	bdef4 = 2,
	sdef = 3,
	qdef = 4
} PMXDeformType;

class PMXVertex {
public:
	float position[3];
	float normal[3];
	float uv[2];
	float addl_vec4s[4][4];
	PMXDeformType deform_type;
	union {
		struct {
			int32_t bones[4];
			float weights[4];
		} non_sdef;
		struct {
			int32_t bones[2];
			float weight;
			float c[3];
			float r0[3];
			float r1[3];
		} sdef;
	} deform;
	float outline_scale;
};

class PMXMaterial : public Resource {
	GDCLASS(PMXMaterial, Resource);
};

class PMXModelInfo : public Resource {
	GDCLASS(PMXModelInfo, Resource);

public:
	String model_name_local;
	String model_name_universal;
	String comment_local;
	String comment_universal;
};

class PMXState : public Resource {
	GDCLASS(PMXState, Resource);
};

class PMX : public Resource {
	GDCLASS(PMX, Resource);

private:
	String model_name_local;
	String model_name_universal;
	String comment_local;
	String comment_universal;
	Vector<PMXVertex> vertices;
	Vector<int32_t> sdef_vertices;
	Vector<Vector3> sdef_data;
	Vector<int32_t> indices;
	Vector<String> textures;
	Array materials;
	Vector<int32_t> bones;
    Vector<int32_t> bone_indices;
    Vector<float> bone_weights;
    Vector<int32_t> triangles;

	int parseBoneWeights(Ref<PMXVertex> vertex, uint32_t i);
	Array getSurfaceArrays(int32_t start, int32_t end);
	String pmx_deform_type_string(PMXDeformType t) {
		switch (t) {
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

protected:
	static void _bind_methods();

public:
	int modelInfoCB(Ref<PMXModelInfo> model);
	int vertexCB(Ref<PMXState> state, int32_t count);
	int triangleCB(Ref<PMXState> state, int32_t count);
	int textureCB(Ref<PMXState> state, int32_t count);
	int materialCB(Ref<PMXState> state, int32_t count);
	int boneCB(Ref<PMXState> state, int32_t count);
	int morphCB(Ref<PMXState> state, int32_t count);

	int parse(PackedByteArray data);
};
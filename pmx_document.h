#pragma once

#include "core/io/resource.h"
#include "core/templates/local_vector.h"
#include "core/templates/safe_refcount.h"

#include "mmd_pmx_state.h"

class PMXDocument : public Resource {
	// https://github.com/kaitai-io/kaitai_struct_formats/blob/3170d3e40fcf2acdfa93cb8a7bea553986c3c27c/3d/mmd_pmx.ksy
	GDCLASS(PMXDocument, Resource);

protected:
	static void _bind_methods();

public:
	Error parse(Ref<PMXMMDState> r_state, String p_path) { return OK; }
	Error serialize(Ref<PMXMMDState> state, Node *p_root, const String &p_path) {
		return OK;
	}
};
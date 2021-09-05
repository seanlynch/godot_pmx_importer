#include "core/variant/variant.h"

#include "pmx_document.h"

void PMXDocument::_bind_methods() {
	ClassDB::bind_method(D_METHOD("parse"), &PMXDocument::parse);
}
Error PMXDocument::serialize(Ref<PMXMMDState> state, Node *p_root, const String &p_path) {
	return OK;
}
Error PMXDocument::parse(Ref<PMXMMDState> r_state, String p_path) {
	return OK;
}

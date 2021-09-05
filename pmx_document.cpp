#include "core/variant/variant.h"

#include "pmx_document.h"

void PMXDocument::_bind_methods() {
	ClassDB::bind_method(D_METHOD("parse"), &PMXDocument::parse);
}

#include "register_types.h"

#include "core/class_db.h"
#include "lasso.h"
// #include "lasso_point.h"

void register_pmx_types() {
	ClassDB::register_class<LassoDB>();
	ClassDB::register_class<LassoPoint>();
}

void unregister_pmx_types() {
	// Nothing to do here in this example.
}
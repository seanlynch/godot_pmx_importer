#include "register_types.h"
#include "src/PMX.h"

#include "core/class_db.h"

void register_pmx_types() {
    ClassDB::register_class<PMX>();
}

void unregister_pmx_types() {
}
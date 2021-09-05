#include "register_types.h"
#include "pmx_document.h"

#include "core/class_db.h"

void register_pmx_types() {
    ClassDB::register_class<PMXDocument>();
}

void unregister_pmx_types() {
}
#pragma once

#include "core/io/resource.h"

class PMXMMDState : public Resource {
	GDCLASS(PMXMMDState, Resource);
	friend class PMXDocument;
	friend class PackedSceneMMDPMX;
};
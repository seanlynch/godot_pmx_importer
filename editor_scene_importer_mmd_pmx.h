/*************************************************************************/
/*  editor_scene_importer_gltf.h                                         */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef EDITOR_SCENE_IMPORTER_MMX_PMX_H
#define EDITOR_SCENE_IMPORTER_MMX_PMX_H

#include "editor/import/resource_importer_scene.h"
#include "scene/main/node.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/surface_tool.h"

#include "thirdparty/ksy/mmd_pmx.h"

class Animation;

#ifdef TOOLS_ENABLED
class EditorSceneImporterMMDPMX : public EditorSceneImporter {
	GDCLASS(EditorSceneImporterMMDPMX, EditorSceneImporter);

public:
	virtual uint32_t get_import_flags() const override;
	virtual void get_extensions(List<String> *r_extensions) const override;
	virtual Node *import_scene(const String &p_path, uint32_t p_flags,
			int p_bake_fps,
			List<String> *r_missing_deps = nullptr,
			Error *r_err = nullptr) override;
	virtual Ref<Animation> import_animation(const String &p_path,
			uint32_t p_flags, int p_bake_fps) override;
};
#endif

class PMXMMDState : public Resource {
	GDCLASS(PMXMMDState, Resource);
};

class PackedSceneMMDPMX : public PackedScene {
	GDCLASS(PackedSceneMMDPMX, PackedScene);

	const real_t mmd_unit_conversion = 0.079f;
	String pick_universal_or_common(std::string p_universal, std::string p_common);

protected:
	static void _bind_methods();

public:
	virtual Node *import_scene(const String &p_path, uint32_t p_flags,
			int p_bake_fps,
			List<String> *r_missing_deps,
			Error *r_err,
			Ref<PMXMMDState> r_state);
	virtual Node *import_mmd_pmx_scene(const String &p_path, uint32_t p_flags, float p_bake_fps, Ref<PMXMMDState> r_state = Ref<PMXMMDState>());
	virtual void pack_mmd_pmx(String p_path, int32_t p_flags = 0,
			real_t p_bake_fps = 1000.0f, Ref<PMXMMDState> r_state = Ref<PMXMMDState>());
};
#endif // EDITOR_SCENE_IMPORTER_MMX_PMX_H

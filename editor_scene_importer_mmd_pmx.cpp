/*************************************************************************/
/*  editor_scene_importer_mmd_pmx.cpp                                       */
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

#include "editor_scene_importer_mmd_pmx.h"

#include "thirdparty/ksy/mmd_pmx.h"

#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/physics_body_3d.h"
#include "scene/animation/animation_player.h"
#include "scene/resources/animation.h"
#include "scene/resources/surface_tool.h"

#include <fstream>
#include <string>

uint32_t EditorSceneImporterMMDPMX::get_import_flags() const {
	return ImportFlags::IMPORT_SCENE;
}

void EditorSceneImporterMMDPMX::get_extensions(List<String> *r_extensions) const {
	r_extensions->push_back("pmx");
}

Node *EditorSceneImporterMMDPMX::import_scene(const String &p_path,
		uint32_t p_flags, int p_bake_fps,
		List<String> *r_missing_deps,
		Error *r_err) {
	Ref<PackedSceneMMDPMX> importer;
	importer.instantiate();
	return importer->import_scene(p_path, p_flags, p_bake_fps, r_missing_deps, r_err, Ref<PMXMMDState>());
}

Ref<Animation> EditorSceneImporterMMDPMX::import_animation(const String &p_path,
		uint32_t p_flags,
		int p_bake_fps) {
	return Ref<Animation>();
}

void PackedSceneMMDPMX::_bind_methods() {
	ClassDB::bind_method(D_METHOD("pack_mmd_pmx", "path", "flags", "bake_fps", "state"),
			&PackedSceneMMDPMX::pack_mmd_pmx, DEFVAL(0), DEFVAL(1000.0f), DEFVAL(Ref<PMXMMDState>()));
	ClassDB::bind_method(D_METHOD("import_mmd_pmx_scene", "path", "flags", "bake_fps", "state"),
			&PackedSceneMMDPMX::import_mmd_pmx_scene, DEFVAL(0), DEFVAL(1000.0f), DEFVAL(Ref<PMXMMDState>()));
}
Node *PackedSceneMMDPMX::import_mmd_pmx_scene(const String &p_path, uint32_t p_flags, float p_bake_fps, Ref<PMXMMDState> r_state) {
	Error err = FAILED;
	List<String> deps;
	return import_scene(p_path, p_flags, p_bake_fps, &deps, &err, r_state);
}

Node *PackedSceneMMDPMX::import_scene(const String &p_path, uint32_t p_flags,
		int p_bake_fps,
		List<String> *r_missing_deps,
		Error *r_err,
		Ref<PMXMMDState> r_state) {
	if (r_state == Ref<PMXMMDState>()) {
		r_state.instantiate();
	}
	std::ifstream ifs(
			ProjectSettings::get_singleton()->globalize_path(p_path).utf8().get_data(), std::ifstream::binary);
	kaitai::kstream ks(&ifs);
	mmd_pmx_t pmx = mmd_pmx_t(&ks);
	Node3D *root = memnew(Node3D);

	std::vector<std::unique_ptr<mmd_pmx_t::bone_t> > *bones = pmx.bones();

	Skeleton3D *skeleton = memnew(Skeleton3D);

	for (int32_t bone_i = 0; bone_i < pmx.bone_count(); bone_i++) {
		std::string name = bones->at(bone_i)->english_name()->value();
		String bone_name;
		bone_name.parse_utf8(name.data());
		skeleton->add_bone(bone_name);
	}
	for (int32_t bone_i = 0; bone_i < pmx.bone_count(); bone_i++) {
		int32_t parent_index = bones->at(bone_i)->parent_index()->value();
		skeleton->set_bone_parent(bone_i, parent_index);
		Transform3D xform;
		real_t x = bones->at(bone_i)->position()->x();
		x *= mmd_unit_conversion;
		real_t y = bones->at(bone_i)->position()->y();
		y *= mmd_unit_conversion;
		real_t z = bones->at(bone_i)->position()->z();
		z *= mmd_unit_conversion;
		xform.origin = Vector3(x, y, z);
		skeleton->set_bone_rest(bone_i, xform);
	}
	root->add_child(skeleton);
	skeleton->set_owner(root);

	Ref<SurfaceTool> surface;
	surface.instantiate();
	surface->begin(Mesh::PRIMITIVE_TRIANGLES);
	MeshInstance3D *mesh_3d = memnew(MeshInstance3D);
	std::vector<std::unique_ptr<mmd_pmx_t::vertex_t> > *vertices = pmx.vertices();
	for (int32_t vertex_i = 0; vertex_i < pmx.vertex_count(); vertex_i++) {
		create_vertex(vertex_i, vertices, surface);
	}
	std::vector<std::unique_ptr<mmd_pmx_t::face_t> > *faces = pmx.faces();
	for (int32_t vertex_i = 0; vertex_i < pmx.face_vertex_count() / 3; vertex_i++) {
		int32_t index = faces->at(vertex_i)->indices()->at(0)->value();
		surface->add_index(index);
		index = faces->at(vertex_i)->indices()->at(2)->value();
		surface->add_index(index);
		index = faces->at(vertex_i)->indices()->at(1)->value();
		surface->add_index(index);
	}
	Ref<ArrayMesh> mesh = surface->commit();
	mesh_3d->set_mesh(mesh);
	std::string std_name = pmx.header()->english_model_name()->value();
	String model_name;
	model_name.parse_utf8(std_name.data());
	mesh_3d->set_name(model_name);
	skeleton->add_child(mesh_3d);
	mesh_3d->set_owner(root);

	std::vector<std::unique_ptr<mmd_pmx_t::rigid_body_t> > *rigid_bodies = pmx.rigid_bodies();
	for (int32_t rigid_bodies_i = 0; rigid_bodies_i < pmx.rigid_body_count(); rigid_bodies_i++) {
		RigidBody3D *rigid_3d = memnew(RigidBody3D);
		std::string std_name = rigid_bodies->at(rigid_bodies_i)->english_name()->value();
		String rigid_name;
		rigid_name.parse_utf8(std_name.data());
		rigid_3d->set_name(rigid_name);
		root->add_child(rigid_3d);
		rigid_3d->set_owner(root);
	}
	return root;
}

void PackedSceneMMDPMX::pack_mmd_pmx(String p_path, int32_t p_flags,
		real_t p_bake_fps, Ref<PMXMMDState> r_state) {
	Error err = FAILED;
	List<String> deps;
	Node *root = import_scene(p_path, p_flags, p_bake_fps, &deps, &err, r_state);
	ERR_FAIL_COND(err != OK);
	pack(root);
}

void PackedSceneMMDPMX::create_vertex(int32_t p_vertex, const std::vector<std::unique_ptr<mmd_pmx_t::vertex_t> > *p_vertices, Ref<SurfaceTool> p_surface) {
	real_t x = p_vertices->at(p_vertex)->normal()->x();
	real_t y = p_vertices->at(p_vertex)->normal()->y();
	real_t z = p_vertices->at(p_vertex)->normal()->z();
	Vector3 normal = Vector3(x, y, z);
	p_surface->set_normal(normal);
	x = p_vertices->at(p_vertex)->uv()->x();
	y = p_vertices->at(p_vertex)->uv()->y();
	Vector2 uv = Vector2(x, y);
	p_surface->set_uv(uv);
	x = p_vertices->at(p_vertex)->position()->x();
	x *= mmd_unit_conversion;
	y = p_vertices->at(p_vertex)->position()->y();
	y *= mmd_unit_conversion;
	z = p_vertices->at(p_vertex)->position()->z();
	z *= mmd_unit_conversion;
	Vector3 point = Vector3(x, y, z);
	p_surface->add_vertex(point);
}

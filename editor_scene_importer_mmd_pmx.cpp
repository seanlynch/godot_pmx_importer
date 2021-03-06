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

#include "editor/import/scene_importer_mesh_node_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/physics_body_3d.h"
#include "scene/animation/animation_player.h"
#include "scene/resources/animation.h"
#include "scene/resources/surface_tool.h"
#include <unistd.h>

#include <cstdint>
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

	std::vector<std::unique_ptr<mmd_pmx_t::material_t> > *materials = pmx.materials();
	struct MMDMaterialVertexCounts {
		uint32_t start = 0;
		uint32_t end = 0;
	};
	Vector<MMDMaterialVertexCounts> material_index_counts;
	material_index_counts.resize(pmx.material_count());
	for (uint32_t material_i = 0; material_i < pmx.material_count(); material_i++) {
		if (material_i != 0) {
			material_index_counts.write[material_i].start = material_index_counts[material_i - 1].end;
		} else {
			material_index_counts.write[material_i].start = 0;
		}
		uint32_t start = material_index_counts[material_i].start;
		uint32_t count = materials->at(material_i)->face_vertex_count();
		material_index_counts.write[material_i].end = start + count;
	}
	for (int32_t material_i = 0; material_i < material_index_counts.size(); material_i++) {
		Ref<SurfaceTool> surface;
		surface.instantiate();
		surface->begin(Mesh::PRIMITIVE_TRIANGLES);
		std::vector<std::unique_ptr<mmd_pmx_t::vertex_t> > *vertices = pmx.vertices();
		for (uint32_t vertex_i = 0; vertex_i < pmx.vertex_count(); vertex_i++) {
			real_t x = vertices->at(vertex_i)->normal()->x();
			real_t y = vertices->at(vertex_i)->normal()->y();
			real_t z = vertices->at(vertex_i)->normal()->z();
			Vector3 normal = Vector3(x, y, z);
			surface->set_normal(normal);
			x = vertices->at(vertex_i)->uv()->x();
			y = vertices->at(vertex_i)->uv()->y();
			z = 0.0f;
			Vector2 uv = Vector2(x, y);
			surface->set_uv(uv);
			x = vertices->at(vertex_i)->position()->x();
			x *= mmd_unit_conversion;
			y = vertices->at(vertex_i)->position()->y();
			y *= mmd_unit_conversion;
			z = vertices->at(vertex_i)->position()->z();
			z *= mmd_unit_conversion;
			Vector3 point = Vector3(x, y, z);
			PackedInt32Array bones;
			bones.push_back(0);
			bones.push_back(0);
			bones.push_back(0);
			bones.push_back(0);
			PackedFloat32Array weights;
			weights.push_back(0.0f);
			weights.push_back(0.0f);
			weights.push_back(0.0f);
			weights.push_back(0.0f);
			if (!vertices->at(vertex_i)->_is_null_skin_weights()) {
				mmd_pmx_t::bone_type_t bone_type = vertices->at(vertex_i)->type();
				switch (bone_type) {
					case mmd_pmx_t::BONE_TYPE_BDEF1: {
						mmd_pmx_t::bdef1_weights_t *pmx_weights = (mmd_pmx_t::bdef1_weights_t *)vertices->at(vertex_i)->skin_weights();
						int64_t bone_index = pmx_weights->bone_index()->value();
						switch (pmx_weights->bone_index()->size()) {
							case 1: {
								if (bone_index != UINT8_MAX) {
									bones.write[0] = bone_index;
									weights.write[0] = 1.0f;
								}
							} break;
							case 2: {
								if (bone_index != UINT16_MAX) {
									bones.write[0] = bone_index;
									weights.write[0] = 1.0f;
								}
							} break;
							case 4: {
								if (bone_index != UINT32_MAX) {
									bones.write[0] = bone_index;
									weights.write[0] = 1.0f;
								}
							} break;
							default:
								break;
								// nothing
						}
					} break;
					case mmd_pmx_t::BONE_TYPE_BDEF2: {
						mmd_pmx_t::bdef2_weights_t *pmx_weights = (mmd_pmx_t::bdef2_weights_t *)vertices->at(vertex_i)->skin_weights();
						for (int32_t count = 0; count < 2; count++) {
							int64_t bone_index = pmx_weights->bone_indices()->at(count)->value();
							switch (pmx_weights->bone_indices()->at(count)->size()) {
								case 1: {
									if (bone_index != UINT8_MAX) {
										bones.write[count] = bone_index;
										weights.write[count] = pmx_weights->weights()->at(count);
									}
								} break;
								case 2: {
									if (bone_index != UINT16_MAX) {
										bones.write[count] = bone_index;
										weights.write[count] = pmx_weights->weights()->at(count);
									}
								} break;
								case 4: {
									if (bone_index != UINT32_MAX) {
										bones.write[count] = bone_index;
										weights.write[count] = pmx_weights->weights()->at(count);
									}
								} break;
								default:
									break;
									// nothing
							}
						}
					} break;
					case mmd_pmx_t::BONE_TYPE_BDEF4: {
						mmd_pmx_t::bdef4_weights_t *pmx_weights = (mmd_pmx_t::bdef4_weights_t *)vertices->at(vertex_i)->skin_weights();
						for (int32_t count = 0; count < RS::ARRAY_WEIGHTS_SIZE; count++) {
							switch (pmx_weights->bone_indices()->at(count)->size()) {
								case 1: {
									uint8_t bone_index = pmx_weights->bone_indices()->at(count)->value();
									if (bone_index != UINT8_MAX) {
										bones.write[count] = bone_index;
										weights.write[count] = pmx_weights->weights()->at(count);
									}
								} break;
								case 2: {
									uint16_t bone_index = pmx_weights->bone_indices()->at(count)->value();
									if (bone_index != UINT16_MAX) {
										bones.write[count] = bone_index;
										weights.write[count] = pmx_weights->weights()->at(count);
									}
								} break;
								case 4: {
									uint32_t bone_index = pmx_weights->bone_indices()->at(count)->value();
									if (bone_index != UINT32_MAX) {
										bones.write[count] = bone_index;
										weights.write[count] = pmx_weights->weights()->at(count);
									}
								} break;
								default:
									break;
									// nothing
							}
						}
					} break;
					case mmd_pmx_t::BONE_TYPE_SDEF: {
					} break;
					case mmd_pmx_t::BONE_TYPE_QDEF: {
					} break;
					default:
						break;
						// nothing
				}
			}
			surface->set_bones(bones);
			real_t renorm = weights[0] + weights[1] + weights[2] + weights[3];
			if (renorm != 0.0 && renorm != 1.0) {
				weights.write[0] /= renorm;
				weights.write[1] /= renorm;
				weights.write[2] /= renorm;
				weights.write[3] /= renorm;
			}
			surface->set_weights(weights);
			surface->add_vertex(point);
		}
		std::vector<std::unique_ptr<mmd_pmx_t::face_t> > *faces = pmx.faces();
		for (uint32_t face_vertex_i = material_index_counts[material_i].start; face_vertex_i < material_index_counts[material_i].end;
				face_vertex_i += 3) {
			uint32_t face_i = face_vertex_i / 3;
			uint32_t index = faces->at(face_i)->indices()->at(0)->value();
			surface->add_index(index);
			index = faces->at(face_i)->indices()->at(2)->value();
			surface->add_index(index);
			index = faces->at(face_i)->indices()->at(1)->value();
			surface->add_index(index);
		}
		Array mesh_array = surface->commit_to_arrays();
		surface->clear();
		String material_name = pick_universal_or_common(materials->at(material_i)->english_name()->value(), materials->at(material_i)->name()->value());
		Ref<StandardMaterial3D> material;
		material.instantiate();
		int64_t texture_size = materials->at(material_i)->texture_index()->size();
		String texture_path;
		int64_t texture_index = materials->at(material_i)->texture_index()->value();
		switch (texture_size) {
			case 1: {
				if (texture_index != UINT8_MAX) {
					std::string raw_texture_path = pmx.textures()->at(texture_index)->name()->value();
					texture_path.parse_utf8(raw_texture_path.data());
				}
			} break;
			case 2: {
				if (texture_index != UINT16_MAX) {
					std::string raw_texture_path = pmx.textures()->at(texture_index)->name()->value();
					texture_path.parse_utf8(raw_texture_path.data());
				}
			} break;
			case 4: {
				if (texture_index != UINT32_MAX) {
					std::string raw_texture_path = pmx.textures()->at(texture_index)->name()->value();
					texture_path.parse_utf8(raw_texture_path.data());
				}
			} break;
			default:
				break;
		}
		if (!texture_path.is_empty()) {
			texture_path = p_path.get_base_dir().plus_file(texture_path);
			texture_path = texture_path.simplify_path();
			Ref<Texture> base_color_tex = ResourceLoader::load(texture_path);
			material->set_texture(StandardMaterial3D::TEXTURE_ALBEDO, base_color_tex);
		}
		mmd_pmx_t::color4_t *diffuse = materials->at(material_i)->diffuse();
		material->set_albedo(Color(diffuse->r(), diffuse->g(), diffuse->b(), diffuse->a()));
		EditorSceneImporterMeshNode3D *mesh_3d = memnew(EditorSceneImporterMeshNode3D);
		Ref<EditorSceneImporterMesh> mesh;
		mesh.instantiate();
		String model_name = pick_universal_or_common(pmx.header()->english_model_name()->value(), pmx.header()->model_name()->value());
		mesh_3d->set_name(material_name);
		mesh->add_surface(Mesh::PRIMITIVE_TRIANGLES, mesh_array, Array(), Dictionary(), material, material_name);
		root->add_child(mesh_3d);
		mesh_3d->set_mesh(mesh);
		mesh_3d->set_owner(root);
	}
	std::vector<std::unique_ptr<mmd_pmx_t::rigid_body_t> > *rigid_bodies = pmx.rigid_bodies();
	for (uint32_t rigid_bodies_i = 0; rigid_bodies_i < pmx.rigid_body_count(); rigid_bodies_i++) {
		RigidBody3D *rigid_3d = memnew(RigidBody3D);
		String rigid_name = pick_universal_or_common(rigid_bodies->at(rigid_bodies_i)->english_name()->value(),
				rigid_bodies->at(rigid_bodies_i)->name()->value());
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

String PackedSceneMMDPMX::pick_universal_or_common(std::string p_universal, std::string p_common) {
	String output_name;
	if (p_universal.empty()) {
		output_name.parse_utf8(p_common.data());
	} else {
		output_name.parse_utf8(p_universal.data());
	}
	return output_name;
}

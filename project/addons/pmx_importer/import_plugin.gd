tool
extends EditorImportPlugin

enum Presets { DEFAULT }

func get_importer_name():
	return "PMX"

func get_visible_name():
	return "PMX Scene"

func get_recognized_extensions():
	return ["pmx"]

func get_save_extension():
	return "tscn"

func get_resource_type():
	return "PackedScene"
	
func get_preset_count():
	return Presets.size()
	
func get_preset_name(preset):
	match preset:
		Presets.DEFAULT:
			return "Default"	
		_:
			return "Unknown"

func get_import_options(preset):
	match preset:
		_:
			return []
			
func make_material(m: Dictionary, textures: Array) -> SpatialMaterial:
	var material := SpatialMaterial.new()
	material.resource_name = m.name_universal if m.name_universal else m.name_local
	material.albedo_color = m.diffuse
	if m.texture >= 0:
		material.albedo_texture = textures[m.texture]
		
	return material
	
func load_textures(source_dir: String, textures: PoolStringArray) -> Array:
	var r := []
	for t in textures:
		var filename = "%s/%s" % [source_dir, t.replace("\\", "/")]
		print("Loading texture ", filename)
		var tex = load(filename)
		print("Got resource ", tex)
		r.push_back(tex)
		
	return r
	
func import(source_file: String, save_path: String, options, r_platform_variants, r_gen_files):
	var path_components := Array(source_file.split("/"))
	var source_dir := PoolStringArray(path_components.slice(0, path_components.size() - 2)).join("/")
	print("source_dir: ", source_dir)
	var file = File.new();
	var err = file.open(source_file, File.READ)
	if err != OK:
		return err
	var source_path: String = file.get_path_absolute()
	file.close()
	var pmx = load("res://PMX.gdns").new()
	var ret = pmx.parse(source_path)
	var scene := Node.new()
	scene.name = pmx.model_name_universal if pmx.model_name_universal else pmx.model_name_local
	var mesh := ArrayMesh.new()
	var arrays: Array = []
	arrays.resize(ArrayMesh.ARRAY_MAX)
	print('Got ', pmx.positions.size(), " positions")
	arrays[ArrayMesh.ARRAY_VERTEX] = pmx.positions
	arrays[ArrayMesh.ARRAY_NORMAL] = pmx.normals
	arrays[ArrayMesh.ARRAY_TEX_UV] = pmx.uvs
	var triangles: PoolIntArray = pmx.triangles
	var textures := load_textures(source_dir, pmx.textures)
	var start := 0
	for m in pmx.materials:
		var name = m.name_universal if m.name_universal else m.name_local
		print("Material: ", name)
		arrays[ArrayMesh.ARRAY_INDEX] = m.indices
		var surf_idx = mesh.get_surface_count() 
		mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
		mesh.surface_set_name(surf_idx, name)
		mesh.surface_set_material(surf_idx, make_material(m, textures))

	var mesh_instance := MeshInstance.new()
	mesh_instance.mesh = mesh
	scene.add_child(mesh_instance)
	mesh_instance.set_owner(scene)
	var packed_scene := PackedScene.new()
	packed_scene.pack(scene)
	ret = ResourceSaver.save("%s.%s" % [save_path, get_save_extension()], packed_scene)
	scene.free()
	return ret

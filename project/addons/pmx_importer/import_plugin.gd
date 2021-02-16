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
			
func create_mesh(pmx) -> ArrayMesh:
	# Initialize the ArrayMesh.
	var arr_mesh = ArrayMesh.new()
	var arrays = []
	arrays.resize(ArrayMesh.ARRAY_MAX)
	arrays[ArrayMesh.ARRAY_VERTEX] = pmx.get_positions()
	arrays[ArrayMesh.ARRAY_NORMAL] = pmx.get_normals()
	arrays[ArrayMesh.ARRAY_TEX_UV] = pmx.get_uvs()
	arrays[ArrayMesh.ARRAY_INDEX] = pmx.get_triangles()
	# Create the Mesh.
	print("Triangle count: ", pmx.get_triangles().size())
	print("Vertex count: ", pmx.get_positions().size())
	arr_mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
	return arr_mesh

func import(source_file, save_path, options, r_platform_variants, r_gen_files):
	var file = File.new();
	var err = file.open(source_file, File.READ)
	if err != OK:
		return err
	var source_path: String = file.get_path_absolute()
	file.close()
	var pmx = preload("res://pmx.gdns").new()
	var ret = pmx.parse(source_path)
	var scene := Node.new()
	var name_local = pmx.get_model_name_local()
	print("Local name: ", name_local)
	var name_universal = pmx.get_model_name_universal()
	print("Universal name: ", name_universal)
	var comment_local = pmx.get_comment_local()
	print("Local comment: ", comment_local)
	var comment_universal = pmx.get_comment_universal()
	print("Universal comment: ", comment_universal)
	var textures = pmx.get_textures()
	print("Textures: ", textures)
	scene.name = name_universal if name_universal else name_local
	var mesh := self.create_mesh(pmx)
	var mesh_instance := MeshInstance.new()
	mesh_instance.mesh = mesh
	scene.add_child(mesh_instance)
	mesh_instance.set_owner(scene)
	var packed_scene := PackedScene.new()
	packed_scene.pack(scene)
	ret = ResourceSaver.save("%s.%s" % [save_path, get_save_extension()], packed_scene)
	scene.free()
	return ret

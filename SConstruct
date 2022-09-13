#!/usr/bin/env python
import os
import sys

# This is the entry point for SCons to build this engine as a GDExtension.
# To build as a module, see `SCsub`.

LIB_NAME = "libvoxel"
BIN_FOLDER = "bin"

# TODO GDX: Share common stuff with `SCsub`

# Dependency on GodotCpp.
# Use the same cross-platform configurations.
# TODO GDX: Make sure this isn't doing too much?
# TODO GDX: Have GodotCpp in thirdparty/ or allow to specify a custom location
env = SConscript("D:/PROJETS/INFO/GODOT/Engine/godot_cpp_fork/SConstruct")

# TODO GDX: Adding our variables produces a warning when provided.
# "WARNING: Unknown SCons variables were passed and will be ignored"
# This is printed by GodotCpp's SConstruct file, which doesn't recognizes them.
# However they seem to be taken into account in our `SConstruct` file though.
# If such a check should exist, it needs to be HERE, not in GodotCpp.

env_vars = Variables()
env_vars.Add(BoolVariable("tools", "Build library with editor tools", True))
env_vars.Add(BoolVariable("voxel_tests", 
    "Build with tests for the voxel module, which will run on startup of the engine", False))
env_vars.Add(BoolVariable("voxel_fast_noise_2", "Build FastNoise2 support (x86-only)", True))
env_vars.Update(env)
Help(env_vars.GenerateHelpText(env))

env.Append(CPPPATH=["."])

env.Append(CPPDEFINES=[
	# See https://github.com/zeux/meshoptimizer/issues/311
	"MESHOPTIMIZER_ZYLANN_NEVER_COLLAPSE_BORDERS",
	# Because of the above, the MeshOptimizer library in this module is different to an official one.
	# Godot 4 includes an official version, which means they would both conflict at linking time.
	# To prevent this clash we wrap the entire library within an additional namespace.
	# This should be solved either by solving issue #311 or by porting the module to a dynamic library (GDExtension).
	"MESHOPTIMIZER_ZYLANN_WRAP_LIBRARY_IN_NAMESPACE",
	# Tell engine-agnostic code we are using Godot Engine as an extension
	"ZN_GODOT_EXTENSION"
])

if env["tools"]:
    # GodotCpp does not define anything regarding editor builds, so we have to define that ourselves.
    # This is the same symbol used in Godot modules.
    env.Append(CPPDEFINES=["TOOLS_ENABLED"])

sources = [
    "constants/voxel_string_names.cpp",
    "constants/cube_tables.cpp",

    "edition/voxel_tool.cpp",
    "edition/voxel_tool_buffer.cpp",

    "storage/voxel_buffer_internal.cpp",
    "storage/voxel_buffer_gd.cpp",
    "storage/voxel_metadata.cpp",
    "storage/voxel_metadata_variant.cpp",
    "storage/voxel_memory_pool.cpp",
    "storage/voxel_data.cpp",
    "storage/voxel_data_map.cpp",
    "storage/voxel_data_block.cpp",
    "storage/funcs.cpp",
    "storage/modifiers.cpp",

    "generators/voxel_generator.cpp",
    "generators/voxel_generator_script.cpp",
    "generators/simple/voxel_generator_flat.cpp",
    "generators/simple/voxel_generator_heightmap.cpp",
    "generators/simple/voxel_generator_waves.cpp",
    "generators/simple/voxel_generator_image.cpp",
    "generators/simple/voxel_generator_noise_2d.cpp",
    "generators/simple/voxel_generator_noise.cpp",

    "generators/graph/code_gen_helper.cpp",
    "generators/graph/fast_noise_lite_gdshader.cpp",
    "generators/graph/image_range_grid.cpp",
    "generators/graph/program_graph.cpp",
    "generators/graph/range_utility.cpp",
    "generators/graph/voxel_generator_graph.cpp",
    "generators/graph/voxel_graph_compiler.cpp",
    "generators/graph/voxel_graph_node_db.cpp",
    "generators/graph/voxel_graph_runtime.cpp",
    "generators/graph/voxel_graph_shader_generator.cpp",

    "streams/instance_data.cpp",
    "streams/voxel_stream.cpp",

    "meshers/voxel_mesher.cpp",
    "meshers/transvoxel/voxel_mesher_transvoxel.cpp",
    "meshers/transvoxel/transvoxel.cpp",
    "meshers/transvoxel/transvoxel_tables.cpp",
    "meshers/transvoxel/transvoxel_shader_minimal.cpp",
    "meshers/blocky/voxel_mesher_blocky.cpp",
    "meshers/blocky/voxel_blocky_library.cpp",
    "meshers/blocky/voxel_blocky_model.cpp",
    "meshers/cubes/voxel_mesher_cubes.cpp",
    "meshers/cubes/voxel_color_palette.cpp",

    "engine/voxel_engine.cpp",
    "engine/voxel_engine_gd.cpp",
    "engine/distance_normalmaps.cpp",
    "engine/generate_block_task.cpp",
    "engine/load_block_data_task.cpp",
    "engine/save_block_data_task.cpp",
    "engine/priority_dependency.cpp",

    "terrain/voxel_mesh_block.cpp",

    # Utilities

    "util/dstack.cpp",
    "util/expression_parser.cpp",
    "util/log.cpp",

    "util/godot/funcs.cpp",
    "util/godot/variant.cpp",
    "util/godot/object.cpp",
    "util/godot/string.cpp",
    "util/godot/mesh.cpp",
    "util/godot/concave_polygon_shape_3d.cpp",
    "util/godot/direct_mesh_instance.cpp",
    "util/godot/direct_static_body.cpp",
    "util/godot/project_settings.cpp",
    "util/godot/geometry_2d.cpp",

    "util/noise/fast_noise_lite/fast_noise_lite_gradient.cpp",
    "util/noise/fast_noise_lite/fast_noise_lite_range.cpp",
    "util/noise/fast_noise_lite/fast_noise_lite.cpp",
    "util/noise/gd_noise_range.cpp",

    "util/thread/thread.cpp",
    "util/thread/godot_thread_helper.cpp",

    "util/tasks/godot/threaded_task_gd.cpp",
    "util/tasks/async_dependency_tracker.cpp",
    "util/tasks/progressive_task_runner.cpp",
    "util/tasks/threaded_task_runner.cpp",
    "util/tasks/time_spread_task_runner.cpp",

    "util/math/box3i.cpp",
    "util/math/sdf.cpp",
    "util/math/vector3.cpp",
    "util/math/vector3f.cpp",
    "util/math/vector3i.cpp",

    # Entry point
    
    "register_types_gdx.cpp",

    # Thirdparty

    "thirdparty/meshoptimizer/allocator.cpp",
    "thirdparty/meshoptimizer/clusterizer.cpp",
    "thirdparty/meshoptimizer/indexcodec.cpp",
    "thirdparty/meshoptimizer/indexgenerator.cpp",
    "thirdparty/meshoptimizer/overdrawanalyzer.cpp",
    "thirdparty/meshoptimizer/overdrawoptimizer.cpp",
    "thirdparty/meshoptimizer/simplifier.cpp",
    "thirdparty/meshoptimizer/spatialorder.cpp",
    "thirdparty/meshoptimizer/stripifier.cpp",
    "thirdparty/meshoptimizer/vcacheanalyzer.cpp",
    "thirdparty/meshoptimizer/vcacheoptimizer.cpp",
    "thirdparty/meshoptimizer/vertexcodec.cpp",
    "thirdparty/meshoptimizer/vertexfilter.cpp",
    "thirdparty/meshoptimizer/vfetchanalyzer.cpp",
    "thirdparty/meshoptimizer/vfetchoptimizer.cpp",
]

# if env["tools"]:
#     sources += [
#     ]

# WARNING
# From a C++ developer point of view, the GodotCpp example and `.gdextension` files are confusing what the
# `debug` and `release` targets usually mean with "editor" and "exported projects".
# In this project, we consider `debug` means "unoptimized + debug symbols, big binary",
# while `release` means "optimized, no debug symbols, small binary".
# "editor" and "exported" should be separate concepts, which we had to define ourselves as `tools`.
# But in `.gdextension` files, `debug` actually means "editor", and `release` means "exported".

bin_name = "{}.{}{}{}".format(
    LIB_NAME,
    env["platform"], 
    ".tools." if env["tools"] else ".",
    env["target"],
)

if env["platform"] == "macos":
    library = env.SharedLibrary(
        "{}/{}.framework/{}".format(
            BIN_FOLDER,
            bin_name,
            bin_name
        ),
        source = sources,
    )
else:
    library = env.SharedLibrary(
        "{}/{}.{}{}".format(
            BIN_FOLDER,
            bin_name,
            env["arch_suffix"],
            env["SHLIBSUFFIX"]
        ),
        source = sources,
    )

Default(library)

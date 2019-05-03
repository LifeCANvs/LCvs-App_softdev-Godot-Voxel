#ifndef VOXEL_LOD_TERRAIN_HPP
#define VOXEL_LOD_TERRAIN_HPP

#include "../providers/voxel_provider.h"
#include "lod_octree.h"
#include "voxel_map.h"
#include "voxel_mesh_updater.h"
#include <core/set.h>
#include <scene/3d/spatial.h>

class VoxelMap;
class VoxelProviderThread;

// Paged terrain made of voxel blocks of variable level of detail.
// Designed for highest view distances, preferably using smooth voxels.
// Voxels are polygonized around the viewer by distance in a very large sphere, usually extending beyond far clip.
// Data is streamed using a VoxelProvider, which must support LOD.
class VoxelLodTerrain : public Spatial {
	GDCLASS(VoxelLodTerrain, Spatial)
public:
	static const int MAX_LOD = 32;

	enum BlockState {
		BLOCK_NONE, // There is no block
		BLOCK_LOAD, // The block is loading
		BLOCK_UPDATE_NOT_SENT, // The block needs an update but wasn't sent yet
		BLOCK_UPDATE_SENT, // The block needs an update which was sent
		BLOCK_IDLE // The block is up to date
	};

	VoxelLodTerrain();
	~VoxelLodTerrain();

	Ref<Material> get_material() const;
	void set_material(Ref<Material> p_material);

	Ref<VoxelProvider> get_provider() const;
	void set_provider(Ref<VoxelProvider> p_provider);

	int get_view_distance() const;
	void set_view_distance(int p_distance_in_voxels);

	void set_lod_split_scale(float p_lod_split_scale);
	float get_lod_split_scale() const;

	void set_lod_count(unsigned int p_lod_count);
	int get_lod_count() const;

	void set_viewer_path(NodePath path);
	NodePath get_viewer_path() const;

	BlockState get_block_state(Vector3 bpos, unsigned int lod_index) const;
	bool is_block_meshed(Vector3 bpos, unsigned int lod_index) const;
	bool is_block_shown(Vector3 bpos, unsigned int lod_index) const;

protected:
	static void _bind_methods();

	void _notification(int p_what);
	void _process();

private:
	int get_block_size() const;
	int get_block_size_pow2() const;
	void make_all_view_dirty_deferred();
	Spatial *get_viewer() const;
	void immerge_block(Vector3i block_pos, unsigned int lod_index);
	void reset_updater();
	Vector3 get_viewer_pos() const;
	void make_block_dirty(Vector3i bpos, unsigned int lod_index);

	void debug_print_lods();

	template <typename A>
	void for_all_blocks(A &action) {
		for (int lod_index = 0; lod_index < MAX_LOD; ++lod_index) {
			if (_lods[lod_index].map.is_valid()) {
				_lods[lod_index].map->for_all_blocks(action);
			}
		}
	}

	// TODO Dare having a grid of octrees for infinite world?
	// This octree doesn't hold any data... hence bool.
	LodOctree<bool> _lod_octree;

	NodePath _viewer_path;

	Ref<VoxelProvider> _provider;
	VoxelProviderThread *_provider_thread = nullptr;
	VoxelMeshUpdater *_block_updater = nullptr;
	std::vector<VoxelMeshUpdater::OutputBlock> _blocks_pending_main_thread_update;

	Ref<Material> _material;

	// Each LOD works in a set of coordinates spanning 2x more voxels the higher their index is
	struct Lod {
		Ref<VoxelMap> map;

		Map<Vector3i, BlockState> block_states;
		std::vector<Vector3i> blocks_pending_update;

		// Reflects LodOctree but only in this LOD
		// TODO Would be nice to use LodOctree directly!
		Set<Vector3i> blocks_in_meshing_area;

		// These are relative to this LOD, in block coordinates
		Vector3i last_viewer_block_pos;
		int last_view_distance_blocks = 0;

		// Members for memory caching
		std::vector<Vector3i> blocks_to_load;
		//std::vector<Vector3i> blocks_to_update;
	};

	Lod _lods[MAX_LOD];
};

VARIANT_ENUM_CAST(VoxelLodTerrain::BlockState)

#endif // VOXEL_LOD_TERRAIN_HPP

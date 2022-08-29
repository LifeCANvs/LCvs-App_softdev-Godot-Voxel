#ifndef VOXEL_DATA_H
#define VOXEL_DATA_H

#include "../generators/voxel_generator.h"
#include "../streams/voxel_stream.h"
#include "modifiers.h"
#include "voxel_data_map.h"

namespace zylann::voxel {

class VoxelDataGrid;

// Generic storage containing everything needed to access voxel data.
// Contains edits, procedural sources and file stream so voxels not physically stored in memory can be obtained.
// This does not contain meshing or instancing information, only voxels.
// Individual calls should be thread-safe.
class VoxelData {
public:
	VoxelData();
	~VoxelData();

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Configuration.
	// Changing these settings while data is already loaded can be expensive, or cause data to be reset.
	// If threaded tasks are still working on the data while this happens, they should be cancelled or ignored.

	inline unsigned int get_block_size() const {
		return _lods[0].map.get_block_size();
	}

	inline unsigned int get_block_size_po2() const {
		return _lods[0].map.get_block_size_pow2();
	}

	inline Vector3i voxel_to_block(Vector3i pos) const {
		return _lods[0].map.voxel_to_block(pos);
	}

	void set_lod_count(unsigned int p_lod_count);

	// Clears voxel data. Keeps modifiers, generator and settings.
	void reset_maps();

	inline unsigned int get_lod_count() const {
		MutexLock rlock(_settings_mutex);
		return _lod_count;
	}

	void set_bounds(Box3i bounds);

	inline Box3i get_bounds() const {
		MutexLock rlock(_settings_mutex);
		return _bounds_in_voxels;
	}

	void set_generator(Ref<VoxelGenerator> generator);

	inline Ref<VoxelGenerator> get_generator() const {
		MutexLock rlock(_settings_mutex);
		return _generator;
	}

	void set_stream(Ref<VoxelStream> stream);

	inline Ref<VoxelStream> get_stream() const {
		MutexLock rlock(_settings_mutex);
		return _stream;
	}

	inline VoxelModifierStack &get_modifiers() {
		return _modifiers;
	}

	inline const VoxelModifierStack &get_modifiers() const {
		return _modifiers;
	}

	void set_streaming_enabled(bool enabled);

	inline bool is_streaming_enabled() const {
		return _streaming_enabled;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Queries.
	// When not specified, the used LOD index is 0.

	VoxelSingleValue get_voxel(Vector3i pos, unsigned int channel_index, VoxelSingleValue defval) const;
	bool try_set_voxel(uint64_t value, Vector3i pos, unsigned int channel_index);

	float get_voxel_f(Vector3i pos, unsigned int channel_index) const;
	bool try_set_voxel_f(real_t value, Vector3i pos, unsigned int channel_index);

	// Copies voxel data in a box from LOD0.
	// `channels_mask` bits tell which channel is read.
	void copy(Vector3i min_pos, VoxelBufferInternal &dst_buffer, unsigned int channels_mask) const;

	// Pastes voxel data in a box at LOD0.
	// `channels_mask` bits tell which channel is pasted.
	// If `use_mask` is used, will only write voxels of the source buffer that are not equal to `mask_value`.
	// If `create_new_blocks` is true, blocks will be created if not found in the area.
	void paste(Vector3i min_pos, const VoxelBufferInternal &src_buffer, unsigned int channels_mask, bool use_mask,
			uint64_t mask_value, bool create_new_blocks);

	bool is_area_loaded(const Box3i p_voxels_box) const;

	// Executes a read+write operation on all voxels in the given area, on a specific channel.
	// If the area intersects the boundaries of the volume, it will be clipped.
	// If the area intersects blocks that aren't loaded, the operation will be cancelled.
	// Returns the box of voxels which were effectively processed.
	template <typename F>
	Box3i write_box(const Box3i &p_voxel_box, unsigned int channel_index, F action) {
		const Box3i voxel_box = p_voxel_box.clipped(get_bounds());
		if (!is_area_loaded(voxel_box)) {
			ZN_PRINT_VERBOSE("Area not editable");
			return Box3i();
		}
		Ref<VoxelGenerator> generator = _generator;
		VoxelDataLodMap::Lod &data_lod0 = _data->lods[0];
		{
			RWLockWrite wlock(data_lod0.map_lock);
			data_lod0.map.write_box(
					voxel_box, channel_index, action, [&generator](VoxelBufferInternal &voxels, Vector3i pos) {
						if (generator.is_valid()) {
							VoxelGenerator::VoxelQueryData q{ voxels, pos, 0 };
							generator->generate_block(q);
						}
					});
		}
		return voxel_box;
	}

	// Executes a read+write operation on all voxels in the given area, on two specific channels.
	// If the area intersects the boundaries of the volume, it will be clipped.
	// If the area intersects blocks that aren't loaded, the operation will be cancelled.
	// Returns the box of voxels which were effectively processed.
	template <typename F>
	Box3i write_box_2(const Box3i &p_voxel_box, unsigned int channel1_index, unsigned int channel2_index, F action) {
		const Box3i voxel_box = p_voxel_box.clipped(get_bounds());
		if (!is_area_loaded(voxel_box)) {
			ZN_PRINT_VERBOSE("Area not editable");
			return Box3i();
		}
		Ref<VoxelGenerator> generator = _generator;
		VoxelDataLodMap::Lod &data_lod0 = _data->lods[0];
		{
			RWLockWrite wlock(data_lod0.map_lock);
			data_lod0.map.write_box_2(voxel_box, channel1_index, channel2_index, action,
					[&generator](VoxelBufferInternal &voxels, Vector3i pos) {
						if (generator.is_valid()) {
							VoxelGenerator::VoxelQueryData q{ voxels, pos, 0 };
							generator->generate_block(q);
						}
					});
		}
		return voxel_box;
	}

	// Generates all non-present blocks in preparation for an edit.
	// Every block intersecting with the box at every LOD will be checked.
	// This function runs sequentially and should be thread-safe. May be used if blocks are immediately needed.
	// It will block if other threads are accessing the same data.
	// WARNING: this does not check if the area is editable.
	void pre_generate_box(Box3i voxel_box);

	// Clears voxel data from blocks that are pure results of generators and modifiers.
	// WARNING: this does not check if the area is editable.
	void clear_cached_blocks_in_voxel_area(Box3i p_voxel_box);

	// Flags all blocks in the given area as modified at LOD0.
	// Also marks them as requiring LOD updates (if lod count is 1 this has no effect).
	// Optionally, returns a list of affected block positions which did not require LOD updates before.
	void mark_area_modified(Box3i p_voxel_box, std::vector<Vector3i> *lod0_new_blocks_to_lod);

	// Sets voxel data at a block position. Also sets wether this is edited data (otherwise it is cached generator
	// results).
	// If the block has different size than expected, returns false and doesn't set the data.
	// If the block already exists, it will not be overwritten, but still returns true.
	// Otherwise, returns true.
	// TODO Might need to expose a parameter for the overwriting behavior.
	bool try_set_block_buffer(
			Vector3i block_position, unsigned int lod_index, std::shared_ptr<VoxelBufferInternal> buffer, bool edited);

	// Sets empty voxel data at a block position. It means this block is known to have no edits and no cached generator
	// data.
	// If the block already exists, it is not overwritten.
	// TODO Might need to expose a parameter for the overwriting behavior.
	void set_empty_block_buffer(Vector3i block_position, unsigned int lod_index);

	// void op(Vector3i bpos, VoxelDataBlock &block)
	template <typename F>
	void for_each_block(F op) {
		const unsigned int lod_count = get_lod_count();
		for (unsigned int lod_index = 0; lod_index < lod_count; ++lod_index) {
			Lod &lod = _lods[lod_index];
			RWLockRead rlock(lod.map_lock);
			lod.map.for_each_block(op);
		}
	}

	// void op(Vector3i bpos, VoxelDataBlock &block)
	template <typename F>
	void for_each_block_at_lod(F op, unsigned int lod_index) const {
		const Lod &lod = _lods[lod_index];
		RWLockRead rlock(lod.map_lock);
		lod.map.for_each_block(op);
	}

	// Tests if a block exists at the specified block position and LOD index.
	// This is mainly used for debugging so it isn't optimal, don't use this if you plan to query many blocks.
	bool has_block(Vector3i bpos, unsigned int lod_index) const;

	// Gets the total amount of allocated blocks. This includes blocks having no voxel data.
	unsigned int get_block_count() const;

	struct BlockLocation {
		Vector3i position;
		uint32_t lod_index;
	};

	// Updates the LODs of all blocks at given positions, and resets their flags telling that they need LOD updates.
	// Optionally, returns a list of affected block positions.
	void update_lods(Span<const Vector3i> modified_lod0_blocks, std::vector<BlockLocation> *out_updated_blocks);

	// void action(VoxelDataBlock &block, Vector3i bpos)
	template <typename F>
	void unload_blocks(Box3i bbox, unsigned int lod_index, F action) {
		Lod &lod = _lods[lod_index];
		RWLockWrite wlock(lod.map_lock);
		bbox.for_each_cell_zxy([&lod, &action](Vector3i bpos) {
			lod.map.remove_block(bpos, [&action, bpos](VoxelDataBlock &block) { action(block, bpos); });
		});
	}

	// Gets missing blocks out of the given block positions.
	// WARNING: positions outside bounds will be considered missing too.
	// TODO Don't consider positions outside bounds to be missing? This is only a byproduct of migrating old code.
	// It doesnt check this because the code using this function already does it (a bit more efficiently, but still).
	void get_missing_blocks(
			Span<const Vector3i> block_positions, unsigned int lod_index, std::vector<Vector3i> &out_missing) const;

	// Gets missing blocks out of the given area in block coordinates.
	// If the area intersects the outside of the bounds, it will be clipped.
	void get_missing_blocks(Box3i p_blocks_box, unsigned int lod_index, std::vector<Vector3i> &out_missing) const;

	unsigned int get_blocks_with_voxel_data(
			Box3i p_blocks_box, unsigned int lod_index, Span<std::shared_ptr<VoxelBufferInternal>> out_blocks) const;

	void get_blocks_grid(VoxelDataGrid &grid, Box3i box_in_voxels, unsigned int lod_index) const;

private:
	void reset_maps_no_settings_lock();

	struct Lod {
		// Storage for edited and cached voxels.
		VoxelDataMap map;
		// This lock should be locked in write mode only when the map gets modified (adding or removing blocks).
		// Otherwise it may be locked in read mode.
		// It is possible to unlock it after we are done querying the map.
		RWLock map_lock;
	};

	static void pre_generate_box(Box3i voxel_box, Span<Lod> lods, unsigned int data_block_size, bool streaming,
			unsigned int lod_count, Ref<VoxelGenerator> generator, VoxelModifierStack &modifiers);

	static inline std::shared_ptr<VoxelBufferInternal> try_get_voxel_buffer_with_lock(
			const Lod &data_lod, Vector3i block_pos, bool &out_generate) {
		RWLockRead rlock(data_lod.map_lock);
		const VoxelDataBlock *block = data_lod.map.get_block(block_pos);
		if (block == nullptr) {
			return nullptr;
		}
		// TODO Thread-safety: this checking presence of voxels is not safe.
		// It can change while meshing takes place if a modifier is moved in the same area,
		// because it invalidates cached data (that doesn't require locking the map, and doesn't lock a VoxelBuffer,
		// so there is no sync going on). One way to fix this is to implement a spatial lock.
		if (!block->has_voxels()) {
			out_generate = true;
			return nullptr;
		}
		return block->get_voxels_shared();
	}

	// Each LOD works in a set of coordinates spanning 2x more voxels the higher their index is.
	// LOD 0 is the primary storage for edited data. Higher indices are "mip-maps".
	// A fixed array is used because max lod count is small, and it doesn't require locking by threads.
	// Note that these LODs do not automatically update, it is up to users of the class to trigger it.
	//
	// TODO Optimize: (low priority) this takes more than 5Kb in the object, even when not using LODs.
	// Each LOD contains an RWLock, which is 242 bytes, so *24 it adds up quickly.
	// A solution would be to allocate LODs dynamically in the constructor (the potential presence of LODs doesnt need
	// to change after being constructed, there is no use case for that so far)
	FixedArray<Lod, constants::MAX_LOD> _lods;

	Box3i _bounds_in_voxels;

	uint8_t _lod_count = 1;

	// If enabled, some data blocks can have the "not loaded" and "loaded" status. Which means we can't assume what they
	// contain, until we load them from the stream.
	// If disabled, all edits are loaded in memory, and we know if a block isn't stored, it means we can use the
	// generator and modifiers to obtain its data.
	// This mostly changes how this class is used, streaming itself is not directly implemented in this class.
	bool _streaming_enabled = true;

	// Procedural generation stack
	VoxelModifierStack _modifiers;
	Ref<VoxelGenerator> _generator;

	// Persistent storage (file(s)).
	Ref<VoxelStream> _stream;

	// This should be locked when accessing settings members.
	// If other locks are needed simultaneously such as voxel maps, they should always be locked AFTER, to prevent
	// deadlocks.
	//
	// It is not a RWLock because it may be locked for VERY short periods of time (just reading small values).
	// In comparison, RWLock uses a `shared_timed_mutex` under the hood, and locking that for reading locks a
	// mutex internally either way.
	// There are times where locking can take longer, but it only happens rarely, when changing LOD count for example.
	Mutex _settings_mutex;
};

} // namespace zylann::voxel

#endif // VOXEL_DATA_H

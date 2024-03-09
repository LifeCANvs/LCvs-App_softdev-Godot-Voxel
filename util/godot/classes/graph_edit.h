#ifndef ZN_GODOT_GRAPH_EDIT_H
#define ZN_GODOT_GRAPH_EDIT_H

#if defined(ZN_GODOT)
#include <scene/gui/graph_edit.h>
#elif defined(ZN_GODOT_EXTENSION)
#include <godot_cpp/classes/graph_edit.hpp>
using namespace godot;
#endif

#include "../../containers/std_vector.h"
#include "../core/string_name.h"
#include "../core/version.h"

namespace zylann::godot {

struct GraphEditConnection {
	StringName from;
	StringName to;
	int from_port = 0;
	int to_port = 0;
	// float activity = 0.0;
};

void get_graph_edit_connections(const GraphEdit &self, StdVector<GraphEditConnection> &out_connections);
Vector2 get_graph_edit_scroll_offset(const GraphEdit &self);
bool is_graph_edit_using_snapping(const GraphEdit &self);
int get_graph_edit_snapping_distance(const GraphEdit &self);

} // namespace zylann::godot

#endif // ZN_GODOT_GRAPH_EDIT_H

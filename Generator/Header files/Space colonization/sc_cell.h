/// Cell of space colonization algorithm grid.
/// \file sc_cell.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#ifndef SC_CELL_H
#define SC_CELL_H

#include <Space colonization/sc_free_space_marker.h>
#include <Tree primitives/tree_primitives.h>
#include <Utilities/glm_utils.h> // Hash

#include <vector>
#include <mutex>

namespace Treegen
{
	namespace SpaceCol
	{
		struct ScCell;
		using rwScCell = std::reference_wrapper<ScCell>;
		using rwConstScCell = std::reference_wrapper<const ScCell>;

		/// Cell of space colonization algorithm grid.
		struct ScCell {
			ScCell() = default;

			/// Constructs new ScCell of space colonization grid.
			/// \param gridPosition Position of cell in grid.
			explicit ScCell(const glm::ivec3& gridPosition) : gridPosition{gridPosition} {}

			/// Constructs new ScCell of space colonization grid.
			/// \param gridX X position in grid.
			/// \param gridY Y position in grid.
			/// \param gridZ Z position in grid.
			explicit ScCell(const int gridX, const int gridY, const int gridZ) : gridPosition{gridX, gridY, gridZ} {}

			/// Clears all data saved in cell.
			void clear() {
				spaceMarkers.clear();
				treeNodes.clear();
				treeBuds.clear();
			}

			glm::ivec3 gridPosition;

			/// \name Data of cell and mutexes used when synchronization of access to data is necessary.
			///@{
			std::mutex spaceMarkersMutex{};
			std::mutex treeNodesMutex{};
			std::mutex treeBudsMutex{};

			std::vector<FreeSpaceMarker> spaceMarkers{};
			std::vector<rwConstTreeNode> treeNodes{};
			std::vector<rwTreeBud> treeBuds{};
			///@}
		};

		inline bool operator==(const ScCell& op1, const ScCell& op2) {
			return op1.gridPosition == op2.gridPosition;
		}

		inline bool operator!=(const ScCell& op1, const ScCell& op2) { return !(op1 == op2); }
		inline bool operator==(const rwScCell op1, const rwScCell op2) { return &op1.get() == &op2.get(); }
	}
}

namespace std
{
	/// std::hash overload for ScCell.
	template <>
	struct hash<Treegen::SpaceCol::ScCell> {
		/// Hash method of ScCell.
		/// \param key ScCell to be hashed.
		/// \returns Hash value of key.
		size_t operator()(const Treegen::SpaceCol::ScCell& key) const {
			return hash<glm::ivec3>{}(key.gridPosition);
		}
	};
}

#endif // SC_CELL_H

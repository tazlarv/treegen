/// Grid of space colonization algorithm.
/// \file sc_grid.cpp
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#include <stdafx.h>
#include <Space colonization/sc_grid.h>

#include <Space colonization/sc_cell.h>
#include <Space colonization/sc_free_space_marker.h>
#include <Tree primitives/tree_primitives.h>

namespace Treegen
{
	namespace SpaceCol
	{
		// ===========================================================================================================
		// Ctor, base support methods
		// ===========================================================================================================

		ScGrid::ScGrid(const float lookupDistance) {
			this->lookupDistance(lookupDistance);
			initGrid();
		}

		void ScGrid::lookupDistance(const float newDistance) {
			if (newDistance <= 0.f) {
				std::invalid_argument{"Space colonization grid lookup distance must be positive!"};
			}
			mLookupDistance = newDistance;
			mLookupCells = unsigned(ceil(mLookupDistance / mCellSideLength));
		}

		glm::ivec3 ScGrid::getGridPosition(const glm::vec3& position) {
			return position / mCellSideLength;
		}

		glm::ivec3 ScGrid::getGridPosition(const float x, const float y, const float z) const {
			return getGridPosition(glm::vec3{x, y, z});
		}

		ScCell& ScGrid::getCellAt(const glm::ivec3& gridPosition) {
			return mGrid[gridPosition.x][gridPosition.y][gridPosition.z];
		}

		ScCell& ScGrid::getCellAt(const int gridX, const int gridY, const int gridZ) {
			return mGrid[gridX][gridY][gridZ];
		}

		ScCell& ScGrid::getCellFor(const glm::vec3& point) {
			return getCellAt(getGridPosition(point));
		}

		// ===========================================================================================================
		// Space markers
		// ===========================================================================================================

		ScCell& ScGrid::addFreeSpaceMarker(FreeSpaceMarker spaceMarker) {
			ScCell& cell = getCellFor(spaceMarker.position);
			std::lock_guard<std::mutex> lock{cell.spaceMarkersMutex};
			cell.spaceMarkers.push_back(std::move(spaceMarker));
			return cell;
		}

		ScCell& ScGrid::removeFreeSpaceMarker(const FreeSpaceMarker& spaceMarker) {
			auto& cell = getCellFor(spaceMarker.position);
			auto& cellMarkers = cell.spaceMarkers;
			std::lock_guard<std::mutex> lock{cell.spaceMarkersMutex};
			const auto itFind = std::find(std::begin(cellMarkers), std::end(cellMarkers), spaceMarker);
			if (itFind != std::end(cellMarkers)) { cellMarkers.erase(itFind); }
			return cell;
		}

		// ===========================================================================================================
		// Tree nodes
		// ===========================================================================================================

		ScCell& ScGrid::addTreeNode(rwConstTreeNode node) {
			ScCell& cell = getCellFor(node.get().position);
			std::lock_guard<std::mutex> lock{cell.treeNodesMutex};
			cell.treeNodes.push_back(node);
			return cell;
		}

		ScCell& ScGrid::removeTreeNode(rwConstTreeNode node) {
			auto& cell = getCellFor(node.get().position);
			auto& cellNodes = cell.treeNodes;
			std::lock_guard<std::mutex> lock{cell.treeNodesMutex};
			const auto itFind = std::find(std::begin(cellNodes), std::end(cellNodes), node);
			if (itFind != std::end(cellNodes)) { cellNodes.erase(itFind); }
			return cell;
		}

		// ===========================================================================================================
		// Tree buds
		// ===========================================================================================================

		ScCell& ScGrid::addTreeBud(rwTreeBud bud) {
			ScCell& cell = getCellFor(bud.get().owner.get().position);
			std::lock_guard<std::mutex> lock{cell.treeBudsMutex};
			cell.treeBuds.push_back(bud);
			return cell;
		}

		ScCell& ScGrid::removeTreeBud(rwTreeBud bud) {
			auto& cell = getCellFor(bud.get().owner.get().position);
			auto& treeBuds = cell.treeBuds;
			std::lock_guard<std::mutex> lock{cell.treeBudsMutex};
			const auto itFind = std::find(std::begin(treeBuds), std::end(treeBuds), bud);
			if (itFind != std::end(treeBuds)) { treeBuds.erase(itFind); }
			return cell;
		}

		// ===========================================================================================================
		// Get cells + Lookup cells API
		// ===========================================================================================================

		void ScGrid::clear() {
			forEachCellDo([](ScCell& cell) { cell.clear(); });
		}

		std::vector<rwScCell> ScGrid::getCellsInRange(const glm::vec3& lowCorner, const glm::vec3& highCorner) {
			return getCellsInRange(getGridPosition(lowCorner), getGridPosition(highCorner));
		}

		std::vector<rwScCell> ScGrid::getCellsInRange(const glm::ivec3& lowCornerCell,
		                                              const glm::ivec3& highCornerCell) {
			std::vector<rwScCell> cellsInRange{};
			forEachCellDo(lowCornerCell, highCornerCell, [&](ScCell& cell) {
				cellsInRange.push_back(cell);
			});
			return cellsInRange;
		}


		std::vector<rwScCell> ScGrid::getCellsInLookupRange(const glm::ivec3& gridPosition) {
			// Calculate ranges
			glm::ivec3 from;
			from.x = (std::max)(0, gridPosition.x - mLookupCells);
			from.y = (std::max)(0, gridPosition.y - mLookupCells);
			from.z = (std::max)(0, gridPosition.z - mLookupCells);

			// mCellsOnSide are counts not indices (=> subtract 1)
			glm::ivec3 to;
			to.x = (std::min)(mCellsOnSide - 1, gridPosition.x + mLookupCells);
			to.y = (std::min)(mCellsOnSide - 1, gridPosition.y + mLookupCells);
			to.z = (std::min)(mCellsOnSide - 1, gridPosition.z + mLookupCells);

			return getCellsInRange(from, to);
		}

		std::vector<rwScCell>
		ScGrid::getCellsInLookupRange(const int middleX, const int middleY, const int middleZ) {
			return getCellsInLookupRange(glm::vec3{middleX, middleY, middleZ});
		}

		void ScGrid::initGrid() {
			int x = 0;
			for (auto&& xAxis : mGrid) {
				int y = 0;
				for (auto&& xyAxis : xAxis) {
					int z = 0;
					for (auto&& cell : xyAxis) {
						cell.gridPosition = {x, y, z};
						++z;
					}
					++y;
				}
				++x;
			}
		}
	}
}

/// Grid of space colonization algorithm.
/// \file sc_grid.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#ifndef SC_GRID_H
#define SC_GRID_H

#include <Utilities/constants.h>
#include <Space colonization/sc_free_space_marker.h>
#include <Space colonization/sc_cell.h>
#include <Tree primitives/tree_primitives.h>

#include <glm/glm.hpp>
#include <array>

namespace Treegen
{
	namespace SpaceCol
	{
		/// Grid of space colonization algorithm.
		class ScGrid {
			/// \name Grid sizes constants.
			///@{
			constexpr static float mCellSideLength = Const::SC_CELL_SIDE;
			constexpr static int mCellsOnSide = Const::SC_GRID_SIDE;
			///@}

			/// Grid representation
			std::array<std::array<std::array<ScCell, mCellsOnSide>, mCellsOnSide>, mCellsOnSide> mGrid{};

			/// \name Lookup distance in grid for assignment of markers to buds.
			///@{
			float mLookupDistance = 0.f;	///< World lookup distance.
			int mLookupCells = 0;			///< Grid lookup distance.
			///@}

		public:
			/// Constructs new grid of space colonization algorithm.
			/// \param lookupDistance Bud lookup distance in space colonization algorithm.
			explicit ScGrid(float lookupDistance);

			/// \name Properties
			///@{
			/// Sets new lookup distance for buds.
			/// \param newDistance New lookup distance.
			void lookupDistance(float newDistance);

			static size_t cellsOnSideCount() { return mCellsOnSide; }	///< Number of cells on side of grid.
			float lookupDistance() const { return mLookupDistance; }	///< Lookup distance of buds in world.
			size_t lookupCells() const { return mLookupCells; }			///< Lookup distance of buds in grid.
			///@}

			/// \name Methods for getting grid cell or cell position based on world or grid coordinates.
			///@{
			static glm::ivec3 getGridPosition(const glm::vec3& position);
			glm::ivec3 getGridPosition(float x, float y, float z) const;
			ScCell& getCellAt(const glm::ivec3& gridPosition);
			ScCell& getCellAt(int gridX, int gridY, int gridZ);
			ScCell& getCellFor(const glm::vec3& point);
			///@}

			/// Adds FreeSpaceMarker to grid.
			/// \return Cell to which was \p spaceMarker added.
			ScCell& addFreeSpaceMarker(FreeSpaceMarker spaceMarker);

			/// Removes FreeSpaceMarker from grid.
			/// If marker is not found does nothing.
			/// \return Cell from which should have been \p spaceMarker removed.
			ScCell& removeFreeSpaceMarker(const FreeSpaceMarker& spaceMarker);

			/// Adds TreeNode to grid.
			/// \return Cell to which was \p node added.
			ScCell& addTreeNode(rwConstTreeNode node);

			/// Removes TreeNode from grid.
			/// If node is not found does nothing.
			/// \return Cell from which should have been \p node removed.
			ScCell& removeTreeNode(rwConstTreeNode node);

			/// Adds TreeBud to grid.
			/// \return Cell to which was \p bud added.
			ScCell& addTreeBud(rwTreeBud bud);

			/// Removes TreeBud from grid.
			/// If bud is not found does nothing.
			/// \return Cell from which should have been \p bud removed.
			ScCell& removeTreeBud(rwTreeBud bud);

			/// \name Addition and removal of multiple instances of type to/from grid based on iterator range.
			///@{
			template <typename TConstIterator>
			void addTreeNode(TConstIterator begin, TConstIterator end) {
				for (auto it = begin; it != end; ++it) { addTreeNode(*it); }
			}

			template <typename TConstIterator>
			void removeTreeNode(TConstIterator begin, TConstIterator end) {
				for (auto it = begin; it != end; ++it) { removeTreeNode(*it); }
			}

			template <typename TIterator>
			void addTreeBud(TIterator begin, TIterator end) {
				for (auto it = begin; it != end; ++it) { addTreeBud(*it); }
			}

			template <typename TIterator>
			void removeTreeBud(TIterator begin, TIterator end) {
				for (auto it = begin; it != end; ++it) { removeTreeBud(*it); }
			}

			template <typename TConstIterator>
			void addFreeSpaceMarker(TConstIterator begin, TConstIterator end) {
				for (auto it = begin; it != end; ++it) { addFreeSpaceMarker(*it); }
			}

			template <typename TConstIterator>
			void removeFreeSpaceMarker(TConstIterator begin, TConstIterator end) {
				for (auto it = begin; it != end; ++it) { removeFreeSpaceMarker(*it); }
			}
			///@}

			/// Calls method on each cell of grid.
			/// \tparam TCellMethod Method taking ScCell& as parameter.
			/// \param method Method to be called on grid cells.
			template <typename TCellMethod>
			void forEachCellDo(TCellMethod method);

			/// Calls method on each cell in world coordinates range.
			/// \tparam TCellMethod Method taking ScCell& as parameter.
			/// \param lowCorner Lowest values of world range.
			/// \param highCorner Highest values of world range.
			/// \param method Method to be called on grid cells.
			/// \warning Throws std::out_of_range exception if range is outside of grid.
			template <typename TCellMethod>
			void forEachCellDo(const glm::vec3& lowCorner, const glm::vec3& highCorner, TCellMethod method) {
				forEachCellDo(getGridPosition(lowCorner), getGridPosition(highCorner), method);
			}

			/// Calls method on each cell in grid cell range.
			/// \tparam TCellMethod Method taking ScCell& as parameter.
			/// \param lowCornerCell Lowest values of grid cell range.
			/// \param highCornerCell Highest values of grid cell range.
			/// \param method Method to be called on grid cells.
			/// \warning Throws std::out_of_range exception if range is outside of grid.
			template <typename TCellMethod>
			void forEachCellDo(const glm::ivec3& lowCornerCell, const glm::ivec3& highCornerCell, TCellMethod method);

			/// Clears all data in cells of grid.
			void clear();

			/// Gets all cells in world coordinates range.
			/// \param lowCorner Lowest values of world range.
			/// \param highCorner Highest values of world range.
			/// \return Cells in specified range.
			/// \warning Throws std::out_of_range exception if range is outside of grid.
			std::vector<rwScCell> getCellsInRange(const glm::vec3& lowCorner, const glm::vec3& highCorner);
			
			/// Gets all cells in grid cell range.
			/// \param lowCornerCell Lowest values of grid cell range.
			/// \param highCornerCell Highest values of grid cell range.
			/// \return Cells in specified range.
			/// \warning Throws std::out_of_range exception if range is outside of grid.
			std::vector<rwScCell> getCellsInRange(const glm::ivec3& lowCornerCell, const glm::ivec3& highCornerCell);

			/// Gets all cells in lookup distance from cell position.
			/// \param gridPosition Cell position.
			/// \return Cells in lookup range.
			/// \warning Throws std::out_of_range exception if position is outside of grid.
			std::vector<rwScCell> getCellsInLookupRange(const glm::ivec3& gridPosition);
			
			/// Gets all cells in lookup distance from cell position.
			/// \param middleX Cell position X.
			/// \param middleY Cell position Y.
			/// \param middleZ Cell position Z.
			/// \return Cells in lookup range.
			/// \warning Throws std::out_of_range exception if position is outside of grid.
			std::vector<rwScCell> getCellsInLookupRange(int middleX, int middleY, int middleZ);

		private:
			/// Initializes coordinates of cells in grid.
			void initGrid();
		};

		template <typename TCellMethod>
		void ScGrid::forEachCellDo(TCellMethod method) {
			for (auto&& xArray : mGrid) {
				for (auto&& xyArray : xArray) {
					for (auto&& cell : xyArray) { method(cell); }
				}
			}
		}

		template <typename TCellMethod>
		void ScGrid::forEachCellDo(const glm::ivec3& lowCornerCell, const glm::ivec3& highCornerCell,
		                           TCellMethod method) {
			for (auto x = lowCornerCell.x; x <= highCornerCell.x; ++x) {
				for (auto y = lowCornerCell.y; y <= highCornerCell.y; ++y) {
					for (auto z = lowCornerCell.z; z <= highCornerCell.z; ++z) { method(mGrid[x][y][z]); }
				}
			}
		}
	}
}

#endif	// SC_GRID_H

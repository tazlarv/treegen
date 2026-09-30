/// Space colonization algorithm.
/// \file sc_algorithm.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#ifndef SC_ALGORITHM_H
#define SC_ALGORITHM_H

#include <Scene/scene.h>
#include <Space colonization/sc_grid.h>
#include <Tree primitives/tree_primitives.h>

#include <glm/glm.hpp>

#include <tbb/parallel_for.h>
#include <tbb/concurrent_vector.h>
#include <tbb/concurrent_unordered_set.h>
#include <tbb/concurrent_unordered_map.h>

#include <memory>
#include <vector>
#include <mutex>

namespace Treegen
{
	namespace SpaceCol
	{
		using budsNodesVectorsPair = std::pair<std::vector<rwTreeBud>, std::vector<rwConstTreeNode>>;

		/// Space colonization algorithm.
		class SpaceColonization {
			/// \name Default values for settings of algorithm.
			///@{
			constexpr static float DEF_ATT_DISTANCE = 5.f;
			constexpr static float DEF_ATT_DIST_SQR = DEF_ATT_DISTANCE * DEF_ATT_DISTANCE;
			constexpr static float DEF_KILL_DISTANCE = 2.f;
			constexpr static float DEF_KILL_DIST_SQR = DEF_KILL_DISTANCE * DEF_KILL_DISTANCE;
			constexpr static size_t DEF_BUD_MARKERS_COUNT = 3;
			constexpr static float DEF_CONE_ANGLE = glm::radians(90.f);
			///@}

			/// \name Algorithm settings and helper values based on them.
			///@{
			float mAttractionDistance = DEF_ATT_DISTANCE;
			float mAttractionDistanceSqr = DEF_ATT_DIST_SQR;

			float mKillDistance = DEF_KILL_DISTANCE;
			float mKillDistanceSqr = DEF_KILL_DIST_SQR;

			size_t mBudMarkersCount = DEF_BUD_MARKERS_COUNT;
			float mValidConeHeight = DEF_ATT_DISTANCE - DEF_KILL_DISTANCE;

			float mConeAngle = DEF_CONE_ANGLE;
			float mConeBaseRadius;
			///@}

			/// \name Bud index management.
			///@{
			std::mutex mBudIndexMutex{};
			unsigned mBudIndex = 0;
			///@}

			/// \name Base data structures.
			///@{
			std::shared_ptr<Scene::BoundedScene> mScene;
			std::unique_ptr<ScGrid> mGrid = nullptr;
			///@}

			/// \name Data structures of algorithm iteration.
			///@{
			/// Cells in current iteration.
			tbb::concurrent_unordered_set<rwScCell, std::hash<ScCell>> mCellsInIteration{};
			/// Buds in current iteration.
			tbb::concurrent_vector<rwTreeBud> mBudsInIteration{}; 
			/// Assigned markers to each bud.
			tbb::concurrent_unordered_multimap<rwTreeBud, rwFreeSpaceMarker, std::hash<TreeBud>> mBudSpaceMarkersPairs
				{};
			///@}

		public:
			/// Constructs new SpaceColonization object.
			/// \param scene Scene representation.
			explicit SpaceColonization(std::shared_ptr<Scene::BoundedScene> scene);

			/// Sets attraction and kill distance.
			/// \param attractionDistance Attraction distance must be at least 3.0.
			/// \param killDistance Kill distance must be at least 2.0 and less than \p attractionDistance.
			/// \warning For invalid arguments throws std::invalid_argument exception.
			void attractionKillDistance(float attractionDistance, float killDistance);
			float attractionDistance() const { return mAttractionDistance; }
			float killDistance() const { return mKillDistance; }

			/// Sets number of generated free space markers for each bud.
			/// \param count Nonzero number of markers.
			/// \warning For invalid argument throws std::invalid_argument exception.
			void budFreeSpaceMarkers(size_t count);
			size_t budFreeSpaceMarkers() const { return mBudMarkersCount; }

			/// Sets angle of perception volume cone.
			/// \param angleRadians Angle of cone in radians from range [0.0, 160.0] degrees.
			/// \warning For invalid argument throws std::invalid_argument exception.
			void coneAngle(float angleRadians);
			float coneAngle() const { return mConeAngle; }

			/// \name Tree nodes manipulation.
			///@{
			void addTreeNode(rwConstTreeNode node);		///< Adds TreeNode to space colonization representation.
			void removeTreeNode(rwConstTreeNode node);	///< Removes TreeNode from space colonization representation.

			/// Adds multiple TreeNode to space colonization representation.
			template <typename TConstIterator>
			void addTreeNode(TConstIterator begin, TConstIterator end);

			/// Removes multiple TreeNode from space colonization representation.
			template <typename TConstIterator>
			void removeTreeNode(TConstIterator begin, TConstIterator end);
			///@}

			/// Adds tree buds for next iteration of space colonization.
			/// Can be called from multiple threads.
			template <typename TIterator>
			void addTreeBuds(TIterator begin, TIterator end);
			
			/// Runs space colonization on inserted buds.
			/// Must not be called before \p addTreeBuds are finished.
			void run();

			/// Clears all data from space colonization.
			void clear();

		private:
			/// Calculates radius of perception cone base.
			float calcConeBaseRadius() const;

			/// Adds bud and its markers to space colonization grid.
			/// \param bud Bud to be added.
			/// \param budIndex Bud index.
			void addSpaceMarkersOfBud(TreeBud& bud, unsigned budIndex);

			/// Gets random position for space marker in its perception volume.
			/// \param bud Bud for which is marker position derived.
			/// \return Marker position of \p bud.
			glm::vec3 getRndBudSpaceMarkerPosition(TreeBud& bud) const;

			/// Assigns free space markers.
			void assignCellMarkers();

			/// Assigns free space markers in cell of space colonization grid.
			/// \param cell Cell for marker assignment.
			void assignCellMarkers(ScCell& cell);

			/// Collects buds and nodes from cells.
			/// \param cells Cells from which are data collected. 
			/// \return Buds and nodes in cell.
			static budsNodesVectorsPair budsNodesFromCells(const std::vector<rwScCell>& cells);

			/// Tries to assign markers to buds.
			/// \param marker Markers to be assigned.
			/// \param budsAndNodes Buds and nodes for proper assignment.
			void tryAssignMarkerToBud(FreeSpaceMarker& marker, const budsNodesVectorsPair& budsAndNodes);

			/// Checks if point is in perception volume of bud.
			/// \param bud Bud whose perception volume is checked.
			/// \param point Point to be checked.
			/// \return Result of check.
			bool isInBudAttractionSpace(const TreeBud& bud, const glm::vec3& point) const;

			/// For all buds in iteration calculates their prefered direction based on assigned markers.
			void calculateBudsSpace();

			/// Clears internal data structures used for run of algorithm.
			void clearAfterRun();
		};

		template <typename TConstIterator>
		void SpaceColonization::addTreeNode(TConstIterator begin, TConstIterator end) {
			for (auto it = begin; it != end; ++it) {
				const TreeNode& node = *it;
				addTreeNode(node);
			}
		}

		template <typename TConstIterator>
		void SpaceColonization::removeTreeNode(TConstIterator begin, TConstIterator end) {
			for (auto it = begin; it != end; ++it) {
				const TreeNode& node = *it;
				removeTreeNode(node);
			}
		}

		template <typename TIterator>
		void SpaceColonization::addTreeBuds(TIterator begin, TIterator end) {
			std::vector<rwTreeBud> buds{};
			for (auto it = begin; it != end; ++ it) { buds.push_back(*it); }

			const unsigned budBaseId = [&]() {
				std::lock_guard<std::mutex> lock{mBudIndexMutex};
				const auto currentBudId = mBudIndex;
				mBudIndex += unsigned(buds.size());
				return currentBudId;
			}();

			tbb::parallel_for(tbb::blocked_range<size_t>(0, buds.size()),
				[&](const tbb::blocked_range<size_t>& range) {
					for (auto i = range.begin(); i != range.end(); ++i) {
						TreeBud& bud = buds[i];
						addSpaceMarkersOfBud(bud, budBaseId + unsigned(i));
						mGrid->addTreeBud(bud);
					}
				});

			mBudsInIteration.grow_by(begin, end);
		}
	}
}

#endif // SC_ALGORITHM_H

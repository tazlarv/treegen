/// Space colonization algorithm.
/// \file sc_algorithm.cpp
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#include <stdafx.h>
#include <Space colonization/sc_algorithm.h>

#include <Space colonization/sc_grid.h>
#include <Tree primitives/tree_primitives.h>
#include <Scene/scene.h>

namespace Treegen
{
	namespace SpaceCol
	{
		// ===========================================================================================================
		// Ctor, properties, setters, getters
		// ===========================================================================================================

		SpaceColonization::SpaceColonization(std::shared_ptr<Scene::BoundedScene> scene) :
			mScene{std::move(scene)} {
			mGrid = std::make_unique<ScGrid>(mAttractionDistance);
			mConeBaseRadius = calcConeBaseRadius();
		}

		void SpaceColonization::attractionKillDistance(const float attractionDistance, const float killDistance) {
			if (attractionDistance < 3.f) { throw std::invalid_argument{"SC - att distance < 3.0f!"}; }
			if (killDistance < 2.f) { throw std::invalid_argument{"SC - kill distance < 2.0f!"}; }
			if (killDistance >= attractionDistance) {
				throw std::invalid_argument{"SC - kill distance >= att_distance!"};
			}
			mAttractionDistance = attractionDistance;
			mAttractionDistanceSqr = attractionDistance * attractionDistance;
			mKillDistance = killDistance;
			mKillDistanceSqr = killDistance * killDistance;
			mValidConeHeight = attractionDistance - killDistance;
			mGrid->lookupDistance(mAttractionDistance);
		}

		void SpaceColonization::budFreeSpaceMarkers(const size_t count) {
			if (count < 1) { throw std::invalid_argument{"Space markers for bud (count) must be positive value!"}; }
			mBudMarkersCount = count;
		}

		void SpaceColonization::coneAngle(const float angleRadians) {
			if (angleRadians <= 0.f || angleRadians > glm::radians(160.f)) {
				throw std::invalid_argument{"Space colonization - cone angle <= 0 or > 160 degrees!"};
			}
			mConeAngle = angleRadians;
			mConeBaseRadius = calcConeBaseRadius();
		}

		// ===========================================================================================================
		// Tree nodes manipulation
		// ===========================================================================================================

		void SpaceColonization::addTreeNode(const rwConstTreeNode node) {
			mGrid->addTreeNode(node);
		}

		void SpaceColonization::removeTreeNode(const rwConstTreeNode node) {
			mGrid->removeTreeNode(node);
		}

		void SpaceColonization::run() {
			assignCellMarkers();
			calculateBudsSpace();
			clearAfterRun();
		}

		void SpaceColonization::clear() {
			mGrid->clear();
			mCellsInIteration.clear();
			mBudsInIteration.clear();
			mBudSpaceMarkersPairs.clear();
		}

		// ===========================================================================================================
		// Private methods
		// ===========================================================================================================

		float SpaceColonization::calcConeBaseRadius() const { return tan(mConeAngle / 2.f) * mAttractionDistance; }

		void SpaceColonization::addSpaceMarkersOfBud(TreeBud& bud, const unsigned budIndex) {
			const unsigned baseIndex = budIndex * unsigned(mBudMarkersCount);
			for (int i = 0; i < mBudMarkersCount; ++i) {
				const auto markerPosition = getRndBudSpaceMarkerPosition(bud);
				if (mScene->isValidDirection(bud.owner.get().position, markerPosition)) {
					mCellsInIteration.insert(mGrid->addFreeSpaceMarker(FreeSpaceMarker{markerPosition, baseIndex + i}));
				}
			}
		}

		glm::vec3 SpaceColonization::getRndBudSpaceMarkerPosition(TreeBud& bud) const {
			const std::uniform_real_distribution<float> percent(0.f, 1.f);

			// Random vector at base of cone of perception volume
			const auto baseOrthogonal = Utils::getRndOrthogonalNormalized(bud.orientationNormalized, bud.rndEngine)
			                            * mConeBaseRadius * percent(bud.rndEngine);

			// Normalized vector from bud to position at base
			const auto rndFromBud = glm::normalize(baseOrthogonal + bud.orientationNormalized * mAttractionDistance);

			// Final position is between kill and (max) attraction distance
			const auto distanceFromBud = mKillDistance + (mValidConeHeight * percent(bud.rndEngine));
			return bud.owner.get().position + rndFromBud * distanceFromBud;
		}

		void SpaceColonization::assignCellMarkers() {
			// Overhead of parallel_for_each is bigger than collecting all to vector and running parallel_for
			std::vector<rwScCell> cells{};
			cells.reserve(mCellsInIteration.size());
			for (auto&& cell : mCellsInIteration) { cells.push_back(cell); }

			tbb::parallel_for(tbb::blocked_range<size_t>(0, cells.size()),
				[&](const tbb::blocked_range<size_t>& range) {
					for (auto i = range.begin(); i != range.end(); ++i) {
						ScCell& cell = cells[i];
						if (!cell.spaceMarkers.empty()) { assignCellMarkers(cell); }
					}
				});
		}

		void SpaceColonization::assignCellMarkers(ScCell& cell) {
			const auto budsAndNodes = budsNodesFromCells(mGrid->getCellsInLookupRange(cell.gridPosition));
			for (auto markerIt = std::begin(cell.spaceMarkers); markerIt != std::end(cell.spaceMarkers); ++markerIt) {
				tryAssignMarkerToBud(*markerIt, budsAndNodes);
			}
		}

		budsNodesVectorsPair SpaceColonization::budsNodesFromCells(const std::vector<rwScCell>& cells) {
			std::vector<rwTreeBud> buds{};
			std::vector<rwConstTreeNode> nodes{};

			for (auto&& cell : cells) {
				buds.insert(std::end(buds), std::begin(cell.get().treeBuds), std::end(cell.get().treeBuds));
				nodes.insert(std::end(nodes), std::begin(cell.get().treeNodes), std::end(cell.get().treeNodes));
			}
			return {buds, nodes};
		}

		void SpaceColonization::tryAssignMarkerToBud(FreeSpaceMarker& marker,
		                                             const budsNodesVectorsPair& budsAndNodes) {
			// Bud can not exist without its node => tests for kill distance on buds dont make sense
			for (auto&& node : budsAndNodes.second) {
				const float distanceSqr = GlmUtils::distanceSqr(marker.position, node.get().position);
				if (distanceSqr <= mKillDistanceSqr) { return; }
			}

			float minDistance = mAttractionDistanceSqr;
			std::vector<rwTreeBud> assignedBuds{};

			for (auto&& bud : budsAndNodes.first) {
				auto& budPosition = bud.get().owner.get().position;
				const float distanceSqr = GlmUtils::distanceSqr(marker.position, budPosition);

				if (distanceSqr > minDistance || !isInBudAttractionSpace(bud, marker.position) ||
				    mScene->intersectsWithScene(budPosition, marker.position)) { continue; }

				// Is in attraction space in valid distance
				if (distanceSqr < minDistance) {
					assignedBuds.clear();
					minDistance = distanceSqr;
					assignedBuds.push_back(bud);
				}
				else { assignedBuds.push_back(bud); }
			}

			if (!assignedBuds.empty()) {
				for (auto&& rwBud : assignedBuds) {
					mBudSpaceMarkersPairs.emplace(rwBud, marker);
				}
			}
		}


		bool SpaceColonization::isInBudAttractionSpace(const TreeBud& bud, const glm::vec3& point) const {
			// For purpose of calculation - tip of cone (bud) is at position (0,0,0) => calculate position for att_p
			const auto relativePointPosition = point - bud.owner.get().position;

			// Projection on normalized orientation vector (cos * ||hypotenuse|| = ||adjacent||)
			const auto coneAxisDistance = glm::dot(relativePointPosition, bud.orientationNormalized);
			if (coneAxisDistance < 0.f) { return false; }

			// Compare expected distance from axis with real one
			const auto coneRadius = mConeBaseRadius * (coneAxisDistance / mAttractionDistance);
			const auto axisToPointVec = relativePointPosition - coneAxisDistance * bud.orientationNormalized;
			const auto axisToPointDistSqr = glm::dot(axisToPointVec, axisToPointVec);

			return axisToPointDistSqr <= coneRadius * coneRadius;
		}

		void SpaceColonization::calculateBudsSpace() {
			tbb::parallel_for(tbb::blocked_range<size_t>(0, mBudsInIteration.size()),
				[&](const tbb::blocked_range<size_t>& range) {
					for (auto i = range.begin(); i != range.end(); ++i) {
						TreeBud& bud = mBudsInIteration[i];

						const auto budMarkersRange = mBudSpaceMarkersPairs.equal_range(bud);
						if (budMarkersRange.first == budMarkersRange.second) { bud.status = BudStatus::NoSpace; }
						else {
							const auto budPosition = bud.owner.get().position;
							glm::vec3 preferedDirection{0.f, 0.f, 0.f};

							// Get markers in deterministic order with sort by id
							// Otherwise rounding errors give different output 
							// depending on order coming from parallel assignment of markers
							std::vector<rwFreeSpaceMarker> markers{};
							for (auto it = budMarkersRange.first; it != budMarkersRange.second; ++it) {
								markers.push_back(it->second);
							}

							std::sort(std::begin(markers), std::end(markers),
								[](const FreeSpaceMarker& a, const FreeSpaceMarker& b) {
									return a.id < b.id;
								});

							for (auto&& rwSpaceMarker : markers) {
								preferedDirection += glm::normalize(rwSpaceMarker.get().position - budPosition);
							}

							bud.setFreeSpaceDir(preferedDirection);
						}
					}
				});
		}

		void SpaceColonization::clearAfterRun() {
			for (auto&& cell : mCellsInIteration) {
				cell.get().treeBuds.clear();
				cell.get().spaceMarkers.clear();
			}
			mCellsInIteration.clear();
			mBudsInIteration.clear();
			mBudSpaceMarkersPairs.clear();
			std::lock_guard<std::mutex> lock{mBudIndexMutex};
			mBudIndex = 0;
		}
	}
}

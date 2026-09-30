/// Light model - shadow propagation.
/// \file shadow_propagation.cpp
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#include <stdafx.h>
#include <Shadow propagation/shadow_propagation.h>

namespace Treegen
{
	namespace ShadowProp
	{
		ShadowPropagation::ShadowPropagation(const size_t pyramidHeight, const float nodeShadow,
		                                     const float diminishMultiplier, const float maxLight) {
			setProperties(pyramidHeight, nodeShadow, diminishMultiplier, maxLight);
		}

		void ShadowPropagation::setProperties(const size_t pyramidHeight, const float nodeShadow,
		                                      const float diminishMultiplier, const float maxLight) {
			if (pyramidHeight == 0) { throw std::invalid_argument{"Pyramid height must be positive!"}; }
			if (nodeShadow < 0.f) { throw std::invalid_argument{"Negative node shadow value!"}; }
			if (diminishMultiplier < 0.f || diminishMultiplier > 1.f) {
				throw std::invalid_argument{"Shadow diminish multiplier not in range [0.0f, 1.0f]!"};
			}
			if (maxLight < 0.f) { throw std::invalid_argument{"Maximum light must not be negative!"}; }


			mPyramidHeight = int(pyramidHeight);
			mNodeShadow = nodeShadow;
			mDiminishMultiplier = diminishMultiplier;
			mMaxLight = maxLight;

			precalcShadowValues();
		}


		void ShadowPropagation::addTreeNode(const TreeNode& node) {
			const auto gridPosition = getGridPosition(node.position);
			iteratePyramid(gridPosition, [&](int depth, SpCell& cell) {
				cell.shadowValue += mShadowValueInDepth[depth];
			});
		}

		void ShadowPropagation::removeTreeNode(const TreeNode& node) {
			const auto gridPosition = getGridPosition(node.position);
			iteratePyramid(gridPosition, [&](int depth, SpCell& cell) {
				cell.shadowValue -= mShadowValueInDepth[depth];
			});
		}

		float ShadowPropagation::getResources(const glm::vec3& position) {
			const auto& cell = getCellFor(position);
			return (std::max)(mMaxLight - cell.shadowValue, 0.0f);
		}

		void ShadowPropagation::assignResources(TreeBud& bud) {
			bud.resources = getResources(bud.owner.get().position);
		}

		void ShadowPropagation::clear() {
			for (auto&& yAxis : mGridYXZ) {
				for (auto&& yxAxis : yAxis) {
					for (auto&& cell : yxAxis) { cell = {}; }
				}
			}
		}

		glm::ivec3 ShadowPropagation::getGridPosition(const glm::vec3& position) {
			return position;
		}

		glm::ivec3 ShadowPropagation::getGridPosition(const float x, const float y, const float z) {
			return getGridPosition(glm::vec3{x, y, z});
		}

		ShadowPropagation::SpCell& ShadowPropagation::getCellAt(const glm::ivec3& gridPosition) {
			return mGridYXZ[gridPosition.y][gridPosition.x][gridPosition.z];
		}

		ShadowPropagation::SpCell& ShadowPropagation::getCellAt(const int gridX, const int gridY, const int gridZ) {
			return mGridYXZ[gridY][gridX][gridZ];
		}

		ShadowPropagation::SpCell& ShadowPropagation::getCellFor(const glm::vec3& point) {
			return getCellAt(getGridPosition(point));
		}

		void ShadowPropagation::precalcShadowValues() {
			mShadowValueInDepth.clear();
			mShadowValueInDepth.push_back(0.f);	// For given cell we do not count shadow of nodes inside of it
			float diminish = 1.f;
			for (int i = 1; i <= mPyramidHeight; ++i) {
				diminish *= mDiminishMultiplier;
				mShadowValueInDepth.push_back(mNodeShadow * diminish);
			}
		}
	}
}

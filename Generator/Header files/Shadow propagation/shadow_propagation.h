/// Light model - shadow propagation.
/// \file shadow_propagation.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#ifndef SHADOW_PROPAGATION_H
#define SHADOW_PROPAGATION_H

#include <Utilities/constants.h>
#include <Tree primitives/tree_primitives.h>

#include <glm/glm.hpp>

#include <vector>
#include <array>

namespace Treegen
{
	namespace ShadowProp
	{
		/// Light model - shadow propagation.
		class ShadowPropagation {
			/// Cell of shadow propagation grid.
			struct SpCell {
				float shadowValue = 0.f;
			};

			/// \name Default values for settings of model.
			///@{
			constexpr static size_t DEF_PYRAMID_HEIGHT = 6;
			constexpr static float DEF_NODE_SHADOW = 0.2f;
			constexpr static float DEF_DIMINISH_MULTIPLIER = 0.5f;
			constexpr static float DEF_MAX_LIGHT = 1.f;
			///@}

			/// \name Model settings and helper values based on them.
			///@{
			int mPyramidHeight = DEF_PYRAMID_HEIGHT;
			float mNodeShadow = DEF_NODE_SHADOW;
			float mDiminishMultiplier = DEF_DIMINISH_MULTIPLIER;
			float mMaxLight = DEF_MAX_LIGHT;
			///@}

			std::vector<float> mShadowValueInDepth{}; ///< Precalculated values of shadow for each layer of shadow pyramid.
			
			/// Grid representation.
			/// Grid axes are in order y-x-z for better cache usage when iterating layer by layer.
			std::array<std::array<std::array<SpCell,
				Const::SP_GRID_SIDE>, Const::SP_GRID_SIDE>, Const::SP_GRID_SIDE> mGridYXZ{};

		public:
			/// Constructs new light model based on shadow propagation.
			/// \param pyramidHeight Positive value of height of shadow propagation pyramid in grid. 
			/// \param nodeShadow Non negative shadow value of node in its grid cell.
			/// \param diminishMultiplier Diminish multiplier of shadow between layers from range [0.0, 1.0].
			/// \param maxLight Non negative maximum light exposure value.
			/// \warning For invalid arguments throws std::invalid_argument exception.
			explicit ShadowPropagation(size_t pyramidHeight = DEF_PYRAMID_HEIGHT, float nodeShadow = DEF_NODE_SHADOW,
			                           float diminishMultiplier = DEF_DIMINISH_MULTIPLIER,
			                           float maxLight = DEF_MAX_LIGHT);

			/// \name Properties getters.
			///@{
			size_t pyramidHeight() const { return size_t(mPyramidHeight); }
			float nodeShadow() const { return mNodeShadow; }
			float diminishingSpeed() const { return mDiminishMultiplier; }
			float maxLight() const { return mMaxLight; }
			///@}

			/// Sets new properties of light model.
			/// \param pyramidHeight Positive value of height of shadow propagation pyramid in grid. 
			/// \param nodeShadow Non negative shadow value of node in its grid cell.
			/// \param diminishMultiplier Diminish multiplier of shadow between layers from range [0.0, 1.0].
			/// \param maxLight Non negative maximum light exposure value.
			/// \warning For invalid arguments throws std::invalid_argument exception.
			/// Does not recalculate values of current nodes in shadow propagation.
			///	Only newly inserted nodes will follow new properties.
			/// For complete change of current nodes based on new properties clear and reinsert nodes 
			/// or do so on new object of ShadowPropagation.
			void setProperties(size_t pyramidHeight, float nodeShadow, float diminishMultiplier, float maxLight);

			void addTreeNode(const TreeNode& node);		///< Adds TreeNode to light model.
			void removeTreeNode(const TreeNode& node);	///< Removes TreeNode from light model.

			/// Adds multiple TreeNode to space light model.
			template <typename TConstIterator>
			void addTreeNode(TConstIterator begin, TConstIterator end);

			/// Removes multiple TreeNode from space light model.
			template <typename TConstIterator>
			void removeTreeNode(TConstIterator begin, TConstIterator end);

			/// Gets available light (resources) at position.
			/// \param position Position to be checked for light.
			/// \return Available resources at position.
			float getResources(const glm::vec3& position);

			/// Assigns available resources to bud.
			/// \param bud Bud to be assigned to.
			void assignResources(TreeBud& bud);

			/// Assigns available resources to all buds in iterator range.
			template <typename TIterator>
			void assignResources(TIterator begin, TIterator end);

			/// Resets light model.
			void clear();

		private:
			/// \name Methods for getting grid cell or cell position based on world and grid coordinates.
			///@{
			static glm::ivec3 getGridPosition(const glm::vec3& position);
			static glm::ivec3 getGridPosition(float x, float y, float z);

			SpCell& getCellAt(const glm::ivec3& gridPosition);
			SpCell& getCellAt(int gridX, int gridY, int gridZ);
			SpCell& getCellFor(const glm::vec3& point);
			///@}

			/// Precalculates shadow values for all layers of propagation.
			void precalcShadowValues();

			/// Iterates part of light model grid cells in shape of pyramid based on current settings and applies method on them.
			/// \tparam TPyramidMethod Method to be applied on pyramid cells. 
			///	Method arguments are supposed to be (int, SpCell&) where int is depth of cell in pyramid.
			/// \param apexPosition Grid position of apex of iterated pyramid.
			/// \param method Method to be called on cells of iterated pyramid.
			template <typename TPyramidMethod>
			void iteratePyramid(const glm::ivec3& apexPosition, TPyramidMethod method);
		};

		template <typename TConstIterator>
		void ShadowPropagation::addTreeNode(TConstIterator begin, TConstIterator end) {
			for (auto it = begin; it != end; ++it) { addTreeNode(*it); }
		}

		template <typename TConstIterator>
		void ShadowPropagation::removeTreeNode(TConstIterator begin, TConstIterator end) {
			for (auto it = begin; it != end; ++it) { removeTreeNode(*it); }
		}

		template <typename TIterator>
		void ShadowPropagation::assignResources(TIterator begin, TIterator end) {
			for (auto it = begin; it != end; ++it) { assignResources(*it); }
		}

		template <typename TPyramidMethod>
		void ShadowPropagation::iteratePyramid(const glm::ivec3& apexPosition, TPyramidMethod method) {
			assert(apexPosition.y >= 0 && apexPosition.y < mGridYXZ.size());

			const auto maxDepth = (apexPosition.y < mPyramidHeight) ? apexPosition.y : mPyramidHeight;
			const auto gridSide = int(mGridYXZ.size());

			for (auto depth = 0; depth <= maxDepth; ++depth) {
				const auto yPos = apexPosition.y - depth;
				const auto xPosBegin = (std::max)(0, apexPosition.x - depth);
				const auto zPosBegin = (std::max)(0, apexPosition.z - depth);
				const auto xPosEnd = (std::min)(gridSide, apexPosition.x + depth + 1);
				const auto zPosEnd = (std::min)(gridSide, apexPosition.z + depth + 1);

				for (auto xPos = xPosBegin; xPos < xPosEnd; ++xPos) {
					for (auto zPos = zPosBegin; zPos < zPosEnd; ++zPos) {
						method(depth, mGridYXZ[yPos][xPos][zPos]);
					}
				}
			}
		}
	}
}


#endif // SHADOW_PROPAGATION_H

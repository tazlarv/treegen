/// Implementation of opposite and alternate type of phyllotaxis.
/// \file general_phyllotaxis.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#ifndef GENERAL_PHYLLOTAXIS_H
#define GENERAL_PHYLLOTAXIS_H

#include <Phyllotaxis/abstract_phyllotaxis.h>
#include <Tree primitives/tree_primitives.h>

#include <glm/glm.hpp>

#include <tbb/concurrent_unordered_map.h>

namespace Treegen
{
	namespace Phyllotaxis
	{
		
		/// Alternate and opposite phyllotaxis.
		class GeneralPhyllotaxis final : public AbstractPhyllotaxis {
		public:
			enum class Type {
				Opposite,
				Alternate
			};

		private:

			Type mType = Type::Alternate;
			
			/// Orientations of buds in nodes.
			/// Orientation from preceding node is needed for setting following node.
			tbb::concurrent_unordered_map<rwTreeNode, glm::vec3, std::hash<TreeNode>> mNodePhyllotacticVec{};

			/// Generates lateral buds in non root node.
			/// \param target Node for which should buds be generated.
			/// \param from Bud from which \p target grew.
			void generateInnerLateralBuds(TreeNode& target, TreeBud& from) override;

		public:

			/// Sets the type of phyllotaxis.
			/// \param type Type of phyllotaxis.
			/// \warning May be changed during growth of tree but is not advised to do so.
			void type(const Type type) { mType = type; }
			
			Type type() const { return mType; }
			size_t lateralBudsCount() const override;

			/// Generates buds in root of tree.
			/// \param root Root node of tree.
			/// \param seed Seed for setting root buds random generators.
			void generateRootBuds(TreeNode& root, unsigned seed) override;

			/// Prepares phyllotaxis to be used for new tree.
			void clear() override;
		};
	}
}

#endif // GENERAL_PHYLLOTAXIS_H

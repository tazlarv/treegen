/// Redistribution of resources based on Extendet Borchert-Honda model.
/// \file ext_borchert_honda.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#ifndef EG_EXT_BORCHERT_HONDA_H
#define EG_EXT_BORCHERT_HONDA_H

#include <Signaling/abstract_resources_model.h>
#include <Tree primitives/tree_primitives.h>

#include <vector>
#include <unordered_map>
#include <queue>

namespace Treegen
{
	namespace Signaling
	{
		/// Extendet Borchert-Honda model.
		class ExtBorchertHondaModel final : public AbstractResourcesModel {

			/// Data used for each node of tree during redistribution upwards.
			struct NodeUpInfo {
				NodeUpInfo(const TreeNode& node, const float resources) :
					node{node}, resources{resources} {}

				const TreeNode& node;
				float resources;
			};

			float mBiasToMain;
			
			/// \name Algorithm run data structures.
			///@{
			std::unordered_map<rwConstTreeNode, float, std::hash<TreeNode>> mResources{};
			std::queue<NodeUpInfo> mBranchBeginNodes{};
			///@}

		public:

			/// Constructs new ExtBorchertHondaModel instance.
			/// \param biasToMain Preference of growing of main branches from range [0.0, 1.0]
			/// \warning For invalid argument throws std::invalid_argument exception.
			explicit ExtBorchertHondaModel(float biasToMain = 0.5f);

			float biasToMain() const { return mBiasToMain; }

			/// Redistributes resources in buds of tree.
			/// \param root Root of tree.
			/// \param buds Buds with resources to be redistributed.
			/// \warning Resources in buds should not be negative values otherwise behaviour is undefined.
			void redistributeResources(const TreeNode& root, const std::vector<rwTreeBud>& buds) override;

		private:

			/// Propagates resources to root.
			/// \param budFrom Bud from which are resources propagated.
			void propagateToRoot(TreeBud& budFrom);

			/// Propagates resources from root back to buds.
			/// \param root Root of tree.
			void propagateUp(const TreeNode& root);

			/// Propagates resources from node upwards back to buds.
			/// \param node Representation of node.
			void propagateUp(const NodeUpInfo& node);

			/// Redistributes resources in node with terminal follower.
			/// \param node Redistribution node.
			/// \param resources Resources to be redistributed.
			/// \return Following terminal node resources to be redistributed.
			float redistribInnerNode(const TreeNode& node, float resources);

			/// Redistributes resources in node without terminal follower.
			/// \param node Redistribution node.
			/// \param resources Resources to be redistributed.
			void redistribTerminalEndpoint(const TreeNode& node, float resources);

			/// Redistributes resources in node back to its buds.
			/// \param node Redistribution node.
			/// \param resources Resources to be redistributed.
			/// \param denominator Denominator of redistribution equation.
			void distribToBuds(const TreeNode& node, float resources, float denominator) const;

			/// Redistributes resources in node into lateral branches that start in it.
			/// \param node Redistribution node.
			/// \param resources Resources to be redistributed.
			/// \param denominator Denominator of redistribution equation.
			void distribLatBranches(const TreeNode& node, float resources, float denominator);

			/// Gets resources of inner node.
			/// Equals to resources in following terminal node.
			/// \param node Inner node.
			/// \return Terminal resources of inner node.
			float getInnerNodeTerminalResources(const TreeNode& node);

			/// Calculates denominator of redistribution equation of node.
			/// \param terminalResources Node terminal resources.
			/// \param lateralResources Node lateral resources.
			/// \return Redistribution equation denominator.
			float getBhDenominator(float terminalResources, float lateralResources) const;
		};
	}
}


#endif // EG_EXT_BORCHERT_HONDA_H

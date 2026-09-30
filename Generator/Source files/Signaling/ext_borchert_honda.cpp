/// Redistribution of resources based on Extendet Borchert-Honda model.
/// \file ext_borchert_honda.cpp
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#include <stdafx.h>
#include <Signaling/ext_borchert_honda.h>

#include <Tree primitives/tree_primitives.h>

namespace Treegen
{
	namespace Signaling
	{
		ExtBorchertHondaModel::ExtBorchertHondaModel(const float biasToMain) {
			if (biasToMain < 0.f || biasToMain > 1.f) {
				throw std::invalid_argument{"Bias must be in range [0.0 - 1.0]!"};
			}
			mBiasToMain = biasToMain;
		}

		void ExtBorchertHondaModel::redistributeResources(const TreeNode& root,
		                                                  const std::vector<rwTreeBud>& buds) {
			// No resources, or redistribution would end up same as input values
			if (buds.empty() || mBiasToMain == 0.5f) { return; }

			for (auto&& bud : buds) { propagateToRoot(bud); }
			propagateUp(root);

			mResources.clear();
			mBranchBeginNodes = {};
		}

		void ExtBorchertHondaModel::propagateToRoot(TreeBud& budFrom) {
			TreeNode* currentNode = &budFrom.owner.get();
			mResources[*currentNode] += budFrom.resources;

			while (!currentNode->isRoot()) {
				currentNode = currentNode->predecessor;
				mResources[*currentNode] += budFrom.resources;
			}
		}

		void ExtBorchertHondaModel::propagateUp(const TreeNode& root) {
			if (mResources[root] == 0.f) { return; }
			mBranchBeginNodes.emplace(root, mResources[root]);

			while (!mBranchBeginNodes.empty()) {
				propagateUp(mBranchBeginNodes.front());
				mBranchBeginNodes.pop();
			}
		}

		void ExtBorchertHondaModel::propagateUp(const NodeUpInfo& node) {
			rwConstTreeNode current = node.node;
			float resources = node.resources;

			// While node has terminal follower (and may have lateral ones)
			while (!current.get().isTerminalEndpoint()) {
				resources = redistribInnerNode(current, resources);
				current = *current.get().terminalFollower;
				if (mResources.find(current) == mResources.end()) { return; }
			}

			// Node may have lateral followers (secondary branches)
			redistribTerminalEndpoint(current, resources);
		}

		float ExtBorchertHondaModel::redistribInnerNode(const TreeNode& node, const float resources) {
			assert(node.terminalFollower);
			const float terminalResources = getInnerNodeTerminalResources(node);
			const float bhDenominator = getBhDenominator(terminalResources, mResources[node] - terminalResources);

			distribToBuds(node, resources, bhDenominator);
			distribLatBranches(node, resources, bhDenominator);

			// Following terminal node resources
			return resources * (mBiasToMain * terminalResources / bhDenominator);
		}

		void ExtBorchertHondaModel::redistribTerminalEndpoint(const TreeNode& node, const float resources) {
			assert(node.isTerminalEndpoint());
			const float terminalResources = node.terminalBud ? node.terminalBud->resources : 0.f;
			const float bhDenominator = getBhDenominator(terminalResources, mResources[node] - terminalResources);

			distribToBuds(node, resources, bhDenominator);
			distribLatBranches(node, resources, bhDenominator);
		}

		void ExtBorchertHondaModel::distribToBuds(const TreeNode& node,
		                                          const float resources,
		                                          const float denominator) const {
			if (node.terminalBud) {
				float& terminalBudResources = node.terminalBud->resources;
				if (terminalBudResources != 0.f) {
					terminalBudResources = resources * (mBiasToMain * terminalBudResources / denominator);
				}
			}

			for (auto&& bud : node.lateralBuds) {
				if (bud->resources != 0.f) {
					bud->resources = resources * ((1.f - mBiasToMain) * bud->resources / denominator);
				}
			}
		}

		void ExtBorchertHondaModel::distribLatBranches(const TreeNode& node,
		                                               const float resources,
		                                               const float denominator) {
			for (auto&& lateralNode : node.lateralFollowers) {
				const auto nodeIt = mResources.find(lateralNode.get());
				if (nodeIt != std::end(mResources)) {
					mBranchBeginNodes.emplace(nodeIt->first,
						resources * ((1.f - mBiasToMain) * nodeIt->second) / denominator);
				}
			}
		}

		float ExtBorchertHondaModel::getInnerNodeTerminalResources(const TreeNode& node) {
			assert(node.terminalFollower);
			const auto find_it = mResources.find(*node.terminalFollower);
			if (find_it == std::end(mResources)) { return 0.f; }
			return find_it->second;
		}

		float ExtBorchertHondaModel::getBhDenominator(const float terminalResources,
		                                              const float lateralResources) const {
			return mBiasToMain * terminalResources + (1.f - mBiasToMain) * lateralResources;
		}
	}
}

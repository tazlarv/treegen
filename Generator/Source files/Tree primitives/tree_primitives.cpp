/// Basic classes for representation of tree structure.
/// \file tree_primitives.cpp
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#include <stdafx.h>
#include <Tree primitives/tree_primitives.h>

namespace Treegen
{
	// ===========================================================================================================
	// TreeBud
	// ===========================================================================================================

	TreeBud::TreeBud(const rwTreeNode owner, const BudType type,
	                 const glm::vec3& orientation, const unsigned seed) :
		owner{owner}, type{type}, orientationNormalized{glm::normalize(orientation)}, rndEngine{seed} {}

	void TreeBud::setFreeSpaceDir(const glm::vec3& direction) {
		assert(glm::length(direction) != 0.f);
		mSpaceDirNormalized = glm::normalize(direction);
	}

	glm::vec3 TreeBud::getFreeSpaceDirNormalized() const { return mSpaceDirNormalized; }

	// ===========================================================================================================
	// TreeNode
	// ===========================================================================================================

	TreeNode::TreeNode(const unsigned id, const glm::vec3& position, TreeBranch* branch) :
		id{id}, position{position}, branch{branch} {}

	bool TreeNode::isRoot() const { return !predecessor; }
	bool TreeNode::isEndpoint() const { return !terminalFollower && lateralFollowers.empty(); }
	bool TreeNode::isTerminalEndpoint() const { return !terminalFollower; }
	bool TreeNode::isBranchingNode() const { return followingNodesCount() > 1; }
	bool TreeNode::isInnerNode() const { return followingNodesCount() == 1; }

	size_t TreeNode::budsCount() const { return terminalBud ? 1 + lateralBuds.size() : lateralBuds.size(); }

	size_t TreeNode::followingNodesCount() const {
		return terminalFollower ? 1 + lateralFollowers.size() : lateralFollowers.size();
	}

	std::vector<rwTreeBud> TreeNode::getBuds() {
		std::vector<rwTreeBud> buds{};
		if (terminalBud) { buds.push_back(*terminalBud); }
		for (auto&& lateralBud : lateralBuds) { buds.push_back(*lateralBud); }
		return buds;
	}

	std::vector<rwTreeNode> TreeNode::getFollowingNodes() {
		std::vector<rwTreeNode> followingNodes{};
		if (terminalFollower) { followingNodes.push_back(*terminalFollower); }
		for (auto&& lateralFollower : lateralFollowers) { followingNodes.push_back(lateralFollower); }
		return followingNodes;
	}

	std::ostream& operator<<(std::ostream& stream, const TreeNode& node) {
		return stream
		       << "Tree node:    ID: " << node.id
		       << "    Position: " << GlmUtils::printVec3D<glm::vec3>(node.position);
	}

	// ===========================================================================================================
	// TreeBranch
	// ===========================================================================================================

	TreeBranch::TreeBranch(const unsigned id, rwTreeNode baseNode, rwTreeNode tipNode) :
		id(id), baseNode(baseNode), tipNode(tipNode) {

		tipNode.get().branch = this;
		nodes.push_back(baseNode);

		if (baseNode != tipNode) {
			nodes.push_back(tipNode);
			TreeBranch* baseBranch = baseNode.get().branch;
			precedingBranch = baseBranch;

			// Potentially there may be multiple new branches growing at the same time.
			std::lock_guard<std::mutex> lock{baseBranch->branchMutex};
			baseBranch->followingBranches.push_back(*this);
		}
	}

	void TreeBranch::growWith(rwTreeNode newTip) {
		assert(newTip == *tipNode.get().terminalFollower);
		tipNode = newTip;
		tipNode.get().branch = this;
		nodes.push_back(newTip);
	}

	bool TreeBranch::isTrunk() const { return !precedingBranch; }
}

namespace std
{
	size_t hash<Treegen::TreeBud>::operator()(const Treegen::TreeBud& key) const {
		size_t seed = hash<glm::vec3>{}(key.orientationNormalized);
		Treegen::Utils::hashCombine(seed, key.owner.get().id);
		return seed;
	}

	size_t hash<Treegen::TreeNode>::operator()(const Treegen::TreeNode& key) const {
		return key.id;
	}

	size_t hash<Treegen::TreeBranch>::operator()(const Treegen::TreeBranch& key) const {
		return key.id;
	}
}

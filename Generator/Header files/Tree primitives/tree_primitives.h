/// Basic classes for representation of tree structure.
/// \file tree_primitives.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#ifndef TREE_PRIMITIVES_H
#define TREE_PRIMITIVES_H

#include <Utilities/glm_utils.h>

#include <glm/glm.hpp>

#include <vector>
#include <ostream>
#include <memory>
#include <random>
#include <type_traits>

#include <mutex>

namespace Treegen
{
	struct TreeBranch;
	struct TreeNode;
	class TreeBud;

	using rwTreeBranch = std::reference_wrapper<TreeBranch>;
	using rwTreeNode = std::reference_wrapper<TreeNode>;
	using rwTreeBud = std::reference_wrapper<TreeBud>;
	using rwConstTreeBranch = std::reference_wrapper<const TreeBranch>;
	using rwConstTreeNode = std::reference_wrapper<const TreeNode>;
	using rwConstTreeBud = std::reference_wrapper<const TreeBud>;

	enum class BudType {
		Terminal,
		Lateral
	};

	enum class BudStatus {
		Active,
		NoLight,
		NoSpace,
		Shed,
		MadeBranch,
		SceneOrBoundsCollision,
	};

	enum class BranchStatus {
		Alive,
		Shed
	};

	// ===========================================================================================================
	// TreeBud
	// ===========================================================================================================

	/// Representation of tree bud.
	class TreeBud {
		glm::vec3 mSpaceDirNormalized = {0.f, 0.f, 0.f};

	public:
		/// Constructs new TreeBud object.
		/// \param owner TreeNode in which is bud made.
		/// \param type Type of bud.
		/// \param orientation Orientation of bud.
		/// \param seed Seed of random generator of bud.
		TreeBud(rwTreeNode owner, BudType type, const glm::vec3& orientation, unsigned seed);

		/// \name Base bud properties.
		///@{
		const rwTreeNode owner;
		const BudType type;
		const glm::vec3 orientationNormalized;
		///@}

		std::minstd_rand rndEngine; ///< Bud random generator.

		/// \name Growth calculation variables.
		///@{
		BudStatus status = BudStatus::Active;
		float resources = 0.f;
		int shootLength = 0;
		///@}

		/// Sets direction towards free space.
		/// \param direction Normalizable direction towards free space.
		/// \warning Method may try to divide by zero exception if length of \p direction is zero.
		void setFreeSpaceDir(const glm::vec3& direction);
		
		/// Gets normalized direction towards free space.
		/// \return Normalized direction towards free space.
		/// \warning Direction must be at least once set by setFreeSpaceDir otherwise returns zero vector.
		glm::vec3 getFreeSpaceDirNormalized() const;
	};

	inline bool operator==(const rwTreeBud op1, const rwTreeBud op2) { return &op1.get() == &op2.get(); }
	inline bool operator!=(const rwTreeBud op1, const rwTreeBud op2) { return !(op1 == op2); }

	// ===========================================================================================================
	// TreeNode
	// ===========================================================================================================

	/// Representation of node of tree structure.
	struct TreeNode {
		/// Constructs new TreeNode object.
		/// \param id Identifier of node.
		/// \param position Position of node in world.
		/// \param branch TreeBranch of which node is part of.
		explicit TreeNode(unsigned id, const glm::vec3& position, TreeBranch* branch = nullptr);

		std::mutex nodeMutex{};	///< Node mutex used for synchronization when necessary.

		const unsigned id;
		bool shed = false;
		glm::vec3 position;

		/// \name Pointers for traversal of tree structure from this node.
		///@{
		TreeNode* predecessor = nullptr;
		TreeNode* terminalFollower = nullptr;
		std::vector<rwTreeNode> lateralFollowers{};
		TreeBranch* branch;
		///@}

		/// \name Node buds.
		///@{
		std::unique_ptr<TreeBud> terminalBud = nullptr;
		std::vector<std::unique_ptr<TreeBud>> lateralBuds{};
		///@}

		/// \name Node type checks.
		///@{
		/// Checks if node is root of tree (it does not have predecessor).
		bool isRoot() const;

		/// Checks if node does not have any following node (there is neither terminal follower nor any lateral ones).
		bool isEndpoint() const;
		
		/// Checks if node does not have terminal follower.
		bool isTerminalEndpoint() const;
		
		/// Checks if node has at least 2 followers (terminal + lateral, or only lateral) => if it is branching node.
		bool isBranchingNode() const;

		/// Checks if node has exactly one follower.
		bool isInnerNode() const;
		///@}

		/// Gets number of buds in node.
		size_t budsCount() const;

		/// Gets all buds in node.
		std::vector<rwTreeBud> getBuds();

		/// Gets number of following nodes.
		size_t followingNodesCount() const;

		/// Gets all following nodes.
		std::vector<rwTreeNode> getFollowingNodes();

		friend std::ostream& operator<<(std::ostream& stream, const TreeNode& node);
	};

	inline bool operator==(const TreeNode& op1, const TreeNode& op2) { return op1.id == op2.id; }
	inline bool operator!=(const TreeNode& op1, const TreeNode& op2) { return !(op1 == op2); }
	inline bool operator<(const TreeNode& op1, const TreeNode& op2) { return op1.id < op2.id; }

	// ===========================================================================================================
	// TreeBranch
	// ===========================================================================================================

	/// Representation of tree branch made up from TreeNodes.
	struct TreeBranch {	
		/// Constructs new TreeNode object.
		/// \param id Identifier of branch.
		/// \param baseNode Branching node or root of tree.
		/// \param tipNode First lateral node from branching node (\p baseNode) or equal to \p baseNode for trunk.
		/// \warning This branch is assigned as branch of \p tipNode and as following branch of \p baseNode branch.
		TreeBranch(unsigned id, rwTreeNode baseNode, rwTreeNode tipNode);

		std::mutex branchMutex{}; ///< Branch mutex used for synchronization when necessary.

		const unsigned id;
		BranchStatus status = BranchStatus::Alive;
		
		/// \name Pointers for traversal of tree structure from nodes of this branch.
		///@{
		rwTreeNode baseNode;
		rwTreeNode tipNode;
		std::vector<rwTreeNode> nodes{}; ///< All branch nodes in order from base to tip.

		TreeBranch* precedingBranch = nullptr;
		std::vector<rwTreeBranch> followingBranches{};
		///@}

		// 

		/// Grows branch by one internode.
		/// \param newTip Terminal follower of current tipNode of this branch.
		void growWith(rwTreeNode newTip);

		/// Checks if branch is trunk of tree.
		bool isTrunk() const;
	};

	inline bool operator==(const TreeBranch& op1, const TreeBranch& op2) { return op1.id == op2.id; }
	inline bool operator!=(const TreeBranch& op1, const TreeBranch& op2) { return !(op1 == op2); }
}

namespace std
{
	/// std::hash overload for TreeBud.
	template <>
	struct hash<Treegen::TreeBud> {
		/// Hash method of TreeBud.
		/// \param key TreeBud to be hashed.
		/// \returns Hash value of key.
		size_t operator()(const Treegen::TreeBud& key) const;
	};

	/// std::hash overload for TreeNode.
	template <>
	struct hash<Treegen::TreeNode> {
		/// Hash method of TreeNode.
		/// \param key TreeNode to be hashed.
		/// \returns Hash value of key.
		size_t operator()(const Treegen::TreeNode& key) const;
	};

	/// std::hash overload for TreeBranch.
	template <>
	struct hash<Treegen::TreeBranch> {
		/// Hash method of TreeBranch.
		/// \param key TreeBranch to be hashed.
		/// \returns Hash value of key.
		size_t operator()(const Treegen::TreeBranch& key) const;
	};
}

#endif // TREE_PRIMITIVES_H

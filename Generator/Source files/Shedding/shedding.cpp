/// Shedding of branches.
/// \file shedding.cpp
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#include <stdafx.h>
#include <Shedding/shedding.h>

#include <Tree primitives/tree_primitives.h>
#include <Shadow propagation/shadow_propagation.h>

namespace Treegen
{
	namespace Shedding
	{
		Shedding::Shedding(std::shared_ptr<ShadowProp::ShadowPropagation> shadowPropagation) :
			mShadowProp{std::move(shadowPropagation)} {}

		void Shedding::sheddingThreshold(const float threshold) {
			mSheddingThreshold = threshold;
		}

		void Shedding::shedBranch(TreeBranch& branch) {
			assert(!branch.isTrunk());

			auto& branchProperties = mBranchAxesProperties.find(branch)->second;
			branchProperties.nodesCnt += int(branch.nodes.size());

			// Base node is branching point (member of different branch)
			// This method is not called on trunk -> for all branches its proper setup
			auto topNode = branch.tipNode;
			while (topNode != branch.baseNode) {
				branchProperties.resources += mShadowProp->getResources(topNode.get().position);
				topNode = *topNode.get().predecessor;
			}

			if (branchProperties.resources / branchProperties.nodesCnt <= mSheddingThreshold) { dropBranch(branch); }

			if (branch.precedingBranch) { // => is lateral branch of some preceding => which should have its own branch
				auto& precedingBranch = *branch.precedingBranch;
				auto& precedingProperties = mBranchAxesProperties.find(precedingBranch)->second;

				if (--precedingProperties.followingBranchesCnt == 0) { mBranchesToTest.push(precedingBranch); }

				precedingProperties.nodesCnt += branchProperties.nodesCnt;
				precedingProperties.resources += branchProperties.resources;
			}
		}

		void Shedding::dropBranch(TreeBranch& branch) {
			std::stack<rwTreeBranch> toShed{};
			toShed.push(branch);

			while (!toShed.empty()) {
				TreeBranch& shedBranch = toShed.top();
				toShed.pop();

				mShedBranches.insert(shedBranch);

				for (auto&& followingBranch : shedBranch.followingBranches) {
					// Branch is alive and not yet marked for shedding
					if (followingBranch.get().status == BranchStatus::Alive && mShedBranches.count(followingBranch) == 0) {
						toShed.push(followingBranch);
					}
				}
			}
		}

		void Shedding::clear() {
			mShedBranches.clear();
			mBranchAxesProperties.clear();
		}
	}
}

/// Shedding of branches.
/// \file shedding.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#ifndef SHEDDING_H
#define SHEDDING_H

#include <Tree primitives/tree_primitives.h>
#include <Shadow propagation/shadow_propagation.h>

#include <memory>
#include <stack>
#include <unordered_set>
#include <unordered_map>

namespace Treegen
{
	namespace Shedding
	{
		/// Shedding of branches.
		class Shedding {

			/// Relevant data for shedding in axis of tree structure.
			struct BranchAxisProperties {
				BranchAxisProperties() = default;
				int nodesCnt = 0;
				int followingBranchesCnt = 0;	///< Counter of visits of axis from following branches (synchronization).
				float resources = 0.f;
			};

			float mSheddingThreshold = 0.f;
			std::shared_ptr<ShadowProp::ShadowPropagation> mShadowProp;

			/// \name Algorithm run data structures.
			///@{
			std::unordered_set<rwTreeBranch, std::hash<TreeBranch>> mShedBranches{};
			std::stack<rwTreeBranch> mBranchesToTest{};
			std::unordered_map<rwTreeBranch, BranchAxisProperties, std::hash<TreeBranch>> mBranchAxesProperties{};
			///@}

		public:

			/// Constructs new object capable of shedding branches of tree.
			/// \param shadowPropagation Light model for resources availability queries.
			explicit Shedding(std::shared_ptr<ShadowProp::ShadowPropagation> shadowPropagation);

			/// Sets shedding threshold.
			/// \param threshold Shedding threshold value.
			void sheddingThreshold(float threshold);
			float sheddingThreshold() const { return mSheddingThreshold; }

			/// Applies shedding in tree structure from given branches downwards .
			/// \tparam TIterator Iterator of TreeBranch.
			/// \param begin Begin iterator.
			/// \param end End iterator.
			/// \return Branches that should be shed.
			template <typename TIterator>
			std::vector<rwTreeBranch> shedBranches(TIterator begin, TIterator end);

		private:
			/// Tests branch if it should be shed.
			/// \param branch Branch to test.
			void shedBranch(TreeBranch& branch);

			/// Marks branch that should be shed and propagates information downwards for even bigger possible shedding.
			/// \param branch Branch to mark.
			void dropBranch(TreeBranch& branch);

			/// Clears all data structures used for run of algorithm.
			void clear();
		};

		template <typename TIterator>
		std::vector<rwTreeBranch> Shedding::shedBranches(TIterator begin, TIterator end) {
			for (auto it = begin; it != end; ++it) {
				TreeBranch& branch = *it; // Allows iterators of reference wrappers of TreeBranches

				const auto aliveFollowingCnt = std::count_if(
					std::begin(branch.followingBranches), std::end(branch.followingBranches), [](TreeBranch& branch) {
						return branch.status == BranchStatus::Alive;
					});

				if (aliveFollowingCnt == 0) { mBranchesToTest.push(branch); }
				mBranchAxesProperties[branch].followingBranchesCnt = int(aliveFollowingCnt);
			}

			while (!mBranchesToTest.empty()) {
				TreeBranch& branch = mBranchesToTest.top();
				mBranchesToTest.pop();
				if (!branch.isTrunk()) { shedBranch(branch); } // Non trunk branch
			}

			std::vector<rwTreeBranch> result(mShedBranches.begin(), mShedBranches.end());
			clear();
			return result;
		}
	}
}

#endif // SHEDDING_H

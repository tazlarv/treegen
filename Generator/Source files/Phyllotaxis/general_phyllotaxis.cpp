/// Implementation of opposite and alternate type of phyllotaxis.
/// \file general_phyllotaxis.cpp
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#include <stdafx.h>
#include <Phyllotaxis/general_phyllotaxis.h>

namespace Treegen
{
	namespace Phyllotaxis
	{
		size_t GeneralPhyllotaxis::lateralBudsCount() const {
			return mType == Type::Alternate ? 1 : 2;
		}

		void GeneralPhyllotaxis::generateRootBuds(TreeNode& root, const unsigned seed) {
			assert(root.isRoot());
			root.terminalBud = std::make_unique<TreeBud>(root, BudType::Terminal, Const::WORLD_UP, seed);
			mNodePhyllotacticVec.emplace(
				root, Utils::getRndOrthogonalNormalized(Const::WORLD_UP, root.terminalBud->rndEngine));
		}

		void GeneralPhyllotaxis::generateInnerLateralBuds(TreeNode& target, TreeBud& from) {
			// Inner node must have predecessor with bud that is equal to from
			// Branches must be defined and phyllotaxis must have been called on predecessor
			assert(target.predecessor && from.owner.get() == *target.predecessor);
			assert(target.branch && target.predecessor->branch);
			assert(mNodePhyllotacticVec.count(*target.predecessor) == 1);

			TreeNode& predecessor = *target.predecessor;
			auto& rndEngine = from.rndEngine;

			const auto axisNormalized = glm::normalize(target.position - target.predecessor->position);
			const glm::vec3 phylloOrientation = [&]() {
				// Accomodate bending of same branch (rotate previous phyllo. orientation by same angle as bending)
				if (target.branch == predecessor.branch) {
					return Utils::rotateVecBasedOnChangedOrientation(
						predecessor.terminalBud.get()->orientationNormalized,
						axisNormalized, mNodePhyllotacticVec[predecessor]);
				} // New branch has new (rnd) starting phyllo. orientation
				else { return Utils::getRndOrthogonalNormalized(axisNormalized, rndEngine); }
			}();

			// Rotation by phyllotactic angle
			const glm::vec3 targetPhylloOrientation =
				GlmUtils::rotateAroundAxis(phylloOrientation, mPhyllotacticAngle, axisNormalized);
			mNodePhyllotacticVec.emplace(target, targetPhylloOrientation);

			// Generation of buds with defined lateral angle
			const glm::vec3 targetOrthPart = targetPhylloOrientation * mLateralAngleTan;
			target.lateralBuds.push_back(std::make_unique<TreeBud>
				(target, BudType::Lateral, axisNormalized + targetOrthPart, rndEngine()));

			if (mType == Type::Opposite) {
				target.lateralBuds.push_back(std::make_unique<TreeBud>
					(target, BudType::Lateral, axisNormalized - targetOrthPart, rndEngine()));
			}
		}

		void GeneralPhyllotaxis::clear() {
			mNodePhyllotacticVec.clear();
		}
	}
}

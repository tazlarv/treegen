/// Abstract base class for setting buds in nodes of tree (phyllotaxis).
/// \file abstract_phyllotaxis.cpp
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#include <stdafx.h>
#include <Phyllotaxis/abstract_phyllotaxis.h>

#include <Tree primitives/tree_primitives.h>

#include <memory>
#include <stdexcept>

namespace Treegen
{
	namespace Phyllotaxis
	{
		AbstractPhyllotaxis::AbstractPhyllotaxis() {
			AbstractPhyllotaxis::phyllotacticAngle(glm::radians(137.5f));
			AbstractPhyllotaxis::lateralAngle(glm::radians(60.f));
		}

		void AbstractPhyllotaxis::phyllotacticAngle(const float angleRadians) {
			if (angleRadians < 0.f || angleRadians > glm::radians(360.f)) {
				throw std::invalid_argument{"Phyllotactic angle outside [0.0-360.0] degrees range!"};
			}
			mPhyllotacticAngle = angleRadians;
		}

		void AbstractPhyllotaxis::lateralAngle(const float angleRadians) {
			if (angleRadians <= 0.f || angleRadians > glm::radians(90.f)) {
				throw std::invalid_argument{"Lateral bud angle outside (0.0-90.0] degrees range!"};
			}
			mLateralAngle = angleRadians;
			mLateralAngleTan = std::tan(angleRadians);
		}

		void AbstractPhyllotaxis::generateInnerBuds(TreeNode& target, TreeBud& from) {
			assert(target.predecessor);
			generateInnerTerminalBud(target, from);
			generateInnerLateralBuds(target, from);
		}

		void AbstractPhyllotaxis::generateInnerTerminalBud(TreeNode& target, TreeBud& from) {
			assert(target.predecessor);
			const auto budSeed = from.rndEngine();
			const auto orientation = target.position - target.predecessor->position;
			target.terminalBud = std::make_unique<TreeBud>(target, BudType::Terminal, orientation, budSeed);
		}
	}
}

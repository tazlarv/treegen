/// Utilities used in Generator project.
/// \file treegen_utils.cpp
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#include <stdafx.h>
#include <Utilities/treegen_utils.h>

namespace Treegen
{
	namespace Utils
	{
		unsigned getRndSeed() {
			std::random_device rndDevice{};
			if (rndDevice.entropy() != 0.0) { return rndDevice(); }
			return unsigned(std::chrono::high_resolution_clock::now().time_since_epoch().count());
		}

		glm::vec3 rotateVecBasedOnChangedOrientation(const glm::vec3& startOrientation,
		                                             const glm::vec3& finalOrientation,
		                                             const glm::vec3& rotateVec) {

			const auto cosAngle = glm::dot(startOrientation, finalOrientation);
			if (std::abs(cosAngle) > Const::COS_1_DEG) {
				if (cosAngle > 0.f) { return rotateVec; } // 0 deg. => parallel
				return -rotateVec; // 180 deg. => opposite (for our usage should never happen)
			}

			const auto rotationAxis = glm::cross(startOrientation, finalOrientation);
			const auto sinAngle = glm::length(rotationAxis);
			const auto angle = atan2(sinAngle, cosAngle);
			return GlmUtils::rotateAroundAxis(rotateVec, angle, rotationAxis);
		}
	}
}

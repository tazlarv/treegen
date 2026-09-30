/// Scene camera based on euler angles and rotation around lookAt point.
/// \file scene_camera.cpp
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#include <stdafx.h>
#include <Camera/scene_camera.h>

namespace Camera
{
	float SceneCamera::movSpeedFromLookupDist(const float lookAtDist) {
		return lookAtDist * 0.001f;
	}

	void SceneCamera::handleMouseMovement(const float xOffset, const float yOffset) {
		const auto inversePosition = position + front * lookAtDistance;

		yaw += xOffset * mouseSensitivity;
		pitch -= yOffset * mouseSensitivity;

		// Limit pitch so there wont be flipping of view when looking up/down
		if (pitch > 89.f) { pitch = 89.f; }
		else if (pitch < -89.f) { pitch = -89.f; }

		// Update camera vectors based on changes
		updateCameraVectors();

		// Update position (inverse to change of direction of camera (we are rotating on sphere))
		position = inversePosition - front * lookAtDistance;
	}

	void SceneCamera::handleMouseScroll(const float yOffset) {
		const auto oldLookupDistance = lookAtDistance;
		lookAtDistance = glm::clamp(lookAtDistance - yOffset * scrollSensitivity * lookAtDistance,
			minLookAtDistance, maxLookAtDistance);

		position = position - front * (lookAtDistance - oldLookupDistance);
		movementSpeed = glm::clamp(movSpeedFromLookupDist(lookAtDistance), minMovementSpeed, maxMovementSpeed);
	}
}

/// Euler camera interface and supporting variables and data structures.
/// \file camera_utils.cpp
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#include <stdafx.h>
#include <Camera/camera_utils.h>

namespace Camera
{
	glm::mat4 IEulerCamera::getViewMatrix() const {
		return glm::lookAt(position, position + front * lookAtDistance, up);
	}

	void IEulerCamera::updateCameraVectors() {
		// All vectors normalised so they can be used for movement of camera
		// Calculate front vector based on pitch and yaw
		front.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
		front.y = sin(glm::radians(pitch));
		front.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
		front = glm::normalize(front);

		// Calculate right and up vectors
		right = glm::normalize(glm::cross(front, worldUp));
		up = glm::normalize(glm::cross(right, front));
	}

	void IEulerCamera::handleCameraMovement(const CameraMovement direction, const float time) {
		const float velocity = movementSpeed * time;

		switch (direction) {
			case CameraMovement::Forward: position += front * velocity;
				break;
			case CameraMovement::Backward: position -= front * velocity;
				break;
			case CameraMovement::Left: position -= right * velocity;
				break;
			case CameraMovement::Right: position += right * velocity;
				break;
			case CameraMovement::Up: position += up * velocity;
				break;
			case CameraMovement::Down: position -= up * velocity;
				break;
		}
	}
}

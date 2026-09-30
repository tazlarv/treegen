/// Euler camera interface, supporting variables and data structures.
/// \file camera_utils.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#ifndef CAMERA_UTILS_H
#define CAMERA_UTILS_H

#include <glm/glm.hpp>

namespace Camera
{
	/// Possible directions of movement of camera.
	enum class CameraMovement {
		Forward,
		Backward,
		Left,
		Right,
		Up,
		Down
	};

	/// \name Default world properties.
	///@{ 
	const glm::vec3 AXES_ORIGIN = {0.f, 0.f, 0.f};
	const glm::vec3 WORLD_UP = {0.f, 1.f, 0.f};
	///@}

	/// \name Default camera properties.
	///@{ 
	// Looking in direction of z axis (from + to - (+ is at viewer position))

	constexpr float DEF_YAW = -90.f;
	constexpr float DEF_PITCH = 0.f;
	constexpr float DEF_ROLL = 0.f;

	constexpr float DEF_FOV_ANGLE = 45.f;
	constexpr float DEF_MIN_FOV_ANGLE = 1.f;
	constexpr float DEF_MAX_FOV_ANGLE = 45.f;
	///@}

	/// Euler camera base class.
	class IEulerCamera {
	public:
		IEulerCamera() { IEulerCamera::updateCameraVectors(); }
		IEulerCamera(const IEulerCamera&) = default;
		IEulerCamera& operator=(const IEulerCamera&) = default;
		IEulerCamera(IEulerCamera&&) = default;
		IEulerCamera& operator=(IEulerCamera&&) = default;
		virtual ~IEulerCamera() = default;

		/// \name Camera properties.
		///@{
		// World properties.
		glm::vec3 position = AXES_ORIGIN;
		glm::vec3 worldUp = WORLD_UP;

		// LookAt distance
		float lookAtDistance = 1.f;
		float minLookAtDistance = 1.f;
		float maxLookAtDistance = 1.f;

		// Euler angles => orientation
		float yaw = DEF_YAW;
		float pitch = DEF_PITCH;
		float roll = DEF_ROLL;

		// Camera movements sensitivity
		float movementSpeed = 1.f;
		float minMovementSpeed = 1.f;
		float maxMovementSpeed = 1.f;

		float mouseSensitivity = 1.f;
		float scrollSensitivity = 1.f;
		
		// Helper vairables (if camera changes fov)
		float fieldOfViewAngle = DEF_FOV_ANGLE;
		float minFovAngle = DEF_MIN_FOV_ANGLE;
		float maxFovAngle = DEF_MAX_FOV_ANGLE;
		///@}

		/// Gets view matrix of camera.
		/// \return View matrix of camera.
		virtual glm::mat4 getViewMatrix() const;

		/// Calculates camera vectors from pitch and yaw.
		virtual void updateCameraVectors();

		/// Handles movement of camera.
		/// \param direction Direction of movement.
		/// \param time Duration of movement.
		virtual void handleCameraMovement(CameraMovement direction, float time);

		/// Handles movement of mouse - turning of camera by offsets of x and y.
		/// \param xOffset Mouse x offset.
		/// \param yOffset Mouse y offset.
		virtual void handleMouseMovement(float xOffset, float yOffset) = 0;

		/// Handles input from mouse scroll.
		/// \param yOffset Mouse scroll rotation offset.
		virtual void handleMouseScroll(float yOffset) = 0;

	protected:
		glm::vec3 front = {};
		glm::vec3 up = {};
		glm::vec3 right = {};
	};
}

#endif // CAMERA_UTILS_H

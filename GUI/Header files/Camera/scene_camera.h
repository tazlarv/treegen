/// Scene camera based on euler angles and rotation around lookAt point.
/// \file scene_camera.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#ifndef SCENE_CAMERA_H
#define SCENE_CAMERA_H

#include <Camera/camera_utils.h>

namespace Camera
{
	/// Scene camera based on euler angles and rotation around lookAt point.
	class SceneCamera : public IEulerCamera {
	public:
		SceneCamera() = default;

		/// Calculates camera movement speed based on lookAt distance.
		/// \param lookAtDist Camera lookAt distance.
		/// \return Camera movement speed.
		static float movSpeedFromLookupDist(float lookAtDist);

		// Handles movement of mouse - turning of camera by offsets of x and y
		/// Handles movement of mouse - turning of camera by offsets of x and y.
		/// \param xOffset Mouse x offset.
		/// \param yOffset Mouse y offset.
		void handleMouseMovement(float xOffset, float yOffset) override;

		/// Handles input from mouse scroll (changes distance to look at point).
		/// \param yOffset Mouse scroll rotation offset.
		void handleMouseScroll(float yOffset) override;
	};
}


#endif // SCENE_CAMERA_H

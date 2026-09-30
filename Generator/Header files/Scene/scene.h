/// Scene representation.
/// \file scene.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#ifndef SCENE_H
#define SCENE_H

#include <Utilities/constants.h>
#include <embree3/rtcore.h>

#include <tuple>

namespace Treegen
{
	namespace Scene
	{
		/// Scene representation potentially bounded in size by data structures in rest of simulation (grid sizes).
		class BoundedScene {

			/// \name Scene bounds.
			/// Because calculations use grid, our scene model bounds are in non negative ranges [0-MAX, 0-MAX, 0-MAX].
			///@{
			constexpr static float mBoundingBoxSide = Const::BOUNDING_BOX_SIDE;
			constexpr static glm::vec3 mLowerBoundingBoxModel = {0.f, 0.f, 0.f};
			constexpr static glm::vec3 mUpperBoundingBoxModel = Const::BOUNDING_BOX_SIDES;
			constexpr static glm::vec3 mBoundingBoxCenterBase = Const::BOUNDING_BOX_CENTER_BASE;
			///@}

			/// \name Loaded scene properties.
			///@{
			float mSceneModelRatio = 1.f;
			glm::vec3 mSceneBasePos{};
			glm::vec3 mLowerScene{};
			glm::vec3 mUpperScene{};
			float mGroundZeroY = 0.f;
			///@}

			/// \name Bounding box in scene coordinates.
			///@{
			bool mBoundingBoxDefined = false;
			glm::vec3 mLowerBoundingBoxScene{};
			glm::vec3 mUpperBoundingBoxScene{};
			///@}

			/// \name Embree ray-caster variables.
			///@{
			RTCDevice mRtcDevice = nullptr;
			RTCScene mRtcScene = nullptr;
			RTCGeometry mRtcGeometry = nullptr;
			unsigned mRtcGeometryId = 0;
			///@}

		public:
			BoundedScene();
			BoundedScene(const BoundedScene&) = delete;
			BoundedScene& operator=(const BoundedScene&) = delete;
			BoundedScene(BoundedScene&&) = delete;
			BoundedScene& operator=(BoundedScene&&) = delete;
			~BoundedScene();

			// ===========================================================================================================
			// Communication with interface and library user
			// ===========================================================================================================

			/// Sets scene - model ratio.
			/// \param sceneModelRatio Positive value representing new ratio.
			/// \warning For invalid argument throws std::invalid_argument exception.
			void sceneModelRatio(float sceneModelRatio);
			float sceneModelRatio() const { return mSceneModelRatio; }

			/// Checks if bounds in scene are defined.
			bool boundingBoxDefined() const { return mBoundingBoxDefined; }

			/// Gets bounds of loaded scene.
			/// \return Minimal and maximal coordinates of bound of sceme.
			std::tuple<glm::vec3, glm::vec3> sceneBounds() const;

			/// Gets bounds of grid used in algorithm in scene coordinates.
			/// \return Minimal and maximal coordinates of algorithm bounding box in scene coordinates.
			/// \warning Returned values are valid only if \p boundingBoxDefined returns true.
			std::tuple<glm::vec3, glm::vec3> boundingBoxSceneBounds() const;

			/// Tries to derive position for root of tree based on intersection of a ray with loaded scene.
			/// \param scenePos Position from which the ray is casted.
			/// \param direction Direction of ray.
			/// \param distance Maximum distance to which ray may travel.
			/// \return Result of ray casting. Bool represents if value in vector can be used as root of a tree.
			std::tuple<bool, glm::vec3> tryGetRootPosition(const glm::vec3& scenePos, const glm::vec3& direction,
			                                               float distance);

			/// Loads triangle mesh geometry of new scene. Deletes representation of previous scene.
			/// \param verticesPos Vertices of scene mesh.
			/// \param triangleIndices Triangles of scene mesh.
			void loadSceneGeometry(const std::vector<float>& verticesPos,
			                       const std::vector<unsigned>& triangleIndices);

			/// Loads empty scene. Deletes representation of previous scene.
			void defaultEmptySceneGeometry();

			// ===========================================================================================================
			// Internal methods for treegen algorithm
			// ===========================================================================================================

			/// Turns off existence of internal bounding box in scene.
			void noBoundingBox()  { mBoundingBoxDefined = false; }
			
			/// Sets scene base position. Should be position of first tree in scene.
			/// Defines position of internal bounding box in scene.
			/// \param sceneBasePos Position of first added tree into scene.
			void sceneBasePos(const glm::vec3& sceneBasePos);

			/// Gets scene base position on which is based internal bounding box position in scene.
			/// \return Scene base position. Equal to position of first tree in scene.
			glm::vec3 sceneBasePos() const { return mSceneBasePos; }

			/// Transforms position from model coordinates into scene coordinates.
			/// \param modelPos Model coordinates.
			/// \return Equivalent scene coordinates.
			glm::vec3 getSceneFromModelPos(const glm::vec3& modelPos);

			/// Transforms position from scene coordinates into model coordinates.
			/// \param scenePos Scene coordinates.
			/// \return Equivalent model coordinates.
			glm::vec3 getModelFromScenePos(const glm::vec3& scenePos);

			/// \name Tree generator occlusion and bounding box bounds tests.
			///@{

			/// Tests if movement from position in direction is possible (if there is no obstacle or bounds).
			/// \param fromModelPosition Starting position of movement in model coordinates.
			/// \param direction Direction of movement.
			/// \param modelDistance Distance of movement.
			/// \return True if movement is possible.
			bool isValidDirection(const glm::vec3& fromModelPosition, const glm::vec3& direction, float modelDistance);
			
			/// Tests if movement from starting position to target position is possible (if there is no obstacle or bounds).
			/// \param fromModelPosition Starting position in model coordinates.
			/// \param toModelPosition Target position in model coordinates.
			/// \return True if movement is possible.
			bool isValidDirection(const glm::vec3& fromModelPosition, const glm::vec3& toModelPosition);
			
			/// Tests if position is in bounding box.
			/// \param modelPosition Position in model coordinates to be tested.
			/// \return True if position is inside of bounding box.
			bool isInBoundingBox(const glm::vec3& modelPosition) const;
			
			/// Tests if movement from position in direction is not obstructed by object in scene.
			/// \param fromModelPosition Starting position of movement in model coordinates.
			/// \param direction Direction of movement.
			/// \param modelDistance Distance of movement.
			/// \return True if movement is obstructed.
			bool intersectsWithScene(const glm::vec3& fromModelPosition, const glm::vec3& direction, float modelDistance);
			
			/// Tests if movement from starting position to target position is not obstructed by object in scene.
			/// \param fromModelPosition Starting position in model coordinates.
			/// \param toModelPosition Target position in model coordinates.
			/// \return True if movement is obstructed.
			bool intersectsWithScene(const glm::vec3& fromModelPosition, const glm::vec3& toModelPosition);
			///@}

		private:
			/// Sets properties for empty scene without trees.
			void setDefaultSceneProperties();

			/// Initializes embree library objects.
			void initializeEmbree();

			/// Prepares embree ray for its casting.
			/// \param ray Ray to be set.
			/// \param from Ray source (starting) position.
			/// \param dir Ray direction.
			/// \param distance Distance into which is ray casted.
			void prepareRtcRay(RTCRay& ray, const glm::vec3& from, const glm::vec3& dir, float distance) const;
		};
	}
}


#endif // SCENE_H

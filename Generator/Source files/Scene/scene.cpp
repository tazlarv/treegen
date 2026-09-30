/// Scene representation.
/// \file scene.cpp
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#include <stdafx.h>
#include <Scene/scene.h>

#include <embree3/rtcore.h>

namespace Treegen
{
	namespace Scene
	{
		BoundedScene::BoundedScene() {
			initializeEmbree();
			setDefaultSceneProperties();
		}

		BoundedScene::~BoundedScene() {
			if (mRtcGeometry) { rtcReleaseGeometry(mRtcGeometry); }
			if (mRtcScene) { rtcReleaseScene(mRtcScene); }
			if (mRtcDevice) { rtcReleaseDevice(mRtcDevice); }
		}

		// ===========================================================================================================
		// Communication with interface and library user
		// ===========================================================================================================

		void BoundedScene::sceneModelRatio(const float sceneModelRatio) {
			if (sceneModelRatio <= 0.f) { throw std::invalid_argument{"Scene - model ratio must be positive value!"}; }
			mSceneModelRatio = sceneModelRatio;

			mLowerBoundingBoxScene = getSceneFromModelPos(mLowerBoundingBoxModel);
			mUpperBoundingBoxScene = getSceneFromModelPos(mUpperBoundingBoxModel);
		}

		std::tuple<glm::vec3, glm::vec3> BoundedScene::sceneBounds() const {
			return {mLowerScene, mUpperScene};
		}

		std::tuple<glm::vec3, glm::vec3> BoundedScene::boundingBoxSceneBounds() const {
			return {mLowerBoundingBoxScene, mUpperBoundingBoxScene};
		}

		std::tuple<bool, glm::vec3>
		BoundedScene::tryGetRootPosition(const glm::vec3& scenePos, const glm::vec3& direction,
		                                 const float distance) {
			const auto dirNormalized = glm::normalize(direction);

			RTCRayHit rayHit{};
			RTCIntersectContext context{};
			rtcInitIntersectContext(&context);
			prepareRtcRay(rayHit.ray, scenePos, dirNormalized, distance);
			rayHit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
			rayHit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;
			rtcIntersect1(mRtcScene, &context, &rayHit);

			glm::vec3 potentialPosition;
			if (rayHit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {		// We got hit to geometry
				const auto normal = glm::normalize(glm::vec3{rayHit.hit.Ng_x, rayHit.hit.Ng_y, rayHit.hit.Ng_z});
				if (normal.y < -0.01f) { return {false, {}}; } // Hitting scene triangle from below
				const auto avoidSelfIntersectMove = mSceneModelRatio >= 1.f ? 1e-3f : mSceneModelRatio * 1e-3f;
				potentialPosition = scenePos + dirNormalized * rayHit.ray.tfar - dirNormalized * avoidSelfIntersectMove;
			}
			else {	// Try hit ground zero of scene
				// Ground zero cant be hit from below
				if (scenePos.y <= mGroundZeroY || dirNormalized.y >= 0.f) { return {false, {}}; }
				const float toGroundZero = -((scenePos.y - mGroundZeroY) / dirNormalized.y);
				potentialPosition = scenePos + toGroundZero * dirNormalized;
				potentialPosition.y = mGroundZeroY; // Rounding error
			}

			return {true, potentialPosition};
		}

		void BoundedScene::loadSceneGeometry(const std::vector<float>& verticesPos,
		                                     const std::vector<unsigned>& triangleIndices) {
			// Internal alignment of 16 bytes (each element should be readable by 16-byte SSE load instruction)
			float* vertices = static_cast<float*>(rtcSetNewGeometryBuffer(
				mRtcGeometry, RTC_BUFFER_TYPE_VERTEX, 0,
				RTC_FORMAT_FLOAT3, sizeof(float) * 4, verticesPos.size() / 3));

			for (int i = 0; i < verticesPos.size() / 3; ++i) {
				vertices[i * 4 + 0] = verticesPos[i * 3 + 0];
				vertices[i * 4 + 1] = verticesPos[i * 3 + 1];
				vertices[i * 4 + 2] = verticesPos[i * 3 + 2];
			}

			void* triangles = static_cast<unsigned*>(rtcSetNewGeometryBuffer(
				mRtcGeometry, RTC_BUFFER_TYPE_INDEX, 0,
				RTC_FORMAT_UINT3, sizeof(unsigned) * 3, triangleIndices.size() / 3));

			std::memcpy(triangles, triangleIndices.data(), sizeof(unsigned) * triangleIndices.size());

			rtcCommitGeometry(mRtcGeometry);
			rtcCommitScene(mRtcScene);

			// Scene bounds and ground zero for tree root placement
			RTCBounds bounds{};
			rtcGetSceneBounds(mRtcScene, &bounds);

			mLowerScene = {bounds.lower_x, bounds.lower_y, bounds.lower_z};
			mUpperScene = {bounds.upper_x, bounds.upper_y, bounds.upper_z};
			mGroundZeroY = bounds.lower_y;
		}


		void BoundedScene::defaultEmptySceneGeometry() {
			rtcSetNewGeometryBuffer(mRtcGeometry, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(float) * 4, 0);
			rtcSetNewGeometryBuffer(mRtcGeometry, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(unsigned) * 3, 0);

			rtcCommitGeometry(mRtcGeometry);
			rtcCommitScene(mRtcScene);

			setDefaultSceneProperties();
		}

		// ===========================================================================================================
		// Internal methods for treegen algorithm
		// ===========================================================================================================

		void BoundedScene::sceneBasePos(const glm::vec3& sceneBasePos) {
			mSceneBasePos = sceneBasePos;

			mLowerBoundingBoxScene = getSceneFromModelPos(mLowerBoundingBoxModel);
			mUpperBoundingBoxScene = getSceneFromModelPos(mUpperBoundingBoxModel);

			mBoundingBoxDefined = true;
		}

		glm::vec3 BoundedScene::getSceneFromModelPos(const glm::vec3& modelPos) {
			// Move to origin, rescale and move to proper position in scene
			return ((modelPos - mBoundingBoxCenterBase) * mSceneModelRatio) + mSceneBasePos;
		}

		glm::vec3 BoundedScene::getModelFromScenePos(const glm::vec3& scenePos) {
			// Move to origin, rescale and move to proper position in model
			return ((scenePos - mSceneBasePos) / mSceneModelRatio) + mBoundingBoxCenterBase;
		}
 
		bool BoundedScene::isValidDirection(const glm::vec3& fromModelPosition, const glm::vec3& direction,
		                                    const float modelDistance) {
			assert(isInBoundingBox(fromModelPosition));

			return !intersectsWithScene(fromModelPosition, direction, modelDistance) &&
			       isInBoundingBox(fromModelPosition + (glm::normalize(direction) * modelDistance));
		}

		bool BoundedScene::isValidDirection(const glm::vec3& fromModelPosition, const glm::vec3& toModelPosition) {
			assert(isInBoundingBox(fromModelPosition));

			const auto direction = toModelPosition - fromModelPosition;
			const auto modelDistance = glm::length(direction);

			return !intersectsWithScene(fromModelPosition, direction, modelDistance) &&
			       isInBoundingBox(toModelPosition);

		}

		bool BoundedScene::isInBoundingBox(const glm::vec3& modelPosition) const {
			return modelPosition.x >= 0.f && modelPosition.x <= mBoundingBoxSide
			       && modelPosition.y >= 0.f && modelPosition.y <= mBoundingBoxSide
			       && modelPosition.z >= 0.f && modelPosition.z <= mBoundingBoxSide;
		}

		bool BoundedScene::intersectsWithScene(const glm::vec3& fromModelPosition, const glm::vec3& direction,
		                                       const float modelDistance) {
			RTCRay ray{};
			RTCIntersectContext context{};
			rtcInitIntersectContext(&context);

			const auto fromScenePosition = getSceneFromModelPos(fromModelPosition);
			const float sceneDistance = mSceneModelRatio * modelDistance;
			prepareRtcRay(ray, fromScenePosition, direction, sceneDistance);

			rtcOccluded1(mRtcScene, &context, &ray);

			return !(ray.tfar == sceneDistance);
		}

		bool BoundedScene::
		intersectsWithScene(const glm::vec3& fromModelPosition, const glm::vec3& toModelPosition) {
			const auto direction = toModelPosition - fromModelPosition;
			const auto modelDistance = glm::length(direction);

			return intersectsWithScene(fromModelPosition, direction, modelDistance);
		}

		// ===========================================================================================================
		// Private methods
		// ===========================================================================================================

		void BoundedScene::setDefaultSceneProperties() {
			mSceneModelRatio = 1.f;

			mSceneBasePos = {0.f, 0.f, 0.f};;

			mLowerScene = mLowerBoundingBoxModel - mBoundingBoxCenterBase;
			mUpperScene = mUpperBoundingBoxModel - mBoundingBoxCenterBase;
			mGroundZeroY = 0.f;

			mBoundingBoxDefined = false;

			mLowerBoundingBoxScene = mLowerScene;
			mUpperBoundingBoxScene = mUpperScene;
		}

		void BoundedScene::initializeEmbree() {
			// Initialization of embree + commit of empty scene
			mRtcDevice = rtcNewDevice("");
			if (!mRtcDevice) { throw std::runtime_error{"Embree is not supported by HW!"}; }

			mRtcScene = rtcNewScene(mRtcDevice);
			if (!mRtcScene) { throw std::runtime_error{"Embree is not supported by HW!"}; }
			rtcSetSceneFlags(mRtcScene, RTC_SCENE_FLAG_ROBUST);

			mRtcGeometry = rtcNewGeometry(mRtcDevice, RTC_GEOMETRY_TYPE_TRIANGLE);
			if (!mRtcGeometry) { throw std::runtime_error{"Embree is not supported by HW!"}; }

			rtcCommitGeometry(mRtcGeometry);
			mRtcGeometryId = rtcAttachGeometry(mRtcScene, mRtcGeometry);
			rtcCommitScene(mRtcScene);
		}

		void BoundedScene::prepareRtcRay(RTCRay& ray, const glm::vec3& from, const glm::vec3& dir,
		                                 const float distance) const {
			ray.org_x = from.x;
			ray.org_y = from.y;
			ray.org_z = from.z;
			ray.tnear = 0.f;

			ray.dir_x = dir.x;
			ray.dir_y = dir.y;
			ray.dir_z = dir.z;

			ray.tfar = distance;
			ray.flags = 0;
		}
	}
}

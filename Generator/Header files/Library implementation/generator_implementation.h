/// Tree generator implementation.
/// \file generator_implementation.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#ifndef GENERATOR_IMPLEMENTATION_H
#define GENERATOR_IMPLEMENTATION_H

#include <Library interface/generator.h>
#include <Library implementation/tree_implementation.h>
#include <Library implementation/environment.h>

#include <unordered_map>
#include <vector>

namespace Treegen
{
	/// Tree generator implementation.
	class GeneratorImplementation final : public Generator {
		Environment mEnvironment{};
		
		std::unordered_map<unsigned, std::unique_ptr<TreeImplementation>> mTrees{};
		unsigned mNewTreeId = 0;
		
		/// \name States and synchronization mutex.
		///@{
		mutable std::mutex mStateLock{};
		bool mEnvironmentLocked = false;
		bool mIsRunning = false;
		bool mIsCancelled = false;
		///@}

	public:

		/// \name Public API implementation.
		///@{

		// ===========================================================================================================
		// Getters
		// ===========================================================================================================

		bool isEnvironmentLocked() const override;
		bool isRunning() const override;
		bool isCanceled() const override;

		float sceneModelRatio() const override;

		float perceptionDistance() const override;
		float killDistance() const override;
		size_t budFreeSpaceMarkers() const override;
		float coneAngle() const override;

		size_t shadowPyramidHeight() const override;
		float nodeShadow() const override;
		float shadowDiminishMultiplier() const override;
		float maximumLightValue() const override;

		// ===========================================================================================================
		// Setters
		// ===========================================================================================================

		void sceneModelRatio(float ratio) override;

		void perceptionOccupancyDistance(float perceptionDistance, float occupancyDistance) override;
		void budFreeSpaceMarkers(size_t count) override;
		void coneAngle(float angleRadians) override;

		void setShadowProperties(size_t pyramidHeight, float nodeShadow, float shadowDiminishMultiplier,
		                         float maximumLight) override;

		// ===========================================================================================================
		// Tree && Scene API
		// ===========================================================================================================

		std::vector<unsigned> validTreeIds() const override;
		unsigned newTree(const glm::vec3& scenePosition) override;
		void deleteTree(unsigned Id) override;
		void deleteAllTrees() override;
		void clearCurrentModels() override;
		Tree& tree(unsigned Id) const override;

		void runIterations(size_t count) override;
		void cancelIterations() override;

		bool boundingBoxDefined() const override;
		std::tuple<glm::vec3, glm::vec3> modelBounds() const override;
		std::tuple<glm::vec3, glm::vec3> sceneBounds() const override;
		std::tuple<glm::vec3, glm::vec3> boundingBoxBounds() const override;

		std::tuple<bool, glm::vec3> tryGetRootPosition(const glm::vec3& scenePos, const glm::vec3& direction,
		                                               float distance) override;
		void loadScene(const std::vector<float>& verticesPositions,
		               const std::vector<unsigned>& triangleIndices) override;
		void defaultEmptyScene() override;

		///@}

	private:
		/// Implementation of \p clearCurrentModels.
		/// \param throwIfRunning Flag - if instead of cleaning should be thrown exception if generator is running.
		/// If generation is running throws std::logic_error exception.
		void clearCurrentModelsInternal(bool throwIfRunning);
		
		/// Does run of one iteration of generative algorithm.
		void runIteration();

		/// If environment is locked throws std::logic_error exception. (Synchronized access to state variable)
		void checkEnvironmentLockException();
		/// If generation is running throws std::logic_error exception. (Synchronized access to state variable)
		void checkIfRunningException();
	};
}

#endif // GENERATOR_IMPLEMENTATION_H

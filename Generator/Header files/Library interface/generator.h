/// Tree generator public API.
/// \file generator.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#ifndef GENERATOR_H
#define GENERATOR_H

#include "tree.h"

#include <glm/vec3.hpp>

#include <vector>
#include <memory>

#ifndef TREEGEN_AS_DLL
#define TREEGEN_API
#else

#ifdef TREEGEN_EXPORTS
#define TREEGEN_API __declspec(dllexport)
#else
#define TREEGEN_API __declspec(dllimport)
#endif

// Disables warnings from exporting of members without dll-interface
#pragma warning(push)
#pragma warning(disable: 4251)

#endif

namespace Treegen
{
	/// Tree generator public API.
	class TREEGEN_API Generator {
	public:
		virtual ~Generator() = default;
		static std::unique_ptr<Generator> createGenerator();

		// ===========================================================================================================
		// Getters
		// ===========================================================================================================

		/// \name Getters.
		///@{
		virtual bool isEnvironmentLocked() const = 0;	///< Checks if setting of properties is allowed.
		virtual bool isRunning() const = 0;				///< Checks if generation is in progress.
		virtual bool isCanceled() const = 0;			///< Checks if generation in progress is cancelled.

		virtual float sceneModelRatio() const = 0;

		virtual float perceptionDistance() const = 0;
		virtual float killDistance() const = 0;
		virtual size_t budFreeSpaceMarkers() const = 0;
		virtual float coneAngle() const = 0;

		virtual size_t shadowPyramidHeight() const = 0;
		virtual float nodeShadow() const = 0;
		virtual float shadowDiminishMultiplier() const = 0;
		virtual float maximumLightValue() const = 0;
		///@}

		// ===========================================================================================================
		// Setters
		// ===========================================================================================================

		/// \name Setters.
		///@{

		/// Sets scene - model ratio.
		/// \param ratio Positive value representing new ratio.
		/// \warning For invalid argument throws std::invalid_argument exception.
		virtual void sceneModelRatio(float ratio) = 0;

		/// Sets attraction and kill distance.
		/// \param perceptionDistance Attraction distance must be at least 3.0.
		/// \param occupancyDistance Occupancy distance must be at least 2.0 and less than \p perceptionDistance.
		/// \warning For invalid arguments throws std::invalid_argument exception.
		virtual void perceptionOccupancyDistance(float perceptionDistance, float occupancyDistance) = 0;

		/// Sets number of generated free space markers for each bud.
		/// \param count Nonzero number of markers.
		/// \warning For invalid argument throws std::invalid_argument exception.
		virtual void budFreeSpaceMarkers(size_t count) = 0;

		/// Sets angle of perception volume cone.
		/// \param angleRadians Angle of cone in radians from range [0.0, 160.0] degrees.
		/// \warning For invalid argument throws std::invalid_argument exception.
		virtual void coneAngle(float angleRadians) = 0;

		/// Sets new properties of light model.
		/// \param pyramidHeight Positive value of height of shadow propagation pyramid in grid. 
		/// \param nodeShadow Non negative shadow value of node in its grid cell.
		/// \param shadowDiminishMultiplier Diminish multiplier of shadow between layers from range [0.0, 1.0].
		/// \param maximumLight Non negative maximum light exposure value.
		virtual void setShadowProperties(size_t pyramidHeight, float nodeShadow,
		                                 float shadowDiminishMultiplier, float maximumLight) = 0;
		///@{

		// ===========================================================================================================
		// Tree && Scene API
		// ===========================================================================================================

		/// Gets identifiers of all trees in generator.
		/// \return Tree identifiers.
		virtual std::vector<unsigned> validTreeIds() const = 0;

		/// Makes new tree.
		/// \param scenePosition Position of tree in scene.
		/// \return Identification number of new tree.
		virtual unsigned newTree(const glm::vec3& scenePosition) = 0;

		/// Deletes tree from generator.
		/// \param Id Identification number of tree to be deleted.
		/// Invalid tree identifier is ignored.
		virtual void deleteTree(unsigned Id) = 0;

		/// Deletes all trees in generator.
		virtual void deleteAllTrees() = 0;

		/// Sets age of all trees to zero - resets all generation that was done.
		/// Settings of generator and trees are not changed.
		/// Allows setting of environment properties.
		virtual void clearCurrentModels() = 0;

		/// Gets tree from its identifier.
		/// \param Id Tree identifier.
		/// \return Tree with given identifier.
		/// \warning If there is no tree with \p Id identifier throws std::invalid_argument exception.
		virtual Tree& tree(unsigned Id) const = 0;

		/// Runs iterations of generative algorithm.
		/// \param count Number of iterations.
		virtual void runIterations(size_t count) = 0;

		/// Cancels run of generator.
		/// Generation is stopped on first check for cancellation.
		/// After cancelled run is finished generator is in valid state for further use.
		virtual void cancelIterations() = 0;

		/// Checks if bounding box limiting space in which generator may generate models is defined.
		/// Box is defined after insertion of first tree into generator.
		virtual bool boundingBoxDefined() const = 0;

		/// Bound of bounding box in internal world coordinates.
		/// \return Minimal and maximal coordinates of bounding box in internal world coordinates.
		virtual std::tuple<glm::vec3, glm::vec3> modelBounds() const = 0;

		/// Gets bounds of loaded scene.
		/// \return Minimal and maximal coordinates of bound of sceme.
		virtual std::tuple<glm::vec3, glm::vec3> sceneBounds() const = 0;

		/// Gets bounds of bounding box limiting space in which generator may generate models.
		/// \return Minimal and maximal coordinates of bounding box in scene.
		/// \warning Returned values are valid only if \p boundingBoxDefined returns true.
		virtual std::tuple<glm::vec3, glm::vec3> boundingBoxBounds() const = 0;

		/// Tries to derive position for root of tree based on intersection of a ray with loaded scene.
		/// \param scenePos Position from which the ray is casted.
		/// \param direction Direction of ray.
		/// \param distance Maximum distance to which ray may travel.
		/// \return Result of ray casting. Bool represents if value in vector can be used as root of a tree.
		virtual std::tuple<bool, glm::vec3> tryGetRootPosition(
			const glm::vec3& scenePos, const glm::vec3& direction, float distance) = 0;

		/// Loads triangle mesh geometry of new scene. Deletes representation of previous scene.
		/// \param verticesPositions Vertices of scene mesh.
		/// \param triangleIndices Triangles of scene mesh.
		virtual void loadScene(const std::vector<float>& verticesPositions,
		                       const std::vector<unsigned>& triangleIndices) = 0;

		/// Loads empty scene. Deletes representation of previous scene.
		virtual void defaultEmptyScene() = 0;

	protected:
		Generator() = default;
		Generator(const Generator&) = default;
		Generator& operator=(const Generator&) = default;
		Generator(Generator&&) = default;
		Generator& operator=(Generator&&) = default;
	};
}

#ifdef TREEGEN_DLL_EXPORT
#pragma warning(pop)
#endif

#endif // GENERATOR_H

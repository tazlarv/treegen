/// Utilities used in Generator project.
/// \file treegen_utils.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#ifndef TREEGEN_UTILS_H
#define TREEGEN_UTILS_H

#include <glm/glm.hpp>

#include <random>
#include <cmath>

namespace Treegen
{
	namespace Utils
	{
		/// Gets random seed for initialization of random generator.
		/// \returns Random seed.
		unsigned getRndSeed();

		/// Rotates vector in same way as is rotation from starting orientation to final orientation of given vector.
		/// \param startOrientation Normalized starting orientation of vector.
		/// \param finalOrientation Normalized final orientation of vector.
		/// \param rotateVec Vector to be rotated.
		/// \returns Rotated vector.
		/// \warning If \p startOrientation or \p finalOrientation is not normalized behaviour of function is undefined.
		glm::vec3 rotateVecBasedOnChangedOrientation(
			const glm::vec3& startOrientation, const glm::vec3& finalOrientation, const glm::vec3& rotateVec);

		/// Combines two hash values into one of same size.
		/// Boost like implementation - https://stackoverflow.com/a/2595226.
		/// \tparam T Type of hashed value.
		/// \param seed Value to be combined with hash of object.
		/// \param value Object to be hashed and its result combined into \p seed.
		template <class T>
		void hashCombine(std::size_t& seed, const T& value) {
			std::hash<T> hasher;
			seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		}

		/// Gets random point on surface of unit sphere.
		/// Marsaglia (1972) (from: The Annals of Mathematical Statistics)
		/// \tparam TRnd Type of random number engine.
		/// \param rnd Random number engine.
		/// \return Coordinates of point on surface of unit sphere.
		template <typename TRnd>
		glm::vec3 getUnitSpherePoint(TRnd& rnd) {
			const std::uniform_real_distribution<float> distribution(-1.f, 1.f);
			float v1, v2, S;

			do {
				v1 = distribution(rnd);
				v2 = distribution(rnd);
				S = v1 * v1 + v2 * v2;
			} while (S >= 1.f);

			const float sqrt1S = std::sqrt(1 - S);
			const float x = 2 * v1 * sqrt1S;
			const float y = 2 * v2 * sqrt1S;
			const float z = 1 - 2 * S;

			return {x, y, z};
		}

		/// Gets random normalized orthogonal vector to given vector.
		/// \tparam TRnd Type of random number engine.
		/// \param vector Normalizable input vector.
		/// \param rnd Random number engine.
		/// \return Random normalized orthogonal vector to inputed one.
		/// \warning If \p vector is not normalizable function may try to divide by zero.
		template <typename TRnd>
		glm::vec3 getRndOrthogonalNormalized(const glm::vec3& vector, TRnd& rnd) {
			assert(glm::length(vector) != 0.f);

			glm::vec3 orthogonal;
			float length;
			do {
				const auto rndVec = getUnitSpherePoint(rnd);
				orthogonal = glm::cross(vector, rndVec);
				length = glm::length(orthogonal);
			} while (length == 0.f);
			
			// Return normalized vector
			return orthogonal * (1.f / length);
		}
	}
}

#endif // TREEGEN_UTILS_H

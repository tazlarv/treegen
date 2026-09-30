/// Utilities for usage of glm library.
/// \file glm_utils.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#ifndef GLM_UTILS_H
#define GLM_UTILS_H

#include <Utilities/treegen_utils.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <ostream>

namespace GlmUtils
{

	/// Calculates squared distance between two positions.
	/// \tparam TVector GLM vector.
	/// \param from First position.
	/// \param to Second position.
	/// \returns Squared distance between positions.
	template <typename TVector>
	auto distanceSqr(const TVector& from, const TVector& to) {
		TVector between{to - from};
		return glm::dot(between, between);
	}
	
	/// Handle that allows printing of glm vector in ostream.
	/// \tparam TVec3D 3 dimensional vector with variables x, y, z.
	template <typename TVec3D>
	struct printVec3D {
		/// \param vec GLM vector to be printed.
		explicit printVec3D(const TVec3D& vec) : vecToPrint{vec} {}
		const TVec3D& vecToPrint;
	};

	/// Ostream overload for printing printVec3D.
	/// \tparam TVec3D 3 dimensional vector with variables x, y, z.
	/// \param stream Output stream.
	/// \param wrapper printVec3D object.
	template <typename TVec3D>
	std::ostream& operator<<(std::ostream& stream, const printVec3D<TVec3D>& wrapper) {
		return stream
		       << "{ "
		       << wrapper.vecToPrint.x << ", " << wrapper.vecToPrint.y << ", " << wrapper.vecToPrint.z
		       << " }";
	}

	/// Makes rotation matrix based on axis and angle of rotation.
	/// \param angleRadians Rotation angle in radians.
	/// \param axis Axis of rotation.
	/// \returns Rotation matrix.
	inline glm::mat3 rotationMatrix(const float angleRadians, const glm::vec3& axis) {
		return glm::mat3(glm::rotate(glm::mat4{1.0f}, angleRadians, axis));
	}

	/// Rotates vector around axis by angle.
	/// \param vector Vector to be rotated.
	/// \param angleRadians Rotation angle in radians.
	/// \param axis Axis of rotation.
	/// \returns Rotated vector.
	inline glm::vec3 rotateAroundAxis(const glm::vec3& vector, const float angleRadians, const glm::vec3& axis) {
		return GlmUtils::rotationMatrix(angleRadians, axis) * vector;
	}
}

namespace std
{
	/// std::hash overload for glm::vec3.
	template <>
	struct hash<glm::vec3> {
		/// Hash method of glm::vec3.
		/// \param key Vector to be hashed.
		/// \returns Hash value of key.
		size_t operator()(const glm::vec3& key) const {
			size_t seed = std::hash<float>{}(key.x);
			Treegen::Utils::hashCombine(seed, key.y);
			Treegen::Utils::hashCombine(seed, key.z);
			return seed;
		}
	};

	/// std::hash overload for glm::ivec3.
	template <>
	struct hash<glm::ivec3> {
		/// Hash method of glm::ivec3.
		/// \param key Vector to be hashed.
		/// \returns Hash value of key.
		size_t operator()(const glm::ivec3& key) const {
			size_t seed = std::hash<int>{}(key.x);
			Treegen::Utils::hashCombine(seed, key.y);
			Treegen::Utils::hashCombine(seed, key.z);
			return seed;
		}
	};
}


#endif // GLM_UTILS_H

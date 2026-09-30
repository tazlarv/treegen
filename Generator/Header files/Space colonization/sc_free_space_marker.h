/// Free space marker structure.
/// \file sc_free_space_marker.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#ifndef SC_FREE_SPACE_MARKER_H
#define SC_FREE_SPACE_MARKER_H

#include <Utilities/glm_utils.h> // Hash

#include <glm/glm.hpp>

#include <ostream>

namespace Treegen
{
	namespace SpaceCol
	{
		struct FreeSpaceMarker;
		using rwFreeSpaceMarker = std::reference_wrapper<FreeSpaceMarker>;
		using rwConstFreeSpaceMarker = std::reference_wrapper<const FreeSpaceMarker>;

		/// Marker representing free space in space colonization algorithm.
		struct FreeSpaceMarker {

			/// Constructs new FreeSpaceMarker.
			/// \param position Position of marker.
			/// \param id Identifier of marker.
			explicit FreeSpaceMarker(const glm::vec3& position, const unsigned id) : position{position}, id{id} {}

			glm::vec3 position;
			unsigned id;

			friend std::ostream& operator<<(std::ostream& stream, const FreeSpaceMarker& point);
		};

		inline bool operator==(const FreeSpaceMarker& op1, const FreeSpaceMarker& op2) {
			return op1.position == op2.position;
		}

		inline bool operator!=(const FreeSpaceMarker& op1, const FreeSpaceMarker& op2) { return !(op1 == op2); }

		inline std::ostream& operator<<(std::ostream& stream, const FreeSpaceMarker& point) {
			return stream << GlmUtils::printVec3D<glm::vec3>(point.position);
		}
	}
}

namespace std
{
	/// std::hash overload for FreeSpaceMarker.
	template <>
	struct hash<Treegen::SpaceCol::FreeSpaceMarker> {
		/// Hash method of FreeSpaceMarker.
		/// \param key FreeSpaceMarker to be hashed.
		/// \returns Hash value of key.
		size_t operator()(const Treegen::SpaceCol::FreeSpaceMarker& key) const {
			return hash<glm::vec3>{}(key.position);
		}
	};
}

#endif // SC_FREE_SPACE_MARKER_H

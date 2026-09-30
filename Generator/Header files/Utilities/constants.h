/// Global constants.
/// \file constants.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <glm/vec3.hpp>

namespace Treegen
{
	namespace Const
	{
		/// \name Utility constants
		///@{

		///	Safety net used for calculations with cross / dot products
		///	so output (of cross product) is nonzero normalizable vector.
		///	MSVC compiler does not compute cos function (it is not constexpr => forces call).
		///	Equal to rounded: std::cos(glm::radians(1.f)).
		constexpr float COS_1_DEG = 0.999848f;

		constexpr glm::vec3 WORLD_UP = {0.f, 1.f, 0.f};
		///@}

		/// \name Settings
		/// Grid and cell sides sizes.
		///@{
		constexpr float BOUNDING_BOX_SIDE = 200.f;
		constexpr glm::vec3 BOUNDING_BOX_SIDES = {BOUNDING_BOX_SIDE, BOUNDING_BOX_SIDE, BOUNDING_BOX_SIDE};
		constexpr glm::vec3 BOUNDING_BOX_CENTER_BASE = {BOUNDING_BOX_SIDE / 2.f, 0.f, BOUNDING_BOX_SIDE / 2.f};

		constexpr float SC_CELL_SIDE = 5.f;
		constexpr size_t SC_GRID_SIDE = size_t(BOUNDING_BOX_SIDE / SC_CELL_SIDE) + 1;

		constexpr float SP_CELL_SIDE = 1.f;
		constexpr size_t SP_GRID_SIDE = size_t(BOUNDING_BOX_SIDE / SP_CELL_SIDE) + 1;
		///@{
	}
}

#endif // CONSTANTS_H

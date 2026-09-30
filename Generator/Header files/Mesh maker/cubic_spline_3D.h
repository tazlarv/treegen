/*
 * cubic_spline_3D.h
 * 
 *  work based on previous:
 *  simple cubic spline interpolation library without external
 *  dependencies
 *
 *  supporting interpolation of sequences of 3D points (in space)
 *  f(t) = (x, y, z) where t is index of point on curve (0, 1,..., N-1)
 *  
 * ---------------------------------------------------------------------
 *  Original work Copyright (C) 2011, 2014 Tino Kluge (ttk448 at gmail.com)
 *  Modified work Copyright (C) 2018 Vojtěch Tázlar
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * ---------------------------------------------------------------------
 *
 */

 /// Cubic spline interpolation optimized for use case of meshing tree structure from TreeGen generator.
 /// \file cubic_spline_3D.h
 /// \date 2018

#ifndef CUBIC_SPLINE_3D_H
#define CUBIC_SPLINE_3D_H

#include <vector>
#include <cassert>

#include <glm/glm.hpp>

namespace Treegen
{
	namespace MeshMaker
	{
		namespace Spline3D
		{
			// Band matrix solver
			class BandMatrix {
				bool mLuDecomposed = false;
				std::vector<std::vector<float>> mUpper{}; // Upper band
				std::vector<std::vector<float>> mLower{}; // Lower band

			public:
				BandMatrix() = default;									// Constructor
				BandMatrix(size_t dim, size_t nUpper, size_t nLower);	// Constructor
				void resize(size_t dim, size_t nUpper, size_t nLower);	// Init with dim, nUpper, nLower

				int dim() const;   // Matrix dimension
				bool decomposed() const { return mLuDecomposed; }
				int numUpper() const { return int(mUpper.size() - 1); }
				int numLower() const { return int(mLower.size() - 1); }
			
				// Access operator
				float& operator ()(int i, int j);			// Write
				float operator ()(int i, int j) const;		// Read
			
				// We can store an additional diogonal (in m_lower_)
				float& savedDiagonal(int i);
				float savedDiagonal(int i) const;

				// All solve methods have undefined behavior if they are called before lu_decompose
				void luDecompose();
				std::vector<float> rSolve(const std::vector<float>& b) const;
				std::vector<float> lSolve(const std::vector<float>& b) const;
				std::vector<float> luSolve(const std::vector<float>& b) const;

				std::vector<glm::vec3> rSolve(const std::vector<glm::vec3>& b) const;
				std::vector<glm::vec3> lSolve(const std::vector<glm::vec3>& b) const;
				std::vector<glm::vec3> luSolve(const std::vector<glm::vec3>& b) const;
			};

			// 3d cubic spline interpolation of points (x, y, z):
			// f(t) = (x, y, z) where t is (implicitly) index of point (0, 1, ..., N - 1)
			// (optimal for sequences where distance between following points is constant)
			class CubicSpline3D {
				std::vector<glm::vec3> mXyzPoints{};
				std::vector<glm::vec3> mXyzA{}, mXyzB{}, mXyzC{}; // Spline coefficients

			public:
				enum class Derivation {
					First,
					Second
				};

				// Interpolation parameters
				// f(x) = a*(x-x_i)^3 + b*(x-x_i)^2 + c*(x-x_i) + y_i
				const size_t pointsCount;
				const Derivation leftBoundaryDerivation;
				const Derivation rightBoundaryDerivation;
				const float leftBoundaryValue;
				const float rightBoundaryValue;

				// Default boundary condition => zero curvature at both ends (natural cubic spline)
				explicit CubicSpline3D(std::vector<glm::vec3> xyzPoints,
				                       Derivation leftBoundaryDerivation = Derivation::Second,
				                       Derivation rightBoundaryDerivation = Derivation::Second,
									   float leftBoundaryValue = 0.f, float rightBoundaryValue = 0.f);

				glm::vec3 operator()(float x) const;
				glm::vec3 derivation(int order, float x) const;

			private:
				size_t getSubsplineIdx(float x) const;
			};
		}
	}
}
#endif // CUBIC_SPLINE_3D_H

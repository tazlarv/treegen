/*
* cubic_spline_3D.cpp
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

/// Cubic spline interpolation optimized for use case of meshing tree structure from generator.
/// \file cubic_spline_3D.cpp
/// \date 2018

#include <stdafx.h> // Std headers and GLM library
#include <Mesh maker/cubic_spline_3D.h>

namespace Treegen
{
	namespace MeshMaker
	{
		namespace Spline3D
		{
			// ===========================================================================================================
			// BandMatrix implementation
			// ===========================================================================================================

			BandMatrix::BandMatrix(const size_t dim, const size_t nUpper, const size_t nLower) {
				resize(dim, nUpper, nLower);
			}

			void BandMatrix::resize(const size_t dim, const size_t nUpper, const size_t nLower) {
				assert(dim > 0);

				mUpper.resize(nUpper + 1);
				mLower.resize(nLower + 1);

				for (size_t i = 0; i < mUpper.size(); i++) { mUpper[i].resize(dim); }
				for (size_t i = 0; i < mLower.size(); i++) { mLower[i].resize(dim); }
			}

			int BandMatrix::dim() const {
				if (mUpper.size() > 0) { return int(mUpper[0].size()); }
				return 0;
			}

			// Defines the new operator (), so that we can access the elements
			// by A(i,j), index going from i=0,...,dim()-1
			float& BandMatrix::operator ()(const int i, const int j) {
				assert(!mLuDecomposed);
				const int k = j - i;       // what band is the entry
				assert((i >= 0) && (i < dim()) && (j >= 0) && (j < dim()));
				assert((-numLower() <= k) && (k <= numUpper()));
			
				// k = 0 -> diogonal, k < 0 lower left part, k > 0 upper right part
				if (k >= 0) { return mUpper[k][i]; }
				return mLower[-k][i];
			}

			float BandMatrix::operator ()(const int i, const int j) const {
				const int k = j - i;       // what band is the entry
				assert((i >= 0) && (i < dim()) && (j >= 0) && (j < dim()));
				assert((-numLower() <= k) && (k <= numUpper()));
			
				// k=0 -> diogonal, k < 0 lower left part, k > 0 upper right part
				if (k >= 0) { return mUpper[k][i]; }
				return mLower[-k][i];
			}

			// Second diag (used in LU decomposition), saved in m_lower_
			float BandMatrix::savedDiagonal(const int i) const {
				assert((i >= 0) && (i < dim()));
				return mLower[0][i];
			}

			float& BandMatrix::savedDiagonal(const int i) {
				assert((i >= 0) && (i < dim()));
				return mLower[0][i];
			}

			// LR-Decomposition of a band matrix
			void BandMatrix::luDecompose() {
				int jMax;

				// Preconditioning
				// Normalize column i so that a_ii=1
				for (int i = 0; i < this->dim(); i++) {
					assert(this->operator()(i, i) != 0.f);

					this->savedDiagonal(i) = 1.f / this->operator()(i, i);
					const int jMin = (std::max)(0, i - this->numLower());
					jMax = (std::min)(this->dim() - 1, i + this->numUpper());
					for (int j = jMin; j <= jMax; j++) {
						this->operator()(i, j) *= this->savedDiagonal(i);
					}
					this->operator()(i, i) = 1.f;   // Prevents rounding errors
				}

				// Gauss LR-Decomposition
				for (int k = 0; k < this->dim(); k++) {
					const int iMax = (std::min)(this->dim() - 1, k + this->numLower());  // num_lower not a mistake!
					for (int i = k + 1; i <= iMax; i++) {
						assert(this->operator()(k, k) != 0.f);

						const float x = -this->operator()(i, k) / this->operator()(k, k);
						this->operator()(i, k) = -x;	// Assembly part of L
						jMax = (std::min)(this->dim() - 1, k + this->numUpper());
						for (int j = k + 1; j <= jMax; j++) {
							// Assembly part of R
							this->operator()(i, j) = this->operator()(i, j) + x * this->operator()(k, j);
						}
					}
				}
				mLuDecomposed = true;
			}

			// Solves Ly=b
			std::vector<float> BandMatrix::lSolve(const std::vector<float>& b) const {
				assert(this->dim() == int(b.size()));

				std::vector<float> x(this->dim());
				for (int i = 0; i < this->dim(); i++) {
					float sum = 0;
					const int jStart = (std::max)(0, i - this->numLower());
					for (int j = jStart; j < i; j++) sum += this->operator()(i, j) * x[j];
					x[i] = (b[i] * this->savedDiagonal(i)) - sum;
				}
				return x;
			}

			// Solves Rx=y
			std::vector<float> BandMatrix::rSolve(const std::vector<float>& b) const {
				assert(this->dim() == int(b.size()));

				std::vector<float> x(this->dim());
				for (int i = this->dim() - 1; i >= 0; i--) {
					float sum = 0;
					const int jStop = (std::min)(this->dim() - 1, i + this->numUpper());
					for (int j = i + 1; j <= jStop; j++) sum += this->operator()(i, j) * x[j];
					x[i] = (b[i] - sum) / this->operator()(i, i);
				}
				return x;
			}

			std::vector<float> BandMatrix::luSolve(const std::vector<float>& b) const {
				assert(mLuDecomposed);
				assert(this->dim() == int(b.size()));

				const auto y = this->lSolve(b);
				return this->rSolve(y);
			}


			// Solves Ly=b
			std::vector<glm::vec3> BandMatrix::lSolve(const std::vector<glm::vec3>& b) const {
				assert(mLuDecomposed);
				assert(this->dim() == int(b.size()));

				std::vector<glm::vec3> x(this->dim());
				for (int i = 0; i < this->dim(); i++) {
					glm::vec3 sum{0.f, 0.f, 0.f};
					const int jStart = (std::max)(0, i - this->numLower());
					for (int j = jStart; j < i; j++) sum += this->operator()(i, j) * x[j];
					x[i] = (b[i] * this->savedDiagonal(i)) - sum;
				}
				return x;
			}

			// Solves Rx=y
			std::vector<glm::vec3> BandMatrix::rSolve(const std::vector<glm::vec3>& b) const {
				assert(mLuDecomposed);
				assert(this->dim() == int(b.size()));

				std::vector<glm::vec3> x(this->dim());
				for (int i = this->dim() - 1; i >= 0; i--) {
					glm::vec3 sum{0.f, 0.f, 0.f};
					const int jStop = (std::min)(this->dim() - 1, i + this->numUpper());
					for (int j = i + 1; j <= jStop; j++) sum += this->operator()(i, j) * x[j];
					x[i] = (b[i] - sum) / this->operator()(i, i);
				}
				return x;
			}

			std::vector<glm::vec3> BandMatrix::luSolve(const std::vector<glm::vec3>& b) const {
				assert(mLuDecomposed);
				assert(this->dim() == int(b.size()));

				const std::vector<glm::vec3> y = this->lSolve(b);
				return this->rSolve(y);
			}

			// ===========================================================================================================
			// CubicSpline3D implementation
			// ===========================================================================================================

			CubicSpline3D::CubicSpline3D(std::vector<glm::vec3> xyzPoints,
			                             const Derivation leftBoundaryDerivation,
			                             const Derivation rightBoundaryDerivation,
			                             const float leftBoundaryValue, const float rightBoundaryValue) :
				mXyzPoints{std::move(xyzPoints)},
				pointsCount{mXyzPoints.size()},
				leftBoundaryDerivation{leftBoundaryDerivation},
				rightBoundaryDerivation{rightBoundaryDerivation},
				leftBoundaryValue{leftBoundaryValue}, rightBoundaryValue{rightBoundaryValue} {
				assert(pointsCount > 2);

				// Whole system is reduced because indices x of f(x) = y of spline are declared as { 0, ..., n-1 }
				// Equations are then reduced (alot of a[x + 1] - a[x] => x + 1 - x => 1 etc.)

				const auto n = int(pointsCount);

				// Setting up the matrix for the parameters b[] and  right hand side of the equation system
				BandMatrix aMatrix(n, 1, 1);
				std::vector<glm::vec3> xyzRhs(n);

				for (int i = 1; i < n - 1; ++i) {
					aMatrix(i, i - 1) = 1.f / 3.f;
					aMatrix(i, i) = (2.f / 3.f) * 2;
					aMatrix(i, i + 1) = 1.f / 3.f;

					xyzRhs[i] = mXyzPoints[i + 1] + mXyzPoints[i - 1] - (2.f * mXyzPoints[i]);
				}

				const glm::vec3 xyzLeftBoundary{leftBoundaryValue, leftBoundaryValue, leftBoundaryValue};
				const glm::vec3 xyzRightBoundary{rightBoundaryValue, rightBoundaryValue, rightBoundaryValue};

				// Boundary conditions
				if (leftBoundaryDerivation == Derivation::Second) {
					// 2*b[0] = f''
					aMatrix(0, 0) = 2.f;
					aMatrix(0, 1) = 0.f;

					xyzRhs[0] = xyzLeftBoundary;
				}
				else if (leftBoundaryDerivation == Derivation::First) {
					// c[0] = f', needs to be re-expressed in terms of b:
					// (2b[0]+b[1])(x[1]-x[0]) = 3 ((y[1]-y[0])/(x[1]-x[0]) - f')
					aMatrix(0, 0) = 2.f;
					aMatrix(0, 1) = 1.f;

					xyzRhs[0] = 3.f * (mXyzPoints[1] - mXyzPoints[0] - xyzLeftBoundary);
				}

				if (rightBoundaryDerivation == Derivation::Second) {
					// 2*b[n-1] = f''
					aMatrix(n - 1, n - 1) = 2.f;
					aMatrix(n - 1, n - 2) = 0.f;

					xyzRhs[n - 1] = xyzRightBoundary;
				}
				else if (rightBoundaryDerivation == Derivation::First) {
					// c[n-1] = f', needs to be re-expressed in terms of b:
					// (b[n-2]+2b[n-1])(x[n-1]-x[n-2])
					// = 3 (f' - (y[n-1]-y[n-2])/(x[n-1]-x[n-2]))
					aMatrix(n - 1, n - 1) = 2.f;
					aMatrix(n - 1, n - 2) = 1.f;

					xyzRhs[n - 1] = 3.f * (xyzRightBoundary - (mXyzPoints[n - 1] - mXyzPoints[n - 2]));
				}

				// Solve the equation system to obtain the parameters b[]
				aMatrix.luDecompose();
				mXyzB = aMatrix.luSolve(xyzRhs);

				// Calculate parameters a[] based on b[]
				mXyzA.resize(n);
				mXyzC.resize(n);

				for (int i = 0; i < n - 1; i++) {
					mXyzA[i] = (1.f / 3.f) * (mXyzB[i + 1] - mXyzB[i]);
					mXyzC[i] = (mXyzPoints[i + 1] - mXyzPoints[i]) - (1.f / 3.f) * (2.f * mXyzB[i] + mXyzB[i + 1]);
				}

				// Left extrapolation is based on 0th spline segment (between points 0, 1)
				// Right extrapolation is based on (n-2)th spline segment (between points n-2, n-1)
			}

			glm::vec3 CubicSpline3D::operator()(const float x) const {
				const auto idx = getSubsplineIdx(x);
				const auto h = x - idx;	// offset from index to x (index of subspline = f from f(x) = y)

				// Interpolation (or extrapolation to left/right)
				return ((mXyzA[idx] * h + mXyzB[idx]) * h + mXyzC[idx]) * h + mXyzPoints[idx];
			}


			glm::vec3 CubicSpline3D::derivation(const int order, const float x) const {
				assert(order > 0);

				const auto idx = getSubsplineIdx(x);
				const auto h = x - idx;	// Offset from index to x (index of subspline = f from f(x) = y)

				// Interpolation (or extrapolation to left/right)
				switch (order) {
					case 1: return (3.f * mXyzA[idx] * h + 2.f * mXyzB[idx]) * h + mXyzC[idx];
					case 2: return 6.f * mXyzA[idx] * h + 2.f * mXyzB[idx];
					case 3: return 6.f * mXyzA[idx];
					default: return {0.f, 0.f, 0.f};
				}
			}

			size_t CubicSpline3D::getSubsplineIdx(const float x) const {
				// Find the first smaller from {0, ..., (n - 2)} (=> cast, not floor)
				const auto n = int(pointsCount);
				const auto val = int(x);
				return val < 0 ? 0 : val < n - 1 ? val : n - 2; // Clamp (0 <= x < n - 1 => subspline index)
			}
		}
	}
}

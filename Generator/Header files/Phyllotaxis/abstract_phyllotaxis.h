/// Abstract base class for setting buds in nodes of tree (phyllotaxis).
/// \file abstract_phyllotaxis.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#ifndef ABSTRACT_PHYLLOTAXIS_H
#define ABSTRACT_PHYLLOTAXIS_H

#include <Tree primitives/tree_primitives.h>

namespace Treegen
{
	namespace Phyllotaxis
	{
		/// Abstract base class for setting buds in nodes of tree (phyllotaxis).
		class AbstractPhyllotaxis {
		protected:

			/// \name Angles and derived variables of phyllotaxis.
			///@{
			float mLateralAngle = 0.f;
			float mLateralAngleTan = 0.f;	///< Cached tangent based on lateral angle.
			float mPhyllotacticAngle = 0.f;
			///@}

			/// Generates terminal bud of non root node.
			/// \param target Node for which should bud be generated.
			/// \param from Bud from which \p target grew.
			virtual void generateInnerTerminalBud(TreeNode& target, TreeBud& from);

			/// Generates lateral buds in non root node.
			/// \param target Node for which should buds be generated.
			/// \param from Bud from which \p target grew.
			virtual void generateInnerLateralBuds(TreeNode& target, TreeBud& from) = 0;

		public:
			AbstractPhyllotaxis();
			AbstractPhyllotaxis(const AbstractPhyllotaxis&) = delete;
			AbstractPhyllotaxis& operator=(const AbstractPhyllotaxis&) = delete;
			AbstractPhyllotaxis(AbstractPhyllotaxis&&) = delete;
			AbstractPhyllotaxis& operator=(AbstractPhyllotaxis&&) = delete;
			virtual ~AbstractPhyllotaxis() = default;
			
			/// \name Basic properties of phyllotaxis.
			///@{
			virtual size_t lateralBudsCount() const = 0;
			float phyllotacticAngle() const { return mPhyllotacticAngle; }
			float lateralAngle() const { return mLateralAngle; }
			///@}

			/// Sets phyllotactic angle.
			/// \param angleRadians Phyllotactic angle in radians from range [0, 360] degrees.
			/// \warning For invalid argument throws std::invalid_argument exception.
			virtual void phyllotacticAngle(float angleRadians);

			/// Sets lateral bud agle.
			/// \param angleRadians Lateral bud angle in radians from range [0, 90] degrees.
			/// \warning For invalid argument throws std::invalid_argument exception.
			virtual void lateralAngle(float angleRadians);
			
			/// Generates buds in root of tree.
			/// \param root Root node of tree.
			/// \param seed Seed for setting root buds random generators.
			virtual void generateRootBuds(TreeNode& root, unsigned seed) = 0;

			/// Generates buds in non root node of tree.
			/// \param target Node for which should buds be generated.
			/// \param from Bud from which \p target grew.
			/// \warning Phyllotaxis may need to analyse properties of buds of preceding TreeNode.
			/// For that reason make sure that phyllotaxis was set on \p from before calling this method.
			virtual void generateInnerBuds(TreeNode& target, TreeBud& from);

			/// Prepares phyllotaxis to be used for new tree.
			virtual void clear() = 0;
		};
	}
}

#endif // ABSTRACT_PHYLLOTAXIS_H

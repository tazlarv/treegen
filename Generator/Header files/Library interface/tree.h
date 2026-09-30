/// Tree representation public API.
/// \file tree.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#ifndef TREE_H
#define TREE_H

#include "output_data_types.h"

#include <glm/vec3.hpp>

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
	enum class PhyllotacticType {
		Opposite,
		Alternate
	};

	/// Tree representation public API.
	class TREEGEN_API Tree {
	public:
		// ===========================================================================================================
		// Getters - model
		// ===========================================================================================================

		/// \name Getters of tree settings.
		///@{

		virtual unsigned treeId() const  = 0;
		virtual unsigned randomSeed() const = 0;
		
		virtual float bareTrunkLength() const = 0;
		virtual float maxShootLength() const = 0;
		virtual float biasToMain() const = 0;

		virtual PhyllotacticType phyllotacticType() const = 0;
		virtual float phyllotacticAngle() const = 0;
		virtual float lateralAngle() const = 0;

		virtual float gravimorphismHorizontal() const = 0;
		virtual float gravimorphismVertical() const = 0;
		virtual float gravimorphismUpDown() const = 0;

		virtual bool applyShedding() const = 0;
		virtual float sheddingThreshold() const = 0;
		virtual float lightSensitivity() const = 0;

		virtual float budDirectionWeight() const = 0;
		virtual float spaceDirectionWeight() const = 0;
		virtual float tropismDirectionWeight() const = 0;
		virtual float tropismAngle() const = 0;
		virtual bool applyTropismToTrunk() const = 0;
		virtual float trunkStraightness() const = 0;

		///@}

		// ===========================================================================================================
		// Getters - mesh
		// ===========================================================================================================

		/// \name Getters of mesh settings.
		///@{

		virtual float branchThickness() const = 0;
		virtual float pipeModelN() const = 0;
		virtual size_t subinternodes() const = 0;
		virtual size_t verticesOnCircumference() const = 0;
		virtual size_t textureWidth() const = 0;
		virtual size_t textureHeight() const = 0;
		virtual float textureInternodesCovered() const = 0;

		///@}

		// ===========================================================================================================
		// Setters - model
		// ===========================================================================================================

		/// \name Setters of tree settings.
		///@{

		/// Sets random seed for generation of tree.
		/// \param seed Random seed.
		virtual void randomSeed(unsigned seed) = 0;

		/// Sets length of trunk from root which wont have any lateral branches.
		/// \param length Non negative length of trunk without branches.
		/// \warning For invalid argument throws std::invalid_argument exception.
		virtual void bareTrunkLength(float length) = 0;

		/// Sets maximal length of shoot that grows in one iteration.
		/// \param length Positive maximal length of shoot.
		/// \warning For invalid argument throws std::invalid_argument exception.
		virtual void maxShootLength(float length) = 0;

		/// Sets preference of growing of main branches.
		/// \param bias Preference from range [0.0, 1.0]
		/// \warning For invalid argument throws std::invalid_argument exception.
		virtual void biasToMain(float bias) = 0;

		/// Sets the type of phyllotaxis.
		/// \param type Type of phyllotaxis.
		virtual void phyllotacticType(PhyllotacticType type) = 0;

		/// Sets phyllotactic angle.
		/// \param angleRadians Phyllotactic angle in radians from range [0, 360] degrees.
		/// \warning For invalid argument throws std::invalid_argument exception.
		virtual void phyllotacticAngle(float angleRadians) = 0;

		/// Sets lateral bud agle.
		/// \param angleRadians Lateral bud angle in radiants from range [0, 90] degrees.
		/// \warning For invalid argument throws std::invalid_argument exception.
		virtual void lateralAngle(float angleRadians) = 0;

		/// Sets horizontal gravimorphic preference.
		/// \param value Horizontal preference in range [0.0, 1.0].
		/// \warning For invalid argument throws std::invalid_argument exception.
		virtual void gravimorphismHorizontal(float value) = 0;

		/// Sets vertical gravimorphic preference.
		/// \param value Vertical preference from range [0.0, 1.0].
		/// \warning For invalid argument throws std::invalid_argument exception.
		virtual void gravimorphismVertical(float value) = 0;

		/// Sets gravimorphic preference of buds heading up on branch (positive) or down (negative).
		/// \param upDown Buds up or down heading preference. Value from range [-0.9, 0.9].
		/// \warning For invalid argument throws std::invalid_argument exception.
		virtual void gravimorphismUpDown(float upDown) = 0;

		/// Sets if shedding should be applied.
		/// \param applyShedding Flag - if shedding should be applied.
		virtual void applyShedding(bool applyShedding) = 0;

		/// Sets shedding threshold.
		/// \param threshold Shedding threshold value.
		virtual void sheddingThreshold(float threshold) = 0;

		/// Sets bud light sensitivity.
		/// \param sensitivity Non negative value of bud light sensitivity.
		/// \warning For invalid argument throws std::invalid_argument exception.
		virtual void lightSensitivity(float sensitivity) = 0;

		/// Sets weight which has bud direction on final growth direction.
		/// \param weight Non negative bud direction weight.
		/// \warning For invalid argument throws std::invalid_argument exception.
		virtual void budDirectionWeight(float weight) = 0;

		/// Sets weight which has direction towards free space on final growth direction.
		/// \param weight Non negative free space direction weight.
		/// \warning For invalid argument throws std::invalid_argument exception.
		virtual void spaceDirectionWeight(float weight) = 0;

		/// Sets weight which has tropism direction on final growth direction.
		/// \param weight Non negative tropism direction weight.
		/// \warning For invalid argument throws std::invalid_argument exception.
		virtual void tropismDirectionWeight(float weight) = 0;

		/// Sets angle of tropism.
		/// \param angleRadians Angle between vertical vector in direction upwards
		/// and tropism direction in radians from range [0, 180] degrees.
		/// \warning For invalid argument throws std::invalid_argument exception.
		virtual void tropismAngle(float angleRadians) = 0;

		/// Sets if tropism should be applied to trunk of tree.
		/// \param apply Flag - if tropism should be applied to trunk.
		virtual void applyTropismToTrunk(bool apply) = 0;

		/// Sets level of trunk straightness.
		/// \param straightness Level of straignes from range [0.0, 1.0].
		/// \warning For invalid argument throws std::invalid_argument exception.
		virtual void trunkStraightness(float straightness) = 0;
		
		///@}

		// ===========================================================================================================
		// Setters - mesh
		// ===========================================================================================================

		/// \name Setters of mesh settings.
		///@{

		/// Sets branch thickness.
		/// \param thickness Thickness of branch from range [0.001, 0.1].
		/// \warning For invalid argument throws std::invalid_argument exception.
		virtual void branchThickness(float thickness) = 0;

		/// Sets pipe model n.
		/// \param n Pipe model n from range [1.0, 3.0].
		/// \warning For invalid argument throws std::invalid_argument exception.
		virtual void pipeModelN(float n) = 0;

		/// Sets number of subinternodes.
		/// \param subinternodes Nonzero number of subinternodes.
		/// \warning For invalid argument throws std::invalid_argument exception.
		virtual void subinternodes(size_t subinternodes) = 0;

		/// Sets number of vertices on circumference of branch.
		/// \param vertices Even number of vertices (at least 4).
		/// \warning For invalid argument throws std::invalid_argument exception.
		virtual void verticesOnCircumference(size_t vertices) = 0;

		/// Sets texture width and height for UV mapping.
		/// \param width Texture width.
		/// \param height Texture height.
		virtual void textureSizes(size_t width, size_t height) = 0;

		/// Sets scaling of height of texture.
		/// \param internodesCovered How many internodes of tree should texture cover. Must be bigger than zero.
		/// \warning For invalid argument throws std::invalid_argument exception.
		virtual void textureInternodesCovered(float internodesCovered) = 0;

		///@}

		// ===========================================================================================================
		// Model observation
		// ===========================================================================================================

		/// \name Generated tree model observation.
		///@{

		/// Gets position of root of tree.
		/// \return Position of root in scene coordinates.
		virtual glm::vec3 rootScenePosition() const = 0;

		/// Gets number of nodes in tree structure.
		/// \return Number of nodes of tree structure.
		virtual size_t nodesCount() const = 0;

		/// Gets representation of tree structure from its nodes and connections between them.
		/// \return Representation of tree structure.
		virtual TreeStructure getTreeStructure() = 0;

		/// Gets representation of tree structure of shed part of tree in form of nodes and connections between them.
		/// \return Representation of shed part of tree.
		virtual TreeStructure getShedBranches() = 0;

		/// Makes mesh of tree.
		/// \return Vertices and triangles of tree mesh.
		virtual TreeMesh makeTreeMesh() = 0;

		/// Cancels making of mesh.
		/// Mesh making is stopped on first check for cancellation.
		/// After cancelled run is finished meshing may be started again.
		virtual void cancelMakeMesh() = 0;

		///@}

	protected:
		Tree() = default;
		Tree(const Tree&) = default;
		Tree& operator=(const Tree&) = default;
		Tree(Tree&&) = default;
		Tree& operator=(Tree&&) = default;
		~Tree() = default;
	};
}

#ifdef TREEGEN_DLL_EXPORT
#pragma warning(pop)
#endif

#endif // TREE_H

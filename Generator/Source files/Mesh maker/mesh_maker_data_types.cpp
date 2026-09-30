/// Basic classes for representation of tree structure during making of mesh.
/// \file mesh_maker_data_types.cpp
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#include <stdafx.h>
#include <Mesh maker/mesh_maker_data_types.h>

#include <Library interface/output_data_types.h>

namespace Treegen
{
	namespace MeshMaker
	{
		bool StructureNode::isRoot() const { return predecessor == nullptr; }
		bool StructureNode::isBranchTip() const { return following.empty(); }
		bool StructureNode::isBranchingNode() const { return following.size() > 1; }
		bool StructureNode::isInnerNode() const { return following.size() == 1; }

		void MeshBranch::calculateSpline() {
			if (!spline) spline = std::make_unique<Spline3D::CubicSpline3D>(structNodesPositions);
		}

		void MeshBranch::interpolateRadii() {
			// Add half of difference of radius difference between nodes 
			// in direction from base to top -> smoothing change of radius along branch
			for (int i = 1; i < structNodesRadii.size() - 1; ++i) {
				structNodesRadii[i] += (structNodesRadii[i - 1] - structNodesRadii[i]) / 2;
			}
		}

		void MeshBranch::collectBranchMesh(TreeMesh& output) {
			const auto node_vertices = meshNodes[0].size();

			// Triangles representing subinternodes
			for (int i = 0; i < meshNodes.size() - 1; ++i) {
				for (int left = 0; left < node_vertices; ++left) {
					const auto right = (left + 1) % node_vertices;
					output.triangles.emplace_back
						(*meshNodes[i + 1][left], *meshNodes[i][left], *meshNodes[i][right]);

					output.triangles.emplace_back
						(*meshNodes[i][right], *meshNodes[i + 1][right], *meshNodes[i + 1][left]);
				}
			}

			// Tip triangles
			for (int left = 0; left < node_vertices; ++left) {
				const auto right = (left + 1) % node_vertices;
				output.triangles.emplace_back(*meshTipVertex, *meshNodes.back()[left], *meshNodes.back()[right]);
			}

			// Move all alocated vertices to output
			output.vertices.push_back(std::move(meshTipVertex));
			for (int i = 0; i < meshNodes.size(); ++i) {
				output.vertices.insert(std::end(output.vertices),
					std::make_move_iterator(std::begin(meshNodes[i])),
					std::make_move_iterator(std::end(meshNodes[i])));
			}

			clearTreeMesh();
		}

		void MeshBranch::clearTreeMesh() {
			meshTipVertex.reset();
			meshNodes.clear();
		}
	}
}

namespace std
{
	size_t hash<Treegen::MeshMaker::StructureNode>::operator()(
		const Treegen::MeshMaker::StructureNode& key) const {
		return hash<Treegen::TreeNode>{}(key.represents);
	}
}

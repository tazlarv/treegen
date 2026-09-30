/// Class for meshing of tree structure.
/// \file mesh_maker.cpp
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#include <stdafx.h>
#include <Mesh maker/mesh_maker.h>

#include <Mesh maker/cubic_spline_3D.h>
#include <Mesh maker/mesh_maker_data_types.h>
#include <Library interface/output_data_types.h>
#include <Tree primitives/tree_primitives.h>

namespace Treegen
{
	namespace MeshMaker
	{
		void MeshMaker::sceneModelRatio(const float ratio) {
			if (ratio <= 0.f) { throw std::invalid_argument{"Scene - model ratio must be positive value!"}; }
			mSceneModelRatio = ratio;
		}

		void MeshMaker::branchThickness(const float thickness) {
			if (thickness < 0.001f || thickness > 0.1f) {
				throw std::invalid_argument{"Branch thickness must be in range 0.001 <= n <= 0.1!"};
			}
			mBranchRadiusMult = thickness;
		}

		void MeshMaker::pipeModelN(const float n) {
			if (n < 1.f || n > 3.f) { throw std::invalid_argument{"Pipe model n is not in range 1.0 <= n <= 3.0!"}; }
			mPipeModelN = n;
		}

		void MeshMaker::subinternodes(const size_t subinternodes) {
			if (subinternodes < 1) { throw std::invalid_argument{"Subinternodes must be positive value!"}; }
			mSubinternodes = subinternodes;
		}

		void MeshMaker::verticesInNode(const size_t verticesCount) {
			if (verticesCount % 2 != 0 || verticesCount == 2) {
				throw std::invalid_argument{"Count must be even number bigger than 2!"};
			}
			mVerticesInNode = verticesCount;
			mNodeRotationAngle = glm::two_pi<float>() / verticesCount;
		}

		void MeshMaker::textureSizes(const size_t width, const size_t height) {
			mTexSizes = {width, height};
			mTexWhRatio = float(width) / height;
		}

		void MeshMaker::textureInternodesCovered(const float internodesCovered) {
			if (internodesCovered <= 0.f) { throw std::invalid_argument{"Height per internode must be positive value!"}; }
			mTexInternodesCovered = internodesCovered;
		}

		void MeshMaker::rndSeed(const unsigned seed) { mRndSeed = seed; }

		TreeMesh MeshMaker::makeMesh(const rwConstTreeNode root, const unsigned treeId) {
			{
				std::lock_guard<std::mutex> lock{mRunCheckMutex};
				mIsRunning = true;
			}

			loadTreeStructure(root, treeId);
			if (isCancelledCheck()) { return returnOnCancel(); }
			makeBranchMeshes();
			if (isCancelledCheck()) { return returnOnCancel(); }
			auto meshOutput = collectTreeMeshData();
			if (isCancelledCheck()) { return returnOnCancel(); }
			clear();
			
			// Could be canceled in this place (must set both state variables under lock to default values)
			{
				std::lock_guard<std::mutex> lock{mRunCheckMutex};
				mIsRunning = false;
				mIsCancelled = false;
				return meshOutput;
			}
		}

		void MeshMaker::cancel() {
			std::lock_guard<std::mutex> lock{mRunCheckMutex};
			if (mIsRunning) { mIsCancelled = true; }
		}

		bool MeshMaker::isCancelledCheck() {
			std::lock_guard<std::mutex> lock{mRunCheckMutex};
			return mIsCancelled;
		}

		TreeMesh MeshMaker::returnOnCancel() {
			clear();
			std::lock_guard<std::mutex> lock{mRunCheckMutex};
			mIsRunning = false;
			mIsCancelled = false;
			return TreeMesh{0};
		}

		void MeshMaker::loadTreeStructure(rwConstTreeNode root, const unsigned treeId) {
			if (!root.get().isEndpoint()) { initStructures(root); } // Cannot make mesh on single node
			mTreeId = treeId;
		}

		void MeshMaker::initStructures(const TreeNode& root) {
			initTreeStructure(root);
			if (isCancelledCheck()) { return; }
			assignRadii();
			if (isCancelledCheck()) { return; }
			initStructureBranches();
			if (isCancelledCheck()) { return; }
			initMeshBranches();
		}

		void MeshMaker::initTreeStructure(const TreeNode& structureRoot) {
			std::stack<rwConstTreeNode> nodeStack{};
			std::stack<rwStructNode> axesStack{};
			std::unordered_map<rwConstTreeNode, rwStructNode, std::hash<TreeNode>> structureRepresentants{};

			auto root = std::make_unique<StructureNode>(structureRoot);
			mRoot = root.get();
			StructureNode& rootRef = *root;

			mStructNodes.push_back(std::move(root));
			structureRepresentants.emplace(structureRoot, rootRef);

			if (structureRoot.terminalFollower) {
				nodeStack.push(*structureRoot.terminalFollower);
				axesStack.push(rootRef);
			}

			for (auto&& follower : structureRoot.lateralFollowers) {
				nodeStack.push(follower.get());
				axesStack.push(rootRef);
			}

			while (!nodeStack.empty()) {
				const TreeNode& node = nodeStack.top();
				nodeStack.pop();

				StructureNode& predecessorRepresentant = structureRepresentants.at(*node.predecessor);
				auto representant = std::make_unique<StructureNode>(node);
				representant->predecessor = &predecessorRepresentant;
				predecessorRepresentant.following.push_back(*representant);

				const auto followersCnt = node.lateralFollowers.size() + (node.terminalFollower == nullptr ? 0 : 1);
				if (followersCnt == 0) {
					representant->precedingAxis = &axesStack.top().get();
					axesStack.pop();
					representant->precedingAxis->followingAxes.push_back(*representant);
					mTipNodes.push_back(*representant);
				}
				else if (followersCnt > 1) {
					representant->precedingAxis = &axesStack.top().get();
					axesStack.pop();
					representant->precedingAxis->followingAxes.push_back(*representant);
					mBranchingNodes.push_back(*representant);

					for (int i = 0; i < followersCnt; ++i) { axesStack.push(*representant); }
				}

				structureRepresentants.emplace(node, *representant);
				mStructNodes.push_back(std::move(representant));

				if (node.terminalFollower) { nodeStack.push(*node.terminalFollower); }
				for (auto&& followingNode : node.lateralFollowers) { nodeStack.push(followingNode.get()); }
			}
		}

		void MeshMaker::assignRadii() {
			std::unordered_map<rwStructNode, int, std::hash<StructureNode>> nodeVisitsCnt{};
			nodeVisitsCnt.reserve(mBranchingNodes.size());

			for (auto&& rwTipNode : mTipNodes) {
				auto radius = mTipRadius;
				StructureNode* currentNode = &rwTipNode.get();

				while (currentNode) {
					if (currentNode->isBranchingNode()) {
						currentNode->branchRadius += pow(radius, mPipeModelN);

						auto& visits = ++nodeVisitsCnt[*currentNode];
						if (visits != currentNode->following.size()) { break; }

						radius = pow(currentNode->branchRadius, 1.f / mPipeModelN);
						currentNode->branchRadius = radius;
					}
					else { currentNode->branchRadius = radius; }

					currentNode = currentNode->predecessor;
				}
			}
		}

		void MeshMaker::initStructureBranches() {
			std::stack<StructureBranch> unfinishedBranches{};

			auto follAxesSize = validFollAxesSize(mRoot->followingAxes);

			for (int i = 1; i < follAxesSize; ++i) {
				unfinishedBranches.emplace(*mRoot, mRoot->followingAxes[i], false);
			}

			// Push biggest radius on top => first to pop (continue current branch)
			unfinishedBranches.emplace(*mRoot, mRoot->followingAxes[0], true);

			while (!unfinishedBranches.empty()) {
				StructureBranch branch = unfinishedBranches.top();
				unfinishedBranches.pop();

				while (true) {
					StructureNode& branchingNode = branch.branchTip.get();
					follAxesSize = validFollAxesSize(branchingNode.followingAxes);

					if (follAxesSize == 0) { break; }

					for (int j = 1; j < follAxesSize; ++j) {
						unfinishedBranches.emplace(branchingNode, branchingNode.followingAxes[j], false);
					}
					branch.branchTip = branchingNode.followingAxes[0];
				}

				mStructBranches.push_back(std::move(branch));
			}
		}

		size_t MeshMaker::validFollAxesSize(std::vector<rwStructNode>& followingAxes) {
			// Move shed followers to end (we do not want to make mesh from them)
			const auto endIt = std::remove_if(std::begin(followingAxes), std::end(followingAxes),
				[](StructureNode& node) {
					return node.represents.shed;
				});

			// Sort rest (non shed) followers by radius in descending order (preceding branch follows biggest radius)
			std::sort(std::begin(followingAxes), endIt,
				[](StructureNode& a, StructureNode& b) {
					return a.branchRadius > b.branchRadius;
				});

			return endIt - std::begin(followingAxes);
		}

		void MeshMaker::initMeshBranches() {
			tbb::parallel_for(tbb::blocked_range<size_t>(0, mStructBranches.size()),
				[&](const tbb::blocked_range<size_t>& range) {
					for (auto i = range.begin(); i != range.end(); ++i) {
						StructureBranch& structBranch = mStructBranches[i];
						MeshBranch meshBranch{unsigned(mRndSeed + i), structBranch.isTrunk};
						std::vector<rwStructNode> branchNodes{};

						// Collect nodes
						auto rwStructureNode = structBranch.branchTip;
						branchNodes.push_back(rwStructureNode);
						do {
							rwStructureNode = *rwStructureNode.get().predecessor;
							branchNodes.push_back(rwStructureNode);
						} while (rwStructureNode != structBranch.branchBase);

						meshBranch.structNodesPositions.reserve(branchNodes.size());
						meshBranch.structNodesRadii.reserve(branchNodes.size());

						// In reverse order (from base to tip) insert them to meshBranch containers
						for (auto it = std::rbegin(branchNodes); it != std::rend(branchNodes); ++it) {
							meshBranch.structNodesPositions.push_back(it->get().represents.position);
							meshBranch.structNodesRadii.push_back(it->get().branchRadius * mBranchRadiusMult);
						}

						// Non trunk branch should not have radius in base node equal to branching point radius but equal to branch radius
						if (!structBranch.isTrunk) { meshBranch.structNodesRadii[0] = meshBranch.structNodesRadii[1]; }

						mMeshBranches.push_back(std::move(meshBranch));
					}
				});
		}


		void MeshMaker::makeBranchMeshes() {
			tbb::parallel_for(tbb::blocked_range<size_t>(0, mMeshBranches.size()),
				[&](const tbb::blocked_range<size_t>& range) {
					for (auto i = range.begin(); i != range.end(); ++i) {
						makeBranchMesh(mMeshBranches[i]);
					}
				});
		}

		void MeshMaker::makeBranchMesh(MeshBranch& branch) {
			// Interpolate multiple times for smoother transition of radius on branch
			for (auto i = 0; i < 3; i++) { branch.interpolateRadii(); }

			if (branch.structNodesPositions.size() == 2) { makeMeshShortBranch(branch); }
			else { makeMeshLongBranch(branch); }

			setTextureCoordinates(branch);
		}

		void MeshMaker::setTextureCoordinates(MeshBranch& branch) {
			// Precalc based on radius of base of branch for texture on diameter
			const auto& baseVertices = branch.meshNodes[0];
			const auto baseVertDistance = glm::length(baseVertices[1]->position - baseVertices[0]->position);
			const auto vertCnt = baseVertices.size();
			const auto halfVertCnt = vertCnt / 2;
			const auto halfBaseCircumference = baseVertDistance * halfVertCnt;
			const auto halfWhRatio = halfBaseCircumference * mSceneModelRatio;
			const auto oppositeS = (halfWhRatio / mTexWhRatio) / mTexInternodesCovered;

			std::vector<float> sCoordinates(vertCnt);
			sCoordinates[0] = 0.f;
			sCoordinates[halfVertCnt] = oppositeS;
			for (int j = 1; j < halfVertCnt; ++j) {
				const float s = (float(j) / halfVertCnt) * oppositeS;
				sCoordinates[j] = s;
				sCoordinates[vertCnt - j] = s;
			}

			// From t per internode (=> 1.f / mTexInternodesCovered) to t per subinternode
			const float tPerSubinternode = 1.f / (mTexInternodesCovered * mSubinternodes);

			for (int i = 0; i < branch.meshNodes.size(); ++i) {
				auto& nodeVertices = branch.meshNodes[i];
				const float t = tPerSubinternode * i;

				for (int j = 0; j < vertCnt; ++j) {
					nodeVertices[j]->textureCoordinates = {sCoordinates[j], t};
				}
			}

			// For t => t coordinate of last node + distance covered)
			branch.meshTipVertex->textureCoordinates = {
				0.f, ((branch.meshNodes.size() - 1) * tPerSubinternode) +
				     (1.414f * branch.structNodesRadii.back() * mSceneModelRatio) / mTexInternodesCovered
			};
		}

		void MeshMaker::makeMeshShortBranch(MeshBranch& branch) {
			assert(branch.structNodesPositions.size() == 2);

			branch.meshNodes.reserve(mSubinternodes + 1); // + 1 for tip

			const auto upDownVec = branch.structNodesPositions[1] - branch.structNodesPositions[0];
			const auto tangent = glm::normalize(upDownVec);
			const auto normal = Utils::getRndOrthogonalNormalized(upDownVec, branch.rndEngine);
				
			// Trunk vs branch base vertices
			const auto baseTangent = branch.isTrunk ? glm::vec3{0.f, 1.f, 0.f} : tangent;
			const auto baseNormal = branch.isTrunk
				                        ? Utils::rotateVecBasedOnChangedOrientation(tangent, baseTangent, normal)
				                        : normal;

			branch.meshNodes.emplace_back(makeVerticesAtPosition(
				branch.structNodesPositions[0], baseTangent, baseNormal, branch.structNodesRadii[0]));

			// Subinternodes
			for (int i = 1; i <= mSubinternodes; ++i) {
				const auto percent = i / float(mSubinternodes);
				const auto position = branch.structNodesPositions[0] + (percent * upDownVec);
				const auto radius = (1.f - percent) * branch.structNodesRadii[0] + percent * branch.structNodesRadii[1];
				branch.meshNodes.emplace_back(makeVerticesAtPosition(position, tangent, normal, radius));
			}

			// Branch tip vertex
			branch.meshTipVertex = std::make_unique<MeshVertex>(
				branch.structNodesPositions[1] + tangent * branch.structNodesRadii[1]);
		}

		void MeshMaker::makeMeshLongBranch(MeshBranch& branch) {
			assert(branch.structNodesPositions.size() > 2);

			branch.meshNodes.reserve(((branch.structNodesPositions.size() - 1) * mSubinternodes) + 1);
			branch.calculateSpline();
		
			// Trunk vs branch base vertices
			auto lastTangent = branch.isTrunk
				                   ? glm::vec3{0.f, 1.f, 0.f}
				                   : glm::normalize(branch.spline->derivation(1, 0.f));
			auto lastNormal = Utils::getRndOrthogonalNormalized(lastTangent, branch.rndEngine);

			branch.meshNodes.emplace_back(makeVerticesAtPosition(
				branch.structNodesPositions[0], lastTangent, lastNormal, branch.structNodesRadii[0]));

			// For each node (not base one) generate its subinternode vertices (BASE: sub sub node ... etc for each branch node)
			// BASE node is generated as last subinternode of previous segment
			for (int i = 0; i < branch.structNodesPositions.size() - 1; ++i) {
				for (int j = 1; j <= mSubinternodes; ++j) {
					const auto percent = j / float(mSubinternodes);
					const auto splineIdx = i + percent;
					const auto newTangent = glm::normalize(branch.spline->derivation(1, splineIdx));
					const auto position = (*branch.spline)(splineIdx);
					const auto radius = (1.0f - percent) * branch.structNodesRadii[i] +
					                    percent * branch.structNodesRadii[i + 1];

					lastNormal = Utils::rotateVecBasedOnChangedOrientation(lastTangent, newTangent, lastNormal);
					lastTangent = newTangent;

					branch.meshNodes.emplace_back(makeVerticesAtPosition(position, lastTangent, lastNormal, radius));
				}
			}

			// Branch tip vertex
			branch.meshTipVertex = std::make_unique<MeshVertex>(
				branch.structNodesPositions.back() + branch.structNodesRadii.back());
		}

		std::vector<std::unique_ptr<MeshVertex>> MeshMaker::makeVerticesAtPosition(
			const glm::vec3& position, const glm::vec3& tangentUp, const glm::vec3& normal, const float radius) const {

			std::vector<std::unique_ptr<MeshVertex>> vertices{};
			vertices.reserve(mVerticesInNode);

			const auto rotationMatrix = GlmUtils::rotationMatrix(mNodeRotationAngle, tangentUp);

			auto rotatedNormal = radius * glm::normalize(normal);
			vertices.push_back(std::make_unique<MeshVertex>(position + rotatedNormal));

			for (int i = 0; i < mVerticesInNode - 1; ++i) {
				rotatedNormal = rotationMatrix * rotatedNormal;
				vertices.push_back(std::make_unique<MeshVertex>(position + rotatedNormal));
			}

			return vertices;
		}

		TreeMesh MeshMaker::collectTreeMeshData() {
			// Output and reserve proper size so we do not grow vectors (move elements around)
			TreeMesh meshData{mTreeId};

			size_t nodeCnt = 0;
			for (auto&& meshBranch : mMeshBranches) { nodeCnt += meshBranch.meshNodes.size(); }
			const auto branchCnt = mMeshBranches.size();

			const auto totalVertices = (nodeCnt * mVerticesInNode) + branchCnt;
			const auto totalTriangles = ((nodeCnt - branchCnt) * mVerticesInNode * 2) + branchCnt * mVerticesInNode;

			meshData.vertices.reserve(totalVertices);
			meshData.triangles.reserve(totalTriangles);

			// Collect vertices and triangles
			for (auto&& meshBranch : mMeshBranches) { meshBranch.collectBranchMesh(meshData); }

			if (isCancelledCheck()) { return TreeMesh{0}; }

			// Postprocess - indices + normals
			calculateVertexProperties(meshData);

			return meshData;
		}

		void MeshMaker::calculateVertexProperties(const TreeMesh& data) {
			// Calculate vertex normals and tangents based on mesh triangles
			tbb::parallel_for(tbb::blocked_range<size_t>(0, data.triangles.size()),
				[&](const tbb::blocked_range<size_t>& range) {
					for (auto i = range.begin(); i != range.end(); ++i) {
						// Set vertex normals based on normalized sum of triangle normals
						auto& triangle = data.triangles[i];

						auto& v1 = triangle.vertices[0].get();
						auto& v2 = triangle.vertices[1].get();
						auto& v3 = triangle.vertices[2].get();

						// Normal calculation
						const auto e1 = v2.position - v1.position;
						const auto e2 = v3.position - v1.position;

						const auto normal = glm::cross(e1, e2);

						v1.normal += normal;
						v2.normal += normal;
						v3.normal += normal;

						// Tangent calculation
						const auto deltaTex1 = v2.textureCoordinates - v1.textureCoordinates;
						const auto deltaTex2 = v3.textureCoordinates - v1.textureCoordinates;

						const auto f = 1.f / (deltaTex1.s * deltaTex2.t - deltaTex2.s * deltaTex1.t);
						const auto tangent = glm::normalize(f * (deltaTex2.t * e1 - deltaTex1.t * e2));

						v1.tangent += tangent;
						v2.tangent += tangent;
						v3.tangent += tangent;
					}
				});

			// Normalize normals and tangents
			// Set ids -> can be used when sending triangles and vertices to opengl buffer
			tbb::parallel_for(tbb::blocked_range<size_t>(0, data.vertices.size()),
				[&](const tbb::blocked_range<size_t>& range) {
					for (auto i = range.begin(); i != range.end(); ++i) {
						auto& uqVertex = data.vertices[i];
						uqVertex->Idx = unsigned(i);
						uqVertex->normal = glm::normalize(uqVertex->normal);
						uqVertex->tangent = glm::normalize(uqVertex->tangent);
					}
				});
		}

		void MeshMaker::clear() {
			mStructNodes.clear();
			mStructBranches.clear();
			mRoot = nullptr;
			mBranchingNodes.clear();
			mTipNodes.clear();
			mMeshBranches.clear();
		}
	}
}

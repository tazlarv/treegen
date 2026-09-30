/// Widget for rendering of OpenGL of TreeGen application.
/// This file uses Qt 5.10.1 - see Licenses in folder External libraries.
/// \file tree_renderGL_widget.cpp
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#include <stdafx.h>
#include <tree_renderGL_widget.h>

#include <Treegen management/treegen_management.h>
#include <Camera/scene_camera.h>

#include <QtWidgets/qopenglwidget.h>
#include <QtGui/qopenglfunctions.h>

TreeRenderGLWidget::TreeRenderGLWidget(QWidget* parent) {
	Q_UNUSED(parent)
}

TreeRenderGLWidget::~TreeRenderGLWidget() {
	makeCurrent();

	mShaderStructure.reset();
	mShaderMesh.reset();

	mVaoBoundingBox.destroy();
	mVboBoundingBox.destroy();

	mVaoTreeRoots.destroy();
	mVboTreeRoots.destroy();

	mVaoTreeNodesA.destroy();
	mVboTreeNodesA.destroy();

	mVaoTreeNodesB.destroy();
	mVboTreeNodesB.destroy();

	mVaoTreeInternodesA.destroy();
	mVboTreeInternodesA.destroy();

	mVaoTreeInternodesB.destroy();
	mVboTreeInternodesB.destroy();

	mVaoSceneGeometry.destroy();
	mVboPositionsSceneGeometry.destroy();
	mVboNormalsSceneGeometry.destroy();
	mEboSceneGeometry.destroy();

	mVaoTreeMeshA.destroy();
	mVboTreeMeshA.destroy();
	mEboTreeMeshA.destroy();

	mVaoTreeMeshB.destroy();
	mVboTreeMeshB.destroy();
	mEboTreeMeshB.destroy();

	if (mBarkDiffuse) { mBarkDiffuse->destroy(); }

	doneCurrent();
}

void TreeRenderGLWidget::setTreegenAccess(std::shared_ptr<TreegenLib> treegenAccess) {
	mTreegen = std::move(treegenAccess);
	onSceneChange();
}


void TreeRenderGLWidget::loadDiffuseTexture(const QImage& diffuseTexture) {
	makeCurrent();

	if (mBarkDiffuse) { mBarkDiffuse->destroy(); }
	mBarkDiffuse = std::make_unique<QOpenGLTexture>(diffuseTexture.mirrored());
	mBarkDiffuse->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
	mBarkDiffuse->setMagnificationFilter(QOpenGLTexture::Linear);
	for (auto&& rwTree : mTreegen->trees) {
		rwTree.get().textureSizes(mBarkDiffuse->width(), mBarkDiffuse->height());
	}

	doneCurrent();
	update();
}

void TreeRenderGLWidget::clearDiffuseTexture() {
	makeCurrent();
	mBarkDiffuse.reset();
	doneCurrent();
	update();
}

void TreeRenderGLWidget::loadBoundingBox() {
	mBoundingBoxLoaded = false;
	update();
}

void TreeRenderGLWidget::loadRootsPositions() {
	makeCurrent();

	mRootsPositionsCnt = mTreegen->trees.size();

	std::vector<glm::vec3> rootsPositions{};
	rootsPositions.reserve(mRootsPositionsCnt);
	for (auto&& rwTree : mTreegen->trees) { rootsPositions.push_back(rwTree.get().rootScenePosition()); }

	mVboTreeRoots.bind();
	mVboPositionsSceneGeometry.allocate(rootsPositions.data(), int(sizeof(float) * 3 * rootsPositions.size()));

	doneCurrent();
	update();
}


void TreeRenderGLWidget::loadScene(std::vector<float> vertexPos, std::vector<float> vertexNormal,
                                   std::vector<unsigned> triangleIndices) {

	makeCurrent();
	mSceneTrianglesCnt = triangleIndices.size() / 3;

	mVboPositionsSceneGeometry.bind();
	mVboPositionsSceneGeometry.allocate(vertexPos.data(), int(sizeof(float) * vertexPos.size()));

	mVboNormalsSceneGeometry.bind();
	mVboNormalsSceneGeometry.allocate(vertexNormal.data(), int(sizeof(float) * vertexNormal.size()));

	mEboSceneGeometry.bind();
	mEboSceneGeometry.allocate(triangleIndices.data(), int(sizeof(unsigned) * triangleIndices.size()));

	mNonDefaultSceneLoaded = true;

	doneCurrent();
	update();
}

void TreeRenderGLWidget::loadDefaultEmptyScene() {
	makeCurrent();

	mSceneTrianglesCnt = 0;

	mVboPositionsSceneGeometry.bind();
	mVboPositionsSceneGeometry.allocate(0);

	mEboSceneGeometry.bind();
	mEboSceneGeometry.allocate(0);

	mNonDefaultSceneLoaded = false;
	mBoundingBoxLoaded = false;

	doneCurrent();
	update();
}


void TreeRenderGLWidget::onSceneChange() {
	setLightPosition();
	resetCamera();
}

void TreeRenderGLWidget::setLightPosition() {
	glm::vec3 sceneMin, sceneMax;
	std::tie(sceneMin, sceneMax) = mTreegen->generator->sceneBounds();
	const auto sceneDelta = sceneMax - sceneMin;
	const auto max = std::max(sceneDelta.x, std::max(sceneDelta.y, sceneDelta.z));
	mLightPosition = qVecGlmVec3D(sceneMax + max);
}


void TreeRenderGLWidget::resetCamera() {
	glm::vec3 sceneMin, sceneMax;
	if (mNonDefaultSceneLoaded) {
		std::tie(sceneMin, sceneMax) = mTreegen->generator->sceneBounds();
	}
	else { std::tie(sceneMin, sceneMax) = mTreegen->generator->boundingBoxBounds(); }

	const glm::vec3 sceneRanges = sceneMax - sceneMin;
	const float maxSceneRange = std::max(sceneRanges.x, std::max(sceneRanges.y, sceneMin.z));

	mCamera = Camera::SceneCamera{};
	const glm::vec3 cameraPos = {
		sceneMin.x + sceneRanges.x / 2.f,
		sceneMin.y + sceneRanges.y / 4.f,
		sceneMin.z + sceneRanges.z * 1.5f
	};

	mCamera.position = cameraPos;
	mCamera.minLookAtDistance = 0.01f;
	mCamera.maxLookAtDistance = maxSceneRange > 100000.f ? maxSceneRange : 100000.f;
	mCamera.lookAtDistance = sceneRanges.z;
	mCamera.updateCameraVectors();

	mCamera.mouseSensitivity *= 0.1f;
	mCamera.scrollSensitivity = 0.02f;

	mCamera.minMovementSpeed = 0.05f;
	mCamera.maxMovementSpeed = mCamera.movSpeedFromLookupDist(mCamera.maxLookAtDistance);
	mCamera.movementSpeed = glm::clamp(mCamera.movSpeedFromLookupDist(mCamera.lookAtDistance),
		mCamera.minMovementSpeed, mCamera.maxMovementSpeed);

	update();
}

void TreeRenderGLWidget::showBoundingBox(const bool show) {
	mShowBoundingBox = show;
	update();
}

void TreeRenderGLWidget::showTreeStructure() {
	mShowMesh = false;
	update();
}

void TreeRenderGLWidget::showMesh() {
	mShowMesh = true;
	mShowMeshStructure = false;
	update();
}

void TreeRenderGLWidget::showMeshStructure() {
	mShowMesh = true;
	mShowMeshStructure = true;
	update();
}


std::tuple<void*, void*, void*, void*> TreeRenderGLWidget::getMappedBuffers(
	const size_t nodes, const size_t internodesVertices, const size_t meshVertices, const size_t meshTriangles) {
	makeCurrent();

	QOpenGLBuffer& vboNodes = mRenderBuffers == Buffers::A ? mVboTreeNodesB : mVboTreeNodesA;
	QOpenGLBuffer& vboTreeInternodes = mRenderBuffers == Buffers::A ? mVboTreeInternodesB : mVboTreeInternodesA;
	QOpenGLBuffer& vboTreeMesh = mRenderBuffers == Buffers::A ? mVboTreeMeshB : mVboTreeMeshA;
	QOpenGLBuffer& eboTreeMesh = mRenderBuffers == Buffers::A ? mEboTreeMeshB : mEboTreeMeshA;

	vboNodes.bind();
	vboNodes.allocate(int(nodes * 3 * sizeof(float)));
	void* const nodesMap = vboNodes.map(QOpenGLBuffer::WriteOnly);

	vboTreeInternodes.bind();
	vboTreeInternodes.allocate(int(internodesVertices * 3 * sizeof(float)));
	void* const internodeMap = vboTreeInternodes.map(QOpenGLBuffer::WriteOnly);

	vboTreeMesh.bind();
	vboTreeMesh.allocate(int(meshVertices * 11 * sizeof(float)));
	void* const treeVerticesMap = vboTreeMesh.map(QOpenGLBuffer::WriteOnly);

	eboTreeMesh.bind();
	eboTreeMesh.allocate(int(meshTriangles * 3 * sizeof(unsigned)));
	void* const treeTrianglesMap = eboTreeMesh.map(QOpenGLBuffer::WriteOnly);
	eboTreeMesh.release();

	mTreeNodesSwappedCnt = nodes;
	mInternodesVerticesSwappedCnt = internodesVertices;
	mTreeMeshTrianglesSwappedCnt = meshTriangles;

	doneCurrent();

	return {nodesMap, internodeMap, treeVerticesMap, treeTrianglesMap};
}

void TreeRenderGLWidget::swapBuffers() {
	makeCurrent();

	QOpenGLBuffer& vboNodes = mRenderBuffers == Buffers::A ? mVboTreeNodesB : mVboTreeNodesA;
	QOpenGLBuffer& vboTreeInternodes = mRenderBuffers == Buffers::A ? mVboTreeInternodesB : mVboTreeInternodesA;
	QOpenGLBuffer& vboTreeMesh = mRenderBuffers == Buffers::A ? mVboTreeMeshB : mVboTreeMeshA;
	QOpenGLBuffer& eboTreeMesh = mRenderBuffers == Buffers::A ? mEboTreeMeshB : mEboTreeMeshA;

	vboNodes.bind();
	vboNodes.unmap();

	vboTreeInternodes.bind();
	vboTreeInternodes.unmap();

	vboTreeMesh.bind();
	vboTreeMesh.unmap();

	eboTreeMesh.bind();
	eboTreeMesh.unmap();
	eboTreeMesh.release();

	mTreeNodesCnt = mTreeNodesSwappedCnt;
	mInternodesVerticesCnt = mInternodesVerticesSwappedCnt;
	mTreeMeshTrianglesCnt = mTreeMeshTrianglesSwappedCnt;

	mVaoTreeNodesCurrent = mRenderBuffers == Buffers::A ? &mVaoTreeNodesB : &mVaoTreeNodesA;
	mVaoTreeInternodesCurrent = mRenderBuffers == Buffers::A ? &mVaoTreeInternodesB : &mVaoTreeInternodesA;
	mVaoTreeMeshCurrent = mRenderBuffers == Buffers::A ? &mVaoTreeMeshB : &mVaoTreeMeshA;

	mRenderBuffers = mRenderBuffers == Buffers::A ? Buffers::B : Buffers::A;

	doneCurrent();
	update();
}

void TreeRenderGLWidget::clearTreesBuffers() {
	mRootsPositionsCnt = 0;
	mTreeNodesCnt = 0;
	mInternodesVerticesCnt = 0;
	mTreeMeshTrianglesCnt = 0;
	update();
}

// ===========================================================================================================
// Internals - rendering, initialize openGL
// ===========================================================================================================

void TreeRenderGLWidget::initializeGL() {
	initializeOpenGLFunctions();

	mShaderStructure = std::make_unique<QOpenGLShaderProgram>(this);
	bool shaderCompile = mShaderStructure->addShaderFromSourceFile(
		QOpenGLShader::Vertex, R"###(:/treegen_gui/shaders/TreeGen structure.vs)###");
	if (!shaderCompile) { mShadersLoaded = false; }

	shaderCompile = mShaderStructure->addShaderFromSourceFile(
		QOpenGLShader::Fragment, R"###(:/treegen_gui/shaders/TreeGen structure.fs)###");
	if (!shaderCompile) { mShadersLoaded = false; }

	shaderCompile = mShaderStructure->link();
	if (!shaderCompile) { mShadersLoaded = false; }

	mShaderStructure->bind();
	mShStructureModelUn = mShaderStructure->uniformLocation("model");
	mShStructureViewUn = mShaderStructure->uniformLocation("view");
	mShStructureProjectionUn = mShaderStructure->uniformLocation("projection");
	mShStructureFarPlaneCoefUn = mShaderStructure->uniformLocation("farPlaneCoef");
	mShStructureColorUn = mShaderStructure->uniformLocation("color");

	mShaderMesh = std::make_unique<QOpenGLShaderProgram>(this);
	shaderCompile = mShaderMesh->addShaderFromSourceFile(
		QOpenGLShader::Vertex, R"###(:/treegen_gui/shaders/TreeGen mesh.vs)###");
	if (!shaderCompile) { mShadersLoaded = false; }

	shaderCompile = mShaderMesh->addShaderFromSourceFile(
		QOpenGLShader::Fragment, R"###(:/treegen_gui/shaders/TreeGen mesh.fs)###");
	if (!shaderCompile) { mShadersLoaded = false; }

	shaderCompile = mShaderMesh->link();
	if (!shaderCompile) { mShadersLoaded = false; }

	mShaderMesh->bind();
	mShMeshModelUn = mShaderMesh->uniformLocation("model");;
	mShMeshViewUn = mShaderMesh->uniformLocation("view");
	mShMeshProjectionUn = mShaderMesh->uniformLocation("projection");
	mShMeshFarPlaneCoefUn = mShaderMesh->uniformLocation("farPlaneCoef");

	mShMeshLightPosUn = mShaderMesh->uniformLocation("light_pos");
	mShMeshViewPosUn = mShaderMesh->uniformLocation("view_pos");
	mShMeshLightAmbientUn = mShaderMesh->uniformLocation("light.ambient");
	mShMeshLightDiffuseUn = mShaderMesh->uniformLocation("light.diffuse");
	mShMeshLightSpecularUn = mShaderMesh->uniformLocation("light.specular");

	mShMeshMaterialAmbientUn = mShaderMesh->uniformLocation("material.ambient");
	mShMeshMaterialDiffuseUn = mShaderMesh->uniformLocation("material.diffuse");
	mShMeshMaterialSpecularUn = mShaderMesh->uniformLocation("material.specular");
	mShMeshMaterialShininessUn = mShaderMesh->uniformLocation("material.shininess");
	mShMeshUseDiffuseMapUn = mShaderMesh->uniformLocation("use_diffuse_map");
	mShMeshDiffuseMapUn = mShaderMesh->uniformLocation("diffuse_map");

	// Buffers creation
	mVaoBoundingBox.create();
	mVboBoundingBox.create();

	mVaoTreeRoots.create();
	mVboTreeRoots.create();

	mVaoTreeNodesA.create();
	mVboTreeNodesA.create();

	mVaoTreeNodesB.create();
	mVboTreeNodesB.create();

	mVaoTreeInternodesA.create();
	mVboTreeInternodesA.create();

	mVaoTreeInternodesB.create();
	mVboTreeInternodesB.create();

	mVaoSceneGeometry.create();
	mVboPositionsSceneGeometry.create();
	mVboNormalsSceneGeometry.create();
	mEboSceneGeometry.create();

	mVaoTreeMeshA.create();
	mVboTreeMeshA.create();
	mEboTreeMeshA.create();

	mVaoTreeMeshB.create();
	mVboTreeMeshB.create();
	mEboTreeMeshB.create();

	// Boundung box cube
	mVaoBoundingBox.bind();
	mVboBoundingBox.bind();
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
	glEnableVertexAttribArray(0);

	// Root positions
	mVaoTreeRoots.bind();
	mVboTreeRoots.bind();
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
	glEnableVertexAttribArray(0);

	// Scene geometry
	mVaoSceneGeometry.bind();
	mEboSceneGeometry.bind();

	mVboPositionsSceneGeometry.bind();
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
	glEnableVertexAttribArray(0);

	mVboNormalsSceneGeometry.bind();
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
	glEnableVertexAttribArray(2);

	// Tree nodes
	mVaoTreeNodesA.bind();
	mVboTreeNodesA.bind();
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
	glEnableVertexAttribArray(0);

	mVaoTreeNodesB.bind();
	mVboTreeNodesB.bind();
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
	glEnableVertexAttribArray(0);

	// Tree lines
	mVaoTreeInternodesA.bind();
	mVboTreeInternodesA.bind();
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
	glEnableVertexAttribArray(0);

	mVaoTreeInternodesB.bind();
	mVboTreeInternodesB.bind();
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
	glEnableVertexAttribArray(0);

	// Tree geometry
	mVaoTreeMeshA.bind();
	mVboTreeMeshA.bind();
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), nullptr);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float),
		reinterpret_cast<void*>(3 * sizeof(float)));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float),
		reinterpret_cast<void*>(5 * sizeof(float)));
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float),
		reinterpret_cast<void*>(8 * sizeof(float)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	mEboTreeMeshA.bind();

	mVaoTreeMeshB.bind();
	mVboTreeMeshB.bind();
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), nullptr);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float),
		reinterpret_cast<void*>(3 * sizeof(float)));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float),
		reinterpret_cast<void*>(5 * sizeof(float)));
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float),
		reinterpret_cast<void*>(8 * sizeof(float)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	mEboTreeMeshB.bind();
	mVaoTreeMeshB.release();
}

void TreeRenderGLWidget::resizeGL(const int w, const int h) {
	if (w != 0 && h != 0) {
		mProjectionMatrix =
			glm::perspective(glm::radians(mCamera.fieldOfViewAngle), w / static_cast<float>(h), mNearPlane, mFarPlane);
		mRayCastingProjectionMatrix =
			glm::perspective(glm::radians(mCamera.fieldOfViewAngle), w / static_cast<float>(h), 1.f, 10.f);
	}
}

void TreeRenderGLWidget::paintGL() {
	if (width() == 0 || height() == 0) return;

	glPointSize(3.f);
	glm::vec3 mBackgroundColor = {0.5f, 0.7f, 0.85f};

	glEnable(GL_DEPTH_TEST);
	glClearColor(mBackgroundColor.r, mBackgroundColor.g, mBackgroundColor.b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	const auto viewMatrix = mCamera.getViewMatrix();

	mShaderStructure->bind();
	mShaderStructure->setUniformValue(mShStructureModelUn, QMatrix4x4{});
	glUniformMatrix4fv(mShStructureViewUn, 1, GL_FALSE, glm::value_ptr(viewMatrix));
	glUniformMatrix4fv(mShStructureProjectionUn, 1, GL_FALSE, glm::value_ptr(mProjectionMatrix));
	mShaderStructure->setUniformValue(mShStructureFarPlaneCoefUn, mfarPlaneCoef);

	// Bounding box
	if (mShowBoundingBox && mTreegen->generator->boundingBoxDefined()) {
		if (!mBoundingBoxLoaded) { bufferBoundingBox(); }
		mShaderStructure->setUniformValue(mShStructureColorUn, QVector3D{0.f, 0.f, 0.f});
		mVaoBoundingBox.bind();
		glDrawArrays(GL_LINES, 0, 24);
		mVaoBoundingBox.release();
	}

	// Base settings
	mShaderMesh->bind();
	mShaderMesh->setUniformValue(mShMeshModelUn, QMatrix4x4{});
	glUniformMatrix4fv(mShMeshViewUn, 1, GL_FALSE, glm::value_ptr(viewMatrix));
	glUniformMatrix4fv(mShMeshProjectionUn, 1, GL_FALSE, glm::value_ptr(mProjectionMatrix));
	mShaderMesh->setUniformValue(mShMeshFarPlaneCoefUn, mfarPlaneCoef);

	mShaderMesh->setUniformValue(mShMeshLightPosUn, mLightPosition);
	mShaderMesh->setUniformValue(mShMeshViewPosUn, qVecGlmVec3D(mCamera.position));

	mShaderMesh->setUniformValue(mShMeshLightAmbientUn, QVector3D{0.35f, 0.35f, 0.35f});
	mShaderMesh->setUniformValue(mShMeshLightDiffuseUn, QVector3D{1.f, 1.f, 1.f});
	mShaderMesh->setUniformValue(mShMeshLightSpecularUn, QVector3D{1.f, 1.f, 1.f});

	// Scene mesh
	if (mNonDefaultSceneLoaded) {
		mShaderMesh->setUniformValue(mShMeshMaterialAmbientUn, QVector3D{0.8f, 0.8f, 0.8f});
		mShaderMesh->setUniformValue(mShMeshMaterialDiffuseUn, QVector3D{0.8f, 0.8f, 0.8f});
		mShaderMesh->setUniformValue(mShMeshMaterialSpecularUn, QVector3D{0.f, 0.f, 0.f});
		mShaderMesh->setUniformValue(mShMeshMaterialShininessUn, 0.f);

		mShaderMesh->setUniformValue(mShMeshUseDiffuseMapUn, int(false));

		mVaoSceneGeometry.bind();
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDrawElements(GL_TRIANGLES, GLsizei(mSceneTrianglesCnt * 3), GL_UNSIGNED_INT, nullptr);
		mVaoSceneGeometry.release();
	}

	if (mShowMesh) {
		if (mShowMeshStructure) { glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); }
		else { glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); }

		if (mBarkDiffuse) {
			mShaderMesh->setUniformValue(mShMeshUseDiffuseMapUn, int(true));
			mShaderMesh->setUniformValue(mShMeshDiffuseMapUn, 0);
			mBarkDiffuse->bind(0);
		}
		else {
			mShaderMesh->setUniformValue(mShMeshUseDiffuseMapUn, int(false));

			mShaderMesh->setUniformValue(mShMeshMaterialAmbientUn, QVector3D{0.545f, 0.271f, 0.075f});
			mShaderMesh->setUniformValue(mShMeshMaterialDiffuseUn, QVector3D{0.545f, 0.271f, 0.075f});
			mShaderMesh->setUniformValue(mShMeshMaterialSpecularUn, QVector3D{0.2f, 0.2f, 0.2f});
			mShaderMesh->setUniformValue(mShMeshMaterialShininessUn, 32.f);
		}

		mVaoTreeMeshCurrent->bind();
		glDrawElements(GL_TRIANGLES, GLsizei(mTreeMeshTrianglesCnt * 3), GL_UNSIGNED_INT, nullptr);
		mVaoTreeMeshA.release();
	}
	else if (!mShowMesh) {
		mShaderStructure->bind();

		mVaoTreeNodesCurrent->bind();
		mShaderStructure->setUniformValue(mShStructureColorUn, QVector3D{0.8f, 0.f, 0.f});
		glDrawArrays(GL_POINTS, 0, GLsizei(mTreeNodesCnt));

		mVaoTreeRoots.bind();
		glDrawArrays(GL_POINTS, 0, GLsizei(mRootsPositionsCnt));

		mVaoTreeInternodesCurrent->bind();
		mShaderStructure->setUniformValue(mShStructureColorUn, QVector3D{1.f, 1.f, 1.f});
		glDrawArrays(GL_LINES, 0, GLsizei(mInternodesVerticesCnt));

		mVaoTreeNodesA.release();
	}

	glDisable(GL_DEPTH_TEST);
}


std::tuple<bool, glm::vec3> TreeRenderGLWidget::getTreeRootPosition() {
	// Reverse transformation from viewport position to direction from camera to that position => unProject
	// Ask generator for root position (casting ray)

	const auto viewportSizes = this->size();
	auto localPos = mapFromGlobal(QCursor::pos());
	localPos.setY(viewportSizes.height() - localPos.y());

	const glm::vec3 winFar = {localPos.x(), localPos.y(), 1.f};
	const glm::vec4 viewport = {0.f, 0.f, viewportSizes.width(), viewportSizes.height()};
	const auto viewMatrix = mCamera.getViewMatrix();

	const auto worldFar = glm::unProject(winFar, viewMatrix, mRayCastingProjectionMatrix, viewport);
	const auto rayDirection = worldFar - mCamera.position;

	return mTreegen->generator->tryGetRootPosition(worldFar, rayDirection, mFarPlane);
}

void TreeRenderGLWidget::bufferBoundingBox() {
	glm::vec3 lower, upper;
	std::tie(lower, upper) = mTreegen->generator->boundingBoxBounds();
	openGL_utils::bufferCube(mVboBoundingBox, lower, upper);
	mBoundingBoxLoaded = true;
}

// ===========================================================================================================
// Events
// ===========================================================================================================

void TreeRenderGLWidget::enterEvent(QEvent* qEvent) {
	setFocus();
	qEvent->accept();
}

void TreeRenderGLWidget::leaveEvent(QEvent* qEvent) {
	clearFocus();
	qEvent->accept();
}


void TreeRenderGLWidget::keyPressEvent(QKeyEvent* qEvent) {
	if (qEvent->isAutoRepeat()) qEvent->ignore();
	else {
		if (qEvent->key() == Qt::Key::Key_N && !mMouseLeftPressed) {
			bool isValid;
			glm::vec3 position;
			std::tie(isValid, position) = getTreeRootPosition();
			if (isValid) { emit makeRootAtPosition(position); }
			qEvent->accept();
		}
		else { qEvent->ignore(); }
	}
}

void TreeRenderGLWidget::mousePressEvent(QMouseEvent* qEvent) {
	if (qEvent->button() == Qt::MouseButton::LeftButton) {
		mMouseLeftPressed = true;
		mCursorLastPos = qEvent->localPos();
		qEvent->accept();
	}
	else { qEvent->ignore(); }
}

void TreeRenderGLWidget::mouseReleaseEvent(QMouseEvent* qEvent) {
	if (qEvent->button() == Qt::MouseButton::LeftButton) {
		mMouseLeftPressed = false;
		qEvent->accept();
	}
	else { qEvent->ignore(); }
}

void TreeRenderGLWidget::mouseMoveEvent(QMouseEvent* qEvent) {
	if (mMouseLeftPressed) {
		const auto delta = qEvent->localPos() - mCursorLastPos;
		mCursorLastPos = qEvent->localPos();

		const auto deltaX = float(delta.x());
		const auto deltaY = float(delta.y());

		if (qEvent->modifiers().testFlag(Qt::AltModifier)) { // Move in plane
			if (deltaX < 0.f) { mCamera.handleCameraMovement(Camera::CameraMovement::Left, deltaX); }
			else { mCamera.handleCameraMovement(Camera::CameraMovement::Right, -deltaX); }

			if (deltaY < 0.f) { mCamera.handleCameraMovement(Camera::CameraMovement::Down, -deltaY); }
			else { mCamera.handleCameraMovement(Camera::CameraMovement::Up, deltaY); }
		} // Rotate camera
		else { mCamera.handleMouseMovement(deltaX, deltaY); }

		qEvent->accept();
		update();
	}
	else { qEvent->ignore(); }
}

void TreeRenderGLWidget::wheelEvent(QWheelEvent* qEvent) {
	if (qEvent->modifiers().testFlag(Qt::AltModifier)) {
		if (qEvent->delta() >= 0) {
			mCamera.handleCameraMovement(Camera::CameraMovement::Forward, float(qEvent->delta()));
		}
		else { mCamera.handleCameraMovement(Camera::CameraMovement::Backward, float(-qEvent->delta())); }
	}
	else { mCamera.handleMouseScroll(qEvent->delta() / 120.f); }

	update();
	qEvent->accept();
}

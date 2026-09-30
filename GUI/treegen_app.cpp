/// Main window of TreeGen application.
/// This file uses Qt 5.10.1 - see Licenses in folder External libraries.
/// \file treegen_app.cpp
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#include <stdafx.h>
#include <treegen_app.h>

#include <Treegen management/treegen_management.h>

#include <QtConcurrent/QtConcurrent>
#include <QtXml/QDomDocument>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

TreegenApp::TreegenApp(QWidget* parent) : QMainWindow(parent) {
	ui.setupUi(this);
	ui.seed_lineEdit->setValidator(new QIntValidator(0, (std::numeric_limits<int>::max)(), this));

	connect(&mIterationsWatcher, SIGNAL(finished()), this, SLOT(finishedRunIterationsHandler()));
	connect(&mBuffeDataWatcher, SIGNAL(finished()), this, SLOT(finishedBufferDataHandler()));

	mIntervalTimer.setTimerType(Qt::PreciseTimer);
	mIntervalTimer.setSingleShot(true);
	connect(&mIntervalTimer, SIGNAL(timeout()), this, SLOT(intervalTimeout()));

	const auto currentPath = QDir::currentPath();
	mSceneDirectory = currentPath + "\\Data\\Scenes";
	mDiffuseMapDirectory = currentPath + "\\Data\\Bark textures";
	mSaveMeshDirectory = currentPath + "\\Data\\Tree meshes";
	mLoadSettingsDirectory = currentPath + "\\Data\\Tree parameters";
	mSaveSettingsDirectory = currentPath + "\\Data\\Tree parameters";

	try {
		mTreegen = std::make_shared<TreegenLib>();
		ui.treeRenderWindow->setTreegenAccess(mTreegen);
	}
	catch (...) { mTreegenInitialized = false; }
}

void TreegenApp::keyPressEvent(QKeyEvent* qEvent) {
	if (qEvent->isAutoRepeat()) qEvent->ignore();
	else {
		qEvent->accept();
		switch (qEvent->key()) {
			case Qt::Key::Key_Escape:
				close();
				break;
			case Qt::Key::Key_A:
				on_saveMesh_pushButton_clicked();
				break;
			case Qt::Key::Key_S:
				on_saveParameters_pushButton_clicked();
				break;
			case Qt::Key::Key_J:
				on_clearTrees_pushButton_clicked();
				break;
			case Qt::Key::Key_K:
				on_deleteRoots_pushButton_clicked();
				break;
			case Qt::Key::Key_L:
				on_cancel_pushButton_clicked();
				break;
			case Qt::Key::Key_F:
				on_growTree_pushButton_clicked();
				break;
			case Qt::Key::Key_G:
				on_grow_pushButton_clicked();
				break;
			case Qt::Key::Key_Q:
				ui.treeRenderWindow->showMesh();
				ui.showMesh_radioButton->setChecked(true);
				break;
			case Qt::Key::Key_W:
				ui.treeRenderWindow->showMeshStructure();
				ui.showMeshStructure_radioButton->setChecked(true);
				break;
			case Qt::Key::Key_E:
				ui.treeRenderWindow->showTreeStructure();
				ui.showTreeStructure_radioButton->setChecked(true);
				break;
			case Qt::Key::Key_R:
				ui.treeRenderWindow->resetCamera();
				break;
			case Qt::Key::Key_T:
				ui.showBoundingBox_checkBox->setChecked(!ui.showBoundingBox_checkBox->isChecked());
			break;
			default:
				qEvent->ignore();
		}
	}
}

void TreegenApp::showEvent(QShowEvent* qEvent) {
	qEvent->accept();
	if (qEvent->spontaneous()) { return; };

	if (!mTreegenInitialized) {
		QMessageBox::critical(this, tr("Unsupported hardware"),
			"Tree generator is not supported by your hardware (most likely processor does not support SSE2 instructions) - please check requirements!");

		ui.growTree_pushButton->setEnabled(false);
		ui.grow_pushButton->setEnabled(false);
		ui.deleteRoots_pushButton->setEnabled(false);
	}

	if (!ui.treeRenderWindow->shadersLoaded()) {
		QMessageBox::critical(this, tr("Shaders failed loading"),
			"Your computer does not support OpenGL 3.3.\n"
			"Try to update your graphics drivers - or use different graphics card.");

		ui.diffuseMapBrowse_pushButton->setEnabled(false);
		ui.diffuseMapClear_pushButton->setEnabled(false);
	}

	if (!mTreegenInitialized || !ui.treeRenderWindow->shadersLoaded()) {
		ui.sceneBrowse_pushButton->setEnabled(false);
		ui.sceneClear_pushButton->setEnabled(false);
	}

	on_sceneClear_pushButton_clicked();
}

void TreegenApp::closeEvent(QCloseEvent* qEvent) {
	qEvent->accept();
	on_cancel_pushButton_clicked();
	mIterationsWatcher.waitForFinished();
	mBuffeDataWatcher.waitForFinished();
}

// ===========================================================================================================
// Helper methods
// ===========================================================================================================

void TreegenApp::pushEnvironmentProperties() {
	auto& gen = *mTreegen->generator;

	gen.perceptionOccupancyDistance(
		float(ui.perceptionDistance_dSpinBox->value()),
		float(ui.occupancyRadius_dSpinBox->value()));
	gen.coneAngle(glm::radians(float(ui.coneAngle_dSpinBox->value())));
	gen.budFreeSpaceMarkers(ui.spaceMarkers_spinBox->value());

	gen.setShadowProperties(
		ui.pyramidHeight_spinBox->value(),
		float(ui.nodeShadow_dSpinBox->value()),
		float(ui.shadowDiminish_dSpinBox->value()),
		1.f);
}

void TreegenApp::pushSceneModelRatio() {
	const auto sceneModelRatio = ui.sceneModelRatio_dSpinBox->value();
	if (sceneModelRatio == mTreegen->generator->sceneModelRatio()) { return; }
	mTreegen->generator->sceneModelRatio(sceneModelRatio);
	ui.treeRenderWindow->loadBoundingBox();
}

void TreegenApp::pushTreeSeeds() {
	if (mTreesNoSeed.empty()) { return; }

	auto seedString = ui.seed_lineEdit->text();

	if (seedString.isEmpty()) {
		for (auto&& tree : mTreesNoSeed) { tree.get().randomSeed(mRndTreeSeedGenerator()); }
	}
	else {
		const unsigned seedValue = std::stoi(seedString.toStdString());
		for (auto&& tree : mTreesNoSeed) { tree.get().randomSeed(seedValue); }
	}

	const auto firstSeed = mTreegen->trees[0].get().randomSeed();
	const bool allSameSeed = [&]() {
		for (auto i = 1; i < mTreegen->trees.size(); ++i) {
			if (mTreegen->trees[i].get().randomSeed() != firstSeed) { return false; }
		}
		return true;
	}();

	if (allSameSeed) { ui.treeSeedValue_label->setText(QString::number(firstSeed)); }
	else { ui.treeSeedValue_label->setText("Multiple"); }

	mTreesNoSeed.clear();
}

void TreegenApp::pushTreeProperties() {
	for (auto&& rwTree : mTreegen->trees) {
		Treegen::Tree& tree = rwTree;

		tree.bareTrunkLength(float(ui.bareTrunkLength_dSpinBox->value()));
		tree.maxShootLength(float(ui.maxShootLength_dSpinBox->value()));

		const Treegen::PhyllotacticType phylloType = ui.phyllotaxisType_comboBox->currentText() == "Alternate"
			                                             ? Treegen::PhyllotacticType::Alternate
			                                             : Treegen::PhyllotacticType::Opposite;
		tree.phyllotacticType(phylloType);
		tree.phyllotacticAngle(glm::radians(float(ui.phyllotaxisAngle_dSpinBox->value())));
		tree.lateralAngle(glm::radians(float(ui.lateralAngle_dSpinBox->value())));

		tree.gravimorphismHorizontal(float(ui.gravimorphismHorizontal_dSpinBox->value()));
		tree.gravimorphismVertical(float(ui.gravimorphismVertical_dSpinBox->value()));
		tree.gravimorphismUpDown(float(ui.gravimorphismUpDown_dSpinBox->value()));

		tree.lightSensitivity(float(ui.budLightSensitivity_dSpinBox->value()));
		tree.applyShedding(ui.shedding_checkBox->isChecked());
		tree.sheddingThreshold(float(ui.sheddingThreshold_dSpinBox->value()));

		tree.biasToMain(float(ui.biasToMain_dSpinBox->value()));

		tree.budDirectionWeight(float(ui.dirBud_dSpinBox->value()));
		tree.spaceDirectionWeight(float(ui.dirSpace_dSpinBox->value()));
		tree.tropismDirectionWeight(float(ui.dirTropism_dSpinBox->value()));
		tree.tropismAngle(glm::radians(float(ui.tropismAngle_dSpinBox->value())));
		tree.applyTropismToTrunk(ui.trunkTropism_checkBox->isChecked());
		tree.trunkStraightness(float(ui.trunkStraightness_dSpinBox->value()));
	}
}

void TreegenApp::pushMeshProperties() {
	for (auto&& rwTree : mTreegen->trees) {
		Treegen::Tree& tree = rwTree;
		tree.branchThickness(float(ui.branchThickness_dSpinBox->value()));
		tree.pipeModelN(float(ui.pipeModelN_dSpinBox->value()));
		tree.subinternodes(ui.subinternodesCount_spinBox->value());
		tree.verticesOnCircumference(ui.verticesInNode_spinBox->value());
		tree.textureInternodesCovered(float(ui.textureInternodesCovered_dSpinBox->value()));
	}
}


void TreegenApp::setStartOfGenerationGUI() {
	ui.sceneBrowse_pushButton->setEnabled(false);
	ui.sceneClear_pushButton->setEnabled(false);
	ui.growTree_pushButton->setEnabled(false);
	ui.grow_pushButton->setEnabled(false);
	ui.deleteRoots_pushButton->setEnabled(false);
	ui.clearTrees_pushButton->setEnabled(false);
	ui.saveParameters_pushButton->setEnabled(false);
	ui.saveMesh_pushButton->setEnabled(false);

	ui.cancel_pushButton->setEnabled(true);
}

void TreegenApp::setEndOfGenerationGUI() {
	ui.sceneBrowse_pushButton->setEnabled(true);
	ui.sceneClear_pushButton->setEnabled(true);
	ui.growTree_pushButton->setEnabled(true);
	ui.grow_pushButton->setEnabled(true);
	ui.deleteRoots_pushButton->setEnabled(true);
	ui.clearTrees_pushButton->setEnabled(true);

	if (ui.meshVerticesValue_label->text().toULongLong() != 0) {
		ui.saveParameters_pushButton->setEnabled(true);
		ui.saveMesh_pushButton->setEnabled(true);
	}

	ui.cancel_pushButton->setEnabled(false);
}


bool TreegenApp::tryLoadScene(const QString& location) {
	tinyobj::attrib_t attrib{};
	std::vector<tinyobj::shape_t> shapes{};
	std::vector<tinyobj::material_t> materials{};
	std::string errorMessage{};

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &errorMessage,
		location.toStdString().c_str(), nullptr, true)) {
		QMessageBox::critical(this, tr("Unable to open file"), QString::fromStdString(errorMessage));
		return false;
	}

	size_t vertexCount = 0;
	for (auto&& shape : shapes) {
		if (!shape.mesh.indices.empty() && shape.mesh.indices[0].normal_index == -1) { continue; }
		vertexCount += shape.mesh.indices.size();
	}

	if (vertexCount == 0) {
		QMessageBox::warning(this, tr("Obj file has no valid geometry"),
			"Obj file does not contain any valid object with defined normals!");
		return false;
	}

	std::vector<float> vertexPos{};
	std::vector<float> vertexNormal{};
	vertexPos.reserve(vertexCount * 3); // Position and normal for each vertex of each triangle
	vertexNormal.reserve(vertexCount * 3);

	for (auto&& shape : shapes) {
		if (!shape.mesh.indices.empty() && shape.mesh.indices[0].normal_index == -1) { continue; }
		for (auto&& idx : shape.mesh.indices) {
			if (idx.vertex_index < 0 || idx.vertex_index >= attrib.vertices.size() ||
			    idx.normal_index < 0 || idx.normal_index >= attrib.normals.size()) {
				QMessageBox::warning(this, tr("Obj file is corrupted"),
					"Obj file has invalid face indices => is corrupted!");
				return false;
			}

			vertexPos.push_back(attrib.vertices[3 * idx.vertex_index + 0]);
			vertexPos.push_back(attrib.vertices[3 * idx.vertex_index + 1]);
			vertexPos.push_back(attrib.vertices[3 * idx.vertex_index + 2]);

			vertexNormal.push_back(attrib.normals[3 * idx.normal_index + 0]);
			vertexNormal.push_back(attrib.normals[3 * idx.normal_index + 1]);
			vertexNormal.push_back(attrib.normals[3 * idx.normal_index + 2]);
		}
	}

	std::vector<unsigned> triangles(vertexCount);
	std::iota(std::begin(triangles), std::end(triangles), 0);

	// There was some proper geometry loaded => reset models, load scene for render, set camera and approx. scene model ratio
	deleteAllTrees();
	mTreegen->generator->loadScene(vertexPos, triangles);
	approximateSceneModelRatio();

	ui.treeRenderWindow->loadScene(std::move(vertexPos), std::move(vertexNormal), std::move(triangles));
	ui.treeRenderWindow->onSceneChange();

	mDefaultScene = false;
	return true;
}

void TreegenApp::approximateSceneModelRatio() {
	glm::vec3 sceneMin, sceneMax, modelSizes;
	std::tie(sceneMin, sceneMax) = mTreegen->generator->sceneBounds();
	std::tie(std::ignore, modelSizes) = mTreegen->generator->modelBounds();
	const float modelHalfSceneHeightRatio = (sceneMax.y - sceneMin.y) / (modelSizes.y * 2.f);
	const float sceneModelRatio = glm::clamp(modelHalfSceneHeightRatio,
		float(ui.sceneModelRatio_dSpinBox->minimum()),
		float(ui.sceneModelRatio_dSpinBox->maximum()));
	mTreegen->generator->sceneModelRatio(sceneModelRatio);
	ui.sceneModelRatio_dSpinBox->setValue(sceneModelRatio);
}


void TreegenApp::addNewTree(const glm::vec3& potentialRoot) {
	if (mIsRunning) { return; }

	// If bounding box is defined (there is at least one tree in scene) check if new root is inside of it
	const bool hasBoundingBox = mTreegen->generator->boundingBoxDefined();
	if (hasBoundingBox) {
		ui.sceneModelRatio_dSpinBox->setEnabled(false);
		ui.saveMesh_pushButton->setEnabled(false);

		glm::vec3 lower, upper;
		std::tie(lower, upper) = mTreegen->generator->boundingBoxBounds();

		if (potentialRoot.x < lower.x || potentialRoot.x > upper.x ||
		    potentialRoot.y < lower.y || potentialRoot.y > upper.y ||
		    potentialRoot.z < lower.z || potentialRoot.z > upper.z) {
			return;
		}
	}

	const auto treeId = mTreegen->generator->newTree(potentialRoot);
	auto& treeRef = mTreegen->generator->tree(treeId);
	mTreegen->trees.push_back(treeRef);
	mTreesNoSeed.push_back(treeRef);

	// After fist tree is made in generator => bounding box defined
	if (!hasBoundingBox) {
		ui.treeAgeCurrentValue_label->setText("0");
		ui.treeRenderWindow->loadBoundingBox();
	}
	ui.treeRenderWindow->loadRootsPositions();

	ui.growTree_pushButton->setEnabled(true);
	ui.grow_pushButton->setEnabled(true);
	ui.deleteRoots_pushButton->setEnabled(true);

	ui.treesCountValue_label->setText(QString::number(mTreegen->trees.size()));
	ui.treeRenderWindow->update();
}

void TreegenApp::deleteAllTrees() {
	mTreegen->generator->deleteAllTrees();
	mTreegen->trees.clear();
	mTreesNoSeed.clear();
	mTreesData.clear();
	mIntervalTimer.stop();

	ui.treesCountValue_label->setText("0");
	ui.treeRenderWindow->clearTreesBuffers();

	ui.treeSeedValue_label->setText("-");
	ui.treeAgeCurrentValue_label->setText("-");
	ui.meshVerticesValue_label->setText("-");
	ui.meshTrianglesValue_label->setText("-");

	ui.sceneModelRatio_dSpinBox->setEnabled(true);

	ui.growTree_pushButton->setEnabled(false);
	ui.grow_pushButton->setEnabled(false);
	ui.growInInterval_checkBox->setChecked(false);
	ui.deleteRoots_pushButton->setEnabled(false);
	ui.clearTrees_pushButton->setEnabled(false);
	ui.saveParameters_pushButton->setEnabled(false);
	ui.saveMesh_pushButton->setEnabled(false);
}

// ===========================================================================================================
// Grow tree main cycle methods
// ===========================================================================================================

void TreegenApp::runIterations(const size_t iterations) {
	mTreesData.clear();

	mTreegen->generator->runIterations(iterations);
	loadTreeStructures();
	loadTreeMeshes();
}

void TreegenApp::loadTreeMeshes() {
	assert(mTreesData.vertices.empty());
	assert(mTreesData.triangles.empty());

	for (auto&& rwTree : mTreegen->trees) {
		auto treeMesh = rwTree.get().makeTreeMesh();
		mTreesData.verticesCount += treeMesh.vertices.size();
		mTreesData.trianglesCount += treeMesh.triangles.size();

		mTreesData.vertices.push_back(std::move(treeMesh.vertices));
		mTreesData.triangles.push_back(std::move(treeMesh.triangles));
	}

	unsigned vertexId = 0;
	for (auto&& treeMeshVertices : mTreesData.vertices) {
		for (auto&& uqVertex : treeMeshVertices) {
			uqVertex->Idx = vertexId++;
		}
	}
}

void TreegenApp::loadTreeStructures() {
	assert(mTreesData.nodesPositions.empty());
	assert(mTreesData.internodes.empty());

	for (auto&& rwTree : mTreegen->trees) {
		const auto structure = rwTree.get().getTreeStructure();
		mTreesData.nodesPositionsCount += structure.nodesPositions.size();
		mTreesData.internodesCount += structure.internodes.size();

		mTreesData.nodesPositions.push_back(std::move(structure.nodesPositions));
		mTreesData.internodes.push_back(std::move(structure.internodes));
	}
}

void TreegenApp::bufferData(void* nodes, void* internodes, void* treesVertices, void* treesTriangles) {
	// Structure
	size_t offsetNodes = 0;
	const auto nodesMemory = static_cast<char*>(nodes);

	for (auto&& treeNodes : mTreesData.nodesPositions) {
		const auto sizeInBytes = treeNodes.size() * 3 * sizeof(float);
		std::memcpy(nodesMemory + offsetNodes, treeNodes.data(), sizeInBytes);
		offsetNodes += sizeInBytes;
	}

	size_t offsetInternodes = 0;
	const auto internodesMemory = static_cast<char*>(internodes);

	for (auto&& treeInternodes : mTreesData.internodes) {
		const auto sizeInBytes = treeInternodes.size() * 3 * sizeof(float);
		std::memcpy(internodesMemory + offsetInternodes, treeInternodes.data(), sizeInBytes);
		offsetInternodes += sizeInBytes;
	}

	// Mesh
	std::vector<float> vertexData{};
	std::vector<unsigned> triangles{};

	vertexData.reserve(11 * mTreesData.verticesCount);
	triangles.reserve(3 * mTreesData.trianglesCount);

	for (auto&& treeVertices : mTreesData.vertices) {
		for (auto&& uqVertex : treeVertices) {
			openGL_utils::pushPointToVec(vertexData, uqVertex->position);
			vertexData.push_back(uqVertex->textureCoordinates.s);
			vertexData.push_back(uqVertex->textureCoordinates.t);
			openGL_utils::pushPointToVec(vertexData, uqVertex->normal);
			openGL_utils::pushPointToVec(vertexData, uqVertex->tangent);
		}
	}

	for (auto&& treeTriangles : mTreesData.triangles) {
		for (auto&& triangle : treeTriangles) {
			triangles.push_back(triangle.vertices[0].get().Idx);
			triangles.push_back(triangle.vertices[1].get().Idx);
			triangles.push_back(triangle.vertices[2].get().Idx);
		}
	}

	std::memcpy(treesVertices, vertexData.data(), vertexData.size() * sizeof(float));
	std::memcpy(treesTriangles, triangles.data(), triangles.size() * sizeof(unsigned));
}

// ===========================================================================================================
// Slots - grow tree - async run management thorugh signal and slots
// ===========================================================================================================

void TreegenApp::finishedRunIterationsHandler() {
	if (mIsCanceled) { mTreesData.clear(); }
	ui.cancel_pushButton->setEnabled(false);
	mIsCanceled = false;

	auto mappedBuffers = ui.treeRenderWindow->getMappedBuffers(
		mTreesData.nodesPositionsCount, mTreesData.internodesCount,
		mTreesData.verticesCount, mTreesData.trianglesCount);

	mBufferDataFuture = QtConcurrent::run(this, &TreegenApp::bufferData,
		std::get<0>(mappedBuffers), std::get<1>(mappedBuffers),
		std::get<2>(mappedBuffers), std::get<3>(mappedBuffers));
	mBuffeDataWatcher.setFuture(mBufferDataFuture);
}

void TreegenApp::finishedBufferDataHandler() {
	pushSceneModelRatio();
	ui.treeRenderWindow->swapBuffers();
	const QLocale uk{"uk"};
	ui.meshVerticesValue_label->setText(uk.toString(mTreesData.verticesCount));
	ui.meshTrianglesValue_label->setText(uk.toString(mTreesData.trianglesCount));

	setEndOfGenerationGUI();
	mIsRunning = false;

	if (ui.growInInterval_checkBox->isChecked()) {
		mIntervalTimer.start(int(ui.growInterval_doubleSpinBox->value() * 1000));
	}
}

void TreegenApp::intervalTimeout() {
	if (!mIgnoreTimer && !mIsRunning && ui.growInInterval_checkBox->isChecked()) {
		on_growTree_pushButton_clicked();
	}
}

// ===========================================================================================================
// Slots - main menu widgets
// ===========================================================================================================

void TreegenApp::on_sceneBrowse_pushButton_clicked() {
	if (mIsRunning) { return; }
	ui.growInInterval_checkBox->setChecked(false);
	mIgnoreTimer = true;
	mIntervalTimer.stop();

	auto scenePath = QFileDialog::getOpenFileName(
		this, tr("Scene geometry"), mSceneDirectory,
		tr("Scene (*.obj)"));

	if (!scenePath.isEmpty()) {
		QFileInfo file{scenePath};
		mSceneDirectory = file.absolutePath();

		const auto result = tryLoadScene(scenePath);
		if (!result) { return; }

		auto& label = *ui.sceneFilename_label;
		label.setText(label.fontMetrics().elidedText(file.fileName(), Qt::ElideRight, label.width() - 2));
	}
}

void TreegenApp::on_sceneClear_pushButton_clicked() {
	if (mIsRunning) { return; }
	ui.growInInterval_checkBox->setChecked(false);
	mIgnoreTimer = true;
	mIntervalTimer.stop();

	deleteAllTrees();

	mTreegen->generator->defaultEmptyScene();
	addNewTree({0.f, 0.f, 0.f});

	ui.sceneModelRatio_dSpinBox->setValue(mTreegen->generator->sceneModelRatio());
	ui.treeRenderWindow->loadDefaultEmptyScene();
	ui.treeRenderWindow->onSceneChange();

	ui.sceneFilename_label->setText("");
	mDefaultScene = true;
}

void TreegenApp::on_sceneModelRatio_dSpinBox_valueChanged(const double newValue) {
	Q_UNUSED(newValue);
	if (mIsRunning) { return; }
	pushSceneModelRatio();
}


void TreegenApp::on_treeRenderWindow_makeRootAtPosition(const glm::vec3 scenePosition) {
	addNewTree(scenePosition);
}


void TreegenApp::on_growTree_pushButton_clicked() {
	if (mIsRunning) { return; }
	mIsRunning = true;

	setStartOfGenerationGUI();

	mTreegen->generator->clearCurrentModels();
	pushEnvironmentProperties();
	mTreesNoSeed = mTreegen->trees;
	pushTreeSeeds();
	pushTreeProperties();
	pushMeshProperties();

	mGrowingBy = ui.treeAge_spinBox->value();
	ui.treeAgeCurrentValue_label->setText(QString::number(mGrowingBy));
	ui.meshVerticesValue_label->setText("-");
	ui.meshTrianglesValue_label->setText("-");

	mIterationsFuture = QtConcurrent::run(this, &TreegenApp::runIterations, mGrowingBy);
	mIterationsWatcher.setFuture(mIterationsFuture);
}

void TreegenApp::on_grow_pushButton_clicked() {
	if (mIsRunning) { return; }
	mIsRunning = true;

	setStartOfGenerationGUI();

	if (!mTreegen->generator->isEnvironmentLocked()) { pushEnvironmentProperties(); }
	pushTreeSeeds();
	pushTreeProperties();
	pushMeshProperties();

	mGrowingBy = ui.iterations_spinBox->value();
	ui.treeAgeCurrentValue_label->setText(
		QString::number(ui.treeAgeCurrentValue_label->text().toInt() + mGrowingBy));
	ui.meshVerticesValue_label->setText("-");
	ui.meshTrianglesValue_label->setText("-");

	mIterationsFuture = QtConcurrent::run(this, &TreegenApp::runIterations, mGrowingBy);
	mIterationsWatcher.setFuture(mIterationsFuture);
}

void TreegenApp::on_growInInterval_checkBox_toggled() {
	if (!ui.growInInterval_checkBox->isChecked()) {
		mIgnoreTimer = true;
		mIntervalTimer.stop();
		return;
	}

	mIgnoreTimer = false;
	if (mIsRunning) { return; }
	on_growTree_pushButton_clicked();
}

void TreegenApp::on_deleteRoots_pushButton_clicked() {
	if (mIsRunning) { return; }
	deleteAllTrees();
	if (mDefaultScene) { addNewTree({0.f, 0.f, 0.f}); }
}

void TreegenApp::on_clearTrees_pushButton_clicked() {
	if (mIsRunning) { return; }

	mTreegen->generator->clearCurrentModels();
	ui.treeRenderWindow->clearTreesBuffers();
	ui.treeRenderWindow->loadRootsPositions();
	ui.treeSeedValue_label->setText("-");
	ui.treeAgeCurrentValue_label->setText("0");
	ui.meshVerticesValue_label->setText("-");
	ui.meshTrianglesValue_label->setText("-");

	ui.clearTrees_pushButton->setEnabled(false);
	ui.saveParameters_pushButton->setEnabled(false);
	ui.saveMesh_pushButton->setEnabled(false);

	mTreesNoSeed = mTreegen->trees;
}

void TreegenApp::on_cancel_pushButton_clicked() {
	if (!mIsRunning) { return; }

	mIsCanceled = true;
	mIntervalTimer.stop();
	ui.growInInterval_checkBox->setChecked(false);

	mTreegen->generator->cancelIterations();
	for (auto&& rwTree : mTreegen->trees) {
		rwTree.get().cancelMakeMesh();
	}
}

void TreegenApp::on_baseParameters_pushButton_clicked() {
	// Load empty params => failure checks will assign default values
	QTextStream text{""};
	loadParameters(text);
}


void TreegenApp::on_loadParameters_pushButton_clicked() {
	auto fileName = QFileDialog::getOpenFileName(
		this, tr("Tree parameters"), mLoadSettingsDirectory, tr("Parameters (*.xml)"));

	if (!fileName.isEmpty()) {
		QFile file{fileName};
		QFileInfo fileInfo{file};
		mLoadSettingsDirectory = fileInfo.absolutePath();

		if (!file.open(QIODevice::ReadOnly)) {
			QMessageBox::critical(this, tr("Unable to open file"), file.errorString());
			return;
		}

		QTextStream inputStream(&file);
		loadParameters(inputStream);
	}
}

void TreegenApp::on_saveParameters_pushButton_clicked() {
	if (mIsRunning) { return; }

	const int timerRemaining = mIntervalTimer.remainingTime();
	mIgnoreTimer = true;
	mIntervalTimer.stop();

	// Opens async event loop and timer can fire during it => starting generation of next tree 
	auto fileName = QFileDialog::getSaveFileName(
		this, tr("Tree parameters"), mSaveSettingsDirectory, tr("Parameters (*.xml)"));

	if (!fileName.isEmpty()) {
		QFile file{fileName};
		QFileInfo fileInfo{file};
		mSaveMeshDirectory = fileInfo.absolutePath();

		if (!file.open(QIODevice::WriteOnly)) {
			QMessageBox::critical(this, tr("Unable to open file"), file.errorString());
		}
		else {
			QTextStream outputStream(&file);
			saveParameters(outputStream);
		}
	}

	if (timerRemaining != -1) {
		mIgnoreTimer = false;
		mIntervalTimer.start(timerRemaining);
	}
}

void TreegenApp::on_saveMesh_pushButton_clicked() {
	if (mIsRunning) { return; }

	const int timerRemaining = mIntervalTimer.remainingTime();
	mIgnoreTimer = true;
	mIntervalTimer.stop();

	// Opens async event loop and timer can fire during it => starting generation of next tree 
	auto fileName = QFileDialog::getSaveFileName(
		this, tr("Mesh save file"), mSaveMeshDirectory, tr("Mesh (*.obj)"));

	if (!fileName.isEmpty()) {
		QFile file{fileName};
		QFileInfo fileInfo{file};
		mSaveMeshDirectory = fileInfo.absolutePath();

		if (!file.open(QIODevice::WriteOnly)) {
			QMessageBox::critical(this, tr("Unable to open file"), file.errorString());
		}
		else {
			QTextStream outputStream(&file);
			saveMesh(outputStream);
		}
	}

	if (timerRemaining != -1) {
		mIgnoreTimer = false;
		mIntervalTimer.start(timerRemaining);
	}
}


void TreegenApp::on_showMesh_radioButton_clicked() {
	ui.treeRenderWindow->showMesh();
}

void TreegenApp::on_showMeshStructure_radioButton_clicked() {
	ui.treeRenderWindow->showMeshStructure();
}

void TreegenApp::on_showTreeStructure_radioButton_clicked() {
	ui.treeRenderWindow->showTreeStructure();
}

void TreegenApp::on_showBoundingBox_checkBox_toggled(const bool checked) {
	ui.treeRenderWindow->showBoundingBox(checked);
}

void TreegenApp::on_resetCamera_pushButton_clicked() {
	ui.treeRenderWindow->resetCamera();
}

// ===========================================================================================================
// Slots - Tree settings widgets
// ===========================================================================================================

void TreegenApp::on_perceptionDistance_dSpinBox_valueChanged(const double newValue) {
	if (ui.occupancyRadius_dSpinBox->value() > newValue) {
		ui.occupancyRadius_dSpinBox->setValue(newValue - 0.01);
	}
}

void TreegenApp::on_occupancyRadius_dSpinBox_valueChanged(const double newValue) {
	if (ui.perceptionDistance_dSpinBox->value() < newValue) {
		ui.perceptionDistance_dSpinBox->setValue(newValue + 0.01);
	}
}


void TreegenApp::on_verticesInNode_spinBox_editingFinished() {
	auto final_value = ui.verticesInNode_spinBox->value();
	if (final_value < 4) { final_value = 4; }
	else if (final_value % 2 != 0) { final_value = final_value - 1; }
	ui.verticesInNode_spinBox->setValue(final_value);
}

void TreegenApp::on_diffuseMapBrowse_pushButton_clicked() {
	auto fileName = QFileDialog::getOpenFileName(
		this, tr("Diffuse texture"), mDiffuseMapDirectory,
		tr("Texture (*.png *.bmp *.jpg *.jpeg)"));

	if (!fileName.isEmpty()) {
		QFileInfo fileInfo{fileName};
		mDiffuseMapDirectory = fileInfo.absolutePath();

		const QImage diffuseTexture{fileName};
		if (diffuseTexture.isNull()) {
			QMessageBox::critical(this, tr("Unable to process texture file"),
				"Error occured during loading of texture from: " + fileName);
			return;
		}
		ui.treeRenderWindow->loadDiffuseTexture(diffuseTexture);

		auto& label = *ui.diffuseMapFilename_label;
		label.setText(label.fontMetrics().elidedText(fileInfo.fileName(), Qt::ElideRight, label.width() - 2));
	}
}

void TreegenApp::on_diffuseMapClear_pushButton_clicked() {
	ui.diffuseMapFilename_label->setText("");
	ui.treeRenderWindow->clearDiffuseTexture();
}

// ===========================================================================================================
// Save/load tree methods
// ===========================================================================================================

void TreegenApp::saveParameters(QTextStream& stream) {
	QDomDocument doc("treeParams");
	QDomElement root = doc.createElement("treeParams");
	doc.appendChild(root);

	QDomElement general = doc.createElement("general");
	root.appendChild(general);

	QDomElement randomSeed = doc.createElement("randomSeed");
	general.appendChild(randomSeed);
	const QDomText randomSeedTxt =
		doc.createTextNode(ui.treeSeedValue_label->text());
	randomSeed.appendChild(randomSeedTxt);

	QDomElement treeAge = doc.createElement("treeAge");
	general.appendChild(treeAge);
	const QDomText treeAgeTxt =
		doc.createTextNode(ui.treeAgeCurrentValue_label->text());
	treeAge.appendChild(treeAgeTxt);

	QDomElement base = doc.createElement("base");
	root.appendChild(base);

	QDomElement bareTrunkLength = doc.createElement("bareTrunkLength");
	base.appendChild(bareTrunkLength);
	const QDomText bareTrunkLengthTxt =
		doc.createTextNode(QString::number(ui.bareTrunkLength_dSpinBox->value()));
	bareTrunkLength.appendChild(bareTrunkLengthTxt);

	QDomElement maxShootLength = doc.createElement("maxShootLength");
	base.appendChild(maxShootLength);
	const QDomText maxShootLengthTxt =
		doc.createTextNode(QString::number(ui.maxShootLength_dSpinBox->value()));
	maxShootLength.appendChild(maxShootLengthTxt);

	QDomElement lateralAngle = doc.createElement("lateralAngle");
	base.appendChild(lateralAngle);
	const QDomText lateralAngleTxt =
		doc.createTextNode(QString::number(ui.lateralAngle_dSpinBox->value()));
	lateralAngle.appendChild(lateralAngleTxt);

	QDomElement horizontalPreference = doc.createElement("horizontalPreference");
	base.appendChild(horizontalPreference);
	const QDomText horizontalPreferenceTxt =
		doc.createTextNode(QString::number(ui.gravimorphismHorizontal_dSpinBox->value()));
	horizontalPreference.appendChild(horizontalPreferenceTxt);

	QDomElement verticalPreference = doc.createElement("verticalPreference");
	base.appendChild(verticalPreference);
	const QDomText verticalPreferenceTxt =
		doc.createTextNode(QString::number(ui.gravimorphismVertical_dSpinBox->value()));
	verticalPreference.appendChild(verticalPreferenceTxt);

	QDomElement upDownPreference = doc.createElement("upDownPreference");
	base.appendChild(upDownPreference);
	const QDomText upDownPreferenceTxt =
		doc.createTextNode(QString::number(ui.gravimorphismUpDown_dSpinBox->value()));
	upDownPreference.appendChild(upDownPreferenceTxt);

	QDomElement budSensitivity = doc.createElement("budSensitivity");
	base.appendChild(budSensitivity);
	const QDomText budSensitivityTxt =
		doc.createTextNode(QString::number(ui.budLightSensitivity_dSpinBox->value()));
	budSensitivity.appendChild(budSensitivityTxt);

	QDomElement biasToMain = doc.createElement("biasToMain");
	base.appendChild(biasToMain);
	const QDomText biasToMainTxt =
		doc.createTextNode(QString::number(ui.biasToMain_dSpinBox->value()));
	biasToMain.appendChild(biasToMainTxt);

	QDomElement budDirWeight = doc.createElement("budDirWeight");
	base.appendChild(budDirWeight);
	const QDomText budDirWeightTxt =
		doc.createTextNode(QString::number(ui.dirBud_dSpinBox->value()));
	budDirWeight.appendChild(budDirWeightTxt);

	QDomElement spaceDirWeight = doc.createElement("spaceDirWeight");
	base.appendChild(spaceDirWeight);
	const QDomText spaceDirWeightTxt =
		doc.createTextNode(QString::number(ui.dirSpace_dSpinBox->value()));
	spaceDirWeight.appendChild(spaceDirWeightTxt);

	QDomElement tropismDirWeight = doc.createElement("tropismDirWeight");
	base.appendChild(tropismDirWeight);
	const QDomText tropismDirWeightTxt =
		doc.createTextNode(QString::number(ui.dirTropism_dSpinBox->value()));
	tropismDirWeight.appendChild(tropismDirWeightTxt);

	QDomElement tropismAngle = doc.createElement("tropismAngle");
	base.appendChild(tropismAngle);
	const QDomText tropismAngleTxt =
		doc.createTextNode(QString::number(ui.tropismAngle_dSpinBox->value()));
	tropismAngle.appendChild(tropismAngleTxt);

	QDomElement tropismToTrunk = doc.createElement("tropismToTrunk");
	base.appendChild(tropismToTrunk);
	const QDomText tropismToTrunkTxt =
		doc.createTextNode(QString::number(ui.trunkTropism_checkBox->isChecked()));
	tropismToTrunk.appendChild(tropismToTrunkTxt);

	QDomElement trunkStraightness = doc.createElement("trunkStraightness");
	base.appendChild(trunkStraightness);
	const QDomText trunkStraightnessTxt =
		doc.createTextNode(QString::number(ui.trunkStraightness_dSpinBox->value()));
	trunkStraightness.appendChild(trunkStraightnessTxt);

	QDomElement applyShedding = doc.createElement("applyShedding");
	base.appendChild(applyShedding);
	const QDomText applySheddingTxt =
		doc.createTextNode(QString::number(ui.shedding_checkBox->isChecked()));
	applyShedding.appendChild(applySheddingTxt);

	QDomElement sheddingThreshold = doc.createElement("sheddingThreshold");
	base.appendChild(sheddingThreshold);
	const QDomText sheddingThresholdTxt =
		doc.createTextNode(QString::number(ui.sheddingThreshold_dSpinBox->value()));
	sheddingThreshold.appendChild(sheddingThresholdTxt);

	QDomElement advanced = doc.createElement("advanced");
	root.appendChild(advanced);

	QDomElement phyllotaxisType = doc.createElement("phyllotaxisType");
	advanced.appendChild(phyllotaxisType);
	const QDomText phyllotaxisTypeTxt =
		doc.createTextNode(ui.phyllotaxisType_comboBox->currentText());
	phyllotaxisType.appendChild(phyllotaxisTypeTxt);

	QDomElement phyllotacticAngle = doc.createElement("phyllotacticAngle");
	advanced.appendChild(phyllotacticAngle);
	const QDomText phyllotacticAngleTxt =
		doc.createTextNode(QString::number(ui.phyllotaxisAngle_dSpinBox->value()));
	phyllotacticAngle.appendChild(phyllotacticAngleTxt);

	QDomElement perceptionDistance = doc.createElement("perceptionDistance");
	advanced.appendChild(perceptionDistance);
	const QDomText perceptionDistanceTxt =
		doc.createTextNode(QString::number(ui.perceptionDistance_dSpinBox->value()));
	perceptionDistance.appendChild(perceptionDistanceTxt);

	QDomElement occupancyRadius = doc.createElement("occupancyRadius");
	advanced.appendChild(occupancyRadius);
	const QDomText occupancyRadiusTxt =
		doc.createTextNode(QString::number(ui.occupancyRadius_dSpinBox->value()));
	occupancyRadius.appendChild(occupancyRadiusTxt);

	QDomElement coneAngle = doc.createElement("coneAngle");
	advanced.appendChild(coneAngle);
	const QDomText coneAngleTxt =
		doc.createTextNode(QString::number(ui.coneAngle_dSpinBox->value()));
	coneAngle.appendChild(coneAngleTxt);

	QDomElement budSpaceMarkers = doc.createElement("budSpaceMarkers");
	advanced.appendChild(budSpaceMarkers);
	const QDomText budSpaceMarkersTxt =
		doc.createTextNode(QString::number(ui.spaceMarkers_spinBox->value()));
	budSpaceMarkers.appendChild(budSpaceMarkersTxt);

	QDomElement pyramidHeight = doc.createElement("pyramidHeight");
	advanced.appendChild(pyramidHeight);
	const QDomText pyramidHeightTxt =
		doc.createTextNode(QString::number(ui.pyramidHeight_spinBox->value()));
	pyramidHeight.appendChild(pyramidHeight);

	QDomElement nodeShadow = doc.createElement("nodeShadow");
	advanced.appendChild(nodeShadow);
	const QDomText nodeShadowTxt =
		doc.createTextNode(QString::number(ui.nodeShadow_dSpinBox->value()));
	nodeShadow.appendChild(nodeShadowTxt);

	QDomElement shadowDiminish = doc.createElement("shadowDiminish");
	advanced.appendChild(shadowDiminish);
	const QDomText shadowDiminishTxt =
		doc.createTextNode(QString::number(ui.shadowDiminish_dSpinBox->value()));
	shadowDiminish.appendChild(shadowDiminishTxt);

	QDomElement mesh = doc.createElement("mesh");
	root.appendChild(mesh);

	QDomElement branchThickness = doc.createElement("branchThickness");
	mesh.appendChild(branchThickness);
	const QDomText branchThicknessTxt =
		doc.createTextNode(QString::number(ui.branchThickness_dSpinBox->value()));
	branchThickness.appendChild(branchThicknessTxt);

	QDomElement branchingThinness = doc.createElement("branchingThinness");
	mesh.appendChild(branchingThinness);
	const QDomText branchingThinnessTxt =
		doc.createTextNode(QString::number(ui.pipeModelN_dSpinBox->value()));
	branchingThinness.appendChild(branchingThinnessTxt);

	QDomElement cyllinderPerInternode = doc.createElement("cyllinderPerInternode");
	mesh.appendChild(cyllinderPerInternode);
	const QDomText cyllinderPerInternodeTxt =
		doc.createTextNode(QString::number(ui.subinternodesCount_spinBox->value()));
	cyllinderPerInternode.appendChild(cyllinderPerInternodeTxt);

	QDomElement circumferenceVertices = doc.createElement("circumferenceVertices");
	mesh.appendChild(circumferenceVertices);
	const QDomText circumferenceVerticesTxt =
		doc.createTextNode(QString::number(ui.verticesInNode_spinBox->value()));
	circumferenceVertices.appendChild(circumferenceVerticesTxt);

	stream << doc.toString();
}

void TreegenApp::loadParameters(QTextStream& stream) {
	QDomDocument doc{"treeParams"};
	doc.setContent(stream.readAll());

	const auto randomSeedNL = doc.elementsByTagName("randomSeed");
	const auto treeAgeNL = doc.elementsByTagName("treeAge");
	const auto bareTrunkLengthNL = doc.elementsByTagName("bareTrunkLength");
	const auto maxShootLengthNL = doc.elementsByTagName("maxShootLength");
	const auto lateralAngleNL = doc.elementsByTagName("lateralAngle");
	const auto horizontalPreferenceNL = doc.elementsByTagName("horizontalPreference");
	const auto verticalPreferenceNL = doc.elementsByTagName("verticalPreference");
	const auto upDownPreferenceNL = doc.elementsByTagName("upDownPreference");
	const auto budSensitivityNL = doc.elementsByTagName("budSensitivity");
	const auto biasToMainNL = doc.elementsByTagName("biasToMain");
	const auto budDirWeightNL = doc.elementsByTagName("budDirWeight");
	const auto spaceDirWeightNL = doc.elementsByTagName("spaceDirWeight");
	const auto tropismDirWeightNL = doc.elementsByTagName("tropismDirWeight");
	const auto tropismAngleNL = doc.elementsByTagName("tropismAngle");
	const auto tropismToTrunkNL = doc.elementsByTagName("tropismToTrunk");
	const auto trunkStraightnessNL = doc.elementsByTagName("trunkStraightness");
	const auto applySheddingNL = doc.elementsByTagName("applyShedding");
	const auto sheddingThresholdNL = doc.elementsByTagName("sheddingThreshold");
	const auto phyllotaxisTypeNL = doc.elementsByTagName("phyllotaxisType");
	const auto phyllotacticAngleNL = doc.elementsByTagName("phyllotacticAngle");
	const auto perceptionDistanceNL = doc.elementsByTagName("perceptionDistance");
	const auto occupancyRadiusNL = doc.elementsByTagName("occupancyRadius");
	const auto coneAngleNL = doc.elementsByTagName("coneAngle");
	const auto budSpaceMarkersNL = doc.elementsByTagName("budSpaceMarkers");
	const auto pyramidHeightNL = doc.elementsByTagName("pyramidHeight");
	const auto nodeShadowNL = doc.elementsByTagName("nodeShadow");
	const auto shadowDiminishNL = doc.elementsByTagName("shadowDiminish");
	const auto branchThicknessNL = doc.elementsByTagName("branchThickness");
	const auto branchingThinnessNL = doc.elementsByTagName("branchingThinness");
	const auto cyllinderPerInternodeNL = doc.elementsByTagName("cyllinderPerInternode");
	const auto circumferenceVerticesNL = doc.elementsByTagName("circumferenceVertices");

	const auto randomSeedQStr = randomSeedNL.at(0).toElement().text();
	const auto treeAgeQStr = treeAgeNL.at(0).toElement().text();
	const auto bareTrunkLengthQStr = bareTrunkLengthNL.at(0).toElement().text();
	const auto maxShootLengthQStr = maxShootLengthNL.at(0).toElement().text();
	const auto lateralAngleQStr = lateralAngleNL.at(0).toElement().text();
	const auto horizontalPreferenceQStr = horizontalPreferenceNL.at(0).toElement().text();
	const auto verticalPreferenceQStr = verticalPreferenceNL.at(0).toElement().text();
	const auto upDownPreferenceQStr = upDownPreferenceNL.at(0).toElement().text();
	const auto budSensitivityQStr = budSensitivityNL.at(0).toElement().text();
	const auto biasToMainQStr = biasToMainNL.at(0).toElement().text();
	const auto budDirWeightQStr = budDirWeightNL.at(0).toElement().text();
	const auto spaceDirWeightQStr = spaceDirWeightNL.at(0).toElement().text();
	const auto tropismDirWeightQStr = tropismDirWeightNL.at(0).toElement().text();
	const auto tropismAngleQStr = tropismAngleNL.at(0).toElement().text();
	const auto tropismToTrunkQStr = tropismToTrunkNL.at(0).toElement().text();
	const auto trunkStraightnessQStr = trunkStraightnessNL.at(0).toElement().text();
	const auto applySheddingQStr = applySheddingNL.at(0).toElement().text();
	const auto sheddingThresholdQStr = sheddingThresholdNL.at(0).toElement().text();
	const auto phyllotaxisTypeQStr = phyllotaxisTypeNL.at(0).toElement().text();
	const auto phyllotacticAngleQStr = phyllotacticAngleNL.at(0).toElement().text();
	const auto perceptionDistanceQStr = perceptionDistanceNL.at(0).toElement().text();
	const auto occupancyRadiusQStr = occupancyRadiusNL.at(0).toElement().text();
	const auto coneAngleQStr = coneAngleNL.at(0).toElement().text();
	const auto budSpaceMarkersQStr = budSpaceMarkersNL.at(0).toElement().text();
	const auto pyramidDepthQStr = pyramidHeightNL.at(0).toElement().text();
	const auto nodeShadowQStr = nodeShadowNL.at(0).toElement().text();
	const auto shadowDiminishQStr = shadowDiminishNL.at(0).toElement().text();
	const auto branchThicknessQStr = branchThicknessNL.at(0).toElement().text();
	const auto branchingThinnessQStr = branchingThinnessNL.at(0).toElement().text();
	const auto cyllinderPerInternodeQStr = cyllinderPerInternodeNL.at(0).toElement().text();
	const auto circumferenceVerticesQStr = circumferenceVerticesNL.at(0).toElement().text();

	bool convertResult;

	const auto seed = randomSeedQStr.toUInt(&convertResult);
	if (convertResult == false) { ui.seed_lineEdit->setText("1"); }
	else { ui.seed_lineEdit->setText(QString::number(seed)); }

	const auto treeAge = treeAgeQStr.toInt(&convertResult);
	if (convertResult == false) { ui.treeAge_spinBox->setValue(35); }
	else { ui.treeAge_spinBox->setValue(treeAge); }

	const auto bareTrunkLength = bareTrunkLengthQStr.toDouble(&convertResult);
	if (convertResult == false) { ui.bareTrunkLength_dSpinBox->setValue(10.0); }
	else { ui.bareTrunkLength_dSpinBox->setValue(bareTrunkLength); }

	const auto maxShootLength = maxShootLengthQStr.toDouble(&convertResult);
	if (convertResult == false) { ui.maxShootLength_dSpinBox->setValue(3.0); }
	else { ui.maxShootLength_dSpinBox->setValue(maxShootLength); }

	const auto lateralAngle = lateralAngleQStr.toDouble(&convertResult);
	if (convertResult == false) { ui.lateralAngle_dSpinBox->setValue(60.0); }
	else { ui.lateralAngle_dSpinBox->setValue(lateralAngle); }

	const auto horizontalPreference = horizontalPreferenceQStr.toDouble(&convertResult);
	if (convertResult == false) { ui.gravimorphismHorizontal_dSpinBox->setValue(1.0); }
	else { ui.gravimorphismHorizontal_dSpinBox->setValue(horizontalPreference); }

	const auto verticalPreference = verticalPreferenceQStr.toDouble(&convertResult);
	if (convertResult == false) { ui.gravimorphismVertical_dSpinBox->setValue(1.0); }
	else { ui.gravimorphismVertical_dSpinBox->setValue(verticalPreference); }

	const auto upDownPreference = upDownPreferenceQStr.toDouble(&convertResult);
	if (convertResult == false) { ui.gravimorphismUpDown_dSpinBox->setValue(0.0); }
	else { ui.gravimorphismUpDown_dSpinBox->setValue(upDownPreference); }

	const auto budSensitivity = budSensitivityQStr.toDouble(&convertResult);
	if (convertResult == false) { ui.budLightSensitivity_dSpinBox->setValue(1.0); }
	else { ui.budLightSensitivity_dSpinBox->setValue(budSensitivity); }

	const auto biasToMain = biasToMainQStr.toDouble(&convertResult);
	if (convertResult == false) { ui.biasToMain_dSpinBox->setValue(0.52); }
	else { ui.biasToMain_dSpinBox->setValue(biasToMain); }

	const auto budDirWeight = budDirWeightQStr.toDouble(&convertResult);
	if (convertResult == false) { ui.dirBud_dSpinBox->setValue(0.5); }
	else { ui.dirBud_dSpinBox->setValue(budDirWeight); }

	const auto spaceDirWeight = spaceDirWeightQStr.toDouble(&convertResult);
	if (convertResult == false) { ui.dirSpace_dSpinBox->setValue(1.0); }
	else { ui.dirSpace_dSpinBox->setValue(spaceDirWeight); }

	const auto tropismDirWeight = tropismDirWeightQStr.toDouble(&convertResult);
	if (convertResult == false) { ui.dirTropism_dSpinBox->setValue(0.0); }
	else { ui.dirTropism_dSpinBox->setValue(tropismDirWeight); }

	const auto tropismAngle = tropismAngleQStr.toDouble(&convertResult);
	if (convertResult == false) { ui.tropismAngle_dSpinBox->setValue(0.0); }
	else { ui.tropismAngle_dSpinBox->setValue(tropismAngle); }

	const auto tropismToTrunk = tropismToTrunkQStr.toUInt(&convertResult);
	if (convertResult == false) { ui.trunkTropism_checkBox->setChecked(false); }
	else { ui.trunkTropism_checkBox->setChecked(bool(tropismToTrunk)); }

	const auto trunkStraightness = trunkStraightnessQStr.toDouble(&convertResult);
	if (convertResult == false) { ui.trunkStraightness_dSpinBox->setValue(0.5); }
	else { ui.trunkStraightness_dSpinBox->setValue(trunkStraightness); }

	const auto applyShedding = applySheddingQStr.toUInt(&convertResult);
	if (convertResult == false) { ui.shedding_checkBox->setChecked(false); }
	else { ui.shedding_checkBox->setChecked(bool(applyShedding)); }

	const auto sheddingThreshold = sheddingThresholdQStr.toDouble(&convertResult);
	if (convertResult == false) { ui.sheddingThreshold_dSpinBox->setValue(0.0); }
	else { ui.sheddingThreshold_dSpinBox->setValue(sheddingThreshold); }

	ui.phyllotaxisType_comboBox->setCurrentText(phyllotaxisTypeQStr);

	const auto phyllotacticAngle = phyllotacticAngleQStr.toDouble(&convertResult);
	if (convertResult == false) { ui.phyllotaxisAngle_dSpinBox->setValue(137.5); }
	else { ui.phyllotaxisAngle_dSpinBox->setValue(phyllotacticAngle); }

	const auto perceptionDistance = perceptionDistanceQStr.toDouble(&convertResult);
	if (convertResult == false) { ui.perceptionDistance_dSpinBox->setValue(5.0); }
	else { ui.perceptionDistance_dSpinBox->setValue(perceptionDistance); }

	const auto occupancyRadius = occupancyRadiusQStr.toDouble(&convertResult);
	if (convertResult == false) { ui.occupancyRadius_dSpinBox->setValue(2.0); }
	else { ui.occupancyRadius_dSpinBox->setValue(occupancyRadius); }

	const auto coneAngle = coneAngleQStr.toDouble(&convertResult);
	if (convertResult == false) { ui.coneAngle_dSpinBox->setValue(90.0); }
	else { ui.coneAngle_dSpinBox->setValue(coneAngle); }

	const auto budSpaceMarkers = budSpaceMarkersQStr.toUInt(&convertResult);
	if (convertResult == false) { ui.spaceMarkers_spinBox->setValue(3); }
	else { ui.spaceMarkers_spinBox->setValue(budSpaceMarkers); }

	const auto pyramidDepth = pyramidDepthQStr.toUInt(&convertResult);
	if (convertResult == false) { ui.pyramidHeight_spinBox->setValue(10); }
	else { ui.pyramidHeight_spinBox->setValue(pyramidDepth); }

	const auto nodeShadow = nodeShadowQStr.toDouble(&convertResult);
	if (convertResult == false) { ui.nodeShadow_dSpinBox->setValue(0.3); }
	else { ui.nodeShadow_dSpinBox->setValue(nodeShadow); }

	const auto shadowDiminish = shadowDiminishQStr.toDouble(&convertResult);
	if (convertResult == false) { ui.shadowDiminish_dSpinBox->setValue(0.5); }
	else { ui.shadowDiminish_dSpinBox->setValue(shadowDiminish); }

	const auto branchThickness = branchThicknessQStr.toDouble(&convertResult);
	if (convertResult == false) { ui.branchThickness_dSpinBox->setValue(0.015); }
	else { ui.branchThickness_dSpinBox->setValue(branchThickness); }

	const auto branchingThinness = branchingThinnessQStr.toDouble(&convertResult);
	if (convertResult == false) { ui.pipeModelN_dSpinBox->setValue(2.0); }
	else { ui.pipeModelN_dSpinBox->setValue(branchingThinness); }

	const auto cyllinderPerInternode = cyllinderPerInternodeQStr.toUInt(&convertResult);
	if (convertResult == false) { ui.subinternodesCount_spinBox->setValue(1); }
	else { ui.subinternodesCount_spinBox->setValue(cyllinderPerInternode); }

	const auto circumferenceVertices = circumferenceVerticesQStr.toUInt(&convertResult);
	if (convertResult == false) { ui.verticesInNode_spinBox->setValue(6); }
	else { ui.verticesInNode_spinBox->setValue(circumferenceVertices); }
}

void TreegenApp::saveMesh(QTextStream& stream) {
	stream << "# Output tree meshes from TreeGen application.\n";
	stream << "# Each group for individual tree.\n";
	stream << "# Contains texture coordinates and explicit normals.\n\n";

	int treeId = 0;
	for (auto i = 0; i < mTreesData.vertices.size(); ++i) {
		if (mTreesData.vertices[i].empty()) { continue; }

		++treeId;
		stream << "o Tree_" << treeId << "\n\n";

		for (auto&& uqVertex : mTreesData.vertices[i]) {
			auto& v = uqVertex->position;
			stream << "v " << v.x << " " << v.y << " " << v.z << "\n";
		}

		stream << "\n";

		for (auto&& uqVertex : mTreesData.vertices[i]) {
			auto& vt = uqVertex->textureCoordinates;
			stream << "vt " << vt.x << " " << vt.y << "\n";
		}

		stream << "\n";

		for (auto&& uqVertex : mTreesData.vertices[i]) {
			auto& vn = uqVertex->normal;
			stream << "vn " << vn.x << " " << vn.y << " " << vn.z << "\n";
		}

		stream << "\n";

		for (auto&& triangle : mTreesData.triangles[i]) {
			const auto idxA = triangle.vertices[0].get().Idx + 1;
			const auto idxB = triangle.vertices[1].get().Idx + 1;
			const auto idxC = triangle.vertices[2].get().Idx + 1;

			stream << "f "
				<< idxA << "/" << idxA << "/" << idxA << " "
				<< idxB << "/" << idxB << "/" << idxB << " "
				<< idxC << "/" << idxC << "/" << idxC << "\n";
		}

		stream << "\n";
	}
}

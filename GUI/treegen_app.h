/// Main window of TreeGen application.
/// This file uses Qt 5.10.1 - see Licenses in folder External libraries.
/// \file treegen_app.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#ifndef TREEGEN_APP_H
#define TREEGEN_APP_H

#include <ui_treegen_gui.h>
#include <QtWidgets/QMainWindow>
#include <QtCore/QFuture>
#include <QtCore/QFutureWatcher>

#include <Treegen management/treegen_management.h>

#include <memory>
#include <random>
#include <chrono>

/// Main window of TreeGen application.
class TreegenApp : public QMainWindow {
Q_OBJECT

public:
	explicit TreegenApp(QWidget* parent = Q_NULLPTR);

private:
	Ui::TreegenGUI ui{};

	/// \name Tree generator management.
	///@{
	std::shared_ptr<TreegenLib> mTreegen = nullptr;
	std::vector<std::reference_wrapper<Treegen::Tree>> mTreesNoSeed{};
	bool mTreegenInitialized = true;

	std::mt19937 mRndTreeSeedGenerator{std::chrono::system_clock::now().time_since_epoch().count()};
	TreesData mTreesData{};
	///@}

	/// \name Browsed directories.
	///@{
	QString mSceneDirectory{};
	QString mDiffuseMapDirectory{};
	QString mSaveMeshDirectory{};
	QString mLoadSettingsDirectory{};
	QString mSaveSettingsDirectory{};
	///@}

	/// \name Async and automatic generator run management.
	///@{
	QFuture<void> mIterationsFuture{};
	QFutureWatcher<void> mIterationsWatcher{};
	QFuture<void> mBufferDataFuture{};
	QFutureWatcher<void> mBuffeDataWatcher{};
	
	QTimer mIntervalTimer{};
	///@}

	/// \name State variables.
	///@{
	int mGrowingBy = 0;
	bool mIgnoreTimer = true;
	bool mDefaultScene = true;
	bool mIsRunning = false;	// Prevents pushing of model size during run of generator.
	bool mIsCanceled = false;
	///@}

	/// Handles most keybinds.
	void keyPressEvent(QKeyEvent* qEvent) override;

	/// Checks if application initialization was successfull.
	void showEvent(QShowEvent* qEvent) override;

	/// Cancels running algorithm and joins running threads.
	void closeEvent(QCloseEvent *qEvent) override;

	/// \name Helper methods
	///@{
	/// Loads environment properties into tree generator.
	void pushEnvironmentProperties();
	
	/// Loads scene model ratio into tree generator.
	void pushSceneModelRatio();
	
	/// Loads trees random seeds.
	void pushTreeSeeds();

	/// Loads trees properties.
	void pushTreeProperties();

	/// Loads trees mesh properties.
	void pushMeshProperties();

	/// Sets GUI at start of tree generator run.
	void setStartOfGenerationGUI();

	/// Sets GUI at end of tree generator run.
	void setEndOfGenerationGUI();

	/// Tries to load scene geometry from file location.
	/// \param location .obj file containing scene geometry.
	/// \return Result of load.
	bool tryLoadScene(const QString& location);

	/// Approximates good value for scene - model ratio based on scene bounds.
	void approximateSceneModelRatio();

	/// Adds new tree into tree generator.
	/// \param potentialRoot Tree root position.
	void addNewTree(const glm::vec3& potentialRoot);

	/// Deletes all trees in tree generator.
	void deleteAllTrees();
	///@}

	/// \name Grow main cycle methods.
	///@{
	/// Runs tree generation algorithm.
	/// \param iterations Number of algorithm iterations.
	void runIterations(size_t iterations);

	/// Loads meshes of trees.
	void loadTreeMeshes();

	/// Loads structures of trees.
	void loadTreeStructures();

	/// Buffers trees data into mapped OpenGL buffers.
	/// \param nodes Tree structure nodes buffer.
	/// \param internodes Tree structure internodes buffer.
	/// \param treesVertices Tree mesh buffer.
	/// \param treesTriangles Tree triangles buffer.
	void bufferData(void* nodes, void* internodes, void* treesVertices, void* treesTriangles);
	///@}

private slots:
	/// \name Grow tree - async run management thorugh signal and slots.
	///@{

	/// Handles end of generation and makes and buffers meshes.
	void finishedRunIterationsHandler();

	/// Handles switch to newly made trees.
	void finishedBufferDataHandler();

	/// Signal of automatic generation start in interval.
	void intervalTimeout();
	///@}

	/// \name Main menu widgets slots.
	///@{

	/// Handles selection of scene.
	void on_sceneBrowse_pushButton_clicked();

	/// Handles clearing of scene - setting empty scene.
	void on_sceneClear_pushButton_clicked();

	/// Handles change of scene model ratio.
	void on_sceneModelRatio_dSpinBox_valueChanged(double newValue);

	/// Makes root at position.
	void on_treeRenderWindow_makeRootAtPosition(glm::vec3 scenePosition);

	/// Grows whole tree.
	void on_growTree_pushButton_clicked();

	/// Starts generative cycles.
	void on_grow_pushButton_clicked();

	/// Handles setting automatic generation.
	void on_growInInterval_checkBox_toggled();
	
	/// Deletes all trees.
	void on_deleteRoots_pushButton_clicked();

	/// Clears all trees.
	void on_clearTrees_pushButton_clicked();

	/// Cancel tree generation.
	void on_cancel_pushButton_clicked();

	/// Sets default tree generation parameters.
	void on_baseParameters_pushButton_clicked();
	
	/// Handles loading of tree parameters.
	void on_loadParameters_pushButton_clicked();

	/// Handles saving of tree parameters.
	void on_saveParameters_pushButton_clicked();

	/// Handles saving of tree mesh.
	void on_saveMesh_pushButton_clicked();

	/// Switches to rendering of tree mesh.
	void on_showMesh_radioButton_clicked();

	/// Switches to rendering of tree mesh structure.
	void on_showMeshStructure_radioButton_clicked();

	/// Switches to rendering of tree structure.
	void on_showTreeStructure_radioButton_clicked();

	/// Starts or stops rendering of bounding box.
	void on_showBoundingBox_checkBox_toggled(bool checked);

	/// Resets camera into base settings for current scene.

	void on_resetCamera_pushButton_clicked();
	///@}

	/// \name Tree settings widgets.
	///@{
	/// Checks and handles change of perception distance with respect to occupancy radius.
	void on_perceptionDistance_dSpinBox_valueChanged(double newValue);

	/// Checks and handles change of occupancy radius with respect to perception distance.
	void on_occupancyRadius_dSpinBox_valueChanged(double newValue);

	/// Sets proper number of vertices in node.
	void on_verticesInNode_spinBox_editingFinished();

	/// Handles loading of diffuse bark texture into QImage.
	void on_diffuseMapBrowse_pushButton_clicked();

	/// Clears barks texture.
	void on_diffuseMapClear_pushButton_clicked();
	///@}

	// ===========================================================================================================
	// Save/load tree methods
	// ===========================================================================================================

	/// \ name Save/load support methods
	///@{
	/// Saves tree parameters into .xml file.
	void saveParameters(QTextStream& stream);

	/// Loads tree parameters from .xml file.
	void loadParameters(QTextStream& stream);

	/// Saves mesh into .obj file.
	void saveMesh(QTextStream& stream);
	///@}
};

#endif // TREEGEN_APP_H

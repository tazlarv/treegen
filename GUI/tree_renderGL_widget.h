/// Widget for rendering of OpenGL of TreeGen application.
/// This file uses Qt 5.10.1 - see Licenses in folder External libraries.
/// \file tree_renderGL_widget.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#ifndef TREE_RENDERGL_WIDGET_H
#define TREE_RENDERGL_WIDGET_H

#include <Treegen management/treegen_management.h>
#include <Camera/scene_camera.h>

#include <QtWidgets/qopenglwidget.h>
#include <QtGui/qopenglfunctions.h>
#include <glm/glm.hpp>

/// Widget for rendering of OpenGL of TreeGen application.
class TreeRenderGLWidget final : public QOpenGLWidget, protected QOpenGLFunctions {
Q_OBJECT

	std::shared_ptr<TreegenLib> mTreegen = nullptr;

	/// \name Mouse and keyboard variables.
	///@{
	bool mMouseLeftPressed = false;
	bool mAltPressed = false;
	QPointF mCursorLastPos = {0.f, 0.f};
	///@}

	/// \ name OpenGL data, properties and variables.
	///@{

	// OpenGL - rendering states
	bool mNonDefaultSceneLoaded = false;
	bool mBoundingBoxLoaded = false;
	bool mStructureLoaded = false;
	bool mMeshLoaded = false;

	bool mShowBoundingBox = true;
	bool mShowMesh = true;
	bool mShowMeshStructure = false;

	// Camera and light
	Camera::SceneCamera mCamera;
	QVector3D mLightPosition{};

	// Logarithmic depth buffer
	constexpr static float mNearPlane = 0.001f;
	constexpr static float mFarPlane = 1000000.f;
	const float mfarPlaneCoef = 2.f / log2(mFarPlane + 1.f);	// Uniform for logarithmic depth buffer.

	// Shaders:
	// Unique ptr is not enough -> needs to be deleted in destructor (when we have opengl context).
	std::unique_ptr<QOpenGLShaderProgram> mShaderStructure = nullptr;
	std::unique_ptr<QOpenGLShaderProgram> mShaderMesh = nullptr;
	bool mShadersLoaded = true;

	// Shader structure uniforms
	int mShStructureModelUn = 0;
	int mShStructureViewUn = 0;
	int mShStructureProjectionUn = 0;
	int mShStructureFarPlaneCoefUn = 0;
	int mShStructureColorUn = 0;

	// Shader mesh uniforms
	int mShMeshModelUn = 0;
	int mShMeshViewUn = 0;
	int mShMeshProjectionUn = 0;
	int mShMeshFarPlaneCoefUn = 0;

	int mShMeshLightPosUn = 0;
	int mShMeshViewPosUn = 0;
	int mShMeshLightAmbientUn = 0;
	int mShMeshLightDiffuseUn = 0;
	int mShMeshLightSpecularUn = 0;

	int mShMeshMaterialAmbientUn = 0;
	int mShMeshMaterialDiffuseUn = 0;
	int mShMeshMaterialSpecularUn = 0;
	int mShMeshMaterialShininessUn = 0;
	int mShMeshUseDiffuseMapUn = 0;
	int mShMeshDiffuseMapUn = 0;

	// Bounding box, root positions and scene buffers
	QOpenGLVertexArrayObject mVaoBoundingBox{};
	QOpenGLBuffer mVboBoundingBox{};

	QOpenGLVertexArrayObject mVaoTreeRoots{};
	QOpenGLBuffer mVboTreeRoots{};

	QOpenGLVertexArrayObject mVaoSceneGeometry{};
	QOpenGLBuffer mVboPositionsSceneGeometry{};
	QOpenGLBuffer mVboNormalsSceneGeometry{};
	QOpenGLBuffer mEboSceneGeometry{QOpenGLBuffer::Type::IndexBuffer};

	// Buffers A
	QOpenGLVertexArrayObject mVaoTreeNodesA{};
	QOpenGLBuffer mVboTreeNodesA{};

	QOpenGLVertexArrayObject mVaoTreeInternodesA{};
	QOpenGLBuffer mVboTreeInternodesA{};

	QOpenGLVertexArrayObject mVaoTreeMeshA{};
	QOpenGLBuffer mVboTreeMeshA{};
	QOpenGLBuffer mEboTreeMeshA{QOpenGLBuffer::Type::IndexBuffer};

	// Buffers B
	QOpenGLVertexArrayObject mVaoTreeNodesB{};
	QOpenGLBuffer mVboTreeNodesB{};

	QOpenGLVertexArrayObject mVaoTreeInternodesB{};
	QOpenGLBuffer mVboTreeInternodesB{};

	QOpenGLVertexArrayObject mVaoTreeMeshB{};
	QOpenGLBuffer mVboTreeMeshB{};
	QOpenGLBuffer mEboTreeMeshB{QOpenGLBuffer::Type::IndexBuffer};

	// Currently rendered buffers
	enum class Buffers { A, B };

	Buffers mRenderBuffers = Buffers::A;
	QOpenGLVertexArrayObject* mVaoTreeNodesCurrent = &mVaoTreeNodesA;
	QOpenGLVertexArrayObject* mVaoTreeInternodesCurrent = &mVaoTreeInternodesA;
	QOpenGLVertexArrayObject* mVaoTreeMeshCurrent = &mVaoTreeMeshA;

	// Current scene / tree sizes (rendered buffers)
	size_t mSceneTrianglesCnt = 0;
	size_t mRootsPositionsCnt = 0;

	size_t mTreeNodesCnt = 0;
	size_t mInternodesVerticesCnt = 0;
	size_t mTreeMeshTrianglesCnt = 0;

	// Future tree model properties (second buffers)
	size_t mTreeNodesSwappedCnt = 0;
	size_t mInternodesVerticesSwappedCnt = 0;
	size_t mTreeMeshTrianglesSwappedCnt = 0;

	std::unique_ptr<QOpenGLTexture> mBarkDiffuse = nullptr;

	glm::mat4 mProjectionMatrix{};
	glm::mat4 mRayCastingProjectionMatrix{};	///< Matrix for inverse transformation calculation.

	///@}

public:
	explicit TreeRenderGLWidget(QWidget* parent);
	TreeRenderGLWidget(const TreeRenderGLWidget&) = delete;
	TreeRenderGLWidget& operator=(const TreeRenderGLWidget&) = delete;
	TreeRenderGLWidget(TreeRenderGLWidget&&) = delete;
	TreeRenderGLWidget& operator=(TreeRenderGLWidget&&) = delete;
	~TreeRenderGLWidget();

	/// Assigns Tree generator library access to rendering window.
	/// \param treegenAccess Tree generator library representation.
	void setTreegenAccess(std::shared_ptr<TreegenLib> treegenAccess);

	/// Check if shaders were successfully loaded.
	/// \return Result of check.
	bool shadersLoaded() const { return mShadersLoaded; }

	/// Loads tree bark diffuse texture from image.
	/// \param diffuseTexture Image containing tree bark texture.
	void loadDiffuseTexture(const QImage& diffuseTexture);

	/// Clears loaded bark texture.
	void clearDiffuseTexture();

	/// Loads data for rendering of generator bounding box.
	void loadBoundingBox();

	/// Loads positions of tree roots for rendering.
	void loadRootsPositions();

	/// Loads scene geometry in form of mesh for rendering.
	/// \param vertexPos Positions of vertices.
	/// \param vertexNormal Normals of vertices.
	/// \param triangleIndices Mesh triangles.
	void loadScene(std::vector<float> vertexPos, std::vector<float> vertexNormal,
	               std::vector<unsigned> triangleIndices);

	/// Loads empty scene for rendering.
	void loadDefaultEmptyScene();

	/// Does necessary calculations after changing of scene.
	void onSceneChange();

	/// Sets light position based on scene sizes.
	void setLightPosition();

	/// Resets camera into base settings for current scene.
	void resetCamera();

	/// Starts or stops rendering of bounding box.
	/// \param show Flag - if bounding box should be shown or not.
	void showBoundingBox(bool show);

	/// Switches to rendering of tree structure.
	void showTreeStructure();

	/// Switches to rendering of tree mesh.
	void showMesh();

	/// Switches to rendering of tree mesh structure.
	void showMeshStructure();

	/// Allocates buffers and maps them into memory for async buffering.
	/// \param nodes Number of tree nodes.
	/// \param internodesVertices Number of tree internode vertices.
	/// \param meshVertices Number of vertices of tree mesh.
	/// \param meshTriangles Number of triangles of tree mesh.
	/// \return Pointers to buffers mapped into memory.
	std::tuple<void*, void*, void*, void*> getMappedBuffers(size_t nodes, size_t internodesVertices,
	                                                        size_t meshVertices, size_t meshTriangles);
	/// Swaps which set of buffers is rendered.
	void swapBuffers();

	/// Clears all buffers containing tree geometry.
	void clearTreesBuffers();

signals :
	/// Signal that someone tried to make new tree at position.
	/// \param scenePosition Potential position for new tree root.
	void makeRootAtPosition(glm::vec3 scenePosition);

private:
	// QOpenGLWidget basic events + openGL data loading

	/// Initalizes opengl variables on start of application.
	void initializeGL() override;

	/// Handles resize of application window.
	/// \param w New width.
	/// \param h New height.
	void resizeGL(int w, int h) override;

	/// Renders OpenGL.
	void paintGL() override;

	/// Gets tree root position from current cursor position in window rendering scene.
	/// \return Tuple where bool represents if returned glm vector is good position for root of new tree.
	std::tuple<bool, glm::vec3> getTreeRootPosition();

	/// Loads bounding box into buffer.
	void bufferBoundingBox();

	// Camera events + variables for them

	/// Sets focus for camera on mouse enter.
	void enterEvent(QEvent* qEvent) override;

	/// Clears focus for camera on mouse leave.
	void leaveEvent(QEvent* qEvent) override;

	/// Handles keybind for making new root at mouse position in scene and Alt keypress for camera movement.
	void keyPressEvent(QKeyEvent* qEvent) override;

	/// Handles start of dragging of mouse.
	void mousePressEvent(QMouseEvent* qEvent) override;

	/// Handles end of dragging of mouse.
	void mouseReleaseEvent(QMouseEvent* qEvent) override;

	/// Handles mouse movement - rotation and movement of camera.
	void mouseMoveEvent(QMouseEvent* qEvent) override;

	/// Handles camera zoom and movement forward and backward.
	void wheelEvent(QWheelEvent* qEvent) override;
};

#endif // TREE_RENDERGL_WIDGET_H

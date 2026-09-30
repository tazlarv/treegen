/// Utilities for usage of Qt and GLM libraries together.
/// \file glm_qt_utils.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#ifndef GLM_QT_UTILS_H
#define GLM_QT_UTILS_H

#include <QVector3D>
#include <glm/glm.hpp>

/// Transforms glm vector to qt vector.
/// \param vec GLM vector.
/// \return Qt vector.
inline QVector3D qVecGlmVec3D(const glm::vec3& vec) {
	return {vec.x, vec.y, vec.z};
}

/// Transforms Qt vector to glm vector.
/// \param vec Qt vector.
/// \return GLM vector.
inline glm::vec3 qVecGlmVec3D(const QVector3D& vec) {
	return {vec.x(), vec.y(), vec.z()};
}

#endif // GLM_QT_UTILS_H

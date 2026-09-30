/// Utilities for usage of OpenGL.
/// \file openGL_utils.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#ifndef OPENGL_UTILS_H
#define OPENGL_UTILS_H

#include <QtGui/qopenglbuffer.h>
#include <glm/vec3.hpp>

#include <vector>

namespace openGL_utils
{
	/// Pushes glm vector into vector of floats.
	/// \param vec Vector of floats.
	/// \param point GLM vector.
	void pushPointToVec(std::vector<float>& vec, const glm::vec3& point);

	/// Buffers cube into VBO.
	/// \param buffer VBO buffer.
	/// \param lower Lower corner of cube.
	/// \param upper Upper corner of cube.
	void bufferCube(QOpenGLBuffer& buffer, const glm::vec3& lower, const glm::vec3& upper);
}


#endif // OPENGL_UTILS_H

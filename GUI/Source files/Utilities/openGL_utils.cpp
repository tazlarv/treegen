/// Utilities for usage of OpenGL.
/// \file openGL_utils.cpp
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#include <stdafx.h>
#include <Utilities/openGL_utils.h>

#include <vector>

namespace openGL_utils
{
	void pushPointToVec(std::vector<float>& vec, const glm::vec3& point) {
		vec.push_back(point.x);
		vec.push_back(point.y);
		vec.push_back(point.z);
	}

	void bufferCube(QOpenGLBuffer& buffer, const glm::vec3& lower, const glm::vec3& upper) {
		std::vector<glm::vec3> corners{
			{lower.x, lower.y, upper.z},
			{upper.x, lower.y, upper.z},
			{upper.x, lower.y, lower.z},
			lower,
			{lower.x, upper.y, upper.z},
			upper,
			{upper.x, upper.y, lower.z},
			{lower.x, upper.y, lower.z}
		};

		std::vector<float> vertexBuffer{};
		for (int i = 0; i < 4; ++i) {
			pushPointToVec(vertexBuffer, corners[i]);
			pushPointToVec(vertexBuffer, corners[(i + 1) % 4]);

			pushPointToVec(vertexBuffer, corners[i + 4]);
			pushPointToVec(vertexBuffer, corners[((i + 1) % 4) + 4]);

			pushPointToVec(vertexBuffer, corners[i]);
			pushPointToVec(vertexBuffer, corners[i + 4]);
		}

		buffer.bind();
		buffer.allocate(vertexBuffer.data(), int(sizeof(float) * vertexBuffer.size()));
	}
}

/// Precompiled header for GUI project.
/// \file stdafx.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#ifndef STDAFX_GUI_H
#define STDAFX_GUI_H

// Disabled warning - nonstandard extension used : nameless struct/union
#pragma warning(push)
#pragma warning(disable: 4201) 
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#pragma warning(pop)

#include <QtWidgets>

#include <Utilities/glm_qt_utils.h>
#include <Utilities/openGL_utils.h>

#include <memory>

#include <tuple>
#include <vector>
#include <iostream>
#include <limits>

#endif // STDAFX_GUI_H

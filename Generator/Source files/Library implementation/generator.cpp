/// Tree generator public API.
/// \file generator.cpp
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#include <stdafx.h>
#include <Library interface/generator.h>

#include <Library implementation/generator_implementation.h>

namespace Treegen
{
	std::unique_ptr<Generator> Generator::createGenerator() {
		return std::make_unique<GeneratorImplementation>();
	}
}

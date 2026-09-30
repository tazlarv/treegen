/// Environment class.
/// \file environment.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <Scene/scene.h>
#include <Space colonization/sc_algorithm.h>
#include <Shadow propagation/shadow_propagation.h>

#include <memory>

namespace Treegen
{
	/// Container for all parts of environment simulation.
	struct Environment {
		Environment() {
			scene = std::make_shared<Scene::BoundedScene>();
			space = std::make_unique<SpaceCol::SpaceColonization>(scene);
			shadow = std::make_shared<ShadowProp::ShadowPropagation>();
		}

		std::shared_ptr<Scene::BoundedScene> scene = nullptr;
		std::shared_ptr<SpaceCol::SpaceColonization> space = nullptr;
		std::shared_ptr<ShadowProp::ShadowPropagation> shadow = nullptr;
	};
}

#endif // ENVIRONMENT_H

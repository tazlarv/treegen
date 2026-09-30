/// Abstract base class for redistribution of resources.
/// \file abstract_resources_model.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#ifndef EG_ABSTRACT_RESOURCES_MODEL_H
#define EG_ABSTRACT_RESOURCES_MODEL_H

#include <Tree primitives/tree_primitives.h>

#include <vector>

namespace Treegen
{
	namespace Signaling
	{
		/// Abstract base class for redistribution of resources.
		class AbstractResourcesModel {
		public:
			AbstractResourcesModel() = default;
			AbstractResourcesModel(const AbstractResourcesModel&) = delete;
			AbstractResourcesModel& operator=(const AbstractResourcesModel&) = delete;
			AbstractResourcesModel(AbstractResourcesModel&&) = delete;
			AbstractResourcesModel& operator=(AbstractResourcesModel&&) = delete;
			virtual ~AbstractResourcesModel() = default;

			/// Redistribution of resources in buds.
			/// \param root Root of tree.
			/// \param budsWithResouces Buds with resources to be redistributed.
			void virtual redistributeResources(const TreeNode& root, const std::vector<rwTreeBud>& budsWithResouces) = 0;
		};
	}
}

#endif // EG_ABSTRACT_RESOURCES_MODEL_H

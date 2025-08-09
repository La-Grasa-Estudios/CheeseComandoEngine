#include "ClassMetadata.h"

#include <Core/Logger.h>

using namespace ENGINE_NAMESPACE;

void ComponentMetadata::Init()
{
	Z_INFO("Init metadata registry");
}

void ComponentMetadata::AddComponentMapping(const char* componentManager, const char* serializedName)
{
	Z_INFO("Registering mapping class {}, serialized name: {}", componentManager, serializedName);
}

void ComponentMetadata::AddFieldMetadata(const char* clazz, const char* name, size_t offset)
{
	Z_INFO("Registering field for class {}, name: {}, offset: {}", clazz, name, offset);
}

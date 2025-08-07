#pragma once

#include <znmsp.h>

BEGIN_ENGINE

#define INTERNAL_COMPONENT_FIELD_STRUCT(clazz, name) Entity##clazz##Field##name##Type

#define DECLARE_COMPONENT(name, fileName) namespace Internal {								\
	 struct internalComponent##name##link##fileName {										\
		internalComponent##name##link##fileName##() {										\
			 ENGINE_NAMESPACE::ComponentMetadata::AddComponentMapping(#name, #fileName);	\
		}																					\
	 } objReg##name##fileName;																\
}
#define DECLARE_COMPONENT_FIELD(clazz, name)																\
namespace Internal																							\
{																											\
	  struct INTERNAL_COMPONENT_FIELD_STRUCT(clazz, name) {													\
		INTERNAL_COMPONENT_FIELD_STRUCT(clazz, name)()														\
		{																									\
			ENGINE_NAMESPACE::ComponentMetadata::AddFieldMetadata(#clazz, #name, offsetof(clazz, name));    \
		}																									\
	} internalRegField##clazz##name;																		\
																											\
}


struct ComponentFieldMetadata
{
	const char* Name;
	size_t Offset;
};

class ComponentMetadata
{
public:
	static void Init();
	static void AddComponentMapping(const char* componentManager, const char* serializedName);
	static void AddFieldMetadata(const char* clazz, const char* name, size_t offset);
};

END_ENGINE
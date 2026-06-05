#pragma once

#include <znmsp.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <glm/ext.hpp>
#include <json/json.hpp>

BEGIN_ENGINE

enum class ComponentDataType
{
	UNKNOWN,
	ENUM, // Just an alias to an int
};

/// <summary>
/// Custom datatypes defined in the edf
/// </summary>
struct ComponentTypedef
{
	ComponentDataType DataType;
	std::vector<uint32_t> EnumValues;
	std::vector<std::string> EnumDisplay;
};

struct ComponentFieldDef
{
	std::string FieldName;
	std::string DisplayName;
	std::string Description;
	std::string Type;
	std::string DisplayAs;
	glm::vec4 DefaultFloatVectorValue; // Used by vec2, vec3, vec4 and quat
	glm::ivec4 DefaultIntVectorValue; // Used by ivec2, ivec3 and ivec4
	std::string DefaultStrValue;
	int32_t DefaultIntValue;
	uint32_t DefaultUIntValue;
	float_t DefaultFloatValue;
	bool DefaultBoolValue;
};

struct ComponentObjectDef
{
	std::string Name;
	std::string Description;
	std::string SerializableName;
	std::vector<std::string> Dependencies;
	std::vector<ComponentFieldDef> Fields;
};

class ComponentLibrary
{
public:
	/// <summary>
	/// Initilizes the library, just parses all .edf files that are in the mounted vfs points (including pak files)
	/// </summary>
	ComponentLibrary();

	uint32_t GetValueForEnum(std::string_view enumname, std::string_view name);
	ComponentObjectDef& GetComponentDefinition(std::string_view name);
	std::vector<std::string>& GetComponentList();

private:
	void ParseEntityDefinitionFile(std::string_view path);
	void ParseDataType(nlohmann::json_abi_v3_11_2::json& dataType);
	void ParseComponent(nlohmann::json_abi_v3_11_2::json& component);
	std::unordered_map<std::string, ComponentTypedef> mDataTypes;
	std::unordered_map<std::string, ComponentObjectDef> mComponents;
	std::vector<std::string> mComponentList;
};

END_ENGINE
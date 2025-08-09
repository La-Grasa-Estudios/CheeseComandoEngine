#include "ComponentLibrary.h"

#include <VFS/ZVFS.h>

using namespace ENGINE_NAMESPACE;

ComponentLibrary::ComponentLibrary()
{
	auto files = ZVFS::GetAllOf(".edf");

	for (auto& file : files)
	{
		ParseEntityDefinitionFile(file);
	}
}

uint32_t ComponentLibrary::GetValueForEnum(std::string_view enumname, std::string_view name)
{
	if (auto e = mDataTypes.find(enumname.data()); e != mDataTypes.end())
	{
		if (e->second.DataType == ComponentDataType::ENUM)
		{
			for (int i = 0; i < e->second.EnumDisplay.size(); i++)
			{
				if (e->second.EnumDisplay[i].compare(name) == 0)
				{
					return e->second.EnumValues[i];
				}
			}
		}
	}

	return 0;
}

void ComponentLibrary::ParseEntityDefinitionFile(std::string_view path)
{
	auto file = ZVFS::GetFile(path.data());
	nlohmann::json json = nlohmann::json::parse(file->Str());

	if (auto optional = json.find("datatypes"); optional != json.end())
	{
		auto& dataTypes = optional.value();

		for (auto& datatype : dataTypes)
		{
			ParseDataType(datatype);
		}
	}

	auto& components = json["components"];

	for (auto& component : components)
	{
		ParseComponent(component);
	}
}

void ComponentLibrary::ParseDataType(nlohmann::json_abi_v3_11_2::json& datatype)
{
	std::string name = datatype["name"];
	std::string type = datatype["type"];
	std::string display = name;
	if (datatype.contains("display"))
		display = datatype["display"];

	ComponentTypedef typeDef{};

	// Parse enum values
	if (type.compare("enum") == 0)
	{
		typeDef.DataType = ComponentDataType::ENUM;
		
		auto& values = datatype["values"];

		uint32_t index = 0;

		for (auto& value : values)
		{
			typeDef.EnumDisplay.push_back(value);
			typeDef.EnumValues.push_back(index++);
		}
	}

	mDataTypes[name] = typeDef;
}

void ComponentLibrary::ParseComponent(nlohmann::json_abi_v3_11_2::json& component)
{
	ComponentObjectDef def{};
	def.Name = component["component"];
	def.Description = component["description"];

	if (auto optional = component.find("requires"); optional != component.end())
	{
		auto& dependencies = optional.value();

		for (auto& dep : dependencies)
		{
			def.Dependencies.push_back(dep);
		}
	}

	auto& fields = component["fields"];

	for (auto& field : fields)
	{
		ComponentFieldDef fieldDef{};

		fieldDef.FieldName = field["name"];
		fieldDef.Type = field["type"];

		if (field.contains("description"))
			fieldDef.Description = field["description"];
		if (field.contains("display"))
			fieldDef.DisplayName = field["display"];
		else
			fieldDef.DisplayName = fieldDef.FieldName;
		if (field.contains("displayas"))
			fieldDef.DisplayAs = field["displayas"];

		if (field.contains("default"))
		{
			if (mDataTypes.contains(fieldDef.Type))
			{
				auto& data = mDataTypes[fieldDef.Type];
				if (data.DataType == ComponentDataType::ENUM)
				{
					fieldDef.DefaultIntValue = GetValueForEnum(fieldDef.Type, field["default"]);
				}
			}
			if (fieldDef.Type.compare("bool") == 0)
			{
				fieldDef.DefaultBoolValue = field["default"];
			}
			if (fieldDef.Type.compare("int") == 0)
			{
				fieldDef.DefaultIntValue = field["default"];
			}
			if (fieldDef.Type.compare("uint") == 0)
			{
				fieldDef.DefaultUIntValue = field["default"];
			}
			if (fieldDef.Type.compare("float") == 0)
			{
				fieldDef.DefaultFloatValue = field["default"];
			}
			if (fieldDef.Type.compare("string") == 0)
			{
				fieldDef.DefaultStrValue = field["default"];
			}
			if (fieldDef.Type.compare("quat") == 0)
			{
				int count = 4;
				for (int i = 0; i < count; i++)
					fieldDef.DefaultFloatVectorValue[i] = field["default"][i];
			}
			if (fieldDef.Type.starts_with("vec"))
			{
				int count = std::stoi(fieldDef.Type.substr(3));
				for (int i = 0; i < count; i++)
					fieldDef.DefaultFloatVectorValue[i] = field["default"][i];
			}
			if (fieldDef.Type.starts_with("ivec"))
			{
				int count = std::stoi(fieldDef.Type.substr(4));
				for (int i = 0; i < count; i++)
					fieldDef.DefaultIntVectorValue[i] = field["default"][i];
			}
		}

		def.Fields.push_back(fieldDef);
	}

	mComponents[def.Name] = def;
}

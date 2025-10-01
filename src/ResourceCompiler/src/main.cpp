// Zircon Pak File Helper.cpp : Este archivo contiene la función "main". La ejecución del programa comienza y termina ahí.
//

#include "VFS/AssetPack.h"
#include "Core/JobManager.h"
#include "Core/Logger.h"
#include "Asset/AssetMaterial.h"
#include "Asset/AssetModel.h"

#include "Renderer/RendererContext.h"

#include "Sound/SngFile.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "md5.h"

#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <json/json.hpp>
#include <VFS/base64.hpp>
#include <sstream>

#include "bc7/Bc7Compress.h"

#include "zlib/ozlibstream.h"

#define AD_MIPMAP_IMPLEMENTATION
#include "ad_mipmap.h"

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "Util/HeapArray.h"

#include "Asset/ShaderPermutationFile.h"
#include <Core/NormByte.h>

#define DXT_ERROR 0xFF778899
#define WRITE_TO_VECTOR(dst, src, count) { int offset = dst.size(); for (int nb = 0; nb < count; nb++) { dst.push_back(0); } memcpy(dst.data() + offset, src, count); }

using namespace ENGINE_NAMESPACE;

uint8_t* UnpackDXT7Image(uint8_t* buffer, uint32_t width, uint32_t height, uint32_t channels, bool* isTransparent) {

	uint8_t* newBuffer = buffer;

	uint32_t texelSize = 4;

	if (channels != 4) {
		if (channels == 1) texelSize = 1;
		if (channels == 2) texelSize = 2;

		newBuffer = new uint8_t[width * height * texelSize];
	}

	if (channels <= 2)
	{
		memcpy(newBuffer, buffer, width * height * channels);
		return newBuffer;
	}

	uint32_t cSize = width * height * channels;
	uint32_t iter = cSize / channels;

	for (uint32_t i = 0; i < iter && channels > 2; i++) {

		if (channels == 4) {
			uint8_t r = buffer[i * 4 + 0];
			buffer[i * 4 + 0] = buffer[i * 4 + 2];
			buffer[i * 4 + 2] = r;
			uint8_t alpha = buffer[i * 4 + 3];
			if (alpha <= 250) {
				*isTransparent = true;
			}
		}

		if (channels == 3) {

			unsigned char r = ((unsigned char*)buffer)[i * 3 + 0];
			unsigned char g = ((unsigned char*)buffer)[i * 3 + 1];
			unsigned char b = ((unsigned char*)buffer)[i * 3 + 2];

			newBuffer[i * 4 + 0] = b;
			newBuffer[i * 4 + 1] = g;
			newBuffer[i * 4 + 2] = r;
			newBuffer[i * 4 + 3] = (unsigned char)255;

		}

	}

	if (channels != 4) {
		return newBuffer;
	}
	else {
		return buffer;
	}
}

Stratum::texturedxt_t FillDXT7(uint8_t* data, int width, int height, int nrChannels, char** ppData, uint32_t* ppSize)
{
	if (!(width % 4 == 0 && height % 4 == 0) || (nrChannels <= 2))
	{
		Stratum::texturedxt_t header;
		header.dxt_size = DXT_ERROR;
		return header;
	}

	uint32_t lodFactor = 0;

	bool transparent = false;

	uint8_t* brga_src = UnpackDXT7Image(data, width, height, nrChannels, &transparent);

	BC7::Bc7Result bc7 = BC7::Compress::Bc7Compress(brga_src, width, height);

	delete[] brga_src;
	if (nrChannels != 4) stbi_image_free(data);

	Stratum::texturedxt_t header;
	header.dxt_format = Stratum::DXT_FORMAT_BC7;
	header.dxt_width = width;
	header.dxt_height = height;
	header.dxt_size = bc7.size;
	header.dxt_transparent = transparent;

	ppData[0] = (char*)bc7.buffer;

	*ppSize = bc7.size;

	return header;
}

AdmBitmap* GenerateMipMaps(std::string file, int* levels, int type)
{

	int width, height, nrChannels;

	uint8_t* data = stbi_load(file.c_str(), &width, &height, &nrChannels, 4);

	if (type == Stratum::TEXTURE_TYPE_NORMAL && nrChannels >= 3)
	{
		uint8_t* newData = new uint8_t[width * height * 2];
		for (int i = 0; i < width * height; i++)
		{
			newData[i * 2 + 0] = data[i * nrChannels + 0];
			newData[i * 2 + 1] = data[i * nrChannels + 1];
		}
		stbi_image_free(data);
		data = newData;
		nrChannels = 2;
	}

	AdmBitmap bitmap;
	bitmap.width = width;
	bitmap.height = height;
	bitmap.bytes_per_pixel = nrChannels;
	bitmap.pixels = data;

	AdmBitmap* mipmaps = adm_generate_mipmaps(levels, &bitmap);

	return mipmaps;
}

struct DXTEntry
{
	Stratum::texturedxt_t header;
	char* data;
};

struct WriteData
{
	void* data;
	uint32_t size;
};

void ProcessBitmaps(AdmBitmap* bitmaps, int levels, std::string file, int dxt, int type)
{
	using namespace ENGINE_NAMESPACE;

	std::vector<DXTEntry> dxtEntries;

	for (int i = 0; i < levels && dxt == Stratum::DXT_FORMAT_BC7; i++)
	{
		AdmBitmap bitmap = bitmaps[i];

		if ((bitmap.width % 4 != 0 || bitmap.height % 4 != 0) && bitmap.width > 4 && bitmap.height > 4)
		{
			dxt = -1;
			break;
		}
	}

	if (dxt == Stratum::DXT_FORMAT_BC7)
	{
		for (int i = 0; i < levels; i++)
		{
			AdmBitmap bitmap = bitmaps[i];

			if (bitmap.width < 4 || bitmap.height < 4)
			{
				continue;
			}

			uint32_t size;
			char* data;
			texturedxt_t header = FillDXT7((uint8_t*)bitmap.pixels, bitmap.width, bitmap.height, bitmap.bytes_per_pixel, &data, &size);
			if (header.dxt_size == DXT_ERROR)
			{
				dxt = -1;
				if (type == TEXTURE_TYPE_NORMAL)
				{
					InfoLog("DXT Error: Normal Maps are currently not supported");
					break;
				}
				InfoLog("DXT Error: Textures using DXT compression should be power of 2 in resolution");
				break;
			}
			DXTEntry entry;
			entry.header = header;
			entry.data = data;
			dxtEntries.push_back(entry);
		}
	}

	textureheader_t header{};
	header.type = type;
	header.mip_count = levels;

	std::filesystem::path path(file);
	path.replace_extension(".ctex");

	//Z_INFO("Nb dxt entries: {}", dxtEntries.size());

	if (dxt == -1)
	{
		InfoLog("Compiling as rgba");
		for (int i = 0; i < levels; i++)
		{
			AdmBitmap bitmap = bitmaps[i];

			texturedxt_t header;

			bool transparent = false;

			uint32_t size = bitmap.width * bitmap.height * bitmap.bytes_per_pixel;

			uint8_t* copy = new uint8_t[size];
			memcpy(copy, bitmap.pixels, size);
			uint8_t* brga_src = UnpackDXT7Image(copy, bitmap.width, bitmap.height, bitmap.bytes_per_pixel, &transparent);
			delete[] brga_src;

			header.dxt_format = -1;
			header.dxt_width = bitmap.width;
			header.dxt_height = bitmap.height;
			header.dxt_size = size;
			header.dxt_transparent = transparent;
			header.nbChannels = bitmap.bytes_per_pixel;

			DXTEntry entry;
			entry.header = header;
			entry.data = (char*)bitmap.pixels;
			dxtEntries.push_back(entry);

			dxt = 0;
		}
	}

	if (dxt != -1)
	{
		InfoLog("Dumping");
		int baseOffset = sizeof(textureheader_t) + dxtEntries.size() * sizeof(texturedxt_t);

		std::vector<WriteData> writes;

		for (int i = 0; i < dxtEntries.size(); i++)
		{
			DXTEntry& entry = dxtEntries[i];

			entry.header.offset = baseOffset;

			std::stringstream strs;
			zlib::ozlibstream zlib(strs);

			zlib.write(entry.data, entry.header.dxt_size);

			zlib.close();

			std::string str = strs.str();
			char* data = new char[str.size()];
			memcpy(data, str.data(), str.size());

			WriteData write;
			write.data = data;
			write.size = str.size();

			writes.push_back(write);
			baseOffset += write.size;

			header.dataLength += write.size;
		}

		header.mip_count = dxtEntries.size();

		char* bigBlock = new char[header.dataLength];
		baseOffset = 0;
		for (int i = 0; i < writes.size(); i++)
		{
			memcpy(bigBlock + baseOffset, writes[i].data, writes[i].size);
			baseOffset += writes[i].size;
			free(writes[i].data);
		}

		std::ofstream out(path, std::ios::binary);
		out.write(reinterpret_cast<const char*>(&header), sizeof(textureheader_t));

		for (int i = 0; i < dxtEntries.size(); i++)
		{
			DXTEntry& entry = dxtEntries[i];
			out.write(reinterpret_cast<const char*>(&entry.header), sizeof(texturedxt_t));
		}

		out.write(bigBlock, header.dataLength);

		delete[] bigBlock;

		return;

	}

	std::ifstream in(file, std::ios::binary);

	if (!in.is_open())
	{
		return;
	}

	header.mip_count = 1;

	std::stringstream data;
	data << in.rdbuf();

	std::string str = data.str();

	std::stringstream strs;
	zlib::ozlibstream zlib(strs);

	zlib.write(str.data(), str.size());

	zlib.close();

	std::string str1 = strs.str();

	header.dataLength = str1.size();

	std::ofstream out(path, std::ios::binary);
	out.write(reinterpret_cast<const char*>(&header), sizeof(textureheader_t));
	out.write(str1.data(), str1.size());
}

void TextureMode(int dxt, int type, std::string file)
{
	using namespace ENGINE_NAMESPACE;

	int levels = 0;
	AdmBitmap* bitmaps = GenerateMipMaps(file, &levels, type);
	ProcessBitmaps(bitmaps, levels, file, dxt, type);
}

void AudioMode(std::vector<std::string> args, const char* src)
{
	if (args.size() != 2 && args.size() != 4)
	{
		Z_INFO("Invalid arguments!");
		return;
	}

	bool isLooped = args.size() == 4;
	std::string loopStart = "";
	std::string loopEnd = "";
	std::string container = "";

	std::string srcFile = src;

	if (srcFile.ends_with("wav"))
	{
		container = "wav";
	}

	if (srcFile.ends_with("mp3"))
	{
		container = "mp3";
	}

	if (container.empty())
	{
		Z_WARN("Invalid song container! expected [mp3, wav]");
		return;
	}

	if (isLooped)
	{
		loopStart = args[1].substr(7);
		loopEnd = args[2].substr(5);
	}

	ZVFS::Init();

	RefBinaryStream file = ZVFS::GetFile(src);

	SngHeader sngHeader{};
	SngMetadataHeader sngMetaHeader{};

	sngMetaHeader.MetadataCount = 4;

	SngMetadataValueKeyPair isLoopedKp = SngMetadataValueKeyPair("isLooped", (isLooped ? "true" : "false"));
	SngMetadataValueKeyPair loopStartKp = SngMetadataValueKeyPair("loopStart", loopStart);
	SngMetadataValueKeyPair loopEndKp = SngMetadataValueKeyPair("loopEnd", loopEnd);
	SngMetadataValueKeyPair containerKp = SngMetadataValueKeyPair("container", container);

	SngFileIndex fileIndex{};

	fileIndex.FileCount = 1;

	sngMetaHeader.MetadataLen = isLoopedKp.Size() + loopStartKp.Size() + loopEndKp.Size() + containerKp.Size();

	SngFileMetadata songMetadata = SngFileMetadata(std::string("song.").append(container));

	fileIndex.FileMetaLen = songMetadata.Size();

	uint64_t initialOffset = sngHeader.Size() + sngMetaHeader.Size() + fileIndex.Size() + sngMetaHeader.MetadataLen + songMetadata.Size();
	songMetadata.ContentIndex = initialOffset;
	songMetadata.ContentsLen = file->Size();

	std::string name = std::filesystem::path(srcFile).replace_extension("sng").string();

	std::ofstream out(name, std::ios::binary);

	sngHeader.Write(out);
	sngMetaHeader.Write(out);
	isLoopedKp.Write(out);
	loopEndKp.Write(out);
	loopStartKp.Write(out);
	containerKp.Write(out);
	fileIndex.Write(out);
	songMetadata.Write(out);
	SngFile::WriteFile(file->As<char>(), file->Size(), out, sngHeader);
	Z_INFO("Output: {}", name);
}

void UserIntefaceImageMode(const char* src)
{
	nlohmann::json json;
	std::ifstream in(src);
	if (!in.is_open())
	{
		printf("Invalid source file\n");
		return;
	}
	in >> json;
	in.close();
	auto& rscStates = json["states"];

	// We need to initialize and mount the default Data directory as root
	std::filesystem::current_path("../");
	ZVFS::Init();
	ZVFS::Mount("Data", true);

	for (auto& state : rscStates.items())
	{
		printf("State %s\n", state.key().c_str());

		auto& obj = state.value();
		std::string srcFile = obj["src"];
		printf("Embed: %s\n", srcFile.c_str());

		bool found = ZVFS::Exists(srcFile.c_str());
		printf("Found: %i\n", found);

		if (found)
		{
			auto file = ZVFS::GetFile(srcFile.c_str());
			auto b64 = base64::to_base64(file->Str());
			Z_INFO("Embedding {} {} bytes", srcFile, b64.size());
			obj["embed"] = b64;
		}
	}
	std::filesystem::current_path("./Bin");
	auto dump = json.dump();
	std::ofstream out(src);
	out << dump;
	out.close();
}

#define INPUT_STRING(name) std::string name; std::getline(std::cin, name)

int __cdecl main(int argc, char* argv[])
{
	if (argc < 2)
	{
		printf("Usage: ResourceCompiler [/nozlib] [/extract] [/root] src\n");
		printf("/nozlib: Dont use Zlib compression\n");
		printf("/extract: Unpacks the specified zpk file\n");
		printf("/uimg: Embeds all referenced resource files specified by the states object into the src .uimg file\n");

		printf("Usage: ResourceCompiler [/texture] -dxt:[bc7, bc5] [-normal, -normal-RG, -albedo, -metalness, -roughness] src\n");

		printf("Usage: ResourceCompiler [/model] -mtfolder=<name> src\n");
		printf("Usage: ResourceCompiler [/audio] -start=<loopstart> -end=<loopend> src\n");
		printf("Usage: ResourceCompiler [/audio] -start=<loopstart> -end=<loopend> src\n");
		printf("Usage: ResourceCompiler [/uimg] src\n");
		return 1;
	}

	std::vector<std::string> args;
	args.reserve(argc);

	for (int i = 1; i < argc; i++)
	{
		args.emplace_back(argv[i]);
	}

	//args.emplace_back("/model");
	//args.emplace_back("-mtfolder=deserto");
	//args.emplace_back("desert.glb");

	const char* src_name = nullptr;
	bool zlib = true;
	bool extract = false;
	bool isRoot = false;

	bool textureMode = false;
	int dxtMode = -1;
	int textureType = 0;

	bool modelMode = false;
	bool audioMode = false;
	bool uimgMode = false;
	
	std::vector<const char*> ignore;
	std::string modelName;

	for (int i = 0, n = (int)args.size(); i < n; i++)
	{
		const char* arg = args[i].c_str();

		if (arg[0] == '/')
		{
			if (strcmp(arg, "/texture") == 0)
			{
				textureMode = true;
				continue;
			}
			if (strcmp(arg, "/model") == 0)
			{
				modelMode = true;
				continue;
			}
			if (strcmp(arg, "/audio") == 0)
			{
				audioMode = true;
				continue;
			}
			if (strcmp(arg, "/uimg") == 0)
			{
				uimgMode = true;
				continue;
			}
			if (strcmp(arg, "/nozlib") == 0)
			{
				zlib = false;
				continue;
			}
			if (strcmp(arg, "/extract") == 0)
			{
				extract = true;
				continue;
			}
			if (strcmp(arg, "/root") == 0)
			{
				isRoot = true;
				continue;
			}
			
#if defined(_WIN32)
			else
			{
				printf("Unknown %s\n", arg);
				continue;
			}
#endif
		}

		if (arg[0] == '-')
		{
			if (strcmp(arg, "-dxt:bc7") == 0)
			{
				dxtMode = Stratum::DXT_FORMAT_BC7;
				continue;
			}
			if (strcmp(arg, "-dxt:bc5") == 0)
			{
				dxtMode = Stratum::DXT_FORMAT_BC5;
				continue;
			}
			if (strcmp(arg, "-normal") == 0)
			{
				textureType = Stratum::TEXTURE_TYPE_NORMAL;
				continue;
			}
			if (strcmp(arg, "-normal-RG") == 0)
			{
				textureType = Stratum::TEXTURE_TYPE_NORMAL2CH;
				continue;
			}
			if (strcmp(arg, "-metalness") == 0)
			{
				textureType = Stratum::TEXTURE_TYPE_METALNESS;
				continue;
			}
			if (strcmp(arg, "-roughness") == 0)
			{
				textureType = Stratum::TEXTURE_TYPE_ROUGHNESS;
				continue;
			}
			if (strcmp(arg, "-albedo") == 0)
			{
				textureType = Stratum::TEXTURE_TYPE_ALBEDO;
				continue;
			}
			if (strncmp(arg, "-mtfolder=", 10) == 0)
			{
				modelName = arg;
				modelName = modelName.substr(10);
				continue;
			}
#if defined(_WIN32)
			else
			{
				printf("Unknown %s\n", arg);
				continue;
			}
#endif
		}

		if (strncmp(arg, "ignore=", 7) == 0)
		{
			std::string str = arg + 7;
			if (str.find_first_of(' ') != std::string::npos)
			{
				str = str.substr(0, str.find_first_of(' '));
			}
			auto ptr = new char[str.size() + 1];
			memset(ptr, 0, str.size() + 1);
			memcpy(ptr, str.data(), str.size());

			InfoLog("Ignoring " << ptr);

			ignore.push_back((const char*)ptr);
			continue;
		}

		if (src_name == nullptr)
		{
			src_name = arg;
		}

	}

	if (!src_name) {

		printf("No input file/dir specified");
		return 1;

	}

	std::string file = src_name;

	if (file.ends_with("uimg"))
	{
		uimgMode = true;
	}

	if (uimgMode)
	{
		UserIntefaceImageMode(src_name);
		return 0;
	}

	if (textureMode)
	{
		TextureMode(dxtMode, textureType, file);
		return 0;
	}

	if (audioMode)
	{
		AudioMode(args, src_name);
		return 0;
	}

	if (file.ends_with(".zpk") || file.ends_with(".cso")) {
		extract = true;
	}

	if (extract) {

		

		if (file.ends_with(".cso"))
		{

			ZVFS::Init();

			SpfFile spf(file);

			std::filesystem::path dir = src_name;
			std::string out = src_name;
			std::string folder;

			for (int i = 0; i < out.size() - 4; i++)
			{
				folder += out[i];
			}

			folder += "/";

			try
			{
				std::filesystem::create_directories(folder);
			}
			catch (const std::exception&)
			{
								
			}

			for (auto meta : spf.Files)
			{
				std::string path = folder;
				path.append(meta.first);
				
				char* buffer = new char[meta.second->ContentsLen];
				spf.ReadFile(meta.second.get(), buffer);

				std::ofstream o(path, std::ios::binary);

				o.write(buffer, meta.second->ContentsLen);

				Z_INFO("Writing {}", path);

				delete[] buffer;
			}

			return 0;
		}

		if (!file.ends_with(".zpk")) {
			printf("%s is not an zpk|spv file!", src_name);
			return 2;
		}

		ENGINE_NAMESPACE::AssetPack::ExtractPakFile(file);
		return 0;
	}

	std::filesystem::path dir = src_name;
	std::string out = src_name;
	out.append(".zpk");
	std::filesystem::path outFile = out;
	outFile = outFile.filename().stem();

	if (!std::filesystem::is_directory(dir)) {
		printf("%s is not a valid directory!", src_name);
		return 3;
	}

	ENGINE_NAMESPACE::JobManager::Init(false);

	ENGINE_NAMESPACE::AssetPacker packer;
	packer.Compress = zlib;
	packer.Pack(dir.string(), outFile.string(), isRoot, ignore.data(), ignore.size());

	return 0;
}

// Ejecutar programa: Ctrl + F5 o menú Depurar > Iniciar sin depurar
// Depurar programa: F5 o menú Depurar > Iniciar depuración

// Sugerencias para primeros pasos: 1. Use la ventana del Explorador de soluciones para agregar y administrar archivos
//   2. Use la ventana de Team Explorer para conectar con el control de código fuente
//   3. Use la ventana de salida para ver la salida de compilación y otros mensajes
//   4. Use la ventana Lista de errores para ver los errores
//   5. Vaya a Proyecto > Agregar nuevo elemento para crear nuevos archivos de código, o a Proyecto > Agregar elemento existente para agregar archivos de código existentes al proyecto
//   6. En el futuro, para volver a abrir este proyecto, vaya a Archivo > Abrir > Proyecto y seleccione el archivo .sln

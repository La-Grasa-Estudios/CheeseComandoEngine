#pragma once

#include "znmsp.h"

#include <istream>
#include <ostream>
#include <map>

#include "VFS/ZVFS.h"

BEGIN_ENGINE

struct SngHeader
{
	char SngIdentify[6]; // Must be SNGPKG 
	uint32_t Version;
	char XorMask[16];

	SngHeader();

	bool IsValid();

	void Read(PakStream in);
	void Write(std::ostream& out) const;

	size_t Size();
};

struct SngMetadataHeader
{
	uint64_t MetadataLen;
	uint64_t MetadataCount;

	 void Read(PakStream in);
	 void Write(std::ostream& out) const;

	size_t Size();
};

struct SngMetadataValueKeyPair
{
	int32_t KeyLen;
	char* Key;
	int32_t ValueLen;
	char* Value;

	SngMetadataValueKeyPair();
	SngMetadataValueKeyPair(const std::string& key, const std::string& value);

	void Read(PakStream in);
	void Write(std::ostream& out) const;

	size_t Size();

	~SngMetadataValueKeyPair();
};

struct SngFileIndex
{
	uint64_t FileMetaLen;
	uint64_t FileCount;

	void Read(PakStream in);
	void Write(std::ostream& out) const;

	size_t Size();
};

struct SngFileMetadata
{
	uint8_t FilenameLen;
	char* Filename;
	uint64_t ContentsLen;
	uint64_t ContentIndex;

	SngFileMetadata();
	SngFileMetadata(const std::string& filename);

	void Read(PakStream in);
	void Write(std::ostream& out) const;

	size_t Size();

	~SngFileMetadata();
};

struct SngFile
{
	SngHeader Header;
	std::map<std::string, std::string> Metadata;
	std::map<std::string, Ref<SngFileMetadata>> Files;

	PakStream Stream;

	SngFile(const std::string& path);

	void ReadFile(SngFileMetadata* pMetadata, char* dst);

	static void WriteFile(char* src, size_t size, std::ofstream& out, const SngHeader& header);
	static void UnMaskFile(char* src, size_t size, const SngHeader& header);

};

class SngFileStream
{
public:

	SngFileStream(const SngFile& file, SngFileMetadata* pFileMetadata);
	SngFileStream(PakStream stream);

	size_t read(void* buffer, size_t size);
	void seekg(size_t pos, int mode = 0);
	void ignore(size_t size);
	size_t tellg();
	bool eof();
	bool is_open();
	bool is_self_contained();

private:
	PakStream m_PakStream;
	size_t m_StreamPosition;
	SngFileMetadata* m_FileMetadata;
	SngHeader m_FileHeader;
};

END_ENGINE
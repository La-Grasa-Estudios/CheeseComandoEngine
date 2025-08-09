#pragma once

#include "znmsp.h"

#include <istream>
#include <ostream>
#include <map>

#include "VFS/ZVFS.h"

BEGIN_ENGINE

struct SpfHeader
{
	char SpfIdentify[6]; // Must be SPFPKG 
	uint32_t Version;
	char XorMask[16];

	SpfHeader();

	bool IsValid();

	void Read(PakStream in);
	void Write(std::ostream& out) const;

	size_t Size();
};

struct SpfMetadataHeader
{
	uint64_t MetadataLen;
	uint64_t MetadataCount;

	 void Read(PakStream in);
	 void Write(std::ostream& out) const;

	size_t Size();
};

struct SpfMetadataValueKeyPair
{
	int32_t KeyLen;
	char* Key;
	int32_t ValueLen;
	char* Value;

	SpfMetadataValueKeyPair();
	SpfMetadataValueKeyPair(const std::string& key, const std::string& value);

	void Read(PakStream in);
	void Write(std::ostream& out) const;

	size_t Size();

	~SpfMetadataValueKeyPair();
};

struct SpfFileIndex
{
	uint64_t FileMetaLen;
	uint64_t FileCount;

	void Read(PakStream in);
	void Write(std::ostream& out) const;

	size_t Size();
};

struct SpfFileMetadata
{
	uint8_t FilenameLen;
	char* Filename;
	uint64_t ContentsLen;
	uint64_t ContentIndex;

	SpfFileMetadata();
	SpfFileMetadata(const std::string& filename);

	void Read(PakStream in);
	void Write(std::ostream& out) const;

	size_t Size();

	~SpfFileMetadata();
};

struct SpfFile
{
	SpfHeader Header;
	std::map<std::string, std::string> Metadata;
	std::map<std::string, Ref<SpfFileMetadata>> Files;

	PakStream Stream;

	SpfFile(const std::string& path);

	void ReadFile(SpfFileMetadata* pMetadata, char* dst);

	static void WriteFile(char* src, size_t size, std::ofstream& out, const SpfHeader& header);
	static void UnMaskFile(char* src, size_t size, const SpfHeader& header);

};

class SpfFileStream
{
public:

	SpfFileStream(const SpfFile& file, SpfFileMetadata* pFileMetadata);
	SpfFileStream(PakStream stream);

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
	SpfFileMetadata* m_FileMetadata;
	SpfHeader m_FileHeader;
};

END_ENGINE
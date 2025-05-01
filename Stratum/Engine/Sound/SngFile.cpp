#include "SngFile.h"

#include <random>

static std::random_device s_RandomDevice;
static std::mt19937_64 s_Engine(s_RandomDevice());
static std::uniform_int_distribution<int> s_UniformDistribution;

using namespace ENGINE_NAMESPACE;

void InitializeXorMask(char* xorMask)
{
	for (int i = 0; i < 16; i++)
	{
		char c = (char)(s_UniformDistribution(s_Engine));
		xorMask[i] = c;
	}
}

SngFile::SngFile(const std::string& path)
{

	PakStream stream = ZVFS::GetFileStream(path.c_str());

	this->Header.Read(stream);

	if (!Header.IsValid())
	{
		return;
	}

	SngMetadataHeader metadata;
	metadata.Read(stream);

	for (int i = 0; i < metadata.MetadataCount; i++)
	{
		SngMetadataValueKeyPair keyPair;
		keyPair.Read(stream);
		this->Metadata[keyPair.Key] = keyPair.Value;
	}

	SngFileIndex fileIndex;
	fileIndex.Read(stream);

	for (int i = 0; i < fileIndex.FileCount; i++)
	{
		Ref<SngFileMetadata> fileMeta = CreateRef<SngFileMetadata>();
		fileMeta->Read(stream);
		this->Files[fileMeta->Filename] = fileMeta;
	}

	Stream = stream;

}

void SngFile::ReadFile(SngFileMetadata* pMetadata, char* dst)
{
	Stream->seekg(pMetadata->ContentIndex);
	Stream->read(dst, pMetadata->ContentsLen);
	UnMaskFile(dst, pMetadata->ContentsLen, Header);
}

void SngFile::WriteFile(char* src, size_t size, std::ofstream& out, const SngHeader& header)
{
	for (int i = 0; i < size; i++)
	{
		char xorKey = header.XorMask[i % 16] ^ (i & 0xFF);
		char c = src[i] ^ xorKey;
		out.write(&c, 1);
	}
}

void SngFile::UnMaskFile(char* src, size_t size, const SngHeader& header)
{
	for (int i = 0; i < size; i++)
	{
		char xorKey = header.XorMask[i % 16] ^ (i & 0xFF);
		src[i] ^= xorKey;
	}
}

SngHeader::SngHeader()
{
	memcpy(SngIdentify, "SNGPKG", 6);
	Version = 1;
	InitializeXorMask(XorMask);
}

bool SngHeader::IsValid()
{
	return strncmp(SngIdentify, "SNGPKG", 6) == 0;
}

void SngHeader::Read(PakStream in)
{
	in->read(SngIdentify, 6);
	in->read((char*)&Version, sizeof(uint32_t));
	in->read(XorMask, 16);
}

void SngHeader::Write(std::ostream& out) const
{
	out.write(SngIdentify, 6);
	out.write((char*)&Version, sizeof(uint32_t));
	out.write(XorMask, 16);
}

size_t SngHeader::Size()
{
	return 6 + 16 + sizeof(uint32_t);
}

void SngMetadataHeader::Read(PakStream in)
{
	in->read((char*)&MetadataLen, sizeof(uint64_t));
	in->read((char*)&MetadataCount, sizeof(uint64_t));
}

void SngMetadataHeader::Write(std::ostream& out) const
{
	out.write((char*)&MetadataLen, sizeof(uint64_t));
	out.write((char*)&MetadataCount, sizeof(uint64_t));
}

size_t SngMetadataHeader::Size()
{
	return sizeof(uint64_t) * 2;
}

SngMetadataValueKeyPair::SngMetadataValueKeyPair()
{
	KeyLen = 0;
	Key = NULL;
	ValueLen = 0;
	Value = NULL;
}

SngMetadataValueKeyPair::SngMetadataValueKeyPair(const std::string& key, const std::string& value)
{
	KeyLen = key.size();
	Key = new char[key.size()];
	memcpy(Key, key.c_str(), key.size());

	ValueLen = value.size();
	Value = new char[value.size()];
	memcpy(Value, value.c_str(), value.size());
}

void SngMetadataValueKeyPair::Read(PakStream in)
{
	in->read((char*)&KeyLen, sizeof(int32_t));
	Key = new char[KeyLen+1];
	memset(Key, 0, KeyLen + 1);
	in->read(Key, KeyLen);
	in->read((char*)&ValueLen, sizeof(int32_t));
	Value = new char[ValueLen+1];
	memset(Value, 0, ValueLen + 1);
	in->read(Value, ValueLen);
}

void SngMetadataValueKeyPair::Write(std::ostream& out) const
{
	out.write((char*)&KeyLen, sizeof(int32_t));
	out.write(Key, KeyLen);
	out.write((char*)&ValueLen, sizeof(int32_t));
	out.write(Value, ValueLen);
}

size_t SngMetadataValueKeyPair::Size()
{
	return KeyLen + ValueLen + sizeof(int32_t) * 2;
}

SngMetadataValueKeyPair::~SngMetadataValueKeyPair()
{
	if (Key) delete[] Key;
	if (Value) delete[] Value;
}

void SngFileIndex::Read(PakStream in)
{
	in->read((char*)&FileMetaLen, sizeof(uint64_t));
	in->read((char*)&FileCount, sizeof(uint64_t));
}

void SngFileIndex::Write(std::ostream& out) const
{
	out.write((char*)&FileMetaLen, sizeof(uint64_t));
	out.write((char*)&FileCount, sizeof(uint64_t));
}

size_t SngFileIndex::Size()
{
	return sizeof(uint64_t) * 2;
}

SngFileMetadata::SngFileMetadata()
{
}

SngFileMetadata::SngFileMetadata(const std::string& filename)
{
	FilenameLen = filename.size();
	Filename = new char[filename.size()];
	memcpy(Filename, filename.c_str(), filename.size());
}

void SngFileMetadata::Read(PakStream in)
{
	in->read((char*)&FilenameLen, sizeof(uint8_t));
	Filename = new char[FilenameLen + 1];
	memset(Filename, 0, FilenameLen + 1);
	in->read(Filename, FilenameLen);
	in->read((char*)&ContentsLen, sizeof(uint64_t));
	in->read((char*)&ContentIndex, sizeof(uint64_t));
}

void SngFileMetadata::Write(std::ostream& out) const
{
	out.write((char*)&FilenameLen, sizeof(uint8_t));
	out.write(Filename, FilenameLen);
	out.write((char*)&ContentsLen, sizeof(uint64_t));
	out.write((char*)&ContentIndex, sizeof(uint64_t));
}

size_t SngFileMetadata::Size()
{
	return FilenameLen + sizeof(uint8_t) + sizeof(uint64_t) * 2;
}

SngFileMetadata::~SngFileMetadata()
{
	if (Filename) delete[] Filename;
}

SngFileStream::SngFileStream(const SngFile& file, SngFileMetadata* pFileMetadata)
{
	m_StreamPosition = 0;
	m_FileMetadata = pFileMetadata;
	m_PakStream = file.Stream;
	m_FileHeader = file.Header;
}

SngFileStream::SngFileStream(PakStream stream)
{
	m_PakStream = stream;
	m_StreamPosition = 0;
	m_FileMetadata = NULL;
	m_FileHeader = {};
}

size_t SngFileStream::read(void* buffer, size_t size)
{
	if (!m_FileMetadata)
	{
		return m_PakStream->read(buffer, size);
	}

	char* buff = (char*)buffer;

	size_t filePointer = m_StreamPosition + m_FileMetadata->ContentIndex;
	size_t g = m_PakStream->tellg();
	if (g != filePointer)
	{
		m_PakStream->seekg(filePointer);
	}
	size_t readed = m_PakStream->read(buffer, size);

	for (int i = 0; i < readed; i++)
	{
		size_t index = i + m_StreamPosition;
		char xorKey = m_FileHeader.XorMask[index % 16] ^ (index & 0xFF);
		buff[i] ^= xorKey;
	}

	m_StreamPosition += readed;
	return readed;
}

void SngFileStream::seekg(size_t pos, int mode)
{
	if (!m_FileMetadata)
	{
		m_PakStream->seekg(pos, mode);
		return; 
	}
	if (mode == SEEK_CUR)
	{
		m_StreamPosition += pos;
		return;
	}
	if (mode == SEEK_END)
	{
		return;
	}
	m_StreamPosition = pos;
}
void SngFileStream::ignore(size_t size)
{
	m_PakStream->ignore(size);
	m_StreamPosition += size;
}

size_t SngFileStream::tellg()
{
	if (!m_FileMetadata)
	{
		return m_PakStream->tellg();
	}
	return m_StreamPosition;
}

bool SngFileStream::eof()
{
	return m_PakStream->eof();
}

bool SngFileStream::is_open()
{
	return m_PakStream->is_open();
}

bool SngFileStream::is_self_contained()
{
	return !m_FileMetadata;
}

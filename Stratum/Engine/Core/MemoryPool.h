#pragma once

#include <cstdint>
#include <vector>
#include "znmsp.h"
#include "Core/Ref.h"

BEGIN_ENGINE

struct MemoryPool {

public:

	MemoryPool(uint32_t size) {
		m_PtrOffset = 0;
		m_Size = size * 1024 * 1024;
		m_MemoryBlock = new char[size * 1024 * 1024];
		m_FreeBlocks = CreateRef <std::vector<MemoryBlock>>();
	}

	template<typename T, typename... Args>
	auto alloc(Args... args) {
		uint32_t size = static_cast<uint32_t>(sizeof(T));

		uint32_t ptr = 99999999;
		std::vector<MemoryBlock>::iterator itt;

		for (auto i = m_FreeBlocks->begin(); i != m_FreeBlocks->end(); ++i) {

			MemoryBlock& block = *i;
			if (block.size == size) {
				ptr = block.position;
				itt = i;
				break;
			}

		}

		if (ptr == 99999999 ){
			ptr = m_PtrOffset;
			m_PtrOffset += size;
		}
		else {
			m_FreeBlocks->erase(itt);
		}
		
		
		void* pObj = m_MemoryBlock + ptr;
		return new (pObj) T(args...);
	}

	template<typename T>
	void dealloc(T* ptr) {

		char* diff = (char*)((char*)ptr - m_MemoryBlock);
		MemoryBlock block = MemoryBlock(static_cast<uint32_t>((uint64_t)diff), static_cast<uint32_t>(sizeof(T)));
		m_FreeBlocks->push_back(block);

	}

	uint32_t GetFreeSpace() {
		return m_Size - m_PtrOffset;
	}

private:

	struct MemoryBlock {
		uint32_t position;
		uint32_t size;
		MemoryBlock(uint32_t position, uint32_t size) {
			this->position = position;
			this->size = size;
		}
	};

	uint32_t m_PtrOffset;
	uint32_t m_Size;
	Ref<std::vector<MemoryBlock>> m_FreeBlocks;
	char* m_MemoryBlock;

};

END_ENGINE
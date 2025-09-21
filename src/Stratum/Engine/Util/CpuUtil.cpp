#include "StrUtil.h"
#include "CpuUtil.h"

#include <array>

using namespace ENGINE_NAMESPACE;

static std::array<int, 4> cpuid(int leaf, int subleaf = 0) {
	std::array<int, 4> regs;
#if defined(_MSC_VER)
	__cpuidex(regs.data(), leaf, subleaf);
#elif defined(__GNUC__) || defined(__clang__)
	__cpuid_count(leaf, subleaf, regs[0], regs[1], regs[2], regs[3]);
#endif
	return regs;
}

Utils::CpuUtil::CpuUtil()
{
	auto regs1 = cpuid(1);
	bool sse41 = (regs1[2] & (1 << 19)) != 0;
	bool avx = (regs1[2] & (1 << 28)) != 0;

	auto regs7 = cpuid(7);
	bool avx2 = (regs7[1] & (1 << 5)) != 0;
	bool avx512f = (regs7[1] & (1 << 16)) != 0;
	bool avx512bw = (regs7[1] & (1 << 30)) != 0;

	CpuUtil::SupportsSSE41 = sse41;
	CpuUtil::SupportsAVX2 = avx && avx2;
	CpuUtil::SupportsAVX512F = avx && avx512f;
	CpuUtil::SupportsAVX512BW = avx && avx512bw;
}

#include "machine.hpp"

#include <cstring>

namespace tinykvm {

void Machine::copy_to_guest(address_t addr, const void* vsrc, size_t len, bool zeroes)
{
	if (m_forked)
	{
		auto* src = (const uint8_t *)vsrc;
		while (len != 0)
		{
			const size_t offset = addr & (vMemory::PAGE_SIZE-1);
			const size_t size = std::min(vMemory::PAGE_SIZE - offset, len);
			auto* page = memory.get_writable_page(addr & ~(uint64_t) 0xFFF, zeroes);
			std::copy(src, src + size, &page[offset]);

			addr += size;
			src += size;
			len -= size;
		}
		return;
	}
	/* Original VM uses identity-mapped memory */
	auto* dst = memory.safely_at(addr, len);
	std::memcpy(dst, vsrc, len);
}

void Machine::copy_from_guest(void* vdst, address_t addr, size_t len)
{
	if (m_forked)
	{
		auto* dst = (uint8_t *)vdst;
		while (len != 0)
		{
			const size_t offset = addr & (vMemory::PAGE_SIZE-1);
			const size_t size = std::min(vMemory::PAGE_SIZE - offset, len);
			auto* page = memory.get_userpage_at(addr & ~(uint64_t) 0xFFF);
			std::copy(&page[offset], &page[offset + size], dst);

			addr += size;
			dst += size;
			len -= size;
		}
		return;
	}
	/* Original VM uses identity-mapped memory */
	auto* src = memory.safely_at(addr, len);
	std::memcpy(vdst, src, len);
}

void Machine::unsafe_copy_from_guest(void* vdst, address_t addr, size_t len)
{
	if (m_forked)
	{
		auto* dst = (uint8_t *)vdst;
		while (len != 0)
		{
			const size_t offset = addr & (vMemory::PAGE_SIZE-1);
			const size_t size = std::min(vMemory::PAGE_SIZE - offset, len);
			auto* page = memory.get_kernelpage_at(addr & ~(uint64_t) 0xFFF);
			std::copy(&page[offset], &page[offset + size], dst);

			addr += size;
			dst += size;
			len -= size;
		}
		return;
	}
	/* Original VM uses identity-mapped memory */
	auto* src = memory.at(addr, len);
	std::memcpy(vdst, src, len);
}

} // tinykvm

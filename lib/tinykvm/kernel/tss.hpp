#pragma once
#include <cstdint>

extern void setup_amd64_tss(
	uint64_t tss_addr, char* tss_ptr, uint64_t gdt_addr, char* gdt_ptr);

extern void setup_amd64_tss_regs(struct kvm_sregs& sregs, uint64_t tss_addr);

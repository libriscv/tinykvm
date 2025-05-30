#include "idt.hpp"

#include "../common.hpp"
#include <array>
#include <cstdio>
#include <cstring>
#include <linux/kvm.h>
struct kvm_sregs;

namespace tinykvm {

// 64-bit IDT entry
struct IDTentry {
	uint16_t offset_1;  // offset bits 0..15
	uint16_t selector;  // a code segment selector in GDT or LDT
	uint8_t  ist;       // 3-bit interrupt stack table offset
	uint8_t  type_attr; // type and attributes, see below
	uint16_t offset_2;  // offset bits 16..31
	uint32_t offset_3;  // 32..63
	uint32_t zero2;
};
static_assert(sizeof(IDTentry) == 16, "AMD64 IDT entries are 16-bytes");

#define IDT_GATE_INTR 0x0e
#define IDT_CPL0      0x00
#define IDT_CPL3      0x60
#define IDT_PRESENT   0x80

struct IDT
{
	/* Just enough for CPU exceptions and 1 timer interrupt */
	std::array<IDTentry, 33> entry;
};

union addr_helper {
	uint64_t whole;
	struct {
		uint16_t lo16;
		uint16_t hi16;
		uint32_t top32;
	};
};

static void set_entry(
	IDTentry& idt_entry,
	uint64_t handler,
	uint16_t segment_sel,
	uint8_t  attributes)
{
	addr_helper addr { .whole = handler };
	idt_entry.offset_1  = addr.lo16;
	idt_entry.offset_2  = addr.hi16;
	idt_entry.offset_3  = addr.top32;
	idt_entry.selector  = segment_sel;
	idt_entry.type_attr = attributes;
	idt_entry.ist       = 1;
	idt_entry.zero2     = 0;
}

void set_exception_handler(void* area, uint8_t vec, uint64_t handler)
{
	auto& idt = *(IDT *)area;
	set_entry(idt.entry[vec], handler, 0x8, IDT_PRESENT | IDT_CPL0 | IDT_GATE_INTR);
	/* Use second IST for double faults */
	//idt.entry[vec].ist = (vec != 8) ? 1 : 2;
}

/* unsigned interrupts[] = { ... } */
#include "builtin/kernel_assembly.h"
static_assert(sizeof(interrupts) > 10 && sizeof(interrupts) <= 4096,
	"Interrupts array must be container within a 4KB page");

const iasm_header& interrupt_header() {
	return *(const iasm_header*) &interrupts[0];
}
iasm_header& mutable_interrupt_header() {
	return *(iasm_header*) &interrupts[0];
}

void setup_amd64_exception_regs(struct kvm_sregs& sregs, uint64_t addr)
{
	sregs.idt.base  = addr;
	sregs.idt.limit = sizeof(IDT) - 1;
}

void setup_amd64_exceptions(uint64_t addr, void* area, void* except_area)
{
	uint64_t offset = addr + interrupt_header().vm64_exception;
	for (int i = 0; i <= 20; i++) {
		if (i == 15) continue;
		//printf("Exception handler %d at 0x%lX\n", i, offset);
		set_exception_handler(area, i, offset);
		offset += interrupt_header().vm64_except_size;
	}
	// Program the timer interrupt (which sends NMI)
	offset += interrupt_header().vm64_except_size;
	set_exception_handler(area, 32, offset);
	// Install exception handling code
	std::memcpy(except_area, interrupts, sizeof(interrupts));
}

TINYKVM_COLD()
void print_exception_handlers(const void* area)
{
	auto* idt = (IDT*) area;
	for (unsigned i = 0; i < idt->entry.size(); i++) {
		const auto& entry = idt->entry[i];
		addr_helper addr;
		addr.lo16 = entry.offset_1;
		addr.hi16 = entry.offset_2;
		addr.top32 = entry.offset_3;
		printf("IDT %u: func=0x%lX sel=0x%X p=%d dpl=%d type=0x%X ist=%u\n",
			i, addr.whole, entry.selector, entry.type_attr >> 7,
			(entry.type_attr >> 5) & 0x3, entry.type_attr & 0xF, entry.ist);
	}
}

struct AMD64_Ex {
	const char* name;
	bool        has_code;
};
static constexpr std::array<AMD64_Ex, 34> exceptions =
{
	AMD64_Ex{"Divide-by-zero Error", false},
	AMD64_Ex{"Debug", false},
	AMD64_Ex{"Non-Maskable Interrupt", false},
	AMD64_Ex{"Breakpoint", false},
	AMD64_Ex{"Overflow", false},
	AMD64_Ex{"Bound Range Exceeded", false},
	AMD64_Ex{"Invalid Opcode", false},
	AMD64_Ex{"Device Not Available", false},
	AMD64_Ex{"Double Fault", true},
	AMD64_Ex{"Reserved", false},
	AMD64_Ex{"Invalid TSS", true},
	AMD64_Ex{"Segment Not Present", true},
	AMD64_Ex{"Stack-Segment Fault", true},
	AMD64_Ex{"General Protection Fault", true},
	AMD64_Ex{"Page Fault", true},
	AMD64_Ex{"Reserved", false},
	AMD64_Ex{"x87 Floating-point Exception", false},
	AMD64_Ex{"Alignment Check", true},
	AMD64_Ex{"Machine Check", false},
	AMD64_Ex{"SIMD Floating-point Exception", false},
	AMD64_Ex{"Virtualization Exception", false},
	AMD64_Ex{"Reserved", false},
	AMD64_Ex{"Reserved", false},
	AMD64_Ex{"Reserved", false},
	AMD64_Ex{"Reserved", false},
	AMD64_Ex{"Reserved", false},
	AMD64_Ex{"Reserved", false},
	AMD64_Ex{"Reserved", false},
	AMD64_Ex{"Reserved", false},
	AMD64_Ex{"Reserved", false},
	AMD64_Ex{"Security Exception", false},
	AMD64_Ex{"Reserved", false},
	AMD64_Ex{"Reserved", false},
	AMD64_Ex{"Execution Timeout", false},
};

const char* amd64_exception_name(uint8_t intr) {
	return exceptions.at(intr).name;
}
bool amd64_exception_code(uint8_t intr) {
	return exceptions.at(intr).has_code;
}

}

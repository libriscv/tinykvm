#include "machine.hpp"

#include "kernel/idt.hpp"
#include "kernel/memory_layout.hpp"
#include <linux/kvm.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <time.h>

#define PRINTER(printer, buffer, fmt, ...) \
	printer(buffer, \
		snprintf(buffer, sizeof(buffer), \
		fmt, ##__VA_ARGS__));
extern "C" int gettid();

namespace tinykvm {
	static constexpr bool VERBOSE_TIMER = false;

void Machine::vCPU::run(uint32_t ticks)
{
	this->timer_ticks = ticks;
	if (timer_ticks != 0) {
		const struct itimerspec its {
			/* Interrupt every 20ms after timeout. This makes sure
			   that we will eventually exit all blocking calls and
			   at the end exit KVM_RUN to timeout the request. If
			   there is a blocking loop that doesn't exit properly,
			   the 20ms recurring interruption should not cause too
			   much wasted CPU-time. */
			.it_interval = {
				.tv_sec = 0, .tv_nsec = 20'000'000L
			},
			/* The execution timeout. */
			.it_value = {
				.tv_sec = ticks / 1000,
				.tv_nsec = (ticks % 1000) * 1000000L
			}
		};
		timer_settime(this->timer_id, 0, &its, nullptr);
		if constexpr (VERBOSE_TIMER) {
			printf("Timer %p enabled\n", timer_id);
		}
	}

	/* When an exception happens during KVM_RUN, we will need to
	   intercept it, in order to disable the timeout timer.
	   TODO: Convert timer disable to local destructor. */
	try {
		this->stopped = false;
		while(run_once());
	} catch (...) {
		disable_timer();
		throw;
	}

	disable_timer();
}
void Machine::vCPU::disable_timer()
{
	if (timer_ticks != 0) {
		this->timer_ticks = 0;
		struct itimerspec its;
		__builtin_memset(&its, 0, sizeof(its));
		timer_settime(this->timer_id, 0, &its, nullptr);
		if constexpr (VERBOSE_TIMER) {
			printf("Timer %p disabled\n", timer_id);
		}
	}
}

long Machine::vCPU::run_once()
{
	int result = ioctl(this->fd, KVM_RUN, 0);
	if (UNLIKELY(result < 0)) {
		if (this->timer_ticks) {
			if constexpr (VERBOSE_TIMER) {
				printf("Timer %p triggered\n", timer_id);
			}
			timeout_exception("Timeout Exception", this->timer_ticks);
		} else if (errno == EINTR) {
			/* XXX: EINTR but not a timeout, return to execution? */
			return KVM_EXIT_UNKNOWN;
		}
		else {
			machine_exception("KVM_RUN failed");
		}
	}

	switch (kvm_run->exit_reason) {
	case KVM_EXIT_HLT:
		machine_exception("Halt from kernel space", 5);

	case KVM_EXIT_DEBUG:
		return KVM_EXIT_DEBUG;

	case KVM_EXIT_FAIL_ENTRY:
		machine_exception("Failed to start guest! Misconfigured?", KVM_EXIT_FAIL_ENTRY);

	case KVM_EXIT_SHUTDOWN:
		machine_exception("Shutdown! Triple fault?", 32);

	case KVM_EXIT_IO:
		if (kvm_run->io.direction == KVM_EXIT_IO_OUT) {
		if (kvm_run->io.port == 0x0) {
			const char* data = ((char *)kvm_run) + kvm_run->io.data_offset;
			const uint32_t intr = *(uint32_t *)data;
			if (intr != 0xFFFF) {
				machine->system_call(intr);
				if (this->stopped) return 0;
				return KVM_EXIT_IO;
			} else {
				this->stopped = true;
				return 0;
			}
		}
		else if (kvm_run->io.port >= 0x80 && kvm_run->io.port < 0x100) {
			auto intr = kvm_run->io.port - 0x80;
			if (intr == 14) // Page fault
			{
				auto regs = registers();
				const uint64_t addr = regs.rdi & ~(uint64_t) 0x8000000000000FFF;
#ifdef VERBOSE_PAGE_FAULTS
				char buffer[256];
				#define PV(val, off) \
					{ uint64_t value; machine->unsafe_copy_from_guest(&value, regs.rsp + off, 8); \
					PRINTER(machine->m_printer, buffer, "Value %s: 0x%lX\n", val, value); }
				try {
					PV("Origin SS",  48);
					PV("Origin RSP", 40);
					PV("Origin RFLAGS", 32);
					PV("Origin CS",  24);
					PV("Origin RIP", 16);
					PV("Error code", 8);
				} catch (...) {}
				PRINTER(machine->m_printer, buffer,
					"*** %s on address 0x%lX (0x%llX)\n",
					amd64_exception_name(intr), addr, regs.rdi);
#endif
				/* Page fault handling */
				/* We should be in kernel mode, otherwise it's fishy! */
				if (UNLIKELY(regs.rip >= INTR_ASM_ADDR+0x1000)) {
					machine_exception("Security violation", intr);
				}

				machine->memory.get_writable_page(addr, false);
				return KVM_EXIT_IO;
			}
			else if (intr == 1) /* Debug trap */
			{
				machine->m_on_breakpoint(*machine);
				return KVM_EXIT_IO;
			}
			/* CPU Exception */
			this->handle_exception(intr);
			machine_exception(amd64_exception_name(intr), intr);
		} else {
			/* Custom Output handler */
			const char* data = ((char *)kvm_run) + kvm_run->io.data_offset;
			machine->m_on_output(*machine, kvm_run->io.port, *(uint32_t *)data);
		}
		} else { // IN
			/* Custom Input handler */
			const char* data = ((char *)kvm_run) + kvm_run->io.data_offset;
			machine->m_on_input(*machine, kvm_run->io.port, *(uint32_t *)data);
		}
		if (this->stopped) return 0;
		return KVM_EXIT_IO;

	case KVM_EXIT_MMIO: {
			char buffer[256];
			PRINTER(machine->m_printer, buffer,
				"Write outside of physical memory at 0x%llX\n",
				kvm_run->mmio.phys_addr);
			machine_exception("Memory write outside physical memory (out of memory?)",
							  kvm_run->mmio.phys_addr);
		}
	case KVM_EXIT_INTERNAL_ERROR:
		machine_exception("KVM internal error");
	}
	char buffer[256];
	PRINTER(machine->m_printer, buffer,
		"Unexpected exit reason %d\n", kvm_run->exit_reason);
	machine_exception("Unexpected KVM exit reason",
		kvm_run->exit_reason);
}

TINYKVM_COLD()
long Machine::step_one()
{
	struct kvm_guest_debug dbg;
	dbg.control = KVM_GUESTDBG_ENABLE | KVM_GUESTDBG_SINGLESTEP;

	if (ioctl(vcpu.fd, KVM_SET_GUEST_DEBUG, &dbg) < 0) {
		machine_exception("KVM_RUN failed");
	}

	return vcpu.run_once();
}

TINYKVM_COLD()
long Machine::run_with_breakpoints(std::array<uint64_t, 4> bp)
{
	struct kvm_guest_debug dbg {};

	dbg.control = KVM_GUESTDBG_ENABLE | KVM_GUESTDBG_USE_HW_BP;
	for (size_t i = 0; i < bp.size(); i++) {
		dbg.arch.debugreg[i] = bp[i];
		if (bp[i] != 0x0)
			dbg.arch.debugreg[7] |= 0x3 << (2 * i);
	}
	//printf("Continue with BPs at 0x%lX, 0x%lX, 0x%lX and 0x%lX\n",
	//	bp[0], bp[1], bp[2], bp[3]);

	if (ioctl(vcpu.fd, KVM_SET_GUEST_DEBUG, &dbg) < 0) {
		machine_exception("KVM_RUN failed");
	}

	return vcpu.run_once();
}

TINYKVM_COLD()
void Machine::vCPU::print_registers()
{
	struct kvm_sregs sregs;
	this->get_special_registers(sregs);
	const auto& printer = machine->m_printer;

	char buffer[1024];
	PRINTER(printer, buffer,
		"CR0: 0x%llX  CR3: 0x%llX\n", sregs.cr0, sregs.cr3);
	PRINTER(printer, buffer,
		"CR2: 0x%llX  CR4: 0x%llX\n", sregs.cr2, sregs.cr4);

	auto regs = registers();
	PRINTER(printer, buffer,
		"RAX: 0x%llX  RBX: 0x%llX  RCX: 0x%llX\n", regs.rax, regs.rbx, regs.rcx);
	PRINTER(printer, buffer,
		"RDX: 0x%llX  RSI: 0x%llX  RDI: 0x%llX\n", regs.rdx, regs.rsi, regs.rdi);
	PRINTER(printer, buffer,
		"RIP: 0x%llX  RBP: 0x%llX  RSP: 0x%llX\n", regs.rip, regs.rbp, regs.rsp);

	PRINTER(printer, buffer,
		"SS: 0x%X  CS: 0x%X  DS: 0x%X  FS: 0x%X  GS: 0x%X\n",
		sregs.ss.selector, sregs.cs.selector, sregs.ds.selector, sregs.fs.selector, sregs.gs.selector);

#if 0
	PRINTER(printer, buffer,
		"CR0 PE=%llu MP=%llu EM=%llu\n",
		sregs.cr0 & 1, (sregs.cr0 >> 1) & 1, (sregs.cr0 >> 2) & 1);
	PRINTER(printer, buffer,
		"CR4 OSFXSR=%llu OSXMMEXCPT=%llu OSXSAVE=%llu\n",
		(sregs.cr4 >> 9) & 1, (sregs.cr4 >> 10) & 1, (sregs.cr4 >> 18) & 1);
#endif
#if 0
	printf("IDT: 0x%llX (Size=%x)\n", sregs.idt.base, sregs.idt.limit);
	print_exception_handlers(memory.at(sregs.idt.base));
#endif
#if 0
	print_gdt_entries(memory.at(sregs.gdt.base), 7);
#endif
}

TINYKVM_COLD()
void Machine::vCPU::handle_exception(uint8_t intr)
{
	auto regs = registers();
	char buffer[1024];
	// Page fault
	const auto& printer = machine->m_printer;
	if (intr == 14) {
		struct kvm_sregs sregs;
		get_special_registers(sregs);
		PRINTER(printer, buffer,
			"*** %s on address 0x%llX\n",
			amd64_exception_name(intr), sregs.cr2);
		uint64_t code;
		machine->unsafe_copy_from_guest(&code, regs.rsp+8,  8);
		PRINTER(printer, buffer,
			"Error code: 0x%lX (%s)\n", code,
			(code & 0x02) ? "memory write" : "memory read");
		if (code & 0x01) {
			PRINTER(printer, buffer,
				"* Protection violation\n");
		} else {
			PRINTER(printer, buffer,
				"* Page not present\n");
		}
		if (code & 0x02) {
			PRINTER(printer, buffer,
				"* Invalid write on page\n");
		}
		if (code & 0x04) {
			PRINTER(printer, buffer,
				"* CPL=3 Page fault\n");
		}
		if (code & 0x08) {
			PRINTER(printer, buffer,
				"* Page contains invalid bits\n");
		}
		if (code & 0x10) {
			PRINTER(printer, buffer,
				"* Instruction fetch failed (NX-bit was set)\n");
		}
	} else {
		PRINTER(printer, buffer,
			"*** CPU EXCEPTION: %s (code: %s)\n",
			amd64_exception_name(intr),
			amd64_exception_code(intr) ? "true" : "false");
	}
	this->print_registers();
	//print_pagetables(memory);
	const bool has_code = amd64_exception_code(intr);

	try {
		uint64_t off = (has_code) ? (regs.rsp+8) : (regs.rsp+0);
		if (intr == 14) off += 8;
		uint64_t rip, cs = 0x0, rsp, ss;
		try {
			machine->unsafe_copy_from_guest(&rip, off+0,  8);
			machine->unsafe_copy_from_guest(&cs,  off+8,  8);
			machine->unsafe_copy_from_guest(&rsp, off+24, 8);
			machine->unsafe_copy_from_guest(&ss,  off+32, 8);

			PRINTER(printer, buffer,
				"Failing RIP: 0x%lX\n", rip);
			PRINTER(printer, buffer,
				"Failing CS:  0x%lX\n", cs);
			PRINTER(printer, buffer,
				"Failing RSP: 0x%lX\n", rsp);
			PRINTER(printer, buffer,
				"Failing SS:  0x%lX\n", ss);
		} catch (...) {}

		/* General Protection Fault */
		if (has_code && intr == 13) {
			uint64_t code = 0x0;
			try {
				machine->unsafe_copy_from_guest(&code,  regs.rsp, 8);
			} catch (...) {}
			if (code != 0x0) {
				PRINTER(printer, buffer,
					"Reason: Failing segment 0x%lX\n", code);
			} else if (cs & 0x3) {
				/* Best guess: Privileged instruction */
				PRINTER(printer, buffer,
					"Reason: Executing a privileged instruction\n");
			} else {
				/* Kernel GPFs should be exceedingly rare */
				PRINTER(printer, buffer,
					"Reason: Protection fault in kernel mode\n");
			}
		}
	} catch (...) {}
}

void Machine::migrate_to_this_thread()
{
	timer_delete(vcpu.timer_id);
	vcpu.timer_id = create_vcpu_timer();
}

} // tinykvm

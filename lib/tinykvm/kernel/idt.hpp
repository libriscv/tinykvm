#include <cstdint>

extern void setup_amd64_exception_regs(struct kvm_sregs& sregs, uint64_t addr);
extern void setup_amd64_exceptions(uint64_t addr, void* area, void* code_area);

extern void set_exception_handler(void* area, uint8_t vec, uint64_t handler);
extern void print_exception_handlers(void* area);

extern const char* amd64_exception_name(uint8_t);
extern bool amd64_exception_code(uint8_t);

struct iasm_header {
	uint16_t vm64_syscall;
	uint16_t vm64_gettimeofday;
	uint16_t vm64_exception;
	uint16_t vm64_except_size;
	uint16_t vm64_dso;
	uint16_t vm64_rexit;
};
const iasm_header& interrupt_header();

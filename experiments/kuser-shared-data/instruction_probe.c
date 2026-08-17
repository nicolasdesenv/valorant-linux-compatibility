/* Independent instruction matrix for read-only KUSER_SHARED_DATA accesses. */

typedef signed long NTSTATUS;
typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef unsigned long ULONG;
typedef unsigned long long ULONG64;

#define STATUS_SUCCESS ((NTSTATUS)0)
#define KUSER_SHARED_DATA_KERNEL_BASE 0xfffff78000000000ULL

#define X86_EFLAGS_CF 0x0001ULL
#define X86_EFLAGS_PF 0x0004ULL
#define X86_EFLAGS_AF 0x0010ULL
#define X86_EFLAGS_ZF 0x0040ULL
#define X86_EFLAGS_SF 0x0080ULL
#define X86_EFLAGS_OF 0x0800ULL
#define CMP_FLAGS_MASK (X86_EFLAGS_CF | X86_EFLAGS_PF | X86_EFLAGS_AF | \
                        X86_EFLAGS_ZF | X86_EFLAGS_SF | X86_EFLAGS_OF)
#define TEST_FLAGS_MASK (X86_EFLAGS_CF | X86_EFLAGS_PF | X86_EFLAGS_ZF | \
                         X86_EFLAGS_SF | X86_EFLAGS_OF)

#define PROBE_MOV8    1
#define PROBE_MOV16   2
#define PROBE_MOV32   3
#define PROBE_MOV64   4
#define PROBE_MOVZX8  5
#define PROBE_MOVZX16 6
#define PROBE_OR32    7
#define PROBE_XOR32   8
#define PROBE_CMP32   9
#define PROBE_TEST32  10

__declspec(dllimport) ULONG __cdecl DbgPrint(const char *format, ...);

#if PROBE_KIND == PROBE_CMP32 || PROBE_KIND == PROBE_TEST32
static int even_parity(UCHAR value)
{
    value ^= value >> 4;
    value &= 0x0f;
    return !((0x6996 >> value) & 1);
}
#endif

#if PROBE_KIND == PROBE_CMP32
static ULONG64 expected_cmp32_flags(ULONG left, ULONG right)
{
    ULONG result = left - right;
    ULONG64 flags = 0;

    if (left < right) flags |= X86_EFLAGS_CF;
    if (even_parity((UCHAR)result)) flags |= X86_EFLAGS_PF;
    if ((left ^ right ^ result) & 0x10) flags |= X86_EFLAGS_AF;
    if (!result) flags |= X86_EFLAGS_ZF;
    if (result & 0x80000000UL) flags |= X86_EFLAGS_SF;
    if ((left ^ right) & (left ^ result) & 0x80000000UL) flags |= X86_EFLAGS_OF;
    return flags;
}
#endif

#if PROBE_KIND == PROBE_TEST32
static ULONG64 expected_test32_flags(ULONG result)
{
    ULONG64 flags = 0;

    if (even_parity((UCHAR)result)) flags |= X86_EFLAGS_PF;
    if (!result) flags |= X86_EFLAGS_ZF;
    if (result & 0x80000000UL) flags |= X86_EFLAGS_SF;
    return flags;
}
#endif

#if !defined(PROBE_KIND) || !defined(PROBE_OFFSET) || !defined(PROBE_WIDTH) || !defined(PROBE_NAME)
#error Probe configuration is incomplete
#endif

__declspec(dllexport)
NTSTATUS DriverEntry(void *driver_object, void *registry_path)
{
    const ULONG64 address_value = KUSER_SHARED_DATA_KERNEL_BASE + PROBE_OFFSET;
    const void *address = (const void *)address_value;

    (void)driver_object;
    (void)registry_path;

    DbgPrint("ksd_matrix: DriverEntry instruction=" PROBE_NAME
             " offset=%#lx width=%u address=%#llx\n",
             (ULONG)PROBE_OFFSET, (unsigned int)PROBE_WIDTH, address_value);

#if PROBE_KIND == PROBE_MOV8
    {
        UCHAR value;
        __asm__ volatile("movb (%1), %0" : "=q"(value) : "r"(address) : "memory");
        DbgPrint("ksd_matrix: continued instruction=" PROBE_NAME " value=%#x PASS\n", value);
    }
#elif PROBE_KIND == PROBE_MOV16
    {
        USHORT value;
        __asm__ volatile("movw (%1), %0" : "=r"(value) : "r"(address) : "memory");
        DbgPrint("ksd_matrix: continued instruction=" PROBE_NAME " value=%#x PASS\n", value);
    }
#elif PROBE_KIND == PROBE_MOV32
    {
        ULONG value;
        __asm__ volatile("movl (%1), %0" : "=r"(value) : "r"(address) : "memory");
        DbgPrint("ksd_matrix: continued instruction=" PROBE_NAME " value=%#lx PASS\n", value);
    }
#elif PROBE_KIND == PROBE_MOV64
    {
        ULONG64 value;
        __asm__ volatile("movq (%1), %0" : "=r"(value) : "r"(address) : "memory");
        DbgPrint("ksd_matrix: continued instruction=" PROBE_NAME " value=%#llx PASS\n", value);
    }
#elif PROBE_KIND == PROBE_MOVZX8
    {
        ULONG value;
        __asm__ volatile("movzbl (%1), %0" : "=r"(value) : "r"(address) : "memory");
        DbgPrint("ksd_matrix: continued instruction=" PROBE_NAME " value=%#lx PASS\n", value);
    }
#elif PROBE_KIND == PROBE_MOVZX16
    {
        ULONG value;
        __asm__ volatile("movzwl (%1), %0" : "=r"(value) : "r"(address) : "memory");
        DbgPrint("ksd_matrix: continued instruction=" PROBE_NAME " value=%#lx PASS\n", value);
    }
#elif PROBE_KIND == PROBE_OR32
    {
        ULONG value = 0;
        __asm__ volatile("orl (%1), %0" : "+r"(value) : "r"(address) : "cc", "memory");
        DbgPrint("ksd_matrix: continued instruction=" PROBE_NAME " value=%#lx PASS\n", value);
    }
#elif PROBE_KIND == PROBE_XOR32
    {
        ULONG value = 0;
        __asm__ volatile("xorl (%1), %0" : "+r"(value) : "r"(address) : "cc", "memory");
        DbgPrint("ksd_matrix: continued instruction=" PROBE_NAME " value=%#lx PASS\n", value);
    }
#elif PROBE_KIND == PROBE_CMP32
    {
        ULONG memory_value;
        ULONG zero = 0;
        ULONG64 flags, expected;

        __asm__ volatile("movl (%1), %0" : "=r"(memory_value) : "r"(address) : "memory");
        expected = expected_cmp32_flags(zero, memory_value);
        __asm__ volatile("cmpl (%2), %1; pushfq; popq %0"
                         : "=r"(flags) : "r"(zero), "r"(address) : "cc", "memory");
        DbgPrint("ksd_matrix: continued instruction=" PROBE_NAME
                 " flags=%#llx expected=%#llx mask=%#llx %s\n",
                 flags & CMP_FLAGS_MASK, expected, CMP_FLAGS_MASK,
                 (flags & CMP_FLAGS_MASK) == expected ? "PASS" : "FAIL");
        if ((flags & CMP_FLAGS_MASK) != expected) return (NTSTATUS)0xc0000001;
    }
#elif PROBE_KIND == PROBE_TEST32
    {
        ULONG memory_value;
        ULONG mask = ~0UL;
        ULONG64 flags, expected;

        __asm__ volatile("movl (%1), %0" : "=r"(memory_value) : "r"(address) : "memory");
        expected = expected_test32_flags(memory_value & mask);
        __asm__ volatile("movl $0x7fffffff, %%ecx; addl $1, %%ecx; stc; "
                         "testl %1, (%2); pushfq; popq %0"
                         : "=r"(flags) : "r"(mask), "r"(address) : "cc", "rcx", "memory");
        DbgPrint("ksd_matrix: continued instruction=" PROBE_NAME
                 " flags=%#llx expected=%#llx mask=%#llx %s\n",
                 flags & TEST_FLAGS_MASK, expected, TEST_FLAGS_MASK,
                 (flags & TEST_FLAGS_MASK) == expected ? "PASS" : "FAIL");
        if ((flags & TEST_FLAGS_MASK) != expected) return (NTSTATUS)0xc0000001;
    }
#else
#error Unknown probe kind
#endif

    return STATUS_SUCCESS;
}

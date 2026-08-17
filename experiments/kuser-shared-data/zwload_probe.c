typedef signed long NTSTATUS;
typedef unsigned long ULONG;
typedef unsigned long long ULONG64;

#define STATUS_SUCCESS ((NTSTATUS)0)
#define KUSER_SHARED_DATA_KERNEL_BASE 0xfffff78000000000ULL

__declspec(dllimport) ULONG __cdecl DbgPrint(const char *format, ...);

#ifndef PROBE_KUSER
#define PROBE_KUSER 0
#endif

__declspec(dllexport)
NTSTATUS DriverEntry(void *driver_object, void *registry_path)
{

    (void)driver_object;
    (void)registry_path;
    DbgPrint("zwload_probe: DriverEntry mode=%u\n", (unsigned int)PROBE_KUSER);

#if PROBE_KUSER
    const void *tick_count = (const void *)(KUSER_SHARED_DATA_KERNEL_BASE + 0x320);
    const void *tick_multiplier = (const void *)(KUSER_SHARED_DATA_KERNEL_BASE + 0x004);
    ULONG64 flags;
    ULONG value;
    __asm__ volatile("movl $0, %%ecx; cmpl (%1), %%ecx; pushfq; popq %0"
                     : "=r"(flags) : "r"(tick_count) : "cc", "memory");
    __asm__ volatile("movl $0xffffffff, %%ecx; testl %%ecx, (%1); pushfq; popq %0"
                     : "=r"(flags) : "r"(tick_multiplier) : "cc", "memory");
    __asm__ volatile("movl (%1), %0" : "=r"(value) : "r"(tick_multiplier) : "memory");
    DbgPrint("zwload_probe: KUSER reads completed value=%#lx flags=%#llx\n", value, flags);
#endif

    DbgPrint("zwload_probe: DriverEntry returning STATUS_SUCCESS\n");
    return STATUS_SUCCESS;
}

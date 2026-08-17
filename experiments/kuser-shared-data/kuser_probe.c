/*
 * Minimal read-only KUSER_SHARED_DATA probe for Wine's ntoskrnl driver path.
 *
 * This code is independent from Riot software. It uses only the public AMD64
 * kernel virtual address and documented KUSER_SHARED_DATA field offsets.
 */

typedef signed long NTSTATUS;
typedef unsigned long ULONG;
typedef unsigned long long ULONG64;

#define STATUS_SUCCESS ((NTSTATUS)0)
#define KUSER_SHARED_DATA_KERNEL_BASE 0xfffff78000000000ULL

__declspec(dllimport) ULONG __cdecl DbgPrint(const char *format, ...);

#ifndef PROBE_OFFSET
#define PROBE_OFFSET 0xffffffffUL
#endif

#ifndef PROBE_WIDTH
#define PROBE_WIDTH 0
#endif

__declspec(dllexport)
NTSTATUS DriverEntry(void *driver_object, void *registry_path)
{
    (void)driver_object;
    (void)registry_path;

    DbgPrint("kuser_probe: DriverEntry reached; offset=%#lx width=%u\n",
             (ULONG)PROBE_OFFSET, (unsigned int)PROBE_WIDTH);

#if PROBE_WIDTH == 32
    {
        volatile const ULONG *address =
            (volatile const ULONG *)(KUSER_SHARED_DATA_KERNEL_BASE + PROBE_OFFSET);
        ULONG value = *address;
        DbgPrint("kuser_probe: read32 offset=%#lx value=%#lx PASS\n",
                 (ULONG)PROBE_OFFSET, value);
    }
#elif PROBE_WIDTH == 64
    {
        volatile const ULONG64 *address =
            (volatile const ULONG64 *)(KUSER_SHARED_DATA_KERNEL_BASE + PROBE_OFFSET);
        ULONG64 value = *address;
        DbgPrint("kuser_probe: read64 offset=%#lx value=%#llx PASS\n",
                 (ULONG)PROBE_OFFSET, value);
    }
#elif PROBE_WIDTH == 0
    DbgPrint("kuser_probe: control PASS\n");
#else
#error Unsupported PROBE_WIDTH
#endif

    return STATUS_SUCCESS;
}

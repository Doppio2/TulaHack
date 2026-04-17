#include <stdio.h>
#include <assert.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <sys/mman.h>
#endif

#include "arena_allocator.h"

/* TODOLIST
   1. Simple Linear arena with 1 chunk.
       b. Write a scratch(???) arena or how it called. TempStart, TempEnd, ArenaGetPos and etc functions.
   2. Dynamic grow arena.        
*/

// While it's simple linear allocator we just need a MemorySize.
func arena 
*ArenaAlloc(u64 MemorySize)
{
    // I need a some cross-platfrom function for this.
    // Maybe MEM_RESERVE|MEM_COMMIT should be separated.
    // And maybe i just need a malloc() for more simplicity

    void *Base;

    // TODO: Should be crosspaltform.
    // TODO: in future i need a some os header/library and use it instead code below.
    // Something like OSReserve
#if defined(_WIN32)
    // NOTE(denis): by default on windows VirtualAlloc with MEM_RESERVE|MEM_COMMIT init the memory by zeroes.
    Base = VirtualAlloc(0, MemorySize, 
                        MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
#elif defined(__linux__) 
    Base = mmap(0, MemorySize, 
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS,
                -1, 0);
    if (Base == MAP_FAILED) {
        perror("mmap failed");        // TODO: change perror later.

        return 0;
    }
#endif
    arena *Arena = (arena *)Base;

    Arena->MemorySize = MemorySize;
    Arena->BasePos = 0;
    Arena->Pos = sizeof(arena);      // TODO: need macro for arena header size.

    return Arena;
}

func void 
ArenaFree(arena *Arena)
{
    // TODO(denis): Check arguments later. 
    // Should be croffplatfrom OsRelease();
#if defined(_WIN32)
    VirtualFree(Arena, 0, MEM_RELEASE);
#elif defined(__linux__) 
    if (munmap(Arena, Arena->MemorySize) != 0) {
        perror("munmap failed");
    }
#endif
}

func void 
*ArenaPush(arena *Arena, u64 Size, u64 Align)
{
    u64 PosOldAligned = AlignPow2(Arena->Pos, Align);
    u64 PosNew = PosOldAligned + Size;

    void *Result = 0;
    if(Arena->MemorySize >= PosNew)
    {
        Result = (u8 *)Arena + PosOldAligned;
        Arena->Pos = PosNew;
    }
    else
    {
        // TODO(denis): Log here.
        assert("Allocation fail!" && false);
    }

    return Result;
}

func void 
ArenaPopTo(arena *Arena, u64 Pos)
{
    Pos = Max(sizeof(arena), Pos);
    u64 NewPos = Pos - Arena->BasePos;    // TODO(denis): i do not need this line.
    assert(NewPos <= Arena->Pos);
    Arena->Pos = NewPos;
}

func void
ArenaPop(arena *Arena, u64 MemorySize)
{
    u64 PosOld = ArenaGetPos(Arena);
    u64 PosNew = PosOld;
    if(MemorySize < PosOld)
    {
        PosNew = PosOld - MemorySize;
        ArenaPopTo(Arena, PosNew);
    }
    else
    {
        // TODO: Log here.
        assert("Could not pop the memory!" && false);
    }
}

func u64
ArenaGetPos(arena *Arena)
{
    u64 Result = Arena->BasePos + Arena->Pos;

    return Result;
}

func void 
ArenaClear(arena *Arena)
{
    ArenaPopTo(Arena, 0);
}

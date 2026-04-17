#if !defined _ARENA_ALLOCATOR_H
#define _ARENA_ALLOCATOR_H

#include "stdbool.h"
#include <string.h>

/*
  TODO: 
    1. The general functions and macros move to different file.
    2. Remove old comments and TODO's 
*/

/* 
This is the approach of the arena allocator: take the absurdly simple linear allocator, which offers lightning fast allocation and deallocation, eliminating per-allocation freeing requirements, first being proved out by the stack, and make that a formal allocator concept—the “arena”. 
Usage code can make as many arenas as necessary, and choose them at will for specific allocations.

An arena allocator’s fundamental API, then, may look like this:
*/

/*    ----Macros----    */
// static naming.
#define func static
#define global_var static

// General based macros.
#define Max(A, B) ((A)>(B)?(A):(B))

// Measures for memory.
#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)

// Align helper macros.
// TODO(denis): maybe i should add assert thing to this.
#if defined(_WIN32)
#define AlignOf(Type)       (__alignof(Type))
#elif defined(__clang__) || defined(__GNUC__)
#define AlignOf(Type)       (__alignof__(Type))
#endif
#define AlignPow2(x, b)     (((x) + (b) - 1)&(~((b) - 1)))
#define AlignDownPow2(x, b) ((x)&(~((b) - 1)))

// Macros for memory memory operation.
#define MemoryZero(Start, Count)   (memset((Start), 0, (Count)))
// NOTE(denis): Note sure, that i need this
#define ZeroStruct(Struct)         MemoryZero((Struct),sizeof(*(Struct)))
#define ZeroArray(Array, Count)    MemoryZero((Array),Count)

// Macro helpers for pushing onto arena.
/* NOTE(denis): The PushArena macro helpers are structured by 2 categories.
   1. By Aligned (auto or manual).
   2. By initialization.

   By default all zero init.
   
   NOTE(denis): 
   by default on windows VirtualAlloc with MEM_RESERVE|MEM_COMMIT fill the memory by zeroes.
   And probably same on linux with mmap.
   So NoZero's macros are useless.
   Buy if i want separetly call the MEM_COMMIT and MEM_RESERVE i need this.
*/
#define ArenaPushZero(Arena, Size, Align) MemoryZero(ArenaPush((Arena), (Size), (Align)), Size)

#define PushArrayNoZeroAligned(Arena, Type, Count, Align) (Type*)ArenaPush((Arena), sizeof(Type)*(Count), (Align))
#define PushArrayAligned(Arena, Type, Count, Align) (Type*)ArenaPushZero(Arena, sizeof(Type)*(Count), Align)
#define PushArrayNoZero(Arena, Type, Count) PushArrayNoZeroAligned(Arena, Type, Count, Max(DefaultAlign, AlignOf(Type)))
#define PushArray(Arena, Type, Count) PushArrayAligned(Arena, Type, Count, Max(DefaultAlign, AlignOf(Type)))

// NOTE(denis): probably i can just use PushArray thing with count = 1. So i can remove line down.
#define PushStructNoZeroAligned(Arena, Type, Align) (Type*)ArenaPush((Arena), sizeof(Type), (Align))
#define PushStructAligned(Arena, Type, Align) (Type*)ArenaPushZero(Arena, sizeof(Type), Align)
#define PushStructNoZero(Arena, Type) PushStructAlignedNoZero(Arena, Type, Max(DefaultAlign, AlignOf(Type)))
#define PushStruct(Arena, Type) PushStructAligned(Arena, Type, Max(DefaultAlign, AlignOf(Type)))

/*    ----Types----    */
// TODO: use stdint.
typedef char               s8;
typedef short              s16;
typedef int                s32;
typedef long long          s64;

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef float f32;
typedef double f64;

typedef bool b32;

// NOTE(denis): Foreawhile it's simple linear allocator.
typedef struct arena 
{
    u64 MemorySize;      
    // NOTE(denis): do i need a field for filled memory???
    u64 BasePos;         // NOTE(denis): Not sure, that i need it for a simple linear allocator.
    u64 Pos;
} arena;

/*    ----Globals----    */
// TODO: Figure out what is reserve size and commit size.
// Foreawhile i just use MEM_RESERVE|MEM_COMMIT and i don't need reserve and commit memory.
global_var u64 ArenaDefaultSize = Kilobytes(64); 
global_var u64 LargeArenaDefaultSize = Megabytes(64); 
global_var u64 DefaultAlign = 8;                     // We can take dafult align as 8 or 16. It does not matter. 

/*    ----Functions----    */
// Create or destroy a 'stack' - an "arena".
func arena *ArenaAlloc(u64 MemorySize);
func void ArenaFree(arena *Arena);

// Push some bytes onto the 'stack' - the way to allocate.
func void *ArenaPush(arena *Arena, u64 Size, u64 Align);

// 2 Ways to free:
// Roll the arena state back by the position.
func void ArenaPopTo(arena *Arena, u64 Pos); 
// Pop some bytes off the 'stack'.
func void ArenaPop(arena *Arena, u64 Size);

// Get the # of bytes currently allocated.
func u64 ArenaGetPos(arena *Arena);

// Also some useful popping helpers:
// For temp arenas.
func void ArenaSetPosBack(arena *Arena, u64 Pos);    
// Set arena state to the zero.
func void ArenaClear(arena *Arena);                  


#endif // arena_allocator.h

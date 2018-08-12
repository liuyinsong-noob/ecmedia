// Copyright 2005-2008 Google Inc. All Rights Reserved.
// Author: jrm@google.com (Jim Meehan)

#include <google/protobuf/stubs/common.h>

namespace yuntongxun_google {
namespace protobuf {
namespace internal {

// These four-byte entries compactly encode how many bytes 0..255 to delete
// in making a string replacement, how many bytes to add 0..255, and the offset
// 0..64k-1 of the replacement string in remap_string.
struct RemapEntry {
  uint8 delete_bytes;
  uint8 add_bytes;
  uint16 bytes_offset;
};

// Exit type codes for state tables. All but the first get stuffed into
// signed one-byte entries. The first is only generated by executable code.
// To distinguish from next-state entries, these must be contiguous and
// all <= kExitNone
typedef enum {
  kExitDstSpaceFull = 239,
  kExitIllegalStructure,  // 240
  kExitOK,                // 241
  kExitReject,            // ...
  kExitReplace1,
  kExitReplace2,
  kExitReplace3,
  kExitReplace21,
  kExitReplace31,
  kExitReplace32,
  kExitReplaceOffset1,
  kExitReplaceOffset2,
  kExitReplace1S0,
  kExitSpecial,
  kExitDoAgain,
  kExitRejectAlt,
  kExitNone               // 255
} ExitReason;


// This struct represents one entire state table. The three initialized byte
// areas are state_table, remap_base, and remap_string. state0 and state0_size
// give the byte offset and length within state_table of the initial state --
// table lookups are expected to start and end in this state, but for
// truncated UTF-8 strings, may end in a different state. These allow a quick
// test for that condition. entry_shift is 8 for tables subscripted by a full
// byte value and 6 for space-optimized tables subscripted by only six
// significant bits in UTF-8 continuation bytes.
typedef struct {
  const uint32 state0;
  const uint32 state0_size;
  const uint32 total_size;
  const int max_expand;
  const int entry_shift;
  const int bytes_per_entry;
  const uint32 losub;
  const uint32 hiadd;
  const uint8* state_table;
  const RemapEntry* remap_base;
  const uint8* remap_string;
  const uint8* fast_state;
} UTF8StateMachineObj;

typedef UTF8StateMachineObj UTF8ScanObj;

#define X__ (kExitIllegalStructure)
#define RJ_ (kExitReject)
#define S1_ (kExitReplace1)
#define S2_ (kExitReplace2)
#define S3_ (kExitReplace3)
#define S21 (kExitReplace21)
#define S31 (kExitReplace31)
#define S32 (kExitReplace32)
#define T1_ (kExitReplaceOffset1)
#define T2_ (kExitReplaceOffset2)
#define S11 (kExitReplace1S0)
#define SP_ (kExitSpecial)
#define D__ (kExitDoAgain)
#define RJA (kExitRejectAlt)

//  Entire table has 9 state blocks of 256 entries each
static const unsigned int utf8acceptnonsurrogates_STATE0 = 0;     // state[0]
static const unsigned int utf8acceptnonsurrogates_STATE0_SIZE = 256;  // =[1]
static const unsigned int utf8acceptnonsurrogates_TOTAL_SIZE = 2304;
static const unsigned int utf8acceptnonsurrogates_MAX_EXPAND_X4 = 0;
static const unsigned int utf8acceptnonsurrogates_SHIFT = 8;
static const unsigned int utf8acceptnonsurrogates_BYTES = 1;
static const unsigned int utf8acceptnonsurrogates_LOSUB = 0x20202020;
static const unsigned int utf8acceptnonsurrogates_HIADD = 0x00000000;

static const uint8 utf8acceptnonsurrogates[] = {
// state[0] 0x000000 Byte 1
  0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,

  0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,

X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,

X__, X__,   1,   1,   1,   1,   1,   1,    1,   1,   1,   1,   1,   1,   1,   1,
  1,   1,   1,   1,   1,   1,   1,   1,    1,   1,   1,   1,   1,   1,   1,   1,
  2,   3,   3,   3,   3,   3,   3,   3,    3,   3,   3,   3,   3,   7,   3,   3,
  4,   5,   5,   5,   6, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,

// state[1] 0x000080 Byte 2 of 2
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,

X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,

  0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,

X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,

// state[2] 0x000000 Byte 2 of 3
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,

X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,

X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
  1,   1,   1,   1,   1,   1,   1,   1,    1,   1,   1,   1,   1,   1,   1,   1,
  1,   1,   1,   1,   1,   1,   1,   1,    1,   1,   1,   1,   1,   1,   1,   1,

X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,

// state[3] 0x001000 Byte 2 of 3
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,

X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,

  1,   1,   1,   1,   1,   1,   1,   1,    1,   1,   1,   1,   1,   1,   1,   1,
  1,   1,   1,   1,   1,   1,   1,   1,    1,   1,   1,   1,   1,   1,   1,   1,
  1,   1,   1,   1,   1,   1,   1,   1,    1,   1,   1,   1,   1,   1,   1,   1,
  1,   1,   1,   1,   1,   1,   1,   1,    1,   1,   1,   1,   1,   1,   1,   1,

X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,

// state[4] 0x000000 Byte 2 of 4
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,

X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,

X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
  3,   3,   3,   3,   3,   3,   3,   3,    3,   3,   3,   3,   3,   3,   3,   3,
  3,   3,   3,   3,   3,   3,   3,   3,    3,   3,   3,   3,   3,   3,   3,   3,
  3,   3,   3,   3,   3,   3,   3,   3,    3,   3,   3,   3,   3,   3,   3,   3,

X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,

// state[5] 0x040000 Byte 2 of 4
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,

X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,

  3,   3,   3,   3,   3,   3,   3,   3,    3,   3,   3,   3,   3,   3,   3,   3,
  3,   3,   3,   3,   3,   3,   3,   3,    3,   3,   3,   3,   3,   3,   3,   3,
  3,   3,   3,   3,   3,   3,   3,   3,    3,   3,   3,   3,   3,   3,   3,   3,
  3,   3,   3,   3,   3,   3,   3,   3,    3,   3,   3,   3,   3,   3,   3,   3,

X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,

// state[6] 0x100000 Byte 2 of 4
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,

X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,

  3,   3,   3,   3,   3,   3,   3,   3,    3,   3,   3,   3,   3,   3,   3,   3,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,

X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,

// state[7] 0x00d000 Byte 2 of 3
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,

X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,

  1,   1,   1,   1,   1,   1,   1,   1,    1,   1,   1,   1,   1,   1,   1,   1,
  1,   1,   1,   1,   1,   1,   1,   1,    1,   1,   1,   1,   1,   1,   1,   1,
  8,   8,   8,   8,   8,   8,   8,   8,    8,   8,   8,   8,   8,   8,   8,   8,
  8,   8,   8,   8,   8,   8,   8,   8,    8,   8,   8,   8,   8,   8,   8,   8,

X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,

// state[8] 0x00d800 Byte 3 of 3
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,

X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,

RJ_, RJ_, RJ_, RJ_, RJ_, RJ_, RJ_, RJ_,  RJ_, RJ_, RJ_, RJ_, RJ_, RJ_, RJ_, RJ_,
RJ_, RJ_, RJ_, RJ_, RJ_, RJ_, RJ_, RJ_,  RJ_, RJ_, RJ_, RJ_, RJ_, RJ_, RJ_, RJ_,
RJ_, RJ_, RJ_, RJ_, RJ_, RJ_, RJ_, RJ_,  RJ_, RJ_, RJ_, RJ_, RJ_, RJ_, RJ_, RJ_,
RJ_, RJ_, RJ_, RJ_, RJ_, RJ_, RJ_, RJ_,  RJ_, RJ_, RJ_, RJ_, RJ_, RJ_, RJ_, RJ_,

X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
X__, X__, X__, X__, X__, X__, X__, X__,  X__, X__, X__, X__, X__, X__, X__, X__,
};

// Remap base[0] = (del, add, string_offset)
static const RemapEntry utf8acceptnonsurrogates_remap_base[] = {
{0, 0, 0} };

// Remap string[0]
static const unsigned char utf8acceptnonsurrogates_remap_string[] = {
0 };

static const unsigned char utf8acceptnonsurrogates_fast[256] = {
0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,

0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,

1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,

1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
};

static const UTF8ScanObj utf8acceptnonsurrogates_obj = {
  utf8acceptnonsurrogates_STATE0,
  utf8acceptnonsurrogates_STATE0_SIZE,
  utf8acceptnonsurrogates_TOTAL_SIZE,
  utf8acceptnonsurrogates_MAX_EXPAND_X4,
  utf8acceptnonsurrogates_SHIFT,
  utf8acceptnonsurrogates_BYTES,
  utf8acceptnonsurrogates_LOSUB,
  utf8acceptnonsurrogates_HIADD,
  utf8acceptnonsurrogates,
  utf8acceptnonsurrogates_remap_base,
  utf8acceptnonsurrogates_remap_string,
  utf8acceptnonsurrogates_fast
};


#undef X__
#undef RJ_
#undef S1_
#undef S2_
#undef S3_
#undef S21
#undef S31
#undef S32
#undef T1_
#undef T2_
#undef S11
#undef SP_
#undef D__
#undef RJA

// Return true if current Tbl pointer is within state0 range
// Note that unsigned compare checks both ends of range simultaneously
static inline bool InStateZero(const UTF8ScanObj* st, const uint8* Tbl) {
  const uint8* Tbl0 = &st->state_table[st->state0];
  return (static_cast<uint32>(Tbl - Tbl0) < st->state0_size);
}

// Scan a UTF-8 string based on state table.
// Always scan complete UTF-8 characters
// Set number of bytes scanned. Return reason for exiting
int UTF8GenericScan(const UTF8ScanObj* st,
                    const char * str,
                    int str_length,
                    int* bytes_consumed) {
  *bytes_consumed = 0;
  if (str_length == 0) return kExitOK;

  int eshift = st->entry_shift;
  const uint8* isrc = reinterpret_cast<const uint8*>(str);
  const uint8* src = isrc;
  const uint8* srclimit = isrc + str_length;
  const uint8* srclimit8 = srclimit - 7;
  const uint8* Tbl_0 = &st->state_table[st->state0];

 DoAgain:
  // Do state-table scan
  int e = 0;
  uint8 c;
  const uint8* Tbl2 = &st->fast_state[0];
  const uint32 losub = st->losub;
  const uint32 hiadd = st->hiadd;
  // Check initial few bytes one at a time until 8-byte aligned
  //----------------------------
  while ((((uintptr_t)src & 0x07) != 0) &&
         (src < srclimit) &&
         Tbl2[src[0]] == 0) {
    src++;
  }
  if (((uintptr_t)src & 0x07) == 0) {
    // Do fast for groups of 8 identity bytes.
    // This covers a lot of 7-bit ASCII ~8x faster then the 1-byte loop,
    // including slowing slightly on cr/lf/ht
    //----------------------------
    while (src < srclimit8) {
      uint32 s0123 = (reinterpret_cast<const uint32 *>(src))[0];
      uint32 s4567 = (reinterpret_cast<const uint32 *>(src))[1];
      src += 8;
      // This is a fast range check for all bytes in [lowsub..0x80-hiadd)
      uint32 temp = (s0123 - losub) | (s0123 + hiadd) |
                    (s4567 - losub) | (s4567 + hiadd);
      if ((temp & 0x80808080) != 0) {
        // We typically end up here on cr/lf/ht; src was incremented
        int e0123 = (Tbl2[src[-8]] | Tbl2[src[-7]]) |
                    (Tbl2[src[-6]] | Tbl2[src[-5]]);
        if (e0123 != 0) {
          src -= 8;
          break;
        }    // Exit on Non-interchange
        e0123 = (Tbl2[src[-4]] | Tbl2[src[-3]]) |
                (Tbl2[src[-2]] | Tbl2[src[-1]]);
        if (e0123 != 0) {
          src -= 4;
          break;
        }    // Exit on Non-interchange
        // Else OK, go around again
      }
    }
  }
  //----------------------------

  // Byte-at-a-time scan
  //----------------------------
  const uint8* Tbl = Tbl_0;
  while (src < srclimit) {
    c = *src;
    e = Tbl[c];
    src++;
    if (e >= kExitIllegalStructure) {break;}
    Tbl = &Tbl_0[e << eshift];
  }
  //----------------------------


  // Exit posibilities:
  //  Some exit code, !state0, back up over last char
  //  Some exit code, state0, back up one byte exactly
  //  source consumed, !state0, back up over partial char
  //  source consumed, state0, exit OK
  // For illegal byte in state0, avoid backup up over PREVIOUS char
  // For truncated last char, back up to beginning of it

  if (e >= kExitIllegalStructure) {
    // Back up over exactly one byte of rejected/illegal UTF-8 character
    src--;
    // Back up more if needed
    if (!InStateZero(st, Tbl)) {
      do {
        src--;
      } while ((src > isrc) && ((src[0] & 0xc0) == 0x80));
    }
  } else if (!InStateZero(st, Tbl)) {
    // Back up over truncated UTF-8 character
    e = kExitIllegalStructure;
    do {
      src--;
    } while ((src > isrc) && ((src[0] & 0xc0) == 0x80));
  } else {
    // Normal termination, source fully consumed
    e = kExitOK;
  }

  if (e == kExitDoAgain) {
    // Loop back up to the fast scan
    goto DoAgain;
  }

  *bytes_consumed = src - isrc;
  return e;
}

int UTF8GenericScanFastAscii(const UTF8ScanObj* st,
                    const char * str,
                    int str_length,
                    int* bytes_consumed) {
  *bytes_consumed = 0;
  if (str_length == 0) return kExitOK;

  const uint8* isrc =  reinterpret_cast<const uint8*>(str);
  const uint8* src = isrc;
  const uint8* srclimit = isrc + str_length;
  const uint8* srclimit8 = srclimit - 7;
  int n;
  int rest_consumed;
  int exit_reason;
  do {
    // Check initial few bytes one at a time until 8-byte aligned
    while ((((uintptr_t)src & 0x07) != 0) &&
           (src < srclimit) && (src[0] < 0x80)) {
      src++;
    }
    if (((uintptr_t)src & 0x07) == 0) {
      while ((src < srclimit8) &&
             (((reinterpret_cast<const uint32*>(src)[0] |
                reinterpret_cast<const uint32*>(src)[1]) & 0x80808080) == 0)) {
        src += 8;
      }
    }
    while ((src < srclimit) && (src[0] < 0x80)) {
      src++;
    }
    // Run state table on the rest
    n = src - isrc;
    exit_reason = UTF8GenericScan(st, str + n, str_length - n, &rest_consumed);
    src += rest_consumed;
  } while ( exit_reason == kExitDoAgain );

  *bytes_consumed = src - isrc;
  return exit_reason;
}

// Hack:  On some compilers the static tables are initialized at startup.
//   We can't use them until they are initialized.  However, some Protocol
//   Buffer parsing happens at static init time and may try to validate
//   UTF-8 strings.  Since UTF-8 validation is only used for debugging
//   anyway, we simply always return success if initialization hasn't
//   occurred yet.
namespace {

bool module_initialized_ = false;

struct InitDetector {
  InitDetector() {
    module_initialized_ = true;
  }
};
InitDetector init_detector;

}  // namespace

bool IsStructurallyValidUTF8(const char* buf, int len) {
  if (!module_initialized_) return true;
  
  int bytes_consumed = 0;
  UTF8GenericScanFastAscii(&utf8acceptnonsurrogates_obj,
                           buf, len, &bytes_consumed);
  return (bytes_consumed == len);
}

}  // namespace internal
}  // namespace protobuf
}  // namespace yuntongxun_google

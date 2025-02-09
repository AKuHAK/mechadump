/*
 * ps3mca-tool - PlayStation 3 Memory Card Adaptor Software
 * Copyright (C) 2011 - jimmikaelkael <jimmikaelkael@wanadoo.fr>
 * Copyright (C) 2011 - "someone who wants to stay anonymous"
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * MechaCon cipher. This is a standard-compliant implementation of DES as
 * described in FIPS 46-3. The internal format of the key schedule has been
 * changed to allow running the cipher more efficiently on large numbers of
 * blocks.
 */

#include "cipher.h"
#include "util.h"

/* PC-1 */
static unsigned char PC1_table[56] = {
    57,
    49,
    41,
    33,
    25,
    17,
    9,
    1,
    58,
    50,
    42,
    34,
    26,
    18,
    10,
    2,
    59,
    51,
    43,
    35,
    27,
    19,
    11,
    3,
    60,
    52,
    44,
    36,
    63,
    55,
    47,
    39,
    31,
    23,
    15,
    7,
    62,
    54,
    46,
    38,
    30,
    22,
    14,
    6,
    61,
    53,
    45,
    37,
    29,
    21,
    13,
    5,
    28,
    20,
    12,
    4,
};

/* Left-Shift table */
static unsigned char LS_table[16] = {
    1,
    1,
    2,
    2,
    2,
    2,
    2,
    2,
    1,
    2,
    2,
    2,
    2,
    2,
    2,
    1,
};

/*
 * PC-2x. This one resembles PC-2 from FIPS 46-3 very closely; the only
 * difference is that (if line counting starts at 1) odd-numbered lines
 * appear before even-numbered lines, but relative order is kept.
 */
static unsigned char PC2x_table[48] = {
    14,
    17,
    11,
    24,
    1,
    5,
    23,
    19,
    12,
    4,
    26,
    8,
    41,
    52,
    31,
    37,
    47,
    55,
    44,
    49,
    39,
    56,
    34,
    53,
    3,
    28,
    15,
    6,
    21,
    10,
    16,
    7,
    27,
    20,
    13,
    2,
    30,
    40,
    51,
    45,
    33,
    48,
    46,
    42,
    50,
    36,
    29,
    32,
};

/*
 * cipherKeyScheduleInner: calculate key schedule of MechaCon cipher.
 */
static void cipherKeyScheduleInner(uint64_t RoundKeys[16], uint64_t Key)
{
    uint64_t Input = 0;

    int i;
    for (i = 0; i < 56; i++)
    {
        if (Key & ((uint64_t)1 << (64 - PC1_table[i])))
        {
            Input |= (uint64_t)1 << (55 - i);
        }
    }

    uint32_t C = (uint32_t)(Input >> 28) & 0x0FFFFFFF;
    uint32_t D = (uint32_t)Input & 0x0FFFFFFF;

    for (i = 0; i < 16; i++)
    {
        /* Left shift. Up to this point, this is a standard DES key schedule */
        if (LS_table[i] == 1)
        {
            C = (C << 1) | (C >> 27);
            D = (D << 1) | (D >> 27);
        }
        else
        {
            C = (C << 2) | (C >> 26);
            D = (D << 2) | (D >> 26);
        }

        uint64_t CD = ((uint64_t)(C & 0x0FFFFFFF) << 28) | (D & 0x0FFFFFFF);
        uint64_t Ki = 0;
        int j;
        for (j = 0; j < 48; j++)
        {
            if (CD & ((uint64_t)1 << (56 - PC2x_table[j])))
            {
                Ki |= (uint64_t)1 << (63 - 2 - ((j / 6) * 8 + (j % 6)));
            }
        }

        RoundKeys[i] = Ki;
    }
}

/* IP. */
static unsigned char IP_table[64] = {
    58,
    50,
    42,
    34,
    26,
    18,
    10,
    2,
    60,
    52,
    44,
    36,
    28,
    20,
    12,
    4,
    62,
    54,
    46,
    38,
    30,
    22,
    14,
    6,
    64,
    56,
    48,
    40,
    32,
    24,
    16,
    8,
    57,
    49,
    41,
    33,
    25,
    17,
    9,
    1,
    59,
    51,
    43,
    35,
    27,
    19,
    11,
    3,
    61,
    53,
    45,
    37,
    29,
    21,
    13,
    5,
    63,
    55,
    47,
    39,
    31,
    23,
    15,
    7,
};

/*
 * cipherIP: apply MechaCon cipher IP to specified value.
 */
static uint64_t cipherIP(uint64_t Value)
{
    uint64_t Output = 0;

    int i;
    for (i = 0; i < 64; i++)
    {
        if (Value & ((uint64_t)1 << (64 - IP_table[i])))
        {
            Output |= (uint64_t)1 << (63 - i);
        }
    }

    return Output;
}


/* inverse IP. */
static unsigned char IPinv_table[64] = {
    40,
    8,
    48,
    16,
    56,
    24,
    64,
    32,
    39,
    7,
    47,
    15,
    55,
    23,
    63,
    31,
    38,
    6,
    46,
    14,
    54,
    22,
    62,
    30,
    37,
    5,
    45,
    13,
    53,
    21,
    61,
    29,
    36,
    4,
    44,
    12,
    52,
    20,
    60,
    28,
    35,
    3,
    43,
    11,
    51,
    19,
    59,
    27,
    34,
    2,
    42,
    10,
    50,
    18,
    58,
    26,
    33,
    1,
    41,
    9,
    49,
    17,
    57,
    25,
};

/*
 * cipherIPInverse: apply MechaCon cipher inverse IP to specified value.
 */
static uint64_t cipherIPInverse(uint64_t Value)
{
    uint64_t Output = 0;

    int i;
    for (i = 0; i < 64; i++)
    {
        if (Value & ((uint64_t)1 << (64 - IPinv_table[i])))
        {
            Output |= (uint64_t)1 << (63 - i);
        }
    }

    return Output;
}


/*
 * DES S+P tables for the fast software implementation. These tables are generated by code in gen.c
 * from the official tables described in FIPS 46-3.
 *
 * All values are reordered to allow simple 6-bit indexing of each table, instead of the bit
 * manipulations necessary when using the tables from FIPS 46-3 directly. The values in these
 * tables represent the corresponding S box output under the P permutation. To get the output of
 * the DES round function, the outputs of all S+P boxes need to be ORed.
 */

/* S1 */
static uint32_t SP_box_1[4 * 16] = {
    0x00808200,
    0x00000000,
    0x00008000,
    0x00808202,
    0x00808002,
    0x00008202,
    0x00000002,
    0x00008000,
    0x00000200,
    0x00808200,
    0x00808202,
    0x00000200,
    0x00800202,
    0x00808002,
    0x00800000,
    0x00000002,
    0x00000202,
    0x00800200,
    0x00800200,
    0x00008200,
    0x00008200,
    0x00808000,
    0x00808000,
    0x00800202,
    0x00008002,
    0x00800002,
    0x00800002,
    0x00008002,
    0x00000000,
    0x00000202,
    0x00008202,
    0x00800000,
    0x00008000,
    0x00808202,
    0x00000002,
    0x00808000,
    0x00808200,
    0x00800000,
    0x00800000,
    0x00000200,
    0x00808002,
    0x00008000,
    0x00008200,
    0x00800002,
    0x00000200,
    0x00000002,
    0x00800202,
    0x00008202,
    0x00808202,
    0x00008002,
    0x00808000,
    0x00800202,
    0x00800002,
    0x00000202,
    0x00008202,
    0x00808200,
    0x00000202,
    0x00800200,
    0x00800200,
    0x00000000,
    0x00008002,
    0x00008200,
    0x00000000,
    0x00808002,
};

/* S2 */
static uint32_t SP_box_2[4 * 16] = {
    0x40084010,
    0x40004000,
    0x00004000,
    0x00084010,
    0x00080000,
    0x00000010,
    0x40080010,
    0x40004010,
    0x40000010,
    0x40084010,
    0x40084000,
    0x40000000,
    0x40004000,
    0x00080000,
    0x00000010,
    0x40080010,
    0x00084000,
    0x00080010,
    0x40004010,
    0x00000000,
    0x40000000,
    0x00004000,
    0x00084010,
    0x40080000,
    0x00080010,
    0x40000010,
    0x00000000,
    0x00084000,
    0x00004010,
    0x40084000,
    0x40080000,
    0x00004010,
    0x00000000,
    0x00084010,
    0x40080010,
    0x00080000,
    0x40004010,
    0x40080000,
    0x40084000,
    0x00004000,
    0x40080000,
    0x40004000,
    0x00000010,
    0x40084010,
    0x00084010,
    0x00000010,
    0x00004000,
    0x40000000,
    0x00004010,
    0x40084000,
    0x00080000,
    0x40000010,
    0x00080010,
    0x40004010,
    0x40000010,
    0x00080010,
    0x00084000,
    0x00000000,
    0x40004000,
    0x00004010,
    0x40000000,
    0x40080010,
    0x40084010,
    0x00084000,
};

/* S3 */
static uint32_t SP_box_3[4 * 16] = {
    0x00000104,
    0x04010100,
    0x00000000,
    0x04010004,
    0x04000100,
    0x00000000,
    0x00010104,
    0x04000100,
    0x00010004,
    0x04000004,
    0x04000004,
    0x00010000,
    0x04010104,
    0x00010004,
    0x04010000,
    0x00000104,
    0x04000000,
    0x00000004,
    0x04010100,
    0x00000100,
    0x00010100,
    0x04010000,
    0x04010004,
    0x00010104,
    0x04000104,
    0x00010100,
    0x00010000,
    0x04000104,
    0x00000004,
    0x04010104,
    0x00000100,
    0x04000000,
    0x04010100,
    0x04000000,
    0x00010004,
    0x00000104,
    0x00010000,
    0x04010100,
    0x04000100,
    0x00000000,
    0x00000100,
    0x00010004,
    0x04010104,
    0x04000100,
    0x04000004,
    0x00000100,
    0x00000000,
    0x04010004,
    0x04000104,
    0x00010000,
    0x04000000,
    0x04010104,
    0x00000004,
    0x00010104,
    0x00010100,
    0x04000004,
    0x04010000,
    0x04000104,
    0x00000104,
    0x04010000,
    0x00010104,
    0x00000004,
    0x04010004,
    0x00010100,
};

/* S4 */
static uint32_t SP_box_4[4 * 16] = {
    0x80401000,
    0x80001040,
    0x80001040,
    0x00000040,
    0x00401040,
    0x80400040,
    0x80400000,
    0x80001000,
    0x00000000,
    0x00401000,
    0x00401000,
    0x80401040,
    0x80000040,
    0x00000000,
    0x00400040,
    0x80400000,
    0x80000000,
    0x00001000,
    0x00400000,
    0x80401000,
    0x00000040,
    0x00400000,
    0x80001000,
    0x00001040,
    0x80400040,
    0x80000000,
    0x00001040,
    0x00400040,
    0x00001000,
    0x00401040,
    0x80401040,
    0x80000040,
    0x00400040,
    0x80400000,
    0x00401000,
    0x80401040,
    0x80000040,
    0x00000000,
    0x00000000,
    0x00401000,
    0x00001040,
    0x00400040,
    0x80400040,
    0x80000000,
    0x80401000,
    0x80001040,
    0x80001040,
    0x00000040,
    0x80401040,
    0x80000040,
    0x80000000,
    0x00001000,
    0x80400000,
    0x80001000,
    0x00401040,
    0x80400040,
    0x80001000,
    0x00001040,
    0x00400000,
    0x80401000,
    0x00000040,
    0x00400000,
    0x00001000,
    0x00401040,
};

/* S5 */
static uint32_t SP_box_5[4 * 16] = {
    0x00000080,
    0x01040080,
    0x01040000,
    0x21000080,
    0x00040000,
    0x00000080,
    0x20000000,
    0x01040000,
    0x20040080,
    0x00040000,
    0x01000080,
    0x20040080,
    0x21000080,
    0x21040000,
    0x00040080,
    0x20000000,
    0x01000000,
    0x20040000,
    0x20040000,
    0x00000000,
    0x20000080,
    0x21040080,
    0x21040080,
    0x01000080,
    0x21040000,
    0x20000080,
    0x00000000,
    0x21000000,
    0x01040080,
    0x01000000,
    0x21000000,
    0x00040080,
    0x00040000,
    0x21000080,
    0x00000080,
    0x01000000,
    0x20000000,
    0x01040000,
    0x21000080,
    0x20040080,
    0x01000080,
    0x20000000,
    0x21040000,
    0x01040080,
    0x20040080,
    0x00000080,
    0x01000000,
    0x21040000,
    0x21040080,
    0x00040080,
    0x21000000,
    0x21040080,
    0x01040000,
    0x00000000,
    0x20040000,
    0x21000000,
    0x00040080,
    0x01000080,
    0x20000080,
    0x00040000,
    0x00000000,
    0x20040000,
    0x01040080,
    0x20000080,
};

/* S6 */
static uint32_t SP_box_6[4 * 16] = {
    0x10000008,
    0x10200000,
    0x00002000,
    0x10202008,
    0x10200000,
    0x00000008,
    0x10202008,
    0x00200000,
    0x10002000,
    0x00202008,
    0x00200000,
    0x10000008,
    0x00200008,
    0x10002000,
    0x10000000,
    0x00002008,
    0x00000000,
    0x00200008,
    0x10002008,
    0x00002000,
    0x00202000,
    0x10002008,
    0x00000008,
    0x10200008,
    0x10200008,
    0x00000000,
    0x00202008,
    0x10202000,
    0x00002008,
    0x00202000,
    0x10202000,
    0x10000000,
    0x10002000,
    0x00000008,
    0x10200008,
    0x00202000,
    0x10202008,
    0x00200000,
    0x00002008,
    0x10000008,
    0x00200000,
    0x10002000,
    0x10000000,
    0x00002008,
    0x10000008,
    0x10202008,
    0x00202000,
    0x10200000,
    0x00202008,
    0x10202000,
    0x00000000,
    0x10200008,
    0x00000008,
    0x00002000,
    0x10200000,
    0x00202008,
    0x00002000,
    0x00200008,
    0x10002008,
    0x00000000,
    0x10202000,
    0x10000000,
    0x00200008,
    0x10002008,
};

/* S7 */
static uint32_t SP_box_7[4 * 16] = {
    0x00100000,
    0x02100001,
    0x02000401,
    0x00000000,
    0x00000400,
    0x02000401,
    0x00100401,
    0x02100400,
    0x02100401,
    0x00100000,
    0x00000000,
    0x02000001,
    0x00000001,
    0x02000000,
    0x02100001,
    0x00000401,
    0x02000400,
    0x00100401,
    0x00100001,
    0x02000400,
    0x02000001,
    0x02100000,
    0x02100400,
    0x00100001,
    0x02100000,
    0x00000400,
    0x00000401,
    0x02100401,
    0x00100400,
    0x00000001,
    0x02000000,
    0x00100400,
    0x02000000,
    0x00100400,
    0x00100000,
    0x02000401,
    0x02000401,
    0x02100001,
    0x02100001,
    0x00000001,
    0x00100001,
    0x02000000,
    0x02000400,
    0x00100000,
    0x02100400,
    0x00000401,
    0x00100401,
    0x02100400,
    0x00000401,
    0x02000001,
    0x02100401,
    0x02100000,
    0x00100400,
    0x00000000,
    0x00000001,
    0x02100401,
    0x00000000,
    0x00100401,
    0x02100000,
    0x00000400,
    0x02000001,
    0x02000400,
    0x00000400,
    0x00100001,
};

/* S8 */
static uint32_t SP_box_8[4 * 16] = {
    0x08000820,
    0x00000800,
    0x00020000,
    0x08020820,
    0x08000000,
    0x08000820,
    0x00000020,
    0x08000000,
    0x00020020,
    0x08020000,
    0x08020820,
    0x00020800,
    0x08020800,
    0x00020820,
    0x00000800,
    0x00000020,
    0x08020000,
    0x08000020,
    0x08000800,
    0x00000820,
    0x00020800,
    0x00020020,
    0x08020020,
    0x08020800,
    0x00000820,
    0x00000000,
    0x00000000,
    0x08020020,
    0x08000020,
    0x08000800,
    0x00020820,
    0x00020000,
    0x00020820,
    0x00020000,
    0x08020800,
    0x00000800,
    0x00000020,
    0x08020020,
    0x00000800,
    0x00020820,
    0x08000800,
    0x00000020,
    0x08000020,
    0x08020000,
    0x08020020,
    0x08000000,
    0x00020000,
    0x08000820,
    0x00000000,
    0x08020820,
    0x00020020,
    0x08000020,
    0x08020000,
    0x08000800,
    0x08000820,
    0x00000000,
    0x08020820,
    0x00020800,
    0x00020800,
    0x00000820,
    0x00000820,
    0x00020020,
    0x08000000,
    0x08020800,
};


/*
 * cipherForward: run MechaCon cipher in forward direction.
 */
static void cipherForward(uint64_t Value, uint64_t *Result, const uint64_t RoundKeys[16])
{
    uint64_t Current = cipherIP(Value);
    uint32_t Low     = (uint32_t)Current;
    uint32_t High    = (uint32_t)(Current >> 32);

    int i;
    for (i = 0; i <= 15; i++)
    {
        uint32_t X = (Low << 29) | (Low >> 3);
        uint32_t Y = (Low << 1) | (Low >> 31);

        X ^= (uint32_t)(RoundKeys[i] >> 32);
        Y ^= (uint32_t)RoundKeys[i];

        uint32_t Tab0   = SP_box_1[(X >> 24) & 0x3F];
        uint32_t Tab1   = SP_box_2[(Y >> 24) & 0x3F];
        uint32_t Tab2   = SP_box_3[(X >> 16) & 0x3F];
        uint32_t Tab3   = SP_box_4[(Y >> 16) & 0x3F];
        uint32_t Tab4   = SP_box_5[(X >> 8) & 0x3F];
        uint32_t Tab5   = SP_box_6[(Y >> 8) & 0x3F];
        uint32_t Tab6   = SP_box_7[(X >> 0) & 0x3F];
        uint32_t Tab7   = SP_box_8[(Y >> 0) & 0x3F];

        uint32_t Tab    = Tab0 | Tab1 | Tab2 | Tab3 | Tab4 | Tab5 | Tab6 | Tab7;

        uint32_t NewLow = High ^ Tab;
        High            = Low;
        Low             = NewLow;
    }

    *Result = cipherIPInverse(((uint64_t)Low << 32) | (uint64_t)High);
}

/*
 * _cipherKeySchedule: calculate key schedule of MechaCon cipher.
 */
static void _cipherKeySchedule(const uint8_t Key[8], uint64_t RoundKeys[16])
{
    uint64_t KeyValue = read_be_uint64(Key);
    cipherKeyScheduleInner(RoundKeys, KeyValue);
}

/*
 * _cipherKeyScheduleReverse: calculate key schedule of MechaCon cipher in reverse order.
 */
static void _cipherKeyScheduleReverse(const uint8_t Key[8], uint64_t RoundKeys[16])
{
    uint64_t ForwardRoundKeys[16];
    int i;
    uint64_t KeyValue = read_be_uint64(Key);
    cipherKeyScheduleInner(ForwardRoundKeys, KeyValue);

    for (i = 0; i < 16; i++)
    {
        RoundKeys[i] = ForwardRoundKeys[15 - i];
    }
}

/*
 * cipherKeySchedule: perform key schedule for multiple invocations of the MechaCon cipher.
 * supports up to three keys.
 */
static int cipherKeySchedule(uint64_t *RoundKeys, const uint8_t *Keys, int KeyCount)
{
    if (KeyCount != 1 && KeyCount != 2 && KeyCount != 3)
    {
        return -1;
    }

    _cipherKeySchedule(Keys + 0, RoundKeys + 0);
    if (KeyCount == 1)
    {
        return 1;
    }

    _cipherKeyScheduleReverse(Keys + 8, RoundKeys + 16);
    if (KeyCount == 2)
    {
        return 2;
    }

    _cipherKeySchedule(Keys + 16, RoundKeys + 32);
    return 3;
}

/*
 * cipherKeyScheduleReverse: perform key schedule for multiple invocations of the MechaCon cipher
 * in reverse order. Supports up to three keys.
 */
static int cipherKeyScheduleReverse(uint64_t *RoundKeys, const uint8_t *Keys, int KeyCount)
{
    if (KeyCount != 1 && KeyCount != 2 && KeyCount != 3)
    {
        return -1;
    }

    _cipherKeyScheduleReverse(Keys + 0, RoundKeys + 0);
    if (KeyCount == 1)
    {
        return 1;
    }

    _cipherKeySchedule(Keys + 8, RoundKeys + 16);
    if (KeyCount == 2)
    {
        return 2;
    }

    memcpy(RoundKeys + 32, RoundKeys, sizeof(RoundKeys[0]) * 16);

    _cipherKeyScheduleReverse(Keys + 16, RoundKeys);
    return 3;
}

/*
 * cipherSingleBlock: Invoke the MechaCon cipher multiple times on a single data block.
 * If KeyCount is set to one, a single invocation is performed.
 * Otherwise, the cipher is called three times; if only two keys
 * are provided, the first key is used two times.
 */
static void cipherSingleBlock(uint8_t Result[8], const uint8_t Data[8],
                              const uint64_t *RoundKeys, int KeyCount)
{
    uint64_t Input = read_be_uint64(Data);

    uint64_t Output;
    cipherForward(Input, &Output, RoundKeys);

    if (KeyCount != 1)
    {
        cipherForward(Output, &Output, RoundKeys + 16);

        if (KeyCount == 2)
        {
            cipherForward(Output, &Output, RoundKeys);
        }
        else
        {
            cipherForward(Output, &Output, RoundKeys + 32);
        }
    }

    write_be_uint64(Result, Output);
}

/*
 * cipherCbcEncrypt: encrypt a buffer using multiple invocations of the MechaCon cipher
 * in CBC mode.
 */
int cipherCbcEncrypt(uint8_t *Result, const uint8_t *Data, size_t Length,
                     const uint8_t *Keys, int KeyCount, const uint8_t IV[8])
{
    uint64_t RoundKeys[16 * 3];
    uint8_t LastBlock[8];
    size_t i, k;

    if (Length <= 0)
    {
        return -2;
    }

    KeyCount = cipherKeySchedule(RoundKeys, Keys, KeyCount);
    if (KeyCount < 1)
    {
        return -1;
    }

    memcpy(LastBlock, IV, sizeof(LastBlock));
    for (i = 0; Length - i * 8 >= 8; i++)
    {
        uint8_t InputBlock[8];

        memxor(LastBlock, Data + i * 8, InputBlock, sizeof(InputBlock));
        cipherSingleBlock(LastBlock, InputBlock, RoundKeys, KeyCount);
        memcpy(Result + i * 8, LastBlock, sizeof(LastBlock));
    }

    if (Length - i * 8 > 0)
    {
        uint8_t BaseBlock[8];

        cipherSingleBlock(BaseBlock, LastBlock, RoundKeys, KeyCount);
        for (k = 0; k < Length - i * 8; k++)
        {
            Result[i * 8 + k] = BaseBlock[k] ^ Data[i * 8 + k];
        }
    }

    return 0;
}

/*
 * cipherCbcDecrypt: decrypt a buffer using multiple invocations of the MechaCon cipher
 * in CBC mode.
 */
int cipherCbcDecrypt(uint8_t *Result, const uint8_t *Data, size_t Length,
                     const uint8_t *Keys, int KeyCount, const uint8_t IV[8])
{
    uint64_t RoundKeys[16 * 3];
    uint8_t LastBlock[8], TailBlock[8];
    size_t i, k;

    if (Length <= 0)
    {
        return -2;
    }

    KeyCount = cipherKeyScheduleReverse(RoundKeys, Keys, KeyCount);
    if (KeyCount < 1)
    {
        return -1;
    }

    memcpy(LastBlock, IV, sizeof(LastBlock));
    for (i = 0; Length - i * 8 >= 8; i++)
    {
        uint8_t OutputBlock[8];

        memcpy(TailBlock, Data + i * 8, sizeof(TailBlock));

        cipherSingleBlock(OutputBlock, TailBlock, RoundKeys, KeyCount);
        memxor(LastBlock, OutputBlock, OutputBlock, sizeof(OutputBlock));

        memcpy(LastBlock, TailBlock, sizeof(TailBlock));
        memcpy(Result + i * 8, OutputBlock, sizeof(LastBlock));
    }

    if (Length - i * 8 > 0)
    {
        uint8_t BaseBlock[8];

        cipherKeySchedule(RoundKeys, Keys, KeyCount);
        cipherSingleBlock(BaseBlock, TailBlock, RoundKeys, KeyCount);
        for (k = 0; k < Length - i * 8; k++)
        {
            Result[i * 8 + k] = BaseBlock[k] ^ Data[i * 8 + k];
        }
    }

    return 0;
}

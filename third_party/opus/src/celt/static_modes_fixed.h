/* The contents of this file was automatically generated by dump_modes.c
   with arguments: 48000 960
   It contains static definitions for some pre-defined modes. */
#include "modes.h"
#include "rate.h"

#ifdef HAVE_ARM_NE10
#define OVERRIDE_FFT 1
#include "static_modes_fixed_arm_ne10.h"
#endif

#ifndef DEF_WINDOW120
#define DEF_WINDOW120
static const opus_val16 window120[120] = {
2, 20, 55, 108, 178,
266, 372, 494, 635, 792,
966, 1157, 1365, 1590, 1831,
2089, 2362, 2651, 2956, 3276,
3611, 3961, 4325, 4703, 5094,
5499, 5916, 6346, 6788, 7241,
7705, 8179, 8663, 9156, 9657,
10167, 10684, 11207, 11736, 12271,
12810, 13353, 13899, 14447, 14997,
15547, 16098, 16648, 17197, 17744,
18287, 18827, 19363, 19893, 20418,
20936, 21447, 21950, 22445, 22931,
23407, 23874, 24330, 24774, 25208,
25629, 26039, 26435, 26819, 27190,
27548, 27893, 28224, 28541, 28845,
29135, 29411, 29674, 29924, 30160,
30384, 30594, 30792, 30977, 31151,
31313, 31463, 31602, 31731, 31849,
31958, 32057, 32148, 32229, 32303,
32370, 32429, 32481, 32528, 32568,
32604, 32634, 32661, 32683, 32701,
32717, 32729, 32740, 32748, 32754,
32758, 32762, 32764, 32766, 32767,
32767, 32767, 32767, 32767, 32767,
};
#endif

#ifndef DEF_LOGN400
#define DEF_LOGN400
static const opus_int16 logN400[21] = {
0, 0, 0, 0, 0, 0, 0, 0, 8, 8, 8, 8, 16, 16, 16, 21, 21, 24, 29, 34, 36, };
#endif

#ifndef DEF_PULSE_CACHE50
#define DEF_PULSE_CACHE50
static const opus_int16 cache_index50[105] = {
-1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 41, 41, 41,
82, 82, 123, 164, 200, 222, 0, 0, 0, 0, 0, 0, 0, 0, 41,
41, 41, 41, 123, 123, 123, 164, 164, 240, 266, 283, 295, 41, 41, 41,
41, 41, 41, 41, 41, 123, 123, 123, 123, 240, 240, 240, 266, 266, 305,
318, 328, 336, 123, 123, 123, 123, 123, 123, 123, 123, 240, 240, 240, 240,
305, 305, 305, 318, 318, 343, 351, 358, 364, 240, 240, 240, 240, 240, 240,
240, 240, 305, 305, 305, 305, 343, 343, 343, 351, 351, 370, 376, 382, 387,
};
static const unsigned char cache_bits50[392] = {
40, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 40, 15, 23, 28,
31, 34, 36, 38, 39, 41, 42, 43, 44, 45, 46, 47, 47, 49, 50,
51, 52, 53, 54, 55, 55, 57, 58, 59, 60, 61, 62, 63, 63, 65,
66, 67, 68, 69, 70, 71, 71, 40, 20, 33, 41, 48, 53, 57, 61,
64, 66, 69, 71, 73, 75, 76, 78, 80, 82, 85, 87, 89, 91, 92,
94, 96, 98, 101, 103, 105, 107, 108, 110, 112, 114, 117, 119, 121, 123,
124, 126, 128, 40, 23, 39, 51, 60, 67, 73, 79, 83, 87, 91, 94,
97, 100, 102, 105, 107, 111, 115, 118, 121, 124, 126, 129, 131, 135, 139,
142, 145, 148, 150, 153, 155, 159, 163, 166, 169, 172, 174, 177, 179, 35,
28, 49, 65, 78, 89, 99, 107, 114, 120, 126, 132, 136, 141, 145, 149,
153, 159, 165, 171, 176, 180, 185, 189, 192, 199, 205, 211, 216, 220, 225,
229, 232, 239, 245, 251, 21, 33, 58, 79, 97, 112, 125, 137, 148, 157,
166, 174, 182, 189, 195, 201, 207, 217, 227, 235, 243, 251, 17, 35, 63,
86, 106, 123, 139, 152, 165, 177, 187, 197, 206, 214, 222, 230, 237, 250,
25, 31, 55, 75, 91, 105, 117, 128, 138, 146, 154, 161, 168, 174, 180,
185, 190, 200, 208, 215, 222, 229, 235, 240, 245, 255, 16, 36, 65, 89,
110, 128, 144, 159, 173, 185, 196, 207, 217, 226, 234, 242, 250, 11, 41,
74, 103, 128, 151, 172, 191, 209, 225, 241, 255, 9, 43, 79, 110, 138,
163, 186, 207, 227, 246, 12, 39, 71, 99, 123, 144, 164, 182, 198, 214,
228, 241, 253, 9, 44, 81, 113, 142, 168, 192, 214, 235, 255, 7, 49,
90, 127, 160, 191, 220, 247, 6, 51, 95, 134, 170, 203, 234, 7, 47,
87, 123, 155, 184, 212, 237, 6, 52, 97, 137, 174, 208, 240, 5, 57,
106, 151, 192, 231, 5, 59, 111, 158, 202, 243, 5, 55, 103, 147, 187,
224, 5, 60, 113, 161, 206, 248, 4, 65, 122, 175, 224, 4, 67, 127,
182, 234, };
static const unsigned char cache_caps50[168] = {
224, 224, 224, 224, 224, 224, 224, 224, 160, 160, 160, 160, 185, 185, 185,
178, 178, 168, 134, 61, 37, 224, 224, 224, 224, 224, 224, 224, 224, 240,
240, 240, 240, 207, 207, 207, 198, 198, 183, 144, 66, 40, 160, 160, 160,
160, 160, 160, 160, 160, 185, 185, 185, 185, 193, 193, 193, 183, 183, 172,
138, 64, 38, 240, 240, 240, 240, 240, 240, 240, 240, 207, 207, 207, 207,
204, 204, 204, 193, 193, 180, 143, 66, 40, 185, 185, 185, 185, 185, 185,
185, 185, 193, 193, 193, 193, 193, 193, 193, 183, 183, 172, 138, 65, 39,
207, 207, 207, 207, 207, 207, 207, 207, 204, 204, 204, 204, 201, 201, 201,
188, 188, 176, 141, 66, 40, 193, 193, 193, 193, 193, 193, 193, 193, 193,
193, 193, 193, 194, 194, 194, 184, 184, 173, 139, 65, 39, 204, 204, 204,
204, 204, 204, 204, 204, 201, 201, 201, 201, 198, 198, 198, 187, 187, 175,
140, 66, 40, };
#endif

#ifndef FFT_TWIDDLES48000_960
#define FFT_TWIDDLES48000_960
static const kiss_twiddle_cpx fft_twiddles48000_960[480] = {
{32767, 0}, {32766, -429},
{32757, -858}, {32743, -1287},
{32724, -1715}, {32698, -2143},
{32667, -2570}, {32631, -2998},
{32588, -3425}, {32541, -3851},
{32488, -4277}, {32429, -4701},
{32364, -5125}, {32295, -5548},
{32219, -5971}, {32138, -6393},
{32051, -6813}, {31960, -7231},
{31863, -7650}, {31760, -8067},
{31652, -8481}, {31539, -8895},
{31419, -9306}, {31294, -9716},
{31165, -10126}, {31030, -10532},
{30889, -10937}, {30743, -11340},
{30592, -11741}, {30436, -12141},
{30274, -12540}, {30107, -12935},
{29936, -13328}, {29758, -13718},
{29577, -14107}, {29390, -14493},
{29197, -14875}, {29000, -15257},
{28797, -15635}, {28590, -16010},
{28379, -16384}, {28162, -16753},
{27940, -17119}, {27714, -17484},
{27482, -17845}, {27246, -18205},
{27006, -18560}, {26760, -18911},
{26510, -19260}, {26257, -19606},
{25997, -19947}, {25734, -20286},
{25466, -20621}, {25194, -20952},
{24918, -21281}, {24637, -21605},
{24353, -21926}, {24063, -22242},
{23770, -22555}, {23473, -22865},
{23171, -23171}, {22866, -23472},
{22557, -23769}, {22244, -24063},
{21927, -24352}, {21606, -24636},
{21282, -24917}, {20954, -25194},
{20622, -25465}, {20288, -25733},
{19949, -25997}, {19607, -26255},
{19261, -26509}, {18914, -26760},
{18561, -27004}, {18205, -27246},
{17846, -27481}, {17485, -27713},
{17122, -27940}, {16755, -28162},
{16385, -28378}, {16012, -28590},
{15636, -28797}, {15258, -28999},
{14878, -29197}, {14494, -29389},
{14108, -29576}, {13720, -29757},
{13329, -29934}, {12937, -30107},
{12540, -30274}, {12142, -30435},
{11744, -30592}, {11342, -30743},
{10939, -30889}, {10534, -31030},
{10127, -31164}, {9718, -31294},
{9307, -31418}, {8895, -31537},
{8482, -31652}, {8067, -31759},
{7650, -31862}, {7233, -31960},
{6815, -32051}, {6393, -32138},
{5973, -32219}, {5549, -32294},
{5127, -32364}, {4703, -32429},
{4278, -32487}, {3852, -32541},
{3426, -32588}, {2999, -32630},
{2572, -32667}, {2144, -32698},
{1716, -32724}, {1287, -32742},
{860, -32757}, {430, -32766},
{0, -32767}, {-429, -32766},
{-858, -32757}, {-1287, -32743},
{-1715, -32724}, {-2143, -32698},
{-2570, -32667}, {-2998, -32631},
{-3425, -32588}, {-3851, -32541},
{-4277, -32488}, {-4701, -32429},
{-5125, -32364}, {-5548, -32295},
{-5971, -32219}, {-6393, -32138},
{-6813, -32051}, {-7231, -31960},
{-7650, -31863}, {-8067, -31760},
{-8481, -31652}, {-8895, -31539},
{-9306, -31419}, {-9716, -31294},
{-10126, -31165}, {-10532, -31030},
{-10937, -30889}, {-11340, -30743},
{-11741, -30592}, {-12141, -30436},
{-12540, -30274}, {-12935, -30107},
{-13328, -29936}, {-13718, -29758},
{-14107, -29577}, {-14493, -29390},
{-14875, -29197}, {-15257, -29000},
{-15635, -28797}, {-16010, -28590},
{-16384, -28379}, {-16753, -28162},
{-17119, -27940}, {-17484, -27714},
{-17845, -27482}, {-18205, -27246},
{-18560, -27006}, {-18911, -26760},
{-19260, -26510}, {-19606, -26257},
{-19947, -25997}, {-20286, -25734},
{-20621, -25466}, {-20952, -25194},
{-21281, -24918}, {-21605, -24637},
{-21926, -24353}, {-22242, -24063},
{-22555, -23770}, {-22865, -23473},
{-23171, -23171}, {-23472, -22866},
{-23769, -22557}, {-24063, -22244},
{-24352, -21927}, {-24636, -21606},
{-24917, -21282}, {-25194, -20954},
{-25465, -20622}, {-25733, -20288},
{-25997, -19949}, {-26255, -19607},
{-26509, -19261}, {-26760, -18914},
{-27004, -18561}, {-27246, -18205},
{-27481, -17846}, {-27713, -17485},
{-27940, -17122}, {-28162, -16755},
{-28378, -16385}, {-28590, -16012},
{-28797, -15636}, {-28999, -15258},
{-29197, -14878}, {-29389, -14494},
{-29576, -14108}, {-29757, -13720},
{-29934, -13329}, {-30107, -12937},
{-30274, -12540}, {-30435, -12142},
{-30592, -11744}, {-30743, -11342},
{-30889, -10939}, {-31030, -10534},
{-31164, -10127}, {-31294, -9718},
{-31418, -9307}, {-31537, -8895},
{-31652, -8482}, {-31759, -8067},
{-31862, -7650}, {-31960, -7233},
{-32051, -6815}, {-32138, -6393},
{-32219, -5973}, {-32294, -5549},
{-32364, -5127}, {-32429, -4703},
{-32487, -4278}, {-32541, -3852},
{-32588, -3426}, {-32630, -2999},
{-32667, -2572}, {-32698, -2144},
{-32724, -1716}, {-32742, -1287},
{-32757, -860}, {-32766, -430},
{-32767, 0}, {-32766, 429},
{-32757, 858}, {-32743, 1287},
{-32724, 1715}, {-32698, 2143},
{-32667, 2570}, {-32631, 2998},
{-32588, 3425}, {-32541, 3851},
{-32488, 4277}, {-32429, 4701},
{-32364, 5125}, {-32295, 5548},
{-32219, 5971}, {-32138, 6393},
{-32051, 6813}, {-31960, 7231},
{-31863, 7650}, {-31760, 8067},
{-31652, 8481}, {-31539, 8895},
{-31419, 9306}, {-31294, 9716},
{-31165, 10126}, {-31030, 10532},
{-30889, 10937}, {-30743, 11340},
{-30592, 11741}, {-30436, 12141},
{-30274, 12540}, {-30107, 12935},
{-29936, 13328}, {-29758, 13718},
{-29577, 14107}, {-29390, 14493},
{-29197, 14875}, {-29000, 15257},
{-28797, 15635}, {-28590, 16010},
{-28379, 16384}, {-28162, 16753},
{-27940, 17119}, {-27714, 17484},
{-27482, 17845}, {-27246, 18205},
{-27006, 18560}, {-26760, 18911},
{-26510, 19260}, {-26257, 19606},
{-25997, 19947}, {-25734, 20286},
{-25466, 20621}, {-25194, 20952},
{-24918, 21281}, {-24637, 21605},
{-24353, 21926}, {-24063, 22242},
{-23770, 22555}, {-23473, 22865},
{-23171, 23171}, {-22866, 23472},
{-22557, 23769}, {-22244, 24063},
{-21927, 24352}, {-21606, 24636},
{-21282, 24917}, {-20954, 25194},
{-20622, 25465}, {-20288, 25733},
{-19949, 25997}, {-19607, 26255},
{-19261, 26509}, {-18914, 26760},
{-18561, 27004}, {-18205, 27246},
{-17846, 27481}, {-17485, 27713},
{-17122, 27940}, {-16755, 28162},
{-16385, 28378}, {-16012, 28590},
{-15636, 28797}, {-15258, 28999},
{-14878, 29197}, {-14494, 29389},
{-14108, 29576}, {-13720, 29757},
{-13329, 29934}, {-12937, 30107},
{-12540, 30274}, {-12142, 30435},
{-11744, 30592}, {-11342, 30743},
{-10939, 30889}, {-10534, 31030},
{-10127, 31164}, {-9718, 31294},
{-9307, 31418}, {-8895, 31537},
{-8482, 31652}, {-8067, 31759},
{-7650, 31862}, {-7233, 31960},
{-6815, 32051}, {-6393, 32138},
{-5973, 32219}, {-5549, 32294},
{-5127, 32364}, {-4703, 32429},
{-4278, 32487}, {-3852, 32541},
{-3426, 32588}, {-2999, 32630},
{-2572, 32667}, {-2144, 32698},
{-1716, 32724}, {-1287, 32742},
{-860, 32757}, {-430, 32766},
{0, 32767}, {429, 32766},
{858, 32757}, {1287, 32743},
{1715, 32724}, {2143, 32698},
{2570, 32667}, {2998, 32631},
{3425, 32588}, {3851, 32541},
{4277, 32488}, {4701, 32429},
{5125, 32364}, {5548, 32295},
{5971, 32219}, {6393, 32138},
{6813, 32051}, {7231, 31960},
{7650, 31863}, {8067, 31760},
{8481, 31652}, {8895, 31539},
{9306, 31419}, {9716, 31294},
{10126, 31165}, {10532, 31030},
{10937, 30889}, {11340, 30743},
{11741, 30592}, {12141, 30436},
{12540, 30274}, {12935, 30107},
{13328, 29936}, {13718, 29758},
{14107, 29577}, {14493, 29390},
{14875, 29197}, {15257, 29000},
{15635, 28797}, {16010, 28590},
{16384, 28379}, {16753, 28162},
{17119, 27940}, {17484, 27714},
{17845, 27482}, {18205, 27246},
{18560, 27006}, {18911, 26760},
{19260, 26510}, {19606, 26257},
{19947, 25997}, {20286, 25734},
{20621, 25466}, {20952, 25194},
{21281, 24918}, {21605, 24637},
{21926, 24353}, {22242, 24063},
{22555, 23770}, {22865, 23473},
{23171, 23171}, {23472, 22866},
{23769, 22557}, {24063, 22244},
{24352, 21927}, {24636, 21606},
{24917, 21282}, {25194, 20954},
{25465, 20622}, {25733, 20288},
{25997, 19949}, {26255, 19607},
{26509, 19261}, {26760, 18914},
{27004, 18561}, {27246, 18205},
{27481, 17846}, {27713, 17485},
{27940, 17122}, {28162, 16755},
{28378, 16385}, {28590, 16012},
{28797, 15636}, {28999, 15258},
{29197, 14878}, {29389, 14494},
{29576, 14108}, {29757, 13720},
{29934, 13329}, {30107, 12937},
{30274, 12540}, {30435, 12142},
{30592, 11744}, {30743, 11342},
{30889, 10939}, {31030, 10534},
{31164, 10127}, {31294, 9718},
{31418, 9307}, {31537, 8895},
{31652, 8482}, {31759, 8067},
{31862, 7650}, {31960, 7233},
{32051, 6815}, {32138, 6393},
{32219, 5973}, {32294, 5549},
{32364, 5127}, {32429, 4703},
{32487, 4278}, {32541, 3852},
{32588, 3426}, {32630, 2999},
{32667, 2572}, {32698, 2144},
{32724, 1716}, {32742, 1287},
{32757, 860}, {32766, 430},
};
#ifndef FFT_BITREV480
#define FFT_BITREV480
static const opus_int16 fft_bitrev480[480] = {
0, 96, 192, 288, 384, 32, 128, 224, 320, 416, 64, 160, 256, 352, 448,
8, 104, 200, 296, 392, 40, 136, 232, 328, 424, 72, 168, 264, 360, 456,
16, 112, 208, 304, 400, 48, 144, 240, 336, 432, 80, 176, 272, 368, 464,
24, 120, 216, 312, 408, 56, 152, 248, 344, 440, 88, 184, 280, 376, 472,
4, 100, 196, 292, 388, 36, 132, 228, 324, 420, 68, 164, 260, 356, 452,
12, 108, 204, 300, 396, 44, 140, 236, 332, 428, 76, 172, 268, 364, 460,
20, 116, 212, 308, 404, 52, 148, 244, 340, 436, 84, 180, 276, 372, 468,
28, 124, 220, 316, 412, 60, 156, 252, 348, 444, 92, 188, 284, 380, 476,
1, 97, 193, 289, 385, 33, 129, 225, 321, 417, 65, 161, 257, 353, 449,
9, 105, 201, 297, 393, 41, 137, 233, 329, 425, 73, 169, 265, 361, 457,
17, 113, 209, 305, 401, 49, 145, 241, 337, 433, 81, 177, 273, 369, 465,
25, 121, 217, 313, 409, 57, 153, 249, 345, 441, 89, 185, 281, 377, 473,
5, 101, 197, 293, 389, 37, 133, 229, 325, 421, 69, 165, 261, 357, 453,
13, 109, 205, 301, 397, 45, 141, 237, 333, 429, 77, 173, 269, 365, 461,
21, 117, 213, 309, 405, 53, 149, 245, 341, 437, 85, 181, 277, 373, 469,
29, 125, 221, 317, 413, 61, 157, 253, 349, 445, 93, 189, 285, 381, 477,
2, 98, 194, 290, 386, 34, 130, 226, 322, 418, 66, 162, 258, 354, 450,
10, 106, 202, 298, 394, 42, 138, 234, 330, 426, 74, 170, 266, 362, 458,
18, 114, 210, 306, 402, 50, 146, 242, 338, 434, 82, 178, 274, 370, 466,
26, 122, 218, 314, 410, 58, 154, 250, 346, 442, 90, 186, 282, 378, 474,
6, 102, 198, 294, 390, 38, 134, 230, 326, 422, 70, 166, 262, 358, 454,
14, 110, 206, 302, 398, 46, 142, 238, 334, 430, 78, 174, 270, 366, 462,
22, 118, 214, 310, 406, 54, 150, 246, 342, 438, 86, 182, 278, 374, 470,
30, 126, 222, 318, 414, 62, 158, 254, 350, 446, 94, 190, 286, 382, 478,
3, 99, 195, 291, 387, 35, 131, 227, 323, 419, 67, 163, 259, 355, 451,
11, 107, 203, 299, 395, 43, 139, 235, 331, 427, 75, 171, 267, 363, 459,
19, 115, 211, 307, 403, 51, 147, 243, 339, 435, 83, 179, 275, 371, 467,
27, 123, 219, 315, 411, 59, 155, 251, 347, 443, 91, 187, 283, 379, 475,
7, 103, 199, 295, 391, 39, 135, 231, 327, 423, 71, 167, 263, 359, 455,
15, 111, 207, 303, 399, 47, 143, 239, 335, 431, 79, 175, 271, 367, 463,
23, 119, 215, 311, 407, 55, 151, 247, 343, 439, 87, 183, 279, 375, 471,
31, 127, 223, 319, 415, 63, 159, 255, 351, 447, 95, 191, 287, 383, 479,
};
#endif

#ifndef FFT_BITREV240
#define FFT_BITREV240
static const opus_int16 fft_bitrev240[240] = {
0, 48, 96, 144, 192, 16, 64, 112, 160, 208, 32, 80, 128, 176, 224,
4, 52, 100, 148, 196, 20, 68, 116, 164, 212, 36, 84, 132, 180, 228,
8, 56, 104, 152, 200, 24, 72, 120, 168, 216, 40, 88, 136, 184, 232,
12, 60, 108, 156, 204, 28, 76, 124, 172, 220, 44, 92, 140, 188, 236,
1, 49, 97, 145, 193, 17, 65, 113, 161, 209, 33, 81, 129, 177, 225,
5, 53, 101, 149, 197, 21, 69, 117, 165, 213, 37, 85, 133, 181, 229,
9, 57, 105, 153, 201, 25, 73, 121, 169, 217, 41, 89, 137, 185, 233,
13, 61, 109, 157, 205, 29, 77, 125, 173, 221, 45, 93, 141, 189, 237,
2, 50, 98, 146, 194, 18, 66, 114, 162, 210, 34, 82, 130, 178, 226,
6, 54, 102, 150, 198, 22, 70, 118, 166, 214, 38, 86, 134, 182, 230,
10, 58, 106, 154, 202, 26, 74, 122, 170, 218, 42, 90, 138, 186, 234,
14, 62, 110, 158, 206, 30, 78, 126, 174, 222, 46, 94, 142, 190, 238,
3, 51, 99, 147, 195, 19, 67, 115, 163, 211, 35, 83, 131, 179, 227,
7, 55, 103, 151, 199, 23, 71, 119, 167, 215, 39, 87, 135, 183, 231,
11, 59, 107, 155, 203, 27, 75, 123, 171, 219, 43, 91, 139, 187, 235,
15, 63, 111, 159, 207, 31, 79, 127, 175, 223, 47, 95, 143, 191, 239,
};
#endif

#ifndef FFT_BITREV120
#define FFT_BITREV120
static const opus_int16 fft_bitrev120[120] = {
0, 24, 48, 72, 96, 8, 32, 56, 80, 104, 16, 40, 64, 88, 112,
4, 28, 52, 76, 100, 12, 36, 60, 84, 108, 20, 44, 68, 92, 116,
1, 25, 49, 73, 97, 9, 33, 57, 81, 105, 17, 41, 65, 89, 113,
5, 29, 53, 77, 101, 13, 37, 61, 85, 109, 21, 45, 69, 93, 117,
2, 26, 50, 74, 98, 10, 34, 58, 82, 106, 18, 42, 66, 90, 114,
6, 30, 54, 78, 102, 14, 38, 62, 86, 110, 22, 46, 70, 94, 118,
3, 27, 51, 75, 99, 11, 35, 59, 83, 107, 19, 43, 67, 91, 115,
7, 31, 55, 79, 103, 15, 39, 63, 87, 111, 23, 47, 71, 95, 119,
};
#endif

#ifndef FFT_BITREV60
#define FFT_BITREV60
static const opus_int16 fft_bitrev60[60] = {
0, 12, 24, 36, 48, 4, 16, 28, 40, 52, 8, 20, 32, 44, 56,
1, 13, 25, 37, 49, 5, 17, 29, 41, 53, 9, 21, 33, 45, 57,
2, 14, 26, 38, 50, 6, 18, 30, 42, 54, 10, 22, 34, 46, 58,
3, 15, 27, 39, 51, 7, 19, 31, 43, 55, 11, 23, 35, 47, 59,
};
#endif

#ifndef FFT_STATE48000_960_0
#define FFT_STATE48000_960_0
/*
 typedef struct {
 int n;
 int maxshift;
 const kiss_fft_state *kfft[4];
 const kiss_twiddle_scalar * OPUS_RESTRICT trig;
 } mdct_lookup;
 */
static const kiss_fft_state fft_state48000_960_0 = {
480,    /* nfft */
17476,    /* scale */
8,      /* scale_shift */
-1,     /* shift */
{5, 96, 3, 32, 4, 8, 2, 4, 4, 1, 0, 0, 0, 0, 0, 0, },    /* factors */
fft_bitrev480,  /* bitrev */
fft_twiddles48000_960,  /* bitrev */
#ifdef OVERRIDE_FFT
(arch_fft_state *)&cfg_arch_480,
#else
NULL,
#endif
};
#endif

#ifndef FFT_STATE48000_960_1
#define FFT_STATE48000_960_1
static const kiss_fft_state fft_state48000_960_1 = {
240,    /* nfft */
17476,    /* scale */
7,      /* scale_shift */
1,      /* shift */
{5, 48, 3, 16, 4, 4, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, },    /* factors */
fft_bitrev240,  /* bitrev */
fft_twiddles48000_960,  /* bitrev */
#ifdef OVERRIDE_FFT
(arch_fft_state *)&cfg_arch_240,
#else
NULL,
#endif
};
#endif

#ifndef FFT_STATE48000_960_2
#define FFT_STATE48000_960_2
static const kiss_fft_state fft_state48000_960_2 = {
120,    /* nfft */
17476,    /* scale */
6,      /* scale_shift */
2,      /* shift */
{5, 24, 3, 8, 2, 4, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, },    /* factors */
fft_bitrev120,  /* bitrev */
fft_twiddles48000_960,  /* bitrev */
#ifdef OVERRIDE_FFT
(arch_fft_state *)&cfg_arch_120,
#else
NULL,
#endif
};
#endif

#ifndef FFT_STATE48000_960_3
#define FFT_STATE48000_960_3
static const kiss_fft_state fft_state48000_960_3 = {
60,     /* nfft */
17476,    /* scale */
5,      /* scale_shift */
3,      /* shift */
{5, 12, 3, 4, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },    /* factors */
fft_bitrev60,   /* bitrev */
fft_twiddles48000_960,  /* bitrev */
#ifdef OVERRIDE_FFT
(arch_fft_state *)&cfg_arch_60,
#else
NULL,
#endif
};
#endif

#endif

#ifndef MDCT_TWIDDLES960
#define MDCT_TWIDDLES960
static const opus_val16 mdct_twiddles960[1800] = {
32767, 32767, 32767, 32766, 32765,
32763, 32761, 32759, 32756, 32753,
32750, 32746, 32742, 32738, 32733,
32728, 32722, 32717, 32710, 32704,
32697, 32690, 32682, 32674, 32666,
32657, 32648, 32639, 32629, 32619,
32609, 32598, 32587, 32576, 32564,
32552, 32539, 32526, 32513, 32500,
32486, 32472, 32457, 32442, 32427,
32411, 32395, 32379, 32362, 32345,
32328, 32310, 32292, 32274, 32255,
32236, 32217, 32197, 32177, 32157,
32136, 32115, 32093, 32071, 32049,
32027, 32004, 31981, 31957, 31933,
31909, 31884, 31859, 31834, 31809,
31783, 31756, 31730, 31703, 31676,
31648, 31620, 31592, 31563, 31534,
31505, 31475, 31445, 31415, 31384,
31353, 31322, 31290, 31258, 31226,
31193, 31160, 31127, 31093, 31059,
31025, 30990, 30955, 30920, 30884,
30848, 30812, 30775, 30738, 30701,
30663, 30625, 30587, 30548, 30509,
30470, 30430, 30390, 30350, 30309,
30269, 30227, 30186, 30144, 30102,
30059, 30016, 29973, 29930, 29886,
29842, 29797, 29752, 29707, 29662,
29616, 29570, 29524, 29477, 29430,
29383, 29335, 29287, 29239, 29190,
29142, 29092, 29043, 28993, 28943,
28892, 28842, 28791, 28739, 28688,
28636, 28583, 28531, 28478, 28425,
28371, 28317, 28263, 28209, 28154,
28099, 28044, 27988, 27932, 27876,
27820, 27763, 27706, 27648, 27591,
27533, 27474, 27416, 27357, 27298,
27238, 27178, 27118, 27058, 26997,
26936, 26875, 26814, 26752, 26690,
26628, 26565, 26502, 26439, 26375,
26312, 26247, 26183, 26119, 26054,
25988, 25923, 25857, 25791, 25725,
25658, 25592, 25524, 25457, 25389,
25322, 25253, 25185, 25116, 25047,
24978, 24908, 24838, 24768, 24698,
24627, 24557, 24485, 24414, 24342,
24270, 24198, 24126, 24053, 23980,
23907, 23834, 23760, 23686, 23612,
23537, 23462, 23387, 23312, 23237,
23161, 23085, 23009, 22932, 22856,
22779, 22701, 22624, 22546, 22468,
22390, 22312, 22233, 22154, 22075,
21996, 21916, 21836, 21756, 21676,
21595, 21515, 21434, 21352, 21271,
21189, 21107, 21025, 20943, 20860,
20777, 20694, 20611, 20528, 20444,
20360, 20276, 20192, 20107, 20022,
19937, 19852, 19767, 19681, 19595,
19509, 19423, 19336, 19250, 19163,
19076, 18988, 18901, 18813, 18725,
18637, 18549, 18460, 18372, 18283,
18194, 18104, 18015, 17925, 17835,
17745, 17655, 17565, 17474, 17383,
17292, 17201, 17110, 17018, 16927,
16835, 16743, 16650, 16558, 16465,
16372, 16279, 16186, 16093, 15999,
15906, 15812, 15718, 15624, 15529,
15435, 15340, 15245, 15150, 15055,
14960, 14864, 14769, 14673, 14577,
14481, 14385, 14288, 14192, 14095,
13998, 13901, 13804, 13706, 13609,
13511, 13414, 13316, 13218, 13119,
13021, 12923, 12824, 12725, 12626,
12527, 12428, 12329, 12230, 12130,
12030, 11930, 11831, 11730, 11630,
11530, 11430, 11329, 11228, 11128,
11027, 10926, 10824, 10723, 10622,
10520, 10419, 10317, 10215, 10113,
10011, 9909, 9807, 9704, 9602,
9499, 9397, 9294, 9191, 9088,
8985, 8882, 8778, 8675, 8572,
8468, 8364, 8261, 8157, 8053,
7949, 7845, 7741, 7637, 7532,
7428, 7323, 7219, 7114, 7009,
6905, 6800, 6695, 6590, 6485,
6380, 6274, 6169, 6064, 5958,
5853, 5747, 5642, 5536, 5430,
5325, 5219, 5113, 5007, 4901,
4795, 4689, 4583, 4476, 4370,
4264, 4157, 4051, 3945, 3838,
3732, 3625, 3518, 3412, 3305,
3198, 3092, 2985, 2878, 2771,
2664, 2558, 2451, 2344, 2237,
2130, 2023, 1916, 1809, 1702,
1594, 1487, 1380, 1273, 1166,
1059, 952, 844, 737, 630,
523, 416, 308, 201, 94,
-13, -121, -228, -335, -442,
-550, -657, -764, -871, -978,
-1086, -1193, -1300, -1407, -1514,
-1621, -1728, -1835, -1942, -2049,
-2157, -2263, -2370, -2477, -2584,
-2691, -2798, -2905, -3012, -3118,
-3225, -3332, -3439, -3545, -3652,
-3758, -3865, -3971, -4078, -4184,
-4290, -4397, -4503, -4609, -4715,
-4821, -4927, -5033, -5139, -5245,
-5351, -5457, -5562, -5668, -5774,
-5879, -5985, -6090, -6195, -6301,
-6406, -6511, -6616, -6721, -6826,
-6931, -7036, -7140, -7245, -7349,
-7454, -7558, -7663, -7767, -7871,
-7975, -8079, -8183, -8287, -8390,
-8494, -8597, -8701, -8804, -8907,
-9011, -9114, -9217, -9319, -9422,
-9525, -9627, -9730, -9832, -9934,
-10037, -10139, -10241, -10342, -10444,
-10546, -10647, -10748, -10850, -10951,
-11052, -11153, -11253, -11354, -11455,
-11555, -11655, -11756, -11856, -11955,
-12055, -12155, -12254, -12354, -12453,
-12552, -12651, -12750, -12849, -12947,
-13046, -13144, -13242, -13340, -13438,
-13536, -13633, -13731, -13828, -13925,
-14022, -14119, -14216, -14312, -14409,
-14505, -14601, -14697, -14793, -14888,
-14984, -15079, -15174, -15269, -15364,
-15459, -15553, -15647, -15741, -15835,
-15929, -16023, -16116, -16210, -16303,
-16396, -16488, -16581, -16673, -16766,
-16858, -16949, -17041, -17133, -17224,
-17315, -17406, -17497, -17587, -17678,
-17768, -17858, -17948, -18037, -18127,
-18216, -18305, -18394, -18483, -18571,
-18659, -18747, -18835, -18923, -19010,
-19098, -19185, -19271, -19358, -19444,
-19531, -19617, -19702, -19788, -19873,
-19959, -20043, -20128, -20213, -20297,
-20381, -20465, -20549, -20632, -20715,
-20798, -20881, -20963, -21046, -21128,
-21210, -21291, -21373, -21454, -21535,
-21616, -21696, -21776, -21856, -21936,
-22016, -22095, -22174, -22253, -22331,
-22410, -22488, -22566, -22643, -22721,
-22798, -22875, -22951, -23028, -23104,
-23180, -23256, -23331, -23406, -23481,
-23556, -23630, -23704, -23778, -23852,
-23925, -23998, -24071, -24144, -24216,
-24288, -24360, -24432, -24503, -24574,
-24645, -24716, -24786, -24856, -24926,
-24995, -25064, -25133, -25202, -25270,
-25339, -25406, -25474, -25541, -25608,
-25675, -25742, -25808, -25874, -25939,
-26005, -26070, -26135, -26199, -26264,
-26327, -26391, -26455, -26518, -26581,
-26643, -26705, -26767, -26829, -26891,
-26952, -27013, -27073, -27133, -27193,
-27253, -27312, -27372, -27430, -27489,
-27547, -27605, -27663, -27720, -27777,
-27834, -27890, -27946, -28002, -28058,
-28113, -28168, -28223, -28277, -28331,
-28385, -28438, -28491, -28544, -28596,
-28649, -28701, -28752, -28803, -28854,
-28905, -28955, -29006, -29055, -29105,
-29154, -29203, -29251, -29299, -29347,
-29395, -29442, -29489, -29535, -29582,
-29628, -29673, -29719, -29764, -29808,
-29853, -29897, -29941, -29984, -30027,
-30070, -30112, -30154, -30196, -30238,
-30279, -30320, -30360, -30400, -30440,
-30480, -30519, -30558, -30596, -30635,
-30672, -30710, -30747, -30784, -30821,
-30857, -30893, -30929, -30964, -30999,
-31033, -31068, -31102, -31135, -31168,
-31201, -31234, -31266, -31298, -31330,
-31361, -31392, -31422, -31453, -31483,
-31512, -31541, -31570, -31599, -31627,
-31655, -31682, -31710, -31737, -31763,
-31789, -31815, -31841, -31866, -31891,
-31915, -31939, -31963, -31986, -32010,
-32032, -32055, -32077, -32099, -32120,
-32141, -32162, -32182, -32202, -32222,
-32241, -32260, -32279, -32297, -32315,
-32333, -32350, -32367, -32383, -32399,
-32415, -32431, -32446, -32461, -32475,
-32489, -32503, -32517, -32530, -32542,
-32555, -32567, -32579, -32590, -32601,
-32612, -32622, -32632, -32641, -32651,
-32659, -32668, -32676, -32684, -32692,
-32699, -32706, -32712, -32718, -32724,
-32729, -32734, -32739, -32743, -32747,
-32751, -32754, -32757, -32760, -32762,
-32764, -32765, -32767, -32767, -32767,
32767, 32767, 32765, 32761, 32756,
32750, 32742, 32732, 32722, 32710,
32696, 32681, 32665, 32647, 32628,
32608, 32586, 32562, 32538, 32512,
32484, 32455, 32425, 32393, 32360,
32326, 32290, 32253, 32214, 32174,
32133, 32090, 32046, 32001, 31954,
31906, 31856, 31805, 31753, 31700,
31645, 31588, 31530, 31471, 31411,
31349, 31286, 31222, 31156, 31089,
31020, 30951, 30880, 30807, 30733,
30658, 30582, 30504, 30425, 30345,
30263, 30181, 30096, 30011, 29924,
29836, 29747, 29656, 29564, 29471,
29377, 29281, 29184, 29086, 28987,
28886, 28784, 28681, 28577, 28471,
28365, 28257, 28147, 28037, 27925,
27812, 27698, 27583, 27467, 27349,
27231, 27111, 26990, 26868, 26744,
26620, 26494, 26367, 26239, 26110,
25980, 25849, 25717, 25583, 25449,
25313, 25176, 25038, 24900, 24760,
24619, 24477, 24333, 24189, 24044,
23898, 23751, 23602, 23453, 23303,
23152, 22999, 22846, 22692, 22537,
22380, 22223, 22065, 21906, 21746,
21585, 21423, 21261, 21097, 20933,
20767, 20601, 20434, 20265, 20096,
19927, 19756, 19584, 19412, 19239,
19065, 18890, 18714, 18538, 18361,
18183, 18004, 17824, 17644, 17463,
17281, 17098, 16915, 16731, 16546,
16361, 16175, 15988, 15800, 15612,
15423, 15234, 15043, 14852, 14661,
14469, 14276, 14083, 13889, 13694,
13499, 13303, 13107, 12910, 12713,
12515, 12317, 12118, 11918, 11718,
11517, 11316, 11115, 10913, 10710,
10508, 10304, 10100, 9896, 9691,
9486, 9281, 9075, 8869, 8662,
8455, 8248, 8040, 7832, 7623,
7415, 7206, 6996, 6787, 6577,
6366, 6156, 5945, 5734, 5523,
5311, 5100, 4888, 4675, 4463,
4251, 4038, 3825, 3612, 3399,
3185, 2972, 2758, 2544, 2330,
2116, 1902, 1688, 1474, 1260,
1045, 831, 617, 402, 188,
-27, -241, -456, -670, -885,
-1099, -1313, -1528, -1742, -1956,
-2170, -2384, -2598, -2811, -3025,
-3239, -3452, -3665, -3878, -4091,
-4304, -4516, -4728, -4941, -5153,
-5364, -5576, -5787, -5998, -6209,
-6419, -6629, -6839, -7049, -7258,
-7467, -7676, -7884, -8092, -8300,
-8507, -8714, -8920, -9127, -9332,
-9538, -9743, -9947, -10151, -10355,
-10558, -10761, -10963, -11165, -11367,
-11568, -11768, -11968, -12167, -12366,
-12565, -12762, -12960, -13156, -13352,
-13548, -13743, -13937, -14131, -14324,
-14517, -14709, -14900, -15091, -15281,
-15470, -15659, -15847, -16035, -16221,
-16407, -16593, -16777, -16961, -17144,
-17326, -17508, -17689, -17869, -18049,
-18227, -18405, -18582, -18758, -18934,
-19108, -19282, -19455, -19627, -19799,
-19969, -20139, -20308, -20475, -20642,
-20809, -20974, -21138, -21301, -21464,
-21626, -21786, -21946, -22105, -22263,
-22420, -22575, -22730, -22884, -23037,
-23189, -23340, -23490, -23640, -23788,
-23935, -24080, -24225, -24369, -24512,
-24654, -24795, -24934, -25073, -25211,
-25347, -25482, -25617, -25750, -25882,
-26013, -26143, -26272, -26399, -26526,
-26651, -26775, -26898, -27020, -27141,
-27260, -27379, -27496, -27612, -27727,
-27841, -27953, -28065, -28175, -28284,
-28391, -28498, -28603, -28707, -28810,
-28911, -29012, -29111, -29209, -29305,
-29401, -29495, -29587, -29679, -29769,
-29858, -29946, -30032, -30118, -30201,
-30284, -30365, -30445, -30524, -30601,
-30677, -30752, -30825, -30897, -30968,
-31038, -31106, -31172, -31238, -31302,
-31365, -31426, -31486, -31545, -31602,
-31658, -31713, -31766, -31818, -31869,
-31918, -31966, -32012, -32058, -32101,
-32144, -32185, -32224, -32262, -32299,
-32335, -32369, -32401, -32433, -32463,
-32491, -32518, -32544, -32568, -32591,
-32613, -32633, -32652, -32669, -32685,
-32700, -32713, -32724, -32735, -32744,
-32751, -32757, -32762, -32766, -32767,
32767, 32764, 32755, 32741, 32720,
32694, 32663, 32626, 32583, 32535,
32481, 32421, 32356, 32286, 32209,
32128, 32041, 31948, 31850, 31747,
31638, 31523, 31403, 31278, 31148,
31012, 30871, 30724, 30572, 30415,
30253, 30086, 29913, 29736, 29553,
29365, 29172, 28974, 28771, 28564,
28351, 28134, 27911, 27684, 27452,
27216, 26975, 26729, 26478, 26223,
25964, 25700, 25432, 25159, 24882,
24601, 24315, 24026, 23732, 23434,
23133, 22827, 22517, 22204, 21886,
21565, 21240, 20912, 20580, 20244,
19905, 19563, 19217, 18868, 18516,
18160, 17802, 17440, 17075, 16708,
16338, 15964, 15588, 15210, 14829,
14445, 14059, 13670, 13279, 12886,
12490, 12093, 11693, 11291, 10888,
10482, 10075, 9666, 9255, 8843,
8429, 8014, 7597, 7180, 6760,
6340, 5919, 5496, 5073, 4649,
4224, 3798, 3372, 2945, 2517,
2090, 1661, 1233, 804, 375,
-54, -483, -911, -1340, -1768,
-2197, -2624, -3052, -3479, -3905,
-4330, -4755, -5179, -5602, -6024,
-6445, -6865, -7284, -7702, -8118,
-8533, -8946, -9358, -9768, -10177,
-10584, -10989, -11392, -11793, -12192,
-12589, -12984, -13377, -13767, -14155,
-14541, -14924, -15305, -15683, -16058,
-16430, -16800, -17167, -17531, -17892,
-18249, -18604, -18956, -19304, -19649,
-19990, -20329, -20663, -20994, -21322,
-21646, -21966, -22282, -22595, -22904,
-23208, -23509, -23806, -24099, -24387,
-24672, -24952, -25228, -25499, -25766,
-26029, -26288, -26541, -26791, -27035,
-27275, -27511, -27741, -27967, -28188,
-28405, -28616, -28823, -29024, -29221,
-29412, -29599, -29780, -29957, -30128,
-30294, -30455, -30611, -30761, -30906,
-31046, -31181, -31310, -31434, -31552,
-31665, -31773, -31875, -31972, -32063,
-32149, -32229, -32304, -32373, -32437,
-32495, -32547, -32594, -32635, -32671,
-32701, -32726, -32745, -32758, -32766,
32767, 32754, 32717, 32658, 32577,
32473, 32348, 32200, 32029, 31837,
31624, 31388, 31131, 30853, 30553,
30232, 29891, 29530, 29148, 28746,
28324, 27883, 27423, 26944, 26447,
25931, 25398, 24847, 24279, 23695,
23095, 22478, 21846, 21199, 20538,
19863, 19174, 18472, 17757, 17030,
16291, 15541, 14781, 14010, 13230,
12441, 11643, 10837, 10024, 9204,
8377, 7545, 6708, 5866, 5020,
4171, 3319, 2464, 1608, 751,
-107, -965, -1822, -2678, -3532,
-4383, -5232, -6077, -6918, -7754,
-8585, -9409, -10228, -11039, -11843,
-12639, -13426, -14204, -14972, -15730,
-16477, -17213, -17937, -18648, -19347,
-20033, -20705, -21363, -22006, -22634,
-23246, -23843, -24423, -24986, -25533,
-26062, -26573, -27066, -27540, -27995,
-28431, -28848, -29245, -29622, -29979,
-30315, -30630, -30924, -31197, -31449,
-31679, -31887, -32074, -32239, -32381,
-32501, -32600, -32675, -32729, -32759,
};
#endif


/*
 typedef struct {
 int n;
 int maxshift;
 const kiss_fft_state *kfft[4];
 const kiss_twiddle_scalar * OPUS_RESTRICT trig;
 } mdct_lookup;
 */
static const CELTMode mode48000_960_120 = {
48000,  /* Fs */
120,    /* overlap */
21,     /* nbEBands */
21,     /* effEBands */
{27853, 0, 4096, 8192, },       /* preemph */
eband5ms,       /* eBands */
3,      /* maxLM */
8,      /* nbShortMdcts */
120,    /* shortMdctSize */
11,     /* nbAllocVectors */
band_allocation,        /* allocVectors */
logN400,        /* logN */
window120,      /* window */
{1920, 3, {&fft_state48000_960_0, &fft_state48000_960_1, &fft_state48000_960_2, &fft_state48000_960_3, }, mdct_twiddles960},    /* mdct */
{392, cache_index50, cache_bits50, cache_caps50},       /* cache */
};

/* List of all the available modes */
#define TOTAL_MODES 1
static const CELTMode * const static_mode_list[TOTAL_MODES] = {
&mode48000_960_120,
};

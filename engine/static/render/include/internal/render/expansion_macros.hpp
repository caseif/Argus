/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// This little INDIRECT_EXPAND macro is necessary because MSVC is awful.
//
// For some reason known only to a few engineers and God, MSVC expands
// __VA_ARGS__ *after* passing it to another macro. From the perspective of the
// second macro, the passed list looks like it contains a single, very long
// argument. Obviously, this is a huge problem when we want to pass it around
// between different macros, like we do here. The solution is to force MSVC to
// do the exapnsion at the appropriate time by wrapping any macro invocations
// passing __VA_ARGS__ with this macro. Thus, the correct macro invocation is
// generated in-place with the expanded arguments.
//
// By the way, when asked for comment, Microsoft responded with, "The Visual C++
// compiler is behaving correctly in this case." Thanks for wasting an hour of
// my life, guys.
#define INDIRECT_EXPAND(x) x

#define M_NARGS(...) INDIRECT_EXPAND(M_NARGS_(__VA_ARGS__,  099, 098, 097, 096, 095, 094, 093, 092, 091, 090, \
                                            089, 088, 087, 086, 085, 084, 083, 082, 081, 080, \
                                            079, 078, 077, 076, 075, 074, 073, 072, 071, 070, \
                                            069, 068, 067, 066, 065, 064, 063, 062, 061, 060, \
                                            059, 058, 057, 056, 055, 054, 053, 052, 051, 050, \
                                            049, 048, 047, 046, 045, 044, 043, 042, 041, 040, \
                                            039, 038, 037, 036, 035, 034, 033, 032, 031, 030, \
                                            029, 028, 027, 026, 025, 024, 023, 022, 021, 020, \
                                            019, 018, 017, 016, 015, 014, 013, 012, 011, 010, \
                                            009, 008, 007, 006, 005, 004, 003, 002, 001, 000))
#define M_NARGS_(   _99, _98, _97, _96, _95, _94, _93, _92, _91, _90, \
                    _89, _88, _87, _86, _85, _84, _83, _82, _81, _80, \
                    _79, _78, _77, _76, _75, _74, _73, _72, _71, _70, \
                    _69, _68, _67, _66, _65, _64, _63, _62, _61, _60, \
                    _59, _58, _57, _56, _55, _54, _53, _52, _51, _50, \
                    _49, _48, _47, _46, _45, _44, _43, _42, _41, _40, \
                    _39, _38, _37, _36, _35, _34, _33, _32, _31, _30, \
                    _29, _28, _27, _26, _25, _24, _23, _22, _21, _20, \
                    _19, _18, _17, _16, _15, _14, _13, _12, _11, _10, \
                    _9, _8, _7, _6, _5, _4, _3, _2, _1, N, ...) N

#define M_CONC(A, B) M_CONC_(A, B)
#define M_CONC_(A, B) A##B
#define M_ID(...) __VA_ARGS__

#define M_FOR_EACH(ACTN, ...) INDIRECT_EXPAND(M_CONC(M_FOR_EACH_, INDIRECT_EXPAND(M_NARGS(__VA_ARGS__))) (ACTN, __VA_ARGS__))

#define M_FOR_EACH_000(ACTN, E) E
#define M_FOR_EACH_001(ACTN, E) ACTN(E)
#define M_FOR_EACH_002(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_001(ACTN, __VA_ARGS__))
#define M_FOR_EACH_003(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_002(ACTN, __VA_ARGS__))
#define M_FOR_EACH_004(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_003(ACTN, __VA_ARGS__))
#define M_FOR_EACH_005(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_004(ACTN, __VA_ARGS__))
#define M_FOR_EACH_006(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_005(ACTN, __VA_ARGS__))
#define M_FOR_EACH_007(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_006(ACTN, __VA_ARGS__))
#define M_FOR_EACH_008(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_007(ACTN, __VA_ARGS__))
#define M_FOR_EACH_009(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_008(ACTN, __VA_ARGS__))
#define M_FOR_EACH_010(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_009(ACTN, __VA_ARGS__))
#define M_FOR_EACH_011(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_010(ACTN, __VA_ARGS__))
#define M_FOR_EACH_012(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_011(ACTN, __VA_ARGS__))
#define M_FOR_EACH_013(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_012(ACTN, __VA_ARGS__))
#define M_FOR_EACH_014(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_013(ACTN, __VA_ARGS__))
#define M_FOR_EACH_015(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_014(ACTN, __VA_ARGS__))
#define M_FOR_EACH_016(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_015(ACTN, __VA_ARGS__))
#define M_FOR_EACH_017(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_016(ACTN, __VA_ARGS__))
#define M_FOR_EACH_018(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_017(ACTN, __VA_ARGS__))
#define M_FOR_EACH_019(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_018(ACTN, __VA_ARGS__))
#define M_FOR_EACH_020(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_019(ACTN, __VA_ARGS__))
#define M_FOR_EACH_021(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_020(ACTN, __VA_ARGS__))
#define M_FOR_EACH_022(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_021(ACTN, __VA_ARGS__))
#define M_FOR_EACH_023(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_022(ACTN, __VA_ARGS__))
#define M_FOR_EACH_024(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_023(ACTN, __VA_ARGS__))
#define M_FOR_EACH_025(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_024(ACTN, __VA_ARGS__))
#define M_FOR_EACH_026(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_025(ACTN, __VA_ARGS__))
#define M_FOR_EACH_027(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_026(ACTN, __VA_ARGS__))
#define M_FOR_EACH_028(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_027(ACTN, __VA_ARGS__))
#define M_FOR_EACH_029(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_028(ACTN, __VA_ARGS__))
#define M_FOR_EACH_030(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_029(ACTN, __VA_ARGS__))
#define M_FOR_EACH_031(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_030(ACTN, __VA_ARGS__))
#define M_FOR_EACH_032(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_031(ACTN, __VA_ARGS__))
#define M_FOR_EACH_033(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_032(ACTN, __VA_ARGS__))
#define M_FOR_EACH_034(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_033(ACTN, __VA_ARGS__))
#define M_FOR_EACH_035(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_034(ACTN, __VA_ARGS__))
#define M_FOR_EACH_036(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_035(ACTN, __VA_ARGS__))
#define M_FOR_EACH_037(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_036(ACTN, __VA_ARGS__))
#define M_FOR_EACH_038(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_037(ACTN, __VA_ARGS__))
#define M_FOR_EACH_039(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_038(ACTN, __VA_ARGS__))
#define M_FOR_EACH_040(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_039(ACTN, __VA_ARGS__))
#define M_FOR_EACH_041(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_040(ACTN, __VA_ARGS__))
#define M_FOR_EACH_042(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_041(ACTN, __VA_ARGS__))
#define M_FOR_EACH_043(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_042(ACTN, __VA_ARGS__))
#define M_FOR_EACH_044(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_043(ACTN, __VA_ARGS__))
#define M_FOR_EACH_045(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_044(ACTN, __VA_ARGS__))
#define M_FOR_EACH_046(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_045(ACTN, __VA_ARGS__))
#define M_FOR_EACH_047(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_046(ACTN, __VA_ARGS__))
#define M_FOR_EACH_048(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_047(ACTN, __VA_ARGS__))
#define M_FOR_EACH_049(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_048(ACTN, __VA_ARGS__))
#define M_FOR_EACH_050(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_049(ACTN, __VA_ARGS__))
#define M_FOR_EACH_051(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_050(ACTN, __VA_ARGS__))
#define M_FOR_EACH_052(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_051(ACTN, __VA_ARGS__))
#define M_FOR_EACH_053(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_052(ACTN, __VA_ARGS__))
#define M_FOR_EACH_054(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_053(ACTN, __VA_ARGS__))
#define M_FOR_EACH_055(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_054(ACTN, __VA_ARGS__))
#define M_FOR_EACH_056(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_055(ACTN, __VA_ARGS__))
#define M_FOR_EACH_057(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_056(ACTN, __VA_ARGS__))
#define M_FOR_EACH_058(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_057(ACTN, __VA_ARGS__))
#define M_FOR_EACH_059(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_058(ACTN, __VA_ARGS__))
#define M_FOR_EACH_060(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_059(ACTN, __VA_ARGS__))
#define M_FOR_EACH_061(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_060(ACTN, __VA_ARGS__))
#define M_FOR_EACH_062(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_061(ACTN, __VA_ARGS__))
#define M_FOR_EACH_063(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_062(ACTN, __VA_ARGS__))
#define M_FOR_EACH_064(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_063(ACTN, __VA_ARGS__))
#define M_FOR_EACH_065(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_064(ACTN, __VA_ARGS__))
#define M_FOR_EACH_066(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_065(ACTN, __VA_ARGS__))
#define M_FOR_EACH_067(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_066(ACTN, __VA_ARGS__))
#define M_FOR_EACH_068(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_067(ACTN, __VA_ARGS__))
#define M_FOR_EACH_069(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_068(ACTN, __VA_ARGS__))
#define M_FOR_EACH_070(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_069(ACTN, __VA_ARGS__))
#define M_FOR_EACH_071(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_070(ACTN, __VA_ARGS__))
#define M_FOR_EACH_072(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_071(ACTN, __VA_ARGS__))
#define M_FOR_EACH_073(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_072(ACTN, __VA_ARGS__))
#define M_FOR_EACH_074(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_073(ACTN, __VA_ARGS__))
#define M_FOR_EACH_075(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_074(ACTN, __VA_ARGS__))
#define M_FOR_EACH_076(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_075(ACTN, __VA_ARGS__))
#define M_FOR_EACH_077(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_076(ACTN, __VA_ARGS__))
#define M_FOR_EACH_078(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_077(ACTN, __VA_ARGS__))
#define M_FOR_EACH_079(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_078(ACTN, __VA_ARGS__))
#define M_FOR_EACH_080(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_079(ACTN, __VA_ARGS__))
#define M_FOR_EACH_081(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_080(ACTN, __VA_ARGS__))
#define M_FOR_EACH_082(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_081(ACTN, __VA_ARGS__))
#define M_FOR_EACH_083(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_082(ACTN, __VA_ARGS__))
#define M_FOR_EACH_084(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_083(ACTN, __VA_ARGS__))
#define M_FOR_EACH_085(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_084(ACTN, __VA_ARGS__))
#define M_FOR_EACH_086(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_085(ACTN, __VA_ARGS__))
#define M_FOR_EACH_087(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_086(ACTN, __VA_ARGS__))
#define M_FOR_EACH_088(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_087(ACTN, __VA_ARGS__))
#define M_FOR_EACH_089(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_088(ACTN, __VA_ARGS__))
#define M_FOR_EACH_090(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_089(ACTN, __VA_ARGS__))
#define M_FOR_EACH_091(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_090(ACTN, __VA_ARGS__))
#define M_FOR_EACH_092(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_091(ACTN, __VA_ARGS__))
#define M_FOR_EACH_093(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_092(ACTN, __VA_ARGS__))
#define M_FOR_EACH_094(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_093(ACTN, __VA_ARGS__))
#define M_FOR_EACH_095(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_094(ACTN, __VA_ARGS__))
#define M_FOR_EACH_096(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_095(ACTN, __VA_ARGS__))
#define M_FOR_EACH_097(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_096(ACTN, __VA_ARGS__))
#define M_FOR_EACH_098(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_097(ACTN, __VA_ARGS__))
#define M_FOR_EACH_099(ACTN, E, ...) ACTN(E) INDIRECT_EXPAND(M_FOR_EACH_098(ACTN, __VA_ARGS__))

#define EXPAND_LIST(macro, ...) INDIRECT_EXPAND(M_FOR_EACH(macro, __VA_ARGS__))

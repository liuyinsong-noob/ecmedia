	.arch armv7-a
	.eabi_attribute 27, 3
	.fpu vfp
	.eabi_attribute 20, 1
	.eabi_attribute 21, 1
	.eabi_attribute 23, 3
	.eabi_attribute 24, 1
	.eabi_attribute 25, 1
	.eabi_attribute 26, 2
	.eabi_attribute 30, 4
	.eabi_attribute 18, 4
	.file	"asm_dec_offsets.c"
	.text
.Ltext0:
	.cfi_sections	.debug_frame
	.section	.text.startup.main,"ax",%progbits
	.align	2
	.global	main
	.hidden	main
	.type	main, %function
main:
	.fnstart
.LFB17:
	.file 1 "/mnt/hgfs/work/MyWebRtc1/third_party/libvpx/build/android/jni/../../../libvpx/vp8/decoder/asm_dec_offsets.c"
	.loc 1 15 0
	.cfi_startproc
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	.loc 1 17 0
#APP
@ 17 "/mnt/hgfs/work/MyWebRtc1/third_party/libvpx/build/android/jni/../../../libvpx/vp8/decoder/asm_dec_offsets.c" 1
	
bool_decoder_user_buffer_end EQU #0
@ 0 "" 2
	.loc 1 18 0
@ 18 "/mnt/hgfs/work/MyWebRtc1/third_party/libvpx/build/android/jni/../../../libvpx/vp8/decoder/asm_dec_offsets.c" 1
	
bool_decoder_user_buffer EQU #4
@ 0 "" 2
	.loc 1 19 0
@ 19 "/mnt/hgfs/work/MyWebRtc1/third_party/libvpx/build/android/jni/../../../libvpx/vp8/decoder/asm_dec_offsets.c" 1
	
bool_decoder_value EQU #8
@ 0 "" 2
	.loc 1 20 0
@ 20 "/mnt/hgfs/work/MyWebRtc1/third_party/libvpx/build/android/jni/../../../libvpx/vp8/decoder/asm_dec_offsets.c" 1
	
bool_decoder_count EQU #12
@ 0 "" 2
	.loc 1 21 0
@ 21 "/mnt/hgfs/work/MyWebRtc1/third_party/libvpx/build/android/jni/../../../libvpx/vp8/decoder/asm_dec_offsets.c" 1
	
bool_decoder_range EQU #16
@ 0 "" 2
	.loc 1 23 0
	mov	r0, #0
	bx	lr
	.cfi_endproc
.LFE17:
	.fnend
	.size	main, .-main
	.text
.Letext0:
	.file 2 "/home/james/android-ndk-r8b/toolchains/arm-linux-androideabi-4.6/prebuilt/linux-x86/bin/../lib/gcc/arm-linux-androideabi/4.6.x-google/include/stddef.h"
	.file 3 "/mnt/hgfs/work/MyWebRtc1/third_party/libvpx/build/android/jni/../../../libvpx/vp8/decoder/dboolhuff.h"
	.file 4 "/home/james/android-ndk-r8b/platforms/android-9/arch-arm/usr/include/unistd.h"
	.section	.debug_info,"",%progbits
.Ldebug_info0:
	.4byte	0x151
	.2byte	0x2
	.4byte	.Ldebug_abbrev0
	.byte	0x4
	.uleb128 0x1
	.4byte	.LASF22
	.byte	0x1
	.4byte	.LASF23
	.4byte	.LASF24
	.4byte	0
	.4byte	0
	.4byte	.Ldebug_ranges0+0
	.4byte	.Ldebug_line0
	.4byte	.Ldebug_pubnames0
	.4byte	.Ldebug_pubtypes0
	.uleb128 0x2
	.byte	0x4
	.byte	0x5
	.ascii	"int\000"
	.uleb128 0x3
	.4byte	.LASF10
	.byte	0x2
	.byte	0xd4
	.4byte	0x43
	.uleb128 0x4
	.byte	0x4
	.byte	0x7
	.4byte	.LASF0
	.uleb128 0x4
	.byte	0x1
	.byte	0x8
	.4byte	.LASF1
	.uleb128 0x4
	.byte	0x1
	.byte	0x6
	.4byte	.LASF2
	.uleb128 0x4
	.byte	0x2
	.byte	0x5
	.4byte	.LASF3
	.uleb128 0x4
	.byte	0x2
	.byte	0x7
	.4byte	.LASF4
	.uleb128 0x4
	.byte	0x8
	.byte	0x5
	.4byte	.LASF5
	.uleb128 0x4
	.byte	0x8
	.byte	0x7
	.4byte	.LASF6
	.uleb128 0x4
	.byte	0x4
	.byte	0x7
	.4byte	.LASF7
	.uleb128 0x4
	.byte	0x4
	.byte	0x5
	.4byte	.LASF8
	.uleb128 0x4
	.byte	0x1
	.byte	0x8
	.4byte	.LASF9
	.uleb128 0x3
	.4byte	.LASF11
	.byte	0x3
	.byte	0x14
	.4byte	0x38
	.uleb128 0x5
	.byte	0x14
	.byte	0x3
	.byte	0x1c
	.4byte	0xe3
	.uleb128 0x6
	.4byte	.LASF12
	.byte	0x3
	.byte	0x1e
	.4byte	0xe3
	.byte	0x2
	.byte	0x23
	.uleb128 0
	.uleb128 0x6
	.4byte	.LASF13
	.byte	0x3
	.byte	0x1f
	.4byte	0xe3
	.byte	0x2
	.byte	0x23
	.uleb128 0x4
	.uleb128 0x6
	.4byte	.LASF14
	.byte	0x3
	.byte	0x20
	.4byte	0x89
	.byte	0x2
	.byte	0x23
	.uleb128 0x8
	.uleb128 0x6
	.4byte	.LASF15
	.byte	0x3
	.byte	0x21
	.4byte	0x31
	.byte	0x2
	.byte	0x23
	.uleb128 0xc
	.uleb128 0x6
	.4byte	.LASF16
	.byte	0x3
	.byte	0x22
	.4byte	0x43
	.byte	0x2
	.byte	0x23
	.uleb128 0x10
	.byte	0
	.uleb128 0x7
	.byte	0x4
	.4byte	0xe9
	.uleb128 0x8
	.4byte	0x4a
	.uleb128 0x3
	.4byte	.LASF17
	.byte	0x3
	.byte	0x23
	.4byte	0x94
	.uleb128 0x4
	.byte	0x8
	.byte	0x4
	.4byte	.LASF18
	.uleb128 0x9
	.byte	0x1
	.4byte	.LASF25
	.byte	0x1
	.byte	0xf
	.byte	0x1
	.4byte	0x31
	.4byte	.LFB17
	.4byte	.LFE17
	.byte	0x2
	.byte	0x7d
	.sleb128 0
	.uleb128 0xa
	.4byte	0x4a
	.4byte	0x128
	.uleb128 0xb
	.4byte	0x43
	.byte	0xff
	.byte	0
	.uleb128 0xc
	.4byte	.LASF19
	.byte	0x3
	.byte	0x25
	.4byte	0x135
	.byte	0x1
	.byte	0x1
	.uleb128 0x8
	.4byte	0x118
	.uleb128 0xc
	.4byte	.LASF20
	.byte	0x4
	.byte	0xac
	.4byte	0x43
	.byte	0x1
	.byte	0x1
	.uleb128 0xc
	.4byte	.LASF21
	.byte	0x4
	.byte	0xb0
	.4byte	0x43
	.byte	0x1
	.byte	0x1
	.byte	0
	.section	.debug_abbrev,"",%progbits
.Ldebug_abbrev0:
	.uleb128 0x1
	.uleb128 0x11
	.byte	0x1
	.uleb128 0x25
	.uleb128 0xe
	.uleb128 0x13
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x1b
	.uleb128 0xe
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x52
	.uleb128 0x1
	.uleb128 0x55
	.uleb128 0x6
	.uleb128 0x10
	.uleb128 0x6
	.uleb128 0x2134
	.uleb128 0x6
	.uleb128 0x2135
	.uleb128 0x6
	.byte	0
	.byte	0
	.uleb128 0x2
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0x8
	.byte	0
	.byte	0
	.uleb128 0x3
	.uleb128 0x16
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x4
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.byte	0
	.byte	0
	.uleb128 0x5
	.uleb128 0x13
	.byte	0x1
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x6
	.uleb128 0xd
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x38
	.uleb128 0xa
	.byte	0
	.byte	0
	.uleb128 0x7
	.uleb128 0xf
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x8
	.uleb128 0x26
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x9
	.uleb128 0x2e
	.byte	0
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0xc
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x40
	.uleb128 0xa
	.byte	0
	.byte	0
	.uleb128 0xa
	.uleb128 0x1
	.byte	0x1
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xb
	.uleb128 0x21
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2f
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0xc
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x3c
	.uleb128 0xc
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_pubnames,"",%progbits
.Ldebug_pubnames0:
	.4byte	0x17
	.2byte	0x2
	.4byte	.Ldebug_info0
	.4byte	0x155
	.4byte	0x100
	.ascii	"main\000"
	.4byte	0
	.section	.debug_pubtypes,"",%progbits
.Ldebug_pubtypes0:
	.4byte	0xff
	.2byte	0x2
	.4byte	.Ldebug_info0
	.4byte	0x155
	.4byte	0x31
	.ascii	"int\000"
	.4byte	0x43
	.ascii	"unsigned int\000"
	.4byte	0x38
	.ascii	"size_t\000"
	.4byte	0x4a
	.ascii	"unsigned char\000"
	.4byte	0x51
	.ascii	"signed char\000"
	.4byte	0x58
	.ascii	"short int\000"
	.4byte	0x5f
	.ascii	"short unsigned int\000"
	.4byte	0x66
	.ascii	"long long int\000"
	.4byte	0x6d
	.ascii	"long long unsigned int\000"
	.4byte	0x74
	.ascii	"long unsigned int\000"
	.4byte	0x7b
	.ascii	"long int\000"
	.4byte	0x82
	.ascii	"char\000"
	.4byte	0x89
	.ascii	"VP8_BD_VALUE\000"
	.4byte	0xee
	.ascii	"BOOL_DECODER\000"
	.4byte	0xf9
	.ascii	"double\000"
	.4byte	0
	.section	.debug_aranges,"",%progbits
	.4byte	0x1c
	.2byte	0x2
	.4byte	.Ldebug_info0
	.byte	0x4
	.byte	0
	.2byte	0
	.2byte	0
	.4byte	.LFB17
	.4byte	.LFE17-.LFB17
	.4byte	0
	.4byte	0
	.section	.debug_ranges,"",%progbits
.Ldebug_ranges0:
	.4byte	.LFB17
	.4byte	.LFE17
	.4byte	0
	.4byte	0
	.section	.debug_line,"",%progbits
.Ldebug_line0:
	.section	.debug_str,"MS",%progbits,1
.LASF5:
	.ascii	"long long int\000"
.LASF10:
	.ascii	"size_t\000"
.LASF11:
	.ascii	"VP8_BD_VALUE\000"
.LASF25:
	.ascii	"main\000"
.LASF24:
	.ascii	"/mnt/hgfs/work/MyWebRtc1/third_party/libvpx/build/a"
	.ascii	"ndroid/jni\000"
.LASF12:
	.ascii	"user_buffer_end\000"
.LASF7:
	.ascii	"long unsigned int\000"
.LASF6:
	.ascii	"long long unsigned int\000"
.LASF23:
	.ascii	"/mnt/hgfs/work/MyWebRtc1/third_party/libvpx/build/a"
	.ascii	"ndroid/jni/../../../libvpx/vp8/decoder/asm_dec_offs"
	.ascii	"ets.c\000"
.LASF21:
	.ascii	"__page_shift\000"
.LASF1:
	.ascii	"unsigned char\000"
.LASF22:
	.ascii	"GNU C 4.6.x-google 20120106 (prerelease)\000"
.LASF19:
	.ascii	"vp8_norm\000"
.LASF15:
	.ascii	"count\000"
.LASF8:
	.ascii	"long int\000"
.LASF18:
	.ascii	"double\000"
.LASF16:
	.ascii	"range\000"
.LASF4:
	.ascii	"short unsigned int\000"
.LASF2:
	.ascii	"signed char\000"
.LASF20:
	.ascii	"__page_size\000"
.LASF13:
	.ascii	"user_buffer\000"
.LASF14:
	.ascii	"value\000"
.LASF17:
	.ascii	"BOOL_DECODER\000"
.LASF3:
	.ascii	"short int\000"
.LASF0:
	.ascii	"unsigned int\000"
.LASF9:
	.ascii	"char\000"
	.section	.note.GNU-stack,"",%progbits

SOXR_DIR     = ../../../soxr/libsoxr/src
MAK_DIR      = ../../../ffmpeg/
BIN_DIR      = $(MAK_DIR)../../../bin

ifeq ($(64BIT),yes)
	PLATFORM = x64
else
	PLATFORM = Win32
endif

ifeq ($(DEBUG),yes)
	CONFIGURATION = Debug
else
	CONFIGURATION = Release
endif

OBJ_DIR	          = $(BIN_DIR)/obj/$(CONFIGURATION)_$(PLATFORM)/ffmpeg/
TARGET_LIB_DIR    = $(BIN_DIR)/lib/$(CONFIGURATION)_$(PLATFORM)
LIB_LIBAVCODEC    = $(OBJ_DIR)libavcodec.a
LIB_LIBAVCODEC_B  = $(OBJ_DIR)libavcodec_b.a
LIB_LIBAVCODEC_C  = $(OBJ_DIR)libavcodec_c.a
LIB_LIBAVFILTER   = $(OBJ_DIR)libavfilter.a
LIB_LIBAVUTIL     = $(OBJ_DIR)libavutil.a
LIB_LIBSWRESAMPLE = $(OBJ_DIR)libswresample.a
LIB_LIBSWSCALE    = $(OBJ_DIR)libswscale.a
TARGET_LIB        = $(TARGET_LIB_DIR)/ffmpeg.lib
ARSCRIPT          = $(OBJ_DIR)script.ar

# Compiler and yasm flags
CFLAGS = -I. -I.. -I $(MAK_DIR) -I $(SOXR_DIR) -D__USE_MINGW_ANSI_STDIO=0 \
	   -DHAVE_AV_CONFIG_H -D_ISOC99_SOURCE -D_XOPEN_SOURCE=600 \
	   -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DOPJ_STATIC \
	   -D_WIN32_WINNT=0x0600 -DWINVER=0x0600 \
	   -fomit-frame-pointer -std=c11  \
	   -fno-common -fno-ident -mthreads
YASMFLAGS = -I. -I$(MAK_DIR) -Pconfig.asm
AVCODECFLAGS= -DBUILDING_avcodec


ifeq ($(64BIT),yes)
	GCC_PREFIX  = x86_64-w64-mingw32-
	TARGET_OS   = x86_64-w64-mingw32
	CFLAGS     += -DWIN64 -D_WIN64 -DARCH_X86_64 -DPIC -I ../thirdparty/64/include
	OPTFLAGS    = -m64 -fno-leading-underscore
	YASMFLAGS  += -f win32 -m amd64 -DWIN64=1 -DARCH_X86_32=0 -DARCH_X86_64=1 -DPIC
else
	TARGET_OS   = i686-w64-mingw32
	CFLAGS     += -DWIN32 -D_WIN32 -DARCH_X86_32 -I ../thirdparty/32/include
	OPTFLAGS    = -m32 -march=i686 -msse -msse2 -mfpmath=sse -mstackrealign
	YASMFLAGS  += -f win32 -m x86 -DWIN32=1 -DARCH_X86_32=1 -DARCH_X86_64=0 -DPREFIX
endif

ifeq ($(DEBUG),yes)
	CFLAGS     += -DDEBUG -D_DEBUG -g -Og
else
	CFLAGS     += -DNDEBUG -UDEBUG -U_DEBUG -O3 -fno-tree-vectorize
endif

# Object directories
OBJ_DIRS = $(OBJ_DIR) \
	$(OBJ_DIR)compat \
	$(OBJ_DIR)libavcodec \
	$(OBJ_DIR)libavcodec/x86 \
	$(OBJ_DIR)libavfilter \
	$(OBJ_DIR)libavfilter/x86 \
	$(OBJ_DIR)libavutil \
	$(OBJ_DIR)libavutil/x86 \
	$(OBJ_DIR)libswresample \
	$(OBJ_DIR)libswresample/x86 \
	$(OBJ_DIR)libswscale \
	$(OBJ_DIR)libswscale/x86 \
	$(TARGET_LIB_DIR)

# Targets
all: make_objdirs $(LIB_LIBAVCODEC) $(LIB_LIBAVCODEC_B) $(LIB_LIBAVFILTER) $(LIB_LIBAVUTIL) $(LIB_LIBSWRESAMPLE) $(LIB_LIBSWSCALE) $(TARGET_LIB)

make_objdirs: $(OBJ_DIRS)
$(OBJ_DIRS):
	$(shell test -d $(@) || mkdir -p $(@))

clean:
	@rm -f $(TARGET_LIB)
	@rm -rf $(OBJ_DIR)

# Objects
SRCS_CF = $(MAK_DIR)config.c

SRCS_LC = \
	\
	libavcodec/ac3_parser.c \
	libavcodec/adts_parser.c \
	libavcodec/allcodecs.c \
	libavcodec/avcodec.c \
	libavcodec/avdct.c \
	libavcodec/avfft.c \
	libavcodec/avpacket.c \
	libavcodec/avpicture.c \
	libavcodec/bitstream.c \
	libavcodec/bitstream_filter.c \
	libavcodec/bitstream_filters.c \
	libavcodec/bsf.c \
	libavcodec/codec_desc.c \
	libavcodec/codec_par.c \
	libavcodec/d3d11va.c \
	libavcodec/decode.c \
	libavcodec/dirac.c \
	libavcodec/dv_profile.c \
	libavcodec/encode.c \
	libavcodec/faandct.c \
	libavcodec/faanidct.c \
	libavcodec/fdctdsp.c \
	libavcodec/fft_fixed_32.c \
	libavcodec/fft_float.c \
	libavcodec/fft_init_table.c \
	libavcodec/file_open.c \
	libavcodec/idctdsp.c \
	libavcodec/imgconvert.c \
	libavcodec/jfdctfst.c \
	libavcodec/jfdctint.c \
	libavcodec/jni.c \
	libavcodec/jrevdct.c \
	libavcodec/mathtables.c \
	libavcodec/mediacodec.c \
	libavcodec/mpeg12framerate.c \
	libavcodec/null_bsf.c \
	libavcodec/options.c \
	libavcodec/parser.c \
	libavcodec/parsers.c \
	libavcodec/profiles.c \
	libavcodec/pthread.c \
	libavcodec/pthread_frame.c \
	libavcodec/pthread_slice.c \
	libavcodec/qsv_api.c \
	libavcodec/raw.c \
	libavcodec/rdft.c \
	libavcodec/simple_idct.c \
	libavcodec/utils.c \
	libavcodec/vorbis_parser.c \
	libavcodec/xiph.c \
	libavcodec/x86/constants.c \
	libavcodec/x86/fdctdsp_init.c \
	libavcodec/x86/fft_init.c \
	libavcodec/x86/idctdsp_init.c \

SRCS_LF = \
	libavfilter/af_aresample.c \
	libavfilter/af_atempo.c \
	libavfilter/af_biquads.c \
	libavfilter/allfilters.c \
	libavfilter/audio.c \
	libavfilter/avfilter.c \
	libavfilter/avfiltergraph.c \
	libavfilter/buffersink.c \
	libavfilter/buffersrc.c \
	libavfilter/formats.c \
	libavfilter/framepool.c \
	libavfilter/framequeue.c \
	libavfilter/graphparser.c \
	libavfilter/pthread.c \
	libavfilter/video.c

SRCS_LU = \
	libavutil/adler32.c \
	libavutil/aes.c \
	libavutil/aes_ctr.c \
	libavutil/audio_fifo.c \
	libavutil/avsscanf.c \
	libavutil/avstring.c \
	libavutil/base64.c \
	libavutil/blowfish.c \
	libavutil/bprint.c \
	libavutil/buffer.c \
	libavutil/camellia.c \
	libavutil/cast5.c \
	libavutil/channel_layout.c \
	libavutil/cpu.c \
	libavutil/crc.c \
	libavutil/des.c \
	libavutil/dict.c \
	libavutil/display.c \
	libavutil/dovi_meta.c \
	libavutil/downmix_info.c \
	libavutil/encryption_info.c \
	libavutil/error.c \
	libavutil/eval.c \
	libavutil/fifo.c \
	libavutil/file.c \
	libavutil/file_open.c \
	libavutil/film_grain_params.c \
	libavutil/fixed_dsp.c \
	libavutil/float_dsp.c \
	libavutil/frame.c \
	libavutil/hash.c \
	libavutil/hdr_dynamic_metadata.c \
	libavutil/hmac.c \
	libavutil/hwcontext.c \
	libavutil/imgutils.c \
	libavutil/integer.c \
	libavutil/intmath.c \
	libavutil/lfg.c \
	libavutil/lls.c \
	libavutil/log.c \
	libavutil/log2_tab.c \
	libavutil/mastering_display_metadata.c \
	libavutil/mathematics.c \
	libavutil/md5.c \
	libavutil/mem.c \
	libavutil/murmur3.c \
	libavutil/opt.c \
	libavutil/parseutils.c \
	libavutil/pixdesc.c \
	libavutil/pixelutils.c \
	libavutil/random_seed.c \
	libavutil/rational.c \
	libavutil/rc4.c \
	libavutil/reverse.c \
	libavutil/ripemd.c \
	libavutil/samplefmt.c \
	libavutil/sha.c \
	libavutil/sha512.c \
	libavutil/slicethread.c \
	libavutil/spherical.c \
	libavutil/stereo3d.c \
	libavutil/tea.c \
	libavutil/threadmessage.c \
	libavutil/time.c \
	libavutil/timecode.c \
	libavutil/tree.c \
	libavutil/twofish.c \
	libavutil/tx.c \
	libavutil/tx_double.c \
	libavutil/tx_float.c \
	libavutil/tx_int32.c \
	libavutil/utils.c \
	libavutil/video_enc_params.c \
	libavutil/xga_font_data.c \
	libavutil/xtea.c \
	libavutil/x86/cpu.c \
	libavutil/x86/fixed_dsp_init.c \
	libavutil/x86/float_dsp_init.c \
	libavutil/x86/imgutils_init.c \
	libavutil/x86/lls_init.c

SRCS_LR = \
	libswresample/audioconvert.c \
	libswresample/dither.c \
	libswresample/options.c \
	libswresample/rematrix.c \
	libswresample/resample.c \
	libswresample/resample_dsp.c \
	libswresample/soxr_resample.c \
	libswresample/swresample.c \
	libswresample/swresample_frame.c \
	libswresample/x86/audio_convert_init.c \
	libswresample/x86/rematrix_init.c \
	libswresample/x86/resample_init.c

# Yasm objects
SRCS_YASM_LC = \
	libavcodec/x86/idctdsp.asm \
	libavcodec/x86/fdct.asm \
	libavcodec/x86/fft.asm \
	libavcodec/x86/simple_idct.asm \
	libavcodec/x86/simple_idct10.asm


SRCS_YASM_LF = 

SRCS_YASM_LU = \
	libavutil/x86/cpuid.asm \
	libavutil/x86/emms.asm \
	libavutil/x86/fixed_dsp.asm \
	libavutil/x86/float_dsp.asm \
	libavutil/x86/imgutils.asm \
	libavutil/x86/lls.asm

SRCS_YASM_LR = \
	libswresample/x86/audio_convert.asm \
	libswresample/x86/rematrix.asm \
	libswresample/x86/resample.asm

OBJS_LC = \
	$(SRCS_LC:%.c=$(OBJ_DIR)%.o) \
	$(OBJ_DIR)config.o \
	$(SRCS_YASM_LC:%.asm=$(OBJ_DIR)%.o)

OBJS_LC_B = \
	$(SRCS_LC_B:%.c=$(OBJ_DIR)%.o)

OBJS_LC_C = \
	$(SRCS_LC_C:%.c=$(OBJ_DIR)%.o)

OBJS_LF = \
	$(SRCS_LF:%.c=$(OBJ_DIR)%.o) \
	$(SRCS_YASM_LF:%.asm=$(OBJ_DIR)%.o)

OBJS_LU = \
	$(SRCS_LU:%.c=$(OBJ_DIR)%.o) \
	$(SRCS_YASM_LU:%.asm=$(OBJ_DIR)%.o)

OBJS_LR = \
	$(SRCS_LR:%.c=$(OBJ_DIR)%.o) \
	$(SRCS_YASM_LR:%.asm=$(OBJ_DIR)%.o)

OBJS_LS = \
	$(SRCS_LS:%.c=$(OBJ_DIR)%.o) \
	$(SRCS_YASM_LS:%.asm=$(OBJ_DIR)%.o)

COMPILE = @$(GCC_PREFIX)gcc -c $(CFLAGS) $(OPTFLAGS) -MMD -Wno-deprecated-declarations -Wno-pointer-to-int-cast -o $@ $<
LIBAR = @$(GCC_PREFIX)ar rc $@ 
YASMC = @yasm $(YASMFLAGS) -I$(<D)/ -o $@ $<

VERSIONH=$(MAK_DIR)/libavutil/ffversion.h

# Commands
$(VERSIONH): Changelog
	sh ffbuild/version.sh . $(MAK_DIR)libavutil/ffversion.h

$(OBJ_DIR)config.o: $(MAK_DIR)config.c
	@echo $<
	$(COMPILE)

$(OBJ_DIR)libavcodec/%.o: libavcodec/%.c
	@echo $<
	$(COMPILE) $(AVCODECFLAGS)

$(OBJ_DIR)%.o: %.c
	@echo $<
	$(COMPILE)

$(OBJ_DIR)%.o: %.asm
	@echo $<
	$(YASMC)

$(LIB_LIBAVCODEC): $(OBJS_LC)
	@echo $@
	$(LIBAR)$(OBJS_LC)

$(LIB_LIBAVCODEC_B): $(OBJS_LC_B)
	@echo $@
	$(LIBAR)$(OBJS_LC_B)

$(LIB_LIBAVCODEC_C): $(OBJS_LC_C)
	@echo $@
	$(LIBAR)$(OBJS_LC_C)

$(LIB_LIBAVFILTER): $(OBJS_LF)
	@echo $@
	$(LIBAR)$(OBJS_LF)

$(LIB_LIBAVUTIL): $(OBJS_LU)
	@echo $@
	$(LIBAR)$(OBJS_LU)

$(LIB_LIBSWSCALE): $(OBJS_LS)
	@echo $@
	$(LIBAR)$(OBJS_LS)

$(LIB_LIBSWRESAMPLE): $(OBJS_LR)
	@echo $@
	$(LIBAR)$(OBJS_LR)

$(TARGET_LIB): $(LIB_LIBAVCODEC) $(LIB_LIBAVCODEC_B) $(LIB_LIBAVCODEC_C) $(LIB_LIBAVFILTER) $(LIB_LIBAVUTIL) $(LIB_LIBSWRESAMPLE)
	@rm -f $(ARSCRIPT)
	@echo "CREATE $@"                   >> $(ARSCRIPT)
	@echo "ADDLIB $(LIB_LIBAVCODEC)"    >> $(ARSCRIPT)
	@echo "ADDLIB $(LIB_LIBAVCODEC_B)"  >> $(ARSCRIPT)
	@echo "ADDLIB $(LIB_LIBAVCODEC_C)"  >> $(ARSCRIPT)
	@echo "ADDLIB $(LIB_LIBAVFILTER)"   >> $(ARSCRIPT)
	@echo "ADDLIB $(LIB_LIBAVUTIL)"     >> $(ARSCRIPT)
	@echo "ADDLIB $(LIB_LIBSWRESAMPLE)" >> $(ARSCRIPT)
	@echo "SAVE"                        >> $(ARSCRIPT)
	@echo "END"                         >> $(ARSCRIPT)
	$(GCC_PREFIX)ar -M < $(ARSCRIPT)

-include $(SRCS_LC:%.c=$(OBJ_DIR)%.d)
-include $(SRCS_LC_B:%.c=$(OBJ_DIR)%.d)
-include $(SRCS_LF:%.c=$(OBJ_DIR)%.d)
-include $(SRCS_LU:%.c=$(OBJ_DIR)%.d)
-include $(SRCS_LR:%.c=$(OBJ_DIR)%.d)
-include $(SRCS_LS:%.c=$(OBJ_DIR)%.d)

.PHONY: clean make_objdirs $(OBJ_DIRS)

.EXTRA_PREREQS+=$(VERSIONH)
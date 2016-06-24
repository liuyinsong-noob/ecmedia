TOOLSET := target
TARGET := amrnb

OUTPUT:= ../../$(OBJDIR)/$(TARGET)
COMMINPUT := ./codecs/opencore-amr/opencore/amr_nb/common/src
DECINPUT:=./codecs/opencore-amr/opencore/amr_nb/dec/src
ENCINPUT:=./codecs/opencore-amr/opencore/amr_nb/enc/src
AMRNBINPUT:=./codecs/opencore-amr/amrnb
MYLIB = ../../$(OUTLIB)

DEFS_Release := '-DWEBRTC_SVNREVISION="2518"' \
	'-D_FILE_OFFSET_BITS=64' \
	'-DOSCL_DLL_H_INCLUDED' 

# Flags passed to all source files.
CFLAGS_Release := -Werror \
	-fno-exceptions \
	-fno-strict-aliasing \
	-Wall \
	-Wno-unused-parameter \
	-Wno-missing-field-initializers \
	-fvisibility=hidden \
	-pipe \
	-fPIC \
	-Wextra \
	-Wno-unused-parameter \
	-Wno-missing-field-initializers \
	-mthumb \
	-march=armv7-a \
	-mtune=cortex-a8 \
	-mfloat-abi=softfp \
	-mfpu=vfpv3-d16 \
	-fno-tree-sra \
	-Wno-psabi \
	-mthumb-interwork \
	-U__linux__ \
	-ffunction-sections \
	-funwind-tables \
	-g \
	-fstack-protector \
	-fno-short-enums \
	-finline-limit=64 \
	-Wa,--noexecstack \
	-Wno-error=non-virtual-dtor \
	-I$(ANDROID_INCLUDE) \
	-I$(STLPORT_DIR) \
	-Os \
	-fno-ident \
	-fdata-sections \
	-ffunction-sections \
	-fomit-frame-pointer

# Flags passed to only C files.
CFLAGS_C_Release := 

# Flags passed to only C++ files.
CFLAGS_CC_Release := -fno-rtti \
	-fno-threadsafe-statics \
	-fvisibility-inlines-hidden \
	-Wsign-compare \
	-Woverloaded-virtual \
	-Wno-abi

INCS_Release := -I./codecs/opencore-amr/opencore/amr_nb/common/include \
    -I./codecs/opencore-amr/opencore/amr_nb/common/src \
    -I./codecs/opencore-amr/oscl \
    -I./codecs/opencore-amr/amrnb  \
    -I./codecs/opencore-amr/opencore/amr_nb/dec/include  \
    -I./codecs/opencore-amr/opencore/amr_nb/dec/src  \
    -I./codecs/opencore-amr/opencore/amr_nb/enc/include  \
    -I./codecs/opencore-amr/opencore/amr_nb/enc/src  \
    -I./codecs/opencore-amr/opencore/common/dec/include

COMMOBJS :=  \
        $(OUTPUT)/add.o \
        $(OUTPUT)/az_lsp.o \
        $(OUTPUT)/bitno_tab.o \
        $(OUTPUT)/bitreorder_tab.o \
        $(OUTPUT)/c2_9pf_tab.o \
        $(OUTPUT)/div_s.o \
        $(OUTPUT)/extract_h.o \
        $(OUTPUT)/extract_l.o \
        $(OUTPUT)/gains_tbl.o \
        $(OUTPUT)/gc_pred.o \
        $(OUTPUT)/get_const_tbls.o \
        $(OUTPUT)/gmed_n.o \
        $(OUTPUT)/gray_tbl.o \
        $(OUTPUT)/grid_tbl.o \
        $(OUTPUT)/int_lpc.o \
        $(OUTPUT)/inv_sqrt.o \
        $(OUTPUT)/inv_sqrt_tbl.o \
        $(OUTPUT)/l_deposit_h.o \
        $(OUTPUT)/l_deposit_l.o \
        $(OUTPUT)/log2.o \
        $(OUTPUT)/log2_norm.o \
        $(OUTPUT)/log2_tbl.o \
        $(OUTPUT)/lsfwt.o \
        $(OUTPUT)/l_shr_r.o \
        $(OUTPUT)/lsp_az.o \
        $(OUTPUT)/lsp.o \
        $(OUTPUT)/lsp_lsf.o \
        $(OUTPUT)/lsp_lsf_tbl.o \
        $(OUTPUT)/lsp_tab.o \
        $(OUTPUT)/mult_r.o \
        $(OUTPUT)/negate.o \
        $(OUTPUT)/norm_l.o \
        $(OUTPUT)/norm_s.o \
        $(OUTPUT)/overflow_tbl.o \
        $(OUTPUT)/ph_disp_tab.o \
        $(OUTPUT)/pow2.o \
        $(OUTPUT)/pow2_tbl.o \
        $(OUTPUT)/pred_lt.o \
        $(OUTPUT)/q_plsf_3.o \
        $(OUTPUT)/q_plsf_3_tbl.o \
        $(OUTPUT)/q_plsf_5.o \
        $(OUTPUT)/q_plsf_5_tbl.o \
        $(OUTPUT)/q_plsf.o \
        $(OUTPUT)/qua_gain_tbl.o \
        $(OUTPUT)/reorder.o \
        $(OUTPUT)/residu.o \
        $(OUTPUT)/round.o \
        $(OUTPUT)/set_zero.o \
        $(OUTPUT)/shr.o \
        $(OUTPUT)/shr_r.o \
        $(OUTPUT)/sqrt_l.o \
        $(OUTPUT)/sqrt_l_tbl.o \
        $(OUTPUT)/sub.o \
        $(OUTPUT)/syn_filt.o \
        $(OUTPUT)/weight_a.o \
        $(OUTPUT)/window_tab.o
 
 DECOBJS :=  \
        $(OUTPUT)/agc.o \
        $(OUTPUT)/amrdecode.o \
        $(OUTPUT)/a_refl.o \
        $(OUTPUT)/b_cn_cod.o \
        $(OUTPUT)/bgnscd.o \
        $(OUTPUT)/c_g_aver.o \
        $(OUTPUT)/d1035pf.o \
        $(OUTPUT)/d2_11pf.o \
        $(OUTPUT)/d2_9pf.o \
        $(OUTPUT)/d3_14pf.o \
        $(OUTPUT)/d4_17pf.o \
        $(OUTPUT)/d8_31pf.o \
        $(OUTPUT)/dec_amr.o \
        $(OUTPUT)/dec_gain.o \
        $(OUTPUT)/dec_input_format_tab.o \
        $(OUTPUT)/dec_lag3.o \
        $(OUTPUT)/dec_lag6.o \
        $(OUTPUT)/d_gain_c.o \
        $(OUTPUT)/d_gain_p.o \
        $(OUTPUT)/d_plsf_3.o \
        $(OUTPUT)/d_plsf_5.o \
        $(OUTPUT)/d_plsf.o \
        $(OUTPUT)/dtx_dec.o \
        $(OUTPUT)/ec_gains.o \
        $(OUTPUT)/ex_ctrl.o \
        $(OUTPUT)/if2_to_ets.o \
        $(OUTPUT)/int_lsf.o \
        $(OUTPUT)/lsp_avg.o \
        $(OUTPUT)/ph_disp.o \
        $(OUTPUT)/post_pro.o \
        $(OUTPUT)/preemph.o \
        $(OUTPUT)/pstfilt.o \
        $(OUTPUT)/qgain475_tab.o \
        $(OUTPUT)/sp_dec.o \
        $(OUTPUT)/wmf_to_ets.o
 
 ENCOBJS :=  \
        $(OUTPUT)/amrencode.o \
        $(OUTPUT)/autocorr.o \
        $(OUTPUT)/c1035pf.o \
        $(OUTPUT)/c2_11pf.o \
        $(OUTPUT)/c2_9pf.o \
        $(OUTPUT)/c3_14pf.o \
        $(OUTPUT)/c4_17pf.o \
        $(OUTPUT)/c8_31pf.o \
        $(OUTPUT)/calc_cor.o \
        $(OUTPUT)/calc_en.o \
        $(OUTPUT)/cbsearch.o \
        $(OUTPUT)/cl_ltp.o \
        $(OUTPUT)/cod_amr.o \
        $(OUTPUT)/convolve.o \
        $(OUTPUT)/cor_h.o \
        $(OUTPUT)/cor_h_x2.o \
        $(OUTPUT)/cor_h_x.o \
        $(OUTPUT)/corrwght_tab.o \
        $(OUTPUT)/div_32.o \
        $(OUTPUT)/dtx_enc.o \
        $(OUTPUT)/enc_lag3.o \
        $(OUTPUT)/enc_lag6.o \
        $(OUTPUT)/enc_output_format_tab.o \
        $(OUTPUT)/ets_to_if2.o \
        $(OUTPUT)/ets_to_wmf.o \
        $(OUTPUT)/g_adapt.o \
        $(OUTPUT)/gain_q.o \
        $(OUTPUT)/g_code.o \
        $(OUTPUT)/g_pitch.o \
        $(OUTPUT)/gsmamr_encoder_wrapper.o \
        $(OUTPUT)/hp_max.o \
        $(OUTPUT)/inter_36.o \
        $(OUTPUT)/inter_36_tab.o \
        $(OUTPUT)/l_abs.o \
        $(OUTPUT)/lag_wind.o \
        $(OUTPUT)/lag_wind_tab.o \
        $(OUTPUT)/l_comp.o \
        $(OUTPUT)/levinson.o \
        $(OUTPUT)/l_extract.o \
        $(OUTPUT)/lflg_upd.o \
        $(OUTPUT)/l_negate.o \
        $(OUTPUT)/lpc.o \
        $(OUTPUT)/ol_ltp.o \
        $(OUTPUT)/pitch_fr.o \
        $(OUTPUT)/pitch_ol.o \
        $(OUTPUT)/p_ol_wgh.o \
        $(OUTPUT)/pre_big.o \
        $(OUTPUT)/pre_proc.o \
        $(OUTPUT)/prm2bits.o \
        $(OUTPUT)/qgain475.o \
        $(OUTPUT)/qgain795.o \
        $(OUTPUT)/q_gain_c.o \
        $(OUTPUT)/q_gain_p.o \
        $(OUTPUT)/qua_gain.o \
        $(OUTPUT)/s10_8pf.o \
        $(OUTPUT)/set_sign.o \
        $(OUTPUT)/sid_sync.o \
        $(OUTPUT)/sp_enc.o \
        $(OUTPUT)/spreproc.o \
        $(OUTPUT)/spstproc.o \
        $(OUTPUT)/ton_stab.o \
        $(OUTPUT)/vad1.o

AMRNBOBJS :=  \
	$(OUTPUT)/amr_interface.o \
 	$(OUTPUT)/wrapper.o \

 
$(MYLIB)/libamrnb.a:$(COMMOBJS) $(DECOBJS) $(ENCOBJS) $(AMRNBOBJS)
	mkdir -p $(MYLIB)
	 $(AR) $(ARFLAGS) $@ $(COMMOBJS) $(DECOBJS) $(ENCOBJS) $(AMRNBOBJS)

$(OUTPUT)/%.o:$(COMMINPUT)/%.cpp
	mkdir -p $(OUTPUT)
	$(CXX) -c -o $@ $(CFLAGS_Release) $(DEFS_Release)  $(CFLAGS_CC_Release) $(INCS_Release) $<

$(OUTPUT)/%.o:$(DECINPUT)/%.cpp
	mkdir -p $(OUTPUT)
	$(CXX) -c -o $@ $(CFLAGS_Release) $(DEFS_Release)  $(CFLAGS_CC_Release) $(INCS_Release) $<

$(OUTPUT)/%.o:$(ENCINPUT)/%.cpp
	mkdir -p $(OUTPUT)
	$(CXX) -c -o $@ $(CFLAGS_Release) $(DEFS_Release)  $(CFLAGS_CC_Release) $(INCS_Release) $<

$(OUTPUT)/%.o:$(AMRNBINPUT)/%.cpp
	mkdir -p $(OUTPUT)
	$(CXX) -c -o $@ $(CFLAGS_Release) $(DEFS_Release)  $(CFLAGS_CC_Release) $(INCS_Release) $<

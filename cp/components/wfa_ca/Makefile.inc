#CC = gcc
LIB=lib
DUT=dut
CA=ca
UCC=ucc
MAKE=make

# This is for WMM-PS
#for Ext TG
#CFLAGS = -g -O2 -D_REENTRANT -DWFA_WMM_PS_EXT -Wall -I../inc

## for WMM-AC, WMM-PS, Voice, PMF, TDLS Test-Bed
##CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_VOICE_EXT -DWFA_STA_TB -Wall -I../inc

## for WMM-AC, WMM-PS Test-Bed
#CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_STA_TB -Wall -I../inc

## for non-Test-Bed WMM-AC, WMM-PS and Voice
#CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_VOICE_EXT -Wall -I../inc

## for PC-ENDPOINT
#CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_VOICE_EXT -DWFA_PC_CONSOLE -Wall -I../inc

## for PC-ENDPOINT No Voice
#CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_PC_CONSOLE -Wall -I../inc

# This is for WPA2 as default
#CFLAGS = -g -O2 -D_REENTRANT -Wall -I../inc


## for DUT
#CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT  -D__APDUT__ -D__P2P__ -Wall -I../inc
#CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT  -D__APDUT__ -Wall -I../inc
CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DCONFIG_BK_FHOST -D__APDUT__ -Wall -Wno-unused-variable -Wno-stringop-truncation -Wno-unused-result -I../inc

# CFLAGS += -DCONFIG_EXT_WIFI
# CFLAGS += -DCONFIG_HE_551
# CFLAGS += -DCONFIG_MBO_527

DUTLIBS = ../lib/libwfa_dut.a -lpthread
CALIBS = ../lib/libwfa_ca.a -lpthread
CADUTLIBS = ../lib/libwfa_cadut.a -lpthread
RANLIB = ranlib

LIBWFA_NAME_DUT = libwfa_dut.a
LIBWFA_NAME_CA = libwfa_ca.a
LIBWFA_NAME_CADUT = libwfa_cadut.a
LIBWFA_NAME = libwfa.a

LIB_OBJS = wfa_sock.o wfa_tg.o wfa_dut_param.o wfa_cs.o wfa_ca_resp.o wfa_tlv.o wfa_typestr.o wfa_cmdtbl.o wfa_cmdproc.o wfa_miscs.o wfa_thr.o wfa_wmmps.o

LIB_OBJS_DUT = wfa_sock.o wfa_tlv.o wfa_dut_param.o wfa_cs.o wfa_cmdtbl.o wfa_tg.o wfa_miscs.o wfa_thr.o wfa_wmmps.o

LIB_OBJS_CA = wfa_sock.o wfa_tlv.o wfa_ca_resp.o wfa_cmdproc.o wfa_miscs.o wfa_typestr.o

LIB_OBJS_CADUT = wfa_sock.o wfa_tlv.o wfa_ca_resp.o wfa_cmdproc.o wfa_miscs.o wfa_typestr.o wfa_dut_param.o wfa_cs.o wfa_cmdtbl.o wfa_tg.o wfa_thr.o wfa_wmmps.o

CLEANFILES = core core.* *.core.* *.o *.a

if (CONFIG_WFA_CA)
	set(incs
		inc
		)

	set(srcs
		ca/wfa_ca.c
		lib/wfa_ca_resp.c
		lib/wfa_cmdproc.c
		lib/wfa_cmdtbl.c
		lib/wfa_cs.c
		lib/wfa_dut_param.c
		lib/wfa_miscs.c
		lib/wfa_sock.c
		lib/wfa_tg.c
		lib/wfa_thr.c
		lib/wfa_tlv.c
		lib/wfa_typestr.c
		lib/wfa_wmmps.c
		)
armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES wpa_supplicant-2.10 bk_wifi)
else()
armino_component_register(PRIV_REQUIRES wpa_supplicant-2.10 bk_wifi)
endif()


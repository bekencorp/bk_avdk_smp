os = os/no_os
linux = os/linux
core = core
build = build
config = config
mshc = drv/mmc/dwc-mshc
msdc = drv/mmc/dwc-msdc
platform = platform/csim_sdlink
rtlsim = platform/rtlsim
hal = platform
pci = platform/uiolinux
mmc = ../../inc/mmc
test = test/core
drvcore = drv/core
clk = drv/clock
terminal = utils/terminal
parser = utils/parser
inc = -I$(os) -I../../inc -I$(mmc) -I. -I$(build) -I$(config) -I$(mshc) -I$(core) -I$(platform) -I$(parser)

mshc:
	make -C $(hal)
	make -C $(platform)
	make -C $(rtlsim)
	make -C $(pci)
	make -C $(os)
	make -C $(terminal)
	make -C $(core)
	make -C $(drvcore)
	make -C $(clk)
	make -C $(parser)
	make -C $(test)
	make -C $(linux)
	make -C $(mshc)
	make -C main/mmc/dwc-mshc

main: $(objs)
	cc -o main $(objs)

clean:
	rm build/*.o

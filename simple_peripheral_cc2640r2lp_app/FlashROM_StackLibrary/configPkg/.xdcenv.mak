#
_XDCBUILDCOUNT = 
ifneq (,$(findstring path,$(_USEXDCENV_)))
override XDCPATH = C:/ti2/simplelink_cc2640r2_sdk_3_20_00_21/source;C:/ti2/simplelink_cc2640r2_sdk_3_20_00_21/kernel/tirtos/packages;C:/ti2/simplelink_cc2640r2_sdk_3_20_00_21/source/ti/blestack
override XDCROOT = C:/ti/xdctools_3_51_03_28_core
override XDCBUILDCFG = ./config.bld
endif
ifneq (,$(findstring args,$(_USEXDCENV_)))
override XDCARGS = 
override XDCTARGETS = 
endif
#
ifeq (0,1)
PKGPATH = C:/ti2/simplelink_cc2640r2_sdk_3_20_00_21/source;C:/ti2/simplelink_cc2640r2_sdk_3_20_00_21/kernel/tirtos/packages;C:/ti2/simplelink_cc2640r2_sdk_3_20_00_21/source/ti/blestack;C:/ti/xdctools_3_51_03_28_core/packages;..
HOSTOS = Windows
endif

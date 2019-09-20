################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Add inputs and outputs from these tool invocations to the build variables 
CFG_SRCS += \
../gpiointerrupt_CC2640R2_LAUNCHXL_tirtos_ccs.cfg 

CMD_SRCS += \
../CC2640R2_LAUNCHXL_TIRTOS.cmd 

C_SRCS += \
../CC2640R2_LAUNCHXL.c \
../CC2640R2_LAUNCHXL_fxns.c \
../ccfg.c \
../gpiointerrupt.c \
../main_tirtos.c 

GEN_CMDS += \
./configPkg/linker.cmd 

GEN_FILES += \
./configPkg/linker.cmd \
./configPkg/compiler.opt 

GEN_MISC_DIRS += \
./configPkg/ 

C_DEPS += \
./CC2640R2_LAUNCHXL.d \
./CC2640R2_LAUNCHXL_fxns.d \
./ccfg.d \
./gpiointerrupt.d \
./main_tirtos.d 

GEN_OPTS += \
./configPkg/compiler.opt 

OBJS += \
./CC2640R2_LAUNCHXL.obj \
./CC2640R2_LAUNCHXL_fxns.obj \
./ccfg.obj \
./gpiointerrupt.obj \
./main_tirtos.obj 

GEN_MISC_DIRS__QUOTED += \
"configPkg\" 

OBJS__QUOTED += \
"CC2640R2_LAUNCHXL.obj" \
"CC2640R2_LAUNCHXL_fxns.obj" \
"ccfg.obj" \
"gpiointerrupt.obj" \
"main_tirtos.obj" 

C_DEPS__QUOTED += \
"CC2640R2_LAUNCHXL.d" \
"CC2640R2_LAUNCHXL_fxns.d" \
"ccfg.d" \
"gpiointerrupt.d" \
"main_tirtos.d" 

GEN_FILES__QUOTED += \
"configPkg\linker.cmd" \
"configPkg\compiler.opt" 

C_SRCS__QUOTED += \
"../CC2640R2_LAUNCHXL.c" \
"../CC2640R2_LAUNCHXL_fxns.c" \
"../ccfg.c" \
"../gpiointerrupt.c" \
"../main_tirtos.c" 



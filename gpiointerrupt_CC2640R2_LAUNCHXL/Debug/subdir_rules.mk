################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: ARM Compiler'
	"C:/ti/ccsv8/tools/compiler/ti-cgt-arm_18.12.3.LTS/bin/armcl" -mv7M3 --code_state=16 --float_support=none -me --include_path="C:/Users/Vineet/Documents/gpiointerrupt_CC2640R2_LAUNCHXL" --include_path="C:/ti/simplelink_cc2640r2_sdk_3_20_00_21/source/ti/posix/ccs" --include_path="C:/ti/ccsv8/tools/compiler/ti-cgt-arm_18.12.3.LTS/include" --define=DeviceFamily_CC26X0R2 -g --diag_warning=225 --diag_warning=255 --diag_wrap=off --display_error_number --gen_func_subsections=on --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

build-847624431:
	@$(MAKE) --no-print-directory -Onone -f subdir_rules.mk build-847624431-inproc

build-847624431-inproc: ../gpiointerrupt_CC2640R2_LAUNCHXL_tirtos_ccs.cfg
	@echo 'Building file: "$<"'
	@echo 'Invoking: XDCtools'
	"C:/ti/ccs910/xdctools_3_55_02_22_core/xs" --xdcpath="C:/ti/simplelink_cc2640r2_sdk_3_20_00_21/source;C:/ti/simplelink_cc2640r2_sdk_3_20_00_21/kernel/tirtos/packages;" xdc.tools.configuro -o configPkg -t ti.targets.arm.elf.M3 -r debug -c "C:/ti/ccsv8/tools/compiler/ti-cgt-arm_18.12.3.LTS" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

configPkg/linker.cmd: build-847624431 ../gpiointerrupt_CC2640R2_LAUNCHXL_tirtos_ccs.cfg
configPkg/compiler.opt: build-847624431
configPkg/: build-847624431



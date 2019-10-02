################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: ARM Compiler'
	"C:/ti/ccs910/ccs/tools/compiler/ti-cgt-arm_18.12.2.LTS/bin/armcl" -mv7M3 --code_state=16 --float_support=none -me --include_path="C:/Users/Vineet/workspace_v9/empty_CC2640R2_LAUNCHXL_tirtos_ccs" --include_path="C:/ti/simplelink_cc2640r2_sdk_3_20_00_21/source/ti/posix/ccs" --include_path="C:/ti/ccs910/ccs/tools/compiler/ti-cgt-arm_18.12.2.LTS/include" --define=DeviceFamily_CC26X0R2 -g --diag_warning=225 --diag_warning=255 --diag_wrap=off --display_error_number --gen_func_subsections=on --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" --include_path="C:/Users/Vineet/workspace_v9/empty_CC2640R2_LAUNCHXL_tirtos_ccs/Debug/syscfg" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

build-1612039217:
	@$(MAKE) --no-print-directory -Onone -f subdir_rules.mk build-1612039217-inproc

build-1612039217-inproc: ../empty.syscfg
	@echo 'Building file: "$<"'
	@echo 'Invoking: SysConfig'
	"C:/ti/ccs910/ccs/utils/sysconfig/sysconfig_cli.bat" -o "syscfg" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

syscfg/summary.csv: build-1612039217 ../empty.syscfg
syscfg/: build-1612039217



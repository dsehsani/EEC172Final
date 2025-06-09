################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
utils/%.obj: ../utils/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"/Applications/TI/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=none -me -Ooff --include_path="/Applications/TI/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/include" --include_path="/Users/bud/Documents/EEC172/Lab/cc3200-support/cc3200sdk_1.5.0/cc3200-sdk/driverlib" --include_path="/Users/bud/Documents/EEC172/Lab/cc3200-support/cc3200sdk_1.5.0/cc3200-sdk/inc" --include_path="/Users/bud/Documents/EEC172/Lab/cc3200-support/cc3200sdk_1.5.0/cc3200-sdk/example/common" --include_path="/Users/bud/Documents/EEC172/Lab/cc3200-support/cc3200sdk_1.5.0/cc3200-sdk/simplelink/" --include_path="/Users/bud/Documents/EEC172/Lab/cc3200-support/cc3200sdk_1.5.0/cc3200-sdk/simplelink/source" --include_path="/Users/bud/Documents/EEC172/Lab/cc3200-support/cc3200sdk_1.5.0/cc3200-sdk/simplelink/include" --include_path="/Users/bud/Documents/EEC172/Lab/cc3200-support/cc3200sdk_1.5.0/cc3200-sdk/simplelink_extlib/provisioninglib" --define=ccs --define=cc3200 -g --gcc --printf_support=full --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="utils/$(basename $(<F)).d_raw" --obj_directory="utils" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '



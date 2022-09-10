include_guard()

include(cmake/Dependencies.cmake)

set(SHADER_MODEL "6_7")
set(SHADER_PREFIX src/gpu)
set(SHADER_SOURCES
	instanceList/genInstances.cs.hlsl
	instanceList/genIndices.cs.hlsl
	instanceList/cull.cs.hlsl
	visibility/worklist.cs.hlsl
	visibility/draw.vs.hlsl
	visibility/draw.ps.hlsl
	tonemap/apply.cs.hlsl
	imgui/imgui.vs.hlsl
	imgui/imgui.ps.hlsl
	bloom/down.cs.hlsl
	bloom/up.cs.hlsl
	sky/genMultiScattering.cs.hlsl
	sky/genTransmittance.cs.hlsl
	sky/genView.cs.hlsl
	sky/draw.cs.hlsl
	hiz/blit.cs.hlsl
	shadeFlat.cs.hlsl
	spd.cs.hlsl)

# Stage 1: Compile HLSL to SPIR-V
foreach(SHADER_SOURCE ${SHADER_SOURCES})
	set(SHADER_INPUT ${PROJECT_SOURCE_DIR}/${SHADER_PREFIX}/${SHADER_SOURCE})
	string(REGEX REPLACE [[.hlsl$]] "" SHADER_SOURCE_WLE ${SHADER_SOURCE})
	set(SHADER_OUTPUT ${PROJECT_BINARY_DIR}/generated/spv/${SHADER_SOURCE_WLE}.spv)
	get_filename_component(SHADER_OUTPUT_DIR ${SHADER_OUTPUT} DIRECTORY)
	
	get_filename_component(SHADER_TYPE ${SHADER_SOURCE_WLE} LAST_EXT)
	string(SUBSTRING ${SHADER_TYPE} 1 -1 SHADER_TYPE)
	set(SHADER_PROFILE ${SHADER_TYPE}_${SHADER_MODEL})
	
	add_custom_command(
		OUTPUT ${SHADER_OUTPUT}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIR}
		# Create HLSL depfile
		COMMAND $ENV{VULKAN_SDK}/Bin/dxc
			-HV 2021 -T ${SHADER_PROFILE} -spirv -fspv-target-env=vulkan1.3
			-I ${PROJECT_SOURCE_DIR}/${SHADER_PREFIX}
			-I ${spd_SOURCE_DIR}/ffx-spd
			-Fo ${SHADER_OUTPUT}
			-MF ${SHADER_OUTPUT}.d
			${SHADER_INPUT}
		# Compile HLSL into SPIR-V
		COMMAND $ENV{VULKAN_SDK}/Bin/dxc
			-HV 2021 -T ${SHADER_PROFILE} -spirv -fspv-target-env=vulkan1.3
			-O3 "$<$<CONFIG:Debug,RelWithDebInfo>:-Zi>" -fspv-reflect
			-I ${PROJECT_SOURCE_DIR}/${SHADER_PREFIX}
			-I ${spd_SOURCE_DIR}/ffx-spd
			-Fo ${SHADER_OUTPUT}
			${SHADER_INPUT}
		DEPFILE ${SHADER_OUTPUT}.d
		VERBATIM COMMAND_EXPAND_LISTS)
endforeach()

# Stage 2: Generate wrapper .c files to inline the SPIR-V
foreach(SHADER_SOURCE ${SHADER_SOURCES})
	string(REGEX REPLACE [[.hlsl$]] "" SHADER_SOURCE_WLE ${SHADER_SOURCE})
	set(SHADER_OUTPUT ${PROJECT_BINARY_DIR}/generated/spv/${SHADER_SOURCE_WLE}.spv)
	set(SHADER_WRAPPER ${SHADER_OUTPUT}.c)
	string(REGEX REPLACE [[\.|/]] "_" SHADER_IDENT ${SHADER_SOURCE_WLE})
	
	add_custom_command(
		OUTPUT ${SHADER_WRAPPER}
		COMMAND ${CMAKE_COMMAND} -E echo "#include \"incbin.h\""                            > ${SHADER_WRAPPER}
		COMMAND ${CMAKE_COMMAND} -E echo "INCBIN(${SHADER_IDENT}, \"${SHADER_OUTPUT}\")\;" >> ${SHADER_WRAPPER}
		DEPENDS ${SHADER_OUTPUT}
		VERBATIM COMMAND_EXPAND_LISTS)
	list(APPEND SHADER_WRAPPERS ${SHADER_WRAPPER})
endforeach()

# Stage 3: Merge all wrappers into a single source file
set(SHADER_WRAPPERS_COMBINED ${PROJECT_BINARY_DIR}/generated/spv/shaders.c)
add_custom_command(
	OUTPUT ${SHADER_WRAPPERS_COMBINED}
	COMMAND incbin-shell ${SHADER_WRAPPERS} -o ${SHADER_WRAPPERS_COMBINED} -p g_ -Ssnakecase
	DEPENDS incbin-shell
	DEPENDS ${SHADER_WRAPPERS}
	VERBATIM COMMAND_EXPAND_LISTS)

add_library(MinoteShaders STATIC ${SHADER_WRAPPERS_COMBINED})
target_link_libraries(MinoteShaders PRIVATE incbin)

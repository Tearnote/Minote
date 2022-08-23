include_guard()

include(cmake/Dependencies.cmake)

add_executable(Model_conv
	src/tools/modelSchema.hpp
	src/tools/modelConv.cpp)
target_link_options(Model_conv PRIVATE "/SUBSYSTEM:CONSOLE")
target_include_directories(Model_conv PRIVATE src)
target_link_libraries(Model_conv PRIVATE meshoptimizer)
target_link_libraries(Model_conv PRIVATE quill::quill)
target_link_libraries(Model_conv PRIVATE assert)
target_link_libraries(Model_conv PRIVATE itlib)
target_link_libraries(Model_conv PRIVATE cgltf)
target_link_libraries(Model_conv PRIVATE mpack)
target_link_libraries(Model_conv PRIVATE gcem)

set(ASSETS_FILENAME assets.db)
add_compile_definitions(ASSETS_P="${ASSETS_FILENAME}")
set(ASSET_OUTPUT ${PROJECT_BINARY_DIR}/${ASSETS_FILENAME})
add_custom_command(
	OUTPUT ${ASSET_OUTPUT}
	COMMAND sqlite-shell ${ASSET_OUTPUT} "VACUUM"
	VERBATIM)

set(MODELS_TABLE "models")
add_compile_definitions(MODELS_TABLE="${MODELS_TABLE}")
set(MODELS
	models/block.glb
	models/sphere.glb
	models/testscene.glb)
add_custom_command(
	OUTPUT ${ASSET_OUTPUT} APPEND
	COMMAND sqlite-shell ${ASSET_OUTPUT} "CREATE TABLE IF NOT EXISTS ${MODELS_TABLE}(name TEXT UNIQUE, data BLOB)"
	VERBATIM)
foreach(MODEL_PATH ${MODELS})
	get_filename_component(MODEL_NAME ${MODEL_PATH} NAME_WLE)
	add_custom_command(
		OUTPUT ${PROJECT_BINARY_DIR}/models/${MODEL_NAME}.model
		COMMAND Model_conv ${PROJECT_SOURCE_DIR}/${MODEL_PATH} ${PROJECT_BINARY_DIR}/models/${MODEL_NAME}.model
		DEPENDS ${PROJECT_SOURCE_DIR}/${MODEL_PATH}
		DEPENDS Model_conv
		VERBATIM COMMAND_EXPAND_LISTS)
	add_custom_command(
		OUTPUT ${ASSET_OUTPUT} APPEND
		COMMAND sqlite-shell ${ASSET_OUTPUT} "INSERT OR REPLACE INTO ${MODELS_TABLE}(name, data) VALUES('${MODEL_NAME}', readfile('${PROJECT_BINARY_DIR}/models/${MODEL_NAME}.model'))"
		DEPENDS ${PROJECT_BINARY_DIR}/models/${MODEL_NAME}.model
		DEPENDS sqlite-shell
		VERBATIM COMMAND_EXPAND_LISTS)
endforeach()

add_custom_target(MinoteAssets DEPENDS ${ASSET_OUTPUT})

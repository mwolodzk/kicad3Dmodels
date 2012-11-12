# file: ${K3D_SOURCE_DIR}/src/vrml/models/mfr_thru/CMakeLists.txt
#

include_directories(
    "${K3D_SOURCE_DIR}/include/common"
    "${K3D_SOURCE_DIR}/include/vrml/common"
    "${K3D_SOURCE_DIR}/include/vrml/models/resistor"
    )

link_directories(${K3D_BINARY_DIR}/src/common ${K3D_BINARY_DIR}/src/vrml/vcom)

# Tool for generating E192, E96, E48, E24, or E12 series ouput
# specification files for use by makeMFR
add_executable(makeEnnn makeEnnn.cpp colorcodes.cpp)

# Parametric model for tubular thru-hole resistors
add_executable(makeMFR makeMFR.cpp resistor.cpp)
target_link_libraries(makeMFR vcom)

install(TARGETS makeMFR makeEnnn RUNTIME DESTINATION bin)
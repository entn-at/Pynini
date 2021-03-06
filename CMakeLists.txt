cmake_minimum_required(VERSION 3.5.0)
project(hello)

set(DEPS_ROOT /home/pzelasko/spoken/vendors/Pynini/installs)
set(OPENFST_ROOT "${DEPS_ROOT}/openfst_build")
set(RE2_ROOT "${DEPS_ROOT}/re2")

include_directories(
	${OPENFST_ROOT}/include
	${RE2_ROOT}/include
	${PROJECT_SOURCE_DIR}/src
)

link_directories(
	${OPENFST_ROOT}/lib
	${RE2_ROOT}/lib
)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -O3 -march=native")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function -Wno-unused-local-typedef -funsigned-char")

add_executable(special_compose
	src/special_compose.cc
	src/wildcardcompose.h
	src/wildcardcomposescript.h
	src/wildcardcomposescript.cc
	src/special_arcs.cc)
target_link_libraries(special_compose
	re2
	fstfarscript
	fstpdtscript
	fstmpdtscript
	fstscript
	fstfar
	fst
	m
	dl
	)
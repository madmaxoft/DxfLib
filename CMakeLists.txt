cmake_minimum_required(VERSION 3.1)

project (DxfLib LANGUAGES CXX)

set (CMAKE_CXX_STANDARD 11)

set (SRCS
	Src/DxfDrawing.cpp
	Src/DxfParser.cpp
	Src/DxfWriter.cpp
)

set (HDRS
	Src/DxfDrawing.h
	Src/DxfParser.h
	Src/DxfWriter.h
)

add_library(DxfLib STATIC ${SRCS} ${HDRS})
target_include_directories(DxfLib INTERFACE Src)





# Tests:

enable_testing()

add_executable(DxfDrawingTest
	Tests/DxfDrawingTest.cpp
)
target_link_libraries(DxfDrawingTest DxfLib TestHelpers)

add_test(NAME DxfDrawingTest
	COMMAND DxfDrawingTest
)





add_executable(DxfParserTest
	Tests/DxfParserTest.cpp
)
target_link_libraries(DxfParserTest DxfLib TestHelpers)

add_test(NAME DxfParserTest
	COMMAND DxfParserTest
)





add_executable(DxfWriterTest
	Tests/DxfWriterTest.cpp
)
target_link_libraries(DxfWriterTest DxfLib TestHelpers)

add_test(NAME DxfWriterTest
	COMMAND DxfWriterTest
)
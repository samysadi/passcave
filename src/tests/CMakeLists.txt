# Config
# ----------------------------------------------------------------------
set(CMAKE_AUTOMOC ON)

enable_testing()

# Find needed libraries
# ----------------------------------------------------------------------
find_package(Qt5 COMPONENTS Test)

# etc
# ----------------------------------------------------------------------
add_executable(test_document "document.h" "document.cpp")
target_link_libraries(test_document passcave-lib Qt5::Test)
add_test(NAME test_document COMMAND test_document)

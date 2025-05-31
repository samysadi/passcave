# Config
# ----------------------------------------------------------------------
set(passcave-cli_HDRS "${PROJECT_BINARY_DIR}/config.h")
set(passcave-cli_SRCS passcave-cli.cpp)

# etc
# ----------------------------------------------------------------------
add_executable(passcave-cli ${passcave-cli_HDRS} ${passcave-cli_SRCS})
target_link_libraries(passcave-cli passcave-lib)

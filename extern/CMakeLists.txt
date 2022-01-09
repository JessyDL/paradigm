set(BUILD_TESTING FALSE)
add_subdirectory(cppcoro)
set_property(TARGET cppcoro PROPERTY FOLDER "extern")

add_subdirectory(fmt)
set_property(TARGET fmt PROPERTY FOLDER "extern")

option(SPDLOG_FMT_EXTERNAL "" ON)
set(spdlog_definitions "{ \"TRACE\", \"DEBUG\", \"INFO\", \"WARNING\", \"ERROR\", \"CRITICAL\", \"OFF\" }")
add_definitions(-DSPDLOG_LEVEL_NAMES=${spdlog_definitions})
add_subdirectory(spdlog)
set_property(TARGET spdlog PROPERTY FOLDER "extern")
remove_definitions(-DSPDLOG_LEVEL_NAMES)
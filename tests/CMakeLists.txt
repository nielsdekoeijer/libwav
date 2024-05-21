add_executable(tests main.cpp)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain Wav::Wav)

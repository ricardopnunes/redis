include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.3 # ou versão estável mais recente
)

FetchContent_MakeAvailable(Catch2)

add_executable(redis_tests
    test_main.cpp
    test_server.cpp
    test_database.cpp
)

target_link_libraries(redis_tests PRIVATE myredis Catch2::Catch2WithMain)
enable_sanitizers(redis_tests)

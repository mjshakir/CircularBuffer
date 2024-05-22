# toolchain-arm.cmake

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR ARM)

set(CMAKE_C_COMPILER "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.29.30037/bin/Hostx64/arm/cl.exe")
set(CMAKE_CXX_COMPILER "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.29.30037/bin/Hostx64/arm/cl.exe")

set(CMAKE_EXE_LINKER_FLAGS_INIT "/SAFESEH:NO")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "/SAFESEH:NO")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "/SAFESEH:NO")
set(CMAKE_C_FLAGS_INIT "/SAFESEH:NO")
set(CMAKE_CXX_FLAGS_INIT "/SAFESEH:NO")

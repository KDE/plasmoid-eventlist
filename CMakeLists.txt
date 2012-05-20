set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/modules )

# search packages used by KDE
find_package(KDE4 REQUIRED)
find_package(Akonadi REQUIRED)
find_package(KdepimLibs 4.6.0 REQUIRED)

include(KDE4Defaults)
include(MacroLibrary)
include(MacroOptionalAddSubdirectory)
include(FindKdepimLibs)

set(CMAKE_REQUIRED_DEFINITIONS ${_KDE4_PLATFORM_DEFINITIONS})
add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS} -DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)

if(HAS_OLD_PIM)
    add_definitions(-DHAS_OLD_PIM)
    message("-- Compiling for older KDE PIM 4.4")
endif(HAS_OLD_PIM)

include_directories(${QDBUS_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${KDE4_INCLUDES}
    ${AKONADI_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/akonadi
    ${CMAKE_SOURCE_DIR}/libkdepim
    ${KDEPIMLIBS_INCLUDE_DIRS}
)

add_subdirectory(src)
if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/po)
    message("-- Found lang files")
    add_subdirectory(po)
endif(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/po)

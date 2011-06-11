set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/modules )

# search packages used by KDE
find_package(KDE4 REQUIRED)
find_package(Akonadi REQUIRED)
find_package(KdepimLibs 4.6.0 REQUIRED)

include(KDE4Defaults)
include(MacroLibrary)
include(MacroOptionalAddSubdirectory)
include(FindKdepimLibs)

SET (CMAKE_REQUIRED_DEFINITIONS ${_KDE4_PLATFORM_DEFINITIONS})
ADD_DEFINITIONS (${QT_DEFINITIONS} ${KDE4_DEFINITIONS} -DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)

IF (HAS_OLD_PIM)
    ADD_DEFINITIONS (-DHAS_OLD_PIM)
    MESSAGE ("-- Compiling for older KDE PIM 4.4")
ENDIF (HAS_OLD_PIM)

INCLUDE_DIRECTORIES (${QDBUS_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${KDE4_INCLUDES}
    ${AKONADI_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/akonadi
    ${CMAKE_SOURCE_DIR}/libkdepim
    ${KDEPIMLIBS_INCLUDE_DIRS}
)

add_subdirectory(src)


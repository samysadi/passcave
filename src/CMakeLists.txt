cmake_minimum_required(VERSION 3.1)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(passcave)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# search for compiler-provided system runtime libraries and add install rules for them
include(InstallRequiredSystemLibraries)

# defs
string(TIMESTAMP passcave_COMPILE_TIME "%Y%m%d%H%M%S")
set(passcave_ROOT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(passcave_DATA_PATH "${passcave_ROOT_PATH}/data")
set(passcave_VERSION_MAJOR 0)
set(passcave_VERSION_MINOR 9)
set(passcave_AUTHOR "Samy Sadi")
set(passcave_AUTHOR_MAIL "samy.sadi.contact@gmail.com")
set(passcave_APPLICATION_NAME "Passcave")
set(passcave_APPLICATION_NAME_NORMALIZED "passcave")
set(passcave_APPLICATION_DESCRIPTION "A password manager and encrypter.")
set(passcave_APPLICATION_EXECUTABLE "passcave-gui")
set(passcave_APPLICATION_EXECUTABLE_NAME "Passcave Gui")
set(passcave_ORGANIZATION "Passcave")
set(passcave_DOMAIN "passcave.samysadi.com")
set(passcave_DOMAIN_REV "com.samysadi.passcave")
set(passcave_COPYRIGHT_YEAR "2016-2025")
set(passcave_LICENSE_DES "GPL3")

# Setup cpack
# ----------------------------------------------------------------------
set(passcave_CPACK_PATH "${passcave_ROOT_PATH}/cpack")

SET(CPACK_PACKAGE_NAME "${passcave_APPLICATION_NAME_NORMALIZED}")
SET(CPACK_PACKAGE_VENDOR "${passcave_AUTHOR}")
SET(CPACK_PACKAGE_CONTACT "${passcave_AUTHOR_MAIL}")
SET(CPACK_PACKAGE_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
#SET(CPACK_PACKAGE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/packages")
SET(CPACK_PACKAGE_VERSION_MAJOR "${passcave_VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${passcave_VERSION_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH "0")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${passcave_CPACK_PATH}/DESCRIPTION")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${passcave_APPLICATION_DESCRIPTION}")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "${passcave_APPLICATION_NAME_NORMALIZED}-${passcave_VERSION_MAJOR}.${passcave_VERSION_MINOR}")
SET(CPACK_PACKAGE_ICON "${passcave_DATA_PATH}/icon-64x64.png")
SET(CPACK_RESOURCE_FILE_LICENSE "${passcave_ROOT_PATH}/LICENSE")
SET(CPACK_RESOURCE_FILE_README "${passcave_CPACK_PATH}/README")
SET(CPACK_RESOURCE_FILE_WELCOME "${passcave_CPACK_PATH}/WELCOME")
SET(CPACK_MONOLITHIC_INSTALL 1)
SET(CPACK_PACKAGE_EXECUTABLES "${passcave_APPLICATION_EXECUTABLE};${passcave_APPLICATION_EXECUTABLE_NAME}")
SET(CPACK_STRIP_FILES 1)

# cpack - BUNDLE
# ----------------------------------------------------------------------
SET(CPACK_BUNDLE_NAME "${CPACK_PACKAGE_NAME}")
configure_file(
	"${passcave_ROOT_PATH}/cpack/Info.plist.in"
	"${CMAKE_CURRENT_BINARY_DIR}/Info.plist"
	@ONLY
)
SET(CPACK_BUNDLE_PLIST "${CMAKE_CURRENT_BINARY_DIR}/Info.plist")
SET(CPACK_BUNDLE_ICON "${passcave_DATA_PATH}/icon.icns")
SET(CPACK_BUNDLE_STARTUP_COMMAND "${passcave_APPLICATION_EXECUTABLE}")

# cpack - Debian
# ----------------------------------------------------------------------
SET(CPACK_DEBIAN_PACKAGE_NAME "${CPACK_PACKAGE_NAME}")
SET(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
if("${CPACK_PACKAGE_ARCHITECTURE}" MATCHES x86_64)
SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
else()
SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${CPACK_PACKAGE_ARCHITECTURE}")
endif()
SET(CPACK_DEBIAN_PACKAGE_RELEASE "${passcave_COMPILE_TIME}")
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libgcrypt20 (>= 1.6.0), libqt5widgets5, libqt5test5")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
SET(CPACK_DEBIAN_PACKAGE_SECTION "utils")
SET(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://${passcave_DOMAIN}")
SET(CPACK_DEBIAN_PACKAGE_PROVIDES "${CPACK_PACKAGE_NAME}")

# cpack - NSIS
# ----------------------------------------------------------------------
#IF(WIN32 AND NOT UNIX)
SET(CPACK_NSIS_MUI_ICON "${passcave_DATA_PATH}/icon-install-48x48.ico")
SET(CPACK_NSIS_MUI_UNIICON "${passcave_DATA_PATH}/icon-uninstall-48x48.ico")
SET(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL "ON")
SET(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_NAME}")
SET(CPACK_NSIS_PACKAGE_NAME "${CPACK_PACKAGE_NAME}")
SET(CPACK_NSIS_HELP_LINK "http://${passcave_DOMAIN}")
SET(CPACK_NSIS_URL_INFO_ABOUT "http://${passcave_DOMAIN}")
SET(CPACK_NSIS_CONTACT "${CPACK_PACKAGE_CONTACT}")
#ELSE(WIN32 AND NOT UNIX)
#ENDIF(WIN32 AND NOT UNIX)

# cpack - RPM
# ----------------------------------------------------------------------
SET(CPACK_RPM_PACKAGE_NAME "${CPACK_PACKAGE_NAME}")
SET(CPACK_RPM_FILE_NAME "RPM-DEFAULT")
SET(CPACK_RPM_PACKAGE_ARCHITECTURE "${CPACK_PACKAGE_ARCHITECTURE}")
SET(CPACK_RPM_PACKAGE_RELEASE "${passcave_COMPILE_TIME}")
SET(CPACK_RPM_PACKAGE_GROUP "Applications/Editors")
SET(CPACK_RPM_PACKAGE_LICENSE "${passcave_LICENSE_DES}")
SET(CPACK_RPM_PACKAGE_URL "http://${passcave_DOMAIN}")
SET(CPACK_RPM_PACKAGE_REQUIRES "libgcrypt >= 1.6.0, qt5 >= 5.0.0")
SET(CPACK_RPM_PACKAGE_PROVIDES "${CPACK_PACKAGE_NAME}")

# other packages - archlinux
# ----------------------------------------------------------------------
if("${CPACK_PACKAGE_ARCHITECTURE}" MATCHES i386)
SET(CPACK_MAKEPKG_PACKAGE_ARCHITECTURE "i686")
else()
SET(CPACK_MAKEPKG_PACKAGE_ARCHITECTURE "${CPACK_PACKAGE_ARCHITECTURE}")
endif()
configure_file(
	"${passcave_ROOT_PATH}/cpack/PKGBUILD.in"
	"${CMAKE_CURRENT_BINARY_DIR}/PKGBUILD"
	@ONLY
)

# enabling cpack
# ----------------------------------------------------------------------
#SET(CPACK_GENERATOR "Bundle;DEB;IFW;NSIS;NSIS64;RPM;SH;STGZ;TGZ;TZ;WIX;ZIP")
include(CPack)

IF(UNIX)
	SET(CMAKE_INSTALL_PREFIX "/usr")
ENDIF(UNIX)

# Setup configuration header file
# ----------------------------------------------------------------------

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/includes/config.h.in"
	"${PROJECT_BINARY_DIR}/config.h"
	@ONLY
)

# Setup source / header files
# ----------------------------------------------------------------------

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/includes)
include_directories(${PROJECT_BINARY_DIR})

set(passcave_HDRS "${PROJECT_BINARY_DIR}/config.h" "includes/exceptions.h" "includes/utils.h" "includes/gcry.h" "includes/document.h" "includes/utils2fa.h")
set(passcave_SRCS "includes/utils.cpp" "includes/gcry.cpp" "includes/document.cpp" "includes/utils2fa.cpp")

# Setup libraries / packages
# ----------------------------------------------------------------------

#qt
find_package(Qt5 COMPONENTS Core)
set(passcave_LIBS ${passcave_LIBS} Qt5::Core)

# gcrypt
find_library(libgcrypt_LIB gcrypt)

if(libgcrypt_LIB)
	set(passcave_LIBS ${passcave_LIBS} ${libgcrypt_LIB})
	message(STATUS "Found libgcrypt: "  ${libgcrypt_LIB})
else()
	message(FATAL_ERROR "libgcrypt not found")
endif()

# etc
# ----------------------------------------------------------------------
add_library(passcave-lib ${passcave_HDRS} ${passcave_SRCS})
target_link_libraries(passcave-lib ${passcave_LIBS})

# cli
add_subdirectory(cli)

# gui
add_subdirectory(gui)

## converters
#if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/converters")
#	add_subdirectory(converters)
#endif()

# tests
add_subdirectory(tests)


# installation / uninstallation
# ----------------------------------------------------------------------
SET(passcave_default_permissions0 OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
SET(passcave_default_permissions_ex OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

if(UNIX AND ("${CMAKE_SYSTEM_NAME}" MATCHES "Linux"))
	# .desktop file for passcave-gui
	configure_file(
		"${passcave_ROOT_PATH}/cpack/passcave-gui.desktop.in"
		"${CMAKE_CURRENT_BINARY_DIR}/passcave-gui.desktop"
		@ONLY
	)
	INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/passcave-gui.desktop"
			DESTINATION "${DESTDIR}/usr/share/applications"
			PERMISSIONS ${passcave_default_permissions0}
			RENAME "passcave-gui.desktop")

	# mime file
	configure_file(
		"${passcave_ROOT_PATH}/cpack/mime/passcave.xml.in"
		"${CMAKE_CURRENT_BINARY_DIR}/mime/passcave.xml"
		@ONLY
	)
	INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/mime/passcave.xml"
			DESTINATION "${DESTDIR}/usr/share/mime/packages"
			PERMISSIONS ${passcave_default_permissions0}
			RENAME "passcave.xml")

	# icons
	set(passcave_SYSTEM_ICON_PATH "${DESTDIR}/usr/share/icons/hicolor")
	foreach(i 16 32 64 128)
		INSTALL(FILES "${passcave_ROOT_PATH}/data/icon-${i}x${i}.png"
				DESTINATION "${passcave_SYSTEM_ICON_PATH}/${i}x${i}/apps"
				PERMISSIONS ${passcave_default_permissions0}
				RENAME "passcave.png")
		INSTALL(FILES "${passcave_ROOT_PATH}/data/icon-document-${i}x${i}.png"
				DESTINATION "${passcave_SYSTEM_ICON_PATH}/${i}x${i}/mimetypes"
				PERMISSIONS ${passcave_default_permissions0}
				RENAME "passcave-document.png")
	endforeach()
	# TODO call xdg-desktop-menu xdg-mime and xdg-icon-resource to update caches
endif()

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

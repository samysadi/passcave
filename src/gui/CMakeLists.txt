# Config
# ----------------------------------------------------------------------
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(passcave-gui_LANGS "translations/french.ts" "translations/english.ts")
set(passcave-gui_LANG_QRC "translations/lang.qrc")
set(passcave-gui_LANG_QRC_BIN "lang.qrc")
set(passcave-gui_HDRS "${PROJECT_BINARY_DIR}/config.h" "includes/runguard.h" "includes/datamodel.h" "includes/preferences.h" "forms/preferencesdialog.h" "forms/passworddialog.h" "forms/propertiesdialog.cpp" "forms/aboutdialog.h" "forms/longtextdialog.h" "forms/clickableqlabel.h" "forms/addnewnodedialog.h" "forms/loadingdialog.h" "passcave-gui.h")
set(passcave-gui_SRCS "includes/runguard.cpp" "includes/datamodel.cpp" "includes/preferences.cpp" "forms/preferencesdialog.cpp" "forms/passworddialog.cpp" "forms/propertiesdialog.cpp" "forms/aboutdialog.cpp" "forms/longtextdialog.cpp" "forms/clickableqlabel.cpp" "forms/addnewnodedialog.cpp" "forms/loadingdialog.cpp" "passcave-gui.cpp")
set(passcave-gui_UIS  "forms/passworddialog.ui" "forms/propertiesdialog.ui" "forms/aboutdialog.ui" "forms/longtextdialog.ui" "forms/loadingdialog.ui" "passcave-gui.ui")
set(passcave-gui_QRCS "passcave-gui.qrc")

# Find needed libraries
# ----------------------------------------------------------------------
find_package(Qt5 COMPONENTS Widgets LinguistTools)

# LOCALIZATION
# ----------------------------------------------------------------------
configure_file(${passcave-gui_LANG_QRC} ${passcave-gui_LANG_QRC_BIN} COPYONLY)
QT5_ADD_TRANSLATION(passcave-gui_QM_Files ${passcave-gui_LANGS})
QT5_ADD_RESOURCES(passcave-gui_QM_RC "${CMAKE_CURRENT_BINARY_DIR}/${passcave-gui_LANG_QRC_BIN}")

# etc
# ----------------------------------------------------------------------

add_executable(passcave-gui ${passcave-gui_HDRS} ${passcave-gui_SRCS} ${passcave-gui_UIS} ${passcave-gui_QRCS} ${passcave-gui_QM_Files} "${passcave-gui_QM_RC}" "passcave-gui.cpp")
target_link_libraries(passcave-gui passcave-lib Qt5::Widgets)

install(TARGETS passcave-gui
		RUNTIME DESTINATION bin
				PERMISSIONS ${passcave_default_permissions_ex})

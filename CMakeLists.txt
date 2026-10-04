cmake_minimum_required(VERSION 3.19)
project(feedr LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Threads REQUIRED)

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(LIBZIM IMPORTED_TARGET libzim)
endif()

qt_standard_project_setup()

qt_add_executable(feedr
    WIN32 MACOSX_BUNDLE
    main.cpp
    htmlarea.cpp
    htmlarea.h
    library.cpp
    library.h
    homepage.cpp
    homepage.h
    readerpage.cpp
    readerpage.h
    settingsdialog.cpp
    settingsdialog.h
    mainwindow.cpp
    mainwindow.h
)

target_link_libraries(feedr
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Threads::Threads
)

if(TARGET PkgConfig::LIBZIM)
    target_link_libraries(feedr PRIVATE PkgConfig::LIBZIM)
else()
    target_link_libraries(feedr PRIVATE zim)
endif()

target_compile_options(feedr PRIVATE
    -pipe
    -O2
    -Wall
    -Wextra
    -D_REENTRANT
)

target_compile_definitions(feedr PRIVATE
    QT_NO_DEBUG
)

set_target_properties(feedr PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

include(GNUInstallDirs)
install(TARGETS feedr
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET feedr
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

cmake_minimum_required( VERSION 2.8.11 )
set (objectName qt-1090)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -g")
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "/usr/local/bin" CACHE PATH "default install path" FORCE )
endif()
#set (CMAKE_INSTALL_PREFIX /usr/local/bin)

if(MINGW)
    add_definitions ( -municode)
endif()

IF(EXISTS (".git") AND IS_DIRECTORY (".git"))
   execute_process (
      COMMAND git rev-parse --short HEAD
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE GIT_COMMIT_HASH
      OUTPUT_STRIP_TRAILING_WHITESPACE
   )
endif ()

if(GIT_COMMIT_HASH)
   add_definitions("-DGITHASH=\"${GIT_COMMIT_HASH}\"")
else ()
   add_definitions ("-DGITHASH=\"       \"")
endif()

########################################################################
# select the release build type by default to get optimization flags
########################################################################
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
   message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "")

### make sure our local CMake Modules path comes first
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake/Modules)

######################################################################
if(DEFINED HACKRF)
   set(HACKRF true)
endif ()

if(DEFINED SDRPLAY)
   set(SDRPLAY true)
endif ()

if(DEFINED RTLSDR)
   set(RTLSDR true)
endif ()

if(DEFINED RTLTCP)
   set(RTLTCP true)
endif ()

#########################################################################
        find_package (PkgConfig)
	find_package (LSBId)

        find_package (Qt5Core REQUIRED)
        find_package (Qt5Widgets REQUIRED)
        find_package (Qt5Network REQUIRED)
        include_directories (
          ${Qt5Network_INCLUDE_DIRS}
        )

	find_package(Qwt)
	if (NOT QWT_FOUND)
	   message(FATAL_ERROR "please install qwt library")
	endif ()
	include_directories ( ${QWT_INCLUDE_DIRS})
	list(APPEND extraLibs ${QWT_LIBRARIES})


##########################################################################
#	The devices

	if (HACKRF)
	   set (${objectName}_UIS
	        ${${objectName}_UIS}
	        ./devices/hackrf-handler/hackrf-widget.ui
	   )

	   set (${objectName}_MOCS
	        ${${objectName}_MOCS}
	        ./devices/hackrf-handler/hackrf-handler.h
	   )

	   include_directories (
	     ./devices/hackrf-handler
	     ./devices/hackrf-handler/libhackrf
	   )

	   set ($(objectName)_HDRS
	        ${${objectName}_HDRS}
	        ./devices/hackrf-handler/hackrf-handler.h
	        ./devices/hackrf-handler/libhackrf/hackrf.h
	   )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./devices/hackrf-handler/hackrf-handler.cpp
	   )

	   add_definitions (-D__HAVE_HACKRF__)
	endif (HACKRF)

	if (SDRPLAY)
#	   find_path (SDRPLAYLIB_INCLUDE_DIR
#                      NAMES mirsdrapi-rsp.h
#                      PATHS
#                      /usr/local/include/
#	   )
#	   include_directories (${SDRPLAYLIB_INCLUDE_DIR})
	   set (${objectName}_UIS
	        ${${objectName}_UIS}
	        ./devices/sdrplay-handler/sdrplay-widget.ui
	   )

	   set (${objectName}_MOCS
	        ${${objectName}_MOCS}
	        ./devices/sdrplay-handler/sdrplay-handler.h
	   )

	   include_directories (
	     ./devices/sdrplay-handler
	   )

	   set ($(objectName)_HDRS
	        ${${objectName}_HDRS}
	        ./devices/sdrplay-handler/sdrplay-handler.h
	        ./devices/sdrplay-handler/mirsdrapi-rsp.h
           )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./devices/sdrplay-handler/sdrplay-handler.cpp
	   )

	 add_definitions (-D__HAVE_SDRPLAY__)
	endif (SDRPLAY)

	if (RTLTCP)
	   set (${objectName}_UIS
	        ${${objectName}_UIS}
	        ./devices/rtltcp-handler/rtl_tcp-widget.ui
	   )

	   set (${objectName}_MOCS
	        ${${objectName}_MOCS}
	        ./devices/rtltcp-handler/rtl_tcp-handler.h
	   )

	   include_directories (
	     ./devices/rtltcp-handler
	   )

	   set ($(objectName)_HDRS
	        ${${objectName}_HDRS}
	        ./devices/rtltcp-handler/rtl_tcp-handler.h
           )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./devices/rtltcp-handler/rtl_tcp-handler.cpp
	   )

	 add_definitions (-D__HAVE_RTLTCP__)
	endif (RTLTCP)

	if (RTLSDR)
#	   find_package(LibRTLSDR)
#	   if (NOT LIBRTLSDR_FOUND)
#	      message(FATAL_ERROR "please install librtlsdr")
#	   endif ()
#	   ###include_directories (${RTLSDR_INCLUDE_DIR})

	   set (${objectName}_UIS
	        ${${objectName}_UIS}
	        ./devices/rtlsdr-handler/rtlsdr-widget.ui
	   )

	   set (${objectName}_MOCS
	        ${${objectName}_MOCS}
	        ./devices/rtlsdr-handler/rtl-dongleselect.h
	        ./devices/rtlsdr-handler/rtlsdr-handler.h
	   )

	   include_directories (
	        ./devices/rtlsdr-handler/
	   )

	   set (${objectName}_HDRS
	        ${${objectName}_HDRS}
	        ./devices/rtlsdr-handler/rtlsdr-handler.h
	        ./devices/rtlsdr-handler/rtl-dongleselect.h
	   )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./devices/rtlsdr-handler/rtlsdr-handler.cpp
	        ./devices/rtlsdr-handler/rtl-dongleselect.cpp
	   )

	   add_definitions (-D__HAVE_RTLSDR__)
	endif()
######################################################################
	find_library (PTHREADS pthread)
	if (NOT(PTHREADS))
	   message (FATAL_ERROR "please install libpthread")
	else (NOT(PTHREADS))
	   set (extraLibs ${extraLibs} ${PTHREADS})
	endif (NOT(PTHREADS))

#######################################################################
#

	include_directories (
	   ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
	   ./
	)
#
	include_directories (
	   ./devices/file-handler
        )

	set (${objectName}_HDRS
	     ${${objectName}_HDRS}
	     ./xclose.h
	     ./qt-1090.h
	     ./ringbuffer.h
	     ./adsb-constants.h
	     ./aircraft-handler.h
	     ./icao-cache.h
	     ./crc-handling.h
	     ./message-handling.h
	     ./responder.h
	     ./device-handler.h
	     ./devices/file-handler/file-handler.h
	     ./syncviewer.h
	)

	set (${objectName}_SRCS
	     ${${objectName}_SRCS}
	     ./xclose.cpp
	     ./main.cpp
	     ./qt-1090.cpp
	     ./aircraft-handler.cpp
	     ./icao-cache.cpp
	     ./crc-handling.cpp
	     ./message-handling.cpp
	     ./responder.cpp
	     ./device-handler.cpp
	     ./devices/file-handler/file-handler.cpp
	     ./syncviewer.cpp
	)

#	QT5_WRAP_UI (UIS ${${objectName}_UIS})

        include_directories (
                  ${QT5Widgets_INCLUDES}
                  ${QT_QTCORE_INCLUDE_DIR}
                  ${QT_QTGUI_INCLUDE_DIR}
        )

        QT5_WRAP_CPP (MOCS ${${objectName}_MOCS}
                     ./qt-1090.h
	             ./device-handler.h
	             ./responder.h
	)

	QT5_WRAP_UI (UIS ${${objectName}_UIS}
                     ./qt-1090.ui)

	add_executable (${objectName}
	                ${${objectName}_SRCS}
                        ${UIS}
                        ${RSCS}
                        ${TRS}
                        ${MOCS}
	)

	target_link_libraries (${objectName}
	                       Qt5::Widgets
                               Qt5::Network
                               ${extraLibs}
	                       -lm -ldl -lrtlsdr -lqhttpserver
	)

	INSTALL (TARGETS ${objectName} DESTINATION .)

########################################################################
# Create uninstall target
########################################################################

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)


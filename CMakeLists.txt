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
if(DEFINED AIRSPY)
   set(AIRSPY true)
endif ()

if(DEFINED SDRPLAY)
   set(SDRPLAY true)
endif ()

if(DEFINED RTLSDR)
   set(RTLSDR true)
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

	if (AIRSPY)
	   find_path (AIRSPYLIB_INCLUDE_DIR
                      NAMES airspy.h
                      PATHS
                      /usr/local/include/
                      /usr/local/include/libairspy
           )
           include_directories (${AIRSPYLIB_INCLUDE_DIR})
           find_library (AIRSPYLIB airspy)
           if(NOT(AIRSPYLIB))
              message (FATAL_ERROR "please install library for airspy")
           else(NOT(AIRSPYLIB))
              list (APPEND extraLibs ${AIRSPYLIB})
           endif ()

	   set (${objectName}_UIS
	        ${${objectName}_UIS}
	        ./devices/airspy-handler/airspy-widget.ui
	   )

	   set (${objectName}_MOCS
	        ${${objectName}_MOCS}
	        ./devices/airspy-handler/airspy-handler.h
	   )

	   include_directories (
	     ./devices/airspy-handler
	     ./devices/airspy-handler/libairspy
	   )

	   set ($(objectName)_HDRS
	        ${${objectName}_HDRS}
	        ./devices/airspy-handler/airspy-handler.h
	        ./devices/airspy-handler/libairspy/airspy.h
	   )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./devices/airspy-handler/airspy-handler.cpp
	   )

	   add_definitions (-D__HAVE_AIRSPY__)
	endif (AIRSPY)

	if (SDRPLAY)
	   find_path (SDRPLAYLIB_INCLUDE_DIR
                      NAMES mirsdrapi-rsp.h
                      PATHS
                      /usr/local/include/
	   )
           include_directories (${SDRPLAYLIB_INCLUDE_DIR})

           find_library (SDRPLAYLIB mirsdrapi-rsp)
           if(NOT(SDRPLAYLIB))
              message(FATAL_ERROR "please install -lmirsdrapi-rsp")
           else(NOT(SDRPLAYLIB))
              list (APPEND extraLibs ${SDRPLAYLIB})
           endif(NOT(SDRPLAYLIB))

	   set (${objectName}_UIS
                ${${objectName}_UIS}
                ./devices/sdrplay-handler/sdrplay-widget.ui
           )

           set (${objectName}_MOCS
                ${${objectName}_MOCS}
                ./devices/sdrplay-handler/sdrplay-handler.h
           )

	   set ($(objectName)_HDRS
                ${${objectName}_HDRS}
                ./devices/sdrplay-handler/sdrplay-handler.h
           )

	   include_directories (
             ./devices/sdrplay-handler
           )

           set (${objectName}_SRCS
                ${${objectName}_SRCS}
                ./devices/sdrplay-handler/sdrplay-handler.cpp
           )

	   set (${objectName}_MOCS
	        ${${objectName}_MOCS} 
	        ./devices/sdrplay-handler/sdrplay-handler.h
	   )

	   add_definitions (-D__HAVE_SDRPLAY__)
	endif (SDRPLAY)

	if (RTLSDR)
           find_package(LibRTLSDR)
           if (NOT LIBRTLSDR_FOUND)
               message(FATAL_ERROR "please install librtlsdr")
           endif ()
	   list (APPEND extraLibs ${LibRTLSDRLIB})
	   ###include_directories (${RTLSDR_INCLUDE_DIR})

	   set (${objectName}_UIS
                ${${objectName}_UIS}
                ./devices/rtlsdr-handler/rtlsdr-widget.ui
           )

           set (${objectName}_MOCS
                ${${objectName}_MOCS}
                ./devices/rtlsdr-handler/rtl-dongleselect.h
                ./devices/rtlsdr-handler/rtlsdr-handler.h
           )

	   set (${objectName}_HDRS
                ${${objectName}_HDRS}
                ./devices/rtlsdr-handler/rtlsdr-handler.h
                ./devices/rtlsdr-handler/rtl-dongleselect.h
	        
           )

	   include_directories (
             ./devices/rtlsdr-handler
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
	     ./device-handler.h
	     ./devices/file-handler/file-handler.h
	     ./anet.h
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
	     ./device-handler.cpp
	     ./devices/file-handler/file-handler.cpp
	     ./anet.cpp
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
	                       -lm -ldl -lrtlsdr
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


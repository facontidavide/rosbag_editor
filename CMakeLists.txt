cmake_minimum_required(VERSION 2.8.3)
project(rosbag_editor)


add_compile_options(-std=c++11)

find_package(catkin REQUIRED COMPONENTS
  rosbag
  rosbag_storage
  roscpp
  tf2_msgs
  tf
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
SET(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

catkin_package(
#  INCLUDE_DIRS include
#  LIBRARIES rosbag_editor
  CATKIN_DEPENDS rosbag rosbag_storage roscpp tf2_msgs tf
#  DEPENDS system_lib
)

###########
## Build ##
###########

## Specify additional locations of header files
## Your package locations should be listed before other locations
include_directories(
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Gui_INCLUDE_DIRS}
    ${Qt5Widgets_INCLUDE_DIRS}
    ${catkin_INCLUDE_DIRS}
    )


## Declare a C++ executable
## With catkin_make all packages are built within a single CMake context
## The recommended prefix ensures that target names across packages don't collide
add_executable(rosbag_editor
    src/main.cpp
    src/rosbag_editor.cpp
    src/rosbag_editor.ui
    src/filter_frames.cpp
    src/filter_frames.ui
    desktop/resources.qrc
    )

## Add cmake target dependencies of the executable
## same as for the library above
add_dependencies(rosbag_editor ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})

## Specify libraries to link a library or executable target against
 target_link_libraries(rosbag_editor
   ${catkin_LIBRARIES}
   Qt5::Core
   Qt5::Widgets
 )


#############
## Install ##
#############

if("${CMAKE_INSTALL_PREFIX}" MATCHES "^/opt/")
  # Handle release case
  set(DESKTOP_APP_DIR "/usr/share/applications")
  set(DESKTOP_ICON_DIR "/usr/share/icons")
else()
  set(DESKTOP_APP_DIR "$ENV{HOME}/.local/share/applications")
  set(DESKTOP_ICON_DIR ${DESKTOP_APP_DIR})
  set(DESKTOP_APP_SUFFIX " (local)")
endif()
set(ROS_DISTRO $ENV{ROS_DISTRO})

# Create directory for the desktop file/icon
install(DIRECTORY DESTINATION
  ${DESKTOP_APP_DIR}
)
install(DIRECTORY DESTINATION
  ${DESKTOP_ICON_DIR}
)

# Configure / install bash launch script
configure_file(
  scripts/${PROJECT_NAME}.bash.in
  scripts/${PROJECT_NAME}.bash
  @ONLY
)

# Install bash script
install(
  PROGRAMS
  ${CMAKE_CURRENT_BINARY_DIR}/scripts/${PROJECT_NAME}.bash
  DESTINATION
  ${CATKIN_PACKAGE_BIN_DESTINATION}
)

# Configure / install desktop launchers
if(DEFINED DESKTOP_APP_SUFFIX)
  configure_file(
    desktop/${PROJECT_NAME}.desktop.in
    desktop/${PROJECT_NAME}_debug.desktop
  )
else()
  configure_file(
    desktop/${PROJECT_NAME}.desktop.in
    desktop/${PROJECT_NAME}.desktop
  )
endif()
if(DEFINED DESKTOP_APP_SUFFIX)
  install(
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/desktop/${PROJECT_NAME}_debug.desktop
    DESTINATION
    ${DESKTOP_APP_DIR}
  )
else()
  install(
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/desktop/${PROJECT_NAME}.desktop
    DESTINATION
    ${DESKTOP_APP_DIR}
  )
endif()

# Install icon
install(
  FILES
  desktop/${PROJECT_NAME}.png
  DESTINATION
  ${DESKTOP_ICON_DIR}
)

## Mark executables and/or libraries for installation
install(TARGETS rosbag_editor rosbag_editor
   ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
   LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
   RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)

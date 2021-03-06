cmake_minimum_required(VERSION 2.8.3)
project(csapex_scan_2d)

# Set the build type.  Options are:
#  Coverage       : w/ debug symbols, w/o optimization, w/ code-coverage
#  Debug          : w/ debug symbols, w/o optimization
#  Release        : w/o debug symbols, w/ optimization
#  RelWithDebInfo : w/ debug symbols, w/ optimization
#  MinSizeRel     : w/o debug symbols, w/ optimization, stripped binaries
#set(ROS_BUILD_TYPE RelWithDebInfo)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

## Find catkin macros and libraries
## if COMPONENTS list like find_package(catkin REQUIRED COMPONENTS xyz)
## is used, also find other catkin packages
find_package(catkin REQUIRED COMPONENTS csapex csapex_math csapex_ros csapex_opencv cslibs_laser_processing)

find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)
set(CMAKE_AUTOMOC ON)

find_package(Boost REQUIRED)

###################################
## catkin specific configuration ##
###################################
## The catkin_package macro generates cmake config files for your package
## Declare things to be passed to dependent projects
## INCLUDE_DIRS: uncomment this if you package contains header files
## LIBRARIES: libraries you create in this project that dependent projects also need
## CATKIN_DEPENDS: catkin_packages dependent projects also need
## DEPENDS: system dependencies of this project that dependent projects also need
catkin_package(
  INCLUDE_DIRS include
  LIBRARIES ${PROJECT_NAME}
  CATKIN_DEPENDS cslibs_laser_processing
#  DEPENDS system_lib
)

###########
## Build ##
###########

## Specify additional locations of header files
## Your package locations should be listed before other locations
# include_directories(include)
include_directories(include
  ${catkin_INCLUDE_DIRS}
  ${Qt5Core_INCLUDE_DIRS} ${Qt5Gui_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS}
)

add_library(${PROJECT_NAME}
    src/scan_message.cpp
    src/labeled_scan_message.cpp
    src/renderer.cpp
    src/binary_io.cpp
)
target_link_libraries(${PROJECT_NAME}
    yaml-cpp ${Boost_LIBRARIES} ${catkin_LIBRARIES})



add_library(${PROJECT_NAME}_plugin_core
    src/register_plugin.cpp
    src/scan_message_renderer.cpp
)

target_link_libraries(${PROJECT_NAME}_plugin_core
    yaml-cpp ${PROJECT_NAME} ${catkin_LIBRARIES})


find_package(Ceres QUIET)
if(NOT "${CERES_LIBRARIES}" STREQUAL "")
message("Building with Ceres support!")
include_directories(${CERES_INCLUDE_DIRS})
set(ADDITIONAL_SRCS "src/roll_pitch_angles.cpp")
set(ADDITIONAL_LIBS "${CERES_LIBRARIES}")
endif()

add_library(${PROJECT_NAME}_plugin_node
    src/segment_labeler.cpp
    src/split_labeled_scan.cpp
    src/labeled_scan_to_scan.cpp
    src/relabel_scan.cpp
    src/label_scan_by_validity.cpp
    src/segmentation_renderer.cpp
    src/segment_distance_filter.cpp
    src/segment_mincount_filter.cpp
    src/segment_label_filter.cpp
    src/scan_renderer.cpp
    src/scan_labeler.cpp
    src/polygon_scan_filter.cpp
    src/rotate_scan.cpp

    src/covariance.cpp
    src/filter.cpp
    src/pitch_angle.cpp
    src/height_pitch_angle.cpp
    src/deg_to_rad.cpp
    ${ADDITIONAL_SRCS}

    src/segmentation.cpp
    src/segmentation_p2p.cpp
    src/segmentation_p2p_expand.cpp
    src/segmentation_p2pline.cpp
    src/segmentation_linefit.cpp
    src/segmentation_linefit_lsq.cpp
    src/segmentation_segment_length.cpp

)

target_link_libraries(${PROJECT_NAME}_plugin_node
    yaml-cpp
    ${PROJECT_NAME}
    ${catkin_LIBRARIES}
    ${ADDITIONAL_LIBS}
)



add_library(${PROJECT_NAME}_plugin_qt
    src/scan_labeler_adapter.cpp
    src/polygon_scan_filter_adapter.cpp
)

target_link_libraries(${PROJECT_NAME}_plugin_qt
    yaml-cpp ${PROJECT_NAME} ${catkin_LIBRARIES})


#add_executable(scan_test src/test.cpp)
#target_link_libraries(scan_test ${PROJECT_NAME}_plugin ${PROJECT_NAME})

#
# INSTALL
#

install(FILES plugins.xml
        DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION})

install(TARGETS ${PROJECT_NAME}_plugin_core ${PROJECT_NAME}_plugin_node ${PROJECT_NAME}_plugin_qt
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION})

install(DIRECTORY include/${PROJECT_NAME}/
        DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION})


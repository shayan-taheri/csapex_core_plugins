cmake_minimum_required(VERSION 2.8.3)
project(csapex_ml)

include (GenerateExportHeader)

find_package(csapex REQUIRED)

list(APPEND CMAKE_PREFIX_PATH ${CSAPEX_CMAKE_MODULE_PATHS})
list(APPEND CMAKE_MODULE_PATH ${CSAPEX_CMAKE_MODULE_PATHS})

find_package(csapex_opencv REQUIRED)
find_package(csapex_core_plugins REQUIRED)

csapex_package(
    INCLUDE_DIRS include
    LIBRARIES ${PROJECT_NAME}
    CATKIN_DEPENDS csapex csapex_core_plugins csapex_opencv
    )

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(
    include
    ${csapex_INCLUDE_DIRS}
    ${csapex_core_plugins_INCLUDE_DIRS}
    ${csapex_opencv_INCLUDE_DIRS}
    ${catkin_INCLUDE_DIRS}
    )

add_library(${PROJECT_NAME} SHARED
    src/feature/features_message.cpp
    )
generate_export_header(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME}
    ${csapex_core_plugins_LIBRARIES}
    ${csapex_opencv_LIBRARIES}
    ${csapex_LIBRARIES}
    ${catkin_LIBRARIES}
    ${Boost_LIBRARIES}
    ${catkin_LIBRARIES})

set(${PROJECT_NAME}_additional_srcs "")
set(${PROJECT_NAME}_additional_libs "")
#find_package(csapex_point_cloud QUIET)
#if(${csapex_point_cloud_FOUND})
#    include_directories(${csapex_point_cloud_INCLUDE_DIRS})
#    list(APPEND
#        ${PROJECT_NAME}_additional_srcs
#        src/feature/features_message_to_labelled_pointcloud.cpp
#        )
#endif()


add_library(${PROJECT_NAME}_core SHARED
    src/register_ml_plugins.cpp
)
target_link_libraries(${PROJECT_NAME}_core ${PROJECT_NAME}
    ${catkin_LIBRARIES}
    ${csapex_LIBRARIES}
)


add_library(${PROJECT_NAME}_plugin SHARED
    src/regression_trees.cpp
    src/regression_trees_trainer.cpp
    src/feature/encode_feature_classification.cpp
    src/feature/decode_feature_classification.cpp
    src/feature/assign_feature_classification_running_labels.cpp
    src/machine_learning_node.cpp
    src/feature/feature_message_get_classification.cpp
    src/sample/features_message_filter.cpp
    src/sample/sample_splitter.cpp
    src/sample/function_sample_generator.cpp
    src/decision_tree.cpp
    src/decision_tree_trainer.cpp

    src/decision_tree_forest.cpp
    src/decision_tree_forest_trainer.cpp

    src/gradient_boosted_trees.cpp
    src/gradient_boosted_trees_trainer.cpp

    src/random_trees.cpp
    src/random_trees_trainer.cpp

    src/svm.cpp
    src/svm_trainer.cpp

    src/extended_svm.hpp
    src/svm_ensemble.cpp
    src/svm_ensemble_trainer.cpp

    src/ada_boost.cpp
    src/ada_boost_trainer.cpp

    src/waldboost/waldboost.cpp
    src/waldboost.cpp
    src/waldboost_trainer.cpp

    src/jannlab_mlp.cpp
    src/mlp/mlp.cpp

    src/mlp_cv.cpp
    src/mlp_cv_trainer.cpp

    src/feature/assign_feature_classifications.cpp
    src/feature/exchange_feature_classification.cpp
    src/feature/join_features.cpp

    src/random_vector.hpp

    ${${PROJECT_NAME}_additional_srcs}
    )

target_link_libraries(${PROJECT_NAME}_plugin
    ${PROJECT_NAME}
    ${csapex_LIBRARIES}
    ${csapex_core_plugins_LIBRARIES}
    ${csapex_opencv_LIBRARIES}
    ${catkin_LIBRARIES}
    ${${PROJECT_NAME}_additional_libs}
    ${csapex_point_cloud_LIBRARIES}
    )

#add_executable(test_mlp src/mlp/mlp.cpp src/mlp/test_mlp.cpp)

#
# INSTALL
#

install(FILES plugins.xml
    DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION})

install(TARGETS ${PROJECT_NAME}_plugin ${PROJECT_NAME}
    ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
    LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
    RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION})

if(WIN32)
    file(COPY ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_export.h
        DESTINATION ${CMAKE_CURRENT_LIST_DIR}/include/${PROJECT_NAME}/)
    install(FILES ${PROJECT_BINARY_DIR}/${PROJECT_NAME}_export.h
        DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}/${PROJECT_NAME}/)
else()
    file(COPY ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_export.h
        DESTINATION ${CMAKE_CURRENT_LIST_DIR}/include/${PROJECT_NAME}/)
    install(FILES ${PROJECT_BINARY_DIR}/${PROJECT_NAME}_export.h
        DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION})
endif()

if(WIN32)
    install(DIRECTORY include/${PROJECT_NAME}/
        DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}/${PROJECT_NAME})
else()
    install(DIRECTORY include/${PROJECT_NAME}/
        DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION})
endif()

if(NOT ${catkin_FOUND})
    install(FILES cmake/${PROJECT_NAME}-config.cmake
        DESTINATION ${CSAPEX_MODULE_DESTINATION})
endif()

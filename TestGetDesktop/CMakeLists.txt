cmake_minimum_required(VERSION 3.2)
project(ObjectTracking)

# ------------------ SET THIS PATH/TO/YOUR/OPENCV/BUILD FOLDER -------------------
set(OpenCV_DIR_HOME "D:/opencv2313/opencv/build")
# --------------------------------------------------------------------------------

set(OpenCV_DIR "${OpenCV_DIR_HOME}/x64/vc14/lib")
find_package(OpenCV REQUIRED)

include_directories(${OpenCV_DIR_HOME}/include)
set(OPVER ${OpenCV_VERSION_MAJOR}${OpenCV_VERSION_MINOR}${OpenCV_VERSION_PATCH})
set(sources TestGetDesktop.cpp pch.cpp pch.h)
add_executable(${PROJECT_NAME} ${sources}) 

set(CMAKE_CONFIGURATION_TYPES Release)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Ot")

if(OpenCV_VERSION_MAJOR GREATER 2)
	target_link_libraries( ${PROJECT_NAME} 
		"${OpenCV_DIR}/opencv_world${OPVER}.lib"
)
else() 
	target_link_libraries( ${PROJECT_NAME} 
		"${OpenCV_DIR}/opencv_core${OPVER}.lib"
		"${OpenCV_DIR}/opencv_highgui${OPVER}.lib"
		"${OpenCV_DIR}/opencv_imgproc${OPVER}.lib"
)
endif()

string(REPLACE "lib" "bin" BIN_PATH "${OpenCV_DIR}")

if(${OpenCV_VERSION_MAJOR} GREATER 2)
	add_custom_command( TARGET ${PROJECT_NAME} POST_BUILD  
   	    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    	    "${BIN_PATH}/opencv_world${OPVER}.dll"           
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
)
else ()
	add_custom_command( TARGET ${PROJECT_NAME} POST_BUILD          
    	     COMMAND ${CMAKE_COMMAND} -E copy_if_different
    	     "${BIN_PATH}/opencv_core${OPVER}.dll"
             "${BIN_PATH}/opencv_highgui${OPVER}.dll" 
             "${BIN_PATH}/opencv_imgproc${OPVER}.dll"              
             $<TARGET_FILE_DIR:${PROJECT_NAME}>
)
endif()

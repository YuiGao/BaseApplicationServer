cmake_minimum_required(VERSION 3.15)
project(CompareServer)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../bin)

set(USE_APR "ON")

set(OPENCV_LIB_PATH /usr/local/lib)

message(${PROJECT_SOURCE_DIR})


set(CMAKE_CXX_COMPILER "/usr/bin/g++")
set(CMAKE_C_COMPILER "/usr/bin/gcc")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Ofast -fopenmp -fpie -fpic")	# -std=gnu++11

set(SRC_FILE
        src/main.cpp
        third_party/src/wslog.cpp
        third_party/src/wslog.h
        third_party/src/mythread.cpp
        third_party/src/mythread.h
        src/DataHandleCore.cpp
        src/DataHandleCore.h
        src/face_tool.cpp
        src/face_tool.h
    #    src/GetFeature.cpp
    #    src/GetFeature.h
        src/HttpServer.cpp
        src/HttpServer.h
        src/base.h
        third_party/src/WsSem.cpp
        third_party/src/WsSem.h
        third_party/src/tool.cpp
        third_party/src/tool.h )

if(USE_APR STREQUAL   "ON")
set(SRC_SOCKET
        third_party/src/wssocket.cpp
        third_party/src/wssocket.h)
    message("use apr socket")
else()
set(SRC_SOCKET
        third_party/src/wssocket_native.cpp
        third_party/src/wssocket_native.h
        )
    message("use native socket")
endif()


set(CMAKE_CXX_STANDARD 14)
include_directories(${PROJECT_SOURCE_DIR}/third_party/src
                    ${PROJECT_SOURCE_DIR}/third_party/include/faceDetect
                    ${PROJECT_SOURCE_DIR}/third_party/include
                    ${PROJECT_SOURCE_DIR}/third_party/include/breakpad
                    ${PROJECT_SOURCE_DIR}/third_party/include/libevent
                    ${PROJECT_SOURCE_DIR}/third_party/include/apr-1
                    ${PROJECT_SOURCE_DIR}/third_party/opencv3.4.2/include
                    ${PROJECT_SOURCE_DIR}/third_party/opencv3.4.2/include/opencv
                    ${PROJECT_SOURCE_DIR}/third_party/opencv3.4.2/include/opencv2
                    ${PROJECT_SOURCE_DIR}/third_party/ncnn/include
        )
if(USE_APR STREQUAL   "ON")
    include_directories(${PROJECT_SOURCE_DIR}/third_party/include/apr-1)
endif()
add_executable(${PROJECT_NAME} ${SRC_FILE} ${SRC_SOCKET})


target_link_libraries(${PROJECT_NAME}
        ${PROJECT_SOURCE_DIR}/third_party/libs/libapr-1.so.0
        ${PROJECT_SOURCE_DIR}/third_party/libs/liblog4cpp.so.5
        ${PROJECT_SOURCE_DIR}/third_party/libs/libevent.a
        ${PROJECT_SOURCE_DIR}/third_party/libs/libbreakpad_client.a
      #  ${PROJECT_SOURCE_DIR}/third_party/libs/libfacereco.a
      #  ${PROJECT_SOURCE_DIR}/third_party/libs/libncnn.a

      #  ${PROJECT_SOURCE_DIR}/third_party/libs/libopencv_world.a


        #${OPENCV_LIB_PATH}/libopencv_flann.so
        ${OPENCV_LIB_PATH}/libopencv_core.so
        ${OPENCV_LIB_PATH}/libopencv_imgcodecs.so

        #${OPENCV_LIB_PATH}/libopencv_photo.so
        ${OPENCV_LIB_PATH}/libopencv_imgproc.so
        ${OPENCV_LIB_PATH}/libopencv_highgui.so


        ${PROJECT_SOURCE_DIR}/third_party/libs/liblibpng.a
        ${PROJECT_SOURCE_DIR}/third_party/libs/liblibjpeg-turbo.a
        ${PROJECT_SOURCE_DIR}/third_party/libs/liblibtiff.a
        ${PROJECT_SOURCE_DIR}/third_party/libs/libzlib.a
     #   ${PROJECT_SOURCE_DIR}/third_party/libs/libFL_Det_V1.so
        m rt dl pthread
        )
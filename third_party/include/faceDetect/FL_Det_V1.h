/**************************************************************************
 * @ Face Detection + landmark Detection  C++ implementions
 * @ Details: Joint the Face + Landmark Detection
 * @ Author: DWC 
 * @ Date: 2020,07,29
***************************************************************************/
#ifndef FL_DET_H
#define FL_DET_H
#pragma once
// load the baselib
#include <stdlib.h>
#include <memory>
#include <vector>
#include <iostream>
#define __attribute __attribute__((visibility("default")))
#ifdef __cplusplus
extern "C"
{
#endif
    typedef struct FACEINFO
    {
        float x1;           // 人脸框左上角横坐标
        float y1;           // 人脸框左上角纵坐标
        float x2;           // 人脸框右下角横坐标
        float y2;           // 人脸框右下角横坐标
        float confidence;   // 人脸置信度
        float landinfo[10]; // 5个特征点信息 (x1,y1,x2,y2,...,x5,y5)
    } faceinfo;
    /*******************************************************************************************
     * @ Model Initialization
     * @Params: thread_ : 初始化线程数目, default = 2;
     * @Params: threshold_ : 人脸检测阈值, default = 0.7;
     * @Params: is_fixed_img_ : 检测图像大小是否固定, default = true;(固定图像大小输入可以加速人脸检测)
     * @Params: image_w_, image_h_:检测图像宽高.如果is_fixed_img_=true,该参数有效
     * @ return 0 -> Success, Other-> Failness
    ********************************************************************************************/
    __attribute int FL_D_Init(int thread_, float threshold_, bool is_fixed_img_, int image_w_, int image_h_);
    /***************************************************
     * @ Function-Detect face and landmark
     * @Params: data_ : 图像数据指针,如果是cv::Mat,则参数传入 mat.data
     * @Params: channels : 图像通道数,限制输入图像为三通道图像,排列顺序为cv::Mat(BGR)格式排列
     * @Params: width_, height_ : 图像数据 宽\高
     * @Params: step, 图像行长度,如果是cv::Mat,则参数传入 mat.step[0]
     * @FACEINFO, 人脸框以及关键点信息
     * @ return 人脸检测数目 
    ****************************************************/
    __attribute int FL_D_Detect(uint8_t *data_, int channels, int &width_, int &height_, int step_, std::vector<FACEINFO> &faces);
    /***************************************************
     * @ Function Resource Release
    ****************************************************/
    __attribute void FL_D_Release();

#ifdef __cplusplus
}
#endif

#endif
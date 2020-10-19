//
// Created by ty on 2020/8/20.
//

#ifndef COMPARESERVER_GETFEATURE_H
#define COMPARESERVER_GETFEATURE_H

#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <vector>

class GetFeature {
public:
    GetFeature(int fixed_width = 0,int fixed_height=0);
    ~GetFeature();
    int feature(cv::Mat& img,std::vector<float >& feature);

private:
    int m_fixed_width;
    int m_fixed_height;
};


#endif //COMPARESERVER_GETFEATURE_H

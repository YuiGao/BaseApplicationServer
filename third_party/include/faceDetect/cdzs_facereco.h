#ifndef __ExtractorAPP__H__
#define __ExtractorAPP__H__
//#define DLL_PUBLIC __attribute__ ((visibility("default")))
// #define  DLLAPI __declspec(dllexport)


#include "opencv2/core.hpp"
#include <vector>
using namespace std;



#ifdef __cplusplus
extern "C" {
#endif
//typedef struct{
//	unsigned char* img_buffer;
//	int img_w;
//	int img_h;
//	int img_c;
//}Normal_Img;

typedef struct{
	int enable_angle;
	int angle_threshold;
	float return_angle;
}Angle_Ctrl;

typedef struct
{
	cv::Rect face_pos;
	cv::Point p0;
	cv::Point p1;
	cv::Point p2;
	cv::Point p3;
	cv::Point p4;
}face_etem;

typedef enum HASX_FACE_RECO_RET
{
    RECO_FAIL = 0,
	RECO_SUCCES,
	OVER_ANGLE
}FACE_RECO_RET;

//注意：本算法支持特征值为512
bool facereco_init_512(const int& threads);

/*
batch_size:现在是传入图片脸的个数。
srcImgs:需要提取特征值的图片，会根据特帧点提取人脸进行对人脸的提取特征值的操作
ret_res:传入图片检测过后得到的人脸框和对应的特征点
dstFeatures:返回的特征值
返回值：
*/
FACE_RECO_RET facereco_proc_512(const cv::Mat &srcImgs, const vector<face_etem> &ret_res, std::vector<float> &dstFeatures,Angle_Ctrl* angle_ctrl);
FACE_RECO_RET facereco_proc_512_extend(const cv::Mat &srcImgs, std::vector<float> &dstFeatures,Angle_Ctrl* angle_ctrl);

/*
f1,f2 两个数组的特征值，进行比对，分数在1到0之间，越接近1表示越相似，越接近0表示越不相识，有可能为负数
*/
float wf_getScores_floats_512(const float* f1, const float* f2);
float wf_getScores_floats_512_extend(const float* f1, const float* f2);
//float wf_getScores_floats_512_openblas(const float* f1, const float* f2);

/*
根据注册人脸数，获取最佳的阈值
*/
int get_the_best_score(int registe_num);

/*
注销
*/
bool facereco_deinit_512();

/*
返回当前算法版本号
*/
std::string get_alg_version(void);

#ifdef __cplusplus
}
#endif


#endif

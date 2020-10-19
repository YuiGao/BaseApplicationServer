#pragma once
#pragma pack(1)
#define IDCARD_INFO  1004
#define IDCARD_INFO_REPONSE 1005

#define STORE_PERSON_INFOMEATION 201	//version 2.1
#define STORE_PERSON_INFOMEATION_RESPONSE 202
#define MAX_IMAGE_NUM 2
#define MAX_IMAGE_SIZE 500*1024

#define DATA_BUFE_SIZE 100*1024*1024
#define FACE_FEATURE_NUMBER 5 //特征点个数

#define ARM_3399_GET_VERSION	0x101
#define ARM_3399_GET_CB			0x102
#define ARM_33399_GET_EXPOSURE	0x103
#define ARM_33399_SET_EXPOSURE	0x104


typedef enum State
{
	CAPTURE_3D_INIT,		 //初始化  
	CAPTURE_3D_INIT_SUCCESS,	//初始化成功 
	CAPTURE_3D_INIT_FAILED,		//初始化失败 

	RECOG_PREPARE_GETID,
	RECOG_PREPARE_GETIMAGE,
	RECOG_PREPARE_RECOG,			//5
	RECOG_RECOG_ING,
	RECOG_RECOG_END,
	RECOG_DYNAMIC_SUCCESS,  //二维动态识别完成
	RECOG_RECOG_SUCCESS,	//人证验证成功
	RECOG_RECOG_FAILED,  //人证验证失败 10
	MANUAL_REGISTER_PREPARE, // 手动输入注册
	MANUAL_REGISTER_COMPLETE, //手动注册完成
	CAPTURE_3D_START,  //开始采集
	CAPTURE_3D_ING,
	CAPTURE_3D_SUCCESS,			//建模成功 15
	CAPTURE_3D_FAILED,			//三维建模失败  
	DYNAMIC_REGISTER_SUCCESS,	//二维动态识别注册成功
	RECOG_BUILD_ING,			//任务进行中
	RECOG_3D_DYNAMIC_COMPLATE, //三维动态识别完成，不一定成功
	RECOG_3D_DYNAMIC_SUCCESS,//三维维动态识别完成 20
	RECOG_DYNAMIC_FAILED,  //动态识别失败   
	SAVE_PASSINFO,			//
	COMPlETE
}en_RECOG_STATE;

typedef struct _tailor_img_info {
	int tex_face_rect[4]; //纹理图人脸框
	int tex_face_landmark[10];//纹理图人脸特征点坐标
	int image_len; //jpg图片大小
	int image_size[2]; //图片尺寸
	int image_position[2];//左上角在原图位置
}st_TAILOR_IMG_INFO;


#define ARM_33399_GET_SRC_IMGS	0x105
typedef struct _src_imgs {
	st_TAILOR_IMG_INFO tex_info; //裁剪后的纹理图信息

	int speckle_left_len[3]; //左散斑3张jpg图片大小
	int speckle_left_size[2]; //左散斑图片尺寸
	int speckle_left_position[2];//左散斑左上角在原图位置

	int speckle_right_len[3]; //右散斑3张jpg图片大小
	int speckle_right_size[2]; //右散斑图片尺寸
	int speckle_right_position[2];//右散斑左上角在原图位置
}st_SRC_IMGS; //传输原图结构体

#define ARM_33399_GET_PARALLAX	0x106
typedef struct _parallax_imgs {
	st_TAILOR_IMG_INFO tex_info; //裁剪后的纹理图信息
	int parallax_len; //视差图jpg图片大小
	int parallax_size[2]; //视差图图片尺寸
	int parallax_position[2];//视差图左上角在原图位置
}st_PARALLAX_IMGS;


#define ARM_33399_GET_MODEL		0x107
typedef struct _model_3d {
	char model_name[100];
	char serial[20];
	int code; //错误码
	st_TAILOR_IMG_INFO tex_info; //裁剪后的纹理图信息
	int landmark_index[5]; //特征点三维索引
	int model_len; //模型大小
}st_MODEL_3D;

typedef struct _model_request_info{
	char serial[20];
	char idnumber[20];
}st_MODEL_REQUEST_INFO;
#pragma pack()
//
// Created by ty on 2020/8/20.
//

#include "GetFeature.h"
#include "FL_Det_V1.h"
#include "cdzs_facereco.h"
#include "wslog.h"

#define MAT_HEAD(mat) img.data,img.channels(),img.cols,img.rows,img.step[0]


bool faceinfo2facereco(faceinfo face, face_etem &et)
{
    et.face_pos.x = face.x1;
    et.face_pos.y = face.y1;
    et.face_pos.width = face.x2 - face.x1;
    et.face_pos.height = face.y2 - face.y1;

    et.p0.x = face.landinfo[0];
    et.p0.y = face.landinfo[1];
    et.p1.x = face.landinfo[2];
    et.p1.y = face.landinfo[3];
    et.p2.x = face.landinfo[4];
    et.p2.y = face.landinfo[5];
    et.p3.x = face.landinfo[6];
    et.p3.y = face.landinfo[7];
    et.p4.x = face.landinfo[8];
    et.p4.y = face.landinfo[9];
    return true;
}
int maxFace(vector<FACEINFO> face_info)
{
    if(face_info.size() < 1) return -1;
    if(face_info.size() == 1) return 0;
    int max = 0;
    int area = (face_info[0].x2 - face_info[0].x1)*(face_info[0].y2 - face_info[0].y1);
    for(int i = 1; i< face_info.size(); i++)
    {
        int temp =  (face_info[i].x2 - face_info[i].x1)*(face_info[i].y2 - face_info[i].y1);
        if(temp > area)
        {
            max = i;
            area = temp;
        }
    }
    return max;
}

GetFeature::GetFeature(int fixed_width,int fixed_height):
    m_fixed_width(fixed_width),
    m_fixed_height(fixed_height)
{
    bool r = facereco_init_512(1);
    bool is_fix_wd = false;
    if(m_fixed_width!= 0 && m_fixed_height != 0)
        is_fix_wd = true;

    int ret = FL_D_Init(1, 0.7, is_fix_wd, m_fixed_width, m_fixed_height);
    if(r != true || ret != 0)
    {
        WS_ERROR("get feature init failed:%d,%d",ret,r);
    } else
    {
        WS_DEBUG("face detect init success");
    }
}
GetFeature::~GetFeature()
{

}
int GetFeature::feature(cv::Mat& img,std::vector<float >& feature)
{
    if(m_fixed_width!= 0 && m_fixed_height != 0)
        cv::resize(img, img, cv::Size(m_fixed_width, m_fixed_height));
    WS_LINE;
    vector<FACEINFO> face_info;
    face_info.clear();
    FL_D_Detect(img.data,img.channels(),img.cols,img.rows,img.step[0],face_info);
    if(face_info.size() == 0)
    {
        WS_DEBUG("do not check out face");
        return -1;
    }
    WS_DEBUG("face number :%d",face_info.size());

    FACE_RECO_RET result;
    Angle_Ctrl angle_ctl1;
    angle_ctl1.enable_angle = 0;
    angle_ctl1.angle_threshold = 30;
    angle_ctl1.return_angle = -1000.0;
    face_etem rect_point;
    vector<face_etem> face_rect;
    face_rect.clear();
    int max = maxFace(face_info);
    WS_DEBUG("max:%d",max);
    faceinfo2facereco(face_info[max],rect_point);
    face_rect.push_back(rect_point);
    result = facereco_proc_512(img, face_rect,feature, &angle_ctl1);
    if (RECO_FAIL == result)
    {
        WS_DEBUG("reco failed");
        return -1;
    }

    WS_END;
    return 0;
}
//
// Created by ty on 2020/8/20.
//

//#include "face_tool.h"
#if 0

    bool faceinfo2cv(faceinfo face, cv::Rect &rect, std::vector<cv::Point> &points) {
        rect.x = face.x1;
        rect.y = face.y1;
        rect.width = face.x2 - face.x1;
        rect.height = face.y2 - face.y1;
        for (int i = 0; i < 5; i++) {
            cv::Point p;
            p.x = face.landinfo[2 * i];
            p.y = face.landinfo[2 * i + 1];
            points.push_back(p);
        }
        return true;
    }

    bool faceinfo2facereco(faceinfo face, face_etem &et) {
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
#endif
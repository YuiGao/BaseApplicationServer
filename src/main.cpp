#include <iostream>
#include <fstream>
#include "opencv2/opencv.hpp"
#include "wslog.h"
#include "client/linux/handler/exception_handler.h"

#include "wssocket.h"
#include "protocol.h"
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>



void Dat_to_Mat(cv::Mat &new_img, char *data, string path, string save_path,int* old_img_size)
{
    int len = 0;
    data = new char[10 * 1024 * 1024];
    FILE *fp = fopen(path.c_str(), "rb");
    char *p = data;
    while (!feof(fp))
    {
        int rlen = fread(p + len, 1, 2 * 1024 * 1024, fp);
        len += rlen;
    }
    fclose(fp);
    char padta[1024000] = {0};
    int j = 0;
    for (int i = 4; i < len - 4; i++)
    {
        padta[j++] = data[i];
    }
    //char *p1 = padta;
    len -= 8;
    std::vector<uchar> datas(padta, padta + len);

    cv::Mat old_img = cv::imdecode(datas, 1);

    old_img_size[0] = old_img.cols;
    old_img_size[1] = old_img.rows;
    //裁减图片
    new_img = old_img(cv::Rect(762, 380, 302, 420));
    // std::cout << save_path.c_str() << std::endl;
    // cv::imwrite(save_path.c_str(), new_img);
    delete[] data;
}

int data_handle(char *data,char *buffer_struct)
{
    string basePath = "../data/";
    st_MODEL_3D model = {0};
    memcpy(model.serial, buffer_struct, 20);
    DIR *dir;
    struct dirent *ptr;
    char *ply_data = new char[100 * 1024 * 1024];
    //char *img_data = new char[100 * 1024 * 1024];
    cv::Mat new_img;
    int ply_size = 0;
    if ((dir = opendir(basePath.c_str())) == NULL)
    {
        perror("Open dir error...");
        exit(1);
    }
    while ((ptr = readdir(dir)) != NULL)
    {
        if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0) ///current dir OR parrent dir
        {
            continue;
        }
        else if (ptr->d_type == 8) ///file
        {
            
            string file_name = basePath;
            file_name += ptr->d_name;
            if (file_name.rfind(".txt") != -1)
            {
                std::ifstream ifile;
                ifile.open(file_name, std::ios::in);
                if (!ifile)
                {
                    printf("打开文件失败!%s\n", file_name.c_str());
                    exit(1); //失败退回操作系统
                }
                else
                {                    
                    for (int i = 0; i < 19; i++)
                    {
                        int value;
                        ifile >> value;
                        if (i < 4)
                        {
                            model.tex_info.tex_face_rect[i] = value;
                            if (i == 0)
                            {
                                model.tex_info.image_position[0] = value;
                            }
                            else if (i == 1)
                            {
                                model.tex_info.image_position[1] = value;
                            }
                            else if (i == 2)
                            {
                                model.tex_info.image_size[0] = value;
                            }
                            else
                            {
                                model.tex_info.image_size[1] = value;
                                model.tex_info.image_len = model.tex_info.image_size[0] * model.tex_info.image_size[1];
                            }
                        }
                        else if (i >= 4 && i < 14)
                        {
                            model.tex_info.tex_face_landmark[i - 4] = value;
                        }
                        else if (i >= 14 && i < 19)
                        {
                            model.landmark_index[i - 14] = value;
                        }
                    }
                }
                ifile.close();
                
            }
            else if (file_name.rfind(".ply") != -1)
            {
                struct stat statbuf;
                if (stat(file_name.c_str(), &statbuf) == 0)
                {
                    ply_size = statbuf.st_size;
                }
                model.model_len = ply_size;
                std::ifstream ifile;
                ifile.open(file_name, std::ifstream::binary);
                if (!ifile)
                {
                    printf("打开文件失败!%s\n", file_name.c_str());
                    exit(1); //失败退回操作系统
                }
                else
                {
                    ifile.read(reinterpret_cast<char *>(ply_data), ply_size);  

                }
                ifile.close();
            }
            else if (file_name.rfind(".dat") != -1)
            {
                char *dat;
                string path = file_name;
                string save_path = file_name;
                save_path.replace(save_path.rfind(".dat"), 4, ".jpg");
                Dat_to_Mat(new_img, dat, path, save_path,model.tex_info.image_size);
                for(int i=0;i<int(strlen(ptr->d_name))-4;i++)
                {
                    model.model_name[i] = ptr->d_name[i];
                }
            }
        }
    }

    std::vector<uchar> img_data;
    cv::imencode(".jpg",new_img,img_data);
    int img_size = img_data.size();

    int model_size = sizeof(model);
    model.tex_info.image_len = img_size;
    memcpy(data, &model, model_size);
    printf("model_size : %d\n", model_size);

    memcpy(data + model_size, img_data.data(), img_size);
    printf("img_size : %d\n", img_size);

    memcpy(data + model_size + img_size, ply_data, ply_size);
    delete []ply_data;
    printf("ply_size : %d\n", ply_size);
    int send_size = model_size + img_size + ply_size;

    return send_size;
}


void OnRecv(int fd, int type, char *buff, int len, void *param)
{
    printf("收到数据为: %s\n", buff);
    WSSocketServer *pThis = (WSSocketServer *)param;
    char *senddata = new char[200 * 1024 * 1024];
    st_MODEL_REQUEST_INFO buffer;
    memcpy(&buffer, buff, len);
    char buffer_struct[20];
    memcpy(buffer_struct, buffer.serial, 20);
    memset(senddata,0,sizeof(senddata));


    int sendLen = data_handle(senddata,buffer_struct);
    st_MODEL_3D new_model;
    memcpy(&new_model, senddata, sizeof(new_model));
    printf("model_name %s\n",new_model.model_name);
    printf("serial %s\n",new_model.serial);
    printf("code %d\n",new_model.code);
    printf("tex_face_rect[0] %d\n",new_model.tex_info.tex_face_rect[0]);
    printf("tex_face_rect[1] %d\n",new_model.tex_info.tex_face_rect[1]);
    printf("tex_face_rect[2] %d\n",new_model.tex_info.tex_face_rect[2]);
    printf("tex_face_rect[3] %d\n",new_model.tex_info.tex_face_rect[3]);
    printf("image_size[0] %d\n",new_model.tex_info.image_size[0]);
    printf("image_size[1] %d\n",new_model.tex_info.image_size[1]);
    printf("image_len %d\n",new_model.tex_info.image_len);
    printf("model_len %d\n",new_model.model_len);

    
    int ret = pThis->SendMsg(fd, type, senddata, sendLen);
    if(ret != sendLen+12)
    {
        printf("发送失败\n");
    }
    else
    {
        printf("发送成功\n");
        printf("send data size: %d\n", sendLen);
    }
    delete []senddata;
    
}

void create_server_socket()
{
    int port = 8089;
    WSSocketServer *server = new WSSocketServer(port, true,200*1024*1024,200*1024*1024);
    server->SetServerRecvCallBack(OnRecv, server);
    server->Listen(100);
}

int main(int argc, char *argv[])
{

    create_server_socket();
    while (true)
    {
        sleep(1);
    }
    return 0;
}

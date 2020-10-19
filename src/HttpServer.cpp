//
// Created by ty on 2020/8/21.
//

#include "HttpServer.h"
#include "wslog.h"
#include "apr_strings.h"
#include "tool.h"
st_RESPONSE gresponse_def[] = {
        { 200,"OK",NULL },      //SUCCESS_200
        { 401,"Key Assert","key验证失败" },  //KEY_FAILED_401
        { 403,"Forbidden","禁止" },			//BAN_403
        { 405,"Method Not Allowed","不支持该方法" },	//NOT_SUPPORT_405
        { 408,"Request Time-out","请求超时" },		//REQUEST_TIMEOUT_408
        { 410,"Request Param error","请求参数错误" },	//REQUEST_ERROR_410
        { 413,"Request Entity Too Large","请求实体太大" },	//REQUEST_BIG_413
        { 503,"Service Unavailable","服务不可用" },			//SERVER_UNAVAILABLE_503
        { 506,"Service busy","服务器繁忙" },			//SERVER_BUSY_506
        { 510,"Not Support","暂时不支持该业务" },			//DONT_SUPPORT_SERVICE_510
        { 512,"Request NULL","请求为空" },				//REQUEST_ERROR_512
        { 513,"Image Error","图片解析错误，格式错误或者受损" },	//PICTURE_ERROR_513
        { 514,"Face Failed","检测人脸失败" },				//NO_FACE_514
        { 515,"Face Bad","图片中人脸质量较差" },			//PICTURE_BAD_515
        { 516,"Invilid Image","非法图片" },				//PICTURE_INVALID_516
        { 517,"No Face","图片检测不到人脸" },			//NO_FACE_IN_PICTURE_517
        { 518,"No ID","查无此人" },
        { 519, "Not the same person", "不是同一人脸" },
        { 1002, "Person Already Exist", "已存在此人" },
        { 1003, "Id Already Exist", "账号已注册" },
        { 1004, "Score is Low", "不满足阈值" },
        { 1005, "No Message in Mysql", "数据库中无信息" },
        { 1006, "Select from Mysql Failed", "数据库查询出错" },
        { 1007, "Insert to Mysql Failed", "插入数据库失败" },
        { 1008, "Can't Connect to Mysql", "服务器没有连接到数据库" },
        { 1009, "App Version is Highest", "已是最新版本" },
        { 1010, "Can't Find Apk File", "找不到本地apk文件" },
        { 1011, "Apk File Name Error", "本地apk文件命名错误" },
        { 1012, "face id is null","提供的face_id为空值，无法删除" },
        { 1013, "delete register information failed", "删除注册信息失败" },
        { 1014, "there is no such person","数据库中没有此人的数据" },
        { 1015,"Failure of Face Binding","人脸绑定失败" },
        { 1016, "first face distinguish failed", "第一次人脸识别失败" },
        { 1017,"post to print service failed","请求打印服务失败" },
        { 1018,"center face authentication failed","比对中心人脸识别失败" },
        { 1019,"post to print service timeout","请求打印服务超时" },
        { 1020,"post to center failed","请求比对中心失败" },
        { 1021,"print service authentication failed","打印服务认证失败" },
        { 1022,"post to center timeout","请求人脸比对中心超时" },
        { 1023, "send to print service failed","向打印服务发送消息失败" },
        { 1024,"print service user auth failed","文印系统用户名认证失败" },
        { 1025,"update audit status failed","修改审核状态失败" }
};




HttpServer::HttpServer(int listen_port):
         m_listen_port(listen_port)
{

}


HttpServer::~HttpServer()
{
}

int HttpServer::Init()
{

   // evthread_use_windows_threads();
    struct event_base *base = event_base_new();
    if (!base)
    {
        WS_ERROR("create event_base failed!");
        return 1;
    }
    struct evhttp *httpd = evhttp_new(base);
    if (!httpd)
    {
        WS_ERROR("create evhttp failed!");
        return 1;
    }
    evhttp_set_allowed_methods(httpd, EVHTTP_REQ_POST | EVHTTP_REQ_GET);
    if (evhttp_bind_socket(httpd, "0.0.0.0", m_listen_port) != 0)
    {
        WS_ERROR("evhttp bind socket failed!");
        return 1;
    }
    evhttp_set_gencb(httpd, send_document_cb, this);
    WS_INFO("http listen port %d success", m_listen_port);
    int ret = event_base_dispatch(base);
    WS_DEBUG("exit:%d", ret);
    return 1;
}


void HttpServer::send_document_cb(struct evhttp_request *req, void *arg)
{
    HttpServer* pThis = (HttpServer*)arg;

    ST_REQS* reqs = new ST_REQS();
    reqs->start = clock();
    GetNowTime(reqs->serial, sizeof(reqs->serial));
    reqs->req = req;
    reqs->index = (pThis->m_recv_index > 100000 ? 1 : pThis->m_recv_index++);

    const char* url = evhttp_request_get_uri(req);
    const char* host = evhttp_request_get_host(req);
    reqs->url = url;
    struct evkeyvalq* headers = evhttp_request_get_input_headers(req);
    const char* type = evhttp_find_header(headers, "Content-Type");//实体长度
    WS_WARN("Content type:%s", type);
    //注意接收序列增加
    WS_INFO("serial:%s (%d)recv from[%s] url:%s  ", reqs->serial,reqs->index, req->remote_host, url);

    pThis->m_call(reqs,pThis->m_arg);
    return;

}

int HttpServer::send_error(ST_REQS* req, en_RESPONSE_CODE code, const char* extra)
{
    struct evbuffer* reponse = evbuffer_new();
    char buffs[1024] = { 0 };
    sprintf(buffs, "{\n\"resultCode\":\"-1\",\n \"resultMsg\":\"error\",\n\"resultCodeSub\":\"%d\",\"resultMsgSub\":\"%s %s\"\n}", response_code(code), response_describe(code), extra);
    char len[20] = { 0 };
    sprintf(len,"%d",(int)strlen(buffs));


    WS_DEBUG("(%d) use %d reponse:%s", req->index, clock() - req->start, buffs);
    evhttp_add_header(req->req->output_headers, "Content-Type", "application/json;charset=UTF-8");
    //evhttp_add_header(req.req->output_headers, "Content-Length", len);
    evbuffer_add(reponse, buffs, strlen(buffs));
    evhttp_send_reply(req->req, 200, "OK", reponse);
    evbuffer_free(reponse);
    delete req;
    return 0;
}
int HttpServer::send_success(ST_REQS* req, std::string& reponse)
{
    struct evbuffer* reponsebuf = evbuffer_new();
    //WS_DEBUG("(%d)reponse:%s",req->index,reponse.c_str());
    evbuffer_add(reponsebuf, reponse.c_str(), reponse.length());
    char len[20] = { 0 };
    sprintf(len,"%d",(int)reponse.length());
    evhttp_add_header(req->req->output_headers, "Content-Type", "application/json;charset=UTF-8");
    evhttp_add_header(req->req->output_headers, "Content-Length", len);
    evhttp_send_reply(req->req, response_code(SUCCESS_200), (const char*)response_describe(SUCCESS_200), reponsebuf);

    if (reponse.size() > 5000)
    {
        WS_INFO("(%d) use %d response:success", req->index, clock() - req->start);
    }
    else
    {
        WS_INFO("(%d) use %d response:%s", req->index, clock() - req->start, reponse.c_str());
    }
    evbuffer_free(reponsebuf);

    delete req;
    return 0;
}
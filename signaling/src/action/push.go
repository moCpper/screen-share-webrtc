package action

import (
	"net/http"
	"signaling/src/comerrors"
	"signaling/src/framework"
	"strconv"
	"log"
)

type pushAction struct{}

func NewPushAction() *pushAction {
	return &pushAction{}
}

type xrtcPushReq struct {
	Cmdno      int    `json:"cmdno"`
	Uid        uint64 `json:"uid"`
	StreamName string `json:"stream_name"`
	Audio      int    `json:"audio"`
	Video      int    `json:"video"`
}

type xrtcPushResp struct {
	ErrNo  int    `json:"err_no"`
	ErrMsg int    `json:"err_msg"`
	Offer  string `json:"offer"`
}

func (*pushAction) Execute(w http.ResponseWriter, cr *framework.ComRequest) {
    // 添加详细日志
    log.Printf("开始处理push请求")
    
    // 基础检查
    if cr == nil {
        log.Printf("cr为空")
        cerr := comerrors.New(comerrors.ParamErr, "invalid request: cr is nil")
        writeJsonErrorResponse(cerr, w, nil)
        return
    }
    
    if cr.R == nil {
        log.Printf("cr.R为空")
        cerr := comerrors.New(comerrors.ParamErr, "invalid request: cr.R is nil")
        writeJsonErrorResponse(cerr, w, nil)
        return
    }

    r := cr.R
    log.Printf("请求参数: %+v", r.Form)
    
    // 解析表单
    if err := r.ParseForm(); err != nil {
        log.Printf("解析表单失败: %v", err)
        cerr := comerrors.New(comerrors.ParamErr, "parse form error: " + err.Error())
        writeJsonErrorResponse(cerr, w, cr)
        return
    }

    // uid 参数处理
    strUid := r.FormValue("uid")
    log.Printf("获取到uid参数: %s", strUid)
    
    if strUid == "" {
        log.Printf("uid为空")
        cerr := comerrors.New(comerrors.ParamErr, "uid is required")
        writeJsonErrorResponse(cerr, w, cr)
        return
    }

    uid, err := strconv.ParseUint(strUid, 10, 64)
    if err != nil || uid <= 0 {
        log.Printf("uid解析失败: %v", err)
        cerr := comerrors.New(comerrors.ParamErr, "invalid uid: " + err.Error())
        writeJsonErrorResponse(cerr, w, cr)
        return
    }

    // streamName 参数处理
    streamName := r.FormValue("streamName")
    log.Printf("获取到streamName参数: %s", streamName)
    
    if streamName == "" {
        log.Printf("streamName为空")
        cerr := comerrors.New(comerrors.ParamErr, "streamName is required")
        writeJsonErrorResponse(cerr, w, cr)
        return
    }

    // audio/video 参数处理
    audio := 0
    if r.FormValue("audio") != "" && r.FormValue("audio") != "0" {
        audio = 1
    }
    log.Printf("audio参数: %d", audio)

    video := 0
    if r.FormValue("video") != "" && r.FormValue("video") != "0" {
        video = 1
    }
    log.Printf("video参数: %d", video)

    // 构造请求对象
    req := &xrtcPushReq{
        Cmdno:      1, // 添加 CMDNO_PUSH 的值
        Uid:        uid,
        StreamName: streamName,
        Audio:      audio,
        Video:      video,
    }
    log.Printf("构造的请求对象: %+v", req)

    // 调用后端服务
    var resp xrtcPushResp
    err = framework.Call("xrtc", req, &resp, cr.LogId)
    if err != nil {
        log.Printf("调用后端服务失败: %v", err)
        cerr := comerrors.New(comerrors.NetworkErr, "call backend failed: " + err.Error())
        writeJsonErrorResponse(cerr, w, cr)
        return
    }

    log.Printf("调用后端服务成功，响应: %+v", resp)
    
    // 返回成功响应
    //writeJsonResponse(&resp, w, cr)
}


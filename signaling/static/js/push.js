'use strict';

var pushBtn = document.getElementById('pushBtn');

pushBtn.addEventListener('click', startPush);

// 通过id获取对应参数
var uid = $("#uid").val();
var streamName = $("#streamName").val();
var audio = $("#audio").val();
var video = $("#video").val();

function startPush(){
    console.log("start push clicked");

    // 发送post
    $.post("/signaling/push",
        { "uid": uid, "streamName": streamName, "audio": audio, "video": video },
        function(data, textStatus){
        },
        "json"
    )
}
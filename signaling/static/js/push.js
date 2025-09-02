'use strict';

var pushBtn = document.getElementById("pushBtn");

pushBtn.addEventListener("click", startPush);

var uid = $("#uid").val();
var streamName = $("#streamName").val();
var audio = $("#audio").val();
var video = $("#video").val();

function startPush() {
    console.log("send push: /signaling/push");

    $.post("/signaling/push",
        {"uid": uid, "streamName": streamName, "audio": audio, "video": video},
        function(data, textStatus) {
            console.log("push response: " + JSON.stringify(data));
            if ("success" == textStatus && 0 == data.errNo) {
                $("#tips1").html("<font color='blue'>推流请求成功!</font>");
            } else {
                $("#tips1").html("<font color='red'>推流请求失败!</font>");
            }
        },
        "json"
    );
}

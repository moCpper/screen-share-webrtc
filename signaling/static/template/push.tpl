<!DOCTYPE html>

<html>
<head>
    <meta charset="uft-8">
    <title>1v1屏幕共享-推流端</title>

    <style type="text/css">
        body{
            font-size: 13px;
        }
        
        .highlight{
            background-color: #eeeeee;
            margin: 0 0 5px 0;
            padding: 0.5em 1.5em;
        }
        
        video:{
            width: 480px;
            height: 360px;
        }

        button{
            background-color: #d84a38;
            border: none;
            border-radius: 2px;
            color: white;
            margin: 5px 0 0 0;
            padding: 0.5em 0.7em 0.6em 0.7em;
        }

        button:hover{
            background-color: #cf402f;
        }

    </style>
</head>

<body>
    <h3>1v1屏幕共享-推流端</h3>

    <div class="highlight">
        推流端基本信息
        <span>
            uid={{.uid}}
            streamName={{.streamName}}
            audio={{.audio}}
            video={{.video}}
        </span>
    </div>
    <div>
        <video id="localVideo" controls autoplay></video>
    </div>

    <button id="pushBtn">开始推流</button>
    <button id="stopPushBtn">停止推流</button>

</body>

</html>
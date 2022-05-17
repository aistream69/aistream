#!/bin/sh

curl -d "{\"type\":\"mq\",\"data\":{\"host\":\"1.15.44.137\",\"port\":5672,\"username\":\"admin\",\"password\":\"admin\",\"exchange\":\"amq.direct\",\"routingkey\":\"\"}}" http://127.0.0.1:15515/api/system/set/output
curl -d "{\"id\":99,\"data\":{\"tcp_enable\":0,\"url\":\"rtsp://127.0.0.1:8554/test.264\"}}" http://127.0.0.1:15515/api/obj/add/rtsp
curl -d "{\"id\":99,\"data\":{\"task\":\"face_capture\",\"params\":{\"preview\":\"hls\"}}}" http://127.0.0.1:15515/api/task/start
curl -d "{\"id\":98,\"data\":{\"tcp_enable\":0,\"url\":\"rtsp://127.0.0.1:8554/test.264\"}}" http://127.0.0.1:15515/api/obj/add/rtsp
curl -d "{\"id\":98,\"data\":{\"task\":\"face_capture\"}}" http://127.0.0.1:15515/api/task/start
curl -d "{\"id\":97,\"data\":{\"tcp_enable\":0,\"url\":\"rtsp://127.0.0.1:8554/test.264\"}}" http://127.0.0.1:15515/api/obj/add/rtsp
curl -d "{\"id\":97,\"data\":{\"task\":\"face_capture\"}}" http://127.0.0.1:15515/api/task/start

sleep 10
while true
do
    curl -d "{\"id\":97,\"data\":{\"task\":\"face_capture\"}}" http://127.0.0.1:15515/api/task/stop
    curl -d "{\"id\":98,\"data\":{\"task\":\"face_capture\"}}" http://127.0.0.1:15515/api/task/stop
    sleep 3
    curl -d "{\"id\":97,\"data\":{\"task\":\"face_capture\"}}" http://127.0.0.1:15515/api/task/start
    curl -d "{\"id\":98,\"data\":{\"task\":\"face_capture\"}}" http://127.0.0.1:15515/api/task/start
    sleep 10
done


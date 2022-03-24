#!/bin/sh

#curl -d "{\"id\":99,\"data\":{\"tcp_enable\":0,\"url\":\"rtsp://127.0.0.1:8554/test.264\"}}" http://127.0.0.1:15515/api/obj/add/rtsp
#curl -d "{\"id\":99,\"data\":{\"task\":\"debug\"}}" http://127.0.0.1:15515/api/task/start
#curl -d "{\"id\":98,\"data\":{\"tcp_enable\":0,\"url\":\"rtsp://127.0.0.1:8554/test.264\"}}" http://127.0.0.1:15515/api/obj/add/rtsp
#curl -d "{\"id\":98,\"data\":{\"task\":\"debug\"}}" http://127.0.0.1:15515/api/task/start

curl -d "{\"id\":99,\"data\":{\"tcp_enable\":0,\"url\":\"rtsp://127.0.0.1:8554/test.264\"}}" http://127.0.0.1:15515/api/obj/add/rtsp
curl -d "{\"id\":99,\"data\":{\"task\":\"face_capture\",\"params\":{\"test\":1}}}" http://127.0.0.1:15515/api/task/start


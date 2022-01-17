#!/bin/sh

curl -d "{\"id\":97,\"data\":{\"tcp_enable\":0,\"url\":\"rtsp://127.0.0.1:8554/test.264\"}}" http://127.0.0.1:15515/api/obj/add/rtsp
curl -d "{\"id\":98,\"data\":{\"sn\":123456789}}" http://127.0.0.1:15515/api/obj/add/gat1400
curl -d "{\"id\":99,\"data\":{\"tcp_enable\":0,\"url\":\"rtsp://127.0.0.1:8554/test.264\"}}" http://127.0.0.1:15515/api/obj/add/rtsp
curl -d "{\"id\":99,\"data\":{\"task\":\"simple\"}}" http://127.0.0.1:15515/api/task/start
curl -d "{\"id\":99,\"data\":{\"task\":\"debug\",\"params\":{\"preview\":{\"enable\":0}}}}" http://127.0.0.1:15515/api/task/start
curl -d "{\"id\":97,\"data\":{\"task\":\"simple\"}}" http://127.0.0.1:15515/api/task/start
curl -d "{\"id\":98,\"data\":{\"task\":\"jpegdec\"}}" http://127.0.0.1:15515/api/task/start


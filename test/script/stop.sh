#!/bin/sh

curl -d "{\"id\":97}" http://127.0.0.1:15515/api/obj/del
curl -d "{\"id\":97,\"data\":{\"task\":\"simple\"}}" http://127.0.0.1:15515/api/task/stop


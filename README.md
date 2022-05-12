# aistream
为算法工程化而生
****

## 特性
* [接口](#文本)
    * Restful API + json
* [输入](#文本)
    * 支持任何格式数据输入，视频、图片、音频、文本等
    * 支持rtsp，rtmp，websocket，gb28181，gat1400，hik/dahua sdk，ftp等
    * 支持互联网数据接入
* [输出](#文本)
    * 结构化数据，RabbitMQ/Kafka + json
* [算法](#文本)
    * 开箱即用，丰富的深度学习模型库，detector/tracker/classifier等，涉及人脸，姿态，声音，车辆，性别，年龄等
    * 支持用户算法模型导入，支持用户动态库函数模型导入，基于json的简单配置，pipeline机制
    * 一切都是插件
* [视频预览](#文本)
    * 支持hls/http-flv/webrtc
* [集群](#文本)
    * 支持master/slave模式，自动负载均衡，无限扩展
    * 每个slave支持GPU一机多卡
    * 支持数据库参数持久化，用户只需要调用一次RESTful，重启自动运行任务

## 测试步骤
### 1. 下载第三方依赖包
    链接：https://pan.baidu.com/s/1_acGS6FxyPRYPK9WTixHvA
    提取码：t3uo
    所有tar.gz/tar.xz文件拷贝到work/pkg
### 2. 编译
    mkdir build
    cd build
    cmake ..
    make
### 3. 运行
    首先搭建rabbitmq服务器(假如IP为10.1.1.3)，如果没有可修改samples/face_detection.json把插件节点rabbitmq删除即可。

    启动服务: 
        ./aistream

    通过Http Restful Api启动任务(人脸检测/跟踪/抓拍):
        curl -d "{\"type\":\"mq\",\"data\":{\"host\":\"10.1.1.3\",\"port\":5672,\"username\":\"guest\",\"password\":\"guest\",\"exchange\":\"amq.direct\",\"routingkey\":\"\"}}" http://127.0.0.1:15515/api/system/set/output
        curl -d "{\"id\":99,\"data\":{\"tcp_enable\":0,\"url\":\"rtsp://127.0.0.1:8554/test.264\"}}" http://127.0.0.1:15515/api/obj/add/rtsp
        curl -d "{\"id\":99,\"data\":{\"task\":\"face_capture\"}}" http://127.0.0.1:15515/api/task/start

    正常运行的话，可以在rabbitmq客户端接收到如下结构化信息(http图片访问依赖nginx服务器，配置见cfg/config.json)：
    {
        "msg_type":     "common",
        "data": {
                "id":   99,
                "timestamp":    1652335593,
                "sceneimg":     {
                        "url":  "http://10.1.1.3:8090/image/20220512/99/1652335592_0_scene.jpg"
                },
                "object":       [{
                                "type": "face",
                                "trackId":      26167,
                                "x":    716,
                                "y":    520,
                                "w":    173,
                                "h":    199,
                                "url":  "http://10.1.1.3:8090/image/20220512/99/1652335592_0_obj.jpg"
                        }]
        }
    }


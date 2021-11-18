# aistream
Everything is a plugin
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
    * 支持用户算法模型导入，支持用户动态库函数模型导入，基于json的简单配置
    * 支持多模型融合及pipeline
* [视频预览](#文本)
    * 支持hls/http-flv
* [集群](#文本)
    * 支持master/slave模式，自动负载均衡，无限扩展
    * 每个slave支持GPU一机多卡
    * 支持数据库参数持久化，用户只需要调用一次RESTful，重启自动运行任务


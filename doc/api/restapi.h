/**
 * @api {POST} /api/system/login 1.01 登录
 * @apiGroup System
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiBody {String}   username        用户名
 * @apiBody {String}   password        密码
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "username":"admin",
 *                              "password":"123456"
 *                          }
 * @apiSuccess (200) {int}      code            0:成功 1:失败
 * @apiSuccess (200) {String}   msg             信息
 * @apiSuccess (200) {String}   token           服务器端生成的token,登录成功后的每个命令都需要在http头部的Authorization字段携带token
 * @apiSuccess (200) {int}      validtime       有效时间,单位:分钟,客户端需要在这个时间内重复登录获取token
 * @apiSuccessExample {json} 返回样例:
 *                          {
 *                              "code":0,
 *                              "msg":"success",
 *                              "data": {
 *                                  "token":"xxxxxxx",
 *                                  "validtime":30
 *                              }
 *                          }
 */

/**
 * @api {POST} /api/system/logout 1.02 退出
 * @apiGroup System
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiBody {String}   username        用户名
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "username":"admin"
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccessExample {json} 返回样例:
 *                          {"code":0,"msg":"success","data":{}}
 */

/**
 * @api {POST} /api/system/set/output 1.03 输出配置
 * @apiGroup System
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiBody {String}   type            输出消息类型,默认为RabbitMQ
 * @apiBody {String}   host            服务器IP地址
 * @apiBody {int}      port            端口
 * @apiBody {String}   username        用户名
 * @apiBody {String}   password        密码
 * @apiBody {String}   exchange        参见RabbitMQ管理页面
 * @apiBody {String}   routingkey      参见RabbitMQ管理页面
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "type": "mq",
 *                              "data": {
 *                                  "host": "192.168.0.10",
 *                                  "port": 5672,
 *                                  "username": "guest",
 *                                  "password": "guest",
 *                                  "exchange": "amq.direct",
 *                                  "routingkey": ""
 *                              }
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccessExample {json} 返回样例:
 *                          {"code":0,"msg":"success","data":{}}
 */

/**
 * @api {POST} /api/system/slave/add 1.04 集群 - 添加服务器
 * @apiGroup System
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiBody {String}   ip              slave机器IP地址
 * @apiBody {int}      rest_port       slave机器RestAPI端口
 * @apiBody {String}   [internet_ip]   公网IP地址,用于云端部署或容器部署时外部访问
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "ip":"192.168.0.100",
 *                              "rest_port":15515,
 *                              "internet_ip":"8.8.8.8"
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccessExample {json} 返回样例:
 *                          {"code":0,"msg":"success","data":{}}
 */

/**
 * @api {POST} /api/system/slave/del 1.05 集群 - 删除服务器
 * @apiGroup System
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiBody {String}   ip              slave机器IP地址
 * @apiBody {int}      rest_port       slave机器RestAPI端口
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "ip":"192.168.0.100",
 *                              "rest_port":15515
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccessExample {json} 返回样例:
 *                          {"code":0,"msg":"success","data":{}}
 */

 /**
 * @api {POST} /api/obj/add/rtsp 2.01 添加设备 - rtsp
 * @apiGroup Object
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiBody {int}      id              设备ID
 * @apiBody {int}      tcp_enable      rtsp模式下的tcp使能, 1:tcp,0:udp
 * @apiBody {String}   url             rtsp视频流地址
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "id":99,
 *                              "data":{
 *                                  "tcp_enable":0,
 *                                  "url":"rtsp://192.168.0.64/h264/ch1/main/av_stream"
 *                              }
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccessExample {json} 返回样例:
 *                          {"code":0,"msg":"success","data":{}}
 */

 /**
 * @api {POST} /api/obj/add/rtmp 2.02 添加设备 - rtmp
 * @apiGroup Object
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiBody {int}      id              设备ID
 * @apiBody {String}   url             rtmp视频流地址
 * @apiBody {String}   [comment]       注: rtmp协议可用于局域网设备云端接入
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "id":99,
 *                              "data":{
 *                                  "url":"rtmp://127.0.0.1:1935/myapp/stream99"
 *                              }
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccessExample {json} 返回样例:
 *                          {"code":0,"msg":"success","data":{}}
 */

 /**
 * @api {POST} /file-xxx 2.03 添加设备 - file
 * @apiGroup Object
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiBody {String}   [comment]       注: file类型比较特殊，文件上传后，可以直接关联模型进行推理,
 *                                         上传url由[5.01 任务支持查询]获取,如/api/task/support?obj=img
 * @apiBody {String}   [comment]       使用示例:curl -F "head={\"filesize\":123,\"userdata\":{}}" 
 *                                              -F "file=@/tmp/test.jpg" http://ip:port/file-resnet50
 * @apiSuccessExample {json} mq输出推理结果样例:
 *                          {
 *                              "msg_type":     "common",
 *                              "data": {
 *                                  "id":   -1674191235,
 *                                  "timestamp":    1660893267,
 *                                  "sceneimg":     {
 *                                      "url":  "http://124.222.53.156:8080/image/20220819/-1674191235/1660893266_599686.jpg"
 *                                  },
 *                                  "object":       [
 *                                      {
 *                                          "type": "name:bicycle, score:0.99",
 *                                          "x":    117,
 *                                          "y":    125,
 *                                          "w":    452,
 *                                          "h":    307
 *                                      },
 *                                      {
 *                                          "type": "name:truck, score:0.94",
 *                                          "x":    473,
 *                                          "y":    87,
 *                                          "w":    219,
 *                                          "h":    79
 *                                      },
 *                                      {
 *                                          "type": "name:dog, score:1.00",
 *                                          "x":    123,
 *                                          "y":    223,
 *                                          "w":    197,
 *                                          "h":    320
 *                                      }
 *                                  ],
 *                                  "userdata":     {
 *                                      "uid":  1660893266580
 *                                  }
 *                              }
 *                          }
 */

 /**
 * @api {POST} /api/obj/del 2.04 删除设备
 * @apiGroup Object
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiBody {int}      id              设备ID
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "id":99
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccessExample {json} 返回样例:
 *                          {"code":0,"msg":"success","data":{}}
 */

 /**
 * @api {POST} /api/task/start 3.01 开始任务
 * @apiGroup Task
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiBody    {int}       id          设备ID
 * @apiBody    {String}    task        任务名称,由[5.01 任务支持查询]获取
 * @apiBody    {String}    params      任务携带参数
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "id":99,
 *                              "data": {
 *                                  "task":"face_capture",
 *                                  "params":{
 *                                      "preview": "hls"
 *                                  }
 *                              }
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccessExample {json} 返回样例:
 *                          {"code":0,"msg":"success","data":{}}
 */

 /**
 * @api {POST} /api/task/stop 3.02 停止任务
 * @apiGroup Task
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiBody    {int}       id          设备ID
 * @apiBody    {String}    task        任务名称
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "id":99,
 *                              "data": {
 *                                  "task":"face_capture"
 *                              }
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccessExample {json} 返回样例:
 *                          {"code":0,"msg":"success","data":{}}
 */

 /**
 * @api {POST} /api/data/query 4.01 数据查询
 * @apiGroup Data
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiBody    {String} type            数据类型，默认为"capture"
 * @apiBody    {int}    start_time      开始时间(timestamp,秒)
 * @apiBody    {int}    stop_time       停止时间(timestamp,秒)
 * @apiBody    {int}    id              设备id
 * @apiBody    {int}    [skip]          可选，跳过多少条，如果参数不存在，取所有
 * @apiBody    {int}    [limit]         可选，每次返回数目，如果参数不存在，取所有
 * @apiBody    {int}    [need_count]    可选，1:返回值携带总数目total字段，0:不返回total，
 *                                              尽量置0，减少查询压力
 * @apiParamExample {json} 请求样例：
 *                          {
 *                              "type": "capture",
 *                              "start_time": 1661863316,
 *                              "stop_time":  1661863378,
 *                              "id":         [91,92],
 *                              "skip":       40,
 *                              "limit":      2,
 *                              "need_count": 1
 *                          }
 * @apiSuccess (200) {int}      code    0:成功 1:失败
 * @apiSuccess (200) {String}   msg     信息
 * @apiSuccessExample {json} 返回样例:
 *    {
 *        "code":0,
 *        "msg":"success",
 *        "data":{
 *            "total":        163,
 *            "capture":      [
 *                {
 *                    "id":   91,
 *                    "timestamp":    1662362188,
 *                    "sceneimg":     {
 *                        "url":  "http://10.0.4.15:8080/image/20220905/91/xxxx_0_scene.jpg"
 *                    },
 *                    "object":       [
 *                        "type": "face",
 *                        "trackid":      72,
 *                        "x":    419,
 *                        "y":    311,
 *                        "w":    156,
 *                        "h":    182,
 *                        "url":  "http://10.0.4.15:8080/image/20220905/91/xxxx_0_obj.jpg"
 *                    ],
 *                    "name": "研发D栋2楼"
 *                },
 *                {
 *                    "id":   92,
 *                    "timestamp":    1662362198,
 *                    "sceneimg":     {
 *                        "url":  "http://10.0.4.15:8080/image/20220905/92/xxxx_0_scene.jpg"
 *                    },
 *                    "object":       [
 *                        "type": "face",
 *                        "trackid":      33,
 *                        "x":    509,
 *                        "y":    277,
 *                        "w":    166,
 *                        "h":    199,
 *                        "url":  "http://10.0.4.15:8080/image/20220905/92/xxxx_0_obj.jpg"
 *                    ],
 *                    "name": "研发D栋3楼"
 *                }
 *            ]
 *        }
 *    }
 */

 /**
 * @api {GET} /api/task/support 5.01 任务支持查询
 * @apiGroup HttpGet
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiBody {String}   [comment]       注: http get查询需要携带参数,格式为/api/task/support?obj=type,其中type为video/img/text等
 * @apiSuccess (200) {int}      code            0:成功 1:失败
 * @apiSuccess (200) {String}   msg             信息
 * @apiSuccess (200) {String}   data            支持的任务详细信息
 * @apiSuccessExample {json} 返回样例(video):
 *                          {
 *                              "code":0,
 *                              "msg":"success",
 *                              "data":{
 *                                  "alg":[
 *                                      {
 *                                          "name": "face_capture",
 *                                          "disabled": 0
 *                                      },
 *                                      {
 *                                          "name": "face_osd",
 *                                          "disabled": 0
 *                                      },
 *                                      {
 *                                          "name": "preview",
 *                                          "disabled": 0
 *                                      },
 *                                  ]
 *                              }
 *                          }
 * @apiSuccessExample {json} 返回样例(img):
 *                          {
 *                              "code":0,
 *                              "msg":"success",
 *                              "data":{
 *                                  "alg":[
 *                                      {
 *                                          "name": "resnet50",
 *                                          "url":  ["http://10.0.4.15:11606/file-resnet50"],
 *                                          "disabled": 0
 *                                      },
 *                                      {
 *                                          "name": "yolov3",
 *                                          "url":  ["http://10.0.4.15:11608/file-yolov3"],
 *                                          "disabled": 0
 *                                      }
 *                                  ]
 *                              }
 *                          }
 */

 /**
 * @api {GET} /api/system/get/info 5.02 系统信息查询
 * @apiGroup HttpGet
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiSuccess (200) {int}      code            0:成功 1:失败
 * @apiSuccess (200) {String}   msg             信息
 * @apiSuccess (200) {String}   data            系统信息
 * @apiSuccessExample {json} 返回样例:
 *                          {
 *                              "code":0,
 *                              "msg":"success",
 *                              "data":{
 *                                  "build":        {
 *                                      "version":      "V0.01.2022071801",
 *                                      "time": "20:21:00 Aug 16 2022"
 *                                  },
 *                                  "output":       {
 *                                      "type": "mq",
 *                                      "host": "10.0.4.15",
 *                                      "port": 5672,
 *                                      "username":     "admin",
 *                                      "password":     "admin",
 *                                      "exchange":     "amq.direct",
 *                                      "routingkey":   ""
 *                                  }
 *                              }
 *                          }
 */

 /**
 * @api {GET} /api/system/slave/status 5.03 集群服务器状态查询
 * @apiGroup HttpGet
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiSuccess (200) {int}      code            0:成功 1:失败
 * @apiSuccess (200) {String}   msg             信息
 * @apiSuccess (200) {String}   data            系统信息
 * @apiSuccessExample {json} 返回样例:
 *                          {
 *                              "code":0,
 *                              "msg":"success",
 *                              "data":{
 *                                  "slave":        [{
 *                                      "name": "test",
 *                                      "ip":   "10.0.4.15",
 *                                      "port": 15515,
 *                                      "internet_ip":  "",
 *                                      "status":       1,
 *                                      "load": 0,
 *                                      "objNum":       0
 *                                  }],
 *                                  "total":        1
 *                              }
 *                          }
 */

 /**
 * @api {GET} /api/obj/id/all 5.04 获取所有设备ID
 * @apiGroup HttpGet
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiBody {String}   [comment]       无
 * @apiSuccess (200) {int}      code            0:成功 1:失败
 * @apiSuccess (200) {String}   msg             信息
 * @apiSuccess (200) {String}   data            系统信息
 * @apiSuccessExample {json} 返回样例:
 *                          {
 *                              "code":0,
 *                              "msg":"success",
 *                              "data":{
 *                                  "obj":  [3, 4, 5]
 *                              }
 *                          }
 */

 /**
 * @api {GET} /api/obj/status 5.05 设备状态查询
 * @apiGroup HttpGet
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiBody {String}   [comment]       注: http get查询需要携带参数,格式为:
 *                                         /api/obj/status?type=rtsp&offset=0&limit=20,
 *                                         其中type为设备类型,offset为偏移量,limit为查询最大个数,
 *                                         可用于分页查询.
 * @apiSuccess (200) {int}      code            0:成功 1:失败
 * @apiSuccess (200) {String}   msg             信息
 * @apiSuccess (200) {String}   data            系统信息
 * @apiSuccessExample {json} 返回样例:
 *                          {
 *                              "code":0,
 *                              "msg":"success",
 *                              "data":{
 *                                  "obj":  [
 *                                      {
 *                                          "name": "test1",
 *                                          "id":   91,
 *                                          "url":  "rtsp://127.0.0.1:8554/test1.264",
 *                                          "alg":  "face_capture",
 *                                          "preview":      "hls",
 *                                          "status":       1,
 *                                          "preview_url":  "http://10.0.4.15:8080/m3u8/stream91/play.m3u8"
 *                                      },
 *                                      {
 *                                          "name": "test2",
 *                                          "id":   92,
 *                                          "url":  "rtsp://127.0.0.1:8554/test2.264",
 *                                          "alg":  "face_osd",
 *                                          "preview":      "http-flv",
 *                                          "status":       1,
 *                                          "preview_url":  "http://10.0.4.15:8080/live?port=1935&app=myapp&stream=stream92"
 *                                      },
 *                                      {
 *                                          "name": "test3",
 *                                          "id":   93,
 *                                          "url":  "rtsp://127.0.0.1:8554/test3.264",
 *                                          "alg":  "none",
 *                                          "preview":      "none",
 *                                          "status":       0,
 *                                          "preview_url":  ""
 *                                      }
 *                                  ],
 *                                  "total": 3
 *                              }
 *                          }
 */

 /**
 * @api {OUT} /mq/output 6.01 输出结果
 * @apiGroup Output
 * @apiVersion 0.1.0
 * @apiDescription 详细描述
 * @apiSuccessExample {json} 输出样例1:
 *                          {
 *                              "msg_type": "common",
 *                              "data":{
 *                                  "id":99,
 *                                  "timestamp": 1660647623,
 *                                  "sceneimg": {
 *                                      "url": "http://xxxx:xxxx/scene.jpg"
 *                                  },
 *                                  "object":       [{
 *                                      "type": "face",
 *                                      "trackid":      6,
 *                                      "x":    368,
 *                                      "y":    302,
 *                                      "w":    184,
 *                                      "h":    177,
 *                                      "url":  "http://xxxx:xxxx/face.jpg"
 *                                  }]
 *                              }
 *                          }
 * @apiSuccessExample {json} 输出样例2:
 *                          {
 *                              "msg_type": "common",
 *                              "data":{
 *                                  "id":99,
 *                                  "timestamp": 1660647623,
 *                                  "sceneimg": {
 *                                      "url": "http://xxxx:xxxx/scene.jpg"
 *                                  },
 *                                  "object":       [{
 *                                      "type": "veh",
 *                                      "plate_num":"苏A12345",
 *                                      "brand":"宝马-宝马X5-2014",
 *                                      "x":    368,
 *                                      "y":    302,
 *                                      "w":    184,
 *                                      "h":    177,
 *                                      "url":  "http://xxxx:xxxx/plate.jpg"
 *                                  }]
 *                              }
 *                          }
 */


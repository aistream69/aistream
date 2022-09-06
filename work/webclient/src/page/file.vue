<template>
    <div class="fillcontain">
        <head-top></head-top>
        <div class="table_container">
        <el-tabs v-model="activeName" @tab-click="handleClick">
            <el-tab-pane label="图片" name="img">
              <el-form :model="fileForm" ref="fileForm" label-position="left">
                <el-form-item label="任务">
                    <el-select v-model="task_select" placeholder="请选择" @change="handleSelect">
                        <el-option
                            v-for="item in algForm"
                            :key="item.value"
                            :label="item.label"
                            :value="item.value"
                            :disabled="item.disabled">
                        </el-option>
                    </el-select>
                </el-form-item>
                <el-form-item style="margin-left:40px">
                    <el-upload
                      class="avatar-uploader"
                      :action="task_url"
                      :http-request="uploadRequest"
                      :data="head_obj"
                      :show-file-list="false"
                      :on-success="uploadImg"
                      :before-upload="beforeImgUpload">
                      <img v-if="fileForm.image_path" :src="fileForm.image_path" class="avatar">
                      <i v-else class="el-icon-plus avatar-uploader-icon"></i>
                    </el-upload>
                </el-form-item>
                <el-form-item style="margin-left:40px">
                    <canvas id="myCanvas" width=800 height=600></canvas>
                    <!--el-image
                      <img
                      id="myImg"
                      style="width: 640px; height: 480px; display:none"
                      v-if="fileForm.image_path2"
                      :src="fileForm.image_path2"
                      fit="cover">
                    </el-image-->
                </el-form-item>
              </el-form>
            </el-tab-pane>
            <el-tab-pane label="文本" name="text">
            </el-tab-pane>
        </el-tabs>
        </div>
    </div>
</template>

<script>
    import axios from 'axios'
    import headTop from '../components/headTop'
    import {Client} from "@stomp/stompjs"
    import {getSystemInfo, getAlgSupport} from '@/api/getData'
    export default {
        data(){
            return {
                tableData: [],
                algForm: [],
    			fileForm: {
    				image_path: '',
    				image_path2: '',
    			},
                canvas_width: 800,
                canvas_height: 600,
                activeName: 'img',
                task_select: '',
                task_url: 'null',
                head_obj: {head:''},
                last_uid: 0,
                http_file_url: [],
                client: null,
                output: {
                    msgType: 'RabbitMQ',
                    ip: 'localhost',
                    port: '15674',
                    user: 'admin',
                    password: 'admin',
                    _exchange: 'amq.direct',
                    _routingKey: '',
                },
            }
        },
        created(){
            this.initData();
            this.connect()
        },
    	components: {
    		headTop,
    	},
        methods: {
            async initData(){
                try{
                    this.getObjs();
                    this.getAlgSupportt();
                }catch(err){
                    console.log('获取数据失败', err);
                }
            },
            async getObjs(){
                this.tableData = [];
                const _data1 = {};
                _data1.name = '图片';
                this.tableData.push(_data1);
                const _data2 = {};
                _data2.name = '文本';
                this.tableData.push(_data2);
            },
            async getAlgSupportt(){
                this.algForm = [];
                const res = await getAlgSupport({obj: this.activeName});
                res.data.alg.forEach(item => {
                    const algData = {};
                    algData.value = item.name;
                    algData.label = item.name;
                    algData.disabled = item.disabled == 0 ? false : true;
                    const _random = Math.random();
                    const idx = Math.floor(_random*item.url.length);
                    this.http_file_url[item.name] = item.url[idx];
                    this.algForm.push(algData);
                    if(this.task_select == '') {
                        this.task_select = item.name;
                        this.task_url = this.http_file_url[item.name];
                    }
                })
            },
			uploadImg(res, file) {
				if (res.code == 0) {
					this.fileForm.image_path = res.data.img_path;
				}else{
					this.$message.warning('上传图片失败！');
				}
			},
			beforeImgUpload(file) {
                if (this.task_select == '') {
					this.$message.warning('请选择任务');
                    return false;
                }
                this.last_uid = file.uid;
                this.head_obj.head = '{"filesize":'+file.size+',"userdata":{"uid":'+this.last_uid+'}}';
				const isRightType = (file.type === 'image/jpeg') || (file.type === 'image/png');
				const isLt10M = file.size / 1024 / 1024 < 10;
				if (!isRightType) {
					this.$message.warning('上传头像图片只能是JPG 格式');
				}
				if (!isLt10M) {
					this.$message.warning('上传头像图片大小不能超过10MB');
				}
				return isRightType && isLt10M;
			},
            // http-request will replace action/on-success autoly
			uploadRequest(param) {
                const formData = new FormData();
                formData.append('head', param.data.head);
                formData.append('file', param.file);
                axios.post(param.action, formData).then(res => {
				    this.$message.success('上传图片成功');
					//this.fileForm.image_path = res.data.data.img_path;
                    this.fileForm.image_path2 = res.data.data.img_path;
                }).catch(response => {
				    this.$message.warning('上传图片失败');
                })
            },
            handleClick(tab, event) {
                this.getAlgSupportt();
            },
            handleSelect(index) {
                if (index in this.http_file_url) {
                    //this.task_url = "/api/file/upload";
                    this.task_url = this.http_file_url[index];
                }
                else {
                    this.$message.warning('find task url failed: ' + index);
                }
            },
            async connect() {
                const res = await getSystemInfo();
                if(res.code != 0) {
                    console.log("get system info failed")
                    return;
                }
                if(JSON.stringify(res.data.output) == '{}') {
                    return;
                }
                this.output.ip = res.data.output.host;
                if(typeof(res.data.output.port) != "undefined") {
                    this.output.port = String(res.data.output.port);
                }
                this.output.user = res.data.output.username;
                this.output.password = res.data.output.password;
                this.output._exchange = res.data.output.exchange;
                this.output._routingKey = res.data.output.routingkey;
                let url = "ws://" + this.output.ip + ":15674/ws";
                let conf = {
                  brokerURL: url,
                  connectHeaders: {
                    login: this.output.user,
                    passcode: this.output.password
                  }
                }
                this.client = new Client(conf);
                this.client.onConnect = (x) => {
                  let subscription = 
                      this.client.subscribe("/exchange/" + this.output._exchange, this.callback);
                }
                this.client.activate();
            },
            callback:  function (message) {
                let frameJson = JSON.parse(message.body);
                if(this.last_uid != frameJson.data.userdata.uid) {
					this.$message.warning('match failed, last_uid:' + 
                            this.last_uid + ', uid:' + frameJson.data.userdata.uid);
                    return;
                }
                var canvasEl = document.getElementById('myCanvas');
                var ctx = canvasEl.getContext('2d');
                var _this = this;
                //var img = document.getElementById("myImg");
                var img = new Image();
                img.src = frameJson.data.sceneimg.url;
                if(img == null) {
					this.$message.warning('img is null');
                    return;
                }
                img.onload = function() {
                    ctx.drawImage(img, 0, 0, _this.canvas_width, _this.canvas_height);
                    ctx.strokeStyle = 'red';
                    ctx.fillStyle = 'red'
                    ctx.font = "20px Arial bolder"
                    var scale_x = _this.canvas_width/img.width;
                    var scale_y = _this.canvas_height/img.height;
                    frameJson.data.object.forEach(item => {
                        if(item.w == 0 && item.h == 0) {
                            ctx.fillText(item.type, item.x, item.y)
                        }
                        else {
                            ctx.strokeRect(item.x*scale_x, item.y*scale_y, item.w*scale_x, item.h*scale_y);
                            ctx.fillText(item.type, item.x*scale_x, (item.y-4)*scale_y)
                        }
                    })
                }
            },
        },
    }
</script>

<style lang="less">
	@import '../style/mixin';
    .uploader_form_item {
        margin-right: 0;
        margin-bottom: 0;
        width: 20%;
    }
    .table_container{
        padding: 20px;
    }
    .avatar-uploader .el-upload {
        border: 1px dashed #d9d9d9;
        border-radius: 6px;
        cursor: pointer;
        position: relative;
        overflow: hidden;
    }
    .avatar-uploader .el-upload:hover {
        border-color: #20a0ff;
    }
    .avatar-uploader-icon {
        font-size: 28px;
        color: #8c939d;
        width: 60px;
        height: 60px;
        line-height: 60px;
        text-align: center;
    }
    .avatar {
        width: 60px;
        height: 60px;
        display: block;
    }
    #myCanvasxx {
        border: 1px solid rgb(199, 198, 198);
    }
</style>

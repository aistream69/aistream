<template>
    <div class="fillcontain">
        <head-top></head-top>
        <div class="videoList">
              <el-form ref="form" label-width="50px">
                <el-form-item label="设备">
                  <el-select v-model="select" size="small" filterable placeholder="请选择" @change="startVideo">
                    <el-option  v-for="(item,index) in options" 
                    :key="index" 
                    :label="item.name" 
                    :value="item.selectVal" 
                    :disabled="item.disabled"> 
                      <span style="float:left;width;120px" :title='item.name'>{{item.name}}</span>
                    </el-option>
                    <div class="Pagination">
                      <el-pagination
                        @size-change="handleSizeChange"
                        @current-change="handleCurrentChange"
                        :current-page="currentPage"
                        :page-size="20"
                        layout="total, prev, pager, next"
                        :total="count">
                      </el-pagination>
                    </div>
                  </el-select>
                </el-form-item>
              </el-form>
        </div>
        <div class="videobox">
          <el-row type="flex" class="row-bg" style="display: flex">
            <el-col :span="12" class="myVideo">
              <div class="grid-content">
                <video id="videoId" style="object-fit:fill;width:864px;height:486px;margin-left:50px"  
                controls  preload="auto" autoplay muted poster="/img/screen.jpg">
                </video>
              </div>
            </el-col>
            <el-col :span="12" class="myVideo" >
              <div class="grid-content">
                <img v-for="(item, i) in sceneImg" :src=item.sceneimg.url class="sceneimage">
              </div>
            </el-col>
          </el-row>
        </div>
        <div slot="header" class="clearfix">
          <span style="margin-left:10px">实时抓拍</span>
        </div>
        <div style="padding:0 45px" v-for="(item, i) in captureHistoryImg">
          <img :src=item.object[0].url class="captureimage" @click="clickImage(item)">
        </div>
        <div style="padding:0 70px;margin-top:130px" v-for="(item, i) in captureHistoryImg">
          <span style="width:110px;float:left" :title='item.object[0].type'>{{item.object[0].type}}</span>
        </div>
    </div>
</template>

<script>
	import headTop from '../components/headTop'
    import {Client} from "@stomp/stompjs"
    import {getSystemInfo, getObjStatus} from '@/api/getData'
    //import {Hls} from '@/utils/hls.min'
    let Hls = require('hls.js');
    import flvjs from 'flv.js'

    export default {
        data(){
            return {
                client: null,
                hls: null,
                select: '',
                offset: 0,
                limit: 20,
                count: 0,
                currentPage: 1,
                options: [],
                captureHistoryImg: [],
                sceneImg: [],
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
    	components: {
    		headTop,
    	},
        mounted() {
            this.connect()
            this.initData();
        },
        methods: {
            async initData(){
                try{
                    this.getObjs();
                }catch(err){
                    console.log('获取数据失败', err);
                }
            },
            async getObjs(){
                const res = await getObjStatus({type: 'all', offset: this.offset, limit: this.limit});
                this.count = res.data.total;
                this.options = [];
                res.data.obj.forEach(item => {
                    const tableData = {};
                    tableData.name = item.name;
                    tableData.id = item.id;
                    tableData.selectVal = item.id + "-" + item.preview_url;
                    tableData.status = item.status;
                    if(item.status == 0 || item.livestream == 0) {
                        tableData.disabled = true;
                    }
                    else {
                        tableData.disabled = false;
                    }
                    this.options.push(tableData);
                })
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
                var s = this.select.split("-");
                var id = parseInt(s[0]);
                let frameJson = JSON.parse(message.body);
                if(id != frameJson.data.id || frameJson['data']['object'].length < 1) {
                    return;
                }
                if(this.captureHistoryImg.length >= 8) {
                  this.captureHistoryImg.shift();
                }
                this.captureHistoryImg.push(frameJson['data']);
            },
            startVideo(selectVal) {
                var s = selectVal.split("-");
                var _url = s[1];
                if(_url.indexOf("play.m3u8") != -1) {
                    if(Hls.isSupported()) {
                        var video = document.getElementById('videoId');
                        if (this.hls) {
                            video.pause();
                            this.hls.destroy();
                            this.hls = null;
                            //console.log("destroy source ok")
                        }

                        //video.crossOrigin = 'anonymous';
                        //var config = {debug:true};
                        this.hls = new Hls(/*config*/);
                        //console.log("load new source:", _url)
                        this.hls.loadSource(_url);
                        this.hls.attachMedia(video);
                        this.hls.on(Hls.Events.MANIFEST_PARSED, function() {
                            video.play()
                        });
                        //this.hls.on(Hls.Events.ERROR, (event, data) => {
                        //    this.$message({
                        //        type: 'warning',
                        //        message: '加载失败'
                        //    });
                        //    if (this.hls) {
                        //        var video = document.getElementById('videoId');
                        //        video.pause();
                        //        this.hls.destroy();
                        //        this.hls = null;
                        //    }
                        //});
                    }
                }
                else if(_url.indexOf("live?port") != -1) {
                    if(flvjs.isSupported()) {
                        var videoElement = document.getElementById('videoId');
                        var flvPlayer = flvjs.createPlayer({
                            type: 'flv',
                            isLive: true,
                            cors: true,
                            hasAudio: false,
                            //duration: 0,
                            //currentTime: 0,
                            url: _url
                        },
                        {
                            enableStashBuffer: false,
                            //stashInitialSize: 200,
                            //autoCleanupSourceBuffer: true,
                            //autoCleanupMaxBackwardDuration: 12,
                            //autoCleanupMinBackwardDuration: 8,
                        });
                        flvPlayer.attachMediaElement(videoElement);
                        flvPlayer.load();
                        flvPlayer.play();
                        flvPlayer.on(flvjs.Events.ERROR, (errorInfo, errType, errDetail) => {
                            console.log("recv Events.ERROR,", errorInfo, ",", errType, ",", errDetail);
                            flvPlayer.pause();
                            flvPlayer.unload();
                            //flvPlayer.detachMediaElement();
                            //flvPlayer.destroy();
                            //flvPlayer= null;
                            flvPlayer.load();
                            flvPlayer.play();
                        });
                        //flvPlayer.on('statistics_info', function (res) {
                        //});
                    }
                }
                else {
                    this.$message({
                        type: 'warning',
                        message: 'not support preview url format'
                    });
                }
            },
            handleSizeChange(val) {
                console.log(`每页 ${val} 条`);
            },
            handleCurrentChange(val) {
                this.currentPage = val;
                this.offset = (val - 1)*this.limit;
                this.getObjs()
            },
            clickImage(item) {
                this.sceneImg = [];
                this.sceneImg.push(item);
            },
        },
    }
</script>

<style lang="less">
  .bg-box {
    display: flex;

    .videoList {
      width: calc(100% - 800px);
      height: 100px;
      margin: 20px 10px;
      /*background-color: #ccc;*/
      flex: 2;
    }
    .videobox {
      flex: 9;
      .grid-content {
        margin-top: 20px;
        margin-left: -10px;
      }
    }
  }
  .captureimage{
    width: 100px;
    height: 120px;
    margin: 5px;
    float: left;
  }
  .sceneimage{
    width: 640px;
    height: 400px;
    margin-top: 42px;
    margin-left: 130px;
  }
  .Pagination{
      display: flex;
      justify-content: flex-start;
      margin-top: 20px;
  }
</style>

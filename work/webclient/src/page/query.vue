<template>
  <div class="fillcontain">
    <head-top></head-top>
    <el-card>
      <el-row :gutter="20">
        <el-col :span="24">
          <div class="search-area">
            <el-form :inline="true" label-width="80px" class="search-el-form-input-custom">
              <el-row :gutter="20" style="margin: 0;">
                <el-col :span="24" class="search-area-form">
                  <el-form-item label="开始时间">
                    <el-date-picker size="small" type="datetime" placeholder="请选择开始时间"
                                    v-model="startTime" style="width: 100%;">
                    </el-date-picker>
                  </el-form-item>
                  <el-form-item label="结束时间">
                    <el-date-picker size="small" type="datetime" placeholder="请选择结束时间"
                                    v-model="stopTime" style="width: 100%;">
                    </el-date-picker>
                  </el-form-item>
                  <el-form-item label="设备">
                    <template>
                      <el-select v-model="selectId" multiple filterable allow-create collapse-tags 
                                    placeholder="请选择设备" @change="objSelect">
                        <el-checkbox v-model="checked" @change='selectAll($event)'>全选</el-checkbox>
                        <el-option
                          v-for="(item,index) in options" 
                          :key="index" 
                          :label="item.name" 
                          :value="item.id"> 
                        </el-option>
                        <div class="Pagination">
                          <el-pagination
                            @size-change="handleSizeChange"
                            @current-change="handleCurrentChange"
                            :current-page="currentPage"
                            :page-size="20"
                            layout="total, prev, pager, next"
                            :total="objTotal">
                          </el-pagination>
                        </div>
                      </el-select>
                    </template>
                  </el-form-item>
                </el-col>
                <el-col :span="24" style="padding-top:20px;padding-left:0px;">
                  <el-row :gutter="20">
                    <el-col :span="20">
                      <el-button type="primary" 
                                class="search-el-form-button-custom" 
                                @click="queryCapture(1)">查询</el-button>
                    </el-col>
                  </el-row>
                </el-col>
              </el-row>
            </el-form>
          </div>
        </el-col>
        <el-col :span="24">
          <div class="search-result">
            <div class="capture-table">
              <el-row :gutter="20">
                <div 
                style= "float:left;
                        width: 160px;
                        height:234px;
                        margin:6px 10px;
                        text-align:center;
                        background: #E8EEF4;"
                  v-for="(item, index) in captureData"
                  :key="index">
                  <img style="width:160px; height:180px; cursor:pointer;" 
                            :src="getObjUrl(item)" 
                            @click="openSceneImg(item)"
                            :alt="null">
                  <p>{{item.name}}</p>
                  <p>{{timeToDate(item.timestamp)}}</p>
                </div>
              </el-row>
              <div class="block" style="float: left">
                <el-pagination
                  @size-change="handleSizeChanged"
                  @current-change="handleCurrentChanged"
                  :current-page.sync="currentPaged"
                  :page-size="40"
                  layout="total, prev, pager, next"
                  :total='dataTotal'>
                </el-pagination>
              </div>
            </div>
          </div>
        </el-col>
      </el-row>
    </el-card>
  </div>
</template>

<script>
    import headTop from '../components/headTop'
    import {getObjStatus,getObjAllId,queryData} from '@/api/getData'
    let _date = require('silly-datetime');
    export default {
        data(){
            return {
                selectId: [],
                offset: 0,
                limit: 20,
                objTotal: 0,
                dataTotal: 0,
                currentPage: 1,
                offsetd: 0,
                currentPaged: 1,
                limitd: 40,
                checked:false,
                options: [],
                objAllId: [],
                startTime: new Date(new Date().getTime() - 3600*24*1000*3).getTime(), // ms
                stopTime: new Date().getTime(),
                captureData: [],
            }
        },
        created(){
        },
        mounted(){
            this.initData();
            this._getObjAllId();
        },
    	components: {
    		headTop,
    	},
        methods: {
            async initData(){
                this.getObjs();
            },
            async _getObjAllId(){
                try{
                    let res = await getObjAllId();
                    if(res.code != 0) {
                        throw new Error(res.msg)
                    }
                    this.objAllId = res.data.obj;
                }catch(err){
                    this.$message({
                        type: 'error',
                        message: err
                    });
                    console.log('get all id failed')
                }
            },
            async getObjs(){
                const res = await getObjStatus({type: 'all', offset: this.offset, limit: this.limit});
                this.objTotal = res.data.total;
                this.options = [];
                res.data.obj.forEach(item => {
                    const tableData = {};
                    tableData.name = item.name;
                    tableData.id = item.id;
                    this.options.push(tableData);
                })
            },
            async queryCapture(count) {
                if(this.selectId.length <= 0) {
                    this.$message({
                        type: 'warning',
                        message: '请选择设备'
                    });
                    return;
                }
                this.captureData = [];
                try{
                    const json = {
                        type: 'capture',
                        start_time: parseInt(this.startTime/1000),
                        stop_time: parseInt(this.stopTime/1000),
                        id: this.selectId,
                        skip: this.offsetd,
                        limit: this.limitd,
                        need_count: count
                    }
                    let res = await queryData(json);
                    if(res.code != 0) {
                        throw new Error(res.msg)
                    }
                    if(typeof(res.data.total) != "undefined") {
                        this.dataTotal = res.data.total;
                    }
                    this.captureData = res.data.capture;
                }catch(err){
                    this.$message({
                        type: 'error',
                        message: err
                    });
                    console.log('数据查询失败')
                }
            },
            objSelect(selectVal) {
            },
            selectAll(val) {
                if(this.checked) {
                    this.selectId = this.objAllId;
                }
                else {
                    this.selectId = [];
                }
            },
            handleSizeChange(val) {
                console.log(`每页 ${val} 条`);
            },
            handleCurrentChange(val) {
                this.currentPage = val;
                this.offset = (val - 1)*this.limit;
                this.getObjs();
            },
            handleSizeChanged(val) {
                console.log(`每页 ${val} 条`);
            },
            handleCurrentChanged(val) {
                this.currentPaged = val;
                this.offsetd = (val - 1)*this.limitd;
                this.queryCapture(0);
            },
            getObjUrl(item) {
                if(item.object.length > 0 && typeof(item.object[0].url) != "undefined") {
                    return item.object[0].url;
                }
                else {
                    return "http://ip:port/null";
                }
            },
            openSceneImg(item) {
                window.open(item.sceneimg.url, '_blank')
            },
            timeToDate(t) {
                return _date.format(new Date(t*1000),'YYYY-MM-DD HH:mm:ss');
            }
        },
    }
</script>

<style lang="less">
	@import '../style/mixin';
    .search-area-form {
      padding-top: 10px;
      padding-bottom: 10px;
      background: #E8EEF4;
      border: 1px solid #B6C2C9;
    }
    .search-result{
      margin-top: 30px;
      padding: 10px;
    }
    .Pagination{
        display: flex;
        justify-content: flex-start;
        margin-top: 20px;
    }
</style>

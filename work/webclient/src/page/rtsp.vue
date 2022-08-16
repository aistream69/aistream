<template>
    <div class="fillcontain">
        <head-top></head-top>
        <div class="table_container">
            <el-table
                :data="tableData"
                style="width: 100%">
                <el-table-column type="expand">
                  <template slot-scope="props">
                    <el-form label-position="left" inline class="demo-table-expand">
                      <el-form-item label="关联任务">
                          <el-select v-model="props.row.alg" placeholder="请选择">
                              <el-option
                                  v-for="item in algForm"
                                  :key="item.value"
                                  :label="item.label"
                                  :value="item.value"
                                  :disabled="item.disabled">
                              </el-option>
                          </el-select>
                      </el-form-item>
                      <el-form-item label="视频预览">
                          <el-select v-model="props.row.preview" placeholder="请选择">
                              <el-option
                                  v-for="item in previewForm"
                                  :key="item.value"
                                  :label="item.label"
                                  :value="item.value"
                                  :disabled="item.disabled">
                              </el-option>
                          </el-select>
                      </el-form-item>
                      <el-form-item label="视频录像" style="white-space: nowrap;">
                          <el-switch on-text="" off-text="" v-model="props.row.recordEnable" disabled></el-switch>
                      </el-form-item>
                    </el-form>
                  </template>
                </el-table-column>
                <el-table-column
                  label="设备名称"
                  prop="name">
                </el-table-column>
                <el-table-column
                  label="设备ID"
                  prop="id">
                </el-table-column>
                <el-table-column
                  label="视频流地址"
                  prop="url">
                </el-table-column>
                <el-table-column label="操作" width="260">
                  <template slot-scope="scope">
                    <el-button
                      size="mini"
                      @click="handleEdit(scope.$index, scope.row)">编辑</el-button>
                    <el-button
                      size="mini"
                      :type=scope.row.button_type
                      @click="handleStart(scope.$index, scope.row)">
                      {{scope.row.button_text}}</el-button>
                    <el-button
                      size="mini"
                      type="primary"
                      @click="handleStop(scope.$index, scope.row)">停止</el-button>
                    <el-button
                      size="mini"
                      type="danger"
                      @click="handleDelete(scope.$index, scope.row)">删除</el-button>
                  </template>
                </el-table-column>
            </el-table>
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
            <div class="button_add">
                <el-button size="small" type="primary" @click="addRtsp()">添加设备</el-button>
            </div>
            <el-dialog title="修改设备信息" v-model="dialogFormVisible">
                <el-form :model="selectTable">
                    <el-form-item label="设备名称" label-width="100px">
                        <el-input v-model="selectTable.name" auto-complete="off"></el-input>
                    </el-form-item>
                    <el-form-item label="设备ID" label-width="100px">
                        <el-input v-model="selectTable.id"></el-input>
                    </el-form-item>
                    <el-form-item label="视频流地址" label-width="100px">
                        <el-input v-model="selectTable.url"></el-input>
                    </el-form-item>
                    <el-form-item label="TCP使能" style="white-space: nowrap;">
                        <el-switch on-text="" off-text="" v-model="selectTable.tcp_enable"></el-switch>
                    </el-form-item>
                </el-form>
              <div slot="footer" class="dialog-footer">
                <el-button @click="dialogFormVisible = false">取 消</el-button>
                <el-button type="primary" @click="updateRtsp">确 定</el-button>
              </div>
            </el-dialog>
        </div>
    </div>
</template>

<script>
    import headTop from '../components/headTop'
    import {startAlg, stopAlg, getObjStatus, getAlgSupport, deleteObj} from '@/api/getData'
    export default {
        data(){
            return {
                offset: 0,
                limit: 20,
                count: 0,
                tableData: [],
                currentPage: 1,
                selectTable: {},
                dialogFormVisible: false,
                previewForm: [{
                    value: 'none',
                    label: 'none'
                }, {
                    value: 'hls',
                    label: 'hls',
                }, {
                    value: 'http-flv',
                    label: 'http-flv',
                },],
                algForm: [],
            }
        },
        created(){
            this.initData();
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
                const res = await getObjStatus({type: 'rtsp', offset: this.offset, limit: this.limit});
                this.count = res.data.total;
                this.tableData = [];
                res.data.obj.forEach(item => {
                    const tableData = {};
                    tableData.name = item.name;
                    tableData.id = item.id;
                    tableData.url = item.url;
                    tableData.tcp_enable = item.tcp_enable == 1 ? true : false;
                    tableData.preview = item.preview;
                    tableData.alg = item.alg;
                    tableData.status = item.status;
                    tableData.recordEnable = false;

                    if(tableData.status == 0) {
                        tableData.button_type = "primary";
                        tableData.button_text = '启动';
                    }
                    else if(tableData.status == 1) {
                        tableData.button_type = "success";
                        tableData.button_text = '运行中';
                    }
                    else {
                        tableData.button_type = "warning";
                        tableData.button_text = '故障';
                    }
                    this.tableData.push(tableData);
                })
            },
            async getAlgSupportt(){
                const res = await getAlgSupport({obj: 'video'});
                this.algForm = [];
                res.data.alg.forEach(item => {
                    const algData = {};
                    algData.value = item.name;
                    algData.label = item.name;
                    algData.disabled = item.disabled == 0 ? false : true;
                    this.algForm.push(algData);
                })
            },
            handleSizeChange(val) {
                console.log(`每页 ${val} 条`);
            },
            handleCurrentChange(val) {
                this.currentPage = val;
                this.offset = (val - 1)*this.limit;
                this.getObjs()
            },
            handleEdit(index, row) {
                this.selectTable = row;
                this.dialogFormVisible = true;
            },
            async handleStart(index, row) {
                if(row.status) {
                    return;
                }
                if(row.alg == 'none') {
                    this.$message({
                        type: 'warning',
                        message: '请选择任务'
                    });
                    return;
                }
                try{
                    const json = {
                        id: row.id,
                        data: {
                            task: row.alg,
                            params: {
                                preview: row.preview
                            }
                        }
                    }
                    let res = await startAlg(json);
                    if (res.code != 0) {
                        throw new Error(res.msg)
                    }
                    row.button_type = "success";
                    row.button_text = '运行中';
                    row.status = 1;
                }catch(err){
                    this.$message({
                        type: 'error',
                        message: err.message
                    });
                    console.log('启动失败')
                }
            },
            async handleStop(index, row) {
                try{
                    const json = {
                        id: row.id,
                        data: {
                            task: row.alg
                        }
                    }
                    let res = await stopAlg(json);
                    if (res.code != 0) {
                        throw new Error(res.msg)
                    }
                    row.status = 0;
                    row.button_type = "primary";
                    row.button_text = '启动';
                }catch(err){
                    this.$message({
                        type: 'error',
                        message: err.message
                    });
                    console.log('停止失败')
                }
            },
            async handleDelete(index, row) {
                try{
                    const json = {
                        id: row.id,
                    }
					let res = await deleteObj(json);
                    if (res.code == 0) {
                        this.$message({
                            type: 'success',
                            message: '删除成功'
                        });
                        this.tableData.splice(index, 1);
                    }else{
                        throw new Error(res.msg)
                    }
                }catch(err){
                    this.$message({
                        type: 'error',
                        message: err.message
                    });
                    console.log('删除失败')
                }
            },
            addRtsp(){
                this.$router.push('/addRtsp');
            },
            async updateRtsp(){
                this.dialogFormVisible = false;
                try{
                    this.$message({
                        type: 'warning',
                        message: '暂不支持编辑，请直接删除/添加'
                    });
                    this.getObjs();
                    //const res = await updateRtsp(this.selectTable)
                    //if (res.status == 1) {
                    //    this.$message({
                    //        type: 'success',
                    //        message: '更新成功'
                    //    });
                    //    this.getSlaveStatus();
                    //}else{
                    //    this.$message({
                    //        type: 'error',
                    //        message: res.message
                    //    });
                    //}
                }catch(err){
                    console.log('更新失败', err);
                }
            },
        },
    }
</script>

<style lang="less">
	@import '../style/mixin';
	.button_add{
		text-align: right;
        margin-top: -32px;
	}
    .demo-table-expand {
        font-size: 0;
    }
    .demo-table-expand label {
        width: 90px;
        color: #555555;
    }
    .demo-table-expand .el-form-item {
        margin-right: 0;
        margin-bottom: 0;
        width: 50%;
    }
    .table_container{
        padding: 20px;
    }
    .Pagination{
        display: flex;
        justify-content: flex-start;
        margin-top: 20px;
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
    .avatar {
        width: 120px;
        height: 120px;
        display: block;
    }
</style>

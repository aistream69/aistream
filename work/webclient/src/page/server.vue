<template>
    <div class="fillcontain">
        <head-top></head-top>
        <div class="table_container">
            <el-table
                :data="tableData"
                style="width: 100%">
                <el-table-column
                  label="名称"
                  prop="name">
                </el-table-column>
                <el-table-column
                  label="IP地址"
                  prop="ip">
                </el-table-column>
                <el-table-column
                  label="端口号"
                  prop="port">
                </el-table-column>
                <el-table-column
                  label="负载"
                  prop="load">
                </el-table-column>
                <el-table-column label="操作" width="260">
                  <template slot-scope="scope">
                    <el-button
                      size="mini"
                      @click="handleEdit(scope.$index, scope.row)">编辑</el-button>
                    <el-button
                      size="mini"
                      :type=scope.row.button_type>
                      {{scope.row.button_text}}</el-button>
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
                <el-button size="small" type="primary" @click="addSlave()">添加服务器</el-button>
            </div>
            <el-dialog title="修改服务器信息" v-model="dialogFormVisible">
                <el-form :model="selectTable">
                    <el-form-item label="名称" label-width="100px">
                        <el-input v-model="selectTable.name" auto-complete="off"></el-input>
                    </el-form-item>
                    <el-form-item label="IP地址" label-width="100px">
                        <el-input v-model="selectTable.ip"></el-input>
                    </el-form-item>
                    <el-form-item label="端口号" label-width="100px">
                        <el-input v-model="selectTable.port"></el-input>
                    </el-form-item>
                    <el-form-item label="外网IP地址" label-width="100px">
                        <el-input v-model="selectTable.internet_ip"></el-input>
                    </el-form-item>
                </el-form>
              <div slot="footer" class="dialog-footer">
                <el-button @click="dialogFormVisible = false">取 消</el-button>
                <el-button type="primary" @click="updateSlave">确 定</el-button>
              </div>
            </el-dialog>
        </div>
    </div>
</template>

<script>
    import headTop from '../components/headTop'
    import {getSlave, delSlave} from '@/api/getData'
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
                    this.getSlaveStatus();
                }catch(err){
                    console.log('获取数据失败', err);
                }
            },
            async getSlaveStatus(){
                const res = await getSlave();
                if(res.code != 0) {
                    return;
                }
                this.count = res.data.total;
                this.tableData = [];
                res.data.slave.forEach(item => {
                    const tableData = {};
                    tableData.name = item.name;
                    tableData.ip = item.ip;
                    tableData.port = item.port;
                    tableData.internet_ip = item.internet_ip;
                    tableData.load = item.load+'%';
                    tableData.status = item.status;
                    if(tableData.status == 1) {
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
            handleSizeChange(val) {
                console.log(`每页 ${val} 条`);
            },
            handleCurrentChange(val) {
                this.currentPage = val;
                this.offset = (val - 1)*this.limit;
                this.getSlaveStatus()
            },
            handleEdit(index, row) {
                this.selectTable = row;
                this.dialogFormVisible = true;
            },
            addSlave(){
                this.$router.push('/addSlave');
            },
            async handleDelete(index, row) {
                try{
                    const json = {
                        ip: row.ip,
                        rest_port: row.port
                    }
					let res = await delSlave(json);
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
            async updateSlave(){
                this.dialogFormVisible = false;
                try{
                    this.$message({
                        type: 'warning',
                        message: '暂不支持编辑，请直接删除/添加'
                    });
                    this.getSlaveStatus();
                    //const res = await updateResturant(this.selectTable)
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
        color: #99a9bf;
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

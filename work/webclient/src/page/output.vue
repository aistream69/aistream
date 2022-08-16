<template>
    <div>
        <head-top></head-top>
        <el-row style="margin-top: 20px;">
  			<el-col :span="12" :offset="4">
		        <el-form :model="formData" :rules="rules" ref="formData" label-width="110px" class="demo-formData">
					<el-form-item label="输出类型" prop="msgType">
						<el-input v-model="formData.msgType" :disabled="true"></el-input>
					</el-form-item>
					<el-form-item label="服务器IP地址" prop="ip">
						<el-input placeholder="必填" v-model="formData.ip"></el-input>
					</el-form-item>
					<el-form-item label="端口号" prop="port">
						<el-input placeholder="必填" v-model="formData.port"></el-input>
					</el-form-item>
					<el-form-item label="用户名" prop="user">
						<el-input placeholder="必填" v-model="formData.user"></el-input>
					</el-form-item>
					<el-form-item label="密码" prop="password">
						<el-input placeholder="必填" v-model="formData.password"></el-input>
					</el-form-item>
					<el-form-item label="exchange" prop="_exchange">
						<el-input placeholder="必填" v-model="formData._exchange"></el-input>
					</el-form-item>
					<el-form-item label="routingkey" prop="_routingKey">
						<el-input placeholder="可选" v-model="formData._routingKey"></el-input>
					</el-form-item>

					<el-form-item class="button_submit">
						<el-button type="primary" @click="submitForm('formData')">确定</el-button>
					</el-form-item>
				</el-form>
  			</el-col>
  		</el-row>
    </div>
</template>

<script>
    import headTop from '@/components/headTop'
    import {setOutput, getSystemInfo} from '@/api/getData'
    export default {
    	data(){
    		return {
    			formData: {
                    msgType: 'RabbitMQ',
                    ip: '',
                    port: '',
                    user: '',
                    password: '',
                    _exchange: '',
                    _routingKey: '',
		        },
		        rules: {
					ip: [
						{ required: true, message: '请输入服务器IP地址', trigger: 'blur' },
					],
					port: [
						{ required: true, message: '请输入端口号', trigger: 'blur' }
					],
					user: [
						{ required: true, message: '请输入用户名', trigger: 'blur' }
					],
					password: [
						{ required: true, message: '请输入密码', trigger: 'blur' }
					],
					_exchange: [
						{ required: true, message: '请输入exchange', trigger: 'blur' }
					],
				},
    		}
    	},
    	components: {
    		headTop,
    	},
        created(){
            this.initData();
        },
    	methods: {
		    submitForm(formName) {
				this.$refs[formName].validate(async (valid) => {
					if(valid) {
						try{
                            const json = {
                                type: 'mq',
                                data: {
                                    host: this.formData.ip,
                                    port: parseInt(this.formData.port),
                                    username: this.formData.user,
                                    password: this.formData.password,
                                    exchange: this.formData._exchange,
                                    routingkey: typeof(this.formData._routingKey) == 'undefined' ? '' : this.formData._routingKey
                                }
                            }
							let result = await setOutput(json);
							if (result.code == 0) {
								this.$message({
					            	type: 'success',
					            	message: '设置成功'
					          	});
							}else{
								this.$message({
					            	type: 'error',
					            	message: result.msg
					          	});
							}
						}catch(err){
							console.log(err)
						}
					} else {
						this.$notify.error({
							title: '错误',
							message: '请检查输入是否正确',
							offset: 100
						});
						return false;
					}
				});
			},
            async initData(){
                try{
                    this.getOutput();
                }catch(err){
                    console.log('获取数据失败', err);
                }
            },
            async getOutput(){
                const res = await getSystemInfo();
				if(res.code != 0) {
                    return;
                }
                this.formData.ip = res.data.output.host;
                if(typeof(res.data.output.port) != "undefined") {
                    this.formData.port = String(res.data.output.port);
                }
                this.formData.user = res.data.output.username;
                this.formData.password = res.data.output.password;
                this.formData._exchange = res.data.output.exchange;
                this.formData._routingKey = res.data.output.routingkey;
            },
		}
    }
</script>

<style lang="less">
	@import '../style/mixin';
	.button_submit{
		text-align: center;
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
	.el-table .info-row {
	    background: #c9e5f5;
	}

	.el-table .positive-row {
	    background: #e2f0e4;
	}
</style>

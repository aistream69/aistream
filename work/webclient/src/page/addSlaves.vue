<template>
    <div>
        <head-top></head-top>
        <el-row style="margin-top: 20px;">
  			<el-col :span="12" :offset="4">
		        <el-form :model="formData" :rules="rules" ref="formData" label-width="110px" class="demo-formData">
					<el-form-item label="名称" prop="name">
						<el-input placeholder="可选" v-model="formData.name"></el-input>
					</el-form-item>
					<el-form-item label="IP地址" prop="ip">
						<el-input placeholder="必填" v-model="formData.ip"></el-input>
					</el-form-item>
					<el-form-item label="端口号" prop="port">
						<el-input placeholder="必填" v-model="formData.port"></el-input>
					</el-form-item>
					<el-form-item label="外网IP地址" prop="internet_ip">
						<el-input placeholder="云端部署时此处可填写服务器外网IP" v-model="formData.internet_ip"></el-input>
					</el-form-item>

					<el-form-item class="button_submit">
						<el-button type="primary" @click="submitForm('formData')">立即创建</el-button>
					</el-form-item>
				</el-form>
  			</el-col>
  		</el-row>
    </div>
</template>

<script>
    import headTop from '@/components/headTop'
    import {addSlave} from '@/api/getData'
    export default {
    	data(){
    		return {
    			formData: {
                    name: '',
                    ip: '',
                    port: '',
                    internet_ip: '',
		        },
		        rules: {
					ip: [
						{ required: true, message: '请输入服务器IP地址', trigger: 'blur' },
					],
					port: [
						{ required: true, message: '请输入服务器端口号', trigger: 'blur' }
					],
				},
    		}
    	},
    	components: {
    		headTop,
    	},
        watch: {
            $route (to, from) {
                this.$router.go(0)
            }
        },
    	methods: {
		    submitForm(formName) {
				this.$refs[formName].validate(async (valid) => {
					if (valid) {
						try{
                            const json = {
                                name: this.formData.name,
                                ip: this.formData.ip,
                                rest_port: parseInt(this.formData.port),
                                internet_ip: this.formData.internet_ip
                            }
							let result = await addSlave(json);
							if (result.code == 0) {
								this.$message({
					            	type: 'success',
					            	message: '添加成功'
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
		            this.$router.push('/server');
				});
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

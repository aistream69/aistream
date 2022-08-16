<template>
    <div>
        <head-top></head-top>
        <el-row style="margin-top: 20px;">
  			<el-col :span="12" :offset="4">
		        <el-form :model="formData" :rules="rules" ref="formData" label-width="110px" class="demo-formData">
					<el-form-item label="设备名称" prop="name">
						<el-input placeholder="必填" v-model="formData.name"></el-input>
					</el-form-item>
					<el-form-item label="设备ID" prop="id">
						<el-input placeholder="必填" v-model="formData.id"></el-input>
					</el-form-item>
					<el-form-item label="视频流地址" prop="rtsp_addr">
						<el-input placeholder="必填" v-model="formData.rtsp_addr"></el-input>
					</el-form-item>
					<el-form-item label="TCP使能" style="white-space: nowrap;">
                        <el-switch on-text="" off-text="" v-model="formData.tcp_enable"></el-switch>
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
    import {addRtsp} from '@/api/getData'
    export default {
    	data(){
    		return {
    			formData: {
                    name: '',
                    id: '',
                    rtsp_addr: '',
                    tcp_enable: false,
		        },
		        rules: {
					name: [
						{ required: true, message: '请输入设备名称', trigger: 'blur' },
					],
					id: [
						{ required: true, message: '请输入设备ID', trigger: 'blur' },
					],
					rtsp_addr: [
						{ required: true, message: '请输入RTSP地址', trigger: 'blur' }
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
                                id: parseInt(this.formData.id),
                                data: {
                                    tcp_enable: this.formData.tcp_enable == true ? 1 : 0,
                                    url: this.formData.rtsp_addr
                                }
                            }
							let result = await addRtsp(json);
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
		            this.$router.push('/rtsp');
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

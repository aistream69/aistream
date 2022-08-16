<template>
	<div class="manage_page fillcontain">
		<el-row style="height: 100%;">
	  		<el-col :span="4"  style="min-height: 100%; background-color: #324057;">
				<el-menu :default-active="defaultActive" style="min-height: 100%;" theme="dark" router>
					<el-menu-item index="manage"><i class="el-icon-menu"></i>首页</el-menu-item>
					<el-submenu index="2">
						<template slot="title"><i class="el-icon-setting"></i>视频预览</template>
						<el-menu-item index="preview">视频预览</el-menu-item>
					</el-submenu>
					<el-submenu index="3">
						<template slot="title"><i class="el-icon-setting"></i>设备管理</template>
						<el-menu-item index="rtsp">RTSP</el-menu-item>
						<el-menu-item index="rtmp">RTMP</el-menu-item>
						<el-menu-item index="file">FILE</el-menu-item>
						<!--el-menu-item index="gb28181">GB28181</el-menu-item-->
						<!--el-menu-item index="audioDevice">音频</el-menu-item-->
					</el-submenu>
					<el-submenu index="4">
						<template slot="title"><i class="el-icon-setting"></i>集群管理</template>
						<el-menu-item index="server">服务器</el-menu-item>
					</el-submenu>
					<el-submenu index="5">
						<template slot="title"><i class="el-icon-setting"></i>系统管理</template>
						<el-menu-item index="output">输出配置</el-menu-item>
						<el-menu-item index="userManage">用户管理</el-menu-item>
						<el-menu-item index="docInfo">文档信息</el-menu-item>
						<el-menu-item index="systemInfo">系统信息</el-menu-item>
					</el-submenu>
				</el-menu>
			</el-col>
			<el-col :span="20" style="height: 100%;overflow: auto;">
				<keep-alive>
				    <router-view></router-view>
				</keep-alive>
			</el-col>
		</el-row>
  	</div>
</template>

<script>
    export default {
		computed: {
			defaultActive: function(){
				return this.$route.path.replace('/', '');
			}
		},
    }
</script>


<style lang="less" scoped>
	@import '../style/mixin';
	.manage_page{
		
	}
</style>

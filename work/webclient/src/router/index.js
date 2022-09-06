import Vue from 'vue'
import Router from 'vue-router'

Vue.use(Router)

const login = r => require.ensure([], () => r(require('@/page/login')), 'login');
const manage = r => require.ensure([], () => r(require('@/page/manage')), 'manage');
const home = r => require.ensure([], () => r(require('@/page/home')), 'home');
const rtsp = r => require.ensure([], () => r(require('@/page/rtsp')), 'rtsp');
const addRtsp = r => require.ensure([], () => r(require('@/page/addRtsp')), 'addRtsp');
const rtmp = r => require.ensure([], () => r(require('@/page/rtmp')), 'rtmp');
const addRtmp = r => require.ensure([], () => r(require('@/page/addRtmp')), 'addRtmp');
const file = r => require.ensure([], () => r(require('@/page/file')), 'file');
const gb28181 = r => require.ensure([], () => r(require('@/page/gb28181')), 'gb28181');
const audioDevice = r => require.ensure([], () => r(require('@/page/audio')), 'audioDevice');
const userManage = r => require.ensure([], () => r(require('@/page/adminList')), 'userManage');
const server = r => require.ensure([], () => r(require('@/page/server')), 'server');
const query = r => require.ensure([], () => r(require('@/page/query')), 'query');
const addSlave = r => require.ensure([], () => r(require('@/page/addSlaves')), 'addSlave');
const systemInit = r => require.ensure([], () => r(require('@/page/systemInit')), 'systemInit');
const output = r => require.ensure([], () => r(require('@/page/output')), 'output');
const docInfo = r => require.ensure([], () => r(require('@/page/docInfo')), 'docInfo');
const systemInfo = r => require.ensure([], () => r(require('@/page/systemInfo')), 'systemInfo');
const preview = r => require.ensure([], () => r(require('@/page/preview')), 'preview');
const test = r => require.ensure([], () => r(require('@/page/test')), 'test');

const routes = [
	{
		path: '/',
		component: login
	},
	{
		path: '/manage',
		component: manage,
		name: '',
		children: [{
			path: '',
			component: home,
			meta: [],
		},{
			path: '/preview',
			component: preview,
			meta: ['视频预览', '视频预览'],
		},{
			path: '/rtsp',
			component: rtsp,
			meta: ['设备管理', 'RTSP'],
		},{
			path: '/addRtsp',
			component: addRtsp,
			meta: ['设备管理', '添加RTSP'],
		},{
			path: '/rtmp',
			component: rtmp,
			meta: ['设备管理', 'RTMP'],
		},{
			path: '/addRtmp',
			component: addRtmp,
			meta: ['设备管理', '添加RTMP'],
		},{
			path: '/file',
			component: file,
			meta: ['设备管理', 'FILE'],
		},{
			path: '/gb28181',
			component: gb28181,
			meta: ['设备管理', 'GB28181'],
		},{
			path: '/audioDevice',
			component: audioDevice,
			meta: ['设备管理', '音频'],
		},{
			path: '/query',
			component: query,
			meta: ['数据管理', '查询'],
		},{
			path: '/server',
			component: server,
			meta: ['集群管理', '服务器'],
		},{
			path: '/addSlave',
			component: addSlave,
			meta: ['集群管理', '添加服务器'],
		},{
			path: '/userManage',
			component: userManage,
			meta: ['系统管理', '用户'],
		},{
			path: '/systemInit',
			component: systemInit,
			meta: ['系统管理', '系统初始化'],
		},{
			path: '/docInfo',
			component: docInfo,
			meta: ['系统管理', '文档信息'],
		},{
			path: '/systemInfo',
			component: systemInfo,
			meta: ['系统管理', '系统信息'],
		},{
			path: '/output',
			component: output,
			meta: ['系统管理', '输出配置'],
		},{
			path: '/test',
			component: test,
			meta: ['系统管理', '测试页面'],
    }]
	}
]

export default new Router({
	routes,
	strict: process.env.NODE_ENV !== 'production',
})

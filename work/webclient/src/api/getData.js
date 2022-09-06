import fetch from '@/config/fetch'

/**
 * 登陆
 */
export const login = data => fetch('/api/system/login', data, 'POST');

/**
 * 退出
 */
export const signout = () => fetch('/api/system/logout');

/**
 * 获取用户信息
 */
export const getAdminInfo = () => fetch('/api/admin/info');

/**
 * 设置系统输出
 */
export const setOutput = data => fetch('/api/system/set/output', data, 'POST');

/**
 * 获取系统信息
 */
export const getSystemInfo = () => fetch('/api/system/get/info');

/**
 * 添加slave
 */
export const addSlave = data => fetch('/api/system/slave/add', data, 'POST');

/**
 * 删除slave
 */
export const delSlave = data => fetch('/api/system/slave/del', data, 'POST');

/**
 * 获取slave列表
 */
export const getSlave = () => fetch('/api/system/slave/status');

/**
 * 添加RTSP设备
 */
export const addRtsp = data => fetch('/api/obj/add/rtsp', data, 'POST');
/**
 * 添加RTMP设备
 */
export const addRtmp = data => fetch('/api/obj/add/rtmp', data, 'POST');
/**
 * 开始算法任务
 */
export const startAlg = data => fetch('/api/task/start', data, 'POST');

/**
 * 停止算法任务
 */
export const stopAlg = data => fetch('/api/task/stop', data, 'POST');

/**
 * 获取支持算法
 */
export const getAlgSupport = data => fetch('/api/task/support', data);

/**
 * 获取设备列表
 */
export const getObjStatus = data => fetch('/api/obj/status', data);
export const getObjAllId = data => fetch('/api/obj/id/all', data);

/**
 * 删除设备
 */
export const deleteObj = data => fetch('/api/obj/del', data, 'POST');

/**
 * 数据查询
 */
export const queryData = data => fetch('/api/data/query', data, 'POST');



<template>
    <div class="fillcontain">
        <head-top></head-top>
        <header class="admin_title"></header>
        <div class="admin_set">
            <ul>
                <li>
                    <span>版本号：</span><span>{{version}}</span>
                </li>
                <li>
                    <span>编译时间：</span><span>{{build_time}}</span>
                </li>
            </ul>
        </div>
    </div>
</template>

<script>
	import headTop from '../components/headTop'
    import {getSystemInfo} from '@/api/getData'

    export default {
        data(){
            return {
                version: '',
                build_time: '',
            }
        },
    	components: {
    		headTop,
    	},
        created(){
            this.getSystemInfoo();
        },
        methods: {
            async getSystemInfoo(){
                const res = await getSystemInfo();
                this.version = res.data.build.version;
                this.build_time = res.data.build.time;
            },
        },
    }
</script>

<style lang="less">
	@import '../style/mixin';
	.explain_text{
		margin-top: 20px;
		text-align: center;
		font-size: 20px;
		color: #333;
	}
    .admin_set{
        width: 60%;
        background-color: #F9FAFC;
        min-height: 400px;
        margin: 20px auto 0;
        border-radius: 10px;
        ul > li{
            padding: 20px;
            span{
                color: #666;
            }
        }
    }
    .admin_title{
        margin-top: 20px;
        .sc(24px, #666);
        text-align: center;
    }
</style>

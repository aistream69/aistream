<template>
    <div>
        <head-top></head-top>
        <el-row style="margin-top:20px;margin-left:40px;">
            <el-button type="primary" size="medium" @click='apiClick()'>Restful API</el-button>
  		</el-row>
    </div>
</template>

<script>
    import headTop from '@/components/headTop'
    export default {
    	data(){
    		return {
    		}
    	},
    	components: {
    		headTop,
    	},
    	methods: {
            apiClick() {
                var _url = "http://" + window.location.host + "/restfulapi"
                //window.location.href = _url;
                window.open(_url, '_blank')
            },
		}
    }
</script>

<style lang="less">
	@import '../style/mixin';
</style>

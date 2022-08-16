<template>
    <div >
        <el-select v-model="taskId" size="small" @change="startVideo">
            <el-option  v-for="(item, index) in restoreTable" :key="index" :label="item.name" :value="item.id">
                <span style="float: left;width;120px" :title='item.name'>{{item.name}}</span>
            </el-option>
            <!--
            <div style="text-align:center">
                <span class="text" @click.stop="prevePage">上一页</span>
                <span class="text" style="padding-left: 30px" @click.stop="nextPage" v-show='selectPage != pageCount'>下一页</span>
            </div>
            -->
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
        </el-select>
    </div>
</template>

<script>
export default {
  data () {
    return {
      offset: 0,
      limit: 20,
      count: 21,
      total: null,
      pageCount: null,
      selectPage: 1,
      currentPage: 1,
      restoreTable: [],
      taskId: '',
    };
  },
  mounted () {
    this.getTableList(); // 初始化
  },
  methods: {
    startVideo(url) {
        console.log("startVideo", url);
    },
    handleSizeChange(val) {
        console.log(`每页 ${val} 条`);
    },
    handleCurrentChange(val) {
        this.currentPage = val;
        this.offset = (val - 1)*this.limit;
    },
    getTableList (form = {}) {
      let obj = Object.assign({},
                        {currentPage:this.selectPage,pageSize: 20}, form);
      this.restoreTable = [{name:'thomas',id:1}, {name:'thomas2',id:2}];
      this.total = 100;
      this.pageCount = Math.ceil(this.total/20);
    },
    prevePage () {
     console.log("prevePage");
      --this.selectPage;
      if (this.selectPage < 1) { // 判断点击的页数是否小于1
        this.selectPage = 1;
      };
      this.getTableList();
    },
    nextPage () {
      console.log("nextPage");
      if (this.selectPage < this.pageCount) { // 判断点击的页数是否小于总页数;
        ++this.selectPage;
        this.getTableList();
      }
    }
  }
};
</script>

<style lang='less' scoped>
    .Pagination{
        display: flex;
        justify-content: flex-start;
        margin-top: 20px;
    }
    .selectJob{
        .xspan
            width 120px
            overflow hidden
            text-overflow ellipsis
            white-space nowrap
        .text
            padding-left 10px
            font-size 14px
            font-weight bold
            cursor pointer
            color cornflowerblue
    }
<style>



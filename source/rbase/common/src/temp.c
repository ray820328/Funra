/**

- 错误：
0. rlog初始化过程中直接或间接调用rinfo等rlog接口
0. 没有调用 rmutex_init，调用了 rmutex_uninit，崩溃
0. % 判0，rdict_release，buckets置0导致get操作崩溃
0. int* sent_len; 所有类型都要rdata_new
0. 宏参数和宏内字段名重名
0. 依赖lua库luaH_getn等使用link失败,elf文件有export，__attribute__((visibility("internal"))) extern
   so必须指定对应平台如: LUA_USE_LINUX
0. win10写utf8文件 "w+,ccs=UTF-8" 任意字符奔溃
 */

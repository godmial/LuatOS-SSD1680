记住一条：所有代码都紧贴着eink！！！

1.源代码下载下来之后放在LUATOS\components里

2.找到 LuatOS\luat\include\luat_libs.h 文件，添加如下代码(建议贴在合宙官方的eink驱动附近):

`LUAMOD_API int luaopen_rink( lua_State *L);`

3.在luatos-soc-2022\project\luatos\xmake.lua(注意这里是soc文件夹，不再是LUATOS文件夹)里添加如下代码(仍然建议贴着eink)：

`add_includedirs(LUATOS_ROOT.."components/rink")`

`add_files(LUATOS_ROOT.."components/rink/*.c")`

4.在luatos-soc-2022\project\luatos\src\luat_base_ec618.c里添加:

`{"rink", luaopen_rink},`

5.结束，编译烧录即可。



ps：由于本人已从上一家公司离职，push之前剔除了所有业务代码，手头目前没有开发板供验证程序是否正常运行，有问题提issue。



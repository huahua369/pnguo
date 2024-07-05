# pnguo vkgui
基于SDL3的简易gui实现。

编译说明：
1. 拉取仓库代码
```
git clone https://github.com/huahua369/pnguo.git
```
更新代码
```
git pull
```
2. CMakeLists.txt修改`SDL3DIR_ROOT`和`SDL3_DIR`需要自行编译SDL3，本代码依赖库为静态库，如要改成引用动态库则改txt前面set(VCPKG_TARGET_TRIPLET x64-windows)
3. vcpkg安装依赖库，根据需求安装，例如```vcpkg install PkgConfig:x64-windows-static```
```
PkgConfig
SDL3
OpenSSL
glm
Stb
nlohmann_json
Vulkan
ZLIB
pango
cairo
fontconfig
harfbuzz
EXPRTK
Clipper2 Clipper2Z
libuv
LIBRSVG
GTHREAD
EnTT
OpenCV
vkvg
```

## To Do 

- [x] 2d骨骼动画渲染
- [x] 2d图集渲染
- [x] flex布局算法
- [x] 字体渲染管理
- [x] svg加载，支持滤镜
- [x] 面板
- [x] 按钮
- [x] 输入框：单行、多行
- [x] 多选框
- [x] 单选框
- [x] 开关控件
- [x] 滚动条 
- [x] 进度条
- [x] 颜色控件
- [x] 滑块控件
- [x] 菜单
- [ ] 表格视图
- [ ] 树形视图
- [ ] 属性视图
- [ ] 对话框
- [ ] 富文本
- [ ] 3D渲染器

## 描述
主实现在目录`third_party/vkgui` 文件读写、窗口创建等。

复制整个third_party目录到自己项目，如需要使用vkvg则取消vkvgcx.cpp这行的注释
```
list(APPEND src_cpp   
"third_party/stb_src/stb_src.c" 
"third_party/vkgui/mapView.cpp"
"third_party/vkgui/tinysdl3.cpp"
"third_party/vkgui/pnguo.cpp"
"third_party/vkgui/win_core.cpp" 
"third_party/tinyspline/parson.c"
"third_party/tinyspline/tinyspline.c"
"third_party/tinyspline/tinysplinecxx.cpp" 
#"third_party/vkvg/vkvgcx.cpp"  
)
```

windows支持OLE拖放，支持渲染已有vulkan纹理，vkvg代码修改了一些编译错误。


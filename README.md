# pnguo
基于SDL3的简易gui实现。

编译说明：
CMakeLists.txt修改`SDL3DIR_ROOT`和`SDL3_DIR`

- vcpkg安装依赖库，根据需求安装，例如```vcpkg install PkgConfig:x64-windows-static```
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
```

## To Do 

- [x] 2d骨骼动画渲染
- [x] 2d图集渲染
- [x] 面板
- [x] 按钮
- [x] 输入框：单行、多行
- [ ] 多选框
- [ ] 单选框
- [ ] 表格视图
- [ ] 树形视图
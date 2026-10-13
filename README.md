# qliteview

将 `liteview` 中的 LiteView 源码移植到 Windows x64，封装为 Qt 6 的 **qliteview.dll**，沿用相邻 `qwebkit` 项目的 QML 声明和 C 初始化入口。

## 使用

交付文件：`dist/qliteview.dll`，当前版本约 106.33 MiB。

宿主负责部署 Qt。在创建 `QGuiApplication` 后，于 GUI 线程加载 DLL 并初始化，然后创建 QML 引擎和页面：

```cpp
QLibrary webModule(QCoreApplication::applicationDirPath() + "/qliteview.dll");
webModule.setLoadHints(QLibrary::PreventUnloadHint);
using Initialize = bool (*)();
using LastError = const char* (*)();
auto initialize = reinterpret_cast<Initialize>(webModule.resolve("qwebkit_initialize"));
auto lastError = reinterpret_cast<LastError>(webModule.resolve("qwebkit_last_error"));
if (!initialize || !initialize()) {
    qFatal("%s", initialize && lastError ? lastError() : qPrintable(webModule.errorString()));
}
```

原有 QML 继续使用 `import QWebKit`、`WebEngineView`、`WebEngineProfile` 等类型。更换宿主加载的 DLL 文件名即可保留这些接口声明；同一进程只初始化 qwebkit 或 qliteview 中的一个，避免重复注册 QML 类型。

`qliteview.dll` 内嵌未压缩的 `livicore.dll`，由安装器统一压缩，避免资源预压缩降低 LZMA2 的压缩效果。这会增加 DLL 和安装后占用，但减少下载安装包大小。首次初始化时，按清单校验并释放到 Qt 的 `GenericCacheLocation/QLiteView/<manifest SHA256>/`，再加载内核。无需额外分发 LiteView 内核 DLL 或 ICU 数据文件。Qt、Qt Quick、Qt WebChannel 和 MSVC 运行库仍由宿主部署。

## 构建

浏览器示例已从相邻 qwebkit 迁移到 `examples/browser`，保留地址栏、前进后退、JS 调用、WebChannel、本地页面、导航拦截、右键菜单和保存 HTML。加载的模块为 `qliteview.dll`，QML 仍使用 `import QWebKit`。

构建完成后运行 `./tools/Run-Example.ps1` 打开示例；可追加 `-Url 'https://www.baidu.com/'` 指定页面，或 `-Headless` 执行示例截图自检。默认使用本机 Qt 6.11.1，也可通过 `-QtDirectory` 指定。示例入口为 `dist/qliteview_example.exe`；未部署 Qt 到同目录时，请通过启动脚本运行。

已验证环境：Windows 11 x64、Qt 6.11.1 MSVC 2022 x64、clang-cl 23.1.3、VS 2022 14.43、Windows SDK 10.0.22621、CMake、Ninja 和 Python 3。目标平台为 Windows 10 及以上 x64；尚未在 Windows 10 实机验证。

```powershell
./tools/Build.ps1 -Jobs 6
```

脚本依次构建内核依赖、内核、Qt DLL，核对接口并运行测试，成功后复制到 `dist/qliteview.dll`。默认 Qt 路径是 `D:/Qt/6.11.1/msvc2022_64`，可用 `-QtDirectory` 修改。LLVM 优先使用相邻 qwebkit 的工具目录，其次为 `C:/Program Files/LLVM/bin`。需要支持 `#embed` 的 clang-cl 23，以嵌入完整 ICU 71 数据。

仅增量链接内核和准备运行时：

```powershell
./tools/Build-WindowsPort.ps1 -WithGl -WithEngine -BuildDirectory build/windows-gl -Target livicore -Jobs 6
./tools/Prepare-Runtime.ps1
```

修改依赖库源码后，请使用完整 `tools/Build.ps1`，确保相应静态库也被重建。内核来自所附 LiteView/Blink/V8 源码。

## 验证

`compatibility/qwebkit-metatypes.json` 保存原项目 7 个 QML 类型的接口基线，`compatibility/webengineview.h` 保存枚举基线。自动检查属性、方法、信号和枚举声明：

```powershell
python tools/check_qml_contract.py build/release/src/QWebKit/meta_types/qt6qliteview_metatypes.json
```

运行测试前，将对应 Qt 的 `bin` 目录加入 PATH：

```powershell
$env:PATH = "D:/Qt/6.11.1/msvc2022_64/bin;" + $env:PATH
$env:QWEBKIT_REQUIRE_EMBEDDED = '1'
ctest --test-dir build/release --output-on-failure
```

测试报告：`build/release/tests/smoke-result.txt`。2026-10-12 完整验证结果为 **22 项通过、0 项失败、0 项跳过**，包括嵌入式内核加载、QML 页面、真实像素合成和透明背景、键盘与中文输入、JS 对象和数组、WebAssembly、Intl、Canvas、qrc/WebChannel、HTTP 和历史导航、gzip/deflate/Brotli 响应解压、iframe.srcdoc 沙箱、业务页面、Profile 隔离、LocalStorage/Cookie 跨进程持久化、加载失败回调、不受信任证书拒绝、HTTPS，以及加载期间销毁视图。

不受信任证书测试使用 Python 在本机启动 HTTPS 服务，测试证书不会安装到系统信任库。公网 HTTPS 和实际业务页面通过环境变量指定：

```powershell
$env:QLITEVIEW_HTTPS_TEST_URL = 'https://www.baidu.com/'
$env:QWEBKIT_HOST_RESOURCES = 'D:/path/to/business.rcc'
$env:QWEBKIT_TEST_HTML = 'D:/path/to/hsjpro-jsapi.html'
$env:QWEBKIT_LIVE_CHECK = '1'
```

设置后重新运行测试。上述完整结果使用客服页 HTTPS 地址及主程序真实资源包，并启用线上检查；已验证客服内容绘制和微信二维码图片加载，截图位于 `build/release/bin/help-live.png`、`build/release/bin/wechat-iframe.png`。未扫码授权或执行实际支付。Windows curl 配置必须保留 `USE_ZLIB` 对应的 `HAVE_ZLIB_H` 和 `HAVE_LIBZ`，否则 gzip/deflate 响应无法解码。

在有效像素比例 100%、125%、150%、200% 下均通过完整回归（各 19 项通过、1 项业务资源测试跳过）。缩放图案在普通渲染与合成图层中均得到 220/220 清晰像素。补充报告位于 `build/smoke-scale-0.8.txt`、`build/smoke-scale-1.2.txt` 和 `build/smoke-scale-1.6.txt`；这些 Qt 缩放因子乘以本机 Windows 的 125% 得到对应有效比例。

## 已知边界

- 保持 Qt/QML 声明，不提供 `WKContextRef`、`WKPageRef` 等 WebKit 原生 C/C++ ABI。`backendName` 如实报告 LiteView。
- `showInspector()` / `devToolsView` 尚未实现，调用会报告 `unsupportedFeatureRequested`；目前无法替代 WebKit Inspector 的调试功能。
- 轻量内核的 WebGL、部分媒体、插件、PDF、WebRTC 和屏幕采集能力受限；请结合 `unsupportedFeatures` 判断具体功能，不要把声明兼容理解为所有浏览器功能完全等价。
- HTTP 缓存采用进程级内存缓存，尚无按 Profile 隔离的磁盘 HTTP 缓存。
- LocalStorage/Cookie 不自动迁移旧 WebKit 的存储格式。私有 Profile 数据相互隔离，但部分内核存储映射保留到进程退出。
- LiteView 在宿主进程内运行，与 WebKit 多进程模型不同。
- 页面使用 Qt 窗口像素比例与 `zoomFactor` 的乘积进行布局和渲染，JavaScript 的 `devicePixelRatio` 同步报告该比例。

## 源码与补丁

`ports/windows` 提供 Windows 平台层及 Qt 桥接，`src/QWebKit` 提供 Qt 适配。上游源码改动记录在 `patches/windows-platform.patch`，可检查补丁与当前源码是否一致：

```powershell
git apply --reverse --check --ignore-space-change --directory=liteview patches/windows-platform.patch
```

原始 LiteView 及第三方组件的许可文件保留在 `liteview` 中。

### 仓库布局

`liteview/` 直接包含编译所需的上游源码及已应用的 Windows 补丁，不是子模块。克隆本仓库后无需另外下载 LiteView 或重复应用补丁。上游来源为 https://gitee.com/openeuler/liteview.git，基准提交为 `e540641b6a7e610fd61c97dc315ba67523fd62d6`。

根目录的 `build/`、`dist/`、`dist.7z`、构建日志、IDE 缓存和 Python 缓存不提交。上游附带的 `liteview/sdk_base/` 是 Windows CMake 移植未使用的 Linux SDK，也不提交；本仓库以 Windows x64 构建为准。`liteview/build/` 是上游构建源码，`liteview/gen/` 包含编译必需的生成源码，ICU 数据及第三方许可证均保留。

Qt 6.12 的 Release 构建请显式指定安装目录：

```powershell
./tools/Build.ps1 -QtDirectory 'D:/Qt/6.12.0/msvc2022_64' -Jobs 6
```

从旧目录布局升级时，需要重新配置本地 CMake 构建目录，避免沿用旧源码路径。

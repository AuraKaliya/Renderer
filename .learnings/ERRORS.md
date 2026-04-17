# Errors

## [ERR-20260416-002] apply-patch-context-mismatch

**Logged**: 2026-04-16T00:00:00+08:00
**Priority**: low
**Status**: resolved
**Area**: config

### Summary
An `apply_patch` update failed because the expected context did not match the current file text.

### Error
```text
apply_patch verification failed: Failed to find expected lines
```

### Context
- Operation: attempted to patch `src/test_viewer/src/viewer_control_panel.cpp` using an outdated snippet.
- Cause: the local file used user-facing `QStringLiteral(...)` text, while the patch expected simpler raw string returns.
- Impact: no code was changed by the failed patch; the file was reopened and patched with the correct surrounding context.

### Suggested Fix
For UI helper switches, read the current function body before patching string-return cases, especially when previous edits may have changed labels or wrappers.

### Metadata
- Reproducible: yes
- Related Files: src/test_viewer/src/viewer_control_panel.cpp

### Resolution
- **Resolved**: 2026-04-16T00:00:00+08:00
- **Notes**: Re-read the current snippet and applied the corrected patch successfully.

---

## [ERR-20260417-003] apply-patch-overbroad-context

**Logged**: 2026-04-17T00:00:00+08:00
**Priority**: medium
**Status**: pending
**Area**: backend

### Summary
在同一文件存在多个相邻 setter 函数时，过宽的 `apply_patch` 上下文误命中了后续函数区域，导致 `viewer_window.cpp` 产生重复且错位的函数块。

### Error
```text
rg found duplicated definitions:
void ViewerWindow::Viewport::setSphereConstructionMode(...)
void ViewerWindow::Viewport::setCylinderRadius(...)
```

### Context
- 操作：为 `setCylinderConstructionMode(...)` 增加轴端点模式状态保留逻辑。
- 文件：`src/test_viewer/src/viewer_window.cpp`
- 现象：补丁匹配到后续球体构造函数附近，插入了重复的圆柱/球体 setter 块，并在球体函数中出现 `cylinder` 变量引用。

### Suggested Fix
编辑包含大量相似 setter 的文件时，先用 `rg -n` 定位函数边界，再用更窄的上下文或分段补丁；补丁后立即用 `rg -n "void .*函数名"` 检查重复定义。

### Metadata
- Reproducible: yes
- Related Files: src/test_viewer/src/viewer_window.cpp

---

## [ERR-20260416-001] qt-moc-spawn-sandbox

**Logged**: 2026-04-16T00:00:00+08:00
**Priority**: low
**Status**: resolved
**Area**: config

### Summary
Sandboxed build blocked the Qt `moc.exe` subprocess during `test_viewer` autogen.

### Error
```text
AutoMoc subprocess error
libuv process spawn failed: operation not permitted
```

### Context
- Command: `cmake --build build --config Debug --target test_viewer`
- The build reached Qt AutoMoc for `viewer_control_panel.h`.
- This was caused by process spawn restrictions, not by a C++ or Qt source error.

### Suggested Fix
When Qt AutoMoc fails with `operation not permitted` under sandboxing, rerun the same CMake build with escalated permissions.

### Metadata
- Reproducible: yes
- Related Files: src/test_viewer/src/viewer_control_panel.h

### Resolution
- **Resolved**: 2026-04-16T00:00:00+08:00
- **Commit/PR**: pending
- **Notes**: Reran `cmake --build build --config Debug --target test_viewer` with approved escalation and the build passed.

---

## [ERR-20260415-001] cpp-pointer-member-access

**Logged**: 2026-04-15T00:00:00+08:00
**Priority**: low
**Status**: resolved
**Area**: frontend

### Summary
Viewer panel state migration briefly used object member access on a `FeatureDescriptor*`.

### Error
```text
viewer_window.cpp(1633,25): error C2228: ".id" left side must have class/struct/union
type is "const renderer::parametric_model::FeatureDescriptor *"
```

### Context
- Command: `cmake --build build --config Debug --target test_viewer`
- During migration from direct feature iteration to unit descriptors, `findObjectFeatureById(...)` returned a pointer but the state push used `feature.id`.

### Suggested Fix
When joining derived unit descriptors back to feature descriptors, use `feature->...` after the null check.

### Metadata
- Reproducible: yes
- Related Files: src/test_viewer/src/viewer_window.cpp

### Resolution
- **Resolved**: 2026-04-15T00:00:00+08:00
- **Commit/PR**: pending
- **Notes**: Replaced pointer member access with `feature->id`, `feature->kind`, and `feature->enabled`.

---

## [ERR-20260403-001] web-open-blender-docs

**Logged**: 2026-04-03T00:00:00+08:00
**Priority**: medium
**Status**: pending
**Area**: docs

### Summary
尝试直接打开 Blender 官方文档页面时，当前工具返回 `402`，无法像普通网页一样稳定抓取正文内容。

### Error
```text
Open failed with status 402
```

### Context
- 操作：尝试直接打开 Blender 官方文档页面以核对 Object Mode / Edit Mode / Modifier 相关表述
- 结果：搜索结果可用，但直接 `open` 官方页面失败
- 影响：本次只能基于官方搜索结果摘要和其它官方资料交叉判断，无法在同一工具链中稳定读取正文

### Suggested Fix
- 后续涉及 Blender 官方文档时，优先保留官方搜索结果链接
- 如需更高精度核对，再尝试其它抓取路径或本地浏览器辅助确认

### Metadata
- Reproducible: unknown
- Related Files: TalkDoc/2026-04-03-参数化建模单元分类与选中编辑方案-讨论.md

---

## [ERR-20260414-004] qt-mouseevent-position-api

**Logged**: 2026-04-14T15:53:19.8218213+08:00
**Priority**: low
**Status**: resolved
**Area**: frontend

### Summary
`QMouseEvent::position()` was not available in the configured Qt headers while compiling `test_viewer`.

### Error
```text
viewer_window.cpp(1678,57): error C2039: "position": is not a member of "QMouseEvent"
viewer_window.cpp(1678,23): error C2737: pickedIndex must initialize const object
```

### Context
- Command: `cmake --build build --config Debug --target test_viewer`
- The new viewport picking code used the Qt 6-style `QMouseEvent::position()` API.
- The existing build environment accepted `QWheelEvent::position()` but not `QMouseEvent::position()`, so mouse click coordinates should stay Qt 5-compatible here.

### Suggested Fix
Use `QMouseEvent::localPos()` for viewport mouse picking coordinates in this project unless the Qt baseline is explicitly raised.

### Metadata
- Reproducible: yes
- Related Files: src/test_viewer/src/viewer_window.cpp
- See Also: ERR-20260414-003

### Resolution
- **Resolved**: 2026-04-14T15:53:19.8218213+08:00
- **Commit/PR**: pending
- **Notes**: Replaced `event->position()` with `event->localPos()` in the left-click pick path.

---

## [ERR-20260414-003] qt-link-msvc-mismatch

**Logged**: 2026-04-14T00:00:00+08:00
**Priority**: medium
**Status**: pending
**Area**: config

### Summary
完整构建进入 `test_viewer` 链接阶段后，出现大量 Qt 符号无法解析。

### Error
```text
error LNK2019: 无法解析的外部符号 "__declspec(dllimport) ... QApplication/QWidget/Qt ..."
libqtmain.sip.a(qtmain_win.o) : error LNK2019: 无法解析的外部符号 _Z5qMainiPPc
fatal error LNK1120: 539 个无法解析的外部命令
```

### Context
- 命令：`cmake --build build --config Debug`
- 结果：`parametric_model`、`render_core`、`render_gl` 已编译通过，`test_viewer` 的 `.cpp` 也已进入编译，最终链接 Qt 时失败
- 现象：链接输入里出现 `libqtmain.sip.a`，而生成器/工具链是 Visual Studio/MSBuild，疑似 Qt 库与 MSVC 工具链不匹配或 Qt 链接配置来自旧缓存

### Suggested Fix
后续需要验证 GUI 可执行文件时，优先清理或新建构建目录，并确保 Qt5 与当前生成器工具链匹配；也可先用 `cmake --build build --config Debug --target render_gl` 验证非 Qt 核心库。

### Metadata
- Reproducible: yes
- Related Files: src/test_viewer/CMakeLists.txt

---

## [ERR-20260414-002] windows-missing-glext-header

**Logged**: 2026-04-14T00:00:00+08:00
**Priority**: medium
**Status**: pending
**Area**: config

### Summary
本地 Visual Studio 构建 `render_gl` 时找不到 `GL/glext.h`。

### Error
```text
gl_renderer.cpp(16,10): error C1083: 无法打开包括文件: “GL/glext.h”: No such file or directory
```

### Context
- 命令：`cmake --build build --config Debug`
- 结果：`parametric_model` 和 `render_core` 已编译通过，`render_gl` 在包含 OpenGL 扩展头时失败
- 影响：需要为 Windows SDK 只提供 OpenGL 1.1 头的环境补一个最小扩展声明兜底，或改为引入稳定的 GL loader/扩展头依赖

### Suggested Fix
在 `gl_renderer.cpp` 中对 `GL/glext.h` 使用 `__has_include` 检测；缺失时仅为本文件实际用到的 GL 常量与函数指针类型提供最小 fallback。

### Metadata
- Reproducible: yes
- Related Files: src/render_gl/src/gl_renderer.cpp

---

## [ERR-20260414-001] rg-wildcard-powershell

**Logged**: 2026-04-14T00:00:00+08:00
**Priority**: low
**Status**: pending
**Area**: config

### Summary
在 PowerShell 中直接把 `src/test_viewer/src/*.h` 这类 glob 传给 `rg` 时，命令返回路径语法错误。

### Error
```text
rg: src/test_viewer/src/*.h: 文件名、目录名或卷标语法不正确。 (os error 123)
```

### Context
- 操作：尝试用单条 `rg` 命令扫描多组 `*.h` / `*.cpp` 文件中的声明和实现线索
- 结果：PowerShell 没有按预期展开这些 glob，`rg` 将其作为字面路径处理并失败
- 影响：分析未受阻，后续应改用 `rg --glob`、`rg --files | rg` 或分别指定目录

### Suggested Fix
在 PowerShell 环境下使用 `rg` 扫描文件类型时，优先使用 `rg --glob "*.h"` / `rg --glob "*.cpp"` 或先通过 `rg --files` 过滤文件列表。

### Metadata
- Reproducible: yes
- Related Files: none

---

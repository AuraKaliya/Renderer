# Learnings

## [LRN-20260402-001] correction

**Logged**: 2026-04-02T00:00:00+08:00
**Priority**: high
**Status**: pending
**Area**: docs

### Summary
Viewport Zoom 在本项目中的最新语义不是纯屏幕放大，而是独立于 Dolly/Lens 的相机工作空间缩放，并且缩放后的窗口仍需支持正常的相机旋转和平移。

### Details
此前将 Viewport Zoom 规划为“不改变相机真实位置、不改变投影视体参数、不改 near/far”的纯屏幕空间放大语义。用户明确修正为：Viewport Zoom 仍是独立概念，但它会改变相机，并且缩放后的窗口是一个可继续进行相机交互的工作窗口。这意味着后续计划和讨论需要从“屏幕放大”转向“带工作空间语义的相机/视口复合缩放”。

### Suggested Action
更新 0.0.15 计划与相关讨论文档，重新明确 Viewport Zoom 与 Dolly/Lens/Focus 的边界及其对 orbitCenter、相机工作空间和交互连续性的影响。

### Metadata
- Source: user_feedback
- Related Files: VersionDoc/0.0.15-开发计划.md, TalkDoc/2026-03-12-视口缩放与参数调整边界-讨论.md
- Tags: viewport-zoom, semantics, correction

---

## [LRN-20260420-001] correction

**Logged**: 2026-04-20T00:00:00+08:00
**Priority**: high
**Status**: pending
**Area**: frontend

### Summary
When viewport picking looks wrong in the Qt test viewer, verify logical widget coordinates versus render framebuffer coordinates before changing hit-selection strategy.

### Details
I initially explored a selection-strategy change after mixed hit logs, but the user correctly pointed out the issue pattern matched bounds and picking mismatch instead. In this project, `QOpenGLWidget` paths use both logical widget size and framebuffer render size. If picking rays, zoom anchors, frame assembly, and framebuffer blit do not use the same render-space dimensions, clicks can appear to miss or target the wrong object even when the scene renders normally.

### Suggested Action
Keep one shared conversion from widget coordinates to render coordinates, and use render viewport dimensions consistently for camera viewport size, ray generation, frame assembly, and framebuffer blit.

### Metadata
- Source: user_feedback
- Related Files: src/test_viewer/src/viewer_window.cpp, src/test_viewer/src/viewer_window.h
- Tags: picking, high-dpi, qopenglwidget, correction

---

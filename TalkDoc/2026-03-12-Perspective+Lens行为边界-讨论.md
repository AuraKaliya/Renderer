# Perspective + Lens 行为边界讨论

## 1. 讨论背景

当前项目已经具备：

- `Perspective + Dolly`
- `Orthographic + Lens`

两条主路径。

后续若继续扩展 `Perspective + Lens`，需要先明确它的行为边界，避免与当前透视模式默认导航语义发生冲突。

## 2. 核心问题

需要先明确以下几点：

1. `Perspective + Lens` 到底修改什么参数
2. 它与 `Perspective + Dolly` 是否允许切换
3. `Focus All / Focus Point / Focus Sphere` 在该模式下应该以什么为主
4. UI 是否需要立即暴露该模式

## 3. 讨论结论

### 3.1 Lens 的统一定义

建议固定 `Lens` 的统一定义：

- 改变投影视体大小
- 不改变相机真实位置

因此在透视模式下：

- `Perspective + Lens` 的唯一核心参数应为 `verticalFovDegrees`

不应混入：

- `distance`
- `orbitCenter`

的主动变化。

### 3.2 Perspective + Lens 的缩放语义

建议：

- 滚轮缩放时只改 `verticalFovDegrees`
- 不改相机真实距离 `distance`

这意味着：

- 画面表现为镜头变焦
- 不是相机前后移动

### 3.3 是否允许与 Perspective + Dolly 切换

建议：

- 允许切换
- 但不作为透视模式默认行为

原因：

- 当前透视模式默认 `Dolly` 更符合通用 3D 观察器习惯
- 若直接切成 `Lens`，会明显改变已有用户手感

结论：

- `Perspective + Dolly` 继续作为默认主路径
- `Perspective + Lens` 作为可选模式存在

### 3.4 Focus Bounds 在 Perspective + Lens 下的建议

建议：

- `Focus Bounds` 仍以 `distance` 拟合为主
- 不把自动聚焦策略切成“自动改 FOV”

原因：

- 若聚焦时自动改 `FOV`，会导致镜头语言频繁变化
- 同一场景在不同操作后会出现透视感不稳定的问题
- `Lens Zoom` 更适合作为用户主动缩放行为，而不是默认 framing 行为

结论：

- `Perspective + Lens` 下：
  - 用户主动滚轮缩放 -> 改 `FOV`
  - `Focus All / Focus Bounds` -> 仍优先改 `distance`

### 3.5 Focus Point 在 Perspective + Lens 下的建议

建议保持当前规则：

- `orbitCenter = point`
- 保持当前 `distance`
- 不自动改 `FOV`

原因：

- 点没有尺寸
- 若自动改 `FOV`，用户难以预期结果

### 3.6 裁剪更新策略

建议：

- `Perspective + Lens` 下的 `near/far` 仍主要跟随真实距离与目标范围
- 不把 `FOV` 变化直接当成裁剪更新的主依据

原因：

- 近远裁剪本质上仍由空间深度关系决定
- `FOV` 改变主要影响可视范围，不直接决定深度分布

## 4. UI 策略建议

### 4.1 是否立即暴露 Zoom Mode 切换

建议：

- 暂不在当前版本立即开放 `Perspective + Lens` 的主动切换 UI

原因：

- 一旦开放，必须同时解释：
  - 当前投影模式
  - 当前缩放模式
  - 各模式下哪些参数有效
- 现有相机面板还需要一次更完整的交互设计整理

### 4.2 UI 最终建议方向

后续若要开放，建议：

- `Projection Mode`
- `Zoom Mode`

明确分开显示，并配套启用/禁用规则：

- `Perspective + Dolly`
  - `Distance` 主可编辑
- `Perspective + Lens`
  - `Vertical FOV` 主可编辑
- `Orthographic + Lens`
  - `Ortho Height` 主可编辑

## 5. 建议的后续实现顺序

建议按以下顺序推进：

1. 先补 `Perspective + Lens` 的控制器行为
2. 保持 `Focus` 与 `Clip` 主要策略不变
3. 再评审并开放 `Zoom Mode` UI 切换

## 6. 讨论结论

- `Perspective + Lens` 的核心语义应为“只改 `verticalFovDegrees`，不改真实距离”
- `Perspective + Dolly` 继续作为默认透视模式主路径
- `Focus Bounds` 在 `Perspective + Lens` 下仍建议优先改 `distance`
- `Focus Point` 在 `Perspective + Lens` 下不建议自动改 `FOV`
- `Perspective + Lens` 可以实现，但 UI 开放应后置

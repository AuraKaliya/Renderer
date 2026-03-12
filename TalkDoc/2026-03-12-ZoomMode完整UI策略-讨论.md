# Zoom Mode 完整 UI 策略讨论

## 1. 讨论背景

当前测试界面已经具备最小相机面板能力：

- `Projection Mode`
- `Zoom Mode`
- `Distance`
- `Vertical FOV`
- `Ortho Height`

但这套 UI 仍属于“最小验证入口”状态，还没有形成完整策略。  
后续如果继续扩展：

- `Perspective + Lens`
- `Orthographic + Lens`
- 后续可能的 `Viewport Zoom`

就必须先把 UI 语义收口，否则会出现“控件能改，但用户不知道改的是哪一层缩放”的问题。

## 2. 当前问题

### 2.1 当前控件关系还不够清晰

当前面板已经同时暴露：

- `Projection`
- `Zoom Mode`
- `Distance`
- `Vertical FOV`
- `Ortho Height`

但还缺少更明确的规则说明：

- 哪些是当前主控参数
- 哪些是当前模式下只读或次级参数
- 哪些是保留但不建议手动调的状态

### 2.2 当前 Orthographic 下的 Zoom Mode 处理是“禁用但可见”

这对最小验证是可接受的，但对长期 UI 来说还不够：

- 用户能看到 `Zoom Mode`
- 但正交模式下为什么禁用、是否未来会开放，并没有在界面层表达

### 2.3 当前相机面板同时承载“状态展示”和“行为控制”

后续如果继续叠加：

- `Viewport Zoom`
- 模型参数变化策略
- 自动适配视图策略

这个面板会很快膨胀。

## 3. 建议的完整 UI 结构

建议把相机 UI 明确拆成三层，而不是继续堆一组平铺参数。

### 3.1 第一层：Projection

负责表达“当前投影类型”：

- `Perspective`
- `Orthographic`

该层只回答一个问题：

- 画面当前采用哪种投影

### 3.2 第二层：Zoom Strategy

负责表达“当前缩放语义”：

- `Dolly`
- `Lens`

该层只回答一个问题：

- 当前滚轮缩放改的到底是什么

### 3.3 第三层：Active Parameter

负责表达“当前真正起作用的参数”：

- `Distance`
- `Vertical FOV`
- `Ortho Height`

这层应根据前两层状态动态启用或降级展示。

## 4. 建议的启用/禁用规则

### 4.1 Perspective + Dolly

建议：

- `Distance`：主可编辑
- `Vertical FOV`：可编辑，但属于次级镜头参数
- `Ortho Height`：禁用
- `Zoom Mode`：可切换

理由：

- 当前主缩放语义是 `Distance`
- 但透视镜头本身的 `FOV` 仍然是合法参数，应保留编辑能力

### 4.2 Perspective + Lens

建议：

- `Distance`：可显示，但不作为主缩放参数
- `Vertical FOV`：主可编辑
- `Ortho Height`：禁用
- `Zoom Mode`：可切换

理由：

- 当前用户主动缩放语义已经变成镜头缩放
- 但 `Distance` 仍然参与：
  - 视角位置
  - `near/far`
  - framing

所以不建议直接禁掉 `Distance`，更合理的是：

- 保留可见
- 弱化其主控地位

### 4.3 Orthographic + Lens

建议：

- `Distance`：可显示，默认不作为主缩放参数
- `Vertical FOV`：禁用
- `Ortho Height`：主可编辑
- `Zoom Mode`：短期可禁用，长期可放开

理由：

- 当前正交模式主缩放参数是 `Ortho Height`
- `Distance` 仍有世界位置和裁剪意义，不应完全隐藏

### 4.4 Orthographic + Dolly

当前不建议立即开放到 UI。  
如果未来开放，建议：

- `Distance`：主可编辑
- `Ortho Height`：可显示，但不是主缩放参数
- `Vertical FOV`：禁用

但这应后置到单独评审，不纳入当前版本策略。

## 5. 建议的视觉表达方式

完整 UI 不建议只靠“enabled / disabled”表达。

建议使用三种层级：

1. 主参数
- 正常可编辑
- 标签可加主状态提示

2. 次级参数
- 可编辑，但视觉弱化

3. 非当前模式参数
- 禁用或折叠

换句话说，长期策略不应只是：

- 开 / 关

而应是：

- 主控
- 次级
- 不适用

## 6. 对当前项目的建议

如果进入下一版本，我建议不要一步做到完整视觉重构，先做“规则完整化”。

建议分两步：

### 6.1 第一阶段

- 固化启用/禁用规则
- 调整 `Distance / FOV / Ortho Height` 在不同模式下的可编辑逻辑
- 补充辅助标签，例如：
  - `Primary`
  - `Secondary`
  - `Inactive`

### 6.2 第二阶段

- 再决定是否把 `Zoom Mode` 从“技术开关”升级成“正式用户可见策略开关”

## 7. 对后续功能的兼容建议

### 7.1 Viewport Zoom

如果后续接入 `Viewport Zoom`，不建议把它塞进当前 `Zoom Mode` 下拉框。

原因：

- `Viewport Zoom` 不是 `Camera Zoom`
- 它应属于视口层能力，而不是相机层能力

### 7.2 模型参数变化策略

“保持当前视角 / 自动适配视图”也不建议塞进当前相机面板主区域。

更合理的是：

- 放在场景或视图策略区域
- 明确它属于“模型变化响应策略”，不是相机缩放策略

## 8. 讨论结论

- `Zoom Mode` 的完整 UI 策略应以“投影类型 + 缩放语义 + 当前主参数”三层组织
- 不建议只靠控件禁用来表达模式差异
- `Distance` 在透视和正交模式下都不应被简单隐藏，而应区分主次关系
- `Orthographic + Dolly` 暂不建议开放到 UI
- `Viewport Zoom` 与模型参数变化策略不应混入当前 `Zoom Mode` UI

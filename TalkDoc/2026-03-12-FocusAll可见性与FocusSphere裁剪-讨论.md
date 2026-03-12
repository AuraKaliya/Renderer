# Focus All 可见性与 Focus Sphere 裁剪讨论

## 1. 讨论背景

在 `0.0.7` 的聚焦能力接入过程中，发现当前实现还存在两个行为缺口：

- `Focus All` 当前未考虑物体显隐状态
- `Focus Sphere` 当前未联动更新 `Near/Far`

这两个问题都直接影响聚焦行为是否完整、是否符合测试界面中的用户预期。

## 2. 讨论内容

本次讨论聚焦以下两个问题：

1. `Focus All` 的聚焦范围应基于全部场景对象，还是仅基于当前可见对象
2. `Focus Sphere` 是否应走统一聚焦链路，以同步更新 `orbitCenter / distance / near / far`

## 3. 讨论结论

### 3.1 Focus All 应考虑显隐状态

- 在当前测试界面语义下，`Focus All` 应基于“当前可见对象”计算范围
- 若仍基于全部对象，则隐藏对象仍会影响聚焦结果，与界面显隐控制的直觉不一致
- 因此推荐：
  - `Focus All` 默认只统计当前 `visible == true` 的对象
  - 若当前没有任何可见对象，则保持当前相机状态不变

### 3.2 Focus Sphere 应统一走聚焦链路

- `Focus Sphere` 当前仍保留独立的相机参数设置逻辑
- 这会导致它无法自动复用 `Focus Point / Focus All` 已建立的：
  - `distance` 更新
  - `near/far` 更新
  - 状态回填链路
- 因此推荐：
  - `Focus Sphere` 先完成可见性和材质预设调整
  - 再基于目标范围调用统一的聚焦 helper

## 4. 对当前开发计划的影响

- 这两个点都属于当前 `0.0.7` 聚焦能力闭环的一部分
- 不需要改写版本目标
- 可以在 `0.0.7` 的步骤 3 中统一收口

## 5. 后续建议

建议在 `0.0.7` 的步骤 3 中按以下顺序处理：

1. 提供“基于当前可见对象”的 `Focus All` 计算入口
2. 将 `Focus Sphere` 改为走统一聚焦链路
3. 验证 `Focus All / Focus Point / Focus Sphere` 在 `orbitCenter / distance / near / far` 上保持一致行为

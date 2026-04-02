# 2026-04-02-参数化建模路线参考Blender与Maya-讨论

## 1. 讨论背景

当前项目的 `parametric_model` 仍处于基础几何生成阶段：

- 仅提供：
  - `Box`
  - `Cylinder`
  - `Sphere`
- 输出形式仍然是一次性 `MeshData`
- `test_viewer` 负责消费生成结果，但尚未建立：
  - 参数对象
  - 历史记录
  - 算子栈
  - 曲线驱动生成

随着 `0.0.16` 已建立“模型变化后的视角保持 / 自动适配视图”最小闭环，后续若要继续推进参数化建模，已经具备接入“模型参数变化 -> mesh 重建 -> bounds 更新 -> 视图策略消费”的基本承载条件。

因此需要结合经典建模软件的成熟路径，讨论当前项目在参数化建模上的发展方向。

## 2. 外部参考

### 2.1 Blender 的参考点

Blender 当前最重要的两条建模路线是：

- Modifier Stack
- Geometry Nodes

官方文档对 Modifier 的定义非常明确：

- Modifier 是对几何体的非破坏性自动操作
- 可以叠加成 stack
- 顺序会影响结果
- 还允许通过 Geometry Nodes 自定义自己的 modifier

这说明 Blender 的核心思路是：

1. 基础几何与后续操作分离
2. 算子顺序是建模语义的一部分
3. 参数化不只等于“改 box 的宽高深”，而是“对象 + 操作栈”

对当前项目的启发：

- 第一阶段不必直接做节点系统
- 但必须尽早把“基础几何参数”和“后续算子链”分层
- 否则后面无法自然演进到类似 Modifier Stack 的结构

### 2.2 Maya 的参考点

Maya 的官方帮助里，参数化建模更偏向：

- Construction History
- DG / node 式输入节点
- 曲线驱动生成

官方文档说明：

- Maya 会通过 construction history 记录对象创建过程中的选项、属性设置和变换
- 可以通过输入节点回到更早阶段修改参数，表面会随之更新
- `Sweep Mesh` 可以根据曲线长度生成可编辑 mesh
- `Boolean`、`Bevel` 等操作在 construction history 打开时都能保留可编辑链路

这说明 Maya 的核心思路是：

1. 每一步建模操作都可以成为可回溯节点
2. 基础 primitive 与后续编辑命令共享统一历史体系
3. 曲线、布尔、倒角等“建模命令”本身就应该是参数对象，而不是一次性烘焙结果

对当前项目的启发：

- 当前 `PrimitiveFactory` 不能长期停留在“只吐最终 mesh”的层级
- 需要逐步升级成：
  - 参数描述
  - 生成器
  - 重建入口
  的组合
- 后续布尔、倒角、扫掠等能力最好从一开始就按“可编辑命令”视角规划

### 2.3 Rhino 的参考点

Rhino 虽然不是多边形 DCC，但它对参数关系和交互式编辑有两个很强的参考价值：

- `History`
- `Gumball`

官方文档说明：

- `History` 会记录输入几何与输出结果之间的连接，输入变化时结果可更新
- 但 History 不是默认全局无脑开启，而是建议按需记录
- `Gumball` 让用户围绕局部原点直接进行移动、缩放、旋转和挤出

这说明 Rhino 的启发点在于：

1. 参数关系应是“可开启、可关闭、可分层”的
2. 参数化建模不仅是数据结构问题，也是交互承载问题
3. 局部操作基准点、局部工作坐标系会影响后续建模体验

对当前项目的启发：

- 后续参数化系统不应该默认把所有结果都变成永久历史链
- 需要明确：
  - 哪些对象保留参数关系
  - 哪些操作烘焙为普通 mesh
- 后续若进入编辑器方向，局部 pivot / gizmo 会和参数建模直接相关

## 3. 对当前项目的关键结论

### 3.1 当前项目不应直接模仿完整 Geometry Nodes

原因：

- 现阶段还没有：
  - 选中系统
  - 对象层编辑器结构
  - modifier 栈 UI
  - 节点图编辑器
- 直接做节点系统，会远超当前工程承载范围

当前更合理的路线是：

- 先做“参数对象 + 重建”
- 再做“可编辑算子栈”
- 再做“曲线驱动与实例化”
- 最后才讨论是否需要节点图

### 3.2 当前项目应优先靠近 Blender Modifier + Maya History 的交集

这个交集可以概括为：

1. 基础几何不是一次性 mesh，而是可编辑参数对象
2. 后续建模命令不是立即烘焙，而是有机会保留为可编辑算子
3. mesh 只是某个时刻的求值结果，不是唯一真实数据

### 3.3 当前阶段最适合的架构升级方向

建议将参数化建模分成 4 层：

1. 参数描述层
   - 例如 `BoxSpec / CylinderSpec / SphereSpec`
2. 生成器层
   - 负责由参数描述生成 `MeshData`
3. 对象建模层
   - 一个对象持有：
     - 基础参数体
     - 可选算子链
4. 求值与同步层
   - 当参数变化时：
     - 重建 mesh
     - 更新 local bounds
     - 更新 GPU mesh
     - 触发 `0.0.16` 已建立的视图策略

## 4. 推荐路线

### 4.1 第一阶段：单体 primitive 参数对象化

目标：

- 把当前 `PrimitiveFactory::makeXxx(...)` 从“直接调用函数”升级成“由参数描述驱动的可重建对象”

建议做法：

- 为基础 primitive 建立显式参数结构：
  - `BoxSpec`
  - `CylinderSpec`
  - `SphereSpec`
- 增加统一入口，例如：
  - `PrimitiveDescriptor`
  - `PrimitiveBuildRequest`
- `PrimitiveFactory` 改为：
  - 校验参数
  - 归一化参数
  - 生成 mesh
  - 输出 bounds

阶段结果：

- 参数变化可以稳定重建同一个对象的 mesh
- `test_viewer` 可以开始真正编辑参数，而不是只看固定 mesh

### 4.2 第二阶段：引入最小建模算子栈

目标：

- 向 Blender Modifier / Maya History 靠拢，但控制范围

建议首批算子：

- `Mirror`
- `Linear Array`
- `Radial Array`
- `Bevel`

原因：

- 这几类算子既有强参数化特征
- 又能很好验证：
  - 顺序
  - enable/disable
  - 局部重建

这一阶段不建议优先做：

- 通用 Boolean 栈
- 通用 Remesh
- 复杂拓扑修复

原因是实现代价和稳定性风险都明显更高。

### 4.3 第三阶段：引入曲线/剖面驱动建模

目标：

- 向 Maya `Sweep Mesh`、Rhino `History + Loft / Extrude / Pipe` 靠近

建议优先能力：

- `Path + Profile Sweep`
- `Lathe / Revolve`
- `Extrude`
- `Pipe / Tube`

这一步的意义很大：

- 能让参数化建模从“基础体尺寸修改”升级到“真实造型流程”
- 非常适合当前项目已有的 orbit / focus / viewport zoom 视口验证路径

### 4.4 第四阶段：实例化与程序化分发

目标：

- 向 Blender Geometry Nodes 的“程序化布置”方向靠近，但仍先不做节点图

建议优先能力：

- `Instance on Points`
- `Scatter / Grid / Curve Array`
- 可控随机种子
- 批量参数变化

这会让系统从“单体建模”扩展到“程序化场景建模”。

## 5. 对当前仓库的具体建议

### 5.1 不建议继续扩大 PrimitiveFactory 的函数数量

如果继续按当前方式增加：

- `makeCone`
- `makeTorus`
- `makeCapsule`

虽然短期能扩 primitive 数量，但长期会带来问题：

- 参数结构分散在函数签名里
- 不利于统一 UI
- 不利于保存/回放参数状态
- 不利于引入算子栈

更好的方向是：

- 先把 primitive 参数对象化
- 再继续扩 primitive 种类

### 5.2 参数变化与 mesh 重建必须进入统一路径

当前后续需要建立：

`参数变化 -> 重新生成 MeshData -> 更新 localBounds -> 更新 repository / GPU -> 触发视图策略`

这里已经和 `0.0.16` 直接衔接。

### 5.3 第一阶段 UI 不宜直接做复杂节点编辑器

建议先用：

- 控制面板
- 对象参数分组
- 基础预设

验证：

- 参数改动
- mesh 重建
- bounds 更新
- keep view / auto frame

闭环稳定后，再讨论更复杂的编辑器承载。

## 6. 讨论结论

- 当前项目的参数化建模路线，不建议直接跳到完整节点系统
- 更适合先参考：
  - Blender 的 Modifier Stack 思路
  - Maya 的 Construction History 思路
  - Rhino 的 History / Gumball 思路
- 当前最优先的不是“再加几个 primitive”，而是建立：
  - 参数对象
  - 重建路径
  - 算子栈预留
- 推荐的总体路线是：
  1. 基础 primitive 参数对象化
  2. 最小建模算子栈
  3. 曲线 / 剖面驱动建模
  4. 实例化与程序化分发

## 7. 对当前版本规划的影响

建议下一版本优先启动：

- `0.0.17`
  - 参数化 primitive 对象化
  - 参数变化 -> mesh 重建 -> bounds / 视图策略闭环

后续版本再考虑：

- `0.0.18`
  - 最小算子栈
- `0.0.19`
  - 曲线/剖面驱动
- `0.0.20+`
  - 实例化 / 程序化布置

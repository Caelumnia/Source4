# Unreal Mesh合批

## 静态合批
StaticMesh最常用的绘制指令 `FRHICommandDrawIndexedPrimitive` 将使用一个 IndexBuffer 并根据索引绘制三角面。而绘制命令所需的顶点与索引数据均储存在 `UStaticMesh` 资源的 `RenderData` 中：![enter image description here](https://media.githubusercontent.com/media/Caelumnia/Source4/AutoMesh/image/GigaMerge0.png)![enter image description here](https://media.githubusercontent.com/media/Caelumnia/Source4/AutoMesh/image/GigaMerge1.png)![enter image description here](https://media.githubusercontent.com/media/Caelumnia/Source4/AutoMesh/image/GigaMerge2.png)

在介绍如何实现合批前，我们先回顾一下 UE4 中是如何渲染一个 Mesh 的。
一次 Mesh Drawcall 通常需要有完整的顶点、索引数据，Shader 与对应的 Uniform Buffer 以及其他全局 Constant Buffer 等信息。在 UE4 中，与资源相关的信息如顶点、索引与材质等将由 `FMeshBatch` 捕获，并由 `FMeshPassProcessor` 在运行时的管线中绑定好 Shader 参数后转变为 `FMeshDrawCommand`，随后发送 RHI 命令到 RHI 队列或 API 中。
因此，要实现多个 Mesh 合并为一次 Drawcall 需要满足以下条件：
- 有相同的材质属性，即需使用同一种 Shader 变体
- 顶点和索引缓冲可以被正确合并
- 在管线中渲染的 Pass 一致
满足以上条件的，可以离线完成合批工作，并作为一个整体 Mesh 参与到渲染中，这大体便是 UE4 中 Merge Actor 工具所完成的功能。在 Merge Actor 的默认参数中，需合并的 Mesh 将依序合并各个 LOD 的顶点工厂，并根据材质与 Section 分别合并索引缓冲与其他数据。

## 动态剔除
仅实现静态的合批只是减少了同材质的 Drawcall，而实际的渲染量并未减少。同时由于合并后的 Mesh 将作为一个完整的 `UStaticMesh` 被使用，因此可能会拥有更大的 Bounds，不利于剔除，造成更多的 Overdraw。因此，为使合批后的 Mesh 支持运行时剔除，需额外维护 Mesh 中各部分的可见性数据，并在运行时做出判断，指导剔除操作。

上文已提到，只有同材质的 Mesh 才有机会被合并，因此一个使用了多个材质的 StaticMesh 往往拥有多个 Section，每个 Section 代表使用了同种材质的三角面；同时在合批前，各个零散的 Mesh 本身拥有自己的 Bounds，在合批后这些 Mesh 的索引数据仍是连续的（因为依序合并，各缓冲也是依序 Append），所以还可以借助这些包围盒剔除不可见的三角面索引。两者结合便可在 Overdraw 接近甚至低于合批前的状态下完成合批操作。

需注意的是，对 Section 的剔除实际上剔除了 Drawcall，因而有可能打断某些内部的渲染状态，而对 IndexBuffer 的剔除则需要更改每帧发送给 API 的索引缓冲，造成更多的内存复制。

### 可见性缓存
![enter image description here](https://media.githubusercontent.com/media/Caelumnia/Source4/AutoMesh/image/GigaMerge3.png)
### IndexBuffer 更新
![enter image description here](https://media.githubusercontent.com/media/Caelumnia/Source4/AutoMesh/image/GigaMerge4.png)
![enter image description here](https://media.githubusercontent.com/media/Caelumnia/Source4/AutoMesh/image/GigaMerge5.png)

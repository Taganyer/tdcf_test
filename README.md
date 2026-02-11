# 测试说明

本项目为[TDCF](https://github.com/Taganyer/TDCF)的测试项目。

- 在单机上进行分布式集群模拟（用一个线程模拟一个节点）。
- 测试项目分为两种：log_test（通信步骤日志打印） martix_test（实现了 SUMMA2D martix multiplication 的任务）。

## 依赖要求

- [TDCF](https://github.com/Taganyer/TDCF)

- [tlog](https://github.com/Taganyer/tlog)（个人私有项目，可选）

## tdcf 测试组件实现部分

- 简单实现了组件所需的所有要求。
- 实现了测试框架要求的组件创建类。
- 组件可用于流程上的正确性测试，也可以通过指定节点间的通信能力（内部节点和外部节点）对算法性能进行简单的模拟测试。


- lua c/api错误处理
- nodesinfo长度时该如何处理
- 在迁移时，slave应该不可读，需要先封，但是读写是针对整个server，如果全部封进slave的读，会造成整个迁移过程中，读都走主，所以需要做到可以按分片进行读写，proxy可能需要知道importing和migrating的状态才行
- 迁移状态只有该server自己才知道，向其他节点发送cluster nodes时不会返回

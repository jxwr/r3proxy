
- lua c/api错误处理
- nodesinfo长度时该如何处理
- 在迁移时，slave应该不可读，需要先封，但是读写是针对整个server，如果全部封slave的读，会造成整个迁移过程(可能几小时)中，读都走主，所以需要做到可以按分片进行读写。如果需要proxy知道importing和migrating的状态，要么controller提供，要么redis集群提供，两者都比较罗嗦。简单的办法是，在迁移前，对slave也设置migrating状态（需要修改redis），这样在访问该slave的迁移slot时，会直接重定向到目标master。
- 迁移状态只有该server自己才知道，向其他节点发送cluster nodes时不会返回

- ASK，需要向目标server先行发送ASKING，再发送命令，如果server不存在（server_table），则设置cluster-nodes更新标志，该次请求失败。tick是每200ms出发一次，也就是说，在新加节点时，最多有200ms的访问失败。这部分可优化，也可不处理。

- MOVED，判断目标slot和当前slot是否相同，相同则表示是slave定向到自己的master，不需要更新slot到replicaset的映射，否则就需要修改。MOVED时，无需发送ASKING。

- 路由
```lua
nearest = {
    tc = {tc,jx,nj02},
    jx = {jx,tc,nj03},
    nj02 = {nj02,nj03,hz01,{tc,jx}},
    nj03 = {nj03,nj02,hz01,{tc,jx}},
    hz01 = {hz01,{nj02,nj03},{tc,jx}}
}
```

```lua
primaryPreferred = {
    tc = {$master,tc,jx,nj02},
    jx = {$master,jx,tc,nj03},
    nj02 = {$master,nj02,nj03,hz01,{tc,jx}},
    nj03 = {$master,nj03,nj02,hz01,{tc,jx}},
    hz01 = {$master,hz01,{nj02,nj03},{tc,jx}}
}
```

```lua
primary = {
    tc = {$master},
    jx = {$master},
    nj02 = {$master},
    nj03 = {$master},
    hz01 = {$master},
}
```

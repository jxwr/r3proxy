
print ("Script Init Begin")

local server0 = server.new("name","127.0.0.1",3002)
local server1 = server.new("name","127.0.0.1",4001)
local server2 = server.new("name","127.0.0.1",4002)
local server3 = server.new("name","127.0.0.1",4003)

local rs = replicaset.new()

server.connect(server0)

replicaset.set_master(rs, server0)
replicaset.add_slave(rs, 1, server1)
replicaset.add_slave(rs, 1, server2)
replicaset.add_slave(rs, 2, server3)

twemproxy.set_replicaset(rs)

print ("Script Init Done")

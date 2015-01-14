
print ("Script Init Begin")

-- init 

local region = pool.region()
local avaliable_zone = pool.avaliable_zone()
local failover_zones = pool.failover_zones()
local machine_room = pool.machine_room()

fazs = string.split(failover_zones, ",")

az_idx_map = {}
az_idx_map[avaliable_zone] = 0
for i,az in ipairs(fazs) do
    az_idx_map[az] = i + 1
end

-- main

local server0 = server.new("server0","127.0.0.1",3002)
local server1 = server.new("server1","127.0.0.1",4001)
local server2 = server.new("server2","127.0.0.1",4002)
local server3 = server.new("server3","127.0.0.1",4003)

local rs = replicaset.new()

server.connect(server0)
server.connect(server1)
server.connect(server2)
server.connect(server3)

replicaset.set_master(rs, server0)
replicaset.add_slave(rs, 0, server2)
replicaset.add_slave(rs, 1, server1)
replicaset.add_slave(rs, 1, server3)

slots.set_replicaset(rs, 0, 16383)

print ("Script Init Done")

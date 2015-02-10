local ffi = require("ffi")
local C = ffi.C

ffi.cdef[[
      struct server;
      struct server_pool;
      struct string { uint32_t len; uint8_t  *data; };

      bool ffi_server_connect(struct server *server);
      bool ffi_server_disconnect(struct server *server);
      struct server* ffi_server_new(
         struct server_pool *pool, const char *name, const char *id, const char *ip, int port);


      struct string ffi_pool_get_region(struct server_pool *pool);
      struct string ffi_pool_get_zone(struct server_pool *pool);
      struct string ffi_pool_get_room(struct server_pool *pool);
      struct string ffi_pool_get_failover_zones(struct server_pool *pool);
]]

local zone = C.ffi_pool_get_zone(__pool)
local failover_zones = C.ffi_pool_get_failover_zones(__pool)

local _M = {
   local_zone = ffi.string(zone.data, zone.len),
   failover_zones = ffi.string(failover_zones.data, failover_zones.len):split(","),
   zone_index = {},
}
local mt = { __index = _M }

-- Initialize zone index
_M.zone_index[_M.local_zone] = 0
for i,zone in ipairs(_M.failover_zones) do
   _M.zone_index[zone] = i
end

function _M.new(self, config)
   local s = {
      id = config.id,
      readable = config.readable,
      writable = config.writable,
      ip = config.ip,
      port = config.port,
      role = config.role,
      master_id = config.master_id,
      region = config.region,
      zone = config.zone,
      room = config.room,
      ranges = config.ranges,
   }
   s.raw = C.ffi_server_new(__pool, config.name, s.id, s.ip, s.port)
   print("local_zone:" .. _M.local_zone .. " zone:" .. s.zone)
   s.tag_idx = _M.zone_index[s.zone]
   if s.tag_idx == nil then
      s.tag_idx = -1
   end
   for z,i in pairs(_M.zone_index) do
      print(i,z)
   end
   return setmetatable(s, mt)
end

function _M.connect(self)
   C.ffi_server_connect(self.raw)
end

function _M.disconnect(self)
   C.ffi_server_disconnect(self.raw)
end

function _M.is_master(self)
   return self.role == "master"
end

function _M.is_slave(self)
   return self.role == "slave"
end

return _M

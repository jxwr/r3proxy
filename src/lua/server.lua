package.path = package.path .. ";lua/?.lua;../?.lua"

local idcmap = require("idcmap")
local ffi = require("ffi")
local C = ffi.C

ffi.cdef[[
      struct server;
      struct server_pool;
      struct string { uint32_t len; uint8_t  *data; };
      typedef int rstatus_t;

      struct string ffi_pool_get_zone(struct server_pool *pool);

      rstatus_t ffi_server_connect(struct server *server);
      rstatus_t ffi_server_disconnect(struct server *server);
      struct server* ffi_server_new(
         struct server_pool *pool, const char *name, const char *id, const char *ip, int port);
]]

local zone = C.ffi_pool_get_zone(__pool)

local _M = {
   local_zone = ffi.string(zone.data, zone.len),
   zone_index = {},
}
local mt = { __index = _M }

-- Initialize zone index
read_preference = idcmap[_M.local_zone]
for i, item in ipairs(read_preference) do
   if item == "$master" then
      _M.zone_index["$master"] = i-1
   elseif type(item) == "table" then
      for j,z in ipairs(item) do
         _M.zone_index[z] = i-1
      end
   elseif type(item) == "string" then
      _M.zone_index[item] = i-1
   end
end

function _M.new(self, config)
   local s = setmetatable({}, mt)
   s:update_config(config)
   s.raw = C.ffi_server_new(__pool, config.addr, s.id, s.ip, s.port)
   if s.raw == nil then
      error("create server object failed.")
   end
   return s
end

function _M.update_config(self, config)
   self.id = config.id
   self.ip = config.ip
   self.port = config.port
   self.readable = config.readable
   self.writable = config.writable
   self.role = config.role
   self.master_id = config.master_id
   self.region = config.region
   self.zone = config.zone
   self.room = config.room
   self.ranges = config.ranges

   if self.role == "master" and _M.zone_index["$master"] then
      self.tag_idx = _M.zone_index["$master"]
   else
      self.tag_idx = _M.zone_index[self.zone] or -1
   end
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

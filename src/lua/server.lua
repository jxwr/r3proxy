local ffi = require("ffi")
local C = ffi.C

ffi.cdef[[
      struct server;
      struct server_pool;

      bool ffi_server_connect(struct server *server);
      bool ffi_server_disconnect(struct server *server);
      struct server* ffi_server_new(
         struct server_pool *pool, const char *name, const char *id, const char *ip, int port);
]]

local _M = {}
local mt = { __index = _M }

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

local ffi = require("ffi")
local C = ffi.C

ffi.cdef[[
      struct replicaset;
      struct server;
      struct server_pool;
      struct string { uint32_t len; uint8_t  *data; };

      int ffi_slots_set_replicaset(struct server_pool *pool, struct replicaset *rs, int left, int right);

      struct replicaset* ffi_replicaset_new();
      struct replicaset* ffi_replicaset_deinit(struct replicaset *rs);
      struct replicaset* ffi_replicaset_delete(struct replicaset *rs);
      void ffi_replicaset_set_master(struct replicaset *rs, struct server *server);
      void ffi_replicaset_add_slave(struct replicaset *rs, int tag_idx, struct server *server);

      struct string ffi_pool_get_region(struct server_pool *pool);
      struct string ffi_pool_get_zone(struct server_pool *pool);
      struct string ffi_pool_get_room(struct server_pool *pool);
      struct string ffi_pool_get_failover_zones(struct server_pool *pool);
]]

local _M = {}
local mt = { __index = _M }

function _M.new(self)
   local raw = C.ffi_replicaset_new();
   return setmetatable({ raw = raw }, mt)
end

function _M.set_master(self, s)
   self.ranges = s.ranges
   s.replica_set = self
   C.ffi_replicaset_set_master(self.raw, s.raw)
end

function _M.add_slave(self, s)
   C.ffi_replicaset_add_slave(self.raw, 0, s.raw)
end

function _M.bind_slots(self)
   for i, range in ipairs(self.ranges) do
      C.ffi_slots_set_replicaset(__pool, self.raw, range.left, range.right)
   end
end

function _M.deinit(self)
   C.ffi_replicaset_deinit(self.raw);
end

return _M

local p = require('lib/utils').prettyPrint
local uv = require('luv')

uv.getaddrinfo(nil, 80, nil, p)

uv.getaddrinfo("facebook.com", 80, {
  v4mapped = true,
  all = true,
  addrconfig = true,
  canonname = true,
  numericserv = true,
  socktype = "STREAM"
}, p)

-- uv.getaddrinfo("google.com", nil, nil, p)

repeat
  print("\nTick..")
until uv.run('once') == 0

print("done")

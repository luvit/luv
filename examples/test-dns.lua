local p = require('lib/utils').prettyPrint
local uv = require('luv')

uv.getaddrinfo(nil, "80", p)

uv.getaddrinfo("facebook.com", nil, p)

repeat
  print("\nTick..")
until uv.run('once') == 0

print("done")

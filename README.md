# node-ipd

This addon provides a Node.js interface to the SunOS Internet Packet
Disturber subsystem.

## Quick Start

	var ipd = require('./lib/ipd_binding');

	var local_ipd = ipd.create();

	local_ipd.setDisturb({ drop: 10, corrupt: 5 });
	console.dir(local_ipd.getDisturb());
	local_ipd.setDisturb();

## API

### create([zone])

This is the only exported method.  It creates an object that serves as a
handle to the specified zone's packet disturber context.  If the zone is not
specified, the current zone is assumed.  This is the normal way to use the
subsystem and the only way to use it in a non-global zone.  In the global
zone, a zone name or ID may be passed, and if that zone exists the object
returned will refer to that zone's packet disturber context.  Note that all
the caveats described by ipdadm(1M) apply to using this mechanism in the
global zone.

## IPDManager

The `create()` method returns an object of type IPDManager, with the
following methods:

### IPDManager.setDisturb([Object])

If no argument is given, all packet disturbance configured for the zone is
removed.  If an argument is given, it must be an object with zero or more of
the properties `corrupt`, `delay`, and `drop`.  The values of these
properties must be numeric and are interpreted as described in ipdadm(1M).

### IPDManager.getDisturb()

Return an object describing the state of the packet disturber.  One or more
of the fields `corrupt`, `delay`, and `drop` will be present.  A function
without a corresponding member is not active in this zone.  The values are
all numeric and may be interpreted as described in ipdadm(1M).

Note that this method throws an exception if either the zone does not exist
(which can occur if it has been deleted since the object was created) or no
packet disturbance is currently configured for the zone.  This is annoying.

## Destruction of IPDManager Objects and Resource Consumption

A file descriptor is opened by the `create()` method and is closed upon
destruction of the object within V8.

## License

MIT.

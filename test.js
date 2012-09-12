var ipd = require('./lib/ipd_binding');

var local_ipd = ipd.create();

try {
	console.dir(local_ipd.getDisturb());
} catch (e) {
	console.dir(e);
}
local_ipd.setDisturb({ drop: 10, corrupt: 5 });
console.dir(local_ipd.getDisturb());
local_ipd.setDisturb({ drop: 0, corrupt: 0 });
try {
	console.dir(local_ipd.getDisturb());
} catch (e) {
	console.dir(e);
}

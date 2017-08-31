
const nvstream = require('bindings')('nvstream');

const params = {frames: 100};


function dataHandler(data) {
	console.log("Got data", typeof data, data.length);	
}

nvstream.initStream(params);

nvstream.setHandler(dataHandler);

nvstream.startStream();

setInterval(function() {
	console.log('interval');
}, 1000);

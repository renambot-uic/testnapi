// load the module
const nvstream = require('bindings')('stream');

function dataHandler(data) {
	console.log("    JS> got data", typeof data, data.length);	
}

const params = {frames: 10000};
nvstream.initStream(params);

nvstream.setHandler(dataHandler);

nvstream.startStream();

setInterval(function() {
	console.log('interval');
}, 10);

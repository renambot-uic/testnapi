
build:
	node-gyp build

conf:
	node-gyp configure

confdebug:
	node-gyp configure --debug

test: build
	node --napi-modules test.js

clean:
	node-gyp clean

.PHONY: test build clean


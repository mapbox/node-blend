build:
	node-waf build

clean:
	node-waf clean

test: build
	@NODE_PATH=./lib:$NODE_PATH expresso test/*.test.js

.PHONY: build clean test
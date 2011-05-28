build:
	node-waf build

clean:
	node-waf clean

test: build
	@expresso test/*.test.js

.PHONY: build clean test
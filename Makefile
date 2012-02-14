build:
	node-waf build

clean:
	node-waf clean
	@rm -rf ./lib/blend.node ./build

test: build
	mocha -R spec

.PHONY: build clean test
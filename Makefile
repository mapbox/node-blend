build:
	node-waf build

clean:
	node-waf clean
	@rm -rf ./lib/blend.node ./build

test: build
	@PATH="./node_modules/mocha/bin:${PATH}" && mocha -R spec

.PHONY: build clean test
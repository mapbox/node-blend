build:
	`npm explore npm -g -- pwd`/bin/node-gyp-bin/node-gyp build

clean:
	@rm -rf ./lib/*.node ./build

test: build
	@PATH=node_modules/mocha/bin:${PATH} mocha -R spec

.PHONY: build clean test
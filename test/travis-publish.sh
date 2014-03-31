#!/bin/bash

set -e

COMMIT_MESSAGE=$(git show -s --format=%B $TRAVIS_COMMIT | tr -d '\n')

if test "${COMMIT_MESSAGE#*'[publish binary]'}" != "$COMMIT_MESSAGE"
    then

    # Inspect binary.
    if [ $platform == "linux" ]; then
        ldd ./lib/blend.node
    else
        otool -L ./lib/blend.node
    fi

    exit 0

    npm install aws-sdk
    ./node_modules/.bin/node-pre-gyp package testpackage
    ./node_modules/.bin/node-pre-gyp publish info

    rm -rf build
    rm -rf lib/blend.node
    npm install --fallback-to-build=false
    npm test

    node-pre-gyp info
fi

#!/bin/sh

rm -rf logs
pip install git+https://github.com/RedisLabsModules/RLTest.git@master
RLTest --module ../libredis-interval-module.so
exit $?
#!/bin/sh
find tests -name "*.lua" | xargs -n1 luajit

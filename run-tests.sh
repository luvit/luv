#!/bin/sh
find tests -name "*.lua" | xargs -l luajit

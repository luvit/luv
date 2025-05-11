#!/usr/bin/env bash

set -e

lua docgen.lua

git diff --quiet -- docs.md
git diff --quiet -- meta.lua

#!/usr/bin/env bash

set -e

lua docs/docgen.lua

git diff --quiet -- docs/docs.md
git diff --quiet -- docs/meta.lua

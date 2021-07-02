export PATH=${PATH}:$HOME/.lua:$HOME/.local/bin:${GITHUB_WORKSPACE}/install/luarocks/bin
bash .ci/setup_lua.sh || exit
eval "$("$HOME/.lua/luarocks" path)"

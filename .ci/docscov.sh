#!/usr/bin/env bash

undocumented_functions=0

if [[ ! -f src/luv.c ]] ; then
	echo "Missing src/luv.c"
	exit 1
fi

func_definitions=$(sed -n '/luaL_Reg luv_functions\[\] = {$/,/^};$/{p;/^\};$/q}' src/luv.c)
funcs_start_line_number=$(awk '/luaL_Reg luv_functions\[\] = \{/ {print FNR}' src/luv.c)
after_funcs=$((funcs_start_line_number+1))
method_definitions=$(tail -n +$after_funcs src/luv.c | sed -n '/luaL_Reg/,/^\};$/p')

for fn in `echo "$func_definitions" | grep -oP "\s+ \{\"\K([^\"]+)(?=\",)"`; do
	# count instances of '### `uv.fn_name' in the docs
	count=$(grep -oPc "^#+\s+\`uv\.$fn\b" docs.md)

	if [ $count -eq 0 ] ; then
		echo $fn
		undocumented_functions=$((undocumented_functions+1))
	fi
done

for fn in `echo "$method_definitions" | grep -oP "\s+ \{\"\K([^\"]+)(?=\",)"`; do
	# count instances of '> method form something:fn_name' in the docs
	count=$(grep -oPc "^> method form [^:]+:$fn\b" docs.md)

	if [ $count -eq 0 ] ; then
		echo "$fn (method form)"
		undocumented_functions=$((undocumented_functions+1))
	fi
done

exit $undocumented_functions

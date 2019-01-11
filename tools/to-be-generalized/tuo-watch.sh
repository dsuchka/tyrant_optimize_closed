#!/bin/bash

mask=${1:-tuo*log}
watch="watch"
if [[ $2 == cat ]]; then
    watch="bash -c"
fi

exec $watch 'for i in '"$mask"'; do
    echo " *** $(basename "$i") ***"
    tail -n5 "$i" | \
        grep -P "Deck improved|Optimized Deck|Upgraded|^\\d+ units" | \
        tail -n2 | \
        sed -r -e "s:\\(([0-9.]+-[0-9.]+, )?([0-9.]+ )+/ ([0-9.]+)\\):(\\1... / \\3):"
    echo
done'

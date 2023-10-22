#!/bin/bash

#SUFFIX="virulence"
#SUFFIX="avenge"
#SUFFIX="flux"
#SUFFIX="SF_SF_x_TC"
INPUT_FILE="$1"
OUTPUT_FILE="${2:-$(dirname "$1")/appraised-deck${SUFFIX:+.${SUFFIX}}.$(basename "$1")}"

declare -i ITERS=5000

DECK="Razogoth Immortal, Krellus' Nexus, Infernal Diabolic, Megalift Foundry, Cleave Condemner, Megalift Foundry, Gaia's Specialist, Armada Xonar, Cleave Condemner, Mephalus Gorge, Infernal Diabolic, Armada Xonar"

TUO_OPTIONS=(
    brawl
    -t 4
    #-e "Virulence"
    no-db
    no-ml
)

TMP_DIR="/tmp"
TMP_FILE_PREFIX="."
TMP_FILE_SUFFIX=".$(basename "$1" ".txt").appraising-deck~"

die() {
    echo " ** ERROR ** $@" 1>&2
    exit 255
}

msg() {
    echo "[$(date +%F' '%T)] $@"
}

# check input file
[[ -f $INPUT_FILE ]] || die "Bad input file (no such file or file isn't regular): $INPUT_FILE"

# reset output file or die
:> "$OUTPUT_FILE" || die "Can't open file for writing: $OUTPUT_FILE"

# show start message
msg "Starting appraise deck (reorder per enemy) for gauntlet $INPUT_FILE (output to $OUTPUT_FILE)"

IFS=$'\n'
while read -r line; do
    if [[ $line =~ (^[a-zA-Z0-9_.+-]+[^:]+):\ ([^/].+[^/])$ ]]; then
        user="${BASH_REMATCH[1]}"
        deck="${BASH_REMATCH[2]}"
        msg "Reordering deck against $user ..."
        TMP_FILE="${TMP_DIR}/${TMP_FILE_PREFIX}${user}${SUFFIX:+.${SUFFIX}}${TMP_FILE_SUFFIX}"
        [[ -f $TMP_FILE ]] && die "Temporary file already exists: $TMP_FILE"
        tuo.sh "$DECK" "$deck" "${TUO_OPTIONS[@]}" reorder $ITERS &> "$TMP_FILE" &
        declare -i tuo_pid=$!
        wait $tuo_pid
        result_line=$(tail -n3 "$TMP_FILE" | fgrep 'Optimized Deck:' )
        if [[ $result_line =~ ^Optimized\ Deck:.*\ \(([0-9.]+)%\ win\)\ ([0-9.]+)(\ \[([0-9.]+)\ per\ win\])?:\ ([^:]+)$ ]]; then
            opt_winrate="${BASH_REMATCH[1]}"
            opt_score="${BASH_REMATCH[2]}"
            opt_win_score="${BASH_REMATCH[4]}"
            opt_deck="${BASH_REMATCH[5]}"
            echo "$(printf "%-8s %-8s %-8s" "$opt_winrate" "$opt_score" "$opt_win_score") $user: $opt_deck" >> "$OUTPUT_FILE"
            rm -rf "$TMP_FILE"
        else
            die "Failed at $user (deck: $deck), see $TMP_FILE for more details"
        fi
    else
        echo "$line" >> "$OUTPUT_FILE"
    fi
done < "$INPUT_FILE"

# show finish message
msg "Appraising deck finished"

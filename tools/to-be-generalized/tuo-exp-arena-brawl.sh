#!/bin/bash

FUND=999999

RUN_BRAWL=0
RUN_ARENA=1

RUN_ATTACK=1
RUN_DEFENSE=1

RANDOM_ATTACK=0
DO_ANNEAL=0

#ATTACK_ITERS=(17 300)
ATTACK_ITERS=(36 250)
#ATTACK_ITERS=(256 256)
DEFENSE_ITERS=(33 1188)
#ATTACK_ITERS=(21 500)
#DEFENSE_ITERS=(38 1000)
#ITER_MUL=3

THREADS=3

((RANDOM_ATTACK)) && ATTACK_ITERS=("${DEFENSE_ITERS[@]}")

declare -a ATK_COMMANDERS=(any)

#ATK_COMMANDERS=(any_r{1..2})
ATK_COMMANDERS+=(any_{im,rd,bt,xn,rt})
#ATK_COMMANDERS+=(any_{coa,cnt})

#ATK_COMMANDERS=(exc_{im,rd,xn,rt})

[[ $TUO_LOGIN == engeji ]] && ATK_COMMANDERS=(any any_r{1..2})
[[ $TUO_LOGIN == pryyf ]] && ATK_COMMANDERS=(dracorex ded nexor ark any)
[[ $TUO_LOGIN == lugofira ]] && ATK_COMMANDERS=(ded nexor nexor any)
[[ $TUO_LOGIN == ken ]] && ATK_COMMANDERS=(dracorex nexor ded ark any)
[[ $TUO_LOGIN == tatyanav ]] && ATK_COMMANDERS=(silus nexor ark ded any ark_emrys)
[[ $TUO_LOGIN == lexicus86 ]] && ATK_COMMANDERS=(silus dracorex nexor ded any)
[[ $TUO_LOGIN == masteraceking ]] && ATK_COMMANDERS=(silus any)

#DEF_COMMANDERS=(silus)

ENEMY_DECK="/^StoredDecks\\.\\d+\\.(DireTide|TidalWave|MasterJedis|TrypticonDYN|UndyingDYN)\\./"
#ENEMY_DECK="stored_decks_top"
ENEMY_ALIAS="sd_top"
#ENEMY_DECK="arena_best"
#ENEMY_ALIAS="$ENEMY_DECK"
#ENEMY_DEF_DECK="arena_comp;DecepticonDEF"
#ENEMY_ATK_DECK="arena_comp;DecepticonATK"
#ENEMY_DECK="arena_full"
#ENEMY_DECK="/^(Arena|GW)\\..*\\.(ImmortalDYN|DireTide|TidalWave|WarEliteFTMFW)\\./"
#ENEMY_DECK="/^(Arena|Brawl)\\..*\\.(ImmortalDYN|DireTide|TidalWave|WarEliteFTMFW|MasterJedis|UnbridledPhoenix|NewHope)\\.[^\\.]+$/"
#ENEMY_ALIAS="arena_top_comp"
#ENEMY_ALIAS="arena_dec"
#ENEMY_ALIAS="${ENEMY_DECK,,}"

#ENEMY_DEF_DECK="GW_GT_MYTH:0.75;GW_GT_HERO:1.25;GW_GT_NORM:1.0"
#ENEMY_DEF_ALIAS="gw_gt_strong"

#ENEMY_DEF_DECK="GW_GT_HERO:1.5;GW_GT_NORM:1.0"
#ENEMY_DEF_ALIAS="gw_gt_well"

#ENEMY_DEF_DECK="BRAWL_GT_ALL"
#ENEMY_DEF_ALIAS="brawl_gt_all"

#ENEMY_DEF_DECK="BRAWL_GT_MYTH:0.75;BRAWL_GT_HERO:1.25;BRAWL_GT_NORM:1.0"
#ENEMY_DEF_ALIAS="brawl_gt_medium"

#ENEMY_DEF_DECK="BRAWL_GT_NORM:1.0;BRAWL_GT_EASY:1.5"
#ENEMY_DEF_ALIAS="brawl_gt_easy"

#ENEMY_DEF_DECK="BRAWL_GT_HERO:1.0;BRAWL_GT_NORM:1.5"
#ENEMY_DEF_ALIAS="brawl_gt_norm"

#ENEMY_DEF_DECK="BRAWL_GT_HERO:1.5;BRAWL_GT_NORM:1.0"
#ENEMY_DEF_ALIAS="brawl_gt_well"

#ENEMY_DEF_DECK="BRAWL_GT_MYTH:1.0;BRAWL_GT_HERO:1.5"
#ENEMY_DEF_ALIAS="brawl_gt_top"

#ENEMY_DEF_DECK="ARENA_GT_MYTH:1.0;ARENA_GT_HERO:1.25;ARENA_GT_NORM:0.75"
#ENEMY_DEF_ALIAS="arena_gt_top"

#ENEMY_DEF_DECK="BRAWL_GT_MYTH"
#ENEMY_DEF_ALIAS="brawl_gt_myth"

#ENEMY_DEF_DECK="BRAWL_GT_HERO"
#ENEMY_DEF_ALIAS="brawl_gt_hero"

#ENEMY_DEF_DECK="CQ_GT_MYTH:0.5;CQ_GT_HERO:1.5;CQ_GT_NORM:1.0"
#ENEMY_DEF_ALIAS="cq_gt_hard"

#ENEMY_ATK_DECK="PREBRAWL_GT_ATK_ALL"
#ENEMY_ATK_ALIAS="${ENEMY_ATK_DECK,,}"

#ENEMY_ATK_DECK="BRAWL_GT_ATK_MYTH:1.0;BRAWL_GT_ATK_HERO:1.5"
#ENEMY_ATK_ALIAS="brawl_atk_top"

#ENEMY_ATK_DECK="BRAWL_GT_ATK_MYTH:1.0;BRAWL_GT_ATK_HERO:1.5;BRAWL_GT_ATK_NORM:1.25"
#ENEMY_ATK_ALIAS="brawl_atk_strong"

#ENEMY_ATK_DECK="GW_GT_ATK_MYTH:1.0;GW_GT_ATK_HERO:1.5;GW_GT_ATK_NORM:1.25"
#ENEMY_ATK_ALIAS="gw_atk_strong"

#ENEMY_ATK_DECK="ARENA_GT_ATK_MYTH:1.0;ARENA_GT_ATK_HERO:1.5;ARENA_GT_ATK_NORM:1.25"
#ENEMY_ATK_DECK="ARENA_GT_ATK_MYTH:1.0;ARENA_GT_ATK_HERO:1.5"
#ENEMY_ATK_ALIAS="arena_atk_strong"

#ENEMY_ATK_DECK="CQ_GT_ATK_ALL"
#ENEMY_ATK_ALIAS="${ENEMY_ATK_DECK,,}"

#ENEMY_ATK_DECK="GW_GT_ATK_HARD;GW_GT_ATK_NORM"
#ENEMY_ATK_ALIAS="gw_gt_atk_strong"


COMMON_OPTIONS=(
    ddd_b64
    _${TUO_LOGIN:-dsuchka}
    _${TUO_LOGIN:-dsuchka}_bb
    _discand_legacy_pve_rewards
    _discand_legacy_pvp_rewards
    _discand_mutant_rewards
    _discand_outdated_p2w
    #_gw_gt
    #_gw_gt_atk
    _brawl_gt
    _brawl_gt_atk
    #_arena_gt
    #_arena_gt_atk
    #_box_dedication
    #_cq_gt
    #_cq_gt_atk
    #_arena_best
    #_sd_top
    _sd
    #_dec
    #_udyn

    -e "IronWill"

    #dom-none
    dom-owned
    #dom-maxed
    #ydom "Constantine's Nexus"
    #ydom "Alpha Shielding"

    endgame 2
    climb-opts:endgame-commander=2
    -t $THREADS
    climb-opts:iter-mul=${ITER_MUL:-6}
    +uc
    +vc
    #mis 0.003
)

#
##  Arena options
#

ARENA_COMMON_OPTIONS=(
)

ARENA_ATTACK_OPTIONS=(
    #climb-opts:open-the-deck
)

ARENA_DEFENSE_OPTIONS=(
    #enemy:ordered
)


#
##  Brawl options
#

BRAWL_COMMON_OPTIONS=(
)

BRAWL_ATTACK_OPTIONS=(
    #-L 7 10
    #yf "CS, DF"
    #ef "IB #2"
)

BRAWL_DEFENSE_OPTIONS=(
    #enemy:ordered
    #enemy:exact-ordered
    #yf "IB #2"
    #ef "CS, DF"
)


## setup fund option or unset variable

if ((FUND)); then
    COMMON_OPTIONS+=("fund" "$FUND")
else
    unset FUND
fi


### BEGIN OF INITIAL DECKS ###

declare -A TUO_INITIAL_DECKS

TUO_EXP_SETTINGS="$HOME/.tuo-exp${TUO_LOGIN:+.$TUO_LOGIN}"

if ! source "$TUO_EXP_SETTINGS"; then
    echo "No such file (TUO-EXP SETTINGS): $TUO_EXP_SETTINGS" 2>&1
    exit 255
fi

## if 'any' commander is not set: set any first found as 'any'
if [[ -z ${TUO_INITIAL_DECKS[any]+x} ]]; then
    for x in "${TUO_INITIAL_DECKS[@]}"; do
        TUO_INITIAL_DECKS[any]=$x
        break
    done
fi

### END OF INITIAL DECKS ###

[[ -z ${DEF_COMMANDERS+x} ]] && [[ ! -z ${ATK_COMMANDERS+x} ]] && DEF_COMMANDERS=("${ATK_COMMANDERS[@]}")

[[ -z ${ATK_COMMANDERS+x} ]] && ATK_COMMANDERS=("${!TUO_INITIAL_DECKS[@]}")
[[ -z ${DEF_COMMANDERS+x} ]] && DEF_COMMANDERS=("${!TUO_INITIAL_DECKS[@]}")

[[ -z ${ENEMY_DEF_DECK+x} ]] && [[ -n $ENEMY_DECK ]] && ENEMY_DEF_DECK=$ENEMY_DECK
[[ -z ${ENEMY_DEF_ALIAS+x} ]] && [[ -n $ENEMY_ALIAS ]] && ENEMY_DEF_ALIAS=$ENEMY_ALIAS

[[ -z ${ENEMY_ATK_DECK+x} ]] && [[ -n $ENEMY_DECK ]] && ENEMY_ATK_DECK=$ENEMY_DECK
[[ -z ${ENEMY_ATK_ALIAS+x} ]] && [[ -n $ENEMY_ALIAS ]] && ENEMY_ATK_ALIAS=$ENEMY_ALIAS

declare -a OPTIONS
declare -a opts

check_commander() {
    local commander=$1
    if [[ -z ${TUO_INITIAL_DECKS[$commander]} ]]; then
        echo "No such commander: $commander (available commanders: ${!TUO_INITIAL_DECKS[@]})"
        return 255
    fi
    return 0
}

run_command() {
    local log=$1
    shift
    local command=("$@")
    if [[ -e "$log" ]]; then
        echo "log already exists: $log"
        return 1
    fi
    ( echo "run: ${command[@]}"; echo ) | tee "$log" >&2
    "${command[@]}" &>> "$log" &
}

TODO_MODE_ATK=(climbex "${ATTACK_ITERS[@]}")
TODO_MODE_DEF=(climbex "${DEFENSE_ITERS[@]}")
if ((DO_ANNEAL)); then
    TODO_MODE_ATK=(anneal "${ATTACK_ITERS[1]}" "1000" "0.001")
    TODO_MODE_DEF=(anneal "${DEFENSE_ITERS[1]}" "1000" "0.001")
fi

## brawl
if ((RUN_BRAWL)); then
    if ((RUN_ATTACK)); then
        ((RANDOM_ATTACK)) && order="random" || order="ordered"
        OPTIONS=(
            "${COMMON_OPTIONS[@]}" "${BRAWL_COMMON_OPTIONS[@]}" "${BRAWL_ATTACK_OPTIONS[@]}"
            "$order" brawl "${TODO_MODE_ATK[@]}"
        )
        for commander in "${ATK_COMMANDERS[@]}"; do
            check_commander "$commander" || continue
            log="tuo-brawl.attack-${order}.${commander}-vs-${ENEMY_DEF_ALIAS}${FUND:+.fund${FUND}}.log"
            opts=("${OPTIONS[@]}")
            [[ $commander =~ ^any(_|$) ]] || opts+=("keep-commander")
            run_command "$log" tuo.sh "${TUO_INITIAL_DECKS[$commander]}" "$ENEMY_DEF_DECK" "${opts[@]}"
        done
    fi
    if ((RUN_DEFENSE)); then
        OPTIONS=(
            "${COMMON_OPTIONS[@]}" "${BRAWL_COMMON_OPTIONS[@]}" "${BRAWL_DEFENSE_OPTIONS[@]}"
            random brawl-defense "${TODO_MODE_DEF[@]}"
        )
        for commander in "${DEF_COMMANDERS[@]}"; do
            check_commander "$commander" || continue
            log="tuo-brawl.defense-random.${commander}-vs-${ENEMY_ATK_ALIAS}${FUND:+.fund${FUND}}.log"
            opts=("${OPTIONS[@]}")
            [[ $commander =~ ^any(_|$) ]] || opts+=("keep-commander")
            run_command "$log" tuo.sh "${TUO_INITIAL_DECKS[$commander]}" "$ENEMY_ATK_DECK" "${opts[@]}"
        done
    fi
fi

## arena
if ((RUN_ARENA)); then
    if ((RUN_ATTACK)); then
        ((RANDOM_ATTACK)) && order="random" || order="ordered"
        OPTIONS=(
            "${COMMON_OPTIONS[@]}" "${ARENA_COMMON_OPTIONS[@]}" "${ARENA_ATTACK_OPTIONS[@]}"
            "$order" fight win "${TODO_MODE_ATK[@]}"
        )
        for commander in "${ATK_COMMANDERS[@]}"; do
            check_commander "$commander" || continue
            log="tuo-arena.attack-${order}.${commander}-vs-${ENEMY_DEF_ALIAS}${FUND:+.fund${FUND}}.log"
            opts=("${OPTIONS[@]}")
            [[ $commander =~ ^any(_|$) ]] || opts+=("keep-commander")
            run_command "$log" tuo.sh "${TUO_INITIAL_DECKS[$commander]}" "$ENEMY_DEF_DECK" "${opts[@]}"
        done
    fi
    if ((RUN_DEFENSE)); then
        OPTIONS=(
            "${COMMON_OPTIONS[@]}" "${ARENA_COMMON_OPTIONS[@]}" "${ARENA_DEFENSE_OPTIONS[@]}"
            random surge defense "${TODO_MODE_DEF[@]}"
        )
        for commander in "${DEF_COMMANDERS[@]}"; do
            check_commander "$commander" || continue
            log="tuo-arena.defense-random.${commander}-vs-${ENEMY_ATK_ALIAS}${FUND:+.fund${FUND}}.log"
            opts=("${OPTIONS[@]}")
            [[ $commander =~ ^any(_|$) ]] || opts+=("keep-commander")
            run_command "$log" tuo.sh "${TUO_INITIAL_DECKS[$commander]}" "$ENEMY_ATK_DECK" "${opts[@]}"
        done
    fi
fi

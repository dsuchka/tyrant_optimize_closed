#!/bin/bash

declare -i PRECQ=1

yforts=""
eforts=""
mode=${1:-both}

#yforts="Phobos Station-2, Andar Quarantine-2, Borean Forges-2, The Spire-2, Jotun's Pantheon-2, SkyCom Complex-2"
#eforts=$yforts

attack_effects=(
    #-e "Asphodel Nexus"
    #-e "Phobos Station"
    -e "Andar Quarantine"

    #-e "The Spire"
    #-e "Ashrock Redoubt"
    #-e "Baron's Claw Labs"
    #-e "SkyCom Complex"
    #-e "Jotun's Pantheon"

    #-e "Colonial Relay"
    #-e "Mech Graveyard"
    #-e "Infested Depot"
    #-e "Seismic Beacon"
    #-e "Tyrolian Outpost"
    #-e "Elder Port"
    #-e "Malort's Den"

    #-e "Red Maw Base"
    #-e "Brood Nest"
    #-e "Borean Forges"
    #-e "Magma Foundry"
)

defense_effects=(
    -e "Asphodel Nexus"
)

(( PRECQ )) && PRE_FLAG="PRE" || PRE_FLAG=""

flags=(
    -f ddd_b64
    -f _${TUO_LOGIN:-dsuchka}
    -f _${TUO_LOGIN:-dsuchka}_bb
    #-f _box_alliance
    -f _discand_legacy_pve_rewards
    -f _discand_legacy_pvp_rewards
    -f _discand_mutant_rewards
    -f _discand_outdated_p2w
    -f _${PRE_FLAG,,}cq_gt_atk
    -f _${PRE_FLAG,,}cq_gt
    -f "climb-opts:iter-mul=6"
    #-f dom-maxed
    -f dom-owned
    -G 2 #1
    -F 5500
    -f +uc
    -f +vc
)

attack=(
    -t 4
    -i 13${TUO_DECK:+00}
    -I 200
)

defense=(
    -t 4
    -i 33${TUO_DECK:+00}
    -I 500
    -d
    -f enemy:ordered
)

declare -A attack_enemies defense_enemies

# enemy def-decks for 'attack' simming
attack_enemies=(
    #[cq_top]="${PRE_FLAG}CQ_GT_MYTH:0.75;${PRE_FLAG}CQ_GT_HERO:1.75;${PRE_FLAG}CQ_GT_NORM:1.5"
    #[cq_myth]="${PRE_FLAG}CQ_GT_MYTH"
    #[cq_hero]="${PRE_FLAG}CQ_GT_HERO"
    [cq_med]="${PRE_FLAG}CQ_GT_HERO;${PRE_FLAG}CQ_GT_NORM"
    #[cq_low]="${PRE_FLAG}CQ_GT_NORM;${PRE_FLAG}CQ_GT_EASY"
    #[cq_top]="${PRE_FLAG}CQ_GT_HARD:0.5;${PRE_FLAG}CQ_GT_NORM:0.75"
    #[cq_low]="${PRE_FLAG}CQ_GT_NORM:0.5;${PRE_FLAG}CQ_GT_EASY:0.75"
    #[brawl_top]="BRAWL_GT__WR_60_64:0.3;BRAWL_GT__WR_65_69:0.4;BRAWL_GT__WR_70_74:0.5;BRAWL_GT__WR_75_79:0.6;BRAWL_GT__WR_80_84:0.6;BRAWL_GT__WR_85_89:0.5"
    #[arena_full]="arena_full"
    #[sd_top]="/^StoredDecks\\.\\d+\\.(PrimalBairs|DireTide|TidalWave|MasterJedis|TrypticonDYN)\\./"
)

# enemy atk-decks for 'defense' simming
defense_enemies=(
    [cq_atk]="${PRE_FLAG}CQ_GT_ATK_MYTH:0.75;${PRE_FLAG}CQ_GT_ATK_HERO:1.75;${PRE_FLAG}CQ_GT_ATK_NORM:1.5"
    #[brawl_atk]="BRAWL_GT_ATK_MYTH:1.0;BRAWL_GT_ATK_HERO:1.25;BRAWL_GT_ATK_NORM:0.75"
)

case "$TUO_LOGIN" in
    (dsuchka|"")
        attack_commanders=(
            any
            any_{im,rd,bt,xn,rt}
        )
        ;;

    (engeji)
        attack_commanders=(
            any
            any_r1
        )
        ;;

    (lugofira)
        attack_commanders=(
            ded
            nexor
        )
        ;;

esac

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

[[ -z ${defense_commanders+x} ]] && [[ ! -z ${attack_commanders+x} ]] && defense_commanders=("${attack_commanders[@]}")

[[ -z ${attack_commanders+x} ]] && attack_commanders=("${!TUO_INITIAL_DECKS[@]}")
[[ -z ${defense_commanders+x} ]] && defense_commanders=("${!TUO_INITIAL_DECKS[@]}")

if [[ -n $TUO_DECK ]]; then
    attack_commanders=(deck)
    defense_commanders=(deck)
fi


if [[ $mode = attack ]] || [[ $mode = both ]]; then
    for enemy in "${!attack_enemies[@]}"; do
        for commander in "${attack_commanders[@]}"; do
            tuo-exp-cq.sh ${TUO_DECK:+-s -D "$TUO_DECK"} -c $commander \
                -Y "$yforts" -E "$eforts" \
                "${attack_effects[@]}" "${attack[@]}" "${flags[@]}" "${attack_enemies[$enemy]}" "${PRE_FLAG}${enemy}"
        done
    done
fi
if [[ $mode = defense ]] || [[ $mode = both ]]; then
    for enemy in "${!defense_enemies[@]}"; do
        for commander in "${defense_commanders[@]}"; do
            tuo-exp-cq.sh ${TUO_DECK:+-s -D "$TUO_DECK"} -c $commander \
                -Y "$yforts" -E "$eforts" \
                "${defense_effects[@]}" "${defense[@]}" "${flags[@]}" "${defense_enemies[$enemy]}" "${PRE_FLAG}${enemy}"
        done
    done
fi

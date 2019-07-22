#!/bin/bash

raid_name="Petrisis Fleshcrafter Raid"
effect="IronWill"
#levels=({01..04})
#levels=({12..13})
#levels=({05..14})
#levels=({10..14})
#levels=({15..20})
#levels=({14..15})
#levels=({14..19})
#levels=({19..21})
#levels=({20..25})

levels=({25..26})
#levels=({45..49})

declare -i FUND=18800
declare -i ITERS_FROM=2000
declare -i ITERS_TILL=100000
declare -i THREADS=2
declare -i ENDGAME=2

declare -A yforts=(
    #[ia_ia]="Inspiring Altar #2"
    [lc_lc]="Lightning Cannon #2"
    #[cs_cs]="Corrosive Spore #2"
    #[df_df]="Death Factory #2"
    #[ds_ds]="Darkspire #2"
    #[mc_mc]="Medical Center #2"

    #[xx_xx]=""

    #[ds3_ds3]="Darkspire-3, Darkspire-3"
    #[cs3_lc3]="Corrosive Spore-3, Lightning Cannon-3"
    #[lc3_lc3]="Lightning Cannon-3(2)"
    #[lc1_lc1]="Lightning Cannon-1(2)"
    #[lc_lc3]="Lightning Cannon, Lightning Cannon-3"
    #[ia1_ia1]="Inspiring Altar-1(2)"
    #[cs_lc]="Corrosive Spore, Lightning Cannon"
    #[cs_df]="Corrosive Spore, Death Factory"
    #[cs_ia]="Corrosive Spore, Inspiring Altar"
    #[cs_mt]="Corrosive Spore, Mortar Tower"
    #[lc_df]="Lightning Cannon, Death Factory"
    #[lc_ia]="Lightning Cannon, Inspiring Altar"

    ### RIP
    ###[sf_sf]="Sky Fortress #2"
)

commanders=(
    any
    any_{im,rd,bt,xn,rt}
    #any_r{1..5}
    #any_{ven,coa}
)

if [[ $TUO_LOGIN == "lugofira" ]]; then
    commanders=(
        ded
        nexor+
        nexor
        any
    )
elif [[ $TUO_LOGIN == "type55" ]]; then
    commanders=(
        silus
    )
elif [[ $TUO_LOGIN == "pryyf" ]]; then
    commanders=(
        drac_raid
        nexor_raid
        ded_raid
        any_raid
    )
elif [[ $TUO_LOGIN == "alexan64" ]]; then
    commanders=(
        barracus
        any
    )
elif [[ $TUO_LOGIN == "engeji" ]]; then
    commanders=(
        any
        any_r1
    )
elif [[ $TUO_LOGIN == "player5028133" ]]; then
    commanders=(
        dracorex
        nexor
        ded
        any
    )
elif [[ $TUO_LOGIN == "morfiziy" ]]; then
    commanders=(
        silus
        barracus
        ded
        any
    )
elif [[ $TUO_LOGIN == "olegv11" ]]; then
    commanders=(
        dracorex
        ded_raid
        any_raid
        nexor_raid
        alaric_raid
    )
elif [[ $TUO_LOGIN == "ken" ]]; then
    commanders=(
        nexor_raid
        ded_raid
        any_raid
    )
elif [[ $TUO_LOGIN == "777stas777" ]]; then
    commanders=(
        silus
        drac
        ded
        any
    )
elif [[ $TUO_LOGIN == "tatyanav" ]]; then
    commanders=(
        silus
        nexor
        emp
        ded
        ark
        any
    )
elif [[ $TUO_LOGIN == "magicman63" ]]; then
    commanders=(
        any
        krellus
    )
elif [[ $TUO_LOGIN == "alexanderb273" ]]; then
    commanders=(
        silus
        nexor
        drac
        ded
        any
    )
fi


for level in "${levels[@]}"; do
    for yfort in "${!yforts[@]}"; do
        for commander in "${commanders[@]}"; do
            tuo-exp-raid.sh \
                -c "$commander" \
                -Y "${yforts[$yfort]}" \
                -e "$effect" \
                -i $ITERS_FROM -I $ITERS_TILL \
                -t $THREADS \
                -G $ENDGAME -F $FUND \
                -f "_${TUO_LOGIN:-dsuchka}" \
                -f "_${TUO_LOGIN:-dsuchka}_bb" \
                -f _discand_legacy_pve_rewards \
                -f _discand_legacy_pvp_rewards \
                -f _discand_mutant_rewards \
                -f _discand_outdated_p2w \
                -f "dom-owned" \
                -f "+uc" \
                -f "+vc" \
                "${raid_name}-${level##0}" "raid_${level}_${yfort}"
                #-r \
                #-U 3 \
        done
    done
done

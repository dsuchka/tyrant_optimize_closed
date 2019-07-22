#!/bin/bash

declare -i FUND=25000
declare -i ENDGAME=2

# phase 1
#gbge="SuperHeroism"

# phase 2
#gbge="CriticalReach"

# phase 3
gbge="Virulence"


ybges=(
    #""
    #"Winter Tempest"
    #"Orbital Cannon"
    #"Charged Up"
    #"Combined Arms"
    #"Ferocity"
    #"Tartarian Gift"
    #"Artillery"
    #"Blightblast"
    #"Divine Blessing"
    #"Inspired"
    #"Progenitor Tech"
    #"Triage"
    #"Emergency Aid"
    #"Mirror Madness"
    #"Sulfuris Essence"
    "Plasma Burst"
    #"Sandblast"
    #"Will of the Nephilim"
)

ebges=(
    #""
    #"Winter Tempest"
    #"Orbital Cannon"
    #"Charged Up"
    #"Combined Arms"
    #"Ferocity"
    #"Tartarian Gift"
    #"Artillery"
    #"Blightblast"
    #"Divine Blessing"
    #"Inspired"
    "Progenitor Tech"
    #"Triage"
    #"Emergency Aid"
    #"Mirror Madness"
    #"Landmine"
    #"Sandblast"
    #"Plasma Burst"
    #"Will of the Nephilim"
)


defense=""
if [[ $1 == "defense" ]]; then
    defense=1
    shift
fi


##
##  YF(Atk) vs EF(Def)  [offense]
##

declare -A atk_yforts=(
    [cs_df]="Corrosive Spore, Death Factory"

    #[ia_ia]="Inspiring Altar #2"
    #[lc_lc]="Lightning Cannon #2"
    #[sf_sf]="Sky Fortress #2"
    #[lc_lc3]="Lightning Cannon, Lightning Cannon-3"

    #[mc_df]="Medical Center, Death Factory"

    #[cs_cs]="Corrosive Spore #2"
    #[cs_lc]="Corrosive Spore, Lightning Cannon"
    #[cs_sf]="Corrosive Spore, Sky Fortress"
    #[cs_ia]="Corrosive Spore, Inspiring Altar"
    #[df_df]="Death Factory #2"
    #[lc_df]="Lightning Cannon, Death Factory"

    #[cs_mt]="Corrosive Spore, Mortar Tower"

    #[sf_sf3]="Sky Fortress, Sky Fortress-3"
    #[sf_lc3]="Sky Fortress, Lightning Cannon-3"
    #[df3_df3]="Death Factory-3(2)"
    #[cs_cs]="Corrosive Spore #2"
    #[cs_ia]="Corrosive Spore, Inspiring Altar"

    #[xx_xx]=""
)

# vs EF(Def)    [offense]

declare -A def_eforts=(
    [ib_ib]="Illuminary Blockade #2"

    #[tc_ib]="Tesla Coil, Illuminary Blockade"

    #[ib_xx]="Illuminary Blockade"
    #[ff_xx]="Forcefield"
    #[fa_xx]="Foreboding Archway"
    #[tc_xx]="Tesla Coil"

    #[ff_ff]="Forcefield #2"
    #[tc_tc]="Tesla Coil #2"
    #[fa_fa]="Foreboding Archway #2"

    #[mf_xx]="Minefield"

    #[fa_ff]="Foreboding Archway, Forcefield"
    #[ib_ff]="Illuminary Blockade, Forcefield"

    #[mf_mf]="Minefield #2"
    #[ff3_xx]="Forcefield-3"
    #[tc3_xx]="Tesla Coil-3"
    #[fa3_xx]="Foreboding Archway-3"

    #[ib_ib3]="Illuminary Blockade, Illuminary Blockade-3"
    #[ib_ff3]="Illuminary Blockade, Forcefield-3"

    ## attack forts
    #[ia_xx]="Inspiring Altar"
    #[lc_xx]="Lightning Cannon"
    #[df_xx]="Death Factory"

    #[xx_xx]=""
)


##
##  YF(Def) vs EF(Atk)  [defense]
##


declare -A def_yforts=(
    [ib_ib]="Illuminary Blockade #2"

    #[ff_xx]="Forcefield"
    #[ff_ff]="Forcefield #2"

    #[mf_xx]="Minefield"
    #[tc_xx]="Tesla Coil"
    #[fa_xx]="Foreboding Archway"
    #[ib_xx]="Illuminary Blockade"

    #[ff_tc]="Forcefield, Tesla Coil"

    #[tc3_xx]="Tesla Coil-3"
    #[ff3_xx]="Forcefield-3"
    #[ib3_xx]="Illuminary Blockade-3"

    #[tc_tc]="Tesla Coil #2"
    #[mf_mf]="Minefield #2"
    #[fa_fa]="Foreboding Archway #2"
    #[ff_ib]="Forcefield, Illuminary Blockade"

    #[xx_xx]=""
)

# vs EF(Atk)    [defense]

declare -A atk_eforts=(
    [cs_df]="Corrosive Spore, Death Factory"
    #[df_cs]="Death Factory, Corrosive Spore"
    #[df_mc]="Death Factory, Medical Center"
    #[df_lc]="Death Factory, Lightning Cannon"

    #[lc_lc]="Lightning Cannon #2"
    #[sf_sf]="Sky Fortress #2"
    #[cs_cs]="Corrosive Spore #2"
    #[cs_sf]="Corrosive Spore, Sky Fortress"
    #[ia_ia]="Inspiring Altar #2"
    #[df_df]="Death Factory #2"
    #[mc_mc]="Medical Center #2"
    #[ds_ds]="Darkspire #2"

    #[mc_cs]="Medical Center, Corrosive Spore"
    #[ds_lc]="Darkspire, Lightning Cannon"

    #[ia_xx]="Inspiring Altar"
    #[lc_xx]="Lightning Cannon"


    #[cs_ia]="Corrosive Spore, Inspiring Altar"
    #[cs_lc]="Corrosive Spore, Lightning Cannon"
    #[ia_lc]="Inspiring Altar, Lightning Cannon"
    #[df3_df]="Death Factory-3, Death Factory"
    #[sf3_sf3]="Sky Fortress-3(2)"
    #[cs_mt]="Corrosive Spore, Mortar Tower"

    #[xx_xx]=""
)

declare -A yforts eforts
declare -a XOPTIONS

#ENEMY_GUILD="(DireTide|TidalWave|TrypticonDYN|UndyingDYN|PrimalBairs|MasterJedis)"
#ENEMY_ALIAS="top"
#ENEMY_GUILD="(UndyingDYN)"
#ENEMY_GUILD="(TrypticonDYN)"
#ENEMY_GUILD="(DeutscheHeldenDYN)"
#ENEMY_GUILD="(EternalDYN)"
#ENEMY_ALIAS="udyn"
#ENEMY_GUILD="(MasterJedis)"
#ENEMY_ALIAS="mj"
#ENEMY_GUILD="(NewHope)"
#ENEMY_GUILD="(NovaSlayers)"
#ENEMY_ALIAS="ns"
ENEMY_GUILD="(TheFallenKnights)"
ENEMY_ALIAS="tfk"
#ENEMY_GUILD="(DireTide)"
#ENEMY_ALIAS="dt"
#ENEMY_GUILD="(SerbianLawyers)"
#ENEMY_ALIAS="sl"
#ENEMY_DECK="/^(Arena|GW)\\.201.....\\.eds..\\.$ENEMY_GUILD\\./"
#ENEMY_DECK="/^(Arena|GW)\\..*\\.$ENEMY_GUILD\\./"
ENEMY_DECK="/^StoredDecks\\..*\\.$ENEMY_GUILD\\./"

#ENEMY_DECK="arena_all"
#ENEMY_DECK="TheNovaForce"
#ENEMY_ALIAS="${ENEMY_DECK,,}"

#ENEMY_DECK="BRAWL_GT_MYTH:0.7;BRAWL_GT_HARD:0.9;BRAWL_GT_NORM:1.1"
#ENEMY_ALIAS="brawl_gt_hard"

#ENEMY_DECK="BRAWL_GT_HARD:0.7;BRAWL_GT_NORM:1.1;BRAWL_GT_EASY:0.8"
#ENEMY_ALIAS="brawl_gt_norm"

#ENEMY_DECK="BRAWL_GT_ATK_MYTH:0.75;BRAWL_GT_ATK_HERO:1.0;BRAWL_GT_ATK_NORM:0.85"
#ENEMY_ALIAS="brawl_gt_atk_strong"

#ENEMY_DECK="BRAWL_GT_ATK_MYTH;BRAWL_GT_ATK_HERO"
#ENEMY_ALIAS="brawl_gt_atk_top"

#ENEMY_DECK="GW_GT_MYTH:0.75;GW_GT_HERO:1.25;GW_GT_NORM:1.0"
#ENEMY_ALIAS="gw_gt_strong"

#ENEMY_DECK="GW_GT_HERO:1.5;GW_GT_NORM:1.0"
#ENEMY_ALIAS="gw_gt_medium"

#ENEMY_DECK="GW_GT_NORM:1.5;GW_GT_EASY:1.0"
#ENEMY_ALIAS="gw_gt_low"

#ENEMY_DECK="GW_GT_ATK_ALL"
#ENEMY_ALIAS="gw_gt_atk"

#ENEMY_DECK="GW_GT_ATK_MYTH;GW_GT_ATK_HERO"
#ENEMY_ALIAS="gw_gt_atk_strong"

#ENEMY_DECK="GW_GT_ATK_HERO;GW_GT_ATK_NORM"
#ENEMY_ALIAS="gw_gt_atk_med"

#ENEMY_DECK="GW_GT_ATK_NORM;GW_GT_ATK_EASY"
#ENEMY_ALIAS="gw_gt_atk_low"

#BGE="_DEVOUR"
#unset BGE

#ENEMY_DECK="BRAWL_GT${BGE}_FANT:1;BRAWL_GT${BGE}_MYTH:2;BRAWL_GT${BGE}_HERO:5;BRAWL_GT${BGE}_NORM:3"
#ENEMY_ALIAS="brawl_gt_strong"

#ENEMY_DECK="BRAWL_GT${BGE}_MYTH:1;BRAWL_GT${BGE}_HERO:3;BRAWL_GT${BGE}_NORM:2"
#ENEMY_ALIAS="brawl_gt_medium"

COMMON_XOPTIONS=(
    -t 6
    -f "climb-opts:iter-mul=6"
    #-f "climb-opts:endgame-commander=1"
    -f _${TUO_LOGIN:-dsuchka}
    -f _${TUO_LOGIN:-dsuchka}_bb
    -f _discand_legacy_pve_rewards
    -f _discand_legacy_pvp_rewards
    -f _discand_mutant_rewards
    -f _discand_outdated_p2w
    #-f _commanders_lv1
    #-f _commanders_lv2
    #-f _stored_decks
    #-f _brawl_gt
    #-f _brawl_gt_atk
    #-f _box_dedication
    -f _gw_gt_atk
    -f _gw_gt
    -f _sd
    #-f dom-maxed
    -f dom-owned
)

commanders=(
    #any
    #any_r1
    #any_{im,bt,rd,xn,rt}
    #any_im
    any_r{1..5}
    #any_x{1..3}
)

if [[ $TUO_LOGIN == "lugofira" ]]; then
    commanders=(
        ded
        #nexor
        #typhon
        #any
    )
elif [[ $TUO_LOGIN == "briki" ]]; then
    commanders=(
        any
        ded
        silus
        nexor
        typhon
    )
elif [[ $TUO_LOGIN == "type55" ]]; then
    commanders=(
        silus
        #barracus
    )
elif [[ $TUO_LOGIN == "pryyf" ]]; then
    commanders=(
        krellus
        dracorex
        nexor
        ded
        any
    )
elif [[ $TUO_LOGIN == "porsche7" ]]; then
    commanders=(
        silus
        ded
        any
    )
elif [[ $TUO_LOGIN == "anyman1979" ]]; then
    commanders=(
        const
        ded
        any
    )
elif [[ $TUO_LOGIN == "prokop" ]]; then
    commanders=(
        silus
        any
    )
elif [[ $TUO_LOGIN == "heid" ]]; then
    commanders=(
        krellus
        any
    )
elif [[ $TUO_LOGIN == "lexicus86" ]]; then
    commanders=(
        silus
        any
    )
elif [[ $TUO_LOGIN == "kapturov" ]]; then
    commanders=(
        krellus
        any
    )
elif [[ $TUO_LOGIN == "777stas777" ]]; then
    commanders=(
        silus
        ded
        any
    )
elif [[ $TUO_LOGIN == "ken" ]]; then
    commanders=(
        dracorex
        halcyon
        nexor
        ded
        any
    )
elif [[ $TUO_LOGIN == "tatyanav" ]]; then
    commanders=(
        #barracus
        silus
        #nexor
        ded
        ark
        any
    )
fi


if (( $defense )); then
    for x in "${!def_yforts[@]}"; do
        yforts[$x]="${def_yforts[$x]}"
    done
    for x in "${!atk_eforts[@]}"; do
        eforts[$x]="${atk_eforts[$x]}"
    done

    XOPTIONS=(
        "${COMMON_XOPTIONS[@]}"
        -i 33${TUO_DECK:+00} -I 500
        -d
        #-f enemy:ordered
    )
else
    for x in "${!atk_yforts[@]}"; do
        yforts[$x]="${atk_yforts[$x]}"
    done
    for x in "${!def_eforts[@]}"; do
        eforts[$x]="${def_eforts[$x]}"
    done

    XOPTIONS=(
        "${COMMON_XOPTIONS[@]}"
        -i 23${TUO_DECK:+00} -I 120
    )
fi


if [[ -n $TUO_DECK ]]; then
    commanders=(deck)
fi

for yfort in "${!yforts[@]}"; do
    for efort in "${!eforts[@]}"; do
        for ybge in "${ybges[@]}"; do
            for ebge in "${ebges[@]}"; do
                for commander in "${commanders[@]}"; do
                    tuo-exp-gw.sh \
                        -c "$commander" \
                        -Y "${yforts[$yfort]}" \
                        -E "${eforts[$efort]}" \
                        -e "${gbge}:${ybge}:${ebge}" \
                        -G "$ENDGAME" -F "$FUND" \
                        -L 3 -U 10 \
                        ${TUO_DECK:+-s -D "$TUO_DECK"} \
                        "${XOPTIONS[@]}" \
                        "${ENEMY_DECK}" "${ENEMY_ALIAS}.${yfort}.${efort}"
                done
            done
        done
    done
done

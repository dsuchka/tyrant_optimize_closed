#NoEnv  ; Recommended for performance and compatibility with future AutoHotkey releases.
#SingleInstance off
SendMode Input  ; Recommended for new scripts due to its superior speed and reliability.
SetWorkingDir %A_ScriptDir%  ; Ensures a consistent starting directory.

BGEffects := "none|Blood-Vengeance|Oath-of-Loyalty|Furiosity|TemporalBacklash|CriticalReach|Devour|HaltedOrders|ZealotsPreservation|Virulence|Enfeeble all X|Enhance all S X|Evolve n S1 S2|Heal all X|Mortar X|Protect all X|Rally all X|Siege all X|Strike all X|Weaken all X|Brigade|Bloodlust X|Counterflux|Divert|EnduringRage|Fortification|Heroism|Metamorphosis|Megamorphosis|Revenge X|TurningTides"
IniFileName := "data\SimpleTUOptimizeStarter.ini"
IniSection := "onLoad"

IniRead, IniMyDeck, %IniFileName%, %IniSection%, MyDeck, Cyrus, Medic, Revolver, Imperial APC, Medic, Imperial APC
IniRead, IniMySiege, %IniFileName%, %IniSection%, MySiege, %A_Space%
IniRead, IniEnemiesDeck, %IniFileName%, %IniSection%, EnemiesDeck, Mission #94
IniRead, IniEnemySiege, %IniFileName%, %IniSection%, EnemySiege, %A_Space%
IniRead, IniVIP, %IniFileName%, %IniSection%, VIP, %A_Space%
IniRead, IniIterations, %IniFileName%, %IniSection%, Iterations, 10000
IniRead, IniThreads, %IniFileName%, %IniSection%, Threads, 4
IniRead, IniSimOptions, %IniFileName%, %IniSection%, SimOptions, %A_Space%
IniRead, IniEffect, %IniFileName%, %IniSection%, Effect, 1
IniRead, IniMode, %IniFileName%, %IniSection%, Mode, 1
IniRead, IniOrder, %IniFileName%, %IniSection%, Order, 1
IniRead, IniOperation, %IniFileName%, %IniSection%, Operation, 1
IniRead, IniEndgame, %IniFileName%, %IniSection%, Endgame, 1
IniRead, IniFund, %IniFileName%, %IniSection%, Fund, 0
IniRead, Inix86, %IniFileName%, %IniSection%, x86, 0

Menu, MyMenu, Add, ownedcards.txt, MenuOwnedcards
Menu, MyMenu, Add, customdecks.txt, MenuCustomdecks
Menu, MyMenu, Add, cardabbrs.txt, MenuCardabbrs
Menu, MyMenu, Add, Update XMLs, MenuUpdate
Menu, MyMenu, Add, Update XMLs (DEV), MenuUpdateDev
Menu, MyMenu, Add, Help, MenuHelp
Menu, MyMenu, Add, Web, MenuWeb
Gui, Menu, MyMenu

Gui, Add, Text, r5, My Deck:
Gui, Add, Text, r1, My Fortress:
Gui, Add, Text, r5, Enemy Deck(s):
Gui, Add, Text, r1, Enemy Fortress:
Gui, Add, Text, r1, VIP Units:
Gui, Add, Text, r1, Quest:
Gui, Add, Text, r1, Effect:
Gui, Add, Text, r1, Mode:
Gui, Add, Text, r1, Operation:
Gui, Add, Text, r1, Flags:

Gui, Add, Edit, vMyDeck ym w600 r5, %IniMyDeck%
Gui, Add, Edit, vMySiege w600 r1, %IniMySiege%
Gui, Add, Edit, vEnemiesDeck w600 r5, %IniEnemiesDeck%
Gui, Add, Edit, vEnemySiege w600 r1, %IniEnemySiege%
Gui, Add, Edit, vVIP w600 r1, %IniVIP%
Gui, Add, DDL, vQuest w200 section, none||Activate X times of Berserk|Activate X times of Enfeeble|Activate X times of Flurry|Activate X times of Heal|Activate X times of Jam|Activate X times of Leech|Activate X times of Poison|Activate X times of Protect|Activate X times of Rally|Activate X times of Strike|Activate X times of Valor|Activate X times of Weaken|Deal X Counter damage|Deal X Poison damage|Destroy X Assault cards|Destroy X Structure cards|Play 3 Structure cards in each battle|Play 4 Bloodthirsty cards in each battle|Play 4 Imperial cards in each battle|Play 4 Raider cards in each battle|Play 4 Righteous cards in each battle|Play 4 Xeno cards in each battle
Gui, Add, Checkbox, vQuestWin ys Checked, and Win
Gui, Add, Text, ys, X=
Gui, Add, Edit, vQuestX number ys w30, 1

IniEffectNum := 0
BGEffectsArrayCount := 0

StringSplit, BGEffectsArray, BGEffects, |
Loop, %BGEffectsArray0%
{
    BGEffectsArrayCount += 1
    if ( BGEffectsArray%a_index% = IniEffect) {
		IniEffectNum := a_index
    }
}

if (IniEffectNum = 0) {
	IniEffectNum := BGEffectsArrayCount + 1
	BGEffects := BGEffects . "|" . IniEffect
}

Gui, Add, ComboBox, vEffect xs Choose%IniEffectNum% section, %BGEffects%
Gui, Add, DDL, altsubmit vMode Choose%IniMode%, Battle / Mission|Battle (defense)|GW / CQ / Surge|GW (defense)|Brawl|Brawl (defense)|Raid|Campaign
Gui, Add, DDL, altsubmit vOperation Group Choose%IniOperation% xs, Climb|Sim|Reorder|Climbex

Gui, Add, Text, ys, Endgame:
Gui, Add, Text, r1, Order:
Gui, Add, Text, r1, Iterations:

Gui, Add, DDL, altsubmit vEndgame ys Choose%IniEndgame%, none|0 - Maxed Units|1 - Maxed Fused|2 - Maxed Quads
Gui, Add, DDL, altsubmit vOrder Group Choose%IniOrder%, Random|Ordered (honor 3-card hand)
Gui, Add, Edit, vIterations w100 r1, %IniIterations%

Gui, Add, Text, ys, Fund:
Gui, Add, Text, r1,
Gui, Add, Text, , Threads:

Gui, Add, Edit, vFund number r1 ys w50, %IniFund%
Gui, Add, Text, r1,
Gui, Add, Edit, vThreads number w20, %IniThreads%

Gui, Add, Edit, vSimOptions r1 xs w600, %IniSimOptions%
Gui, Add, Button, default r2 w100 x100 y+15 section, Simulate
Gui, Add, Checkbox, vx86 Checked%Inix86%, x86 (32-bit)
Gui, Add, Button, r2 w100 ys xs+200, Exit
Gui, Show,, Simple Tyrant Unleashed Optimize Starter v2.52.4
return

ButtonSimulate:
Gui, Submit
MyDeck := RegExReplace(MyDeck, "\R")
MyDeck := RegExReplace(MyDeck, "(^\s*|\s*$)")

EnemiesDeck := RegExReplace(EnemiesDeck, "\R")
EnemiesDeck := RegExReplace(EnemiesDeck, "(^\s*|\s*$)")

SimOptions := RegExReplace(SimOptions, "(^\s*|\s*$)")

GuiControl, , Edit1, %MyDeck% ; this will put the content of the variable in the editbox (edit1 is taken by the winspy)
GuiControl, , Edit3, %EnemiesDeck% ; this will put the content of the variable in the editbox (edit1 is taken by the winspy)
GuiControl, , Edit11, %SimOptions% ; this will put the content of the variable in the editbox (edit11 is taken by the winspy)
Gui, Submit, NoHide ; save the changes and not hide the windows)

selTUO := (x86 ? "tuo-x86" : "tuo")
selMode := (Mode == 1 ? "pvp" : Mode == 2 ? "pvp-defense" : Mode == 3 ? "gw" : Mode == 4 ? "gw-defense" :Mode == 5 ? "brawl" : Mode == 6 ? "brawl-defense" : Mode == 7 ? "raid" : "campaign")
selOrder := (Order == 1 ? "random" : "ordered")
selOperation :=  (Operation == 1 ? "climb" : Operation == 2 ? "sim" : Operation == 3 ? "reorder": "climbex")
selMySiege := (MySiege == "" ? "" : "yf """ MySiege """ ")
selEnemySiege := ( EnemySiege == "" ? "" : "ef """ EnemySiege """ ")
selVIP := ( VIP == "" ? "" : "vip """ VIP """ " )
StringReplace, Quest, Quest, %A_Space%X%A_Space%,%A_Space%%QuestX%%A_Space%
StringReplace, Quest, Quest, Activate, su
StringReplace, Quest, Quest, %A_Space%times%A_Space%of
StringReplace, Quest, Quest, Deal, sd
StringReplace, Quest, Quest, %A_Space%damage
StringReplace, Quest, Quest, Destroy, ck
StringReplace, Quest, Quest, %A_Space%cards
StringReplace, Quest, Quest, Play, cu
StringReplace, Quest, Quest, in%A_Space%each%A_Space%battle, each
selQuest := (Quest == "" || Quest == "none" ? "" : "quest """ Quest (QuestWin ? " win" : "") """ ")
selEffect := ( Effect == "" || Effect == "none" ? "" : "-e """ Effect """ ")
selThreads := ( Threads == "4" ? "" : "-t " Threads " ")
selSimOptions := ( SimOptions == "" ? "" : SimOptions " ")
EndgameVal := Endgame -2
selEndgame := (Endgame <= 1 ? "" : "endgame " EndgameVal " ")
selFund := (Fund == "" ? "" : "fund " Fund " ")
execString = %selTUO% "%MyDeck%" "%EnemiesDeck%" %selMode% %selOrder% %selMySiege%%selEnemySiege%%selVIP%%selQuest%%selEffect%%selThreads%%selEndgame%%selFund%%selSimOptions%%selOperation% %Iterations%
Run, cmd.exe /c title TUOptimizeOutput && echo %execString% && %execString% & pause
Gui, Show
return

MenuHelp:
Gui, Submit
selTUO := (x86 ? "tuo-x86" : "tuo")
Run, cmd.exe /c title TUOptimizeOutput && echo %selTUO% && %selTUO% & pause
Gui, Show
return

MenuWeb:
Gui, Submit
Run https://github.com/dsuchka/tyrant_optimize/releases
Gui, Show
return

MenuUpdate:
MsgBox, 0, Update started, Updating XML files.`nPlease wait at least one minute. A new window should open soon.`nThis Window will auto close in 5 seconds. , 5
UrlDownloadToFile, *0 http://mobile.tyrantonline.com/assets/fusion_recipes_cj2.xml, data\fusion_recipes_cj2.xml
had_error := false
if ErrorLevel
{
    MsgBox, Error downloading fusion_recipes_cj2.xml.
    had_error := true
}
UrlDownloadToFile, *0 http://mobile.tyrantonline.com/assets/missions.xml, data\missions.xml
if ErrorLevel
{
    MsgBox, Error downloading missions.xml.
    had_error := true
}
UrlDownloadToFile, *0 http://mobile.tyrantonline.com/assets/skills_set.xml, data\skills_set.xml
if ErrorLevel
{
    MsgBox, Error downloading skills_set.xml.
    had_error := true
}
UrlDownloadToFile, *0 http://mobile.tyrantonline.com/assets/levels.xml, data\levels.xml
if ErrorLevel
{
    MsgBox, Error downloading levels.xml.
    had_error := true
}
Loop, 13
{
    UrlDownloadToFile, *0 http://mobile.tyrantonline.com/assets/cards_section_%A_Index%.xml, data\cards_section_%A_Index%.xml
    if ErrorLevel
    {
        MsgBox, Error downloading cards_section_%A_Index%.xml.
        had_error := true
    }
}
UrlDownloadToFile, *0 https://raw.githubusercontent.com/dsuchka/tyrant_optimize/merged/data/raids.xml, data\raids.xml
if ErrorLevel
{
    MsgBox, Error downloading raids.xml.
    had_error := true
}
UrlDownloadToFile, *0 https://raw.githubusercontent.com/dsuchka/tyrant_optimize/merged/data/bges.txt, data\bges.txt
if ErrorLevel
{
    MsgBox, Error downloading bges.txt.
    had_error := true
}
if !had_error
    MsgBox, 0, Update finished, xml files successfully updated.`nThis Window will auto close in 2 seconds., 2
Gui, Show
return

MenuUpdateDev:
MsgBox, 0, Update started, Updating XML (DEV) files.`nPlease wait at least one minute. A new window should open soon.`nThis Window will auto close in 5 seconds. , 5
UrlDownloadToFile, *0 http://mobile-dev.tyrantonline.com/assets/fusion_recipes_cj2.xml, data\fusion_recipes_cj2.xml
had_error := false
if ErrorLevel
{
    MsgBox, Error downloading fusion_recipes_cj2.xml.
    had_error := true
}
UrlDownloadToFile, *0 http://mobile-dev.tyrantonline.com/assets/missions.xml, data\missions.xml
if ErrorLevel
{
    MsgBox, Error downloading missions.xml.
    had_error := true
}
UrlDownloadToFile, *0 http://mobile-dev.tyrantonline.com/assets/skills_set.xml, data\skills_set.xml
if ErrorLevel
{
    MsgBox, Error downloading skills_set.xml.
    had_error := true
}
UrlDownloadToFile, *0 http://mobile-dev.tyrantonline.com/assets/levels.xml, data\levels.xml
if ErrorLevel
{
    MsgBox, Error downloading levels.xml.
    had_error := true
}
Loop, 13
{
    UrlDownloadToFile, *0 http://mobile-dev.tyrantonline.com/assets/cards_section_%A_Index%.xml, data\cards_section_%A_Index%.xml
    if ErrorLevel
    {
        MsgBox, Error downloading cards_section_%A_Index%.xml.
        had_error := true
    }
}
UrlDownloadToFile, *0 https://raw.githubusercontent.com/dsuchka/tyrant_optimize/merged/data/raids.xml, data\raids.xml
if ErrorLevel
{
    MsgBox, Error downloading raids.xml.
    had_error := true
}
UrlDownloadToFile, *0 https://raw.githubusercontent.com/dsuchka/tyrant_optimize/merged/data/bges.txt, data\bges.txt
if ErrorLevel
{
    MsgBox, Error downloading bges.txt.
    had_error := true
}
if !had_error
    MsgBox, 0, Update finished, xml files successfully updated.`nThis Window will auto close in 2 seconds., 2
Gui, Show
return


MenuOwnedcards:
Gui, Submit
Run, Notepad.exe data\ownedcards.txt
Gui, Show
return

MenuCustomdecks:
Gui, Submit
Run, Notepad.exe data\customdecks.txt
Gui, Show
return

MenuCardabbrs:
Gui, Submit
Run, Notepad.exe data\cardabbrs.txt
Gui, Show
return

GuiClose:
ButtonExit:
Gui, Submit
IniWrite, %MyDeck%, %IniFileName%, %IniSection%, MyDeck
IniWrite, %MySiege%, %IniFileName%, %IniSection%, MySiege
IniWrite, %EnemiesDeck%, %IniFileName%, %IniSection%, EnemiesDeck
IniWrite, %EnemySiege%, %IniFileName%, %IniSection%, EnemySiege
IniWrite, %VIP%, %IniFileName%, %IniSection%, VIP
IniWrite, %Effect%, %IniFileName%, %IniSection%, Effect
IniWrite, %Mode%, %IniFileName%, %IniSection%, Mode
IniWrite, %Order%, %IniFileName%, %IniSection%, Order
IniWrite, %Operation%, %IniFileName%, %IniSection%, Operation
IniWrite, %Iterations%, %IniFileName%, %IniSection%, Iterations
IniWrite, %Threads%, %IniFileName%, %IniSection%, Threads
IniWrite, %SimOptions%, %IniFileName%, %IniSection%, SimOptions
IniWrite, %Endgame%, %IniFileName%, %IniSection%, Endgame
IniWrite, %Fund%, %IniFileName%, %IniSection%, Fund
IniWrite, %x86%, %IniFileName%, %IniSection%, x86

while true
{
  IfWinExist, TUOptimizeOutput
      WinClose ; use the window found above
  else
      break
}
ExitApp

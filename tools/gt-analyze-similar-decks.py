#!/usr/bin/python3

import sys
import os
import re

fname = sys.argv[1]
if not os.path.isfile(fname):
    raise Exception("Not a regular file: {}".format(fname))
with_commander = 'no-commander' not in sys.argv[2:]
with_dominion = 'no-dominion' not in sys.argv[2:]

# 96.4578  GW_GT_DEF_107: Battlemaster Krellus, Alpha Scimitar, Silv...
# Arena.Guild.<:-(nick)*(name)-:>: Battlemaster Krellus, Alpha Scimitar, Silv...
DECK_FMT = re.compile("^(?P<prefix>(?P<points>\s*[0-9.]+\s+)?(?P<deck_name>.+?): )(?P<cards>.*)$")
CARD_FMT = re.compile("^(?P<name>.+?)(?P<level>-\d)?\s*(?:\s+#(?P<count>\d+))?$")

class NameCountType:
    CARD_TYPE_COMMANDER = 1
    CARD_TYPE_DOMINION = 2
    CARD_TYPE_NORMAL = 3
    def __init__(self, name, count, card_type):
        self.name = name
        self.count = count
        self.card_type = card_type
        if self.card_type != NameCountType.CARD_TYPE_NORMAL and self.count != 1:
            raise ValueError("card_type={}: must have exactly one copy, but count={}".format(card_type, count))
    def __str__(self):
        if self.card_type == NameCountType.CARD_TYPE_NORMAL:
            return "{} #{}".format(self.name, self.count)
        return self.name
    def __repr__(self):
        return self.__str__()

class Deck:
    def __init__(self, name, cards, points):
        self.name = name
        self.cards = list(cards)
        self.commander = self.cards.pop(0) if self.cards[0].card_type == NameCountType.CARD_TYPE_COMMANDER else None
        self.dominion = self.cards.pop(0) if self.cards[0].card_type == NameCountType.CARD_TYPE_DOMINION else None
        self.cards.sort(key = lambda x: x.name)
        self.points = points
        self.name2card = dict([(c.name, c) for c in cards])
        if not self.commander:
            raise ValueError("no commander for name={}".format(name))
        for c in self.cards:
            if c.card_type != NameCountType.CARD_TYPE_NORMAL:
                raise ValueError("deck contains non-normal card: name={}: card -> {}".format(name, c))
    def __str__(self):
        cards = [self.commander]
        if self.dominion is not None:
            cards.append(self.dominion)
        cards.extend(self.cards)
        cards_view = "<{:02d} unt> {}".format(self.cardsCount(), str(cards))
        if self.points is None:
            return "({}) {}".format(self.name, cards_view)
        else:
            return "({}) [ {:^3.2f} ] {}".format(self.name, self.points, cards_view)
    def __repr__(self):
        return self.__str__()
    def cardsCount(self):
        count = 0
        for card in self.cards:
            count += card.count
        return count
    def similarity_points(self, that):
        cmd_points = 1.0
        if with_commander:
            if self.commander.name != that.commander.name:
                cmd_points = 0.95
        dom_points = 1.0
        if with_dominion:
            if self.dominion.name != that.dominion.name:
                dom_points = 0.9
        max_cards_len = max(self.cardsCount(), that.cardsCount())
        common_cards = 0
        diff_cards = 0
        cards_1 = dict([(c.name, c.count) for c in self.cards])
        cards_2 = dict([(c.name, c.count) for c in that.cards])
        keys = set(list(cards_1.keys()) + list(cards_2.keys()))
        for name in keys:
            count_1 = cards_1[name] if name in cards_1 else 0
            count_2 = cards_2[name] if name in cards_2 else 0
            diff = abs(count_1 - count_2)
            diff_cards += diff
            common_cards += max(count_1, count_2) - diff
        diff_len = max_cards_len - common_cards
        if diff_len:
            value = (common_cards * 2.0 + (diff_cards / (diff_len * 2.0))) / (max_cards_len * 2.0)
        else:
            value = common_cards / max_cards_len
        return value * cmd_points * dom_points

class CmpResult:
    def __init__(self, deck_pair, points):
        self.deck_pair = deck_pair
        self.points = points

decks = {}
with open(fname, 'r') as f:
    for line in f:
        line = line.rstrip()
        m = DECK_FMT.match(line)
        if not m:
            continue
        deck_name = m.group('deck_name')
        points = float(m.group('points')) if m.group('points') else None
        cards = []
        n2c = {}
        for card in m.group('cards').split(', '):
            m = CARD_FMT.match(card)
            if not m:
                raise Exception("bad card: " + card)
            name = m.group('name')
            count = int(m.group('count') or 1)
            if name in n2c:
                n2c[name].count += count
            else:
                card_type = NameCountType.CARD_TYPE_NORMAL
                if len(cards) == 0:
                    card_type = NameCountType.CARD_TYPE_COMMANDER
                elif len(cards) == 1 and ((name.startswith("Alpha ") or name.endswith(" Nexus"))):
                    card_type = NameCountType.CARD_TYPE_DOMINION
                n2c[name] = NameCountType(name, count, card_type)
                cards.append(n2c[name])
        if len(cards) <= 1:
            continue
        decks[deck_name] = Deck(deck_name, cards, points)

deck_names = list(decks.keys())
deck_names.sort()

results = []

def show_result(r):
    n1, n2 = r.deck_pair
    d1, d2 = map(lambda n: decks[n], (n1, n2))
    print("{:<8.3f}\n\t{}\n\t{}".format(r.points, d1, d2))

for i in range(0, len(deck_names)-1):
    for j in range(i+1, len(deck_names)):
        n1, n2 = (deck_names[i], deck_names[j])
        d1, d2 = map(lambda n: decks[n], (n1, n2))
        if None not in (d1.points, d2.points) and (d1.points < d2.points):
            n1, n2 = (n2, n1)
            d1, d2 = (d2, d1)
        sim_points = d1.similarity_points(d2)
        res = CmpResult((n1, n2), sim_points)
        results.append(res)

results.sort(key = lambda x: -x.points)
for r in results[0:max(int(len(results)/50), 1)]:
    show_result(r)

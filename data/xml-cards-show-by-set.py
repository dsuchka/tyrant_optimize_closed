#!/usr/bin/python3

import os
import sys

from xml.dom import minidom
from xml.dom.minidom import Element

XML_DIR = "~/3pp/tyrant_optimize/data/"

MAX_CARDS_SECTIONS = 100

def filterChildNodesByTypeAndName(root, nodeType, nodeName):
    return filter(
            lambda n: (n.nodeType == nodeType)
                and ((nodeName is None) or (n.nodeName == nodeName)),
            root.childNodes)

def getFirstChildNode(n, nodeType, nodeName=None):
    if not n:
        return None
    try:
        return next(filterChildNodesByTypeAndName(n, nodeType, nodeName))
    except StopIteration:
        return None

def getAllChildNodes(n, nodeType, nodeName=None):
    if not n:
        return None
    return list(filterChildNodesByTypeAndName(n, nodeType, nodeName))

def getFirstChildElementNode(n, nodeName):
    return getFirstChildNode(n, Element.ELEMENT_NODE, nodeName)

def getFirstChildTextNodeValue(n):
    try:
        return getFirstChildNode(n, Element.TEXT_NODE).nodeValue
    except AttributeError:
        return None

def getAllChildElementNodes(n, nodeName):
    return getAllChildNodes(n, Element.ELEMENT_NODE, nodeName)

class Card:
    def __init__(self, root_id, name):
        self.root_id = root_id
        self.ids = [root_id]
        self.name = name
        self.set = 0
        self.recipe_from_ids = set()
        self.used_for_ids = set()
    def addLevel(self, next_id):
        self.ids.append(next_id)
    def setSet(self, set):
        self.set = set
    def getLevels(self):
        return len(self.ids)
    def toString(self, level=-1):
        if level == -1:
            level = self.getLevels()
        elif (level < 0) or (level >= self.getLevels()):
            raise ValueError("WTF? Level is " + str(level))
        if (level == self.getLevels()):
            return self.name
        return "{}-{}".format(self.name, level)


# load cards

cards = []
id2cardLv = {}

for i in range(1, MAX_CARDS_SECTIONS):
    xml_fname = os.path.join(os.path.expanduser(XML_DIR), 'cards_section_{}.xml'.format(i))
    if not os.path.exists(xml_fname):
        break

    file_cards_by_id = {}
    xml = minidom.parse(xml_fname)
    root = getFirstChildElementNode(xml, 'root')
    for unit in getAllChildElementNodes(root, 'unit'):
        card_id = int(getFirstChildTextNodeValue(getFirstChildElementNode(unit, 'id')))
        card_name = getFirstChildTextNodeValue(getFirstChildElementNode(unit, 'name'))
        if card_name is None:
            print("Warning: file {}: found an unit(id={}) without name".format(xml_fname, card_id), file=sys.stderr)
            continue
        card = Card(card_id, card_name)
        cards.append(card)
        card_set_raw = getFirstChildTextNodeValue(getFirstChildElementNode(unit, 'set'))
        if not card_set_raw:
            print("Warning: file {}: found an unit(id={}) [{}] without set".format(xml_fname, card_id, card.toString()), file=sys.stderr)
        else:
            card.setSet(int(card_set_raw))

        # upgrades
        lv_id_tuples = [(1, card_id)]
        for upgrade in getAllChildElementNodes(unit, 'upgrade'):
            xid = int(getFirstChildTextNodeValue(getFirstChildElementNode(upgrade, 'card_id')))
            xlv = int(getFirstChildTextNodeValue(getFirstChildElementNode(upgrade, 'level')))
            lv_id_tuples.append((xlv, xid))
        lv_id_tuples.sort(key=lambda x: x[0])
        lvn = 1
        for (xlv, xid) in lv_id_tuples:
            if xlv != lvn:
                raise Exception("xlv={} / lvn={} for card={}".format(xlv, lvn, card.toString()))
            id2cardLv[xid] = (card, xlv)
            lvn = lvn + 1
            if xlv == 1:
                continue
            card.addLevel(xid)

# load recipes
xml_fname = os.path.join(os.path.expanduser(XML_DIR), 'fusion_recipes_cj2.xml')
xml = minidom.parse(xml_fname)
root = getFirstChildElementNode(xml, 'root')
for recipe in getAllChildElementNodes(root, 'fusion_recipe'):
    card_id = int(getFirstChildTextNodeValue(getFirstChildElementNode(recipe, 'card_id')))
    if card_id not in id2cardLv:
        print("Warning: ignore recipe for card_id={} (unknown id)".format(card_id), file=sys.stderr)
        continue
    card = id2cardLv[card_id][0]
    for res in getAllChildElementNodes(recipe, 'resource'):
        res_card_id = int(res.getAttribute('card_id'))
        if res_card_id not in id2cardLv:
            print("Warning: ignore recipe part for card_id={}: unknown resouce card_id={}"
                .format(card_id, res_card_id), file=sys.stderr)
            continue
        res_card = id2cardLv[res_card_id][0]
        card.recipe_from_ids.add(res_card_id)
        res_card.used_for_ids.add(card_id)

def displayForSet(set_number):
    for card in cards:
        if card.set != set_number:
            continue
        def displayCardRecursively(card):
            print(card.toString())
            for xid in card.used_for_ids:
                next_card = id2cardLv[xid][0]
                if len(next_card.recipe_from_ids) != 1:
                    continue
                displayCardRecursively(next_card)
        displayCardRecursively(card)

print()
print("//  ***  Standard  ***")
displayForSet(1000)

print()
print("//  ***  Event  ***")
displayForSet(3000)

print()
print("//  ***  Event Alternate  ***")
displayForSet(3001)

print()
print("//  ***  PvE Rewards  ***")
displayForSet(4000)

print()
print("//  ***  PvE Rewards (hold)  ***")
displayForSet(4001)

print()
print("//  ***  Mutant Rewards  ***")
displayForSet(4250)

print()
print("//  ***  PvP Rewards  ***")
displayForSet(4500)

print()
print("//  ***  PvP Rewards (hold)  ***")
displayForSet(4501)
            
print()
print("//  ***  Legacy PvE Rewards  ***")
displayForSet(4700)

print()
print("//  ***  Legacy PvP Rewards  ***")
displayForSet(4750)
            
print()
print("//  ***  Invisible Rewards  ***")
displayForSet(5000)

print()
print("//  ***  Promotion (Exclusive)  ***")
displayForSet(9000)

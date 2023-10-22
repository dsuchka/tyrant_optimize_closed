#!/usr/bin/python3

import os
import sys
import re
import time
import pickle
import urllib
import urllib3
import json
import readline
import atexit
import certifi
import configparser
import optparse

from random import randint

from xml.dom import minidom
from xml.dom.minidom import Element

from urllib3.util.timeout import Timeout
from urllib3 import PoolManager, Retry

XML_DIR = '~/3pp/tyrant_optimize/data/'

DEFAULT_USER_DB_PATH = '~/.tu-deck-grabber.udb'
DEFAULT_CONFIG_PATH = '~/.tu-deck-grabber.ini'

TUC_VERSION = 'tuc v1.1'
TUF_VERSION = 'tuf v1.1'

#PROTOCOL = 'http'
#API_HOST = 'localhost:8000'
PROTOCOL = 'https'
API_HOST = 'mobile.tyrantonline.com'
API_PATH = 'api.php'


STATIC_HEADERS = {
    'User-Agent': 'Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/72.0.3626.96 Safari/537.36',
    'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8',
    'DNT': '1',
    'Accept-Language': 'en-US,en;q=0.5',
    'Accept-Encoding': 'gzip, compress',
    'Connection': 'keep-alive',
    'Content-Type': 'application/x-www-form-urlencoded',
}


BASIC_BODY_PARAMS = {
    'unity': 'Unity5_4_2',
    'client_version': '80',
    'device_type': 'Chrome+72.0.3626.96',
    'os_version': 'Linux+-',
    'platform': 'Web',
}

# parse options
parser = optparse.OptionParser(
    usage='Usage: %prog [options]'
)

parser.add_option('-u', '--user', metavar='LOGIN', action='store', help='select user')
parser.add_option('-c', '--config', metavar='FILE',
    action='store', default = DEFAULT_CONFIG_PATH,
    help='specify config file [default: %default]'
)
parser.add_option('-b', '--user-db', metavar='FILE',
    action='store', default = DEFAULT_USER_DB_PATH,
    help='specify user-db file [default: %default]'
)
(options, args) = parser.parse_args()

# parse config
config = configparser.ConfigParser()
config_fname = os.path.expanduser(options.config)
if not os.path.exists(config_fname):
    print('ERROR: no such config file: {}'.format(config_fname))
    sys.exit(1)
config.read(config_fname)

# setup personal params
login = options.user or config['CORE']['default_login']
if login not in config:
    print('ERROR: no section named \'{}\' in config file: {}'.format(login, config_fname))
    sys.exit(1)
PERSONAL_BODY_PARAMS = dict(config[login].items())
PERSONAL_URL_PARAMS = {}
PERSONAL_URL_PARAMS['user_id'] = PERSONAL_BODY_PARAMS['user_id']

def getFirstChildNode(n, nodeType, nodeName=None):
    if not n:
        return None
    try:
        return next(filter(
            lambda n: n.nodeType == nodeType and (
                nodeName is None or n.nodeName == nodeName
            ), n.childNodes
        ))
    except StopIteration:
        return None

def getAllChildNodes(n, nodeType, nodeName=None):
    if not n:
        return None
    return list(filter(
        lambda n: n.nodeType == nodeType and (
            nodeName is None or n.nodeName == nodeName
        ), n.childNodes
    ))

def getFirstChildElementNode(n, nodeName):
    return getFirstChildNode(n, Element.ELEMENT_NODE, nodeName)

def getFirstChildTextNodeValue(n):
    try:
        return getFirstChildNode(n, Element.TEXT_NODE).nodeValue
    except AttributeError:
        return None

def getAllChildElementNodes(n, nodeName):
    return getAllChildNodes(n, Element.ELEMENT_NODE, nodeName)


class UserDbEntry:
    def __init__(self, kv):
        self.user_id = int(kv['user_id'])
        self.name = kv['name']
        self.guild_name = (kv['guild_name'] or '') if ('guild_name' in kv) else ''
        self.level = int(kv['level'])
        self.elo = int(kv['hunting_elo'])
        self._refreshed_at = int(time.time())
    def __cmp__(self, other):
        if self.user_id > other.user_id: return 1
        if self.user_id < other.user_id: return -1
        if self.name > other.name: return 1
        if self.name < other.name: return -1
        if self.guild_name > other.guild_name: return 1
        if self.guild_name < other.guild_name: return -1
        if self.level > other.level: return 1
        if self.level < other.level: return -1
        return 0
    def __eq__(self, other):
        return self.__cmp__(other) == 0
    def __str__(self):
        return '[{}] {} ({} lvl / elo: {})'.format(self.guild_name, self.name, self.level, self.elo)

class UserDb:
    def __init__(self, entries = None):
        self.id2entry = dict((x.user_id, x) for x in (entries or []))
        self._dirty = False
    def append(self, entry):
        self.id2entry[entry.user_id] = entry
        self._refreshed_at = int(time.time())
        self._dirty = True
    def getByUserId(self, uid):
        if uid in self.id2entry:
            return self.id2entry[uid]
        return None
    def size(self):
        return len(self.id2entry)

user_db = UserDb()

udb_fname = os.path.expanduser(options.user_db)
if os.path.exists(udb_fname):
    with open(udb_fname, 'rb') as f:
        user_db = pickle.load(f)
        print('INFO: user-db(total {} entries) loaded from dump: {}'.format(user_db.size(), udb_fname))

id_to_cards = {}
fusion_from_cards = {}
fusion_into_cards = {}

faction_id_to_name_scname_tuple = {
    1: ('Imperial', 'im'),
    2: ('Raider', 'rd'),
    3: ('Bloodthirsty', 'bt'),
    4: ('Xeno', 'xn'),
    5: ('Righteous', 'rt'),
    6: ('Progenitor', 'pg'),
}

rarity_id_to_name_scname_tuple = {
    1: ('Common', 'cm'),
    2: ('Rare', 'rr'),
    3: ('Epic', 'ep'),
    4: ('Legendary', 'lg'),
    5: ('Vindicator', 'vd'),
    6: ('Mythic', 'mt'),
}

fusion_id_to_name = {
    0: 'Single',
    1: 'Dual',
    2: 'Quad',
}

def getFactionName(factionId):
    if factionId in faction_id_to_name_scname_tuple:
        return faction_id_to_name_scname_tuple[factionId][0]
    return '(Unknown faction: {})'.format(factionId)

def getFactionShortcutName(factionId):
    if factionId in faction_id_to_name_scname_tuple:
        return faction_id_to_name_scname_tuple[factionId][1]
    return '(Unknown faction: {})'.format(factionId)

def getRarityName(rarityId):
    if rarityId in rarity_id_to_name_scname_tuple:
        return rarity_id_to_name_scname_tuple[rarityId][0]
    return '(Unknown rarity: {})'.format(rarityId)

def getRarityShortcutName(rarityId):
    if rarityId in rarity_id_to_name_scname_tuple:
        return rarity_id_to_name_scname_tuple[rarityId][1]
    return '(Unknown rarity: {})'.format(rarityId)

def getFusionName(fusionId):
    if fusionId in fusion_id_to_name:
        return fusion_id_to_name[fusionId]
    return '(Unknown fusion: {})'.format(fusionId)

def parseUnitSkills(root):
    skills = []
    for skill_node in getAllChildElementNodes(root, 'skill'):
        skill = {}
        for x in ('id', 'x', 'y', 'all', 'trigger', 'n', 'c', 's', 'card_id'):
            v = skill_node.getAttributeNode(x)
            if v:
                skill[x] = v.value
        if 'id' not in skill:
            raise Exception('skill without id: {}'.format(skill))
        skills.append(skill)
    return skills

def formatUnitSkill(skill):
    fmt = skill['id']
    if 'trigger' in skill and skill['trigger']:
        fmt = '[On {}] {}'.format(skill['trigger'], fmt)
    if 'n' in skill and skill['n']:
        fmt += ' ' + str(skill['n'])
    elif 'all' in skill and skill['all']:
        fmt += ' all'
    if 'y' in skill and skill['y']:
        fmt += ' ' + getFactionName(int(skill['y']))
    if 's' in skill and skill['s']:
        fmt += ' ' + str(skill['s'])
    if 'x' in skill and skill['x']:
        fmt += ' ' + str(skill['x'])
    if 'c' in skill and skill['c']:
        fmt += ' every ' + str(skill['c'])
    if 'card_id' in skill and skill['card_id']:
        fmt += ' [card "{}"]'.format(getCardNameById(int(skill['card_id'])))
    return fmt

# fusions
xml_fname = os.path.join(os.path.expanduser(XML_DIR), 'fusion_recipes_cj2.xml')
while os.path.exists(xml_fname):
    st = os.stat(xml_fname)
    xml_file_hash = int(st.st_mtime * 0x1b1 + st.st_size) & 0xFFFFFFFF
    print('INFO: fusion recipes xml file: {} (hash: 0x{:08x})'.format(xml_fname, xml_file_hash))

    dump_fname = re.sub('\.xml$', '.pck', xml_fname)
    if os.path.exists(dump_fname):
        with open(dump_fname, 'rb') as f:
            xmagic = pickle.load(f)
            if xmagic != TUF_VERSION:
                print('INFO: reloading {}: old format'.format(xml_fname))
            else:
                xhash = pickle.load(f)
                if xhash == xml_file_hash:
                    fusion_from_cards.update(pickle.load(f))
                    fusion_into_cards.update(pickle.load(f))
                    print('INFO: image of {} loaded from dump: {}'.format(xml_fname, dump_fname))
                    break
                print('INFO: reloading {}: dumped hash(0x{:08x}) <> actual hash(0x{:08x})'.format(xml_fname, xhash, xml_file_hash))

    xml = minidom.parse(xml_fname)
    root = getFirstChildElementNode(xml, 'root')
    for recp in getAllChildElementNodes(root, 'fusion_recipe'):
        def recp_get(name, castType=None, defValue=None):
            v = getFirstChildTextNodeValue(getFirstChildElementNode(recp, name))
            return defValue if not v else castType(v) if castType else v
        card_id = recp_get('card_id', int)
        for res in getAllChildElementNodes(recp, 'resource'):
            def recp_get_attr(attr, castType=None, defValue=None):
                v = res.getAttributeNode(attr).value
                return defValue if not v else castType(v) if castType else v
            res_card_id = recp_get_attr('card_id', int)
            count = recp_get_attr('number', int)

            # id -> {resources=count}
            if (card_id not in fusion_from_cards):
                fusion_from_cards[card_id] = {}
            fusion_from_cards[card_id][res_card_id] = count

            # id -> [available fusions]
            if (res_card_id not in fusion_into_cards):
                fusion_into_cards[res_card_id] = []
            if (card_id not in fusion_into_cards[res_card_id]):
                fusion_into_cards[res_card_id].append(card_id)

    # dump file image
    with open(dump_fname, 'wb') as f:
        pickle.dump(TUF_VERSION, f)
        pickle.dump(xml_file_hash, f)
        pickle.dump(fusion_from_cards, f)
        pickle.dump(fusion_into_cards, f)
        print('INFO: {} parsed & image saved to dump: {}'.format(xml_fname, dump_fname))
    break # exit loop

# cards
for i in range(1, 100):
    xml_fname = os.path.join(os.path.expanduser(XML_DIR), 'cards_section_{}.xml'.format(i))
    if not os.path.exists(xml_fname):
        break
    st = os.stat(xml_fname)
    xml_file_hash = int(((st.st_mtime * 31) + i) * 31 + st.st_size) & 0xFFFFFFFF
    print('INFO: next cards xml file: {} (hash: 0x{:08x})'.format(xml_fname, xml_file_hash))

    dump_fname = re.sub('\.xml$', '.pck', xml_fname)
    if os.path.exists(dump_fname):
        with open(dump_fname, 'rb') as f:
            xmagic = pickle.load(f)
            if xmagic != TUC_VERSION:
                print('INFO: reloading {}: old format'.format(xml_fname))
            else:
                xhash = pickle.load(f)
                if xhash == xml_file_hash:
                    id_to_cards.update(pickle.load(f))
                    print('INFO: image of {} loaded from dump: {}'.format(xml_fname, dump_fname))
                    continue
                print('INFO: reloading {}: dumped hash(0x{:08x}) <> actual hash(0x{:08x})'.format(xml_fname, xhash, xml_file_hash))

    file_cards_by_id = {}
    xml = minidom.parse(xml_fname)
    root = getFirstChildElementNode(xml, 'root')
    for unit in getAllChildElementNodes(root, 'unit'):
        def unit_get(name, castType=None, defValue=None, unit=unit):
            v = getFirstChildTextNodeValue(getFirstChildElementNode(unit, name))
            return defValue if not v else castType(v) if castType else v
        cards = []
        card = {}
        card['id'] = unit_get('id', int)
        card['name'] = unit_get('name')
        card['set'] = unit_get('set', int) or 0
        card['fusion'] = unit_get('fusion_level', int) or 0
        card['attack'] = unit_get('attack', int) or 0
        card['hp'] = unit_get('health', int) or 0
        card['delay'] = unit_get('cost', int) or 0
        card['rarity'] = unit_get('rarity', int) or 1
        card['faction'] = unit_get('type', int) or 1
        card['level'] = unit_get('level', int) or 1

        if card['id'] is None:
            print('Warning: file {}: found an unit(name={}) without id'.format(xml_fname, card['name']))
            continue

        if card['name'] is None:
            print('Warning: file {}: found an unit(id={}) without name'.format(xml_fname, card['id']))
            continue

        level_raw = unit_get('level')
        card['full_name'] = '{}-{}'.format(card['name'], card['level'])
        card['maxed'] = False
        low_level_id = card['id']
        top_level_id = card['id']
        file_cards_by_id[top_level_id] = card

        card['skills'] = parseUnitSkills(unit)

        cards.append(card)

        card_prev = card
        for upgrade in getAllChildElementNodes(unit, 'upgrade'):
            def upgrade_get(name, castType=None, defValue=None):
                return unit_get(name, castType=castType, unit=upgrade, defValue=defValue)
            card_next = {}
            card_next.update(card_prev)
            card_next['id'] = upgrade_get('card_id', int)
            card_next['level'] = upgrade_get('level', int)
            card_next['hp'] = upgrade_get('health', int) or card_prev['hp']
            card_next['attack'] = upgrade_get('attack', int) or card_prev['attack']
            card_next['delay'] = upgrade_get('cost', int) or card_prev['delay']
            card_next['full_name'] = '{}-{}'.format(card_next['name'], card_next['level'])
            card_next['skills'] = parseUnitSkills(upgrade) or card_prev['skills']
            top_level_id = card_next['id']
            file_cards_by_id[top_level_id] = card_next
            cards.append(card_next)
            card_prev = card_next

        file_cards_by_id[top_level_id]['full_name'] = card['name']
        file_cards_by_id[top_level_id]['maxed'] = True

        for card in cards:
            card['low_id'] = low_level_id
            card['top_id'] = top_level_id

    # dump file image
    with open(dump_fname, 'wb') as f:
        pickle.dump(TUC_VERSION, f)
        pickle.dump(xml_file_hash, f)
        pickle.dump(file_cards_by_id, f)
        print('INFO: {} parsed & image saved to dump: {}'.format(xml_fname, dump_fname))
    id_to_cards.update(file_cards_by_id)

# apply orig_set (only for fusions with single source)
for card in id_to_cards.values():
    def get_orig_set(card):
        if (not card): return None
        if (not card['fusion']): return card['set']
        res = fusion_from_cards.get(card['low_id'])
        if (not res) or (len(res) != 1): return None
        return get_orig_set(id_to_cards.get(next(iter(res.keys()))))
    orig_set = get_orig_set(card)
    if (orig_set):
        card['orig_set'] = orig_set

def getCardNameById(card_id):
    if card_id in id_to_cards:
        return id_to_cards[card_id]['full_name']
    return '[{}]'.format(card_id)

def encodeUrlParams(kvmap):
    return '&'.join('{}={}'.format(k, v) for (k, v) in kvmap.items())

class TUApiRequestData:
    HTTP_VERB_POST = 'POST'
    HTTP_VERB_GET = 'GET'

    def __init__(self):
        self.verb = TUApiRequestData.HTTP_VERB_POST
        self.url_params = {}
        self.body_params = {}

    def updateUrlParams(self, params):
        self.url_params.update(params)

    def setUrlParamMessage(self, message):
        self.url_params['message'] = message

    def getUrlParamMessage(self):
        return self.url_params['message']

    def updateBodyParams(self, params):
        self.body_params.update(params)

    def setDefaultParams(self, api_stat_name):
        self.url_params.update(PERSONAL_URL_PARAMS)
        if api_stat_name:
            self.body_params.update({
                'api_stat_name': api_stat_name,
                'api_stat_time': str(randint(22,777)),
            })
        self.body_params.update({
            'data_usage': str(randint(11111, 888888)),
            'timestamp': str(int(time.time())),
        })
        self.body_params.update(BASIC_BODY_PARAMS)
        self.body_params.update(PERSONAL_BODY_PARAMS)

    def getUrlParamsEncoded(self):
        return encodeUrlParams(self.url_params)

    def getBodyParamsEncoded(self):
        return encodeUrlParams(self.body_params)

class TUApiClient:
    def __init__(self, http, login, defaultTries = 3):
        self.http = http
        self.defaultTries = defaultTries
        self.lastBuybackData = {}
        self.lastUserCards = {}
        self.lastUserDecks = {}
        self.lastUserData = {}
        self.login = login

    def __enrich_last_data(self, resp):
        if 'buyback_data' in resp:
            self.lastBuybackData = resp['buyback_data']
        if 'user_cards' in resp:
            self.lastUserCards = resp['user_cards']
        if 'user_data' in resp:
            self.lastUserData = resp['user_data']
        if 'user_decks' in resp:
            self.lastUserDecks = resp['user_decks']

    def getUserName(self):
        if self.lastUserData and 'name' in self.lastUserData:
            return self.lastUserData['name']
        return 'login:' + self.login

    def __run_api_req_with_retries(self, method_name, request_data, tries = None):
        try_no = 0
        tries = (self.defaultTries) if (tries is None) else (tries)
        while (try_no < tries):
            try_no += 1
            r = http.request(
                request_data.verb,
                '{}://{}/{}?{}'.format(PROTOCOL, API_HOST, API_PATH, request_data.getUrlParamsEncoded()),
                headers = STATIC_HEADERS,
                decode_content = True,
                preload_content = False,
                body = request_data.getBodyParamsEncoded()
            )
            data = r.read().decode('UTF-8')
            if not data:
                print('WARN: {}: no data (try #{})'.format(method_name, try_no))
                continue
            prev_name, last_name = ['/tmp/.tu-deck-grabber.{}.{}'.format(method_name, x) for x in ('prev', 'last')]
            if os.path.exists(last_name):
                os.rename(last_name, prev_name)
            with open(last_name, 'wb') as f:
                f.write(bytes(data, 'UTF-8'))
                f.flush()
            resp = json.loads(data)
            self.__enrich_last_data(resp)
            return resp
        print('ERROR: {}: no data (all tries are spent)'.format(method_name))
        return None

    def __mkRequestData(self, method_name, body_params, api_stat_name = None):
        request_data = TUApiRequestData()
        request_data.setUrlParamMessage(method_name)
        request_data.setDefaultParams(api_stat_name or method_name)
        request_data.updateBodyParams(body_params)
        return request_data

    def getUserAccount(self):
        rd = self.__mkRequestData('getUserAccount', { 'udid': 'n/a' }, api_stat_name = None)
        return self.__run_api_req_with_retries(rd.getUrlParamMessage(), rd)

    def runInit(self):
        rd = self.__mkRequestData('init', {}, api_stat_name = 'getUserAccount')
        return self.__run_api_req_with_retries(rd.getUrlParamMessage(), rd)

    def getHuntingTargets(self):
        rd = self.__mkRequestData('getHuntingTargets', {
            'dummy': 'data',
        })
        return self.__run_api_req_with_retries(rd.getUrlParamMessage(), rd)

    def getBattleResults(self):
        rd = self.__mkRequestData('getBattleResults', {
            'battle_id': '0',
            'host_id': '0',
        })
        return self.__run_api_req_with_retries(rd.getUrlParamMessage(), rd)

    def salvageCard(self, cid):
        rd = self.__mkRequestData('salvageCard', {
            'card_id': str(cid),
        })
        return self.__run_api_req_with_retries(rd.getUrlParamMessage(), rd)

    def salvageL1CommonCards(self):
        rd = self.__mkRequestData('salvageL1CommonCards', {})
        return self.__run_api_req_with_retries(rd.getUrlParamMessage(), rd)

    def salvageL1RareCards(self):
        rd = self.__mkRequestData('salvageL1RareCards', {})
        return self.__run_api_req_with_retries(rd.getUrlParamMessage(), rd)

    def getDecksText(self, card_id2name_resolver = getCardNameById):
        if not self.lastUserDecks:
            return None
        txt = '// Begin decks dump for user: {}\n'.format(self.getUserName())
        def _get_attr(d, name, typ=str):
            if name not in d or d[name] is None:
                return None
            return typ(d[name])
        decks = list(self.lastUserDecks.values())
        decks.sort(key = lambda x: _get_attr(x, 'card_id', typ=int) or 0)
        for deck in decks:
            deck_id = _get_attr(deck, 'deck_id', typ=int)
            txt += '\n// Deck #' + str(deck_id)
            name = _get_attr(deck, 'name')
            if name:
                txt += ' "{}"'.format(name)
            if _get_attr(self.lastUserData, 'active_deck', typ=int) == deck_id:
                txt += ' (active)'
            if _get_attr(self.lastUserData, 'defense_deck', typ=int) == deck_id:
                txt += ' (defense)'
            txt += '\n'
            deck_txt = card_id2name_resolver(_get_attr(deck, 'commander_id', typ=int))
            dom_id = _get_attr(deck, 'dominion_id', typ=int)
            if dom_id:
                deck_txt += ', ' + card_id2name_resolver(dom_id)
            for card_id, card_count in deck['cards'].items():
                deck_txt += ', ' + card_id2name_resolver(int(card_id))
                if int(card_count) > 1:
                    deck_txt += ' #' + str(card_count)
            txt += '{}.deck_{}: {}\n'.format(self.getUserName(), deck_id, deck_txt)
        txt += '\n// End of decks dump\n'
        return txt

    def getCardsText0(self, data, number_field, card_id2name_resolver = getCardNameById):
        txt = ''
        factions = [1, 2, 3, 4, 5, 6]
        rarities = [6, 5, 4, 3]
        f_r_cards = {}
        def _set_f_r_cards(c_id, count):
            if c_id not in id_to_cards:
                raise Exception('No such card: id=' + str(c_id))
            c = id_to_cards[c_id]
            f = c['faction']
            r = c['rarity']
            if f not in f_r_cards:
                f_r_cards[f] = {}
            if r not in f_r_cards[f]:
                f_r_cards[f][r] = {}
            f_r_cards[f][r][c_id] = count
        for c_id, c_obj in data.items():
            c_id = int(c_id)
            count = int(number_field in c_obj and c_obj[number_field] or 0)
            if count:
                _set_f_r_cards(c_id, count)
        for f in factions:
            if f not in f_r_cards:
                continue
            txt += '\n// *** [ {} ] ***\n'.format(getFactionName(f))
            for r in rarities:
                if r not in f_r_cards[f]:
                    continue
                txt += '// ++ (( {} )) ++\n'.format(getRarityName(r))
                for c_id, count in f_r_cards[f][r].items():
                    txt += card_id2name_resolver(c_id)
                    txt += ' (+' + str(count) + ')'
                    txt += '\n'
        return txt

    def getBuybackText(self, card_id2name_resolver = getCardNameById):
        if not self.lastBuybackData:
            return None
        txt = '// Buyback cards of user "{}"\n'.format(self.getUserName())
        txt += self.getCardsText0(self.lastBuybackData, 'number', card_id2name_resolver)
        return txt

    def getCardsText(self, card_id2name_resolver = getCardNameById):
        if not self.lastUserCards:
            return None
        txt = '// Cards of user "{}" (without buyback)\n'.format(self.getUserName())
        txt += self.getCardsText0(self.lastUserCards, 'num_owned', card_id2name_resolver)
        return txt

def doHuntAndEnrichUserDb(client):
    global orig_res
    res = orig_res = client.getHuntingTargets()
    if (not res) or ('hunting_targets' not in res):
        print('WARN: hunt failed')
        return
    res = res['hunting_targets']
    for (uid, entry) in res.items():
        new_entry = UserDbEntry(entry)
        old_entry = user_db.getByUserId(new_entry.user_id)
        if old_entry is None:
            user_db.append(new_entry)
            print('INFO: user-db: added entry: {}'.format(new_entry))
        else:
            if new_entry == old_entry:
                continue
            print('INFO: user-db: upgraded entry: {} -> {}'.format(old_entry, new_entry))
            user_db.append(new_entry)
    if user_db._dirty:
        user_db._dirty = False
        tmp_udb_fname = udb_fname + '~'
        old_udb_fname = udb_fname + '.old'
        with open(tmp_udb_fname, 'wb') as f:
            pickle.dump(user_db, f)
        if (os.path.exists(udb_fname)):
            os.renames(udb_fname, old_udb_fname)
        os.renames(tmp_udb_fname, udb_fname)
        print('INFO: user-db: synced to disk: {} (total {} entries)'.format(udb_fname, user_db.size()))

def doGrabLastDeck(client):
    global orig_res
    res = orig_res = client.getBattleResults()
    if (not res) or ('battle_data' not in res):
        print('WARN: grab failed')
        return
        return
    res = res['battle_data']
    enemy_id = int(res['enemy_id'] or -1)
    enemy_name = res['enemy_name'] if ('enemy_name' in res) else '__UNNAMED__'
    enemy_size = int(res['eds'] or 1000)
    host_size = int(res['hds'] or 1000)
    enemy_udb_entry = user_db.id2entry[enemy_id] if (enemy_id in user_db.id2entry) else None
    end_time = int(res['end_time']) if ('end_time' in res) else int(time.time())
    winner = int(res['winner']) if ('winner' in res) else None
    rewards_list = res['rewards'] if ('rewards' in res) else None
    host_is_attacker = bool(res['host_is_attacker'])
    enemy_commander_id = int(res['defend_commander' if host_is_attacker else 'attack_commander'])
    card_map = dict((int(k), int(v)) for (k, v) in res['card_map'].items())
    is_attacker_card = lambda x: 1 <= x <= 50
    is_attacker_fort = lambda x: 51 <= x < 100
    is_defender_card = lambda x: 101 <= x <= 150
    is_defender_fort = lambda x: 151 <= x < 200
    is_attacker_both = lambda x: is_attacker_card(x) or is_attacker_fort(x)
    is_defender_both = lambda x: is_defender_card(x) or is_defender_fort(x)
    enemy_predicate = is_defender_both if host_is_attacker else is_attacker_both
    enemy_card_predicate = is_defender_card if host_is_attacker else is_attacker_card
    enemy_fort_predicate = is_defender_fort if host_is_attacker else is_attacker_fort
    enemy_card_id_to_count = {}
    enemy_forts = []
    enemy_played_cards_count = 0
    for (card_uid, card_id) in card_map.items():
        if not enemy_predicate(card_uid):
            continue
        if enemy_card_predicate(card_uid):
            if card_id in enemy_card_id_to_count:
                enemy_card_id_to_count[card_id] += 1
            else:
                enemy_card_id_to_count[card_id] = 1
        elif enemy_fort_predicate(card_uid):
            enemy_forts.append(card_id)
        else:
            pass # TODO notice?

        if not enemy_fort_predicate(card_uid):
            enemy_played_cards_count += 1

    # determine game type
    game_type = 'Arena'
    pvp_points = None
    if not host_is_attacker:
        game_type = 'Unknown'
        if rewards_list is not None:
            for rewards in rewards_list:
                if 'pvp_points' in rewards:
                    pvp_points = int(rewards['pvp_points'])
                    game_type = 'Brawl'
                    break
                if 'war_points' in rewards:
                    game_type = 'GW'
                if 'conquest_influence' in rewards:
                    game_type = 'CQ'

    # deck header
    enemy_guild_name = enemy_udb_entry and enemy_udb_entry.guild_name or '__UNKNOWN__'
    out = ''
    if config.getboolean('CORE', 'output_game_type'):
        out += game_type + '.'
    if config.getboolean('CORE', 'output_timestamp'):
        out += time.strftime('%Y%m%d', time.localtime(end_time)) + '.'
    if config.getboolean('CORE', 'output_winlose'):
        if winner is not None:
            out += 'Win.' if winner else 'Lose.'
        else:
            out += 'BIP.' # Battle In Progress
    if config.getboolean('CORE', 'output_pvp_points'):
        if pvp_points is not None:
            out += 'pvp{:02d}.'.format(pvp_points)
    if config.getboolean('CORE', 'output_hds'):
        out += 'hds{:02d}.'.format(host_size)
    if config.getboolean('CORE', 'output_eds'):
        out += 'eds{:02d}.'.format(enemy_size)
    if config.getboolean('CORE', 'output_guild'):
        out += re.sub('(?a)[^\w]', '_', enemy_guild_name) + '.'
    out += re.sub('(?a)[^\w]', '_', enemy_name)
    if config.getboolean('CORE', 'output_missing'):
        missing_cards = enemy_size - enemy_played_cards_count
        if (missing_cards > 0):
            out += '.m{}'.format(missing_cards)
    out += ': '

    # append commander
    out += getCardNameById(enemy_commander_id)

    # append forts
    for card_id in enemy_forts:
        out += ', ' + getCardNameById(card_id)

    # append cards
    card_ids_ordered = list(enemy_card_id_to_count.keys())
    sort_key = (lambda cid: getCardNameById(cid)) if config.getboolean('CORE', 'sort_cards_by_name') else (lambda cid: cid)
    card_ids_ordered.sort(reverse=False, key=sort_key)
    for card_id in card_ids_ordered:
        count = enemy_card_id_to_count[card_id]
        out += ', ' + getCardNameById(card_id)
        if (count > 1):
            out += ' #' + str(count)

    print('Grabbed deck: ' + out)

## configure readline
histfile = os.path.join(os.path.expanduser('~'), '.tu_deck_grabber_history')
if os.path.exists(histfile):
    readline.read_history_file(histfile)
readline.set_history_length(1000)
readline.read_init_file()
atexit.register(readline.write_history_file, histfile)

with PoolManager(1,
            timeout = Timeout(connect=15.0, read=20.0, total=30.0),
            retries = Retry(total=3),
            cert_reqs = 'CERT_REQUIRED',
            ca_certs = certifi.where(),
        ) as http:

    client = TUApiClient(http, login)

    while True:
        try:
            line = input('{} ~> '.format(login))
        except EOFError:
            print()
            line = None
        if line is None:
            break
        elif not line:
            continue
        args = []
        line = line.strip() + ' '
        word = None
        quote = False
        esc = False
        for c in line:
            if not esc:
                if c == '\\':
                    esc = True
                    continue
                if c == ' ' and not quote:
                    if word:
                        args.append(word)
                        word = None
                    continue
                if c == '"':
                    quote = not quote
                    continue
            else:
                esc = False
            if not quote:
                c = c.lower()
            word = word + c if word else c
        if args[0] == 'exit':
            break
        if args[0] == 'init':
            client.getUserAccount()
            client.runInit()
            continue
        if args[0] == 'grab':
            doGrabLastDeck(client)
            continue
        if args[0] == 'hunt':
            doHuntAndEnrichUserDb(client)
            continue
        if args[0] == 'salvage':
            if len(args) < 2:
                print('USAGE: salvage <CARD_ID|commons|rares> [COUNT]')
                continue
            if args[1] == 'commons':
                rsp = client.salvageL1CommonCards()
            elif args[1] == 'rares':
                rsp = client.salvageL1RareCards()
            elif re.match(r'^\d+$', args[1]):
                cid = int(args[1])
                count = 1 if len(args) < 3 else int(args[2])
                for i in range(0, count):
                    rsp = client.salvageCard(cid)
            sp = rsp['user_data']['salvage']
            print('SP: {}'.format(sp))
            continue
        if args[0] == 'card':
            if len(args) < 2:
                print('USAGE: card <ID|"NAME"|"/REGEX/">')
                continue
            def showCard(card, with_fusions=True):
                print(' >> Card [{}] {}'.format(card['id'], card['full_name']))
                print('    {} {} {} (set {}{})'.format(
                    getFusionName(card['fusion']),
                    getRarityName(card['rarity']),
                    getFactionName(card['faction']),
                    card['set'],
                    (', orig set {}'.format(card['orig_set']) if ('orig_set' in card) else '')
                ))
                print('    ATT {} / HP {} / CD {}'.format(card['attack'], card['hp'], card['delay']))
                for s in card['skills']:
                    print('      >  ' + formatUnitSkill(s))
                if (with_fusions):
                    f_from = fusion_from_cards.get(card['low_id'], None)
                    f_into = fusion_into_cards.get(card['top_id'], None)
                    if (f_from):
                        print('    Fused from:')
                        for f_c_id, cnt in f_from.items():
                            print('      > card "{}" x {}'.format(getCardNameById(f_c_id), cnt))
                    if (f_into):
                        print('    Fuses into:')
                        for f_c_id in f_into:
                            print('      > card "{}"'.format(getCardNameById(f_c_id)))
            if re.match(r'^\d+$', args[1]):
                c_id = int(args[1])
                if c_id not in id_to_cards:
                    print('No such card: id=' + args[1])
                    continue
                showCard(id_to_cards[c_id])
            else:
                if re.match(r'^/.+/$', args[1]):
                    pattern_regex = re.compile(args[1][1:-1])
                    pattern_pred = lambda c: pattern_regex.match(c['full_name']) and c['maxed']
                else:
                    pattern_pred = lambda c: c['full_name'] == args[1]
                for c in id_to_cards.values():
                    if pattern_pred(c):
                        showCard(c)
            continue
        if args[0] == 'dump':
            if len(args) < 2:
                print('USAGE: dump <tuc|tuf|decks|cards|buyback> [PATH_TO_FILE]')
                continue
            out_fname = None if len(args) < 3 else args[2]
            def write_dump(data, append = False):
                if out_fname:
                    mode = append and 'wta' or 'wt'
                    w_size = 0
                    with open(out_fname, mode) as f:
                        w_size = f.write(data)
                        f.flush()
                    print('done (file: \'{}\', written {} bytes)'.format(out_fname, w_size))
                    return
                sys.stdout.write(' --- >> BEGIN DUMP DATA << ---\n')
                sys.stdout.write(data)
                if not data.endswith('\n'):
                    sys.stdout.write('\n')
                sys.stdout.write(' --- >> END DUMP DATA << ---\n')
                sys.stdout.flush()
            if args[1] == 'tuc':
                try:
                    write_dump(json.dumps(id_to_cards, indent=2))
                except Exception as e:
                    print('could not dump tuc data: {}'.format(e))
                continue
            if args[1] == 'tuf':
                try:
                    m = {'from': fusion_from_cards, 'into': fusion_into_cards}
                    write_dump(json.dumps(m, indent=2))
                except Exception as e:
                    print('could not dump tuf data: {}'.format(e))
                continue
            if args[1] == 'decks':
                try:
                    write_dump(client.getDecksText())
                except Exception as e:
                    print('could not dump decks: {}'.format(e))
                    import traceback
                    traceback.print_exc()
                continue
            if args[1] == 'cards':
                try:
                    write_dump(client.getCardsText())
                except Exception as e:
                    print('could not dump cards: {}'.format(e))
                    import traceback
                    traceback.print_exc()
                continue
            if args[1] == 'buyback':
                try:
                    write_dump(client.getBuybackText())
                except Exception as e:
                    print('could not dump cards: {}'.format(e))
                    import traceback
                    traceback.print_exc()
                continue
        print('ERROR: unknown command: {} (supported: [ grab | hunt | dump | exit ])'.format(line))

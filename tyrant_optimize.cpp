// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//------------------------------------------------------------------------------
//#define NDEBUG
#define BOOST_THREAD_USE_LIB
#include <cassert>
#include <chrono>
#include <cstring>
#include <ctime>
#include <functional>
#include <iostream>
#include <map>
#include <unordered_map>
#include <set>
#include <stack>
#include <string>
#include <tuple>
#include <vector>
#include <math.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/math/distributions/binomial.hpp>
#include <boost/optional.hpp>
#include <boost/range/join.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/timer/timer.hpp>
#include <boost/tokenizer.hpp>
#include "card.h"
#include "cards.h"
#include "deck.h"
#include "read.h"
#include "sim.h"
#include "tyrant.h"
#include "xml.h"

#if defined(ANDROID) || defined(__ANDROID__)
#include <jni.h>
#include <android/log.h>
#endif
struct Requirement
{
    std::unordered_map<const Card*, unsigned> num_cards;
};

namespace {
    gamemode_t gamemode{fight};
    OptimizationMode optimization_mode{OptimizationMode::notset};
    std::unordered_map<unsigned, unsigned> owned_cards;
    const Card* owned_alpha_dominion{nullptr};
    bool use_owned_cards{true};
    unsigned min_deck_len{1};
    unsigned max_deck_len{10};
    unsigned freezed_cards{0};
    unsigned fund{0};
    long double target_score{100};
    long double min_increment_of_score{0};
    long double confidence_level{0.99};
    bool use_top_level_card{true};
    bool use_top_level_commander{true};
    bool mode_open_the_deck{false};
    bool use_owned_dominions{true};
    bool use_maxed_dominions{false};
    unsigned use_fused_card_level{0};
    unsigned use_fused_commander_level{0};
    bool print_upgraded{false};
    bool print_values{false};
    bool simplify_output{false};
    bool show_ci{false};
    bool use_harmonic_mean{false};
    unsigned iterations_multiplier{10};
    unsigned sim_seed{0};
    unsigned flexible_iter{20};
    unsigned flexible_turn{10};
    Requirement requirement;
#ifndef NQUEST
    Quest quest;
#endif
    std::unordered_set<unsigned> allowed_candidates;
    std::unordered_set<unsigned> disallowed_candidates;

    std::chrono::time_point<std::chrono::system_clock> start_time;
    long double maximum_time{0};
    long double temperature = 1000;
    long double coolingRate = 0.001;
    unsigned yfpool{0};
    unsigned efpool{0};

    std::vector<Faction> factions;
    bool invert_factions{false};
    bool only_recent{false};
    bool prefered_recent{false};
    unsigned recent_percent{5};
    std::vector<Skill::Skill> skills;
    bool invert_skills{false};
    std::vector<Skill::Skill> prefered_skills;
    unsigned prefered_factor{3};

	#if defined(ANDROID) || defined(__ANDROID__)
	JNIEnv *envv;
    jobject objv;
    #endif
}

void init()
{
    gamemode = fight;
    optimization_mode =OptimizationMode::notset;
    owned_cards.clear();
    owned_alpha_dominion = nullptr;
    use_owned_cards=true;
    min_deck_len=1;
    max_deck_len=10;
    freezed_cards=0;
    fund=0;
    target_score=100;
    min_increment_of_score=0;
    confidence_level=0.99;
    use_top_level_card=true;
    use_top_level_commander=true;
    mode_open_the_deck=false;
    use_owned_dominions=true;
    use_maxed_dominions=false;
    use_fused_card_level=0;
    use_fused_commander_level=0;
    print_upgraded=false;
    print_values=false;
    simplify_output=false;
    show_ci=false;
    use_harmonic_mean=false;
    iterations_multiplier=10;
    sim_seed=0;
    flexible_iter=20;
    flexible_turn=10;
    requirement.num_cards.clear();
#ifndef NQUEST
    //quest = new Quest(); //TODO Quest bugged in Android now here
#endif
    allowed_candidates.clear();
    disallowed_candidates.clear();

    //std::chrono::time_point<std::chrono::system_clock> start_time;
    maximum_time=0;
    temperature = 1000;
    coolingRate = 0.001;
    yfpool=0;
    efpool=0;

    factions.clear();
    invert_factions=false;
    only_recent=false;
    prefered_recent=false;
    recent_percent=5;
    skills.clear();
    invert_skills=false;
    prefered_skills.clear();
    prefered_factor=3;
}

#if defined(ANDROID) || defined(__ANDROID__)
extern "C" JNIEXPORT void

JNICALL
Java_de_neuwirthinformatik_Alexander_mTUO_TUOIntentService_callMain(
        JNIEnv *env,
        jobject obj/* this */,jobjectArray stringArray) {
	envv = env;
	objv = obj;
	//from: https://stackoverflow.com/questions/8870174/is-stdcout-usable-in-android-ndk and https://gist.github.com/dzhioev/6127982
    class androidbuf: public std::streambuf {
    public:
        enum { bufsize = 256 }; // ... or some other suitable buffer size
        androidbuf() { this->setp(buffer, buffer + bufsize -1); }
    private:
        int overflow(int c) {
            if (c == traits_type::eof()) {
                *this->pptr() = traits_type::to_char_type(c);
                this->sbumpc();
            }
            return this->sync()? traits_type::eof(): traits_type::not_eof(c);
        }
        int sync() {
            int rc = 0;
            if (this->pbase() != this->pptr()) {
                auto sss = std::string(this->pbase(),
                                       this->pptr() - this->pbase()).c_str();
                __android_log_print(ANDROID_LOG_DEBUG,
                                    "TUO_TUO",
                                    "%s",
                                    sss);
                jstring jstr = envv->NewStringUTF(sss);
                jclass clazz = envv->FindClass("de/neuwirthinformatik/Alexander/mTUO/TUOIntentService");
                jmethodID messageMe = envv->GetMethodID(clazz, "output", "(Ljava/lang/String;)V");
                envv->CallVoidMethod(objv, messageMe, jstr);
                rc = 0;
                this->setp(buffer, buffer + bufsize -1);
            }
            return rc;
        }
        char buffer[bufsize];
    };
    std::cout.rdbuf(new androidbuf);
    std::cerr.rdbuf(new androidbuf);
    __android_log_write(ANDROID_LOG_DEBUG, "TUO_TUO", "START");
    int stringCount = env->GetArrayLength(stringArray);
    char** param= new char*[stringCount];
    const char** cparam= new const char*[stringCount];
	jstring* strs = new jstring[stringCount];
    for (int i=0; i<stringCount; i++) {
        strs[i] = (jstring) (*env).GetObjectArrayElement( stringArray, i);
        cparam[i] = ((*env).GetStringUTFChars( strs[i], NULL));
		param[i] = const_cast<char*>(cparam[i]);
    }

    main(stringCount,param);
    std::cout << std::flush;
    __android_log_write(ANDROID_LOG_DEBUG, "TUO_TUO", "END");

	for (int i=0; i<stringCount; i++) {
		env->ReleaseStringUTFChars(strs[i], cparam[i]);
    }
    //std::string text = "return";
    //return env->NewStringUTF(text.c_str());

}

extern "C" JNIEXPORT jstring

JNICALL
Java_de_neuwirthinformatik_Alexander_mTUO_TUOIntentService_stringFromJNI( JNIEnv* env,
                                                  jobject thiz,jstring s )
{
    std::string str = env->GetStringUTFChars(s,NULL);
    str+="hello.txt";
    __android_log_write(ANDROID_LOG_DEBUG, "TUO_TUO", str.c_str());
    FILE* file = fopen( str.c_str(),"w+");

    if (file != NULL)
    {
        fputs("HELLO WORLD!\n", file);
        fflush(file);
        fclose(file);
    }

    return env->NewStringUTF( "Hello from JNI (with file io)!");
}
#endif
using namespace std::placeholders;
//------------------------------------------------------------------------------
std::string card_id_name(const Card* card)
{
    std::stringstream ios;
    if (card)
    {
        ios << "[" << card->m_id << "] " << card->m_name;
    }
    else
    {
        ios << "-void-";
    }
    return ios.str();
}
std::string card_slot_id_names(const std::vector<std::pair<signed, const Card *>> card_list)
{
    if (card_list.empty())
    {
        return "-void-";
    }
    std::stringstream ios;
    std::string separator = "";
    for (const auto & card_it : card_list)
    {
        ios << separator;
        separator = ", ";
        if (card_it.first >= 0)
        { ios << card_it.first << " "; }
        ios << "[" << card_it.second->m_id << "] " << card_it.second->m_name;
    }
    return ios.str();
}
//------------------------------------------------------------------------------
Deck* find_deck(Decks& decks, const Cards& all_cards, std::string deck_name)
{
    Deck* deck = decks.find_deck_by_name(deck_name);
    if (deck != nullptr)
    {
        deck->resolve();
        return(deck);
    }
    decks.decks.emplace_back(Deck{all_cards});
    deck = &decks.decks.back();
    deck->set(deck_name);
    deck->resolve();
    return(deck);
}
//---------------------- $80 deck optimization ---------------------------------
unsigned get_required_cards_before_upgrade(std::unordered_map<unsigned, unsigned>& owned_cards,
        const std::vector<const Card *>& card_list, std::map<const Card*, unsigned>& num_cards)
{
    unsigned deck_cost = 0;
    std::set<const Card*> unresolved_cards;
    for (const Card * card : card_list)
    {
        ++ num_cards[card];
        unresolved_cards.insert(card);
    }
    // un-upgrade according to type/category
    // * use fund for normal cards
    // * use only top-level cards for initial (basic) dominion (Alpha Dominion) and dominion material (Dominion Shard)
    while (!unresolved_cards.empty())
    {
        // pop next unresolved card
        auto card_it = unresolved_cards.end();
        auto card = *(--card_it);
        unresolved_cards.erase(card_it);

        // assume unlimited common/rare level-1 cards (standard set)
        if ((card->m_set == 1000) && (card->m_rarity <= 2) && (card->is_low_level_card()))
        { continue; }

        // keep un-defused (top-level) basic dominion & its material
        if ((card->m_id == 50002) || (card->m_category == CardCategory::dominion_material))
        { continue; }

        // defuse if inventory lacks required cards and recipe is not empty
        if ((fund || (card->m_category != CardCategory::normal))
                && (owned_cards[card->m_id] < num_cards[card]) && !card->m_recipe_cards.empty())
        {
            unsigned num_under = num_cards[card] - owned_cards[card->m_id];
            num_cards[card] = owned_cards[card->m_id];

            // do count cost (in SP) only for normal cards
            if (card->m_category == CardCategory::normal)
            {
                deck_cost += num_under * card->m_recipe_cost;
            }

            // enqueue recipe cards as unresolved
            for (auto recipe_it : card->m_recipe_cards)
            {
                num_cards[recipe_it.first] += num_under * recipe_it.second;
                unresolved_cards.insert(recipe_it.first);
            }
        }
    }
    return deck_cost;
}

inline unsigned get_required_cards_before_upgrade(const std::vector<const Card *>& card_list, std::map<const Card*, unsigned>& num_cards)
{
    return get_required_cards_before_upgrade(owned_cards, card_list, num_cards);
}

unsigned get_deck_cost(const Deck* deck)
{
    if (!use_owned_cards)
    { return 0; }
    std::map<const Card*, unsigned> num_in_deck;
    unsigned deck_cost = 0;
    if (deck->commander)
    { deck_cost += get_required_cards_before_upgrade({deck->commander}, num_in_deck); }
    deck_cost += get_required_cards_before_upgrade(deck->cards, num_in_deck);
    for (auto it: num_in_deck)
    {
        unsigned card_id = it.first->m_id;
        if (it.second > owned_cards[card_id])
        { return UINT_MAX; }
    }
    return deck_cost;
}

bool is_in_recipe(const Card* card, const Card* material)
{
    // is it already material?
    if (card == material)
    { return true; }

    // no recipes
    if (card->m_recipe_cards.empty())
    { return false; }

    // avoid illegal
    if (card->m_category == CardCategory::dominion_material)
    { return false; }

    // check recursively
    for (auto recipe_it : card->m_recipe_cards)
    {
        // is material found?
        if (recipe_it.first == material)
        { return true; }

        // go deeper ...
        if (is_in_recipe(recipe_it.first, material))
        { return true; }
    }

    // found nothing
    return false;
}

bool is_owned_or_can_be_fused(const Card* card)
{
    if (owned_cards[card->m_id])
    { return true; }
    if (!fund && (card->m_category == CardCategory::normal))
    { return false; }
    std::map<const Card*, unsigned> num_in_deck;
    unsigned deck_cost = get_required_cards_before_upgrade({card}, num_in_deck);
    if ((card->m_category == CardCategory::normal) && (deck_cost > fund))
    {
        while (!card->is_low_level_card() && (deck_cost > fund))
        {
            card = card->downgraded();
            num_in_deck.clear();
            deck_cost = get_required_cards_before_upgrade({card}, num_in_deck);
        }
        if (deck_cost > fund)
        { return false; }
    }
    std::map<unsigned, unsigned> num_under;
    for (auto it: num_in_deck)
    {
        if (it.second > owned_cards[it.first->m_id])
        {
            if ((card->m_category == CardCategory::dominion_alpha) && use_maxed_dominions
                    && !is_in_recipe(card, owned_alpha_dominion))
            {
                if (it.first->m_id != 50002)
                {
                    num_under[it.first->m_id] += it.second - owned_cards[it.first->m_id];
                }
                continue;
            }
            return false;
        }
    }
    if (!num_under.empty())
    {
        std::map<const Card*, unsigned>& refund = dominion_refund[owned_alpha_dominion->m_fusion_level][owned_alpha_dominion->m_level];
        for (auto & refund_it : refund)
        {
            unsigned refund_id = refund_it.first->m_id;
            if (!num_under.count(refund_id)) { continue; }
            num_under[refund_id] = safe_minus(num_under[refund_id], refund_it.second);
            if (!num_under[refund_id]) { num_under.erase(refund_id); }
        }
    }
    return num_under.empty();
}

std::string alpha_dominion_cost(const Card* dom_card)
{
    assert(dom_card->m_category == CardCategory::dominion_alpha);
    if (!owned_alpha_dominion) { return "(no owned alpha dominion)"; }
    std::unordered_map<unsigned, unsigned> _owned_cards;
    std::unordered_map<unsigned, unsigned> refund_owned_cards;
    std::map<const Card*, unsigned> num_cards;
    std::map<const Card*, unsigned>& refund = dominion_refund[owned_alpha_dominion->m_fusion_level][owned_alpha_dominion->m_level];
    unsigned own_dom_id = 50002;
    if (is_in_recipe(dom_card, owned_alpha_dominion))
    {
        own_dom_id = owned_alpha_dominion->m_id;
    }
    else if (owned_alpha_dominion->m_id != 50002)
    {
        for (auto& it : refund)
        {
            if (it.first->m_category != CardCategory::dominion_material)
            { continue; }
            refund_owned_cards[it.first->m_id] += it.second;
        }
    }
    _owned_cards[own_dom_id] = 1;
    get_required_cards_before_upgrade(_owned_cards, {dom_card}, num_cards);
    std::string value("");
    for (auto& it : num_cards)
    {
        if (it.first->m_category != CardCategory::dominion_material)
        { continue; }
        value += it.first->m_name + " x " + std::to_string(it.second) + ", ";
    }
    if (!is_in_recipe(dom_card, owned_alpha_dominion))
    {
        num_cards.clear();
        get_required_cards_before_upgrade(_owned_cards, {owned_alpha_dominion}, num_cards);
        value += "using refund: ";
        for (auto& it : refund)
        {
            signed num_under(it.second - (signed)num_cards[it.first]);
            value += it.first->m_name + " x " + std::to_string(it.second) + "/" + std::to_string(num_under) + ", ";
        }
    }
    // remove trailing ', ' for non-empty string / replace empty by '(none)'
    if (!value.empty()) { value.erase(value.end() - 2, value.end()); }
    else { value += "(none)"; }
    return value;
}

// remove val from oppo if found, otherwise append val to self
    template <typename C>
void append_unless_remove(C & self, C & oppo, typename C::const_reference val)
{
    for (auto it = oppo.begin(); it != oppo.end(); ++ it)
    {
        if (*it == val)
        {
            oppo.erase(it);
            return;
        }
    }
    self.push_back(val);
}

// insert card at to_slot into deck limited by fund; store deck_cost
// return true if affordable
bool adjust_deck(Deck * deck, const signed from_slot, const signed to_slot, const Card * card, unsigned fund, std::mt19937 & re, unsigned & deck_cost,
        std::vector<std::pair<signed, const Card *>> & cards_out, std::vector<std::pair<signed, const Card *>> & cards_in)
{
    bool is_random = (deck->strategy == DeckStrategy::random) || (deck->strategy == DeckStrategy::flexible);
    cards_out.clear();
    cards_in.clear();
    if (from_slot < 0)
    {
        if (card->m_category == CardCategory::dominion_alpha)
        { // change alpha dominion
            cards_out.emplace_back(-1, deck->alpha_dominion);
            deck->alpha_dominion = card;
            cards_in.emplace_back(-1, deck->alpha_dominion);
            deck_cost = get_deck_cost(deck);
            return true;
        }

        // change commander
        cards_out.emplace_back(-1, deck->commander);
        deck->commander = card;
        cards_in.emplace_back(-1, deck->commander);
        deck_cost = get_deck_cost(deck);
        return (deck_cost <= fund);
    }
    if (from_slot < (signed)deck->cards.size())
    { // remove card from the deck
        cards_out.emplace_back(is_random ? -1 : from_slot, deck->cards[from_slot]);
        deck->cards.erase(deck->cards.begin() + from_slot);
    }
    if (card == nullptr)
    { // remove card (no new replacement for removed card)
        deck_cost = get_deck_cost(deck);
        return (deck_cost <= fund);
    }

    // backup deck cards
    const Card * old_commander = deck->commander;
    std::vector<const Card *> cards = deck->cards;

    // try to add new card into the deck, downgrade it if necessary
    {
        const Card * candidate_card = card;
        deck->commander = nullptr;
        deck->cards.clear();
        deck->cards.emplace_back(card);
        deck_cost = get_deck_cost(deck);
        if (!use_top_level_card && (deck_cost > fund))
        {
            while ((deck_cost > fund) && !candidate_card->is_low_level_card())
            {
                candidate_card = candidate_card->downgraded();
                deck->cards[0] = candidate_card;
                deck_cost = get_deck_cost(deck);
            }
        }
        if (deck_cost > fund)
        { return false; }
        cards_in.emplace_back(is_random ? -1 : to_slot, deck->cards[0]);
    }

    // try to add commander into the deck, downgrade it if necessary
    {
        const Card * candidate_card = old_commander;
        deck->commander = candidate_card;
        deck_cost = get_deck_cost(deck);
        if (!use_top_level_commander && (deck_cost > fund))
        {
            while ((deck_cost > fund) && !candidate_card->is_low_level_card())
            {
                candidate_card = candidate_card->downgraded();
                deck->commander = candidate_card;
                deck_cost = get_deck_cost(deck);
            }
        }
        if (deck_cost > fund)
        { return false; }
        if (deck->commander != old_commander)
        {
            append_unless_remove(cards_out, cards_in, {-1, old_commander});
            append_unless_remove(cards_in, cards_out, {-1, deck->commander});
        }
    }

    // added backuped deck cards back (place cards strictly before/after card inserted above according to slot index)
    for (signed i = 0; i < (signed)cards.size(); ++ i)
    {
        // try to add cards[i] into the deck, downgrade it if necessary
        const Card * candidate_card = cards[i];
        auto in_it = deck->cards.end() - (i < to_slot); // (before/after according to slot index)
        in_it = deck->cards.insert(in_it, candidate_card);
        deck_cost = get_deck_cost(deck);
        if (!use_top_level_card && (deck_cost > fund))
        {
            while ((deck_cost > fund) && !candidate_card->is_low_level_card())
            {
                candidate_card = candidate_card->downgraded();
                *in_it = candidate_card;
                deck_cost = get_deck_cost(deck);
            }
        }
        if (deck_cost > fund)
        { return false; }
        if (*in_it != cards[i])
        {
            append_unless_remove(cards_out, cards_in, {is_random ? -1 : i + (i >= from_slot), cards[i]});
            append_unless_remove(cards_in, cards_out, {is_random ? -1 : i + (i >= to_slot), *in_it});
        }
    }
    return !cards_in.empty() || !cards_out.empty();
}

unsigned check_requirement(const Deck* deck, const Requirement & requirement
#ifndef NQUEST
        , const Quest & quest
#endif
        )
{
    unsigned gap = safe_minus(min_deck_len, deck->cards.size());
    if (!requirement.num_cards.empty())
    {
        std::unordered_map<const Card*, unsigned> num_cards;
        num_cards[deck->commander] = 1;
        for (auto card: deck->cards)
        {
            ++ num_cards[card];
        }
        for (auto it: requirement.num_cards)
        {
            gap += safe_minus(it.second, num_cards[it.first]);
        }
    }
#ifndef NQUEST
    if (quest.quest_type != QuestType::none)
    {
        unsigned potential_value = 0;
        switch (quest.quest_type)
        {
            case QuestType::skill_use:
            case QuestType::skill_damage:
                for (const auto & ss: deck->commander->m_skills)
                {
                    if (quest.quest_key == ss.id)
                    {
                        potential_value = quest.quest_value;
                        break;
                    }
                }
                break;
            case QuestType::faction_assault_card_kill:
            case QuestType::type_card_kill:
                potential_value = quest.quest_value;
                break;
            default:
                break;
        }
        for (auto card: deck->cards)
        {
            switch (quest.quest_type)
            {
                case QuestType::skill_use:
                case QuestType::skill_damage:
                    for (const auto & ss: card->m_skills)
                    {
                        if (quest.quest_key == ss.id)
                        {
                            potential_value = quest.quest_value;
                            break;
                        }
                    }
                    break;
                case QuestType::faction_assault_card_use:
                    potential_value += (quest.quest_key == card->m_faction);
                    break;
                case QuestType::type_card_use:
                    potential_value += (quest.quest_key == card->m_type);
                    break;
                default:
                    break;
            }
            if (potential_value >= (quest.must_fulfill ? quest.quest_value : 1))
            {
                break;
            }
        }
        gap += safe_minus(quest.must_fulfill ? quest.quest_value : 1, potential_value);
    }
#endif
    return gap;
}

void claim_cards(const std::vector<const Card*> & card_list)
{
    std::map<const Card *, unsigned> num_cards;
    get_required_cards_before_upgrade(card_list, num_cards);
    for (const auto & it: num_cards)
    {
        const Card * card = it.first;
        if(card->m_category == CardCategory::dominion_material)continue;
        if(card->m_category == CardCategory::dominion_alpha)continue;
        unsigned num_to_claim = safe_minus(it.second, owned_cards[card->m_id]);
        if (num_to_claim > 0)
        {
            owned_cards[card->m_id] += num_to_claim;
            if (debug_print >= 0)
            {
                std::cerr << "WARNING: Need extra " << num_to_claim << " " << card->m_name << " to build your initial deck: adding to owned card list.\n";
            }
        }
    }
}

//------------------------------------------------------------------------------
FinalResults<long double> compute_score(const EvaluatedResults& results, std::vector<long double>& factors)
{
    FinalResults<long double> final{0, 0, 0, 0, 0, 0, results.second};
    long double max_possible = max_possible_score[(size_t)optimization_mode];
    for (unsigned index(0); index < results.first.size(); ++index)
    {
        final.wins += results.first[index].wins * factors[index];
        final.draws += results.first[index].draws * factors[index];
        final.losses += results.first[index].losses * factors[index];
        //APN
        auto trials = results.second;
        auto prob = 1-confidence_level;
        auto successes = results.first[index].points/max_possible;
        if(successes > trials)
        {
            successes = trials;
            //printf("WARNING: biominal successes > trials");
            _DEBUG_MSG(2,"WARNING: biominal successes > trials");
        }

        auto lower_bound = boost::math::binomial_distribution<>::find_lower_bound_on_p(trials, successes, prob) * max_possible;
        auto upper_bound = boost::math::binomial_distribution<>::find_upper_bound_on_p(trials, successes, prob) * max_possible;
        if (use_harmonic_mean)
        {
            final.points += factors[index] / results.first[index].points;
            final.points_lower_bound += factors[index] / lower_bound;
            final.points_upper_bound += factors[index] / upper_bound;
        }
        else
        {
            final.points += results.first[index].points * factors[index];
            final.points_lower_bound += lower_bound * factors[index];
            final.points_upper_bound += upper_bound * factors[index];
        }
    }
    long double factor_sum = std::accumulate(factors.begin(), factors.end(), 0.);
    final.wins /= factor_sum * (long double)results.second;
    final.draws /= factor_sum * (long double)results.second;
    final.losses /= factor_sum * (long double)results.second;
    if (use_harmonic_mean)
    {
        final.points = factor_sum / ((long double)results.second * final.points);
        final.points_lower_bound = factor_sum / final.points_lower_bound;
        final.points_upper_bound = factor_sum / final.points_upper_bound;
    }
    else
    {
        final.points /= factor_sum * (long double)results.second;
        final.points_lower_bound /= factor_sum;
        final.points_upper_bound /= factor_sum;
    }
    return final;
}
//------------------------------------------------------------------------------
volatile unsigned thread_num_iterations{0}; // written by threads
EvaluatedResults *thread_results{nullptr}; // written by threads
volatile const FinalResults<long double> *thread_best_results{nullptr};
volatile bool thread_compare{false};
volatile bool thread_compare_stop{false}; // written by threads
volatile bool destroy_threads;
//------------------------------------------------------------------------------
// Per thread data.
// seed should be unique for each thread.
// d1 and d2 are intended to point to read-only process-wide data.
struct SimulationData
{
    std::mt19937 re;
    const Cards& cards;
    const Decks& decks;
    std::vector<std::shared_ptr<Deck>> your_decks;
    std::vector<Hand*> your_hands;
    std::vector<std::shared_ptr<Deck>> enemy_decks;
    std::vector<Hand*> enemy_hands;
    std::vector<long double> factors;
    gamemode_t gamemode;
#ifndef NQUEST
    Quest quest;
#endif
    std::array<signed short, PassiveBGE::num_passive_bges> your_bg_effects, enemy_bg_effects;
    std::vector<SkillSpec> your_bg_skills, enemy_bg_skills;

    SimulationData(unsigned seed, const Cards& cards_, const Decks& decks_, unsigned num_your_decks_,unsigned num_enemy_decks_, std::vector<long double> factors_, gamemode_t gamemode_,
#ifndef NQUEST
            Quest & quest_,
#endif
            std::array<signed short, PassiveBGE::num_passive_bges>& your_bg_effects_,
            std::array<signed short, PassiveBGE::num_passive_bges>& enemy_bg_effects_,
            std::vector<SkillSpec>& your_bg_skills_,
            std::vector<SkillSpec>& enemy_bg_skills_) :
        re(seed),
        cards(cards_),
        decks(decks_),
        your_decks(num_your_decks_),
        enemy_decks(num_enemy_decks_),
        factors(factors_),
        gamemode(gamemode_),
#ifndef NQUEST
        quest(quest_),
#endif
        your_bg_effects(your_bg_effects_),
        enemy_bg_effects(enemy_bg_effects_),
        your_bg_skills(your_bg_skills_),
        enemy_bg_skills(enemy_bg_skills_)
        {
            for (size_t i = 0; i < num_your_decks_; ++i)
            {
                your_hands.emplace_back(new Hand(nullptr));
            }
            for (size_t i = 0; i < num_enemy_decks_; ++i)
            {
                enemy_hands.emplace_back(new Hand(nullptr));
            }
        }

    ~SimulationData()
    {
        for (auto hand: enemy_hands) { delete(hand); }
        for (auto hand: your_hands) { delete(hand); }
    }

    void set_decks(std::vector<Deck*> const your_decks_, std::vector<Deck*> const & enemy_decks_)
    {
        for (unsigned i(0); i < your_decks_.size(); ++i)
        {
            your_decks[i].reset(your_decks_[i]->clone());
            your_hands[i]->deck = your_decks[i].get();
        }
        for (unsigned i(0); i < enemy_decks_.size(); ++i)
        {
            enemy_decks[i].reset(enemy_decks_[i]->clone());
            enemy_hands[i]->deck = enemy_decks[i].get();
        }
    }

    inline std::vector<Results<uint64_t>> evaluate()
    {
        std::vector<Results<uint64_t>> res;
        res.reserve(enemy_hands.size()*your_hands.size());
        for(Hand* your_hand : your_hands)
        {
            for (Hand* enemy_hand: enemy_hands)
            {
                your_hand->reset(re);
                enemy_hand->reset(re);
                Field fd(re, cards, *your_hand, *enemy_hand, gamemode, optimization_mode,
    #ifndef NQUEST
                        quest,
#endif
                        your_bg_effects, enemy_bg_effects, your_bg_skills, enemy_bg_skills, flexible_iter,flexible_turn);
                Results<uint64_t> result(play(&fd));
                if (__builtin_expect(mode_open_the_deck, false))
                {
                    // are there remaining (unopened) cards?
                    if (fd.players[1]->deck->shuffled_cards.size())
                    {
                        // apply min score (there are unopened cards, so mission failed)
                        result.points = min_possible_score[(size_t)optimization_mode];
                    }
                }
                res.emplace_back(result);
            }
        }
        //std::cout << std::endl<<  "Deck hash: " << your_hand.deck->hash() << "#"<< std::endl;
        return(res);
    }
};
//------------------------------------------------------------------------------
class Process;
void thread_evaluate(boost::barrier& main_barrier,
        boost::mutex& shared_mutex,
        SimulationData& sim,
        const Process& p,
        unsigned thread_id);
//------------------------------------------------------------------------------
class Process
{
    public:
        unsigned num_threads;
        std::vector<boost::thread*> threads;
        std::vector<SimulationData*> threads_data;
        boost::barrier main_barrier;
        boost::mutex shared_mutex;
        const Cards& cards;
        const Decks& decks;
        const std::vector<Deck*> your_decks;
        const std::vector<Deck*> enemy_decks;
        std::vector<long double> factors;
        gamemode_t gamemode;
#ifndef NQUEST
        Quest quest;
#endif
        std::array<signed short, PassiveBGE::num_passive_bges> your_bg_effects, enemy_bg_effects;
        std::vector<SkillSpec> your_bg_skills, enemy_bg_skills;

        Process(unsigned num_threads_, const Cards& cards_, const Decks& decks_, std::vector<Deck*> your_decks_, std::vector<Deck*> enemy_decks_, std::vector<long double> factors_, gamemode_t gamemode_,
#ifndef NQUEST
                Quest & quest_,
#endif
                std::array<signed short, PassiveBGE::num_passive_bges>& your_bg_effects_,
                std::array<signed short, PassiveBGE::num_passive_bges>& enemy_bg_effects_,
                std::vector<SkillSpec>& your_bg_skills_, std::vector<SkillSpec>& enemy_bg_skills_) :
            num_threads(num_threads_),
            main_barrier(num_threads+1),
            cards(cards_),
            decks(decks_),
            your_decks(your_decks_),
            enemy_decks(enemy_decks_),
            factors(factors_),
            gamemode(gamemode_),
#ifndef NQUEST
            quest(quest_),
#endif
            your_bg_effects(your_bg_effects_),
            enemy_bg_effects(enemy_bg_effects_),
            your_bg_skills(your_bg_skills_),
            enemy_bg_skills(enemy_bg_skills_)
            {
                destroy_threads = false;
                unsigned seed(sim_seed ? sim_seed : std::chrono::system_clock::now().time_since_epoch().count() * 2654435761);  // Knuth multiplicative hash
                if (num_threads_ == 1)
                {
                    std::cout << "RNG seed " << seed << std::endl;
                }
                for (unsigned i(0); i < num_threads; ++i)
                {
                    threads_data.push_back(new SimulationData(seed + i, cards, decks,your_decks.size(), enemy_decks.size(), factors, gamemode,
#ifndef NQUEST
                                quest,
#endif
                                your_bg_effects, enemy_bg_effects, your_bg_skills, enemy_bg_skills));
                    threads.push_back(new boost::thread(thread_evaluate, std::ref(main_barrier), std::ref(shared_mutex), std::ref(*threads_data.back()), std::ref(*this), i));
                }
            }

        ~Process()
        {
            destroy_threads = true;
            main_barrier.wait();
            for (auto thread: threads) { thread->join(); }
            for (auto data: threads_data) { delete(data); }
        }

        EvaluatedResults & evaluate(unsigned num_iterations, EvaluatedResults & evaluated_results)
        {
            if (num_iterations <= evaluated_results.second)
            {
                return evaluated_results;
            }
            thread_num_iterations = num_iterations - evaluated_results.second;
            thread_results = &evaluated_results;
            thread_compare = false;
            // unlock all the threads
            main_barrier.wait();
            // wait for the threads
            main_barrier.wait();
            return evaluated_results;
        }

        EvaluatedResults & compare(unsigned num_iterations, EvaluatedResults & evaluated_results, const FinalResults<long double> & best_results)
        {
            if (num_iterations <= evaluated_results.second)
            {
                return evaluated_results;
            }
            thread_num_iterations = num_iterations - evaluated_results.second;
            thread_results = &evaluated_results;
            thread_best_results = &best_results;
            thread_compare = true;
            thread_compare_stop = false;
            // unlock all the threads
            main_barrier.wait();
            // wait for the threads
            main_barrier.wait();
            return evaluated_results;
        }
};
//------------------------------------------------------------------------------
void thread_evaluate(boost::barrier& main_barrier,
        boost::mutex& shared_mutex,
        SimulationData& sim,
        const Process& p,
        unsigned thread_id)
{
    while (true)
    {
        main_barrier.wait();
        sim.set_decks(p.your_decks, p.enemy_decks);
        if (destroy_threads)
        { return; }
        while (true)
        {
            shared_mutex.lock(); //<<<<
            if (thread_num_iterations == 0 || (thread_compare && thread_compare_stop)) //!
            {
                shared_mutex.unlock(); //>>>>
                main_barrier.wait();
                break;
            }
            else
            {
                --thread_num_iterations; //!
                shared_mutex.unlock(); //>>>>
                std::vector<Results<uint64_t>> result{sim.evaluate()};
                shared_mutex.lock(); //<<<<
                std::vector<uint64_t> thread_score_local(thread_results->first.size(), 0u); //!
                for (unsigned index(0); index < result.size(); ++index)
                {
                    thread_results->first[index] += result[index]; //!
                    thread_score_local[index] = thread_results->first[index].points; //!
                }
                ++thread_results->second; //!
                unsigned thread_total_local{thread_results->second}; //!
                shared_mutex.unlock(); //>>>>
                if (thread_compare && thread_id == 0 && thread_total_local > 1)
                {
                    unsigned score_accum = 0;
                    // Multiple defense decks case: scaling by factors and approximation of a "discrete" number of events.
                    if (result.size() > 1)
                    {
                        long double score_accum_d = 0.0;
                        for (unsigned i = 0; i < thread_score_local.size(); ++i)
                        {
                            score_accum_d += thread_score_local[i] * sim.factors[i];
                        }
                        score_accum_d /= std::accumulate(sim.factors.begin(), sim.factors.end(), .0);
                        score_accum = score_accum_d;
                    }
                    else
                    {
                        score_accum = thread_score_local[0];
                    }
                    bool compare_stop(false);
                    long double max_possible = max_possible_score[(size_t)optimization_mode];

                    //APN
                    auto trials = thread_total_local;
                    auto prob = 1-confidence_level;
                    auto successes = score_accum / max_possible;
                    if(successes > trials)
                    {
                        successes = trials;
                        printf("WARNING: biominal successes > trials in Threads");
                        _DEBUG_MSG(2,"WARNING: biominal successes > trials in Threads");
                    }


                    // Get a loose (better than no) upper bound. TODO: Improve it.
                    compare_stop = (boost::math::binomial_distribution<>::find_upper_bound_on_p(trials, successes, prob) * max_possible <
                            thread_best_results->points + min_increment_of_score);
                    if (compare_stop)
                    {
                        shared_mutex.lock(); //<<<<
                        //std::cout << thread_total_local << "\n";
                        thread_compare_stop = true; //!
                        shared_mutex.unlock(); //>>>>
                    }
                }
            }
        }
    }
}
//------------------------------------------------------------------------------
void print_score_info(const EvaluatedResults& results, std::vector<long double>& factors)
{
    auto final = compute_score(results, factors);
    std::cout << final.points << " (";
    if(!simplify_output)
    {
        if (show_ci)
        {
            std::cout << final.points_lower_bound << "-" << final.points_upper_bound << ", ";
        }
        for (const auto & val: results.first)
        {
            switch(optimization_mode)
            {
                case OptimizationMode::raid:
                case OptimizationMode::campaign:
                case OptimizationMode::brawl:
                case OptimizationMode::brawl_defense:
                case OptimizationMode::war:
                case OptimizationMode::war_defense:
    #ifndef NQUEST
                case OptimizationMode::quest:
    #endif
                    std::cout << val.points << " ";
                    break;
                default:
                    std::cout << val.points / 100 << " ";
                    break;
            }
        }
    }
    else {
      std::cout << "...";
    }
    std::cout << "/ " << results.second << ")" << std::endl;
}
//------------------------------------------------------------------------------
void print_results(const EvaluatedResults& results, std::vector<long double>& factors)
{
    auto final = compute_score(results, factors);
    std::cout << "win%: " << final.wins * 100.0 << " (";
    if(!simplify_output)
    {
        for (const auto & val : results.first)
        {
            std::cout << val.wins << " ";
        }
    }
    else {std::cout << "...";}
    std::cout << "/ " << results.second << ")" << std::endl;

    std::cout << "stall%: " << final.draws * 100.0 << " (";
    if(!simplify_output)
    {
        for (const auto & val : results.first)
        {
            std::cout << val.draws << " ";
        }
    }
    else {std::cout << "...";}
    std::cout << "/ " << results.second << ")" << std::endl;

    std::cout << "loss%: " << final.losses * 100.0 << " (";
    if(!simplify_output)
    {
        for (const auto & val : results.first)
        {
            std::cout << val.losses << " ";
        }
    }
    else {std::cout << "...";}
    std::cout << "/ " << results.second << ")" << std::endl;

#ifndef NQUEST
    if (optimization_mode == OptimizationMode::quest)
    {
        // points = win% * win_score + (must_win ? win% : 100%) * quest% * quest_score
        // quest% = (points - win% * win_score) / (must_win ? win% : 100%) / quest_score
        std::cout << "quest%: " << (final.points - final.wins * quest.win_score) / (quest.must_win ? final.wins : 1) / quest.quest_score * 100 << std::endl;
    }
#endif

    unsigned min_score = min_possible_score[(size_t)optimization_mode];
    unsigned max_score = max_possible_score[(size_t)optimization_mode];
    switch(optimization_mode)
    {
        case OptimizationMode::raid:
        case OptimizationMode::campaign:
        case OptimizationMode::brawl:
        case OptimizationMode::brawl_defense:
        case OptimizationMode::war:
        case OptimizationMode::war_defense:
#ifndef NQUEST
        case OptimizationMode::quest:
#endif
            std::cout << "score: " << final.points;
            if (optimization_mode == OptimizationMode::brawl)
            {
                auto win_points = final.wins ? ((final.points - min_score * (1.0 - final.wins)) / final.wins) : final.points;
                std::cout << " [" << win_points << " per win]";
            }
            else if (optimization_mode == OptimizationMode::brawl_defense)
            {
                auto opp_win_points = final.losses ? max_score - ((final.points - (max_score - min_score) * (1.0 - final.losses)) / final.losses) : final.points;
                std::cout << " [" << opp_win_points << " per opp win]";
            }
            else if (optimization_mode == OptimizationMode::war)
            {
                auto win_points = final.wins ? ((final.points - min_score * (1.0 - final.wins)) / final.wins) : final.points;
                std::cout << " [" << win_points << " per win]";
            }
            else if (optimization_mode == OptimizationMode::war_defense)
            {
                auto opp_win_points = final.losses ? max_score - ((final.points - (max_score - min_score) * (1.0 - final.losses)) / final.losses) : final.points;
                std::cout << " [" << opp_win_points << " per opp win]";
            }
            std::cout << " (";
            if(!simplify_output)
            {
                for (const auto & val: results.first)
                {
                    std::cout << val.points << " ";
                }
            }
            else {std::cout << "...";}
            std::cout << "/ " << results.second << ")" << std::endl;
            if (show_ci)
            {
                std::cout << "ci: " << final.points_lower_bound << " - " << final.points_upper_bound << std::endl;
            }
            break;
        default:
            break;
    }
}

//------------------------------------------------------------------------------
// Prints which cards have been upgraded
void print_upgraded_cards(Deck* deck)
{
    if(!print_upgraded)return;
    auto owned_cards_c = owned_cards;
    std::cout << "Upgraded Cards: ";
    auto cards = deck->cards;
    cards.insert(cards.begin(), deck->commander);
    for (const Card * card : cards)
    {

        if ((card->m_set == 1000) && (card->m_rarity <= 2) && (card->is_low_level_card()))
        { continue; }

        if ((card->m_id == 50002) || (card->m_category == CardCategory::dominion_material))
        { continue; }

        if ((fund || (card->m_category != CardCategory::normal))
                && (owned_cards_c[card->m_id] == 0))
        {
            //print card
            std::cout << card->m_name << ", ";
        }
        if(owned_cards_c[card->m_id] >0)owned_cards_c[card->m_id]--;
    }
    std::cout << std::endl;
}
//------------------------------------------------------------------------------
void print_cards_inline(std::vector<const Card*> cards)
{
    std::string last_name;
    unsigned num_repeat(0);
    bool first = true;
    for (const Card* card: cards)
    {
        if (card->m_name == last_name)
        {
            ++ num_repeat;
        }
        else
        {
            if (num_repeat > 1)
            {
                std::cout << " #" << num_repeat;
            }
            std::cout << (first?"":", ") << card->m_name;
            first = false;
            last_name = card->m_name;
            num_repeat = 1;
        }
    }
    if (num_repeat > 1)
    {
        std::cout << " #" << num_repeat;
    }
    std::cout << std::endl;
}
void print_score_inline(const FinalResults<long double> score) {
    // print optimization result details
    switch(optimization_mode)
    {
        case OptimizationMode::raid:
        case OptimizationMode::campaign:
        case OptimizationMode::brawl:
        case OptimizationMode::brawl_defense:
        case OptimizationMode::war:
        case OptimizationMode::war_defense:
#ifndef NQUEST
        case OptimizationMode::quest:
#endif
            std::cout << "(" << score.wins * 100 << "% win";
#ifndef NQUEST
            if (optimization_mode == OptimizationMode::quest)
            {
                std::cout << ", " << (score.points - score.wins * quest.win_score) / (quest.must_win ? score.wins : 1) / quest.quest_score * 100 << "% quest";
            }
#endif
            if (show_ci)
            {
                std::cout << ", " << score.points_lower_bound << "-" << score.points_upper_bound;
            }
            std::cout << ") ";
            break;
        case OptimizationMode::defense:
            std::cout << "(" << score.draws * 100.0 << "% stall) ";
            break;
        default:
            break;
    }
    std::cout << score.points;
    unsigned min_score = min_possible_score[(size_t)optimization_mode];
    unsigned max_score = max_possible_score[(size_t)optimization_mode];
    if (optimization_mode == OptimizationMode::brawl)
    {
        auto win_points = score.wins ? ((score.points - min_score * (1.0 - score.wins)) / score.wins) : score.points;
        std::cout << " [" << win_points << " per win]";
    }
    else if (optimization_mode == OptimizationMode::brawl_defense)
    {
        auto opp_win_points = score.losses ? max_score - ((score.points - (max_score - min_score) * (1.0 - score.losses)) / score.losses) : score.points;
        std::cout << " [" << opp_win_points << " per opp win]";
    }
    else if (optimization_mode == OptimizationMode::war)
    {
        auto win_points = score.wins ? ((score.points - min_score * (1.0 - score.wins)) / score.wins) : score.points;
        std::cout << " [" << win_points << " per win]";
    }
    else if (optimization_mode == OptimizationMode::war_defense)
    {
        auto opp_win_points = score.losses ? max_score - ((score.points - (max_score - min_score) * (1.0 - score.losses)) / score.losses) : score.points;
        std::cout << " [" << opp_win_points << " per opp win]";
    }
}
//------------------------------------------------------------------------------
// Calculates and prints individual card value in this deck
void print_sim_card_values(Deck* original_deck, Process& p, unsigned iter) // run_deck == p.your_decks[0]
{
    if(!print_values)return;
    if(p.your_decks.size() != 1)return; // only for single deck
    std::string last_name;
    long  double score;
    Deck* sim_deck = p.your_decks[0];
    Deck* your_deck = original_deck->clone(); //save the original deck
    auto cards = your_deck->cards;
    //sim original/base result
    sim_deck->commander = your_deck->commander;
    sim_deck->cards = your_deck->cards;
    sim_deck->alpha_dominion = your_deck->alpha_dominion;
    EvaluatedResults results = { EvaluatedResults::first_type(p.enemy_decks.size()*p.your_decks.size()), 0 };
    results = p.evaluate(iter, results);
    const FinalResults<long  double> fr_base= compute_score(results,p.factors);
    long double base = fr_base.points;
    std::cout << "Value of Cards: ";
    for (unsigned i =0; i < cards.size();++i)
    {
        auto card = cards[i];
        if (card->m_name == last_name)
        {
        }
        else
        {
            last_name = card->m_name;
            //sim it
            sim_deck->cards = your_deck->cards; //reset cards
            sim_deck->cards.erase(sim_deck->cards.begin() + i); //remove to test card
            EvaluatedResults results = { EvaluatedResults::first_type(p.enemy_decks.size()*p.your_decks.size()), 0 };
            results = p.evaluate(iter, results);
            const FinalResults<long  double> fr= compute_score(results,p.factors);
            score = base - fr.points; //subtract from result to get value
            std::cout << card->m_name << " (" << score << "), ";
        }
    }
    std::cout << std::endl;
}
//------------------------------------------------------------------------------
void print_deck_inline(const unsigned deck_cost, const FinalResults<long double> score, Deck * deck)
{
    // print units count
    std::cout << deck->cards.size() << " units: ";

    // print deck cost (if fund is enabled)
    if (fund > 0)
    {
        std::cout << "$" << deck_cost << " ";
    }
    print_score_inline(score);
    /*
    // print optimization result details
    switch(optimization_mode)
    {
        case OptimizationMode::raid:
        case OptimizationMode::campaign:
        case OptimizationMode::brawl:
        case OptimizationMode::brawl_defense:
        case OptimizationMode::war:
        case OptimizationMode::war_defense:
#ifndef NQUEST
        case OptimizationMode::quest:
#endif
            std::cout << "(" << score.wins * 100 << "% win";
#ifndef NQUEST
            if (optimization_mode == OptimizationMode::quest)
            {
                std::cout << ", " << (score.points - score.wins * quest.win_score) / (quest.must_win ? score.wins : 1) / quest.quest_score * 100 << "% quest";
            }
#endif
            if (show_ci)
            {
                std::cout << ", " << score.points_lower_bound << "-" << score.points_upper_bound;
            }
            std::cout << ") ";
            break;
        case OptimizationMode::defense:
            std::cout << "(" << score.draws * 100.0 << "% stall) ";
            break;
        default:
            break;
    }
    std::cout << score.points;
    unsigned min_score = min_possible_score[(size_t)optimization_mode];
    unsigned max_score = max_possible_score[(size_t)optimization_mode];
    if (optimization_mode == OptimizationMode::brawl)
    {
        auto win_points = score.wins ? ((score.points - min_score * (1.0 - score.wins)) / score.wins) : score.points;
        std::cout << " [" << win_points << " per win]";
    }
    else if (optimization_mode == OptimizationMode::brawl_defense)
    {
        auto opp_win_points = score.losses ? max_score - ((score.points - (max_score - min_score) * (1.0 - score.losses)) / score.losses) : score.points;
        std::cout << " [" << opp_win_points << " per opp win]";
    }
    else if (optimization_mode == OptimizationMode::war)
    {
        auto win_points = score.wins ? ((score.points - min_score * (1.0 - score.wins)) / score.wins) : score.points;
        std::cout << " [" << win_points << " per win]";
    }
    else if (optimization_mode == OptimizationMode::war_defense)
    {
        auto opp_win_points = score.losses ? max_score - ((score.points - (max_score - min_score) * (1.0 - score.losses)) / score.losses) : score.points;
        std::cout << " [" << opp_win_points << " per opp win]";
    }
    */

    // print commander
    std::cout << ": " << deck->commander->m_name;

    // print dominions
    if (deck->alpha_dominion)
    { std::cout << ", " << deck->alpha_dominion->m_name; }

    // print deck cards
    if (deck->strategy == DeckStrategy::random || deck->strategy == DeckStrategy::flexible)
    {
        std::sort(deck->cards.begin(), deck->cards.end(), [](const Card* a, const Card* b) { return a->m_id < b->m_id; });
    }
    std::cout << ", ";
    print_cards_inline(deck->cards);
}
//------------------------------------------------------------------------------
inline bool is_timeout_reached()
{
    if(__builtin_expect(maximum_time > 0,false))
    {
        if((std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now()-start_time)).count() > maximum_time*60*60)
        {
            std::cout << "Time expired! Current result:" << std::endl;
            return true;
        }
    }
    return false;
}
inline std::vector<std::vector<const Card*>> get_candidate_lists(Process& proc)
{
    std::vector<std::vector<const Card*>> ret_candidates;
    std::vector<const Card*> commander_candidates;
    std::vector<const Card*> alpha_dominion_candidates;
    std::vector<const Card*> card_candidates;

    // resolve available to player cards
    auto player_assaults_and_structures = proc.cards.player_commanders;
    player_assaults_and_structures.insert(player_assaults_and_structures.end(), proc.cards.player_structures.begin(), proc.cards.player_structures.end());
    player_assaults_and_structures.insert(player_assaults_and_structures.end(), proc.cards.player_assaults.begin(), proc.cards.player_assaults.end());
    for (auto it = player_assaults_and_structures.begin(); it != player_assaults_and_structures.end();++it)
    {
        const Card* card = *it;
        // skip illegal
        if ((card->m_category != CardCategory::dominion_alpha)
                && (card->m_category != CardCategory::normal))
        { continue; }

        // skip dominions when their climbing is disabled
        if ((card->m_category == CardCategory::dominion_alpha) && (!use_owned_dominions))
        { continue; }

        // try to skip a card unless it's allowed
        if (!allowed_candidates.count(card->m_id))
        {
            // skip disallowed always
            if (disallowed_candidates.count(card->m_id))
            { continue; }

            // handle dominions
            if (card->m_category == CardCategory::dominion_alpha)
            {
                // skip non-top-level dominions anyway
                // (will check it later and downgrade if necessary according to amount of material (shards))
                /*if (!card->is_top_level_card())
                  { continue; }

                // skip basic dominions
                if ((card->m_id == 50001) || (card->m_id == 50002))
                { continue; }*/
            }

            // handle normal cards
            else
            {
                // skip non-top-level cards (adjust_deck() will try to downgrade them if necessary)
                bool use_top_level = (card->m_type == CardType::commander) ? use_top_level_commander : use_top_level_card;
                if (!card->is_top_level_card() and (fund || use_top_level || !owned_cards[card->m_id]))
                { continue; }

                // skip lowest fusion levels
                unsigned use_fused_level = (card->m_type == CardType::commander) ? use_fused_commander_level : use_fused_card_level;
                if (card->m_fusion_level < use_fused_level)
                { continue; }
            }
        }


        if(use_owned_cards && card->m_category == CardCategory::dominion_alpha && !owned_cards[card->m_id])
        {
            if(use_maxed_dominions && card->m_used_for_cards.size()==0)
            {

            }
            else
            {continue;}
        }
        // skip unavailable cards anyway when ownedcards is used
        if (use_owned_cards && !(card->m_category == CardCategory::dominion_alpha) && !is_owned_or_can_be_fused(card))
        {
            continue;
        }
        //mono
        if(!factions.empty() && (std::find(factions.begin(), factions.end(), card->m_faction) == factions.end()) != invert_factions) //XOR
        {
            continue;
        }


        // enqueue candidate according to category & type
        if (card->m_type == CardType::commander)
        {
            commander_candidates.emplace_back(card);
        }
        else if (card->m_category == CardCategory::dominion_alpha)
        {
            alpha_dominion_candidates.emplace_back(card);
        }
        else if (card->m_category == CardCategory::normal)
        {
            bool contains{false};
            if(!skills.empty())
            {
              for(Skill::Skill skill_id : skills)
              {
                  if(card->m_skill_value[skill_id])
                  {
                    contains = true;
                    break;
                  }
              }
              if(!contains != invert_skills) continue; //XOR
            }
            if(only_recent && !((it + (player_assaults_and_structures.size()*recent_percent/100)  )> player_assaults_and_structures.end()))continue; //latest %
            if(prefered_recent && it + (player_assaults_and_structures.size()*recent_percent)/100 > player_assaults_and_structures.end()) //latest %
            {
                for(unsigned k=0; k < prefered_factor;++k)
                  card_candidates.emplace_back(card);
            }
            for(Skill::Skill skill_id : prefered_skills)
            {
                if(card->m_skill_value[skill_id])
                {
                    for(unsigned k =0;k<prefered_factor;++k)
                      card_candidates.emplace_back(card);
                }
            }

            card_candidates.emplace_back(card);
        }
    }
    card_candidates.emplace_back(nullptr);

    ret_candidates.emplace_back(commander_candidates);
    ret_candidates.emplace_back(alpha_dominion_candidates);
    ret_candidates.emplace_back(card_candidates);
    return ret_candidates;
}
inline bool try_improve_deck(Deck* d1, unsigned from_slot, unsigned to_slot, const Card* card_candidate,
        const Card*& best_commander, const Card*& best_alpha_dominion, std::vector<const Card*>& best_cards,
        FinalResults<long double>& best_score, unsigned& best_gap, std::string& best_deck,
        std::unordered_map<std::string, EvaluatedResults>& evaluated_decks, EvaluatedResults& zero_results,
        unsigned long& skipped_simulations, Process& proc)
{
    unsigned deck_cost(0);
    std::vector<std::pair<signed, const Card *>> cards_out, cards_in;
    std::mt19937& re = proc.threads_data[0]->re;

    // setup best deck
    d1->commander = best_commander;
    d1->alpha_dominion = best_alpha_dominion;
    d1->cards = best_cards;

    // try to adjust the deck
    if (!adjust_deck(d1, from_slot, to_slot, card_candidate, fund, re, deck_cost, cards_out, cards_in))
    { return false; }

    // check gap
    unsigned new_gap = check_requirement(d1, requirement
#ifndef NQUEST
            , quest
#endif
            );
    if ((new_gap > 0) && (new_gap >= best_gap))
    { return false; }

    // check previous simulations
    auto && cur_deck = d1->hash();
    auto && emplace_rv = evaluated_decks.insert({cur_deck, zero_results});
    auto & prev_results = emplace_rv.first->second;
    if (!emplace_rv.second)
    {
        skipped_simulations += prev_results.second;
    }

    // Evaluate new deck
    auto compare_results = proc.compare(best_score.n_sims, prev_results, best_score);
    auto current_score = compute_score(compare_results, proc.factors);

    // Is it better ?
    if (new_gap < best_gap || current_score.points > best_score.points + min_increment_of_score)
    {
        // Then update best score/slot, print stuff
        std::cout << "Deck improved: " << d1->hash() << ": " << card_slot_id_names(cards_out) << " -> " << card_slot_id_names(cards_in) << ": ";
        best_gap = new_gap;
        best_score = current_score;
        best_deck = cur_deck;
        best_commander = d1->commander;
        best_alpha_dominion = d1->alpha_dominion;
        best_cards = d1->cards;
        print_score_info(compare_results, proc.factors);
        print_deck_inline(deck_cost, best_score, d1);
        return true;
    }

    return false;
}
//------------------------------------------------------------------------------
void hill_climbing(unsigned num_min_iterations, unsigned num_iterations, Deck* d1, Process& proc, Requirement & requirement
#ifndef NQUEST
        , Quest & quest
#endif
        )
{
    EvaluatedResults zero_results = { EvaluatedResults::first_type(proc.enemy_decks.size()), 0 };
    std::string best_deck = d1->hash();
    std::unordered_map<std::string, EvaluatedResults> evaluated_decks{{best_deck, zero_results}};
    EvaluatedResults& results = proc.evaluate(num_min_iterations, evaluated_decks.begin()->second);
    print_score_info(results, proc.factors);
    FinalResults<long double> best_score = compute_score(results, proc.factors);
    const Card* best_commander = d1->commander;
    const Card* best_alpha_dominion = d1->alpha_dominion;
    std::vector<const Card*> best_cards = d1->cards;
    unsigned deck_cost = get_deck_cost(d1);
    fund = std::max(fund, deck_cost);
    print_deck_inline(deck_cost, best_score, d1);
    std::mt19937& re = proc.threads_data[0]->re;
    unsigned best_gap = check_requirement(d1, requirement
#ifndef NQUEST
            , quest
#endif
            );
    bool is_random = (d1->strategy == DeckStrategy::random) || (d1->strategy == DeckStrategy::flexible);
    bool deck_has_been_improved = true;
    unsigned long skipped_simulations = 0;
    std::vector<const Card*> commander_candidates;
    std::vector<const Card*> alpha_dominion_candidates;
    std::vector<const Card*> card_candidates;

    auto mixed_candidates = get_candidate_lists(proc);
    commander_candidates = mixed_candidates.at(0);
    alpha_dominion_candidates = mixed_candidates.at(1);
    card_candidates = mixed_candidates.at(2);
    // add current alpha dominion to candidates if necessary
    // or setup first candidate into the deck if no alpha dominion defined
    if (use_owned_dominions)
    {
        if (best_alpha_dominion)
        {
            if (!std::count(alpha_dominion_candidates.begin(), alpha_dominion_candidates.end(), best_alpha_dominion))
            {
                alpha_dominion_candidates.emplace_back(best_alpha_dominion);
            }
        }
        else if (!alpha_dominion_candidates.empty())
        {
            best_alpha_dominion = d1->alpha_dominion = alpha_dominion_candidates[0];
        }
        if (debug_print > 0)
        {
            for (const Card* dom_card : alpha_dominion_candidates)
            {
                std::cout << " ** next Alpha Dominion candidate: " << dom_card->m_name
                    << " ($: " << alpha_dominion_cost(dom_card) << ")" << std::endl;
            }
        }
        if (!best_alpha_dominion && owned_alpha_dominion)
        {
            best_alpha_dominion = owned_alpha_dominion;
            std::cout << "Setting up owned Alpha Dominion into a deck: " << best_alpha_dominion->m_name << std::endl;
        }
    }
    //std::reverse(card_candidates.begin(), card_candidates.end());



    // << main climbing loop >>
    for (unsigned from_slot(freezed_cards), dead_slot(freezed_cards); ;
            from_slot = std::max(freezed_cards, (from_slot + 1) % std::min<unsigned>(max_deck_len, best_cards.size() + 1)))
    {
        if(is_timeout_reached()){ break;}
        if (deck_has_been_improved)
        {
            dead_slot = from_slot;
            deck_has_been_improved = false;
        }
        else if (from_slot == dead_slot || best_score.points - target_score > -1e-9)
        {
            if (best_score.n_sims >= num_iterations || best_gap > 0)
            { break; }
            auto & prev_results = evaluated_decks[best_deck];
            skipped_simulations += prev_results.second;
            // Re-evaluate the best deck
            d1->commander = best_commander;
            d1->alpha_dominion = best_alpha_dominion;
            d1->cards = best_cards;
            auto evaluate_result = proc.evaluate(std::min(prev_results.second * iterations_multiplier, num_iterations), prev_results);
            best_score = compute_score(evaluate_result, proc.factors);
            std::cout << "Results refined: ";
            print_score_info(evaluate_result, proc.factors);
            dead_slot = from_slot;
        }
        if (best_score.points - target_score > -1e-9)
        { continue; }

        // commander
        if (requirement.num_cards.count(best_commander) == 0)
        {
            // << commander candidate loop >>
            for (const Card* commander_candidate: commander_candidates)
            {
                if (best_score.points - target_score > -1e-9)
                { break; }
                if (commander_candidate == best_commander)
                { continue; }
                deck_has_been_improved |= try_improve_deck(d1, -1, -1, commander_candidate,
                        best_commander, best_alpha_dominion, best_cards, best_score, best_gap, best_deck,
                        evaluated_decks, zero_results, skipped_simulations, proc);
            }
            // Now that all commanders are evaluated, take the best one
            d1->commander = best_commander;
            d1->alpha_dominion = best_alpha_dominion;
            d1->cards = best_cards;
        }

        // alpha dominion
        if (use_owned_dominions && !alpha_dominion_candidates.empty())
        {
            // << alpha dominion candidate loop >>
            for (const Card* alpha_dominion_candidate: alpha_dominion_candidates)
            {
                if (best_score.points - target_score > -1e-9)
                { break; }
                if (alpha_dominion_candidate == best_alpha_dominion)
                { continue; }
                deck_has_been_improved |= try_improve_deck(d1, -1, -1, alpha_dominion_candidate,
                        best_commander, best_alpha_dominion, best_cards, best_score, best_gap, best_deck,
                        evaluated_decks, zero_results, skipped_simulations, proc);
            }
            // Now that all alpha dominions are evaluated, take the best one
            d1->commander = best_commander;
            d1->alpha_dominion = best_alpha_dominion;
            d1->cards = best_cards;
        }

        // shuffle candidates
        std::shuffle(card_candidates.begin(), card_candidates.end(), re);

        // << card candidate loop >>
        //for (const Card* card_candidate: card_candidates)
        for (auto it = card_candidates.begin(); it != card_candidates.end();++it)
        {
            const Card* card_candidate = *it;
            for (unsigned to_slot(is_random ? from_slot : card_candidate ? freezed_cards : (best_cards.size() - 1));
                    to_slot < (is_random ? (from_slot + 1) : (best_cards.size() + (from_slot < best_cards.size() ? 0 : 1)));
                    ++ to_slot)
            {
                if (card_candidate ?
                        (from_slot < best_cards.size() && (from_slot == to_slot && card_candidate == best_cards[to_slot])) // 2 Omega -> 2 Omega
                        :
                        (from_slot == best_cards.size())) // void -> void
                { continue; }
                deck_has_been_improved |= try_improve_deck(d1, from_slot, to_slot, card_candidate,
                        best_commander, best_alpha_dominion, best_cards, best_score, best_gap, best_deck,
                        evaluated_decks, zero_results, skipped_simulations, proc);
            }
            if (best_score.points - target_score > -1e-9)
            { break; }

        }
    }
    d1->commander = best_commander;
    d1->alpha_dominion = best_alpha_dominion;
    d1->cards = best_cards;
    unsigned simulations = 0;
    for (auto evaluation: evaluated_decks)
    { simulations += evaluation.second.second; }
    std::cout << "Evaluated " << evaluated_decks.size() << " decks (" << simulations << " + " << skipped_simulations << " simulations)." << std::endl;
    std::cout << "Optimized Deck: ";
    print_deck_inline(get_deck_cost(d1), best_score, d1);
    print_upgraded_cards(d1);
    print_sim_card_values(d1,proc,num_iterations);
}

inline FinalResults<long double> fitness(Deck* d1,
        FinalResults<long double>& best_score,
        std::unordered_map<std::string, EvaluatedResults>& evaluated_decks, EvaluatedResults& zero_results,
        unsigned long& skipped_simulations, Process& proc)
{

    // check previous simulations
    auto && cur_deck = d1->hash();
    //std::cout << "Deck hash: " << d1->hash() << " with ";
    auto && emplace_rv = evaluated_decks.insert({cur_deck, zero_results});
    auto & prev_results = emplace_rv.first->second;
    if (!emplace_rv.second)
    {
        skipped_simulations += prev_results.second;
    }

    // Evaluate new deck

    auto compare_results= proc.evaluate(best_score.n_sims, prev_results);

    auto current_score = compute_score(compare_results, proc.factors);

    //best_score = current_score;
    //auto best_deck = d1->clone();
    //print_score_info(compare_results, proc.factors);
    //print_deck_inline(get_deck_cost(best_deck), best_score, best_deck);
    return current_score;
}

inline long double acceptanceProbability(long double old_score, long double new_score, long double temperature)
{
    if(new_score > old_score)
    {
        return 1;
    }
    //100/max_score normalizes the acceptance probability with the used mode/score-range
    //1000 is factor to set the temperature lower (can be changed indirect by setting begin-temperature and its decrease)
    return exp(((new_score-old_score)/temperature*1000*100)/max_possible_score[(size_t)optimization_mode]);
}

void simulated_annealing(unsigned num_min_iterations, unsigned num_iterations, Deck* cur_deck, Process& proc, Requirement & requirement
#ifndef NQUEST
        , Quest & quest
#endif
        )
{
    EvaluatedResults zero_results = { EvaluatedResults::first_type(proc.enemy_decks.size()), 0 };
    //std::string best_deck = d1->hash();
    std::unordered_map<std::string, EvaluatedResults> evaluated_decks{{cur_deck->hash(), zero_results}};
    EvaluatedResults& results = proc.evaluate(num_min_iterations, evaluated_decks.begin()->second);
    print_score_info(results, proc.factors);
    FinalResults<long double> best_score = compute_score(results, proc.factors);
    //const Card* best_commander = d1->commander;
    //const Card* best_alpha_dominion = cur_deck->alpha_dominion;
    std::vector<const Card*> best_cards = cur_deck->cards;
    unsigned deck_cost = get_deck_cost(cur_deck);
    fund = std::max(fund, deck_cost);
    print_deck_inline(deck_cost, best_score, cur_deck);
    std::mt19937& re = proc.threads_data[0]->re;
    unsigned cur_gap = check_requirement(cur_deck, requirement
#ifndef NQUEST
            , quest
#endif
            );

    bool is_random = (cur_deck->strategy == DeckStrategy::random) || (cur_deck->strategy == DeckStrategy::flexible);
    unsigned long skipped_simulations = 0;
    std::vector<const Card*> all_candidates;

    auto mixed_candidates = get_candidate_lists(proc);
    all_candidates.reserve(mixed_candidates.at(0).size()+mixed_candidates.at(1).size()+mixed_candidates.at(2).size());
    all_candidates.insert(all_candidates.end(), std::make_move_iterator(mixed_candidates.at(0).begin()),std::make_move_iterator(mixed_candidates.at(0).end()));
    all_candidates.insert(all_candidates.end(), std::make_move_iterator(mixed_candidates.at(1).begin()),std::make_move_iterator(mixed_candidates.at(1).end()));
    all_candidates.insert(all_candidates.end(), std::make_move_iterator(mixed_candidates.at(2).begin()),std::make_move_iterator(mixed_candidates.at(2).end()));
    //clear
    mixed_candidates.at(0).clear(); mixed_candidates.at(0).shrink_to_fit();
    mixed_candidates.at(1).clear(); mixed_candidates.at(1).shrink_to_fit();
    mixed_candidates.at(2).clear(); mixed_candidates.at(2).shrink_to_fit();
    mixed_candidates.clear();mixed_candidates.shrink_to_fit();

    // add current alpha dominion to candidates if necessary
    // or setup first candidate into the deck if no alpha dominion defined
    if (use_owned_dominions)
    {
        if (cur_deck->alpha_dominion)
        {
            if (!std::count(all_candidates.begin(), all_candidates.end(), cur_deck->alpha_dominion))
            {
                all_candidates.emplace_back(cur_deck->alpha_dominion);
            }
        }
        if (!cur_deck->alpha_dominion && owned_alpha_dominion)
        {
            cur_deck->alpha_dominion = owned_alpha_dominion;
            std::cout << "Setting up owned Alpha Dominion into a deck: " << cur_deck->alpha_dominion->m_name << std::endl;
        }
    }

    Deck* prev_deck = cur_deck->clone();
    Deck* best_deck = cur_deck->clone();

    FinalResults<long double> prev_score = best_score;
    FinalResults<long double> cur_score = best_score;

    unsigned best_gap = cur_gap;

    deck_cost = 0;

    unsigned from_slot(freezed_cards);
    unsigned from_slot_tmp(freezed_cards);
    unsigned to_slot(1);

    if(debug_print >0)std::cout << "Starting Anneal" << std::endl;
    while(temperature > 1 && !(best_score.points - target_score > -1e-9 || is_timeout_reached()))
    {
        cur_deck->commander = prev_deck->commander;
        cur_deck->alpha_dominion = prev_deck->alpha_dominion;
        cur_deck->cards = prev_deck->cards;
        from_slot = std::max(freezed_cards, (from_slot+1) % std::min<unsigned>(max_deck_len, cur_deck->cards.size() +1));
        const Card* candidate = all_candidates.at(std::uniform_int_distribution<unsigned>(0,all_candidates.size()-1)(re));


        if((!candidate || (candidate->m_category == CardCategory::normal && candidate->m_type != CardType::commander && candidate->m_category != CardCategory::dominion_alpha)))
        {

            to_slot = std::uniform_int_distribution<unsigned>(is_random ? from_slot : candidate ? freezed_cards : (cur_deck->cards.size() -1),(is_random ? (from_slot+1) : (cur_deck->cards.size() + ( from_slot < cur_deck->cards.size() ? 0 : 1)))-1)(re);
            if(candidate ?
                    (from_slot < cur_deck->cards.size() && (from_slot == to_slot && candidate == cur_deck->cards[to_slot]))
                    :
                    (from_slot == best_cards.size()))
            {
                continue;
            }
            from_slot_tmp = from_slot;
        }
        else if(candidate->m_type == CardType::commander && requirement.num_cards.count(cur_deck->commander) == 0)
        {
            cur_deck->commander = candidate;
            from_slot_tmp = -1;
            to_slot = -1;
        }
        else if(candidate->m_category == CardCategory::dominion_alpha && use_owned_dominions)
        {
            cur_deck->alpha_dominion = candidate;
            from_slot_tmp = -1;
            to_slot = -1;
        }
        else{
            continue;
        }

        std::vector<std::pair<signed, const Card * >> cards_out, cards_in;
        if (!adjust_deck(cur_deck, from_slot_tmp, to_slot, candidate, fund, re, deck_cost, cards_out, cards_in))
        { continue;}
        cur_gap = check_requirement(cur_deck, requirement
#ifndef NQUEST
                , quest
#endif
                );
        if ((cur_gap > 0) && (cur_gap >= best_gap))
        { continue; }

        //same deck skip
        if(cur_deck->hash().compare(prev_deck->hash())==0)continue;



        cur_score = fitness(cur_deck, best_score, evaluated_decks, zero_results, skipped_simulations, proc);

        if(acceptanceProbability(prev_score.points, cur_score.points , temperature) > std::uniform_real_distribution<double>(0,1)(re))
        {
            if(cur_score.points > best_score.points)
            {
                best_score = cur_score;
                best_deck = cur_deck->clone();
                best_gap = cur_gap;
                std::cout << "Deck improved: " << best_deck->hash() << ": (temp=" << temperature << ") :";
                print_deck_inline(get_deck_cost(best_deck), best_score, best_deck);
            }
            if(debug_print>0)std::cout << "UPDATED DECK: " << cur_deck->hash() << ": (temp=" << temperature << ") :";
            if(debug_print>0)print_deck_inline(get_deck_cost(cur_deck), cur_score, cur_deck);
            prev_score = cur_score;
            prev_deck = cur_deck->clone();
        }
        temperature *=1-coolingRate;
    }
    unsigned simulations = 0;
    for (auto evaluation: evaluated_decks)
    { simulations += evaluation.second.second; }
    std::cout << "Evaluated " << evaluated_decks.size() << " decks (" << simulations << " + " << skipped_simulations << " simulations)." << std::endl;
    std::cout << "Optimized Deck: ";
    print_deck_inline(get_deck_cost(best_deck), best_score, best_deck);
    print_upgraded_cards(best_deck);
    print_sim_card_values(best_deck,proc,num_iterations);
}
unsigned factorial(unsigned n)
{
   	unsigned retval = 1;
   	for (int i = n; i > 1; --i)
   		   retval *= i;
   	return retval;
}

void recursion(unsigned num_iterations, std::vector<unsigned> used, unsigned pool, std::vector<const Card*> forts,Process&proc, FinalResults<long double>& best_score,std::vector<const Card*> & best_forts,std::unordered_map<std::string,EvaluatedResults> & evaluated_decks, EvaluatedResults& zero_results, unsigned long& skipped_simulations)
{
    if(used.size()==pool)
    {
        for(auto your_deck : proc.your_decks)
        {
            your_deck->fortress_cards.clear();
            for(unsigned i = 0; i < pool;++i)
            {
                your_deck->fortress_cards.emplace_back(forts[used[i]]);
            }
        }
        //sim
        std::stringstream ios;
        encode_deck_ext_b64(ios,proc.your_decks[0]->fortress_cards);
        auto hash = ios.str();
        auto && emplace_rv = evaluated_decks.insert({hash,zero_results});
        auto & prev_results = emplace_rv.first->second;
        if(!emplace_rv.second)
        {
          skipped_simulations += prev_results.second;
        }
        auto compare_results= proc.evaluate(num_iterations, prev_results);
        auto current_score = compute_score(compare_results, proc.factors);

        if(current_score.points > best_score.points+min_increment_of_score) {
            best_score = current_score;
            std::vector<const Card*> copy_forts(proc.your_decks[0]->fortress_cards);
            best_forts = copy_forts;
            std::cout << "Forts improved: " << hash << " : ";
            print_score_info(compare_results, proc.factors);
            print_score_inline(best_score);
            std::cout << ": ";
            print_cards_inline(best_forts);
        }
        used.clear();
        used.shrink_to_fit();
    }
    else
    {
        for(unsigned i =0;i < forts.size();++i)
        {
            if(std::find(used.begin(),used.end(),i)==used.end()) //not contained
            {
                std::vector<unsigned> tmp_used (used);
                tmp_used.emplace_back(i);
                recursion(num_iterations,tmp_used,pool,forts,proc,best_score,best_forts,evaluated_decks,zero_results,skipped_simulations);
            }
        }
    }
}

void forts_climbing(unsigned num_iterations, Process& proc) {
    EvaluatedResults zero_results = { EvaluatedResults::first_type(proc.enemy_decks.size()*proc.your_decks.size()), 0 };
    unsigned pool = std::get<0>(proc.your_decks[0]->variable_forts[0]);
    std::vector<const Card*> forts(std::get<2>(proc.your_decks[0]->variable_forts[0]));
    for(unsigned i =0; i < proc.your_decks.size();++i)
    {
          proc.your_decks[i]->variable_forts.clear();
    }
    std::vector<unsigned> used;
    used.reserve(pool);
    std::vector<const Card*> best_forts{pool};
    FinalResults<long double> best_score;
    unsigned long skipped_simulations{0};
    std::unordered_map<std::string,EvaluatedResults> evaluated_decks{{"",zero_results}};
    recursion(num_iterations,used,pool,forts, proc,best_score,best_forts,evaluated_decks,zero_results,skipped_simulations);
    unsigned simulations = 0;
    for (auto evaluation: evaluated_decks)
    { simulations += evaluation.second.second; }
    std::cout << "Evaluated " << evaluated_decks.size() << " decks (" << simulations << " + " << skipped_simulations << " simulations)." << std::endl;
    std::cout << "Optimized Deck: ";
    print_cards_inline(best_forts);
}


//------------------------------------------------------------------------------
enum Operation {
    noop,
    simulate,
    climb,
    climb_forts,
    anneal,
    reorder,
    debug,
    debuguntil,
};
//------------------------------------------------------------------------------
extern void(*skill_table[Skill::num_skills])(Field*, CardStatus* src_status, const SkillSpec&);
void print_available_effects()
{
    std::cout << "Available effects besides activation skills:\n"
        "  Bloodlust X\n"
        "  Brigade\n"
        "  Counterflux\n"
        "  Divert\n"
        "  Enduring-Rage\n"
        "  Fortification\n"
        "  Heroism\n"
        "  Zealots-Preservation\n"
        "  Metamorphosis\n"
        "  Megamorphosis\n"
        "  Revenge X\n"
        "  Turning-Tides\n"
        "  Virulence\n"
        "  Halted-Orders\n"
        "  Devour X\n"
        "  Critical-Reach\n"
        "  Temporal-Backlash\n"
        "  Furiosity\n"
        "  Oath-Of-Loyalty\n"
        "  Blood-Vengeance\n"
        "  Cold-Sleep\n"
        "  Iron-Will\n"
        "  Unity\n"
        "  Devotion\n"
        "  Crackdown\n"
        ;
}
bool check_input_amount(int argc, char** argv, int argIndex,int number)
{
	if(argc <=argIndex+number)
	{
		std::cerr << argv[argIndex] << " needs " << number << " paramters" <<std::endl;
		return true;
	}
	return false;
}
void input_error(std::string msg)
{
	std::cerr << msg << std::endl;
	exit(EXIT_FAILURE);
}
void usage(int argc, char** argv)
{
    std::cout << "Tyrant Unleashed Optimizer (TUO) " << TYRANT_OPTIMIZER_VERSION << "\n"
        "usage: " << argv[0] << " Your_Deck Enemy_Deck [Flags] [Operations]\n"
        "\n"
        "Your_Deck:\n"
        "  the name/hash/cards of a custom deck.\n"
        "\n"
        "Enemy_Deck:\n"
        "  semicolon separated list of defense decks, syntax:\n"
        "  deck1[:factor1];deck2[:factor2];...\n"
        "  where deck is the name/hash/cards of a mission, raid, quest or custom deck, and factor is optional. The default factor is 1.\n"
        "  example: \'fear:0.2;slowroll:0.8\' means fear is the defense deck 20% of the time, while slowroll is the defense deck 80% of the time.\n"
        "\n"
        "Flags:\n"
        "  -e \"<effect>\": set the battleground effect; you may use -e multiple times.\n"
        "  -r: the attack deck is played in order instead of randomly (respects the 3 cards drawn limit).\n"
        "  -s: use surge (default is fight).\n"
        "  -t <num>: set the number of threads, default is 4.\n"
        "  win:     simulate/optimize for win rate. default for non-raids.\n"
        "  defense: simulate/optimize for win rate + stall rate. can be used for defending deck or win rate oriented raid simulations.\n"
        "  raid:    simulate/optimize for average raid damage (ARD). default for raids.\n"
        "Flags for climb:\n"
        "  -c: don't try to optimize the commander.\n"
        "  -L <min> <max>: restrict deck size between <min> and <max>.\n"
        "  -o: restrict to the owned cards listed in \"data/ownedcards.txt\".\n"
        "  -o=<filename>: restrict to the owned cards listed in <filename>.\n"
        "  fund <num>: invest <num> SP to upgrade cards.\n"
        "  target <num>: stop as soon as the score reaches <num>.\n"
        "\n"
        "Operations:\n"
        "  sim <num>: simulate <num> battles to evaluate a deck.\n"
        "  climb <num>: perform hill-climbing starting from the given attack deck, using up to <num> battles to evaluate a deck.\n"
        "  reorder <num>: optimize the order for given attack deck, using up to <num> battles to evaluate an order.\n"
#ifndef NDEBUG
        "  debug: testing purpose only. very verbose output. only one battle.\n"
        "  debuguntil <min> <max>: testing purpose only. fight until the last fight results in range [<min>, <max>]. recommend to redirect std::coutput.\n"
#endif
        ;
}

bool parse_bge(
        std::string bge_name,
        unsigned player,
        const std::unordered_map<std::string, std::string>& bge_aliases,
        std::array<signed short, PassiveBGE::num_passive_bges>& your_bg_effects,
        std::array<signed short, PassiveBGE::num_passive_bges>& enemy_bg_effects,
        std::vector<SkillSpec>& your_bg_skills,
        std::vector<SkillSpec>& enemy_bg_skills,
        std::unordered_set<std::string> used_bge_aliases
        )
{
    // skip empty
    trim(bge_name);
    if (bge_name.empty()) { return true; }

    // is effect combined?
    if (bge_name.find_first_of(";|") != std::string::npos)
    {
        std::vector<std::string> bges;
        boost::split(bges, bge_name, boost::is_any_of(";|"));
        for (auto && next_bge: bges)
        {
            if (!parse_bge(next_bge, player, bge_aliases, your_bg_effects, enemy_bg_effects, your_bg_skills, enemy_bg_skills, used_bge_aliases))
            { return false; }
        }
        return true;
    }

    // try to resolve bge name as alias
    std::string simple_bge_name = simplify_name(bge_name);
    const auto bge_alias_itr = bge_aliases.find(simple_bge_name);
    if (bge_alias_itr != bge_aliases.end())
    {
        if (!used_bge_aliases.insert(simple_bge_name).second)
        {
            throw std::runtime_error("BGE alias: " + bge_name + ": Circular reference");
        }
        return parse_bge(bge_alias_itr->second, player, bge_aliases, your_bg_effects, enemy_bg_effects, your_bg_skills, enemy_bg_skills, used_bge_aliases);
    }

    // parse as passive or skill based BGE
    std::vector<std::string> tokens, skill_name_list;
    boost::split(tokens, bge_name, boost::is_any_of(" "));
    boost::split(skill_name_list, tokens[0], boost::is_any_of("+"));
    try
    {
        for (auto && skill_name: skill_name_list)
        {
            PassiveBGE::PassiveBGE passive_bge_id = passive_bge_name_to_id(skill_name);
            Skill::Skill skill_id = skill_name_to_id(skill_name);
            if (passive_bge_id != PassiveBGE::no_bge)
            {
                // map bge id to its value (if present otherwise zero)
                signed short bge_value = (tokens.size() > 1) ? boost::lexical_cast<signed short>(tokens[1]) : -1;
                if (!bge_value)
                    throw std::runtime_error("BGE " + skill_name + ": zero value means no BGE");
                if ((player == 0) or (player == 2))
                    your_bg_effects[passive_bge_id] = bge_value;
                if ((player == 1) or (player == 2))
                    enemy_bg_effects[passive_bge_id] = bge_value;
            }
            else if (skill_table[skill_id] != nullptr)
            {
                unsigned skill_index = 1;
                // activation BG skill
                SkillSpec bg_skill{skill_id, 0, allfactions, 0, 0, Skill::no_skill, Skill::no_skill, false, 0,};

                // skill [ ALL | N ] ...
                if (skill_index < tokens.size() && boost::to_lower_copy(tokens[skill_index]) == "all")
                {
                    bg_skill.all = true;
                    skill_index += 1;
                }
                else if (skill_index + 1 < tokens.size() && isdigit(*tokens[skill_index].c_str()))
                {
                    bg_skill.n = boost::lexical_cast<unsigned>(tokens[skill_index]);
                    skill_index += 1;
                }

                // skill n [ FACTION ] ...
                if ((skill_index + 1) < tokens.size())
                {
                    for (auto f = allfactions + 1; f < num_factions; ++f)
                    {
                        if (boost::to_lower_copy(tokens[skill_index]) == faction_names[f])
                        {
                            bg_skill.y = static_cast<Faction>(f);
                            skill_index += 1;
                            break;
                        }
                    }
                }

                if (skill_index < tokens.size())
                {
                    bg_skill.s = skill_name_to_id(tokens[skill_index]);
                    if (bg_skill.s != Skill::no_skill)
                    {
                        skill_index += 1;
                        if (skill_index < tokens.size() && (boost::to_lower_copy(tokens[skill_index]) == "to" || boost::to_lower_copy(tokens[skill_index]) == "into"))
                        {
                            skill_index += 1;
                        }
                        if (skill_index < tokens.size())
                        {
                            bg_skill.s2 = skill_name_to_id(tokens[skill_index]);
                            if (bg_skill.s2 != Skill::no_skill)
                            {
                                skill_index += 1;
                            }
                        }
                    }
                }
                if (skill_index < tokens.size())
                {
                    if (bg_skill.id == Skill::jam || bg_skill.id == Skill::overload)
                    {
                        bg_skill.n = boost::lexical_cast<unsigned>(tokens[skill_index]);
                    }
                    else
                    {
                        bg_skill.x = boost::lexical_cast<unsigned>(tokens[skill_index]);
                    }
                }
                switch (player)
                {
                    case 0:
                        your_bg_skills.push_back(bg_skill);
                        break;
                    case 1:
                        enemy_bg_skills.push_back(bg_skill);
                        break;
                    case 2:
                        your_bg_skills.push_back(bg_skill);
                        enemy_bg_skills.push_back(bg_skill);
                        break;
                    default:
                        throw std::runtime_error("Bad player number: " + boost::lexical_cast<std::string>(player));
                }
            }
            else
            {
                std::cerr << "Error: unrecognized effect \"" << bge_name << "\".\n";
                std::cout << "Unrecognized effect \"" << bge_name << "\".\n";
                print_available_effects();
                return false;
            }
        }
    }
    catch (const boost::bad_lexical_cast & e)
    {
        throw std::runtime_error("Expect a number in effect \"" + bge_name + "\"");
    }
    return true;
}

FinalResults<long double> run(int argc, char** argv)
{
    FinalResults<long double> fr;
    unsigned opt_num_threads(4);
    DeckStrategy::DeckStrategy opt_your_strategy(DeckStrategy::random);
    DeckStrategy::DeckStrategy opt_enemy_strategy(DeckStrategy::random);
    std::string opt_forts, opt_enemy_forts;
    std::string opt_doms, opt_enemy_doms;
    std::string opt_hand, opt_enemy_hand;
    std::string opt_vip;
    std::string opt_allow_candidates;
    std::string opt_disallow_candidates;
    std::string opt_disallow_recipes;
#ifndef NQUEST
    std::string opt_quest;
#endif
    std::string opt_target_score;
    std::vector<std::string> fn_suffix_list{"",};
    std::string prefix = "";
    std::vector<std::string> opt_owned_cards_str_list;
    bool opt_do_optimization(false);
    bool opt_do_reorder(false);
    bool opt_keep_commander{false};
    std::vector<std::tuple<unsigned, unsigned, Operation>> opt_todo;
    std::vector<std::string> opt_effects[3];  // 0-you; 1-enemy; 2-global
    std::array<signed short, PassiveBGE::num_passive_bges> opt_bg_effects[2];
    std::vector<SkillSpec> opt_bg_skills[2];
    std::unordered_set<unsigned> disallowed_recipes;

    for (int argIndex = 3; argIndex < argc; ++argIndex)
    {
        // Codec
        if (strcmp(argv[argIndex], "ext_b64") == 0)
        {
            hash_to_ids = hash_to_ids_ext_b64;
            encode_deck = encode_deck_ext_b64;
        }
        else if (strcmp(argv[argIndex], "wmt_b64") == 0)
        {
            hash_to_ids = hash_to_ids_wmt_b64;
            encode_deck = encode_deck_wmt_b64;
        }
        else if (strcmp(argv[argIndex], "ddd_b64") == 0)
        {
            hash_to_ids = hash_to_ids_ddd_b64;
            encode_deck = encode_deck_ddd_b64;
        }
        // Base Game Mode
        else if (strcmp(argv[argIndex], "fight") == 0)
        {
            gamemode = fight;
        }
        else if (strcmp(argv[argIndex], "-s") == 0 || strcmp(argv[argIndex], "surge") == 0)
        {
            gamemode = surge;
        }
        // Base Scoring Mode
        else if (strcmp(argv[argIndex], "win") == 0)
        {
            optimization_mode = OptimizationMode::winrate;
        }
        else if (strcmp(argv[argIndex], "defense") == 0)
        {
            optimization_mode = OptimizationMode::defense;
        }
        else if (strcmp(argv[argIndex], "raid") == 0)
        {
            optimization_mode = OptimizationMode::raid;
        }
        // Mode Package
        else if (strcmp(argv[argIndex], "campaign") == 0)
        {
            gamemode = surge;
            optimization_mode = OptimizationMode::campaign;
        }
        else if (strcmp(argv[argIndex], "pvp") == 0)
        {
            gamemode = fight;
            optimization_mode = OptimizationMode::winrate;
        }
        else if (strcmp(argv[argIndex], "pvp-defense") == 0)
        {
            gamemode = surge;
            optimization_mode = OptimizationMode::defense;
        }
        else if (strcmp(argv[argIndex], "brawl") == 0)
        {
            gamemode = surge;
            optimization_mode = OptimizationMode::brawl;
        }
        else if (strcmp(argv[argIndex], "brawl-defense") == 0)
        {
            gamemode = fight;
            optimization_mode = OptimizationMode::brawl_defense;
        }
        else if (strcmp(argv[argIndex], "gw") == 0)
        {
            gamemode = surge;
            optimization_mode = OptimizationMode::war;
        }
        else if (strcmp(argv[argIndex], "gw-defense") == 0)
        {
            gamemode = fight;
            optimization_mode = OptimizationMode::war_defense;
        }
        // Others
        else if (strcmp(argv[argIndex], "keep-commander") == 0 || strcmp(argv[argIndex], "-c") == 0)
        {
            opt_keep_commander = true;
        }
        else if (strcmp(argv[argIndex], "mono") == 0 || strcmp(argv[argIndex], "-m") == 0 || strcmp(argv[argIndex], "factions") == 0 || strcmp(argv[argIndex], "-f") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            if(strcmp(argv[argIndex+1],"") != 0) {
                factions.push_back(faction_name_to_id(argv[argIndex + 1]));
            }
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "no-mono") == 0 || strcmp(argv[argIndex], "no-factions") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            factions.push_back(faction_name_to_id(argv[argIndex + 1]));
            invert_factions = true;
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "strategy") == 0 || strcmp(argv[argIndex], "skill") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            if(strcmp(argv[argIndex+1],"") != 0) {
                if (strcmp(argv[argIndex + 1], "recent") == 0) {
                    only_recent = true;
                } else {
                    skills.push_back(skill_name_to_id(argv[argIndex + 1]));
                }
            }
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "no-strategy") == 0 || strcmp(argv[argIndex], "no-skill") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            if(strcmp(argv[argIndex+1],"") != 0) {
            	skills.push_back(skill_name_to_id(argv[argIndex + 1]));
            	invert_skills=true;
	    }
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "prefered-strategy") == 0 || strcmp(argv[argIndex], "prefered-skill") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            if(strcmp(argv[argIndex+1],"") != 0) {
		    if(strcmp(argv[argIndex+1], "recent") == 0)
		    {
			prefered_recent = true;
		    }
		    else
		    {
			prefered_skills.push_back(skill_name_to_id(argv[argIndex + 1]));
		    }
	    }
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "prefered-factor") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            prefered_factor= std::stoi(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "recent-percent") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            recent_percent= std::stoi(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "effect") == 0 || strcmp(argv[argIndex], "-e") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            opt_effects[2].push_back(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "ye") == 0 || strcmp(argv[argIndex], "yeffect") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            opt_effects[0].push_back(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "ee") == 0 || strcmp(argv[argIndex], "eeffect") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            opt_effects[1].push_back(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "freeze") == 0 || strcmp(argv[argIndex], "-F") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            freezed_cards = atoi(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "-L") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,2))exit(1);
            min_deck_len = atoi(argv[argIndex + 1]);
            max_deck_len = atoi(argv[argIndex + 2]);
            argIndex += 2;
        }
        else if (strcmp(argv[argIndex], "-o-") == 0)
        {
            use_owned_cards = false;
        }
        else if (strcmp(argv[argIndex], "-o") == 0)
        {
            opt_owned_cards_str_list.push_back(prefix+"data/ownedcards.txt");
            use_owned_cards = true;
        }
        else if (strncmp(argv[argIndex], "-o=", 3) == 0)
        {
            std::string fname{prefix};
            fname += (argv[argIndex]+3);
            opt_owned_cards_str_list.push_back(fname);
            use_owned_cards = true;
        }
        else if (strncmp(argv[argIndex], "_", 1) == 0)
        {
            fn_suffix_list.push_back(argv[argIndex]);
        }
        else if (strcmp(argv[argIndex], "prefix") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            prefix = argv[argIndex+1];
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "fund") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            fund = atoi(argv[argIndex+1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "dom-none") == 0)
        {
            use_owned_dominions = false;
            use_maxed_dominions = false;
        }
        else if (strcmp(argv[argIndex], "dom+") == 0 || strcmp(argv[argIndex], "dominion+") == 0 || strcmp(argv[argIndex], "dom-owned") == 0)
        {
            use_owned_dominions = true;
            use_maxed_dominions = false;
        }
        else if (strcmp(argv[argIndex], "dom-") == 0 || strcmp(argv[argIndex], "dominion-") == 0 || strcmp(argv[argIndex], "dom-maxed") == 0)
        {
            use_owned_dominions = true;
            use_maxed_dominions = true;
        }
        else if (strcmp(argv[argIndex], "random") == 0)
        {
            opt_your_strategy = DeckStrategy::random;
        }
        else if (strcmp(argv[argIndex], "-r") == 0 || strcmp(argv[argIndex], "ordered") == 0)
        {
            opt_your_strategy = DeckStrategy::ordered;
        }
        else if (strcmp(argv[argIndex], "flexible") == 0)
        {
            opt_your_strategy = DeckStrategy::flexible;
        }
        else if (strcmp(argv[argIndex], "flexible-iter") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            flexible_iter = atoi(argv[argIndex+1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "flexible-turn") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            flexible_turn = atoi(argv[argIndex+1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "exact-ordered") == 0)
        {
            opt_your_strategy = DeckStrategy::exact_ordered;
        }
        else if (strcmp(argv[argIndex], "enemy:flexible") == 0)
        {
            opt_enemy_strategy = DeckStrategy::flexible;
        }
        else if (strcmp(argv[argIndex], "enemy:ordered") == 0)
        {
            opt_enemy_strategy = DeckStrategy::ordered;
        }
        else if (strcmp(argv[argIndex], "enemy:flexible") == 0)
        {
            opt_enemy_strategy = DeckStrategy::flexible;
        }
        else if (strcmp(argv[argIndex], "enemy:exact-ordered") == 0)
        {
            opt_enemy_strategy = DeckStrategy::exact_ordered;
        }
        else if (strcmp(argv[argIndex], "endgame") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            use_fused_card_level = atoi(argv[argIndex+1]);
            argIndex += 1;
        }
#ifndef NQUEST
        else if (strcmp(argv[argIndex], "quest") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            opt_quest = argv[argIndex+1];
            argIndex += 1;
        }
#endif
        else if (strcmp(argv[argIndex], "threads") == 0 || strcmp(argv[argIndex], "-t") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            opt_num_threads = atoi(argv[argIndex+1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "target") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            opt_target_score = argv[argIndex+1];
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "turnlimit") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            turn_limit = atoi(argv[argIndex+1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "mis") == 0)
        {
            min_increment_of_score = atof(argv[argIndex+1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "timeout") == 0) //set timeout in hours. tuo will stop approx. at the given time.
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            maximum_time = atof(argv[argIndex+1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "cl") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            confidence_level = atof(argv[argIndex+1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "+so") == 0)
        {
            simplify_output = true;
        }
        else if (strcmp(argv[argIndex], "+uc") == 0)
        {
            print_upgraded = true;
        }
        else if (strcmp(argv[argIndex], "+vc") == 0)
        {
            print_values = true;
        }
        else if (strcmp(argv[argIndex], "+ci") == 0)
        {
            show_ci = true;
        }
        else if (strcmp(argv[argIndex], "+hm") == 0)
        {
            use_harmonic_mean = true;
        }
        else if (strcmp(argv[argIndex], "seed") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            sim_seed = atoi(argv[argIndex+1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "-v") == 0)
        {
            -- debug_print;
        }
        else if (strcmp(argv[argIndex], "+v") == 0)
        {
            ++ debug_print;
        }
        else if (strcmp(argv[argIndex], "vip") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            opt_vip = argv[argIndex + 1];
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "allow-candidates") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            opt_allow_candidates = argv[argIndex + 1];
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "disallow-candidates") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            opt_disallow_candidates = argv[argIndex + 1];
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "disallow-recipes") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            opt_disallow_recipes = argv[argIndex + 1];
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "hand") == 0)  // set initial hand for test
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            opt_hand = argv[argIndex + 1];
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "enemy:hand") == 0)  // set enemies' initial hand for test
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            opt_enemy_hand = argv[argIndex + 1];
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "yf") == 0 || strcmp(argv[argIndex], "yfort") == 0)  // set forts
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            opt_forts = std::string(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "yfpool") == 0 || strcmp(argv[argIndex], "yfortpool") == 0)  // set forts
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            yfpool = std::stoi(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "ef") == 0 || strcmp(argv[argIndex], "efort") == 0)  // set enemies' forts
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            opt_enemy_forts = std::string(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "efpool") == 0 || strcmp(argv[argIndex], "efortpool") == 0)  // set forts
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            efpool = std::stoi(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "yd") == 0 || strcmp(argv[argIndex], "ydom") == 0)  // set dominions
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            opt_doms = std::string(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "ed") == 0 || strcmp(argv[argIndex], "edom") == 0)  // set enemies' dominions
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            opt_enemy_doms = std::string(argv[argIndex + 1]);
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "sim") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            opt_todo.push_back(std::make_tuple((unsigned)atoi(argv[argIndex + 1]), 0u, simulate));
            if (std::get<0>(opt_todo.back()) < 10) { opt_num_threads = 1; }
            argIndex += 1;
        }
        // climbing tasks
        else if (strcmp(argv[argIndex], "climbex") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,2))exit(1);
            opt_todo.push_back(std::make_tuple((unsigned)atoi(argv[argIndex + 1]), (unsigned)atoi(argv[argIndex + 2]), climb));
            if (std::get<1>(opt_todo.back()) < 10) { opt_num_threads = 1; }
            opt_do_optimization = true;
            argIndex += 2;
        }
        else if (strcmp(argv[argIndex], "climb") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            opt_todo.push_back(std::make_tuple((unsigned)atoi(argv[argIndex + 1]), (unsigned)atoi(argv[argIndex + 1]), climb));
            if (std::get<1>(opt_todo.back()) < 10) { opt_num_threads = 1; }
            opt_do_optimization = true;
            argIndex += 1;
        }
        else if (strcmp(argv[argIndex], "climb_forts") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            opt_todo.push_back(std::make_tuple((unsigned)atoi(argv[argIndex + 1]), (unsigned)atoi(argv[argIndex + 1]), climb_forts));
            if (std::get<1>(opt_todo.back()) < 10) { opt_num_threads = 1; }
            argIndex += 1;
        }
        else if ( strcmp(argv[argIndex], "anneal") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,3))exit(1);
            opt_todo.push_back(std::make_tuple((unsigned)atoi(argv[argIndex + 1]), (unsigned)atoi(argv[argIndex + 1]), anneal));
            temperature = std::stod(argv[argIndex+2]);
            coolingRate = std::stod(argv[argIndex+3]);
            if (std::get<1>(opt_todo.back()) < 10) { opt_num_threads = 1; }
            opt_do_optimization = true;
            argIndex += 3;
        }
        else if (strcmp(argv[argIndex], "reorder") == 0)
        {
	    if(check_input_amount(argc,argv,argIndex,1))exit(1);
            opt_todo.push_back(std::make_tuple((unsigned)atoi(argv[argIndex + 1]), (unsigned)atoi(argv[argIndex + 1]), reorder));
            if (std::get<1>(opt_todo.back()) < 10) { opt_num_threads = 1; }
            opt_do_reorder = true;
            argIndex += 1;
        }
        // climbing options
        else if (strncmp(argv[argIndex], "climb-opts:", 11) == 0)
        {
            std::string climb_opts_str(argv[argIndex] + 11);
            boost::tokenizer<boost::char_delimiters_separator<char>> climb_opts{climb_opts_str, boost::char_delimiters_separator<char>{false, ",", ""}};
            for (const auto & opt : climb_opts)
            {
                const auto delim_pos = opt.find("=");
                const bool has_value = (delim_pos != std::string::npos);
                const std::string & opt_name = has_value ? opt.substr(0, delim_pos) : opt;
                const std::string opt_value{has_value ? opt.substr(delim_pos + 1) : opt};
                auto ensure_opt_value = [](const bool has_value, const std::string & opt_name)
                {
                    if (!has_value)
                    { throw std::runtime_error("climb-opts:" + opt_name + " requires an argument"); }
                };
                if ((opt_name == "iter-mul") or (opt_name == "iterations-multiplier"))
                {
                    ensure_opt_value(has_value, opt_name);
                    iterations_multiplier = std::stoi(opt_value);
                }
                else if ((opt_name == "egc") or (opt_name == "endgame-commander") or (opt_name == "min-commander-fusion-level"))
                {
                    ensure_opt_value(has_value, opt_name);
                    use_fused_commander_level = std::stoi(opt_value);
                }
                else if (opt_name == "use-all-commander-levels")
                {
                    use_top_level_commander = false;
                }
                else if (opt_name == "use-all-card-levels")
                {
                    use_top_level_card = false;
                }
                else if ((opt_name == "recent-boost") or (opt_name == "rb")) //prefer new cards in hill climb and break climb loop faster
                {
                    prefered_recent = true;
                }
                else if ((opt_name == "recent-boost-times") or (opt_name == "rbt")) //prefer new cards in hill climb and break climb loop faster
                {
                    ensure_opt_value(has_value, opt_name);
                    prefered_factor = std::stoi(opt_value);
                }
                else if ((opt_name == "recent-boost-percent") or (opt_name == "rbp")) //prefer new cards in hill climb and break climb loop faster
                {
                    ensure_opt_value(has_value, opt_name);
                    recent_percent = std::stoi(opt_value);
                }
                else if ((opt_name == "otd") or (opt_name == "open-the-deck"))
                {
                    mode_open_the_deck = true;
                }
                else
                {
                    std::cerr << "Error: Unknown climb option " << opt_name;
                    if (has_value)
                    { std::cerr << " (value is: " << opt_value << ")"; }
                    std::cerr << std::endl;
                    exit(1);
                }
            }
        }
        else if (strcmp(argv[argIndex], "debug") == 0)
        {
	          if(check_input_amount(argc,argv,argIndex,1))exit(1);
            opt_todo.push_back(std::make_tuple(0u, 0u, debug));
            opt_num_threads = 1;
        }
        else if (strcmp(argv[argIndex], "debuguntil") == 0)
        {
            // output the debug info for the first battle that min_score <= score <= max_score.
            // E.g., 0 0: lose; 100 100: win (non-raid); 20 100: at least 20 damage (raid).
	          if(check_input_amount(argc,argv,argIndex,2))exit(1);
            opt_todo.push_back(std::make_tuple((unsigned)atoi(argv[argIndex + 1]), (unsigned)atoi(argv[argIndex + 2]), debuguntil));
            opt_num_threads = 1;
            argIndex += 2;
        }
        else
        {
            std::cerr << "Error: Unknown option " << argv[argIndex] << std::endl;
            exit(1);
        }
    }

    Cards all_cards;
    Decks decks;
    std::unordered_map<std::string, std::string> bge_aliases;
    load_skills_set_xml(all_cards, prefix+"data/skills_set.xml", true);
    for (unsigned section = 1;
            load_cards_xml(all_cards, prefix+"data/cards_section_" + to_string(section) + ".xml", false);
            ++ section);
    all_cards.organize();
    load_levels_xml(all_cards, prefix+"data/levels.xml", true);
    all_cards.fix_dominion_recipes();
    for (const auto & suffix: fn_suffix_list)
    {
        load_decks_xml(decks, all_cards, prefix+"data/missions" + suffix + ".xml", prefix+"data/raids" + suffix + ".xml", suffix.empty());
        load_recipes_xml(all_cards, prefix+"data/fusion_recipes_cj2" + suffix + ".xml", suffix.empty());
        read_card_abbrs(all_cards, prefix+"data/cardabbrs" + suffix + ".txt");
    }
    for (const auto & suffix: fn_suffix_list)
    {
        load_custom_decks(decks, all_cards, prefix+"data/customdecks" + suffix + ".txt");
        map_keys_to_set(read_custom_cards(all_cards, prefix+"data/allowed_candidates" + suffix + ".txt", false), allowed_candidates);
        map_keys_to_set(read_custom_cards(all_cards, prefix+"data/disallowed_candidates" + suffix + ".txt", false), disallowed_candidates);
        map_keys_to_set(read_custom_cards(all_cards, prefix+"data/disallowed_recipes" + suffix + ".txt", false), disallowed_recipes);
    }

    read_bge_aliases(bge_aliases,prefix+ "data/bges.txt");

    fill_skill_table();

    if (opt_do_optimization and use_owned_cards)
    {
        if (opt_owned_cards_str_list.empty())
        {  // load default files only if specify no -o=
            for (const auto & suffix: fn_suffix_list)
            {
                std::string filename = prefix+"data/ownedcards" + suffix + ".txt";
                if (boost::filesystem::exists(filename))
                {
                    opt_owned_cards_str_list.push_back(filename);
                }
            }
        }
        std::map<unsigned, unsigned> _owned_cards;
        for (const auto & oc_str: opt_owned_cards_str_list)
        {
            read_owned_cards(all_cards, _owned_cards, oc_str);
        }

        // keep only one copy of alpha dominion
        for (auto owned_it = _owned_cards.begin(); owned_it != _owned_cards.end(); )
        {
            const Card* owned_card = all_cards.by_id(owned_it->first);
            bool need_remove = (!owned_it->second);
            if (!need_remove && (owned_card->m_category == CardCategory::dominion_alpha))
            {
                if (!owned_alpha_dominion)
                {
                    owned_alpha_dominion = owned_card;
                }
                else
                {
                    /*std::cerr << "Warning: ownedcards already contains alpha dominion (" << owned_alpha_dominion->m_name
                      << "): removing additional " << owned_card->m_name << std::endl;
                      need_remove = true;*/
                }
            }
            if (need_remove) { owned_it = _owned_cards.erase(owned_it); }
            else { ++owned_it; }
        }
        if (!owned_alpha_dominion && use_owned_dominions)
        {
            //owned_alpha_dominion = all_cards.by_id(50002);
            //std::cerr << "Warning: dominion climbing enabled and no alpha dominion found in owned cards, adding default "
            //    << owned_alpha_dominion->m_name << std::endl;
        }
        if (owned_alpha_dominion)
        { _owned_cards[owned_alpha_dominion->m_id] = 1; }

        // remap owned cards to unordered map (should be quicker for searching)
        owned_cards.reserve(_owned_cards.size());
        for (auto owned_it = _owned_cards.begin(); owned_it != _owned_cards.end(); ++owned_it)
        {
            owned_cards[owned_it->first] = owned_it->second;
        }
    }

    // parse BGEs
    opt_bg_effects[0].fill(0);
    opt_bg_effects[1].fill(0);
    for (int player = 2; player >= 0; -- player)
    {
        for (auto && opt_effect: opt_effects[player])
        {
            std::unordered_set<std::string> used_bge_aliases;
            if (!parse_bge(opt_effect, player, bge_aliases, opt_bg_effects[0], opt_bg_effects[1], opt_bg_skills[0], opt_bg_skills[1], used_bge_aliases))
            {
                exit(1);
            }
        }
    }


    // parse allowed candidates from options
    try
    {
        auto && id_marks = string_to_ids(all_cards, opt_allow_candidates, "allowed-candidates");
        for (const auto & cid : id_marks.first)
        {
            allowed_candidates.insert(cid);
        }
    }
    catch(const std::runtime_error& e)
    {
        std::cerr << "Error: allow-candidates " << opt_allow_candidates << ": " << e.what() << std::endl;
        exit(1);
    }

    // parse disallowed candidates from options
    try
    {
        auto && id_marks = string_to_ids(all_cards, opt_disallow_candidates, "disallowed-candidates");
        for (const auto & cid : id_marks.first)
        {
            disallowed_candidates.insert(cid);
        }
    }
    catch(const std::runtime_error& e)
    {
        std::cerr << "Error: disallow-candidates " << opt_disallow_candidates << ": " << e.what() << std::endl;
        exit(1);
    }

    // parse & drop disallowed recipes
    try
    {
        auto && id_dis_recipes = string_to_ids(all_cards, opt_disallow_recipes, "disallowed-recipes");
        for (auto & cid : id_dis_recipes.first)
        { all_cards.erase_fusion_recipe(cid); }
    }
    catch(const std::runtime_error& e)
    {
        std::cerr << "Error: disallow-recipes " << opt_disallow_recipes << ": " << e.what() << std::endl;
        exit(1);
    }
    for (auto cid : disallowed_recipes)
    { all_cards.erase_fusion_recipe(cid); }

#ifndef NQUEST
    if (!opt_quest.empty())
    {
        try
        {
            optimization_mode = OptimizationMode::quest;
            std::vector<std::string> tokens;
            boost::split(tokens, opt_quest, boost::is_any_of(" -"));
            if (tokens.size() < 3)
            {
                throw std::runtime_error("Expect one of: su n skill; sd n skill; cu n faction/strcture; ck n structure");
            }
            auto type_str = boost::to_lower_copy(tokens[0]);
            quest.quest_value = boost::lexical_cast<unsigned>(tokens[1]);
            auto key_str = boost::to_lower_copy(tokens[2]);
            unsigned quest_index = 3;
            if (type_str == "su" || type_str == "sd")
            {
                Skill::Skill skill_id = skill_name_to_id(key_str);
                if (skill_id == Skill::no_skill)
                {
                    std::cerr << "Error: Expect skill in quest \"" << opt_quest << "\".\n";
                    exit(1);
                }
                quest.quest_type = type_str == "su" ? QuestType::skill_use : QuestType::skill_damage;
                quest.quest_key = skill_id;
            }
            else if (type_str == "cu" || type_str == "ck")
            {
                if (key_str == "assault")
                {
                    quest.quest_type = type_str == "cu" ? QuestType::type_card_use : QuestType::type_card_kill;
                    quest.quest_key = CardType::assault;
                }
                else if (key_str == "structure")
                {
                    quest.quest_type = type_str == "cu" ? QuestType::type_card_use : QuestType::type_card_kill;
                    quest.quest_key = CardType::structure;
                }
                else
                {
                    for (unsigned i = 1; i < Faction::num_factions; ++ i)
                    {
                        if (key_str == boost::to_lower_copy(faction_names[i]))
                        {
                            quest.quest_type = type_str == "cu" ? QuestType::faction_assault_card_use : QuestType::faction_assault_card_kill;
                            quest.quest_key = i;
                            break;
                        }
                    }
                    if (quest.quest_key == 0)
                    {
                        std::cerr << "Error: Expect assault, structure or faction in quest \"" << opt_quest << "\".\n";
                        exit(1);
                    }
                }
            }
            else if (type_str == "cs")
            {
                unsigned card_id;
                unsigned card_num;
                char num_sign;
                char mark;
                try
                {
                    parse_card_spec(all_cards, key_str, card_id, card_num, num_sign, mark);
                    quest.quest_type = QuestType::card_survival;
                    quest.quest_key = card_id;
                }
                catch (const std::runtime_error& e)
                {
                    std::cerr << "Error: Expect a card in quest \"" << opt_quest << "\".\n";
                    exit(1);
                }
            }
            else if (type_str == "suoc" && tokens.size() >= 4)
            {
                Skill::Skill skill_id = skill_name_to_id(key_str);
                if (skill_id == Skill::no_skill)
                {
                    std::cerr << "Error: Expect skill in quest \"" << opt_quest << "\".\n";
                    exit(1);
                }
                unsigned card_id;
                unsigned card_num;
                char num_sign;
                char mark;
                try
                {
                    parse_card_spec(all_cards, boost::to_lower_copy(tokens[3]), card_id, card_num, num_sign, mark);
                    quest_index += 1;
                    quest.quest_type = QuestType::skill_use;
                    quest.quest_key = skill_id;
                    quest.quest_2nd_key = card_id;
                }
                catch (const std::runtime_error& e)
                {
                    std::cerr << "Error: Expect a card in quest \"" << opt_quest << "\".\n";
                    exit(1);
                }
            }
            else
            {
                throw std::runtime_error("Expect one of: su n skill; sd n skill; cu n faction/strcture; ck n structure");
            }
            quest.quest_score = quest.quest_value;
            for (unsigned i = quest_index; i < tokens.size(); ++ i)
            {
                const auto & token = tokens[i];
                if (token == "each")
                {
                    quest.must_fulfill = true;
                    quest.quest_score = 100;
                }
                else if (token == "win")
                { quest.must_win = true; }
                else if (token.substr(0, 2) == "q=")
                { quest.quest_score = boost::lexical_cast<unsigned>(token.substr(2)); }
                else if (token.substr(0, 2) == "w=")
                { quest.win_score = boost::lexical_cast<unsigned>(token.substr(2)); }
                else
                { throw std::runtime_error("Cannot recognize " + token); }
            }
            max_possible_score[(size_t)optimization_mode] = quest.quest_score + quest.win_score;
        }
        catch (const boost::bad_lexical_cast & e)
        {
            std::cerr << "Error: Expect a number in quest \"" << opt_quest << "\".\n";
            exit(1);
        }
        catch (const std::runtime_error& e)
        {
            std::cerr << "Error: quest " << opt_quest << ": " << e.what() << std::endl;
            exit(1);
        }
    }
#endif

    //std::string your_deck_name{argv[1]};
    std::string your_deck_list{argv[1]};
    std::string enemy_deck_list{argv[2]};
    auto && your_deck_list_parsed = parse_deck_list(your_deck_list, decks);
    auto && enemy_deck_list_parsed = parse_deck_list(enemy_deck_list, decks);

    //Deck* your_deck{nullptr};
    std::vector<Deck*> your_decks;
    std::vector<Deck*> enemy_decks;
    std::vector<long double> your_decks_factors;
    std::vector<long double> enemy_decks_factors;
    for(auto deck_parsed : your_deck_list_parsed)
    {
        Deck* your_deck{nullptr};
        try
        {
            your_deck = find_deck(decks, all_cards, deck_parsed.first)->clone();
        }
        catch(const std::runtime_error& e)
        {
            std::cerr << "Error: Deck " <<  deck_parsed.first << ": " << e.what() << std::endl;
            exit(1);
        }
        if (your_deck == nullptr)
        {
            std::cerr << "Error: Invalid attack deck name/hash " << deck_parsed.first << ".\n";
            exit(1);
        }
        else if (!your_deck->variable_cards.empty())
        {
            std::cerr << "Error: Invalid attack deck " << deck_parsed.first  << ": has optional cards.\n";
            your_deck = nullptr;
        }
        else if (!your_deck->variable_forts.empty())
        {
            std::cerr << "Error: Invalid attack deck " << deck_parsed.first << ": has optional cards.\n";
            your_deck = nullptr;
        }
        if (your_deck == nullptr)
        {
            usage(argc, argv);
            exit(1);
            //return 255;
        }

        your_deck->strategy = opt_your_strategy;
        if (!opt_forts.empty())
        {
            try
            {
                if(!yfpool)
                  your_deck->add_forts(opt_forts + ",");
                else
                  your_deck->add_pool_forts(opt_forts + ",",yfpool);
            }
            catch(const std::runtime_error& e)
            {
                std::cerr << "Error: yfort " << opt_forts << ": " << e.what() << std::endl;
                exit(1);
            }
        }
        if (!opt_doms.empty())
        {
            try
            {
                your_deck->add_dominions(opt_doms + ",", true);
            }
            catch(const std::runtime_error& e)
            {
                std::cerr << "Error: ydom " << opt_doms << ": " << e.what() << std::endl;
                exit(1);
            }
        }

        try
        {
            your_deck->set_vip_cards(opt_vip);
        }
        catch(const std::runtime_error& e)
        {
            std::cerr << "Error: vip " << opt_vip << ": " << e.what() << std::endl;
            exit(1);
        }

        try
        {
            your_deck->set_given_hand(opt_hand);
        }
        catch(const std::runtime_error& e)
        {
            std::cerr << "Error: hand " << opt_hand << ": " << e.what() << std::endl;
            exit(1);
        }

        your_decks.push_back(your_deck);
        your_decks_factors.push_back(deck_parsed.second);
    }

    target_score = opt_target_score.empty() ? max_possible_score[(size_t)optimization_mode] : boost::lexical_cast<long double>(opt_target_score);

    for (auto deck_parsed: enemy_deck_list_parsed)
    {
        Deck* enemy_deck{nullptr};
        try
        {
            enemy_deck = find_deck(decks, all_cards, deck_parsed.first);
        }
        catch(const std::runtime_error& e)
        {
            std::cerr << "Error: Deck " << deck_parsed.first << ": " << e.what() << std::endl;
            exit(1);
        }
        if (enemy_deck == nullptr)
        {
            std::cerr << "Error: Invalid defense deck name/hash " << deck_parsed.first << ".\n";
            usage(argc, argv);
            exit(1);
        }
        if (optimization_mode == OptimizationMode::notset)
        {
            if (enemy_deck->decktype == DeckType::raid)
            {
                optimization_mode = OptimizationMode::raid;
            }
            else if (enemy_deck->decktype == DeckType::campaign)
            {
                gamemode = surge;
                optimization_mode = OptimizationMode::campaign;
            }
            else
            {
                optimization_mode = OptimizationMode::winrate;
            }
        }
        enemy_deck->strategy = opt_enemy_strategy;
        if (!opt_enemy_doms.empty())
        {
            try
            {
                enemy_deck->add_dominions(opt_enemy_doms + ",", true);
            }
            catch(const std::runtime_error& e)
            {
                std::cerr << "Error: edom " << opt_enemy_doms << ": " << e.what() << std::endl;
                exit(1);
            }
        }
        if (!opt_enemy_forts.empty())
        {
            try
            {
                if(!efpool)
                  enemy_deck->add_forts(opt_enemy_forts + ",");
                else
                  enemy_deck->add_pool_forts(opt_enemy_forts + ",",efpool);

            }
            catch(const std::runtime_error& e)
            {
                std::cerr << "Error: efort " << opt_enemy_forts << ": " << e.what() << std::endl;
                exit(1);
            }
        }
        try
        {
            enemy_deck->set_given_hand(opt_enemy_hand);
        }
        catch(const std::runtime_error& e)
        {
            std::cerr << "Error: enemy:hand " << opt_enemy_hand << ": " << e.what() << std::endl;
            exit(1);
        }
        enemy_decks.push_back(enemy_deck);
        enemy_decks_factors.push_back(deck_parsed.second);
    }
    std::vector<long double> factors(your_decks_factors.size()*enemy_decks_factors.size());
    for(unsigned i =0; i < factors.size();++i)
    {
        factors[i] = your_decks_factors[i/enemy_decks_factors.size()]*enemy_decks_factors[i%enemy_decks_factors.size()];
    }
    if((opt_do_optimization || opt_do_reorder) && your_decks.size() != 1) {
        std::cerr << "Optimization only works with a single deck" << std::endl;
        exit(1);
    }
    if(opt_do_optimization || opt_do_reorder)  // => your_decks.site()==1
    {
        auto your_deck = your_decks[0];

        if (opt_keep_commander)
        {
            requirement.num_cards[your_deck->commander] = 1;
        }
        for (auto && card_mark: your_deck->card_marks)
        {
            auto && card = card_mark.first < 0 ? your_deck->commander : your_deck->cards[card_mark.first];
            auto mark = card_mark.second;
            if ((mark == '!') && ((card_mark.first >= 0) || !opt_keep_commander))
            {
                requirement.num_cards[card] += 1;
            }
        }
        // Force to claim cards in your initial deck.
        if (opt_do_optimization and use_owned_cards)
        {
            claim_cards({your_deck->commander});
            claim_cards(your_deck->cards);
            if (your_deck->alpha_dominion)
                claim_cards({your_deck->alpha_dominion});
        }

        // shrink any oversized deck to maximum of 10 cards + commander
        // NOTE: do this AFTER the call to claim_cards so that passing an initial deck of >10 cards
        //       can be used as a "shortcut" for adding them to owned cards. Also this allows climb
        //       to figure out which are the best 10, rather than restricting climb to the first 10.
        if (your_deck->cards.size() > max_deck_len)
        {
            your_deck->shrink(max_deck_len);
            if (debug_print >= 0)
            {
                std::cerr << "WARNING: Too many cards in your deck. Trimmed.\n";
            }
        }
        freezed_cards = std::min<unsigned>(freezed_cards, your_deck->cards.size());
    }

    if (debug_print >= 0)
    {
        for (unsigned i(0); i < your_decks.size(); ++i)
        {
            auto your_deck = your_decks[i];
            std::cout << "Your Deck:" << your_decks_factors[i] << ": "<< (debug_print > 0 ? your_deck->long_description() : your_deck->medium_description()) << std::endl;
        }
        for (unsigned bg_effect = PassiveBGE::no_bge; bg_effect < PassiveBGE::num_passive_bges; ++bg_effect)
        {
            auto bge_value = opt_bg_effects[0][bg_effect];
            if (!bge_value)
                continue;
            std::cout << "Your BG Effect: " << passive_bge_names[bg_effect];
            if (bge_value != -1)
                std::cout << " " << bge_value;
            std::cout << std::endl;
        }
        for (const auto & bg_skill: opt_bg_skills[0])
        {
            std::cout << "Your BG Skill: " << skill_description(all_cards, bg_skill) << std::endl;
        }

        for (unsigned i(0); i < enemy_decks.size(); ++i)
        {
            auto enemy_deck = enemy_decks[i];
            std::cout << "Enemy's Deck:" << enemy_decks_factors[i] << ": "
                << (debug_print > 0 ? enemy_deck->long_description() : enemy_deck->medium_description()) << std::endl;
        }
        for (unsigned bg_effect = PassiveBGE::no_bge; bg_effect < PassiveBGE::num_passive_bges; ++bg_effect)
        {
            auto bge_value = opt_bg_effects[1][bg_effect];
            if (!bge_value)
                continue;
            std::cout << "Enemy's BG Effect: " << passive_bge_names[bg_effect];
            if (bge_value != -1)
                std::cout << " " << bge_value;
            std::cout << std::endl;
        }
        for (const auto & bg_skill: opt_bg_skills[1])
        {
            std::cout << "Enemy's BG Skill: " << skill_description(all_cards, bg_skill) << std::endl;
        }
    }
    if (enemy_decks.size() == 1)
    {
        auto enemy_deck = enemy_decks[0];
        for (auto x_mult_ss : enemy_deck->effects)
        {
            if (debug_print >= 0)
            {
                std::cout << "Enemy's X-Mult BG Skill (effective X = round_up[X * " << enemy_deck->level << "]): "
                    << skill_description(all_cards, x_mult_ss);
                if (x_mult_ss.x) { std::cout << " (eff. X = " << ceil(x_mult_ss.x * enemy_deck->level) << ")"; }
                std::cout << std::endl;
            }
            opt_bg_skills[1].push_back({x_mult_ss.id,
                    (unsigned)ceil(x_mult_ss.x * enemy_deck->level),
                    x_mult_ss.y, x_mult_ss.n, x_mult_ss.c,
                    x_mult_ss.s, x_mult_ss.s2, x_mult_ss.all});
        }
    }

    Process p(opt_num_threads, all_cards, decks, your_decks, enemy_decks, factors, gamemode,
#ifndef NQUEST
            quest,
#endif
            opt_bg_effects[0], opt_bg_effects[1], opt_bg_skills[0], opt_bg_skills[1]);

    auto your_deck = your_decks[0];

    for (auto op: opt_todo)
    {
        switch(std::get<2>(op))
        {
            case noop:
                break;
            case simulate: {
                               EvaluatedResults results = { EvaluatedResults::first_type(enemy_decks.size()*your_decks.size()), 0 };
                               results = p.evaluate(std::get<0>(op), results);
                               print_results(results, p.factors);
                               fr = compute_score(results,p.factors);
                               print_sim_card_values(your_deck,p,std::get<0>(op));
                               break;
                           }
            case climb_forts: {
                                  forts_climbing(std::get<0>(op),p);
                                  break;
                              }
            case climb: {
                            //TODO check for your_decks.size()==1
                            hill_climbing(std::get<0>(op), std::get<1>(op), your_deck, p, requirement
#ifndef NQUEST
                                    , quest
#endif
                                    );
                            break;
                        }
            case anneal: {
                              //TODO check for your_decks.size()==1
                             simulated_annealing(std::get<0>(op), std::get<1>(op), your_deck, p, requirement
#ifndef NQUEST
                                     , quest
#endif
                                     );
                             break;

                         }
            case reorder: {

                              //TODO check for your_decks.size()==1
                              your_deck->strategy = DeckStrategy::ordered;
                              use_owned_cards = true;
                              use_top_level_card = false;
                              use_top_level_commander = false;
                              use_owned_dominions = false;
                              if (min_deck_len == 1 && max_deck_len == 10)
                              {
                                  min_deck_len = max_deck_len = your_deck->cards.size();
                              }
                              fund = 0;
                              debug_print = -1;
                              owned_cards.clear();
                              claim_cards({your_deck->commander});
                              claim_cards(your_deck->cards);
                              hill_climbing(std::get<0>(op), std::get<1>(op), your_deck, p, requirement
#ifndef NQUEST
                                      , quest
#endif
                                      );
                              break;
                          }
            case debug: {
                            ++ debug_print;
                            debug_str.clear();
                            EvaluatedResults results{EvaluatedResults::first_type(enemy_decks.size()), 0};
                            results = p.evaluate(1, results);
                            print_results(results, p.factors);
                            -- debug_print;
                            break;
                        }
            case debuguntil: {
                                 ++ debug_print;
                                 ++ debug_cached;
                                 while (true)
                                 {
                                     debug_str.clear();
                                     EvaluatedResults results{EvaluatedResults::first_type(enemy_decks.size()), 0};
                                     results = p.evaluate(1, results);
                                     auto score = compute_score(results, p.factors);
                                     if (score.points >= std::get<0>(op) && score.points <= std::get<1>(op))
                                     {
                                         std::cout << debug_str << std::flush;
                                         print_results(results, p.factors);
                                         break;
                                     }
                                 }
                                 -- debug_cached;
                                 -- debug_print;
                                 break;
                             }
        }
    }
    return fr;
}

#ifndef TEST
int main(int argc,char** argv)
{
#ifndef NTIMER
    boost::timer::auto_cpu_timer t;
#endif
    start_time = std::chrono::system_clock::now();
    if (argc == 2 && strcmp(argv[1], "-version") == 0)
    {
        std::cout << "Tyrant Unleashed Optimizer " << TYRANT_OPTIMIZER_VERSION << std::endl;
        return 0;
    }
    if (argc <= 2)
    {
        usage(argc, argv);
        return 255;
    }
	init();
  run(argc,argv);
  return 0;
}
#endif

#ifndef SIM_H_INCLUDED
#define SIM_H_INCLUDED

#include <boost/pool/pool.hpp>
#include <string>
#include <array>
#include <deque>
#include <tuple>
#include <vector>
#include <unordered_map>
#include <map>
#include <random>

#include "tyrant.h"

class Card;
class Cards;
class Deck;
class Field;
class Achievement;

extern unsigned turn_limit;

inline unsigned safe_minus(unsigned x, unsigned y)
{
    return(x - std::min(x, y));
}

//---------------------- Represent Simulation Results ----------------------------
template<typename result_type>
struct Results
{
    result_type wins;
    result_type draws;
    result_type losses;
    result_type points;
    template<typename other_result_type>
    Results& operator+=(const Results<other_result_type>& other)
    {
        wins += other.wins;
        draws += other.draws;
        losses += other.losses;
        points += other.points;
        return *this;
    }
};

typedef std::pair<std::vector<Results<int64_t>>, unsigned> EvaluatedResults;

template<typename result_type>
struct FinalResults
{
    result_type wins;
    result_type draws;
    result_type losses;
    result_type points;
    result_type points_lower_bound;
    result_type points_upper_bound;
    uint64_t n_sims;
};

void fill_skill_table();
Results<uint64_t> play(Field* fd);
// Pool-based indexed storage.
//---------------------- Pool-based indexed storage ----------------------------
template<typename T>
class Storage
{
public:
    typedef typename std::vector<T*>::size_type size_type;
    typedef T value_type;
    Storage(size_type size) :
        m_pool(sizeof(T))
    {
        m_indirect.reserve(size);
    }

    inline T& operator[](size_type i)
    {
        return(*m_indirect[i]);
    }

    inline T& add_back()
    {
        m_indirect.emplace_back((T*) m_pool.malloc());
        return(*m_indirect.back());
    }

    template<typename Pred>
    void remove(Pred p)
    {
        size_type head(0);
        for(size_type current(0); current < m_indirect.size(); ++current)
        {
            if(p((*this)[current]))
            {
                m_pool.free(m_indirect[current]);
            }
            else
            {
                if(current != head)
                {
                    m_indirect[head] = m_indirect[current];
                }
                ++head;
            }
        }
        m_indirect.erase(m_indirect.begin() + head, m_indirect.end());
    }

    void reset()
    {
        for(auto index: m_indirect)
        {
            m_pool.free(index);
        }
        m_indirect.clear();
    }

    inline size_type size() const
    {
        return(m_indirect.size());
    }

    std::vector<T*> m_indirect;
    boost::pool<> m_pool;
};
//------------------------------------------------------------------------------
enum class CardStep
{
    none,
    attacking,
    attacked,
};
//------------------------------------------------------------------------------
struct CardStatus
{
    const Card* m_card;
    unsigned m_index;
    unsigned m_player;
    unsigned m_delay;
    Faction m_faction;
    unsigned m_attack;
    unsigned m_hp;
    unsigned m_max_hp;
    CardStep m_step;

    unsigned m_corroded_rate;
    unsigned m_corroded_weakened;
    unsigned m_enfeebled;
    unsigned m_evaded;
    unsigned m_inhibited;
    bool m_jammed;
    bool m_overloaded;
    unsigned m_paybacked;
    unsigned m_poisoned;
    unsigned m_protected;
    unsigned m_rallied;
    unsigned m_derallied;
    bool m_rush_attempted;
    bool m_sundered;
    unsigned m_weakened;

    signed m_primary_skill_offset[num_skills];
    signed m_evolved_skill_offset[num_skills];
    unsigned m_enhanced_value[num_skills];
    unsigned m_skill_cd[num_skills];

    CardStatus() {}

    void set(const Card* card);
    void set(const Card& card);
    std::string description() const;
    inline unsigned skill_base_value(Skill skill_id) const;
    unsigned skill(Skill skill_id) const;
    bool has_skill(Skill skill_id) const;
    unsigned enhanced(Skill skill) const;
    unsigned protected_value() const;
};
//------------------------------------------------------------------------------
// Represents a particular draw from a deck.
// Persistent object: call reset to get a new draw.
class Hand
{
public:

    Hand(Deck* deck_) :
        deck(deck_),
        assaults(15),
        structures(15)
    {
    }

    void reset(std::mt19937& re);

    Deck* deck;
    CardStatus commander;
    Storage<CardStatus> assaults;
    Storage<CardStatus> structures;
};

struct Quest
{
    QuestType::QuestType quest_type;
    unsigned quest_key;
    unsigned quest_2nd_key;
    unsigned quest_value;
    unsigned quest_score; // score for quest goal
    unsigned win_score;   // score for win regardless quest goal
    bool must_fulfill;  // true: score iff value is reached; false: score proportion to achieved value
    bool must_win;      // true: score only if win
    Quest() :
        quest_type(QuestType::none),
        quest_key(0),
        quest_value(0),
        quest_score(100),
        win_score(0),
        must_fulfill(false),
        must_win(false)
    {}
};

//------------------------------------------------------------------------------
// struct Field is the data model of a battle:
// an attacker and a defender deck, list of assaults and structures, etc.
class Field
{
public:
    bool end;
    std::mt19937& re;
    const Cards& cards;
    // players[0]: the attacker, players[1]: the defender
    std::array<Hand*, 2> players;
    unsigned tapi; // current turn's active player index
    unsigned tipi; // and inactive
    Hand* tap;
    Hand* tip;
    std::vector<CardStatus*> selection_array;
    unsigned turn;
    gamemode_t gamemode;
    OptimizationMode optimization_mode;
    const Quest quest;
    std::unordered_map<unsigned, unsigned> bg_effects; // passive BGE
    std::vector<SkillSpec> bg_skills[2]; // active BGE, casted every turn
    // With the introduction of on death skills, a single skill can trigger arbitrary many skills.
    // They are stored in this, and cleared after all have been performed.
    std::deque<std::tuple<CardStatus*, SkillSpec>> skill_queue;
    std::vector<CardStatus*> killed_units;
    enum phase
    {
        playcard_phase,
        legion_phase,
        commander_phase,
        structures_phase,
        assaults_phase,
        end_phase,
    };
    // the current phase of the turn: starts with playcard_phase, then commander_phase, structures_phase, and assaults_phase
    phase current_phase;
    // the index of the card being evaluated in the current phase.
    // Meaningless in playcard_phase,
    // otherwise is the index of the current card in players->structures or players->assaults
    unsigned current_ci;

    bool assault_bloodlusted;
    unsigned bloodlust_value;
    unsigned quest_counter;

    Field(std::mt19937& re_, const Cards& cards_, Hand& hand1, Hand& hand2, gamemode_t gamemode_, OptimizationMode optimization_mode_, const Quest & quest_,
            std::unordered_map<unsigned, unsigned>& bg_effects_, std::vector<SkillSpec>& your_bg_skills_, std::vector<SkillSpec>& enemy_bg_skills_) :
        end{false},
        re(re_),
        cards(cards_),
        players{{&hand1, &hand2}},
        turn(1),
        gamemode(gamemode_),
        optimization_mode(optimization_mode_),
        quest(quest_),
        bg_effects{bg_effects_},
        bg_skills{your_bg_skills_, enemy_bg_skills_},
        assault_bloodlusted(false),
        bloodlust_value(0),
        quest_counter(0)
    {
    }

    inline unsigned rand(unsigned x, unsigned y)
    {
        return(std::uniform_int_distribution<unsigned>(x, y)(re));
    }

    inline unsigned flip()
    {
        return(this->rand(0,1));
    }

    template <typename T>
    inline T random_in_vector(const std::vector<T>& v)
    {
        assert(v.size() > 0);
        return(v[this->rand(0, v.size() - 1)]);
    }

    template <typename CardsIter, typename Functor>
    inline unsigned make_selection_array(CardsIter first, CardsIter last, Functor f);
    inline const std::vector<CardStatus *> adjacent_assaults(const CardStatus * status);
    inline void print_selection_array();

    inline void inc_counter(QuestType::QuestType quest_type, unsigned quest_key, unsigned quest_2nd_key = 0, unsigned value = 1)
    {
        if (quest.quest_type == quest_type && quest.quest_key == quest_key && (quest.quest_2nd_key == 0 || quest.quest_2nd_key == quest_2nd_key))
        {
            quest_counter += value;
        }
    }
};

#endif

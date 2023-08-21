// exec: ./tuo-test , verbose: ./tuo-test --log_level=all
// set-iterations ./tuo-test 100  , default = 10 // more than 100 cause errors
#ifdef TEST
#ifdef NQUEST // only without quests
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE sim

#include <chrono>
#include <boost/test/included/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>
#include <iostream>
#include <fstream>
#include <iostream>
#include <exception>
#include <sstream>
#include "tyrant.h"
#include "tyrant_optimize.h"
#include "sim.h"
#include "read.h"

using namespace std;
namespace bdata = boost::unit_test::data;
typedef std::tuple<FinalResults<long double>,std::string,double> Result; // score, output, time
int iter = 10;
unsigned seed = 0;
//limit for float diffing
//disable
double eps = 1;
//enable
//double eps = 0.0000001;

//pipe output: https://stackoverflow.com/questions/5405016/can-i-check-my-programs-output-with-boost-test
struct ios_redirect {
    ios_redirect( std::streambuf * new_buffer ,std::ostream& ios)
        : old( ios.rdbuf( new_buffer )), iios(ios)
    { }

    ~ios_redirect( ) {
        iios.rdbuf( old );
    }

private:
    std::streambuf * old;
    std::ostream& iios;
};

struct TestInfo{
  std::string your_deck,enemy_deck,bge;
};
std::ostream& operator<<(std::ostream& os, const TestInfo& ti)
  {
    return os << "Your Deck: " <<  ti.your_deck << "; Enemy Deck: " << ti.enemy_deck << "; BGE: " << ti.bge;
  }

inline Result run_sim(int argc,const char** argv, bool pipe_output=true)
{
    Result res;
    FinalResults<long double> fr;
    debug_str.clear();
    //

    auto start_time = std::chrono::system_clock::now();
    //pipe output
    std::stringstream output;
    std::stringstream eoutput;
    {
    ios_redirect guard2(eoutput.rdbuf(),std::cerr); //block warnings
    if(pipe_output){
        ios_redirect guard1(output.rdbuf() ,std::cout);

          //////////////////////
          // only single thread for string stream build
          //////////////////////
          const char** param = new const char*[argc+2];
          for(int i = 0; i < argc;i++)
            param[i] = const_cast<char*>(argv[i]);
          param[argc] = const_cast<char*>("-t");
          param[argc+1] = const_cast<char*>("1");
        fr = run(argc+2,param).second;
    }
    else{
        //no guard here
		  //////////////////////
          // only single thread, else crashes
          //////////////////////
        const char** param = new const char*[argc+2];
          for(int i = 0; i < argc;i++)
            param[i] = const_cast<char*>(argv[i]);
          param[argc] = const_cast<char*>("-t");
          param[argc+1] = const_cast<char*>("1");
        fr = run(argc+2,param).second;
    }
  }

    auto end_time = std::chrono::system_clock::now();
    std::chrono::duration<double> delta_t = (end_time - start_time);
    res= std::make_tuple(fr,"\n" + debug_str + output.str(),delta_t.count());
    return res;
}

inline void check_win(Result result) {
    BOOST_CHECK_MESSAGE(
      1-eps<=std::get<0>(result).wins &&
      eps>=std::get<0>(result).losses &&
      eps>=std::get<0>(result).draws
      ,std::get<1>(result));
    //BOOST_CHECK(100==result.points);
}
//inline void check_win_sim(const char* your_deck, const char* enemy_deck, const char* bge="") {
inline void check_win_sim(TestInfo ti) {
    /////////////
    // Max. Iter == 100, else check_win fails with integer vs double equal in check_win
    ////////////
    string s = std::to_string(iter);
    char * ii = new char[s.length()];
    strcpy(ii,s.c_str());
    s = std::to_string(seed);
    char * iii = new char[s.length()];
    strcpy(iii,s.c_str());
    const char* argv[] = {"tuo",ti.your_deck.c_str(),ti.enemy_deck.c_str(),"-e",ti.bge.c_str(),"sim", ii,"seed", iii, "prefix" , "./"}; //much output on error?! // better 100 iterations for test, 10 for checking errors
    //const char* argv[] = {"tuo",ti.your_deck.c_str(),ti.enemy_deck.c_str(),"-e",ti.bge.c_str(),"sim", ii,"seed", iii}; //much output on error?! // better 100 iterations for test, 10 for checking errors
    Result result(run_sim(sizeof(argv)/sizeof(*argv),argv));
    delete ii;
	delete iii;
    //result.second += "\nTest: " + ti.your_deck + "; " + ti.enemy_deck + "; " + ti.bge;
    check_win(result);
}

inline void genetic(std::string gnt1,std::string gnt2){
    string s = std::to_string(iter);
    char * ii = new char[s.length()];
    strcpy(ii,s.c_str());
    const char* argv[] = {"tuo",gnt1.c_str(),gnt2.c_str(),"brawl","genetic",ii};
    Result result(run_sim(sizeof(argv)/sizeof(*argv),argv,false));
    std::ofstream mf;
    mf.open("out.csv", std::ios_base::app);
    mf << gnt1 << ";" << gnt2 << ";" << std::get<0>(result).points << ";" << std::get<2>(result) << std::endl;
    mf.close();
}

inline void check_algo(std::string gnt1,std::string gnt2,std::string algo) {
    int iter = 100;
    string s = std::to_string(iter);
    char * ii = new char[s.length()];
    strcpy(ii,s.c_str());
    s = std::to_string(seed);
    char * iii = new char[s.length()];
    strcpy(iii,s.c_str());
    const char* argv1[] = {"tuo",gnt1.c_str(),gnt2.c_str(),"sim",ii, "seed", iii, "prefix" , "tests/algo/"};
    Result result_sim(run_sim(sizeof(argv1)/sizeof(*argv1),argv1,false));
    const char* argv2[] = {"tuo",gnt1.c_str(),gnt2.c_str(),algo.c_str(),ii, "seed", iii, "prefix" , "tests/algo/"};
    Result result_opt(run_sim(sizeof(argv2)/sizeof(*argv2),argv2,false));
    delete ii;
	delete iii;
    BOOST_CHECK_MESSAGE(std::get<0>(result_sim).wins < std::get<0>(result_opt).wins, 
        std::get<1>(result_sim) + "\n" + std::get<1>(result_opt)
    );
}


std::vector<TestInfo> read_test_file(const std::string filename) {
    std::vector<TestInfo> ret;
    std::ifstream test_file(filename);
    if(test_file.good())
    {
        try{
            while(test_file && !test_file.eof())
            {
                TestInfo ti;
                std::string line,yd,ed,bg;
                std::getline(test_file,line);
                if(is_line_empty_or_commented(line)){ continue;}
                std::istringstream iss(line);
                std::getline(iss,ti.your_deck,';');
                std::getline(iss,ti.enemy_deck,';');
                std::getline(iss,ti.bge,';');
                ret.push_back(ti);
            }
        }
        catch (std::exception& e)
        {
        }
    }
    return ret;
}

BOOST_AUTO_TEST_SUITE(test)
BOOST_AUTO_TEST_CASE(test_init)
{
    init();
    debug_print++;
    debug_cached++;
    debug_line =true;
	seed=std::chrono::system_clock::now().time_since_epoch().count() * 2654435761;
    if(boost::unit_test::framework::master_test_suite().argc>=2)
    {
        iter = atoi(boost::unit_test::framework::master_test_suite().argv[1]);
    }
	if(boost::unit_test::framework::master_test_suite().argc>=3)
	{		
		seed=atoi(boost::unit_test::framework::master_test_suite().argv[2]);
	}
	BOOST_TEST_MESSAGE("ITER: " << iter);
	BOOST_TEST_MESSAGE("SEED: " << seed);
    BOOST_CHECK(1==1);//..
}


//BOOST_AUTO_TEST_SUITE(test_algo)
//BOOST_AUTO_TEST_SUITE(test_algo_climb) // bench_climb
//BOOST_AUTO_TEST_CASE(test_algo_climb)
//{
//        check_algo("Mission#136","Mission#136","climb");
//
//}
//BOOST_AUTO_TEST_SUITE_END()
//BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(test_sim )
/////////////////////////////////////
// Test Cases !should! be very close fights for maximum sensitivity of errors
/////////////////////////////////////
BOOST_AUTO_TEST_SUITE(test_single_units)
BOOST_DATA_TEST_CASE(test_single_units,bdata::make(read_test_file("tests/test_sinlge_units.csv")),ti)
{
   check_win_sim(ti);
}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(test_multi_units)
BOOST_DATA_TEST_CASE(test_multi_units,bdata::make(read_test_file("tests/test_multi_units.csv")),ti)
{
   check_win_sim(ti);
}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(test_bges)
BOOST_DATA_TEST_CASE(test_bges,bdata::make(read_test_file("tests/test_bges.csv")),ti)
{
   check_win_sim(ti);
}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(test_whole_decks)
BOOST_DATA_TEST_CASE(test_whole_decks,bdata::make(read_test_file("tests/test_whole_decks.csv")),ti)
{
   check_win_sim(ti);
}
BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(test_crashes)
BOOST_AUTO_TEST_CASE(test_crashes)
{
    eps=1.; //only check for crashes now
    std::vector<std::vector<TestInfo>> aati;
    //aati.emplace_back(read_test_file("tests/test_whole_decks.csv"));
    //aati.emplace_back(read_test_file("tests/test_bges.csv"));
    //aati.emplace_back(read_test_file("tests/test_multi_units.csv"));
    aati.emplace_back(read_test_file("tests/test_crash.csv"));
    std::string decks="";
    for(auto t=aati.begin(); t!=aati.end(); ++t)
      for(auto tt=t->begin(); tt!=t->end(); ++tt)
          decks += tt->your_deck + ";" + tt->enemy_deck + ";";
    TestInfo ti;
    ti.your_deck=decks;
    ti.enemy_deck=decks;
    ti.bge="";
    check_win_sim(ti);
}
BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
#endif
#endif

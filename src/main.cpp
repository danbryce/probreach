//
// Created by fedor on 04/03/16.
//

#include <iostream>
#include <sstream>
#include <parser/csv/csvparser.h>
#include <iomanip>
#include "pdrh_config.h"
#include "model.h"
#include "algorithm.h"
#include "rnd.h"
extern "C"
{
    #include "pdrhparser.h"
}

#include "easylogging++.h"

extern "C" int yyparse();
extern "C" FILE *yyin;

INITIALIZE_EASYLOGGINGPP

int main(int argc, char* argv[])
{
    START_EASYLOGGINGPP(argc, argv);
    el::Logger* parser_logger = el::Loggers::getLogger("parser");
    el::Logger* algorithm_logger = el::Loggers::getLogger("algorithm");
    el::Logger* solver_logger = el::Loggers::getLogger("solver");
    el::Logger* series_parser_logger = el::Loggers::getLogger("series-parser");
    el::Logger* config_parser_logger = el::Loggers::getLogger("config");
    el::Logger* rng_logger = el::Loggers::getLogger("ran_gen");
    el::Logger* model_logger = el::Loggers::getLogger("model");

    // parse command line
    parse_pdrh_config(argc, argv);
    // setting precision on the output
    std::cout.precision(16);

    // PDRH PARSER
    CLOG_IF(global_config.verbose, INFO, "parser") << "Model file: " << global_config.model_filename;
    FILE *pdrhfile = fopen(global_config.model_filename.c_str(), "r");
    if (!pdrhfile)
    {
        CLOG(ERROR, "parser") << "Couldn't open " << global_config.model_filename;
        return -1;
    }
    std::stringstream s, pdrhnameprep;
    pdrhnameprep << global_config.model_filename << ".preprocessed";
    s << "cpp -w -P " << global_config.model_filename << " > " << pdrhnameprep.str().c_str();
    int res = system(s.str().c_str());
    // cheking the result of system call
    if(res != 0)
    {
        CLOG(ERROR, "parser") << "Problem occured while preprocessing " << global_config.model_filename;
        exit(EXIT_FAILURE);
    }
    // parsing the preprocessed file
    FILE *pdrhfileprep = fopen(pdrhnameprep.str().c_str(), "r");
    // make sure it's valid:
    if (!pdrhfileprep)
    {
        CLOG(ERROR, "parser") << "Couldn't open " << pdrhnameprep.str();
        exit(EXIT_FAILURE);
    }
    // set lex to read from it instead of defaulting to STDIN:
    yyin = pdrhfileprep;
    // parse through the input until there is no more:
    do
    {
        yyparse();
    }
    while (!feof(yyin));
    std::remove(pdrhnameprep.str().c_str());
    CLOG_IF(global_config.verbose, INFO, "parser") << "OK";
    //cout << pdrh::model_to_string() << endl;
    switch(pdrh::model_type)
    {
        // hybrid automata
        case pdrh::HA:
        {
            decision_procedure::result res = algorithm::evaluate_ha(global_config.reach_depth_min, global_config.reach_depth_max);
            if (res == decision_procedure::SAT)
            {
                std::cout << "sat" << std::endl;
            }
            else if (res == decision_procedure::UNDET)
            {
                std::cout << "undet" << std::endl;
            }
            else if (res == decision_procedure::UNSAT)
            {
                std::cout << "unsat" << std::endl;
            }
            else if (res == decision_procedure::ERROR)
            {
                std::cout << "error" << std::endl;
                return EXIT_FAILURE;
            }
            break;
        }
        // probabilistic hybrid automata
        case pdrh::PHA:
        {
            // move this check to parser
            /*
            if(pdrh::par_map.size() > 0)
            {
                CLOG(ERROR, "algorithm") << "Found " << pdrh::par_map.size() << " nondeterministic parameters. Please specify correct model type";
                std::cout << "error" << std::endl;
                return EXIT_FAILURE;
            }
            */
            capd::interval probability;
            if(global_config.chernoff_flag)
            {
                probability = algorithm::evaluate_pha_chernoff(global_config.reach_depth_min, global_config.reach_depth_max, global_config.chernoff_acc, global_config.chernoff_conf);
            }
            else if(global_config.bayesian_flag)
            {
                probability = algorithm::evaluate_pha_bayesian(global_config.reach_depth_min, global_config.reach_depth_max, global_config.bayesian_acc, global_config.bayesian_conf);
            }
            else
            {
                probability = algorithm::evaluate_pha(global_config.reach_depth_min, global_config.reach_depth_max);
            }
            // modifying the interval if outside (0,1)
            if(probability.leftBound() < 0)
            {
                probability.setLeftBound(0);
            }
            if(probability.rightBound() > 1)
            {
                probability.setRightBound(1);
            }
            std::cout << probability << " | " << capd::intervals::width(probability) << std::endl;
            break;
        }
        // nondeterministic probabilistic hybrid automata
        case pdrh::NPHA:
        {
            if(global_config.sobol_flag)
            {
                pair<box, capd::interval> probability = algorithm::evaluate_npha_sobol(global_config.reach_depth_min,
                                                                                       global_config.reach_depth_max,
                                                                                       global_config.sample_size);
                std::cout << probability.first << " : " << probability.second << " | " << capd::intervals::width(probability.second) << std::endl;
            }
            else if(global_config.cross_entropy_flag)
            {
                pair<box, capd::interval> probability = algorithm::evaluate_npha_cross_entropy(global_config.reach_depth_min,
                                                                                               global_config.reach_depth_max,
                                                                                               global_config.sample_size);
                std::cout << probability.first << " : " << probability.second << " | " << capd::intervals::width(probability.second) << std::endl;
            }
            else
            {
                std::map<box, capd::interval> probability_map = algorithm::evaluate_npha(global_config.reach_depth_min, global_config.reach_depth_max);
                for(auto it = probability_map.cbegin(); it != probability_map.cend(); it++)
                {
                    std::cout << std::scientific << it->first << " | " << it->second << std::endl;
                }
            }
            break;
        }
        // parameter synthesis
        case pdrh::PSY:
        {
            //CLOG(ERROR, "algorithm") << "Parameter synthesis is currently not supported";
            //exit(EXIT_FAILURE);
            if(global_config.series_filename.empty())
            {
                CLOG(ERROR, "series-parser") << "Time series file is not specified";
                return EXIT_FAILURE;
            }
            map<string, vector<pair<pdrh::node*, pdrh::node*>>> time_series = csvparser::parse(global_config.series_filename);
            tuple<vector<box>, vector<box>, vector<box>> boxes = algorithm::evaluate_psy(time_series);
            vector<box> sat_boxes = get<0>(boxes);
            vector<box> undet_boxes = get<1>(boxes);
            vector<box> unsat_boxes = get<2>(boxes);
            cout << "sat" << endl;
            for(box b : sat_boxes)
            {
                cout << b << endl;
            }
            cout << "undet" << endl;
            for(box b : undet_boxes)
            {
                cout << b << endl;
            }
            cout << "unsat" << endl;
            for(box b : unsat_boxes)
            {
                cout << b << endl;
            }
            break;
        }
        case pdrh::NHA:
        {
            CLOG(ERROR, "algorithm") << "Nondeterministic hybrid automata is currently not supported";
            exit(EXIT_FAILURE);
            //break;
        }
    }
    // unregister the loggers
    el::Loggers::unregisterLogger("parser");
    el::Loggers::unregisterLogger("algorithm");
    el::Loggers::unregisterLogger("solver");
    el::Loggers::unregisterLogger("series-parser");
    el::Loggers::unregisterLogger("config");
    el::Loggers::unregisterLogger("ran_gen");
    el::Loggers::unregisterLogger("model");
    return EXIT_SUCCESS;
}
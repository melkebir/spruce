/*
 * enum.cpp
 *
 *  Created on: 29-sep-2015
 *      Author: M. El-Kebir
 */

#include <lemon/arg_parser.h>
#include <iostream>
#include <fstream>
#include "utils.h"
#include "config.h"
#include "rootedcladisticancestrygraph.h"
#include "rootedcladisticnoisyancestrygraph.h"
#include "rootedcladisticenumeration.h"
#include "rootedcladisticnoisyenumeration.h"
#include "cnaenumerate.h"
#include "noisycnaenumerate.h"
#include "character.h"
#include "charactermatrix.h"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

using namespace gm;

typedef RootedCladisticAncestryGraph::StateTreeVector StateTreeVector;

bool parse_purity_vector(const std::string& purityString,
                         int m,
                         StlDoubleVector& purityValues)
{
  // parse purity vector
  purityValues = StlDoubleVector(m, 0);
  StringVector s;
  boost::split(s, purityString, boost::is_any_of(" "));
  if (s.size() != m)
  {
    std::cerr << "Invalid purity vector size. Expected " << m << " values, got " << s.size() << std::endl;
    return false;
  }
  
  for (int p = 0; p < m; ++p)
  {
    purityValues[p] = boost::lexical_cast<double>(s[p]);
  }
  return true;
}

void read_cladistic_tensor(std::ifstream& inFile,
                           RealTensor& F,
                           StateTreeVector& S)
{
  g_lineNumber = 0;
  
  inFile >> F;
  F.setLabels(inFile);
  std::string line;
  gm::getline(inFile, line);
  
  S = StateTreeVector(F.n(), StateTree(F.k()));
  
  for (int c = 0; c < F.n(); ++c)
  {
    try
    {
      inFile >> S[c];
    }
    catch (std::runtime_error& e)
    {
      std::cerr << "State tree file. " << getLineNumber() << e.what() << std::endl;
      exit(1);
    }
  }
}

void read_cladistic_tensor_noisy(std::ifstream& inFile,
                                 RealTensor& F,
                                 StateTreeVector& S,
                                 RealTensor& F_lb,
                                 RealTensor& F_ub)
{
  read_cladistic_tensor(inFile, F, S);
  
  std::string line;
  gm::getline(inFile, line);
  inFile >> F_lb;
  gm::getline(inFile, line);
  inFile >> F_ub;
}

void enumerate(int limit,
               int timeLimit,
               int threads,
               int state_tree_limit,
               std::ifstream& inFile,
               const std::string& intervalFile,
               int lowerbound,
               bool monoclonal,
               const std::string& purityString,
               bool writeCliqueFile,
               bool readCliqueFile,
               const std::string& cliqueFile,
               int offset,
               const IntSet& whiteList,
               const std::string& logFilename,
               int logInterval,
               SolutionSet& sols)
{
  CharacterMatrix M;
  
  try
  {
    g_lineNumber = 0;
    inFile >> M;
  }
  catch (std::runtime_error& e)
  {
    std::cerr << "Character matrix file. " << e.what() << std::endl;
    exit(1);
  }
  
  if (!intervalFile.empty())
  {
    std::ifstream inFile2(intervalFile.c_str());
    if (inFile2.good())
    {
      try
      {
        M.setIntervals(inFile2);
      }
      catch (std::runtime_error& e)
      {
        std::cerr << "Interval file. " << e.what() << std::endl;
        exit(1);
      }
    }
  }
  
  StlDoubleVector purityValues;
  if (purityString != "" && !parse_purity_vector(purityString, M.m(), purityValues))
  {
    return;
  }
//  if (monoclonal && purityString == "")
//  {
//    std::cerr << "Expected purity values" << std::endl;
//    return;
//  }
  
  if (g_verbosity >= VERBOSE_ESSENTIAL)
  {
    std::cerr << "Intializing copy-state matrix ..." << std::endl;
  }
  M.init();
  
  if (monoclonal)
  {
    M.applyHeuristic();
  }
  
  CompatibilityGraph* pComp = NULL;
  if (!readCliqueFile)
  {
    if (g_verbosity >= VERBOSE_ESSENTIAL)
    {
      std::cerr << std::endl << "Initializing compatibility graph ..." << std::endl;
    }
    pComp = new CompatibilityGraph(M);
    pComp->init(IntPairSet(), -1);
  }
  else
  {
    std::ifstream inFile(cliqueFile);
    pComp = new CompatibilityGraph(M);
    pComp->init(inFile);
  }
  
  if (writeCliqueFile)
  {
    std::ofstream outFile(cliqueFile);
    if (outFile.good())
    {
      pComp->write(outFile);
    }
  }
  
  NoisyCnaEnumerate alg(M, purityValues, *pComp, lowerbound);
  alg.init(state_tree_limit);
  alg.enumerate(limit, timeLimit, threads,
                state_tree_limit, monoclonal, offset, whiteList,
                logFilename, logInterval);

  sols = alg.sols();
  delete pComp;
  
  if (g_verbosity >= VERBOSE_ESSENTIAL)
  {
    std::cerr << "Generated " << sols.solutionCount() << " solutions" << std::endl;
  }
  inFile.close();
}

int main(int argc, char** argv)
{
  int limit = -1;
  int timeLimit = -1;
  int threads = 2;
  int state_tree_limit = -1;
  int random_seed = 0;
  int lowerbound = 0;
  bool polyclonal = false;
  bool perfectData = false;
  std::string purityString;
  std::string cliqueFile;
  int offset = 0;
  int verbosityLevel = 1;
  std::string whiteListString;
  std::string logFilename;
  int logInterval = 30;
  
  lemon::ArgParser ap(argc, argv);
  ap.boolOption("-version", "Show version number")
    .refOption("v", "Verbosity level (default: 1)", verbosityLevel)
    .boolOption("cladistic", "Cladistic character mode")
    .refOption("p", "Polyclonal", polyclonal)
    .refOption("purity", "Purity values (used for fixing trunk)",  purityString)
    .refOption("clique", "Clique file", cliqueFile)
    .refOption("perfect", "Perfect data mode", perfectData)
    .refOption("t", "Number of threads (default: 2)", threads)
    .refOption("l", "Maximum number of trees to enumerate (default: -1)", limit)
    .refOption("ll", "Time limit in seconds (default: -1)", timeLimit)
    .refOption("s", "Number of cliques to consider (default: -1)", state_tree_limit)
    .refOption("o", "Clique offset (default: 0)", offset)
    .refOption("r", "Seed for random number generator", random_seed)
    .refOption("lb", "Lower bound on #characters in enumerated trees (default: 0)", lowerbound)
    .refOption("w", "Characters that must be present in the solution trees", whiteListString)
    .refOption("logFile", "File for storing benchmarking information", logFilename)
    .refOption("logInterval", "Logging interval in seconds (default: 30)", logInterval)
    .other("input_1", "Input file")
    .other("input_2", "Interval file relating SNVs affected by the same CNA");
  ap.parse();

  g_rng = std::mt19937(random_seed);
  g_verbosity = static_cast<VerbosityLevel>(verbosityLevel);
  
  if (ap.given("-version"))
  {
    std::cout << "Version number: " << SPRUCE_VERSION << std::endl;
    return 0;
  }
  
  if (ap.files().size() == 0)
  {
    std::cerr << "Error: missing input file" << std::endl;
    return 1;
  }
  
  std::ifstream inFile(ap.files()[0].c_str());
  if (!inFile.good())
  {
    std::cerr << "Unable to open '" << ap.files()[0].c_str()
              << "' for reading" << std::endl;
    return 1;
  }
  
  IntSet whiteList;
  if (!whiteListString.empty())
  {
    StringVector s;
    boost::split(s, whiteListString, boost::is_any_of(", ;"));
    for (const std::string& str : s)
    {
      whiteList.insert(boost::lexical_cast<int>(str));
    }
  }
  
  bool readCliqueFile = false;
  bool writeCliqueFile = false;
  if (cliqueFile != "" && boost::filesystem::exists(cliqueFile))
  {
    readCliqueFile = true;
  }
  else if (cliqueFile != "")
  {
    writeCliqueFile = true;
  }
  
  SolutionSet sols;
  enumerate(limit, timeLimit, threads,
            state_tree_limit, inFile,
            (ap.files().size() > 1 ? ap.files()[1] : ""),
            lowerbound,
            !polyclonal,
            purityString,
            writeCliqueFile,
            readCliqueFile,
            cliqueFile,
            offset,
            whiteList,
            logFilename,
            logInterval,
            sols);
  std::cout << sols;
  
  return 0;
}

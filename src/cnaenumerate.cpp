/*
 * cnaenumerate.cpp
 *
 *  Created on: 19-oct-2015
 *      Author: M. El-Kebir
 */

#include <stdio.h>
#include "cnaenumerate.h"
#include "realtensor.h"
#include "rootedcladisticenumeration.h"

namespace gm {

CnaEnumerate::CnaEnumerate(const CharacterMatrix& M)
  : _M(M)
  , _sols()
  , _treeSize(-1)
  , _combinations(-1)
  , _real_n(-1)
{
}
  
void CnaEnumerate::init()
{
  const int m = _M.m();
  const int n = _M.n();
  assert(m > 0 && n > 0);

  _real_n = n;
  
  // S[c] = possible statetrees
  _combinations = 1;
  for (int c = 0; c < n; ++c)
  {
    int s = _M.numStateTrees(c);
    if (s == 0)
    {
      if (g_verbosity >= VERBOSE_ESSENTIAL)
      {
        std::cerr << "Tossing out character " << _M(0, c).characterLabel() << " (" << _M(0, c).characterIndex() << "), no compatible state trees across all samples" << std::endl;
      }
      --_real_n;
    }
    else
    {
      _combinations *= s;
    }
  }
}
  
void CnaEnumerate::enumerate(int limit,
                             int timeLimit,
                             int threads,
                             bool monoclonal,
                             const IntSet& whiteList,
                             const std::string& logFilename,
                             int logInterval)
{
  char buf[1024];
  const int n = _M.n();
  
  _sols.clear();
  _treeSize = -1;
  
  StlIntVector pi(n, 0);
  int count = 0;
  do {
    if (g_verbosity >= VERBOSE_ESSENTIAL)
    {
      std::cerr << std::endl << "State tree combination " << ++count << "/" << _combinations << " ..." << std::endl;
    }
    
    std::ofstream outLog;
    if (logInterval != 0)
    {
      snprintf(buf, 1024, "%s_S%d.log", logFilename.c_str(), count);
      outLog = std::ofstream(buf);
      if (!outLog.good())
      {
        std::cerr << "Error opening file '" << buf << "' for writing." << std::endl;
        exit(1);
      }
    }
    
    solve(pi, limit, timeLimit,
          threads, monoclonal, whiteList,
          outLog, logInterval);
//    for (int c = 0; c < n; ++c)
//    {
//      std::cout << pi[c] << " ";
//    }
//    std::cout << std::endl;
  } while (next(pi));
}
  
bool CnaEnumerate::next(StlIntVector& pi) const
{
  // find entry to increment
  int c = pi.size() - 1;
  for (; c >= 0; --c)
  {
    int s = _M.numStateTrees(c);
    if (s == 0) continue;
    assert(0 <= pi[c] && pi[c] < s);
    if (pi[c] == s - 1)
    {
      // continue
    }
    else
    {
      break;
    }
  }
  
  if (c == -1)
  {
    return false;
  }
  else
  {
    // increment and reset everything to the right
    ++pi[c];
    for (int d = c + 1; d < pi.size(); ++d)
    {
      pi[d] = 0;
    }
    return true;
  }
}
  
void CnaEnumerate::init(const StlIntVector& pi,
                        RealTensor& F,
                        StateTreeVector& S) const
{
  const int m = _M.m();
  assert(m > 0);
  const int n = _M.n();
  const int k = _M.k();
  
  // construct F and S
  F = RealTensor(k, m, _real_n);
  S.clear();
  for (int p = 0; p < m; ++p)
  {
    F.setRowLabel(p, _M(p, 0).sampleLabel());
  }
  
  char buf[1024];
  
  int real_c = 0;
  for (int c = 0; c < n; ++c)
  {
    int s = _M.numStateTrees(c);
    if (s == 0)
      continue;
    
    snprintf(buf, 1024, "%d", real_c);
    F.setColLabel(real_c, buf);
    S.push_back(_M.stateTree(pi[c], c));
    
    for (int i = 0; i < k; ++i)
    {
      for (int p = 0; p < m; ++p)
      {
        double f_p_ci = _M.get_f_lb(pi[c], p, c, i);
        F.set(i, p, real_c, f_p_ci);
      }
    }
    ++real_c;
  }
}
  
void CnaEnumerate::solve(const StlIntVector& pi,
                         int limit,
                         int timeLimit,
                         int threads,
                         bool monoclonal,
                         const IntSet& whiteList,
                         std::ostream& outLog,
                         int logInterval)
{
  // construct F and S
  RealTensor F;
  StateTreeVector S;
  init(pi, F, S);
  
  RootedCladisticAncestryGraph G(F, S);
  G.init();
  G.setLabels(F);
//  G.writeDOT(std::cout);
  
  RootedCladisticEnumeration enumerate(G,
                                       limit,
                                       timeLimit,
                                       threads,
                                       _treeSize,
                                       monoclonal,
                                       false,
                                       whiteList,
                                       outLog, logInterval);
  enumerate.run();
  
  if (g_verbosity >= VERBOSE_ESSENTIAL)
  {
    if (g_verbosity >= VERBOSE_NON_ESSENTIAL)
    {
      std::cerr << std::endl;
    }
    std::cerr << "Tree size: " << enumerate.objectiveValue() << "/" << _treeSize
              << " (" << enumerate.solutionCount() << " trees)" << std::endl;
  }
  
  if (enumerate.objectiveValue() >= _treeSize)
  {
    if (enumerate.objectiveValue() > _treeSize)
    {
      _sols.clear();
      _treeSize = enumerate.objectiveValue();
    }
    enumerate.populateSolutionSet(_sols);
  }
}
  
void CnaEnumerate::get(int combination,
                       RealTensor& F,
                       StateTreeVector& S) const
{
  assert(0 <= combination && combination < _combinations);
  
  const int n = _M.n();
  
  StlIntVector pi(n, 0);
  for (int count = 0; count < combination; ++count)
  {
    next(pi);
  }
  
  init(pi, F, S);
}
  
} // namespace gm

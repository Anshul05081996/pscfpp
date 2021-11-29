#ifndef PSCF_SWEEP_TMPL_TPP
#define PSCF_SWEEP_TMPL_TPP

/*
* PSCF - Polymer Self-Consistent Field Theory
*
* Copyright 2016 - 2019, The Regents of the University of Minnesota
* Distributed under the terms of the GNU General Public License.
*/

#include "SweepTmpl.h"
#include <util/misc/Log.h>

namespace Pscf {

   using namespace Util;

   /*
   * Constructor
   */
   template <class State>
   SweepTmpl<State>::SweepTmpl(int historyCapacity)
    : ns_(0),
      baseFileName_(),
      historyCapacity_(historyCapacity)
   {
      setClassName("SweepTmpl"); 
      states_.allocate(historyCapacity_);
      stateHistory_.allocate(historyCapacity_);
      sHistory_.allocate(historyCapacity_);
      c_.allocate(historyCapacity_);
   }

   /*
   * Destructor.
   */
   template <class State>
   SweepTmpl<State>::~SweepTmpl()
   {}

   /*
   * Read parameters.
   */
   template <class State>
   void SweepTmpl<State>::readParameters(std::istream& in)
   {
      read<int>(in, "ns", ns_);
      read<std::string>(in, "baseFileName", baseFileName_);
   }

   template <class State>
   void SweepTmpl<State>::sweep()
   {

      // Compute and output ds
      double ds = 1.0/double(ns_);
      double ds0 = ds;
      Log::file() << std::endl;
      Log::file() << "ns = " << ns_ << std::endl;
      Log::file() << "ds = " << ds  << std::endl;

      // Initial setup, before a sweep
      setup();
      Log::file() << "Checkpoint 0 (exited setup before sweep) \n";

      // Solve for initial state of sweep
      int error;
      double sNew = 0.0;
      Log::file() << std::endl;
      Log::file() << "Attempt s = " << sNew << std::endl;
      bool isContinuation = false; // False on first step
      error = solve(isContinuation);
      
      if (error) {
         UTIL_THROW("Failure to converge initial state of sweep");
      } else {
         Log::file() << "Checkpoint 1 (accepting initial solution) \n";
         accept(sNew);
      }

      Log::file() << "Checkpoint 2 (begin sweep loop) \n";
      // Loop over states on path
      bool finished = false;   // Are we finished with the loop?
      while (!finished) {
         error = 1;
         while (error) {

            sNew = s(0) + ds; 
            Log::file() << std::endl;
            Log::file() << "Attempt s = " << sNew << std::endl;
            Log::file() << "Checkpoint 3 (entering setParameters) \n";

            // Set non-adjustable system parameters to new values
            setParameters(sNew);

            // Set a guess for all state variables by polynomial extrapolation.
            extrapolate(sNew);
            Log::file() << "Checkpoint 4 (exited extrapolate) \n";

            // Attempt iterative SCFT solution
            isContinuation = true;
            error = solve(isContinuation);
            Log::file() << "Checkpoint 5 (exited solve) \n";

            if (error) {

               // Upon failure, reset state to last converged solution
               reset();

               // Decrease ds by half
               ds *= 0.50;
               if (ds < 0.1*ds0) {
                  UTIL_THROW("Step size too small in sweep");
               }

            } else {

               // Upon successful convergence, update history and nAccept
               accept(sNew);
               Log::file() << "Checkpoint 6 (accepted solution) \n";

            }
         }
         if (sNew + ds > 1.0000001) {
            finished = true;
         }
      }

      // Clean up after end of sweep
      cleanup();

   }

   /*
   * Initialize history variables (must be called by setup function).
   */
   template <class State>
   void SweepTmpl<State>::initialize()
   {
      UTIL_CHECK(historyCapacity_  > 1);
      UTIL_CHECK(states_.capacity() == historyCapacity_);
      UTIL_CHECK(sHistory_.capacity() == historyCapacity_);
      UTIL_CHECK(stateHistory_.capacity() == historyCapacity_);

      // Check allocation of all states, allocate if necessary
      for (int i = 0; i < historyCapacity_; ++i) {
         checkAllocation(states_[i]);
      }

      // Set pointers in stateHistory_ to refer to objects in states_
      nAccept_ = 0;
      historySize_ = 0;
      for (int i = 0; i < historyCapacity_; ++i) {
         sHistory_[i] = 0.0;
         stateHistory_[i] = &states_[i];
      }
   }

   template <class State>
   void SweepTmpl<State>::accept(double sNew)
   {
      // Shift elements of sHistory_
      for (int i = historyCapacity_ - 1; i > 0; --i) {
         sHistory_[i] = sHistory_[i-1];
      }
      sHistory_[0] = sNew;
      // Shift elements of stateHistory_ (pointers to stored solutions)
      State* temp;
      temp = stateHistory_[historyCapacity_-1];
      for (int i = historyCapacity_ - 1; i > 0; --i) {
         stateHistory_[i] = stateHistory_[i-1];
      }
      stateHistory_[0] = temp;
      // Update counters
      ++nAccept_;
      if (historySize_ < historyCapacity_) {
         ++historySize_;
      }
      // Call getSolution to copy system state to state(0).
      Log::file() << "Checkpoint 1.05! \n";
      getSolution();
   }

   /*
   * Use Lagrange polynomials to compute coefficients for continuation.
   */
   template <class State>
   void SweepTmpl<State>::setCoefficients(double sNew)
   {
      UTIL_CHECK(historySize_ <= historyCapacity_);
      if (historySize_ == 1) {
         c_[0] = 1.0;
      } else {
         double num, den;
         int i, j;
         for (i = 0; i < historySize_; ++i) {
            num = 1.0;
            den = 1.0;
            for (j = 0; j < historySize_; ++j) {
               if (j != i) {
                  num *= (sNew - s(j));
                  den *= (s(i) - s(j));
               }
            }
            c_[i] = num/den;
         }
      }
   }

   template <class State>
   void SweepTmpl<State>::cleanup()
   {}

} // namespace Pscf
#endif

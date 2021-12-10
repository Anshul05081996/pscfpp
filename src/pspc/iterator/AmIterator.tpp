#ifndef PSPC_AM_ITERATOR_TPP
#define PSPC_AM_ITERATOR_TPP

/*
* PSCF - Polymer Self-Consistent Field Theory
*
* Copyright 2016 - 2019, The Regents of the University of Minnesota
* Distributed under the terms of the GNU General Public License.
*/

#include "AmIterator.h"
#include <pspc/System.h>
#include <pscf/inter/ChiInteraction.h>
#include <util/containers/FArray.h>
#include <util/format/Dbl.h>
#include <util/misc/Timer.h>
#include <cmath>

namespace Pscf {
namespace Pspc
{

   using namespace Util;

   /*
   * Constructor
   */
   template <int D>
   AmIterator<D>::AmIterator(System<D>& system)
    : Iterator<D>(system),
      epsilon_(0),
      lambda_(0),
      nHist_(0),
      maxHist_(0)
   {  setClassName("AmIterator"); }

   /*
   * Destructor
   */
   template <int D>
   AmIterator<D>::~AmIterator()
   {}

   /*
   * Read parameter file block.
   */
   template <int D>
   void AmIterator<D>::readParameters(std::istream& in)
   {
      isFlexible_ = 0; // default value (fixed cell)
      read(in, "maxItr", maxItr_);
      read(in, "epsilon", epsilon_);
      read(in, "maxHist", maxHist_);
      readOptional(in, "isFlexible", isFlexible_);
   }

   /*
   * Setup and allocate memory required by iterator.
   */
   template <int D>
   void AmIterator<D>::setup()
   {
      // Allocate ring buffers
      resHists_.allocate(maxHist_+1);
      wHists_.allocate(maxHist_+1);

      if (isFlexible_) {
         stressHists_.allocate(maxHist_+1);
         cellParamHists_.allocate(maxHist_+1);
      }

      // Allocate outer arrays used in iteration (number of monomers)
      int nMonomer = system().mixture().nMonomer();
      wArrays_.allocate(nMonomer);
      dArrays_.allocate(nMonomer);
      tempRes.allocate(nMonomer);

      // Determine number of basis functions (nBasis - 1 if canonical)
      if (isCanonical()) {
         shift_ = 1;
      } else {
         shift_ = 0;
      }
      
      // Allocate inner arrays with number of basis functions
      int nBasis = system().basis().nBasis();
      for (int i = 0; i < nMonomer; ++i) {
         wArrays_[i].allocate(nBasis);
         dArrays_[i].allocate(nBasis);
         tempRes[i].allocate(nBasis);
      }
   }

   /*
   * Solve iteratively.
   */
   template <int D>
   int AmIterator<D>::solve()
   {
      // Preconditions:
      UTIL_CHECK(system().hasWFields());
      // Assumes basis.makeBasis() has been called
      // Assumes AmIterator.allocate() has been called
      // TODO: Check these conditions on entry

      Timer convertTimer;
      Timer solverTimer;
      Timer stressTimer;
      Timer updateTimer;
      Timer::TimePoint now;
      bool done;

      FieldIo<D>& fieldIo = system().fieldIo();

      // Solve MDE for initial state
      solverTimer.start();
      system().compute();
      now = Timer::now();
      solverTimer.stop(now);

      // Compute initial stress if needed
      if (isFlexible_) {
         stressTimer.start(now);
         system().mixture().computeStress();
         now = Timer::now();
         stressTimer.stop(now);
      }

      // Iterative loop
      for (int itr = 1; itr <= maxItr_; ++itr) {

         updateTimer.start(now);

         Log::file()<<"---------------------"<<std::endl;
         Log::file()<<" Iteration  "<<itr<<std::endl;

         if (itr <= maxHist_) {
            lambda_ = 1.0 - pow(0.9, itr);
            nHist_ = itr-1;
         } else {
            lambda_ = 1.0;
            nHist_ = maxHist_;
         }
         computeResidual();

         // Test for convergence
         done = isConverged();

         if (done) {

            updateTimer.stop();
            Log::file() << "----------CONVERGED----------"<< std::endl;

            // Output timing results
            double solverTime = solverTimer.time();
            double convertTime = convertTimer.time();
            double updateTime = updateTimer.time();
            double totalTime = updateTime + convertTime + solverTime;
            double stressTime = 0.0;
            if (isFlexible_) {
               stressTime = stressTimer.time();
               totalTime += stressTime;
            }
            Log::file() << "\n";
            Log::file() << "Iterator times contributions:\n";
            Log::file() << "\n";
            Log::file() << "solver time  = " << solverTime  << " s,  "
                        << solverTime/totalTime << "\n";
            if (isFlexible_) {
               Log::file() << "stress time  = " << stressTime  << " s,  "
                           << stressTime/totalTime << "\n";
            }
            Log::file() << "convert time = " << convertTime << " s,  "
                        << convertTime/totalTime << "\n";
            Log::file() << "update time  = "  << updateTime  << " s,  "
                        << updateTime/totalTime << "\n";
            Log::file() << "total time   = "  << totalTime   << " s  ";
            Log::file() << "\n\n";

            // If the unit cell is rigid, compute and output final stress 
            if (!isFlexible_) {
               system().mixture().computeStress();
               Log::file() << "Final stress:" << "\n";
               for (int m=0; m < system().unitCell().nParameter(); ++m){
                  Log::file() << "Stress  "<< m << "   = "
                              << Dbl(system().mixture().stress(m)) 
                              << "\n";
               }
               Log::file() << "\n";
            }

            // Successful completion (i.e., converged within tolerance)
            cleanUp();
            return 0;

         } else {
            if (itr <= maxHist_ + 1) {
               if (nHist_ > 0) {
                  invertMatrix_.allocate(nHist_, nHist_);
                  coeffs_.allocate(nHist_);
                  vM_.allocate(nHist_);
               }
            }
            minimizeCoeff(itr);
            buildOmega(itr);

            if (itr <= maxHist_) {
               if (nHist_ > 0) {
                  invertMatrix_.deallocate();
                  coeffs_.deallocate();
                  vM_.deallocate();
               }
            }
            now = Timer::now();
            updateTimer.stop(now);

            // Convert wFields from Basis to RGrid
            convertTimer.start(now);
            fieldIo.convertBasisToRGrid(system().wFields(),
                                        system().wFieldsRGrid());
            now = Timer::now();
            convertTimer.stop(now);

            // Solve MDE
            solverTimer.start(now);
            system().compute();
            now = Timer::now();
            solverTimer.stop(now);

            // Compute stress if needed
            if (isFlexible_){
               stressTimer.start(now);
               system().mixture().computeStress();
               now = Timer::now();
               stressTimer.stop(now);
            }

         }

      }
      // Failure: iteration counter itr reached maxItr without converging
      cleanUp();
      return 1;
   }

   template <int D>
   void AmIterator<D>::computeResidual()
   {
      // Relevant quantities
      const int nMonomer = system().mixture().nMonomer();
      const int nParameter = system().unitCell().nParameter();
      const int nBasis = system().basis().nBasis();

      // Store current w field in history ringbuffer 
      wHists_.append(system().wFields());
      
      // If variable unit cell, store current unit cell parameters 
      if (isFlexible_) {
         cellParamHists_.append(system().unitCell().parameters());
      }

      // Initialize temporary residuals workspace 
      for (int i = 0 ; i < nMonomer; ++i) {
         for (int k = 0; k < nBasis; ++k) {
            tempRes[i][k] = 0;
         }
      }
      
      // Compute residual vector
      for (int i = 0; i < nMonomer; ++i) {
         for (int j = 0; j < nMonomer; ++j) {
            for (int k = shift_; k < nBasis; ++k) {
               tempRes[i][k] +=( (system().interaction().chi(i,j)*system().cField(j)[k])
                               - (system().interaction().idemp(i,j)*system().wField(j)[k]) );
            }
         }
      }

      // Account for incompressibility in the grand-canonical or mixed case
      if (!isCanonical()) {
         for (int i = 0; i < nMonomer; ++i) {
            tempRes[i][0] -= 1/system().interaction().sum_inv();
         }
      }

      // Store residuals
      resHists_.append(tempRes);

      // If variable unit cell, store stress
      if (isFlexible_){
         FArray<double, 6 > tempCp;
         for (int i = 0; i < nParameter ; i++){
            tempCp [i] = -((system().mixture()).stress(i));
         }
         stressHists_.append(tempCp);
      }
   }

   template <int D>
   bool AmIterator<D>::isConverged()
   {
      const int nMonomer = system().mixture().nMonomer();
      const int nParameter = system().unitCell().nParameter();
      const int nBasis = system().basis().nBasis();

      double error;

      #if 0
      // Error as defined in Matsen's Papers
      double dError = 0;
      double wError = 0;
      for ( int i = 0; i < nMonomer; i++) {
         for ( int j = shift_; j < nBasis; j++) {
            dError += resHists_[0][i][j] * resHists_[0][i][j];

            //the extra shift is due to the zero indice coefficient being
            //exactly known
            wError += system().wField(i)[j+1] * system().wField(i)[j+1];
         }
      }

      if (isFlexible_){
         for ( int i = 0; i < nParameter ; i++) {
            dError += stressHists_[0][i] *  stressHists_[0][i];
            wError += system().unitCell().parameters()[i]
                     *system().unitCell().parameters()[i];
         }
      }
      Log::file() << " dError :" << Dbl(dError)<<std::endl;
      Log::file() << " wError :" << Dbl(wError)<<std::endl;
      error = sqrt(dError / wError);
      #endif

      // Error by Max Residuals
      double temp1 = 0;
      double temp2 = 0;
      for ( int i = 0; i < nMonomer; i++) {
         for ( int j = shift_; j < nBasis; j++) {
            if (temp1 < fabs (resHists_[0][i][j]))
                temp1 = fabs (resHists_[0][i][j]);
         }
      }
      Log::file() << "SCF Error   = " << Dbl(temp1) << std::endl;
      error = temp1;

      if (isFlexible_){
         for ( int i = 0; i < nParameter ; i++) {
            if (temp2 < fabs (stressHists_[0][i])) {
                temp2 = fabs (stressHists_[0][i]);
            }
         }
         // Output current stress values
         for (int m=0;  m < nParameter ; ++m){
            Log::file() << "Stress  "<< m << "   = "
                        << Dbl(system().mixture().stress(m)) <<"\n";
         }
         error = (temp1>(100*temp2)) ? temp1 : (100*temp2);
         // 100 is chose as stress rescale factor
         // TODO: Separate SCF and stress tolerance limits
      }
      Log::file() << "Error       = " << Dbl(error) << std::endl;

      // Output current unit cell parameter values
      if (isFlexible_){
         for (int m=0; m < nParameter ; ++m){
               Log::file() << "Parameter " << m << " = "
                           << Dbl(system().unitCell().parameters()[m])
                           << "\n";
         }
      }

      // Check if total error is below tolerance
      if (error < epsilon_) {
         return true;
      } else {
         return false;
      }
   }

   template <int D>
   void AmIterator<D>::minimizeCoeff(int itr)
   {
      const int nMonomer = system().mixture().nMonomer();
      const int nParameter = system().unitCell().nParameter();
      const int nBasis = system().basis().nBasis();

      if (itr == 1) {
         //do nothing
      } else {
         
         double elm, elm_cp;

         for (int i = 0; i < nHist_; ++i) {
            for (int j = i; j < nHist_; ++j) {

               invertMatrix_(i,j) = 0;
               for (int k = 0; k < nMonomer; ++k) {
                  elm = 0;
                  for (int l = shift_; l < nBasis; ++l) {
                     elm +=
                            ((resHists_[0][k][l] - resHists_[i+1][k][l])*
                             (resHists_[0][k][l] - resHists_[j+1][k][l]));
                  }
                  invertMatrix_(i,j) += elm;
               }

               if (isFlexible_){
                  elm_cp = 0;
                  for (int m = 0; m < nParameter ; ++m){
                     elm_cp += ((stressHists_[0][m] - stressHists_[i+1][m])*
                                (stressHists_[0][m] - stressHists_[j+1][m]));
                  }
                  invertMatrix_(i,j) += elm_cp;
               }
               invertMatrix_(j,i) = invertMatrix_(i,j);
            }

            vM_[i] = 0;
            for (int j = 0; j < nMonomer; ++j) {
               for (int k = shift_; k < nBasis; ++k) {
                  vM_[i] += ( (resHists_[0][j][k] - resHists_[i+1][j][k]) *
                               resHists_[0][j][k] );
               }
            }

            if (isFlexible_){
               elm_cp = 0;
               for (int m = 0; m < nParameter ; ++m){
                  vM_[i] += ((stressHists_[0][m] - stressHists_[i+1][m]) *
                             (stressHists_[0][m]));
               }
            }
         }

         if (itr == 2) {
            coeffs_[0] = vM_[0] / invertMatrix_(0,0);
         } else {
            LuSolver solver;
            solver.allocate(nHist_);
            solver.computeLU(invertMatrix_);
            solver.solve(vM_, coeffs_);
         }
      }
   }

   template <int D>
   void AmIterator<D>::buildOmega(int itr)
   {
      const int nMonomer = system().mixture().nMonomer();
      const int nParameter = system().unitCell().nParameter();
      const int nBasis = system().basis().nBasis();

      if (itr == 1) {
         for (int i = 0; i < nMonomer; ++i) {
            for (int k = shift_; k < nBasis; ++k) {
               system().wField(i)[k]
                      = wHists_[0][i][k] + lambda_*resHists_[0][i][k];
            }
         }

         if (isFlexible_){
            parameters_.clear();
            for (int m = 0; m < nParameter ; ++m){
               parameters_.append(cellParamHists_[0][m]
                              + lambda_* stressHists_[0][m]);

            }
            system().setUnitCell(parameters_);
         }

      } else {
         for (int j = 0; j < nMonomer; ++j) {
            for (int k = shift_; k < nBasis; ++k) {
               wArrays_[j][k] = wHists_[0][j][k];
               dArrays_[j][k] = resHists_[0][j][k];
            }
         }
         for (int i = 0; i < nHist_; ++i) {
            for (int j = 0; j < nMonomer; ++j) {
               for (int k = shift_; k < nBasis; ++k) {
                  wArrays_[j][k] += coeffs_[i] * ( wHists_[i+1][j][k] -
                                                   wHists_[0][j][k] );
                  dArrays_[j][k] += coeffs_[i] * ( resHists_[i+1][j][k] -
                                                   resHists_[0][j][k] );
               }
            }
         }
         // set the homogeneous basis function values explicitly 
         // if system is canonical
         if (isCanonical()) {
            for (int i = 0; i < nMonomer; ++i) {
               dArrays_[i][0] = 0.0;
               wArrays_[i][0] = 0.0;
               for (int j = 0; j < nMonomer; ++j) {
                  wArrays_[i][0] += system().interaction().chi(i,j)*system().cField(j)[0];
               }
            }
         }
         for (int i = 0; i < nMonomer; ++i) {
            for (int k = 0; k < nBasis; ++k) {
               system().wField(i)[k] = wArrays_[i][k]
                                         + lambda_ * dArrays_[i][k];
            }
         }
         if (isFlexible_){
            for (int m = 0; m < nParameter ; ++m){
               wCpArrays_[m] = cellParamHists_[0][m];
               dCpArrays_[m] = stressHists_[0][m];
            }
            for (int i = 0; i < nHist_; ++i) {
               for (int m = 0; m < nParameter ; ++m) {
                  wCpArrays_[m] += coeffs_[i]*( cellParamHists_[i+1][m] -
                                                cellParamHists_[0][m]);
                  dCpArrays_[m] += coeffs_[i]*( stressHists_[i+1][m] -
                                                stressHists_[0][m]);
               }
            }
            parameters_.clear();
            for (int m = 0; m < nParameter ; ++m){
               parameters_.append(wCpArrays_[m] + lambda_ * dCpArrays_[m]);
            }
            system().setUnitCell(parameters_);
         }
      }
   }

   template <int D>
   bool AmIterator<D>::isCanonical()
   {
      // check ensemble of all polymers
      for (int i = 0; i < system().mixture().nPolymer(); ++i) {
         if (system().mixture().polymer(i).ensemble() == Species::Open) {
            return false;
         }
      }
      // check ensemble of all solvents
      for (int i = 0; i < system().mixture().nSolvent(); ++i) {
         if (system().mixture().solvent(i).ensemble() == Species::Open) {
            return false;
         }
      }
      // returns true if false was never returned
      return true;


   }

   template <int D>
   void AmIterator<D>::cleanUp()
   {
      // Deallocate allocated arrays and matrices, if allocated.
      if (invertMatrix_.isAllocated()) {
         invertMatrix_.deallocate();
      }
      if (coeffs_.isAllocated()) {
         coeffs_.deallocate();
      }
      if (vM_.isAllocated()) {
         vM_.deallocate();
      }

      
   }

}
}
#endif

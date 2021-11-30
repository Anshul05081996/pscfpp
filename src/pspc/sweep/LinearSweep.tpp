#ifndef PSPC_LINEAR_SWEEP_TPP
#define PSPC_LINEAR_SWEEP_TPP

/*
* PSCF - Polymer Self-Consistent Field Theory
*
* Copyright 2016 - 2019, The Regents of the University of Minnesota
* Distributed under the terms of the GNU General Public License.
*/

#include "LinearSweep.h"
#include <pspc/System.h>
#include <cstdio>

namespace Pscf {
namespace Pspc {

   using namespace Util;

   template <int D>
   LinearSweep<D>::LinearSweep(System<D>& system)
   : Sweep<D>(system)
   {}

   template <int D>
   void LinearSweep<D>::readParameters(std::istream& in)
   {
      // Call the base class's readParameters function.
      SweepTmpl< BasisFieldState<D> >::readParameters(in);
      
      // Read in the number of sweep parameters and allocate.
      this->read(in, "nParameter", nParameter_);
      parameters_.allocate(nParameter_);
      
      // Read in array of LinearSweepParameters, calling << for each
      this->template readDArray< LinearSweepParameter<D> >(in, "parameters", parameters_, nParameter_);

      // verify net zero change in volume fractions if being swept
      double sum = 0.0;
      for (int i = 0; i < nParameter_; ++i) {
         if (parameters_[i].type() == "phi") {
            sum += parameters_[i].change();
         }
      }
      UTIL_CHECK(sum > -0.0001);
      UTIL_CHECK(sum < 0.0001);
   }

   template <int D>
   void LinearSweep<D>::setup()
   {
      // Verify that the LinearSweep has a system pointer
      UTIL_CHECK(hasSystem());

      // Call base class's setup function
      Sweep<D>::setup();
      
      // Set system pointer and initial value for each parameter object
      for (int i = 0; i < nParameter_; ++i) {
         parameters_[i].setSystem(system());
         parameters_[i].getInitial();
      }
   }

   template <int D>
   void LinearSweep<D>::setParameters(double s)
   {
      // Update the system parameter values
      for (int i = 0; i < nParameter_; ++i) {
         parameters_[i].update(s);
      }
   }

   template <int D>
   void LinearSweep<D>::outputSummary(std::ostream& out)
   {}

}
}

#endif

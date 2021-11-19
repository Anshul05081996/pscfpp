#ifndef PSPC_K_FIELD_COMPARISON_TPP
#define PSPC_K_FIELD_COMPARISON_TPP

/*
* PSCF - Polymer Self-Consistent Field Theory
*
* Copyright 2016 - 2019, The Regents of the University of Minnesota
* Distributed under the terms of the GNU General Public License.
*/

#include "KFieldComparison.h"
#include <cmath>

namespace Pscf {
namespace Pspc {

   // Default Constructor
   template <int D>
   KFieldComparison<D>::KFieldComparison(int begin)
    : maxDiff_(0.0),
      rmsDiff_(0.0),
      begin_(begin)
   {};

   // Comparator for individual fields.
   template <int D>
   double KFieldComparison<D>::compare(RFieldDft<D> const& a, RFieldDft<D> const& b)
   {
      UTIL_CHECK(a.capacity() > 0);
      UTIL_CHECK(a.capacity() == b.capacity());
      int n = a.capacity();
      double diffSq, diff, d0, d1;
      maxDiff_ = 0.0;
      rmsDiff_ = 0.0;
      for (int i = begin_; i < n; ++i) {
         d0 = a[i][0] - b[i][0];
         d1 = a[i][1] - b[i][1];
         diffSq = d0*d0 + d1*d1;
         diff = sqrt(diffSq);
         if (diff > maxDiff_) {
            maxDiff_ = diff;
         }
         rmsDiff_ += diffSq;
         //std::cout << i
         //          << " " << a[i][0]  << " " << a[i][1]
         //          << " " << b[i][0]  << " " << b[i][1]
         //          << " " << diff << std::endl;
      }
      rmsDiff_ = rmsDiff_/double(n);
      rmsDiff_ = sqrt(rmsDiff_);
      return maxDiff_;
   }

   // Comparator for arrays of fields
   template <int D>
   double KFieldComparison<D>::compare(DArray< RFieldDft<D> > const & a,
                                       DArray< RFieldDft<D> > const & b)
   {
      UTIL_CHECK(a.capacity() > 0);
      UTIL_CHECK(a.capacity() == b.capacity());
      UTIL_CHECK(a[0].capacity() > 0);
      int m = a.capacity();
      double diffSq, diff, d0, d1;
      maxDiff_ = 0.0;
      rmsDiff_ = 0.0;
      int i, j, n;
      for (i = 0; i < m; ++i) {
         n = a[i].capacity();
         UTIL_CHECK(n > 0);
         UTIL_CHECK(n == b[i].capacity());
         for (j = begin_; j < n; ++j) {
            d0 = a[i][j][0] - b[i][j][0];
            d1 = a[i][j][1] - b[i][j][1];
            diffSq = d0*d0 + d1*d1;
            diff = sqrt(diffSq);
            //std::cout << i
            //          << "  (" << a[i][j][0]  << " ,  " << a[i][j][1]
            //          << ") (" << b[i][j][0]  << " ,  " << b[i][j][1]
            //          << ")  " << diff << std::endl;
            if (diff > maxDiff_) {
               maxDiff_ = diff;
            }
            rmsDiff_ += diffSq;
         }
      }
      rmsDiff_ = rmsDiff_/double(m*n);
      rmsDiff_ = sqrt(rmsDiff_);
      return maxDiff_;
   }

}
}
#endif

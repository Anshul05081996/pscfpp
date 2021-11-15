#ifndef PSPC_FFT_TEST_H
#define PSPC_FFT_TEST_H

#include <test/UnitTest.h>
#include <test/UnitTestRunner.h>

#include <pspc/field/FFT.h>
#include <pspc/field/RField.h>
#include <pspc/field/RFieldDft.h>
#include <pspc/field/RFieldComparison.h>

#include <util/math/Constants.h>
#include <util/format/Dbl.h>

using namespace Util;
using namespace Pscf::Pspc;

class FftTest : public UnitTest 
{
public:

   void setUp() {}
   void tearDown() {}

   void testConstructor();
   void testTransform1D();
   void testTransform2D();
   void testTransform3D();

};

void FftTest::testConstructor()
{
   printMethod(TEST_FUNC);
   {
      FFT<1> v;
      //TEST_ASSERT(v.capacity() == 0 );
      //TEST_ASSERT(!v.isAllocated() );
   }
} 

void FftTest::testTransform1D() {
   printMethod(TEST_FUNC);
   //printEndl();

   int n = 10;
   IntVec<1> d;
   d[0] = n;

   FFT<1> v;
   v.setup(d);

   RField<1> in;
   RFieldDft<1> out;
   in.allocate(d);
   out.allocate(d);

   // Initialize input data
   double x;
   double twoPi = 2.0*Constants::Pi;
   for (int i = 0; i < n; ++i) {
      x = twoPi*float(i)/float(n); 
      in[i] = cos(x);
   }

   v.forwardTransform(in, out);
   RField<1> inCopy;
   inCopy.allocate(d);
   v.inverseTransform(out, inCopy);

   for (int i = 0; i < n; ++i) {
      TEST_ASSERT(eq(in[i], inCopy[i]));
   }
}

void FftTest::testTransform2D() 
{
   printMethod(TEST_FUNC);
   //printEndl();

   int n1 = 3;
   int n2 = 3;
   IntVec<2> d;
   d[0] = n1;
   d[1] = n2;

   FFT<2> v;
   v.setup(d);

   RField<2> in;
   RFieldDft<2> out;
   in.allocate(d);
   out.allocate(d);

   TEST_ASSERT(eq(in.capacity() / in.meshDimensions()[1],
                  out.capacity() / (out.meshDimensions()[1]/2 + 1)));

   int rank = 0;
   for (int i = 0; i < n1; i++) {
      for (int j = 0; j < n2; j++) {
         rank = j + (i * n2);
         in[rank] = 1;
      }
   }

   v.forwardTransform(in, out);
   RField<2> inCopy;
   inCopy.allocate(d);
   v.inverseTransform(out, inCopy);

   for (int i = 0; i < n1; i++) {
      for (int j = 0; j < n2; j++) {
         rank = j + (i * n2);
         TEST_ASSERT(eq(in[rank], inCopy[rank]));
      }
   }
}

void FftTest::testTransform3D() {
   printMethod(TEST_FUNC);
   //printEndl();

   int n1 = 3;
   int n2 = 3;
   int n3 = 3;
   IntVec<3> d;
   d[0] = n1;
   d[1] = n2;
   d[2] = n3;

   FFT<3> v;
   v.setup(d);

   RField<3> in;
   RFieldDft<3> out;
   in.allocate(d);
   out.allocate(d);

   TEST_ASSERT(eq(in.capacity() / in.meshDimensions()[2],
                  out.capacity() / (out.meshDimensions()[2]/2 + 1)));

   int rank = 0;
   for (int i = 0; i < n1; i++) {
      for (int j = 0; j < n2; j++) {
         for (int k = 0; k < n3; k++){
            rank = k + ((j + (i * n2)) * n3);
            in[rank] = 1.0 + double(rank)/double(in.capacity());
         }
      }
   }

   v.forwardTransform(in, out);
   RField<3> inCopy;
   inCopy.allocate(d);
   v.inverseTransform(out, inCopy);

   for (int i = 0; i < n1; i++) {
      for (int j = 0; j < n2; j++) {
         for (int k = 0; k < n3; k++){
            rank = k + ((j + (i * n1)) * n3);
            TEST_ASSERT(eq(in[rank], inCopy[rank]));
         }
      }
   }

   RFieldComparison<3> comparison;
   comparison.compare(in, inCopy);
   //std::cout << std::endl;
   //std::cout << "maxDiff = " 
   //          << Dbl(comparison.maxDiff(), 20, 13)
   //          << std::endl;
   TEST_ASSERT(comparison.maxDiff() < 1.0E-12);

}

TEST_BEGIN(FftTest)
TEST_ADD(FftTest, testConstructor)
TEST_ADD(FftTest, testTransform1D)
TEST_ADD(FftTest, testTransform2D)
TEST_ADD(FftTest, testTransform3D)
TEST_END(FftTest)

#endif

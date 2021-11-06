System{
  Mixture{
    nMonomer  2
    monomers  0   A   1.0  
              1   B   1.0 
    nPolymer  1
    Polymer{
       nBlock  2
       nVertex 3
       blocks  0  0  0  1  0.3
               1  1  1  2  0.7
       phi     1.0
    }
    ds   0.01
  }
  ChiInteraction{
    chi  0   0   0.0
         1   0   20.0
         1   1   0.0
  }
  unitCell    hexagonal   1.6651597308
  mesh        32    32
  groupName   p_6_m_m
  AmIterator{
   maxItr 100
   epsilon 1e-10
   maxHist 15
   isFlexible 1
  }
}

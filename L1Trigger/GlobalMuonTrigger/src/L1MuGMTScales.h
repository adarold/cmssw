//-------------------------------------------------
//
//   \class L1MuGMTScales
//
/**   Description:  Class that creates all scales used inside the GMT
 *                  
 *
 *   An automatic check is done for the calo eta scales.
*/                  
//                
//   $Date: 2006/11/16 18:25:41 $
//   $Revision: 1.3 $ 
//
//   Author :
//   Hannes Sakulin      HEPHY / Vienna
//
//   Migrated to CMSSW:
//   I. Mikulec
//
//--------------------------------------------------
#ifndef DataFormatsL1GlobalMuonTrigger_L1MuGMTScales_h
#define DataFormatsL1GlobalMuonTrigger_L1MuGMTScales_h

#include <cmath>
#include <iostream>

#include "DataFormats/L1GlobalMuonTrigger/interface/L1MuScale.h"

#include "FWCore/MessageLogger/interface/MessageLogger.h"

class L1MuGMTScales {
 public:

  /// constructor
  L1MuGMTScales() {
    
    // used as a Singleton, the constructor is run when the GMT is first used.
    //
   

    const float reducedetabins[4][9] = {
      { 0.00, 0.22, 0.27, 0.58, 0.77, 0.87, 0.92, 1.24, 1.3 }, // DT
      { 0.00, 0.06, 0.25, 0.41, 0.54, 0.70, 0.83, 0.93, 1.04}, // bRPC
      { 0.9,  1.06, 1.26, 1.46, 1.66, 1.86, 2.06, 2.26, 2.5 }, // CSC
      { 1.04, 1.24, 1.36, 1.48, 1.61, 1.73, 1.85, 1.97, 2.10}  // fRPC
    };
    
    for (int i=0; i<4; i++) 
      m_ReducedEtaScale[i] = new L1MuSymmetricBinnedScale(4, 8, reducedetabins[i]);

  
    //
    // Scales used in Matching Units
    //
    float deta_unit = 0.04;
    float deta_min = -7.5 * deta_unit;
    float deta_max = 7.5 * deta_unit;
    m_DeltaEtaScale[0] = new L1MuBinnedScale (new L1MuSignedPacking<4>, 15, deta_min, deta_max, 7); // DT-RPC
    m_DeltaEtaScale[1] = new L1MuBinnedScale (new L1MuSignedPacking<4>, 15, deta_min, deta_max, 7); // CSC-RPC
    m_DeltaEtaScale[2] = new L1MuBinnedScale (new L1MuSignedPacking<4>, 15, deta_min, deta_max, 7); // DT-CSC
    m_DeltaEtaScale[3] = new L1MuBinnedScale (new L1MuSignedPacking<4>, 15, deta_min, deta_max, 7); // CSC-DT
    m_DeltaEtaScale[4] = new L1MuBinnedScale (new L1MuSignedPacking<4>, 15, deta_min, deta_max, 7); // CSC-bRPC
    m_DeltaEtaScale[5] = new L1MuBinnedScale (new L1MuSignedPacking<4>, 15, deta_min, deta_max, 7); // DT-fRPC

    // delta phi scale
    m_DeltaPhiScale = new L1MuBinnedScale (new L1MuSignedPacking<3>, 8, -11.25 * M_PI/180., 8.75 * M_PI/180.0, 4); 

    //
    // reduced eta scale for matching in the overlap region
    //
    // reserve highest code for Out Of Range
    // 
    const float RpcOvlEtaBins[8] = {
      0.72,  0.83,  0.93,  1.04,  1.14,  1.24,  1.36, 1.48 };  


    m_OvlEtaScale[0] = new L1MuSymmetricBinnedScale (4, 7, 1.3 * 18./32. , 1.3); // DT
    // FIXME **** dt scale: two original bins in one new bin
    // one-to-one mapping should be possible with new eta scale

    m_OvlEtaScale[1] = new L1MuSymmetricBinnedScale (4, 7, RpcOvlEtaBins) ; // bRPC
    m_OvlEtaScale[2] = new L1MuSymmetricBinnedScale (4, 7, 0.9, 1.25);    // CSC
    m_OvlEtaScale[3] = new L1MuSymmetricBinnedScale (4, 7, RpcOvlEtaBins) ; // fRPC

    //
    float caloEtaBounds[15] = { 
      -3.000, -2.172, -1.740, -1.392, -1.044, -0.696, -0.348, 0.,
      0.348,  0.696, 1.044,  1.392,  1.740,  2.172,  3.000 };
         
    m_CaloEtaScale =  new L1MuBinnedScale (new L1MuUnsignedPacking<4>, 14, caloEtaBounds);
  };

  /// destructor
  virtual ~L1MuGMTScales() {
    for (int i=0; i<4; i++) {
      delete m_ReducedEtaScale[i];
      delete m_OvlEtaScale[i];
    }
    for (int i=0; i<6; i++) {
      delete m_DeltaEtaScale[i];
    }

    delete m_DeltaPhiScale;
    delete m_CaloEtaScale;
  };
  
  /// get the recuced eta scale for matching in the overlap region (4 bit); isys = 0(DT), 1(bRPC), 2(CSC), 3(fwdRPC)
  L1MuScale* getReducedEtaScale(int isys) const { 
    if (isys<0 || isys>3) edm::LogWarning("GMTScaleRangeViolation") << "Error in L1MuGMTScales:: isys out of range: " << isys; 
    return m_ReducedEtaScale[isys]; 
  };

  /// get the delta eta scale; idx = 0(DT=RPC), 1(CSC-RPC), 2(DT-CSC), 3(CSC-DT), 4(bRPC-CSC), 5(fRPC-DT)
  L1MuScale* getDeltaEtaScale(int idx) const { 
    if (idx<0 || idx>5) edm::LogWarning("GMTScaleRangeViolation") << "Error in L1MuGMTScales:: isys out of range: " << idx; 
    return m_DeltaEtaScale[idx]; 
  };

  /// get the delta phi scale ( 3 bits)
  L1MuScale* getDeltaPhiScale() const {     return m_DeltaPhiScale;   };

  /// get the overlap eta scale (4 bits);  isys = 0(DT), 1(bRPC), 2(CSC), 3(fwdRPC)
  L1MuScale* getOvlEtaScale(int isys) const { 
    if (isys<0 || isys>3) edm::LogWarning("GMTScaleRangeViolation") << "Error in L1MuGMTScales:: isys out of range: " << isys; 
    return m_OvlEtaScale[isys]; 
  };


  /// get the calorimeter eta scale
  L1MuScale *getCaloEtaScale() const { return m_CaloEtaScale;}; 


 private:
  L1MuScale *m_ReducedEtaScale[4];
  L1MuScale *m_DeltaEtaScale[6];
  L1MuScale *m_DeltaPhiScale;
  L1MuScale *m_OvlEtaScale[4];
  L1MuScale *m_CaloEtaScale;
};


#endif










































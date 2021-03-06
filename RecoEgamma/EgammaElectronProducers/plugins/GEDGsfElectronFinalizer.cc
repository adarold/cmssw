#include "RecoEgamma/EgammaElectronProducers/plugins/GEDGsfElectronFinalizer.h"

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"

#include "DataFormats/EgammaCandidates/interface/GsfElectron.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"

#include <iostream>
#include <string>

using namespace reco;

GEDGsfElectronFinalizer::GEDGsfElectronFinalizer(const edm::ParameterSet& cfg) {
  previousGsfElectrons_ =
      consumes<reco::GsfElectronCollection>(cfg.getParameter<edm::InputTag>("previousGsfElectronsTag"));
  pfCandidates_ = consumes<reco::PFCandidateCollection>(cfg.getParameter<edm::InputTag>("pfCandidatesTag"));
  outputCollectionLabel_ = cfg.getParameter<std::string>("outputCollectionLabel");
  edm::ParameterSet pfIsoVals(cfg.getParameter<edm::ParameterSet>("pfIsolationValues"));

  tokenElectronIsoVals_.push_back(
      consumes<edm::ValueMap<float> >(pfIsoVals.getParameter<edm::InputTag>("pfSumChargedHadronPt")));
  tokenElectronIsoVals_.push_back(
      consumes<edm::ValueMap<float> >(pfIsoVals.getParameter<edm::InputTag>("pfSumPhotonEt")));
  tokenElectronIsoVals_.push_back(
      consumes<edm::ValueMap<float> >(pfIsoVals.getParameter<edm::InputTag>("pfSumNeutralHadronEt")));
  tokenElectronIsoVals_.push_back(consumes<edm::ValueMap<float> >(pfIsoVals.getParameter<edm::InputTag>("pfSumPUPt")));
  tokenElectronIsoVals_.push_back(
      consumes<edm::ValueMap<float> >(pfIsoVals.getParameter<edm::InputTag>("pfSumEcalClusterEt")));
  tokenElectronIsoVals_.push_back(
      consumes<edm::ValueMap<float> >(pfIsoVals.getParameter<edm::InputTag>("pfSumHcalClusterEt")));

  nDeps_ = tokenElectronIsoVals_.size();

  if (cfg.existsAs<edm::ParameterSet>("regressionConfig")) {
    const edm::ParameterSet& iconf = cfg.getParameterSet("regressionConfig");
    const std::string& mname = iconf.getParameter<std::string>("modifierName");
    auto cc = consumesCollector();
    gedRegression_ = ModifyObjectValueFactory::get()->create(mname, iconf, cc);
  }

  produces<reco::GsfElectronCollection>(outputCollectionLabel_);
}

GEDGsfElectronFinalizer::~GEDGsfElectronFinalizer() {}

// ------------ method called to produce the data  ------------
void GEDGsfElectronFinalizer::produce(edm::Event& event, const edm::EventSetup& setup) {
  // Output collection
  auto outputElectrons_p = std::make_unique<reco::GsfElectronCollection>();

  if (gedRegression_) {
    gedRegression_->setEvent(event);
    gedRegression_->setEventContent(setup);
  }

  // read input collections
  // electrons
  edm::Handle<reco::GsfElectronCollection> gedElectronHandle;
  event.getByToken(previousGsfElectrons_, gedElectronHandle);

  // PFCandidates
  edm::Handle<reco::PFCandidateCollection> pfCandidateHandle;
  event.getByToken(pfCandidates_, pfCandidateHandle);
  // value maps
  std::vector<edm::Handle<edm::ValueMap<float> > > isolationValueMaps(nDeps_);

  for (unsigned i = 0; i < nDeps_; ++i) {
    event.getByToken(tokenElectronIsoVals_[i], isolationValueMaps[i]);
  }

  // prepare a map of PFCandidates having a valid GsfTrackRef to save time
  std::map<reco::GsfTrackRef, const reco::PFCandidate*> gsfPFMap;
  reco::PFCandidateCollection::const_iterator it = pfCandidateHandle->begin();
  reco::PFCandidateCollection::const_iterator itend = pfCandidateHandle->end();
  for (; it != itend; ++it) {
    // First check that the GsfTrack is non null
    if (it->gsfTrackRef().isNonnull()) {
      if (abs(it->pdgId()) == 11)  // consider only the electrons
        gsfPFMap[it->gsfTrackRef()] = &(*it);
    }
  }

  // Now loop on the electrons
  unsigned nele = gedElectronHandle->size();
  for (unsigned iele = 0; iele < nele; ++iele) {
    reco::GsfElectronRef myElectronRef(gedElectronHandle, iele);

    reco::GsfElectron newElectron(*myElectronRef);
    reco::GsfElectron::PflowIsolationVariables isoVariables;
    isoVariables.sumChargedHadronPt = (*(isolationValueMaps)[0])[myElectronRef];
    isoVariables.sumPhotonEt = (*(isolationValueMaps)[1])[myElectronRef];
    isoVariables.sumNeutralHadronEt = (*(isolationValueMaps)[2])[myElectronRef];
    isoVariables.sumPUPt = (*(isolationValueMaps)[3])[myElectronRef];
    isoVariables.sumEcalClusterEt = (*(isolationValueMaps)[4])[myElectronRef];
    isoVariables.sumHcalClusterEt = (*(isolationValueMaps)[5])[myElectronRef];

    newElectron.setPfIsolationVariables(isoVariables);

    // now set a status if not already done (in GEDGsfElectronProducer.cc)
    //     std::cout << " previous status " << newElectron.mvaOutput().status << std::endl;
    if (newElectron.mvaOutput().status <= 0) {
      std::map<reco::GsfTrackRef, const reco::PFCandidate*>::const_iterator itcheck =
          gsfPFMap.find(newElectron.gsfTrack());
      reco::GsfElectron::MvaOutput myMvaOutput(newElectron.mvaOutput());
      if (itcheck != gsfPFMap.end()) {
        // it means that there is a PFCandidate with the same GsfTrack
        myMvaOutput.status = 3;  //as defined in PFCandidateEGammaExtra.h
        //this is currently fully redundant with mvaOutput.stats so candidate for removal
        newElectron.setPassPflowPreselection(true);
      } else {
        myMvaOutput.status = 4;  //
        //this is currently fully redundant with mvaOutput.stats so candidate for removal
        newElectron.setPassPflowPreselection(false);
      }
      newElectron.setMvaOutput(myMvaOutput);
    }

    if (gedRegression_) {
      gedRegression_->modifyObject(newElectron);
    }
    outputElectrons_p->push_back(newElectron);
  }

  event.put(std::move(outputElectrons_p), outputCollectionLabel_);
}

// -*- C++ -*-
//
// Package:    CNNFiltering/CNNInference
// Class:      CNNInference
//
/**\class CNNInference CNNInference.cc CNNFiltering/CNNInference/plugins/CNNInference.cc

Description: [one line class summary]

Implementation:
[Notes on implementation]
*/
//
// Original Author:  adrianodif
//         Created:  Tue, 30 Jan 2018 12:05:21 GMT
//
//


#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"

// system include files
#include <memory>
#include <vector>
#include <algorithm>

// user include files
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Utilities/interface/EDGetToken.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/StreamID.h"
#include "FWCore/Framework/interface/GetterOfProducts.h"
#include "FWCore/Framework/interface/ProcessMatch.h"
#include "FWCore/ServiceRegistry/interface/ServiceRegistry.h"
#include "FWCore/ServiceRegistry/interface/Service.h"

#include "CommonTools/UtilAlgos/interface/TFileService.h"

#include "SimTracker/TrackerHitAssociation/interface/ClusterTPAssociation.h"

#include "SimDataFormats/PileupSummaryInfo/interface/PileupSummaryInfo.h"

#include "RecoTracker/TkHitPairs/interface/HitPairGeneratorFromLayerPair.h"
#include "RecoTracker/TkHitPairs/interface/IntermediateHitDoublets.h"

#include "DataFormats/Provenance/interface/ProductID.h"
#include "DataFormats/Common/interface/Handle.h"
#include "DataFormats/TrackerRecHit2D/interface/SiPixelRecHit.h"
#include "DataFormats/TrackerRecHit2D/interface/BaseTrackerRecHit.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "DataFormats/VertexReco/interface/Vertex.h"

#include "RecoTracker/TkHitPairs/interface/RecHitsSortedInPhi.h"
#include "RecoTracker/TkHitPairs/interface/IntermediateHitDoublets.h"

#include <iostream>
#include <string>
#include <fstream>

#include "TH2F.h"
#include "TTree.h"
#include "DataFormats/DetId/interface/DetId.h"
#include "DataFormats/SiPixelDetId/interface/PXBDetId.h"
#include "DataFormats/SiPixelDetId/interface/PXFDetId.h"
#include "DataFormats/TrackerRecHit2D/interface/SiPixelRecHit.h"
#include "DataFormats/BeamSpot/interface/BeamSpot.h"
//
// class declaration
//

#include "PhysicsTools/TensorFlow/interface/TensorFlow.h"

// If the analyzer does not use TFileService, please remove
// the template argument to the base class so the class inherits
// from  edm::one::EDAnalyzer<> and also remove the line from
// constructor "usesResource("TFileService");"
// This will improve performance in multithreaded jobs.

class CNNInference : public edm::one::EDAnalyzer<edm::one::SharedResources>  {
public:
  explicit CNNInference(const edm::ParameterSet&);
  ~CNNInference();

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);


private:
  virtual void beginJob() override;
  virtual void analyze(const edm::Event&, const edm::EventSetup&) override;
  virtual void endJob() override;
  int particleBit();

  // ----------member data ---------------------------

  int doubletSize;
  std::string processName_;
  edm::EDGetTokenT<IntermediateHitDoublets> intHitDoublets_;
  edm::EDGetTokenT<ClusterTPAssociation> tpMap_;
  edm::EDGetTokenT<reco::BeamSpot>  bsSrc_;
  edm::EDGetTokenT<std::vector<PileupSummaryInfo>>  infoPileUp_;
  // edm::GetterOfProducts<IntermediateHitDoublets> getterOfProducts_;

  float padHalfSize;
  int padSize, tParams, cnnLayers, infoSize;

  TTree* cnntree;

  UInt_t test;


};

//
// constants, enums and typedefs
//

//
// static data member definitions
//

//
// constructors and destructor
//
CNNInference::CNNInference(const edm::ParameterSet& iConfig):
processName_(iConfig.getParameter<std::string>("processName")),
intHitDoublets_(consumes<IntermediateHitDoublets>(iConfig.getParameter<edm::InputTag>("doublets"))),
tpMap_(consumes<ClusterTPAssociation>(iConfig.getParameter<edm::InputTag>("tpMap")))
{

  // usesResource("TFileService");
  //
  // edm::Service<TFileService> fs;
  // cnntree = fs->make<TTree>("CNNTree","Doublets Tree");

  // cnntree->Branch("test",      &test,          "test/I");

  edm::InputTag beamSpotTag = iConfig.getParameter<edm::InputTag>("beamSpot");
  bsSrc_ = consumes<reco::BeamSpot>(beamSpotTag);

  infoPileUp_ = consumes<std::vector<PileupSummaryInfo> >(iConfig.getParameter< edm::InputTag >("infoPileUp"));

  padHalfSize = 8;
  padSize = (int)(padHalfSize*2);
  tParams = 26;
  cnnLayers = 10;
  infoSize = 67;
}


CNNInference::~CNNInference()
{

  // do anything here that needs to be done at desctruction time
  // (e.g. close files, deallocate resources etc.)

}


//featuresLab = ['inX', 'inY', 'inZ', 'inPhi', 'inR', 'inDetSeq', 'inIsBarrel',
//'inLayer', 'inLadder', 'inSide', 'inDisk', 'inPanel', 'inModule', 'inIsFlipped',
//'inClustX', 'inClustY', 'inClustSize', 'inClustSizeX', 'inClustSizeY', 'inPixelZero',
//'inAvgCharge', 'inOverFlowX', 'inOverFlowY', 'inSkew', 'inIsBig', 'inIsBad', 'inIsEdge',
//'inAx1', 'inAx2', 'inSumADC',
//'outX', 'outY', 'outZ', 'outPhi', 'outR', 'outDetSeq',
//'outIsBarrel', 'outLayer', 'outLadder', 'outSide', 'outDisk', 'outPanel', 'outModule',
//'outIsFlipped', 'outClustX', 'outClustY', 'outClustSize', 'outClustSizeX',
//'outClustSizeY', 'outPixelZero', 'outAvgCharge', 'outOverFlowX', 'outOverFlowY',
//'outSkew', 'outIsBig', 'outIsBad', 'outIsEdge', 'outAx1', 'outAx2', 'outSumADC',
//
//'deltaA', 'deltaADC', 'deltaS', 'deltaR', 'deltaPhi', 'deltaZ', 'ZZero']
//
// member functions
//

// ------------ method called for each event  ------------
void
CNNInference::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup)
{
  using namespace edm;

  double padMean = 13382.0011321, padSigma = 10525.1252954;

  // int detOnArr[10] = {0,1,2,3,14,15,16,29,30,31};
  // std::vector<int> detOn(detOnArr,detOnArr+sizeof(detOnArr)/sizeof(int));

  // std::cout<<"CNNDoublets Analyzer"<<std::endl;

  edm::Handle<IntermediateHitDoublets> iHd;
  iEvent.getByToken(intHitDoublets_,iHd);

  edm::Handle<ClusterTPAssociation> tpClust;
  iEvent.getByToken(tpMap_,tpClust);

  // test = iEvent.id().event();
  //
  // cnntree->Fill();

  int eveNumber = iEvent.id().event();
  int runNumber = iEvent.id().run();
  int lumNumber = iEvent.id().luminosityBlock();

  std::vector<int> pixelDets{0,1,2,3,14,15,16,29,30,31};
  std::vector<int> partiList{11,13,15,22,111,211,311,321,2212,2112,3122,223};

  // reco::Vertex thePrimaryV, theBeamSpotV;

  //The Beamspot
  edm::Handle<reco::BeamSpot> recoBeamSpotHandle;
  iEvent.getByToken(bsSrc_,recoBeamSpotHandle);
  reco::BeamSpot const & bs = *recoBeamSpotHandle;
  // reco::Vertex theBeamSpotV(bs.position(), bs.covariance3D());

  edm::Handle< std::vector<PileupSummaryInfo> > puinfoH;
  iEvent.getByToken(infoPileUp_,puinfoH);
  PileupSummaryInfo puinfo;

  for (unsigned int puinfo_ite=0;puinfo_ite<(*puinfoH).size();++puinfo_ite){
    if ((*puinfoH)[puinfo_ite].getBunchCrossing()==0){
      puinfo=(*puinfoH)[puinfo_ite];
      break;
    }
  }

  int puNumInt = puinfo.getPU_NumInteractions();

  // std::vector<edm::Handle<IntermediateHitDoublets> > handles;
  // getterOfProducts_.fillHandles(iEvent, handles);

  // std::vector<edm::Handle<IntermediateHitDoublets> > intDoublets;
  // iEvent.getManyByType(intDoublets);

  // for (auto const& handle : handles)
  // {
  //   if(!handle.failedToGet())
  //   std::cout << handle.provenance()->moduleLabel()<< std::endl;
  // }

  std::string fileName = "doublets/" + std::to_string(lumNumber) +"_"+std::to_string(runNumber) +"_"+std::to_string(eveNumber);
  fileName += "_" + processName_ + "_dnn_doublets.txt";
  std::ofstream outCNNFile(fileName, std::ofstream::app);

  fileName = "doublets/" + std::to_string(lumNumber) +"_"+std::to_string(runNumber) +"_"+std::to_string(eveNumber);
  fileName += "_" + processName_ + "_dnn_doublets_tf.txt";
  std::ofstream outTFFile(fileName, std::ofstream::app);

  fileName = "doublets/" + std::to_string(lumNumber) +"_"+std::to_string(runNumber) +"_"+std::to_string(eveNumber);
  fileName += "_" + processName_ + "_dnn_doublets_inf.txt";
  std::ofstream outInference(fileName, std::ofstream::app);


  // Load graph
  tensorflow::setLogging("3");
  // edm::FileInPath modelFilePath();
  //tensorflow::GraphDef* graphDef = tensorflow::loadGraphDef("/lustre/home/adrianodif/jpsiphi/MCs/QCDtoPhiML/CMSSW_10_2_1/tmp/test_graph_tfadd.pb");
  //tensorflow::Session* session = tensorflow::createSession(graphDef);
  tensorflow::GraphDef* graphDef = tensorflow::loadGraphDef("/lustre/home/adrianodif/CNNDoublets/CMSSW/CMSSW_10_3_0_pre4/test.pb");
  tensorflow::Session* session = tensorflow::createSession(graphDef);


  float ax1, ax2, deltaADC = 0.0, deltaPhi = 0.0, deltaR = 0.0, deltaA = 0.0, deltaS = 0.0, deltaZ = 0.0, zZero = 0.0;

  std::vector < float > zeroPad;
  for (int nx = 0; nx < padSize; ++nx)
    for (int ny = 0; ny < padSize; ++ny)
      zeroPad.push_back(0.0);

  for (std::vector<IntermediateHitDoublets::LayerPairHitDoublets>::const_iterator lIt = iHd->layerSetsBegin(); lIt != iHd->layerSetsEnd(); ++lIt)
  {
    DetLayer const * innerLayer = lIt->doublets().detLayer(HitDoublets::inner);
    if(find(pixelDets.begin(),pixelDets.end(),innerLayer->seqNum())==pixelDets.end()) continue;   //TODO change to std::map ?

    DetLayer const * outerLayer = lIt->doublets().detLayer(HitDoublets::outer);
    if(find(pixelDets.begin(),pixelDets.end(),outerLayer->seqNum())==pixelDets.end()) continue;

    int innerLayerId = find(pixelDets.begin(),pixelDets.end(),innerLayer->seqNum()) - pixelDets.begin();
    int outerLayerId = find(pixelDets.begin(),pixelDets.end(),outerLayer->seqNum()) - pixelDets.begin();


    //     HitDoublets lDoublets = std::move(lIt->doublets());
    // std::cout << "Size: " << lIt->doublets().size() << std::endl;

    int numOfDoublets = int(lIt->doublets().size());

    tensorflow::Tensor inputPads(tensorflow::DT_FLOAT, {numOfDoublets,padSize,padSize,cnnLayers*2});
    tensorflow::Tensor inputFeat(tensorflow::DT_FLOAT, {numOfDoublets,infoSize});
    std::vector<tensorflow::Tensor> outputs;
    std::vector<float> labels;

    float* vPad = inputPads.flat<float>().data();
    float* vLab = inputFeat.flat<float>().data();

    int padCounter = 0;
    int infoCounter = 0;

    for (size_t i = 0; i < lIt->doublets().size(); i++)
    {

      std::vector <unsigned int> hitIds, subDetIds, detSeqs;

      std::vector< std::vector< float>> hitPars,hitLabs;
      std::vector< std::vector< float>> hitPads,inHitPads,outHitPads;
      std::vector< float > inHitPars, outHitPars, inPad, outPad;
      std::vector< float > inHitLabs, outHitLabs;
      std::vector< float > inTP, outTP, theTP;

      std::vector< RecHitsSortedInPhi::Hit> hits;
      std::vector< const SiPixelRecHit*> siHits;
      std::vector< SiPixelRecHit::ClusterRef> clusters;
      std::vector< DetId> detIds;
      std::vector< const GeomDet*> geomDets;

      int doubOffset = (padSize*padSize*cnnLayers*2)*i;
      int infoOffset = (infoSize)*i;

      // std::cout << i << std::endl;
      // std::cout << "Num pads: " << inputPads.NumElements() << std::endl;
      // std::cout << "Num feats: " << inputFeat.NumElements() << std::endl;
      // std::cout <<doubOffset << std::endl;
      // std::cout <<infoOffset << std::endl;
      // std::cout << numOfDoublets << std::endl;


      inHitPads.clear();
      outHitPads.clear();

      for(int i = 0; i < cnnLayers; ++i)
      {
        inHitPads.push_back(zeroPad);
        outHitPads.push_back(zeroPad);
      }


      deltaPhi = 0.0;
      deltaR = 0.0;
      deltaA = 0.0;
      deltaADC = 0.0;
      deltaS = 0.0;
      zZero = 0.0;

      hits.clear(); siHits.clear(); clusters.clear();
      detIds.clear(); geomDets.clear(); hitIds.clear();
      subDetIds.clear(); detSeqs.clear(); hitPars.clear(); theTP.clear();
      inHitPars.clear(); outHitPars.clear();

      hits.push_back(lIt->doublets().hit(i, HitDoublets::inner)); //TODO CHECK EMPLACEBACK
      hits.push_back(lIt->doublets().hit(i, HitDoublets::outer));

      for (auto h : hits)
      {
        detIds.push_back(h->hit()->geographicalId());
        subDetIds.push_back((h->hit()->geographicalId()).subdetId());
      }
      // innerDetId = innerHit->hit()->geographicalId();

      if (! (((subDetIds[0]==1) || (subDetIds[0]==2)) && ((subDetIds[1]==1) || (subDetIds[1]==2)))) continue;

      hitIds.push_back(lIt->doublets().innerHitId(i));
      hitIds.push_back(lIt->doublets().outerHitId(i));

      siHits.push_back(dynamic_cast<const SiPixelRecHit*>((hits[0])));
      siHits.push_back(dynamic_cast<const SiPixelRecHit*>((hits[1])));

      clusters.push_back(siHits[0]->cluster());
      clusters.push_back(siHits[1]->cluster());

      detSeqs.push_back(innerLayer->seqNum());
      detSeqs.push_back(outerLayer->seqNum());

      geomDets.push_back(hits[0]->det());
      geomDets.push_back(hits[1]->det());

      hitPars.push_back(inHitPars);
      hitPars.push_back(outHitPars);

      hitPads.push_back(inPad);
      hitPads.push_back(outPad);

      hitLabs.push_back(inHitLabs);
      hitLabs.push_back(outHitLabs);

      HitDoublets::layer layers[2] = {HitDoublets::inner, HitDoublets::outer};


      for(int j = 0; j < 2; ++j)
      {

        //4
        hitPars[j].push_back((hits[j]->hit()->globalState()).position.x()); //1
        hitPars[j].push_back((hits[j]->hit()->globalState()).position.y());
        hitPars[j].push_back((hits[j]->hit()->globalState()).position.z()); //3

        hitLabs[j].push_back((hits[j]->hit()->globalState()).position.x()); //1
        hitLabs[j].push_back((hits[j]->hit()->globalState()).position.y());
        hitLabs[j].push_back((hits[j]->hit()->globalState()).position.z()); //3

        float phi = lIt->doublets().phi(i,layers[j]) >=0.0 ? lIt->doublets().phi(i,layers[j]) : 2*M_PI + lIt->doublets().phi(i,layers[j]);

        hitPars[j].push_back(phi); //Phi //FIXME
        hitPars[j].push_back(lIt->doublets().r(i,layers[j])); //R //TODO add theta and DR

        hitPars[j].push_back(detSeqs[j]); //det number //6

        hitLabs[j].push_back(phi); //Phi //FIXME
        hitLabs[j].push_back(lIt->doublets().r(i,layers[j])); //R //TODO add theta and DR

        hitLabs[j].push_back(detSeqs[j]); //det number //6

        //Module labels
        if(subDetIds[j]==1) //barrel
        {
          hitPars[j].push_back(float(true));  //isBarrel //7
          hitPars[j].push_back(PXBDetId(detIds[j]).layer());
          hitPars[j].push_back(PXBDetId(detIds[j]).ladder());
          hitPars[j].push_back(-1.0);
          hitPars[j].push_back(-1.0);
          hitPars[j].push_back(-1.0);
          hitPars[j].push_back(PXBDetId(detIds[j]).module());   //14

          hitLabs[j].push_back(float(true)); //isBarrel //7
          hitLabs[j].push_back(PXBDetId(detIds[j]).layer());
          hitLabs[j].push_back(PXBDetId(detIds[j]).ladder());
          hitLabs[j].push_back(-1.0);
          hitLabs[j].push_back(-1.0);
          hitLabs[j].push_back(-1.0);
          hitLabs[j].push_back(PXBDetId(detIds[j]).module()); //14

        }
        else
        {
          hitPars[j].push_back(float(false)); //isBarrel
          hitPars[j].push_back(-1.0);
          hitPars[j].push_back(-1.0);
          hitPars[j].push_back(PXFDetId(detIds[j]).side());
          hitPars[j].push_back(PXFDetId(detIds[j]).disk());
          hitPars[j].push_back(PXFDetId(detIds[j]).panel());
          hitPars[j].push_back(PXFDetId(detIds[j]).module());

          hitLabs[j].push_back(float(false)); //isBarrel
          hitLabs[j].push_back(-1.0);
          hitLabs[j].push_back(-1.0);
          hitLabs[j].push_back(PXFDetId(detIds[j]).side());
          hitLabs[j].push_back(PXFDetId(detIds[j]).disk());
          hitLabs[j].push_back(PXFDetId(detIds[j]).panel());
          hitLabs[j].push_back(PXFDetId(detIds[j]).module());
        }

        //Module orientation
        ax1 = geomDets[j]->surface().toGlobal(Local3DPoint(0.,0.,0.)).perp(); //15
        ax2 = geomDets[j]->surface().toGlobal(Local3DPoint(0.,0.,1.)).perp();

        hitPars[j].push_back(float(ax1<ax2)); //isFlipped
        hitPars[j].push_back(ax1); //Module orientation y
        hitPars[j].push_back(ax2); //Module orientation x

        hitLabs[j].push_back(float(ax1<ax2)); //isFlipped
        hitLabs[j].push_back(ax1); //Module orientation y
        hitLabs[j].push_back(ax2); //Module orientation x

        //TODO check CLusterRef & OmniClusterRef

        //ClusterInformations
        hitPars[j].push_back((float)clusters[j]->x()); //20
        hitPars[j].push_back((float)clusters[j]->y());
        hitPars[j].push_back((float)clusters[j]->size());
        hitPars[j].push_back((float)clusters[j]->sizeX());
        hitPars[j].push_back((float)clusters[j]->sizeY());
        hitPars[j].push_back((float)clusters[j]->pixel(0).adc); //25
        hitPars[j].push_back(float(clusters[j]->charge())/float(clusters[j]->size())); //avg pixel charge

        hitPars[j].push_back((float)(clusters[j]->sizeX() > padSize));//27
        hitPars[j].push_back((float)(clusters[j]->sizeY() > padSize));
        hitPars[j].push_back((float)(clusters[j]->sizeY()) / (float)(clusters[j]->sizeX()));

        hitPars[j].push_back((float)siHits[j]->spansTwoROCs());
        hitPars[j].push_back((float)siHits[j]->hasBadPixels());
        hitPars[j].push_back((float)siHits[j]->isOnEdge()); //31
//inX
        hitLabs[j].push_back((float)clusters[j]->x()); //20
        hitLabs[j].push_back((float)clusters[j]->y());
        hitLabs[j].push_back((float)clusters[j]->size());
        hitLabs[j].push_back((float)clusters[j]->sizeX());
        hitLabs[j].push_back((float)clusters[j]->sizeY());
        hitLabs[j].push_back((float)clusters[j]->pixel(0).adc); //25
        hitLabs[j].push_back(float(clusters[j]->charge())/float(clusters[j]->size())); //avg pixel charge

        hitLabs[j].push_back((float)(clusters[j]->sizeX() > padSize));//27
        hitLabs[j].push_back((float)(clusters[j]->sizeY() > padSize));
        hitLabs[j].push_back((float)(clusters[j]->sizeY()) / (float)(clusters[j]->sizeX()));

        hitLabs[j].push_back((float)siHits[j]->spansTwoROCs());
        hitLabs[j].push_back((float)siHits[j]->hasBadPixels());
        hitLabs[j].push_back((float)siHits[j]->isOnEdge()); //31

        //Cluster Pad
        TH2F hClust("hClust","hClust",
        padSize,
        clusters[j]->x()-padHalfSize,
        clusters[j]->x()+padHalfSize,
        padSize,
        clusters[j]->y()-padHalfSize,
        clusters[j]->y()+padHalfSize);

        //Initialization
        for (int nx = 0; nx < padSize; ++nx)
        for (int ny = 0; ny < padSize; ++ny)
        hClust.SetBinContent(nx,ny,0.0);

        for (int k = 0; k < clusters[j]->size(); ++k)
        hClust.SetBinContent(hClust.FindBin((float)clusters[j]->pixel(k).x, (float)clusters[j]->pixel(k).y),(float)clusters[j]->pixel(k).adc);

        //Linearizing the cluster

        for (int ny = padSize; ny>0; --ny)
        {
          for(int nx = 0; nx<padSize; nx++)
          {
            int n = (ny+2)*(padSize + 2) - 2 -2 - nx - padSize; //see TH2 reference for clarification
            hitPars[j].push_back(hClust.GetBinContent(n));
            hitPads[j].push_back(hClust.GetBinContent(n));
          }
        }



        //ADC sum
        hitPars[j].push_back(float(clusters[j]->charge()));
        hitLabs[j].push_back(float(clusters[j]->charge()));

        deltaA   -= ((float)clusters[j]->size()); deltaA *= -1.0;
        deltaADC -= clusters[j]->charge(); deltaADC *= -1.0; //At the end == Outer Hit ADC - Inner Hit ADC
        deltaS   -= ((float)(clusters[j]->sizeY()) / (float)(clusters[j]->sizeX())); deltaS *= -1.0;
        deltaR   -= lIt->doublets().r(i,layers[j]); deltaR *= -1.0;
        deltaPhi -= phi; deltaPhi *= -1.0;
      }

      for (int nx = 0; nx < padSize*padSize; ++nx)
          inHitPads[innerLayerId][nx] = (hitPads[0][nx]- padMean)/padSigma;
      for (int nx = 0; nx < padSize*padSize; ++nx)
          outHitPads[outerLayerId][nx] = (hitPads[1][nx]- padMean)/padSigma;
      //
      // std::cout << "inHitPads.size()=" << inHitPads.size() <<std::endl;
      // std::cout << "outHitPads.size()=" << outHitPads.size() <<std::endl;
      //
      // std::cout << "inHitPads[innerLayerId].size()=" << inHitPads[innerLayerId].size() <<std::endl;
      // std::cout << "outHitPads[outerLayerId].size()=" << outHitPads[outerLayerId].size() <<std::endl;

      int thisOffset = 0;

      for (int jc = 0; jc < 10; jc++)
      {
        thisOffset = jc * padSize*padSize;
        for (int nx = 0; nx < padSize*padSize; nx++)
        {
          vPad[thisOffset + nx + doubOffset] = inHitPads[jc][nx];
          padCounter++;
        }
      }

      for (int jc = 0; jc < 10; jc++)
      {
        thisOffset = jc * padSize*padSize + 10 * padSize*padSize;
        for (int nx = 0; nx < padSize*padSize; nx++)
        {
          vPad[thisOffset + nx + doubOffset ] = outHitPads[jc][nx];
          padCounter++;
        }
      }


      // std::cout << "Inner hit layer : " << innerLayer->seqNum() << " - " << innerLayerId<< std::endl;
      //
      // for(int i = 0; i < cnnLayers; ++i)
      // {
      //   std::cout << i << std::endl;
      //   auto thisOne = inHitPads[i];
      //   for (int nx = 0; nx < padSize; ++nx)
      //     for (int ny = 0; ny < padSize; ++ny)
      //     {
      //       std::cout << thisOne[ny + nx*padSize] << " ";
      //     }
      //     std::cout << std::endl;
      //
      // }
      //
      // std::cout << "Outer hit layer : " << outerLayer->seqNum() << " - " << outerLayerId<< std::endl;
      // for(int i = 0; i < cnnLayers; ++i)
      // {
      //   std::cout << i << std::endl;
      //   auto thisOne = outHitPads[i];
      //   for (int nx = 0; nx < padSize; ++nx)
      //     for (int ny = 0; ny < padSize; ++ny)
      //     {
      //       std::cout << thisOne[ny + nx*padSize + doubOffset] << " ";
      //     }
      //     std::cout << std::endl;
      //
      // }
      //
      // std::cout << "TF Translation" << std::endl;
      // for(int i = 0; i < cnnLayers*2; ++i)
      // {
      //   std::cout << i << std::endl;
      //   int theOffset = i*padSize*padSize;
      //   for (int nx = 0; nx < padSize; ++nx)
      //     for (int ny = 0; ny < padSize; ++ny)
      //     {
      //       std::cout << vPad[(ny + nx*padSize) + theOffset + doubOffset] << " ";
      //     }
      //     std::cout << std::endl;
      //
      // }

      // std::cout << "hitLabs[j].size()=" << hitLabs[0].size() <<std::endl;
      // for (size_t i = 0; i < 2*hitLabs[0].size() + 8; i++)
      //   std::cout << vLab [i + doubOffset] << " ";
      // std::cout << std::endl;


      //std::cout << outputs[1].DebugString() << std::endl;


      deltaPhi *= deltaPhi > M_PI ? 2*M_PI - fabs(deltaPhi) : 1.0;

      //Tp Matching
      auto rangeIn = tpClust->equal_range(hits[0]->firstClusterRef());
      auto rangeOut = tpClust->equal_range(hits[1]->firstClusterRef());

      // std::cout << "Doublet no. "  << i << " hit no. " << lIt->doublets().innerHitId(i) << std::endl;

      std::vector< std::pair<int,int> > kPdgIn, kPdgOut, kIntersection;

      for(auto ip=rangeIn.first; ip != rangeIn.second; ++ip)
      kPdgIn.push_back({ip->second.key(),(*ip->second).pdgId()});

      for(auto ip=rangeOut.first; ip != rangeOut.second; ++ip)
      kPdgOut.push_back({ip->second.key(),(*ip->second).pdgId()});

      // if(rangeIn.first == rangeIn.second) std::cout<<"In unmatched"<<std::endl;
      std::set_intersection(kPdgIn.begin(), kPdgIn.end(),kPdgOut.begin(), kPdgOut.end(), std::back_inserter(kIntersection));
      // std::cout << "Intersection : "<< kIntersection.size() << std::endl;

      //TODO in case of unmatched but associated to a tp we could save both tp to study the missmatching
      //Matched :D
      if (kIntersection.size()>0)
      {
        labels.push_back(1.0);
        // in case of multiple tp matching both hits use the first one for labels
        auto kPar = ((std::find(kPdgIn.begin(), kPdgIn.end(), kIntersection[0]) - kPdgIn.begin()) + rangeIn.first);

        auto particle = *kPar->second;
        TrackingParticle::Vector momTp = particle.momentum();
        TrackingParticle::Point  verTp  = particle.vertex();

        theTP.push_back(1.0);
        theTP.push_back(kIntersection.size());

        unsigned int iParticle = 0;
        std::vector <int> commonPdg;

        for (size_t iInt = 0; iInt < kIntersection.size(); iInt++)
          commonPdg.push_back(kIntersection[iInt].second);

        for (unsigned int iTr = 0; iTr<partiList.size(); iTr++ ) {
          if(std::find(commonPdg.begin(),commonPdg.end(),partiList[iTr])!=(commonPdg.end()))
              iParticle += (1<<iTr);
        }

        theTP.push_back(iParticle);
        // std::cout << kPar->second.key() << std::endl;

        for (size_t k = 0; k < 2; k++) {


          theTP.push_back(1.0); // 1
          theTP.push_back(kPar->second.key()); // 2
          theTP.push_back(momTp.x()); // 3
          theTP.push_back(momTp.y()); // 4
          theTP.push_back(momTp.z()); // 5
          theTP.push_back(particle.pt()); //6

          theTP.push_back(particle.mt());
          theTP.push_back(particle.et());
          theTP.push_back(particle.massSqr()); //9

          theTP.push_back(particle.pdgId());
          theTP.push_back(particle.charge()); //11

          theTP.push_back(particle.numberOfTrackerHits()); //TODO no. pixel hits?
          theTP.push_back(particle.numberOfTrackerLayers());
          //TODO is cosmic?
          theTP.push_back(particle.phi());
          theTP.push_back(particle.eta());
          theTP.push_back(particle.rapidity()); //16

          theTP.push_back(verTp.x());
          theTP.push_back(verTp.y());
          theTP.push_back(verTp.z());
          theTP.push_back((-verTp.x()*sin(momTp.phi())+verTp.y()*cos(momTp.phi()))); //dxy
          theTP.push_back((verTp.z() - (verTp.x() * momTp.x()+
          verTp.y() *
          momTp.y())/sqrt(momTp.perp2()) *
          momTp.z()/sqrt(momTp.perp2()))); //21 //dz //TODO Check MomVert //search parametersDefiner



          /*
          //W.R.T. PCA
          edm::ESHandle<ParametersDefinerForTP> parametersDefinerTPHandle;
          setup.get<TrackAssociatorRecord>().get(parametersDefiner,parametersDefinerTPHandle);
          auto parametersDefinerTP = parametersDefinerTPHandle->clone();

          TrackingParticle::Vector momentum = parametersDefinerTP->momentum(event,setup,tpr);
          TrackingParticle::Point vertex = parametersDefinerTP->vertex(event,setup,tpr);
          dxySim = (-vertex.x()*sin(momentum.phi())+vertex.y()*cos(momentum.phi()));
          dzSim = vertex.z() - (vertex.x()*momentum.x()+vertex.y()*momentum.y())/sqrt(momentum.perp2())
          * momentum.z()/sqrt(momentum.perp2());
          */

          theTP.push_back(particle.eventId().bunchCrossing()); //22
          theTP.push_back(1.0); //For compatibility with tracks infos, nothing special.
          theTP.push_back(1.0);
          theTP.push_back(1.0);
          theTP.push_back(1.0);
        }

        //TODO Check for other parameters

      }
      else
      {
        labels.push_back(0.0);

        theTP.push_back(-1.0);
        theTP.push_back(-1.0);
        theTP.push_back(-1.0);
        //Check inParticle
        auto inPar = rangeIn.first;
        if (rangeIn.first != rangeIn.second)
        {

          auto particle = *inPar->second;

          TrackingParticle::Vector momTp = particle.momentum();
          TrackingParticle::Point  verTp  = particle.vertex();


          theTP.push_back(1.0); // 1
          theTP.push_back(inPar->second.key()); // 2
          theTP.push_back(momTp.x()); // 3
          theTP.push_back(momTp.y()); // 4
          theTP.push_back(momTp.z()); // 5
          theTP.push_back(particle.pt()); //6

          theTP.push_back(particle.mt());
          theTP.push_back(particle.et());
          theTP.push_back(particle.massSqr()); //9

          theTP.push_back(particle.pdgId());
          theTP.push_back(particle.charge()); //11

          theTP.push_back(particle.numberOfTrackerHits()); //TODO no. pixel hits?
          theTP.push_back(particle.numberOfTrackerLayers());
          //TODO is cosmic?
          theTP.push_back(particle.phi());
          theTP.push_back(particle.eta());
          theTP.push_back(particle.rapidity()); //16

          theTP.push_back(verTp.x());
          theTP.push_back(verTp.y());
          theTP.push_back(verTp.z());
          theTP.push_back((-verTp.x()*sin(momTp.phi())+verTp.y()*cos(momTp.phi()))); //dxy
          theTP.push_back((verTp.z() - (verTp.x() * momTp.x()+
          verTp.y() *
          momTp.y())/sqrt(momTp.perp2()) *
          momTp.z()/sqrt(momTp.perp2()))); //21 //dz //TODO Check MomVert //search parametersDefiner

          /*
          //W.R.T. PCA
          edm::ESHandle<ParametersDefinerForTP> parametersDefinerTPHandle;
          setup.get<TrackAssociatorRecord>().get(parametersDefiner,parametersDefinerTPHandle);
          auto parametersDefinerTP = parametersDefinerTPHandle->clone();

          TrackingParticle::Vector momentum = parametersDefinerTP->momentum(event,setup,tpr);
          TrackingParticle::Point vertex = parametersDefinerTP->vertex(event,setup,tpr);
          dxySim = (-vertex.x()*sin(momentum.phi())+vertex.y()*cos(momentum.phi()));
          dzSim = vertex.z() - (vertex.x()*momentum.x()+vertex.y()*momentum.y())/sqrt(momentum.perp2())
          * momentum.z()/sqrt(momentum.perp2());
          */

          theTP.push_back(particle.eventId().bunchCrossing());
          theTP.push_back(1.0);
          theTP.push_back(1.0);
          theTP.push_back(1.0);
          theTP.push_back(1.0);
        }
        else
        {
        for (int i = 0; i < tParams; i++)
          theTP.push_back(-1.0);

        }

        auto outPar = rangeOut.first;

        if (rangeOut.first != rangeOut.second)
        {

          auto particle = *outPar->second;

          TrackingParticle::Vector momTp = particle.momentum();
          TrackingParticle::Point  verTp  = particle.vertex();

          theTP.push_back(1.0); // 1
          theTP.push_back(outPar->second.key()); // 2
          theTP.push_back(momTp.x()); // 3
          theTP.push_back(momTp.y()); // 4
          theTP.push_back(momTp.z()); // 5
          theTP.push_back(particle.pt()); //6

          theTP.push_back(particle.mt());
          theTP.push_back(particle.et());
          theTP.push_back(particle.massSqr()); //9

          theTP.push_back(particle.pdgId());
          theTP.push_back(particle.charge()); //11

          theTP.push_back(particle.numberOfTrackerHits()); //TODO no. pixel hits?
          theTP.push_back(particle.numberOfTrackerLayers());
          //TODO is cosmic?
          theTP.push_back(particle.phi());
          theTP.push_back(particle.eta());
          theTP.push_back(particle.rapidity()); //16

          theTP.push_back(verTp.x());
          theTP.push_back(verTp.y());
          theTP.push_back(verTp.z());
          theTP.push_back((-verTp.x()*sin(momTp.phi())+verTp.y()*cos(momTp.phi()))); //dxy
          theTP.push_back((verTp.z() - (verTp.x() * momTp.x()+
          verTp.y() *
          momTp.y())/sqrt(momTp.perp2()) *
          momTp.z()/sqrt(momTp.perp2()))); //21 //dz //TODO Check MomVert //search parametersDefiner


          /*
          //W.R.T. PCA
          edm::ESHandle<ParametersDefinerForTP> parametersDefinerTPHandle;
          setup.get<TrackAssociatorRecord>().get(parametersDefiner,parametersDefinerTPHandle);
          auto parametersDefinerTP = parametersDefinerTPHandle->clone();

          TrackingParticle::Vector momentum = parametersDefinerTP->momentum(event,setup,tpr);
          TrackingParticle::Point vertex = parametersDefinerTP->vertex(event,setup,tpr);
          dxySim = (-vertex.x()*sin(momentum.phi())+vertex.y()*cos(momentum.phi()));
          dzSim = vertex.z() - (vertex.x()*momentum.x()+vertex.y()*momentum.y())/sqrt(momentum.perp2())
          * momentum.z()/sqrt(momentum.perp2());
          */

          theTP.push_back(particle.eventId().bunchCrossing());
          theTP.push_back(1.0);
          theTP.push_back(1.0);
          theTP.push_back(1.0);
          theTP.push_back(1.0);

        }
        else
        for (int i = 0; i < tParams; i++) {
          theTP.push_back(-1.0);
        }


      }

      zZero = (hits[0]->hit()->globalState()).position.z();
      zZero -= lIt->doublets().r(i,layers[0]) * (deltaZ/deltaR);

      outCNNFile << runNumber << "\t" << eveNumber << "\t" << lumNumber << "\t" << puNumInt << "\t";
      outCNNFile <<innerLayer->seqNum() << "\t" << outerLayer->seqNum() << "\t";
      outCNNFile << bs.x0() << "\t" << bs.y0() << "\t" << bs.z0() << "\t" << bs.sigmaZ() << "\t";

      for (int j = 0; j < 2; j++)
      for (size_t i = 0; i < hitLabs[j].size(); i++)
        {
          infoCounter++;
          vLab[i + j * hitLabs[j].size() + infoOffset] = hitLabs[j][i];
        }
      vLab[ 2 * hitLabs[0].size() + 0 + infoOffset] = deltaA   ; infoCounter++;
      vLab[ 2 * hitLabs[0].size() + 1 + infoOffset] = deltaADC ; infoCounter++;
      vLab[ 2 * hitLabs[0].size() + 2 + infoOffset] = deltaS   ; infoCounter++;
      vLab[ 2 * hitLabs[0].size() + 3 + infoOffset] = deltaR   ; infoCounter++;
      vLab[ 2 * hitLabs[0].size() + 4 + infoOffset] = deltaPhi ; infoCounter++;
      vLab[ 2 * hitLabs[0].size() + 5 + infoOffset] = deltaZ   ; infoCounter++;
      vLab[ 2 * hitLabs[0].size() + 6 + infoOffset] = zZero    ; infoCounter++;

      for (int j = 0; j < 2; j++)
        for (size_t i = 0; i < hitPars[j].size(); i++)
        outCNNFile << hitPars[j][i] << "\t";

      outCNNFile << deltaA   << "\t";
      outCNNFile << deltaADC << "\t";
      outCNNFile << deltaS   << "\t";
      outCNNFile << deltaR   << "\t";
      outCNNFile << deltaPhi << "\t";
      outCNNFile << deltaZ   << "\t";
      outCNNFile << zZero    << "\t";

      for (size_t i = 0; i < theTP.size(); i++)
      outCNNFile << theTP[i] << "\t";

      outCNNFile << 542.1369;
      outCNNFile << std::endl;
      // outCNNFile << hitPars[0].size() << " -- " <<  hitPars[1].size() << " -- " << theTP.size() << std::endl << std::endl;
      // std::cout << hitPars[0].size() << " " << hitPars[1].size() << " " << theTP.size() << std::endl;

      outTFFile << runNumber << "\t" << eveNumber << "\t" << lumNumber << "\t" << puNumInt << "\t";
      outTFFile << innerLayer->seqNum() << "\t" << outerLayer->seqNum() << "\t";
      outTFFile << bs.x0() << "\t" << bs.y0() << "\t" << bs.z0() << "\t" << bs.sigmaZ() << "\t";

      for (int jc = 0; jc < cnnLayers*2*padSize*padSize; jc++)
        outTFFile << vPad[jc + doubOffset] << "\t";

      for (int ji = 0; ji < infoSize; ji++)
        outTFFile << vLab[ji + infoOffset] << "\t";

      outTFFile << 542.1369;
      outTFFile << std::endl;


    }//end single doublet

    tensorflow::run(session, { { "hit_shape_input", inputPads }, { "info_input", inputFeat } },
                  { "output/Softmax" }, &outputs);

    std::cout << "START" << std::endl;
    std::cout << outputs[0].DebugString() << std::endl;
    std::cout << outputs.size() << std::endl;
    std::cout << labels.size() << std::endl;
    std::cout << infoCounter << " - " << inputFeat.DebugString() << std::endl;
    std::cout << padCounter  << " - "  << inputPads.DebugString() << std::endl;

    // for (size_t i = 0; i < labels.size(); i++) {
    //   float outs = outputs[0].matrix<float>()(i,0);
    //   float outs2 = outputs[0].matrix<float>()(i,1);
    //   std::cout << outs << " - " << outs2 << " - " << labels[i] << std::endl;
    // }

    for (size_t i = 0; i < labels.size(); i++)
    {
      float outs = outputs[0].matrix<float>()(i,0);
      float outs2 = outputs[0].matrix<float>()(i,1);
      outInference << outs << " - " << outs2 << " - " << labels[i] << std::endl;
    }

    std::cout << "DONE" << std::endl;

  }
  // auto range = clusterToTPMap.equal_range(dynamic_cast<const BaseTrackerRecHit&>(hit).firstClusterRef());
  //      for(auto ip=range.first; ip != range.second; ++ip) {
  //        const auto tpKey = ip->second.key();
  //        if(tpKeyToIndex.find(tpKey) == tpKeyToIndex.end()) // filter out TPs not given as an input
  //          continue;
  //        func(tpKey);
  //      }


}


// ------------ method called once each job just before starting event loop  ------------
void
CNNInference::beginJob()
{
}

// ------------ method called once each job just after ending the event loop  ------------
void
CNNInference::endJob()
{
}

// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void
CNNInference::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  //The following says we do not know what parameters are allowed so do no validation
  // Please change this to state exactly what you do use, even if it is no parameters
  edm::ParameterSetDescription desc;
  desc.setUnknown();
  descriptions.addDefault(desc);
}

//define this as a plug-in
DEFINE_FWK_MODULE(CNNInference);

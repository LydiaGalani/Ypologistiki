#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TSystem.h>
#include <TLine.h>
#include <vector>
#include <cmath>
#include <iostream>
#include <string>
#include <algorithm>

enum EtaRegion { BARREL=0, ENDCAP=1, FORWARD=2 };

struct CutDef {
  bool has = false;
  double x = 0.0;        // cut value
  bool keepLess = true;  // true: keep x < cut, false: keep x > cut
};

static std::string RegionTag(EtaRegion r){
  if (r==BARREL)  return "eta00_26";
  if (r==ENDCAP)  return "eta27_30";
  return "eta30_50";
}

static std::string RegionLabel(EtaRegion r){
  if (r==BARREL)  return "|#eta|<2.6";
  if (r==ENDCAP)  return "2.7<|#eta|<3.0";
  return "3.0<|#eta|<5.0";
}

static EtaRegion WhichRegion(double abseta){
  if (abseta < 2.6) return BARREL;
  if (abseta > 2.7 && abseta < 3.0) return ENDCAP;
  if (abseta > 3.0 && abseta < 5.0) return FORWARD;
  // Outside our requested bins -> return something but we will skip
  return FORWARD;
}

// Tight JetID (piecewise). Note: for our plots we also *select* the eta bin,
// but JetID is evaluated with the region-appropriate thresholds.
bool PassJetID_Tight(float abseta,
                     float nhf, float nemf,
                     float chf, float cemf,
                     int chMult, int neMult)
{
    const int nConst = chMult + neMult;

    // |eta| <= 2.7
    if (abseta <= 2.7f) {
        if (nhf  >= 0.90f) return false;
        if (nemf >= 0.90f) return false;
        if (nConst <= 1)   return false;

        // extra for |eta| <= 2.4
        if (abseta <= 2.4f) {
            if (chf <= 0.0f)   return false;
            if (chMult <= 0)   return false;
            if (cemf >= 0.90f) return false;
        }
        return true;
    }

    // 2.7 < |eta| <= 3.0
    if (abseta <= 3.0f) {
        if (nhf  >= 0.98f) return false;
        if (nemf <= 0.01f) return false;
        if (neMult <= 2)   return false;
        return true;
    }

    // |eta| > 3.0
    if (nhf  <= 0.02f) return false;
    if (nemf >= 0.90f) return false;
    if (neMult <= 10)  return false;
    return true;
}

// Cuts to DRAW as vertical lines per variable & region (Tight JetID)
CutDef GetCutFor(const std::string& var, EtaRegion r){
  CutDef c;

  // NHF
  if (var=="NHF"){
    c.has = true;
    if (r==BARREL)  { c.x=0.90; c.keepLess=true; }
    if (r==ENDCAP)  { c.x=0.98; c.keepLess=true; }
    if (r==FORWARD) { c.x=0.02; c.keepLess=false; } // keep > 0.02
    return c;
  }

  // NEMF
  if (var=="NEMF"){
    c.has = true;
    if (r==BARREL)  { c.x=0.90; c.keepLess=true; }
    if (r==ENDCAP)  { c.x=0.01; c.keepLess=false; } // keep > 0.01
    if (r==FORWARD) { c.x=0.90; c.keepLess=true; }
    return c;
  }

  // neMult
  if (var=="neMult"){
    if (r==ENDCAP)  { c.has=true; c.x=2;  c.keepLess=false; } // keep >2
    if (r==FORWARD) { c.has=true; c.x=10; c.keepLess=false; } // keep >10
    return c;
  }

  // nConstituents (only central selection uses it)
  if (var=="nConstituents" && r==BARREL){
    c.has=true; c.x=1; c.keepLess=false; // keep >1
    return c;
  }

  // CHF / chMult / CEMF are only used for |eta|<=2.4 (subset of BARREL bin)
  // We still draw them on the BARREL plots for reference.
  if (var=="CHF" && r==BARREL){
    c.has=true; c.x=0.0; c.keepLess=false; // keep >0
    return c;
  }
  if (var=="chMult" && r==BARREL){
    c.has=true; c.x=0.0; c.keepLess=false; // keep >0
    return c;
  }
  if (var=="CEMF" && r==BARREL){
    c.has=true; c.x=0.90; c.keepLess=true; // keep <0.90
    return c;
  }

  // MUF: no tight cut in these definitions (line not drawn)
  return c;
}

void DrawOverlayWithCut(TH1D* hNo, TH1D* hId,
                        const std::string& var,
                        EtaRegion r,
                        const char* outPdf)
{
  TCanvas c("c","c",900,700);
  c.SetLogy(true);

  const std::string title = RegionLabel(r) + ", p_{T}>30 GeV; " + var;
  hNo->SetTitle(title.c_str());
  hNo->GetXaxis()->SetTitle(var.c_str());
  hNo->GetYaxis()->SetTitle("Jets");
  hNo->SetStats(false);

  hNo->SetMinimum(0.5);

  hNo->SetLineWidth(2);
  hId->SetLineWidth(2);
  hNo->SetLineColor(kBlack);
  hId->SetLineColor(kRed+1);

  double ymax = std::max(hNo->GetMaximum(), hId->GetMaximum());
  hNo->SetMaximum(ymax*2.0);

  hNo->Draw("hist");
  hId->Draw("hist same");

  // draw cut line if exists
  CutDef cd = GetCutFor(var, r);
  TLine line;
  bool drewLine=false;
  if (cd.has){
    line.SetLineWidth(2);
    line.SetLineStyle(2);
    line.SetLineColor(kBlue+2);
    line.DrawLine(cd.x, 0.5, cd.x, ymax*2.0);
    drewLine=true;
  }

  TLegend leg(0.58, 0.70, 0.90, 0.90);
  leg.SetBorderSize(0);
  leg.AddEntry(hNo, "No JetID", "l");
  leg.AddEntry(hId, "Tight JetID", "l");
  if (drewLine){
    std::string cutTxt = "cut: " + var + (cd.keepLess ? " < " : " > ") + std::to_string(cd.x);
    // tidy formatting for common values
    if (cd.x==0.90) cutTxt = "cut: " + var + (cd.keepLess ? " < 0.90" : " > 0.90");
    if (cd.x==0.98) cutTxt = "cut: " + var + (cd.keepLess ? " < 0.98" : " > 0.98");
    if (cd.x==0.02) cutTxt = "cut: " + var + (cd.keepLess ? " < 0.02" : " > 0.02");
    if (cd.x==0.01) cutTxt = "cut: " + var + (cd.keepLess ? " < 0.01" : " > 0.01");
    if (cd.x==2)    cutTxt = "cut: " + var + (cd.keepLess ? " < 2" : " > 2");
    if (cd.x==10)   cutTxt = "cut: " + var + (cd.keepLess ? " < 10" : " > 10");
    if (cd.x==1)    cutTxt = "cut: " + var + (cd.keepLess ? " < 1" : " > 1");
    if (cd.x==0.0)  cutTxt = "cut: " + var + (cd.keepLess ? " < 0" : " > 0");
    leg.AddEntry((TObject*)0, cutTxt.c_str(), "");
  } else {
    leg.AddEntry((TObject*)0, "no cut in this #eta region", "");
  }
  leg.Draw();

  c.SaveAs(outPdf);
}

void Create_hist_and_plot_fixed()
{
  // ===== SETTINGS =====
  const char* inputFile = "run3_MinimumBias_runC_reduced_tree.root";
  const char* treeName  = "tree";
  const char* outDir    = "C:/work/ROOT/plots";
  const float ptCut     = 30.0f;
  // ====================

  gSystem->mkdir(outDir, kTRUE);

  TFile *f = TFile::Open(inputFile);
  if (!f || f->IsZombie()){
    std::cout << "ERROR: cannot open " << inputFile << "\n";
    return;
  }
  TTree *tree = (TTree*)f->Get(treeName);
  if (!tree){
    std::cout << "ERROR: tree '" << treeName << "' not found\n";
    return;
  }

  // Branches
  std::vector<float> *jetPt=nullptr, *jetEta=nullptr;
  std::vector<float> *jetNhf=nullptr, *jetNemf=nullptr, *jetMuf=nullptr, *jetChf=nullptr, *jetCemf=nullptr;
  std::vector<int>   *chMult=nullptr, *neMult=nullptr;

  tree->SetBranchAddress("jetPt", &jetPt);
  tree->SetBranchAddress("jetEta",&jetEta);
  tree->SetBranchAddress("jetNhf",&jetNhf);
  tree->SetBranchAddress("jetNemf",&jetNemf);
  tree->SetBranchAddress("jetMuf",&jetMuf);
  tree->SetBranchAddress("jetChf",&jetChf);
  tree->SetBranchAddress("jetCemf",&jetCemf);
  tree->SetBranchAddress("chMult",&chMult);
  tree->SetBranchAddress("neMult",&neMult);

  // Create histograms per region (NoID & ID)
  // Fractions: 60 bins [0,1]
  auto Hfrac = [&](const char* name){ return new TH1D(name, name, 60, 0, 1); };
  // Multiplicities: choose ranges generous
  auto Hmult = [&](const char* name, int nb, double x0, double x1){ return new TH1D(name, name, nb, x0, x1); };

  const std::vector<std::string> vars = {"NHF","NEMF","MUF","CHF","CEMF","chMult","neMult","nConstituents"};

  // Indexing: [region][var] -> pair of hists
  TH1D* hNo[3][8]; TH1D* hId[3][8];

  for(int r=0;r<3;r++){
    for(int v=0;v<(int)vars.size();v++){
      std::string baseNo = "hNo_"+RegionTag((EtaRegion)r)+"_"+vars[v];
      std::string baseId = "hId_"+RegionTag((EtaRegion)r)+"_"+vars[v];

      if (vars[v]=="chMult" || vars[v]=="neMult") {
        hNo[r][v] = Hmult(baseNo.c_str(), 100, 0, 100);
        hId[r][v] = Hmult(baseId.c_str(), 100, 0, 100);
      } else if (vars[v]=="nConstituents") {
        hNo[r][v] = Hmult(baseNo.c_str(), 160, 0, 160);
        hId[r][v] = Hmult(baseId.c_str(), 160, 0, 160);
      } else {
        hNo[r][v] = Hfrac(baseNo.c_str());
        hId[r][v] = Hfrac(baseId.c_str());
      }
    }
  }

  // Loop events/jets
  const Long64_t nentries = tree->GetEntries();
  std::cout << "Entries: " << nentries << "\n";

  for(Long64_t i=0;i<nentries;i++){
    tree->GetEntry(i);
    if(!jetPt || !jetEta) continue;

    const size_t nj = jetPt->size();
    for(size_t j=0;j<nj;j++){
      float pt = jetPt->at(j);
      if (pt < ptCut) continue;

      float eta = jetEta->at(j);
      float abseta = std::fabs(eta);

      // Select only the 3 requested eta bins
      bool inBarrel  = (abseta < 2.6f);
      bool inEndcap  = (abseta > 2.7f && abseta < 3.0f);
      bool inForward = (abseta > 3.0f && abseta < 5.0f);
      if(!(inBarrel || inEndcap || inForward)) continue;

      EtaRegion reg = inBarrel ? BARREL : (inEndcap ? ENDCAP : FORWARD);

      // safety
      if(!jetNhf||j>=jetNhf->size()) continue;
      if(!jetNemf||j>=jetNemf->size()) continue;
      if(!jetMuf||j>=jetMuf->size()) continue;
      if(!jetChf||j>=jetChf->size()) continue;
      if(!jetCemf||j>=jetCemf->size()) continue;
      if(!chMult||j>=chMult->size()) continue;
      if(!neMult||j>=neMult->size()) continue;

      float nhf  = jetNhf->at(j);
      float nemf = jetNemf->at(j);
      float muf  = jetMuf->at(j);
      float chf  = jetChf->at(j);
      float cemf = jetCemf->at(j);
      int chm = chMult->at(j);
      int nem = neMult->at(j);
      int nConst = chm + nem;

      // Fill NoID
      hNo[reg][0]->Fill(nhf);
      hNo[reg][1]->Fill(nemf);
      hNo[reg][2]->Fill(muf);
      hNo[reg][3]->Fill(chf);
      hNo[reg][4]->Fill(cemf);
      hNo[reg][5]->Fill(chm);
      hNo[reg][6]->Fill(nem);
      hNo[reg][7]->Fill(nConst);

      // Fill TightID (full piecewise jetID)
      bool pass = PassJetID_Tight(abseta, nhf, nemf, chf, cemf, chm, nem);
      if(pass){
        hId[reg][0]->Fill(nhf);
        hId[reg][1]->Fill(nemf);
        hId[reg][2]->Fill(muf);
        hId[reg][3]->Fill(chf);
        hId[reg][4]->Fill(cemf);
        hId[reg][5]->Fill(chm);
        hId[reg][6]->Fill(nem);
        hId[reg][7]->Fill(nConst);
      }
    }
  }

  // Save overlays with cut lines
  for(int r=0;r<3;r++){
    for(int v=0;v<(int)vars.size();v++){
      std::string outPdf = std::string(outDir) + "/" + vars[v] + "_overlay_" + RegionTag((EtaRegion)r) + "_pt30_logy.pdf";
      DrawOverlayWithCut(hNo[r][v], hId[r][v], vars[v], (EtaRegion)r, outPdf.c_str());
    }
  }

  // Save ROOT output
  TFile fout("out_hist_overlay_etaBins.root","RECREATE");
  for(int r=0;r<3;r++){
    for(int v=0;v<(int)vars.size();v++){
      hNo[r][v]->Write();
      hId[r][v]->Write();
    }
  }
  fout.Close();

  std::cout << "DONE.\nPDFs in: " << outDir << "\nROOT: out_hist_overlay_etaBins.root\n";
}

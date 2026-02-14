#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <TCanvas.h>
#include <TSystem.h>
#include <vector>
#include <iostream>

void jets_histograms() {

    // ===== ΡΥΘΜΙΣΕΙΣ =====
    const char* inputFile = "run3_MinimumBias_runC_reduced_tree.root";
    const char* treeName  = "tree";
    const char* outDir    = "C:/work/ROOT/plots";
    const float ptCut     = 30.0;
    // ====================

    gSystem->mkdir(outDir, kTRUE);

    TFile *f = TFile::Open(inputFile);
    if (!f || f->IsZombie()) {
        std::cout << "ERROR: cannot open file\n";
        return;
    }

    TTree *tree = (TTree*)f->Get(treeName);
    if (!tree) {
        std::cout << "ERROR: tree not found\n";
        return;
    }

    // ---- Branches ----
    std::vector<float> *jetPt   = nullptr;
    std::vector<float> *jetNhf  = nullptr;
    std::vector<float> *jetNemf = nullptr;
    std::vector<float> *jetMuf  = nullptr;
    std::vector<float> *jetChf  = nullptr;
    std::vector<float> *jetCemf = nullptr;
    std::vector<int>   *chMult  = nullptr;
    std::vector<int>   *neMult  = nullptr;

    tree->SetBranchAddress("jetPt",   &jetPt);
    tree->SetBranchAddress("jetNhf",  &jetNhf);
    tree->SetBranchAddress("jetNemf", &jetNemf);
    tree->SetBranchAddress("jetMuf",  &jetMuf);
    tree->SetBranchAddress("jetChf",  &jetChf);
    tree->SetBranchAddress("jetCemf", &jetCemf);
    tree->SetBranchAddress("chMult",  &chMult);
    tree->SetBranchAddress("neMult",  &neMult);

    // ---- Histograms ----
    TH1D *h_NHF  = new TH1D("h_NHF",  "NHF;fraction;jets", 50, 0, 1);
    TH1D *h_NEMF = new TH1D("h_NEMF", "NEMF;fraction;jets",50, 0, 1);
    TH1D *h_MUF  = new TH1D("h_MUF",  "MUF;fraction;jets", 50, 0, 1);
    TH1D *h_CHF  = new TH1D("h_CHF",  "CHF;fraction;jets", 50, 0, 1);
    TH1D *h_CEMF = new TH1D("h_CEMF", "CEMF;fraction;jets",50, 0, 1);

    TH1D *h_chMult = new TH1D("h_chMult","Charged multiplicity;N;jets",40,0,40);
    TH1D *h_neMult = new TH1D("h_neMult","Neutral multiplicity;N;jets",40,0,40);
    TH1D *h_nConst = new TH1D("h_nConst","N constituents;N;jets",60,0,60);

    // ---- Loop ----
    Long64_t nentries = tree->GetEntries();
    for (Long64_t i = 0; i < nentries; ++i) {
        tree->GetEntry(i);

        for (size_t j = 0; j < jetPt->size(); ++j) {
            if (jetPt->at(j) < ptCut) continue;

            h_NHF ->Fill(jetNhf ->at(j));
            h_NEMF->Fill(jetNemf->at(j));
            h_MUF ->Fill(jetMuf ->at(j));
            h_CHF ->Fill(jetChf ->at(j));
            h_CEMF->Fill(jetCemf->at(j));

            h_chMult->Fill(chMult->at(j));
            h_neMult->Fill(neMult->at(j));
            h_nConst->Fill(chMult->at(j) + neMult->at(j));
        }
    }

    // ---- Save plots (PDF) ----
    auto Save = [&](TH1D* h, const char* name){
        TCanvas c("c","c",800,600);
        h->Draw("hist");
        c.SaveAs(Form("%s/%s.pdf", outDir, name));
    };

    Save(h_NHF,"NHF_pt30");
    Save(h_NEMF,"NEMF_pt30");
    Save(h_MUF,"MUF_pt30");
    Save(h_CHF,"CHF_pt30");
    Save(h_CEMF,"CEMF_pt30");
    Save(h_chMult,"chMult_pt30");
    Save(h_neMult,"neMult_pt30");
    Save(h_nConst,"nConstituents_pt30");

    // ---- Save ROOT file ----
    TFile fout("out_hist.root","RECREATE");
    h_NHF->Write(); h_NEMF->Write(); h_MUF->Write();
    h_CHF->Write(); h_CEMF->Write();
    h_chMult->Write(); h_neMult->Write(); h_nConst->Write();
    fout.Close();

    std::cout << "DONE. Plots in " << outDir << std::endl;
}

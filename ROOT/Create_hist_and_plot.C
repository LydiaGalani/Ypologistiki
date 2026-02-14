#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <TCanvas.h>
#include <iostream>
#include <vector>

void Create_hist_and_plot() {

   
    TFile *f = TFile::Open("run3_MinimumBias_runC_reduced_tree.root");
    if (!f || f->IsZombie()) {
        std::cout << "Error opening file!" << std::endl;
        return;
    }

   
    TTree *tree = (TTree*)f->Get("tree");
    if (!tree) {
        std::cout << "Tree 'tree' not found!" << std::endl;
        f->ls();
        return;
    }


    std::vector<float> *jpt = 0;
    std::vector<float> *chf = 0;
    std::vector<float> *nhf = 0;

    tree->SetBranchAddress("jetPt",  &jpt);
    tree->SetBranchAddress("jetChf", &chf);
    tree->SetBranchAddress("jetNhf", &nhf);  

    // ===== Histogram =====
    TH1D *h_CHF = new TH1D("h_CHF", "CHF for jets with p_{T} > 30 GeV;CHF;Jets", 50, 0, 1);

    // ===== Event loop =====
    Long64_t nentries = tree->GetEntries();
    std::cout << "Number of total entries = " << nentries << std::endl;

    for (Long64_t i = 0; i < nentries; i++) {

        tree->GetEntry(i);

        int no_jets = (jpt ? (int)jpt->size() : 0);

        for (int j = 0; j < no_jets; ++j) {

            // Cut: jet pT > 30
            if (jpt->at(j) > 30) {
                h_CHF->Fill(chf->at(j));
            }

        } // end jet loop
    } // end event loop

    // ===== Draw + save =====
    TCanvas *c = new TCanvas("c", "CHF", 700, 600);
       h_CHF->Draw("hist");
    c->SaveAs("CHF_pt30.png");
   
    // ===== Save to ROOT file (optional) =====
    TFile *fout = new TFile("out_hist.root", "RECREATE");
    h_CHF->Write();
    fout->Close();

    std::cout << "Done: CHF_pt30.png and out_hist.root created" << std::endl;
}

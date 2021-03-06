#include <gtest/gtest.h>

#include "enrich_functions.h"

#include "agent_tests.h"
#include "context.h"
#include "facility_tests.h"

namespace mbmore {

  // Benchmarked against mbmore_enrich_compare.ipynb  
  // https://github.com/mbmcgarry/data_analysis/tree/master/enrich_calcs
  namespace enrichfunctiontests {
    // Fixed for a cascade separating out U235 from U238 in UF6 gas
    const double M = 0.352; // kg/mol UF6
    const double dM = 0.003; // kg/mol U238 - U235
    const double x = 1000;  // Pressure ratio (Glaser)
    
    // General cascade assumptions
    const double flow_internal = 2.0 ;  
    const double eff = 1.0;  
    const double cut = 0.5;  
    
    // Centrifuge params used in Python test code
    // (based on Glaser SGS 2009 paper)
    const double v_a = 485; // m/s
    const double height = 0.5; // meters
    const double diameter = 0.15; // meters
    const double feed_m = 15 * 60 * 60 / ((1e3) * 60 * 60 * 1000.0); // kg/sec
    const double temp = 320.0 ; //Kelvin
    
    // Cascade params used in Python test code (Enrichment_Calculations.ipynb)
    const double feed_assay = 0.0071;
    const double product_assay = 0.035;
    const double waste_assay = 0.001;
    const double feed_c = 739 / (30.4 * 24 * 60 * 60); // kg/month -> kg/sec
    const double product_c = 77 / (30.4 * 24 * 60 * 60); // kg/month -> kg/sec

    //del U=7.0323281e-08 alpha=1.16321
    double delU = CalcDelU(v_a, height, diameter, feed_m, temp, cut, eff,
			   M, dM, x, flow_internal);
    
    double alpha = AlphaBySwu(delU, feed_m, cut, M);    
    const double tol_assay = 1e-5;
    const double tol_qty = 1e-6;
    const double tol_num = 1e-2;
   
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Find product assay from separation factor alpha
TEST(Enrich_Functions_Test, TestAssays) {

  double cur_alpha = 1.4;
  double cur_f_assay = 0.007;

  double cpp_assay = ProductAssayByAlpha(cur_alpha, cur_f_assay);
  
  double pycode_assay = 0.009772636;
  double tol = 1e-6;
  
  EXPECT_NEAR(cpp_assay, pycode_assay, tol);

  double n_stages = 5;
  double pycode_w_assay = 0.00095311 ;

  double cpp_w_assay = WasteAssayFromNStages(cur_alpha, cur_f_assay, n_stages);
  
  EXPECT_NEAR(cpp_w_assay, pycode_w_assay, tol);

}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Calculate ideal SWU params of single machine (separation potential delU
// and separation factor alpha)
TEST(Enrich_Functions_Test, TestSWU) {

  double pycode_U = 7.03232816847e-08;
  double tol = 1e-9;
  
  EXPECT_NEAR(delU, pycode_U, tol);

  double pycode_alpha = 1.16321;
  double tol_alpha = 1e-2;
  EXPECT_NEAR(alpha, pycode_alpha, tol_alpha);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Ideal cascade design, and then using away from ideal design

TEST(Enrich_Functions_Test, TestCascade) {
  double n_machines = MachinesPerCascade(delU, product_assay,
					 waste_assay, feed_c, product_c);

  double pycode_n_mach = 25990.392;
  EXPECT_NEAR(n_machines, pycode_n_mach, tol_num);

  std::pair<double, double> n_stages = FindNStages(alpha, feed_assay,
						   product_assay,
						   waste_assay);
  int pycode_n_enrich_stage = 11;
  int pycode_n_strip_stage = 13;

 
  //  int n_stage_enrich = (int) n_stages.first + 1;  // Round up to next integer
  //  int n_stage_waste = (int) n_stages.second + 1;  // Round up to next integer
  int n_stage_enrich = n_stages.first;
  int n_stage_waste = n_stages.second;

  EXPECT_EQ(n_stage_enrich, pycode_n_enrich_stage);
  EXPECT_EQ(n_stage_waste, pycode_n_strip_stage);

 // Now test assays when cascade is modified away from ideal design
  // (cascade optimized for natural uranium feed, now use 20% enriched
  double feed_assay_mod = 0.20;

  double mod_product_assay = ProductAssayFromNStages(alpha, feed_assay_mod,
						     n_stage_enrich);
  double mod_waste_assay = WasteAssayFromNStages(alpha, feed_assay_mod,
						 n_stage_waste);

  std::cout << "alpha " << alpha << " feed " << feed_assay_mod << " nstage " << n_stage_enrich << " unrounded stages " << n_stages.first << std::endl;
  double pycode_mod_product_assay = 0.60085;
  double pycode_mod_waste_assay = 0.0290846;
  EXPECT_NEAR(mod_product_assay, pycode_mod_product_assay, tol_assay);
  EXPECT_NEAR(mod_waste_assay, pycode_mod_waste_assay, tol_assay);
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Determine the output of the first enrich/strip stage of a cascade
  // based on the design params for the cascade
  TEST(Enrich_Functions_Test, TestStages) {
    double product_assay_s = ProductAssayByAlpha(alpha, feed_assay);
    double n_mach_e = MachinesPerStage(alpha, delU, feed_c);
    double product_s = ProductPerEnrStage(alpha, feed_assay,
					product_assay_s, feed_c);

    double enrich_waste = feed_c - product_s;
    double enrich_waste_assay = (feed_c * feed_assay - product_s *
				 product_assay_s)/enrich_waste;

    double pycode_product_assay_s = 0.0082492;
    double pycode_n_mach_e = 53.287;
    double pycode_product_s = 0.0001408;

    EXPECT_NEAR(n_mach_e, pycode_n_mach_e, tol_num);
    EXPECT_NEAR(product_assay_s, pycode_product_assay_s, tol_assay);
    EXPECT_NEAR(product_s, pycode_product_s, tol_qty);

    double n_mach_w = MachinesPerStage(alpha, delU, enrich_waste);
    double strip_waste_assay = WasteAssayByAlpha(alpha, enrich_waste_assay);

    // This AVERY EQN doesn't work for some reason
    //    double strip_waste = WastePerStripStage(alpha, enrich_waste_assay,
    //					    strip_waste_assay, enrich_waste);

    double pycode_n_mach_w = 26.6127;
    double pycode_waste_assay_s = 0.005117;
    //    double pycode_waste_s = 8.60660553717e-05;

    EXPECT_NEAR(n_mach_w, pycode_n_mach_w, tol_num);
    EXPECT_NEAR(strip_waste_assay, pycode_waste_assay_s, tol_assay);
    //    EXPECT_NEAR(strip_waste, pycode_waste_s, tol_qty);
    
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// tests the steady state flow rates for a cascade
//
TEST(Enrich_Functions_Test, TestCascadeDesign) {
  double fa = 0.10;
  double pa = 0.20;
  double wa = 0.05;

  std::vector<double> pycode_flows = {0.00030693,  0.00061387,  0.0009208 ,
				      0.00122774,  0.00153467,  0.00127889,
				      0.00102311,  0.00076734,  0.00051156,
				      0.00025578};

  
  std::vector<int> pycode_machines={59, 117, 175, 233, 291, 243, 194,
				    146, 97, 49};

  std::pair<double, double> n_stages = FindNStages(alpha, fa, pa, wa);
  std::vector<double> flows = CalcFeedFlows(n_stages, feed_c, cut);

  // if # Machines for the stage is within tol_num of an integer
  // then round down. Otherwise round up to the next integer machine to
  // preserve steady-state flow calculations.
  std::vector<std::pair<int, double>> stage_info =
    CalcStageFeatures(fa, alpha, delU, cut, n_stages, flows);

  for (int i = 0; i < pycode_flows.size(); i++){
    EXPECT_NEAR(flows[i], pycode_flows[i], tol_num);
    int nmach = stage_info[i].first;
    EXPECT_EQ(nmach, pycode_machines[i]);
  }

  // not enough machines
  int max_centrifuges = 80;
  std::pair<int, double> design_params = DesignCascade(feed_c, alpha, delU,
						       cut, max_centrifuges,
						       n_stages);
  int py_tot_mach = 79;
  double py_opt_feed = 1.30116169899e-05;
  
  EXPECT_EQ(py_tot_mach, design_params.first);
  EXPECT_NEAR(py_opt_feed, design_params.second, tol_qty);
  
  // more machines than requested capacity
  max_centrifuges = 1000;
  design_params = DesignCascade(feed_c, alpha, delU,
				cut, max_centrifuges,
				n_stages);
  py_tot_mach = 986;
  py_opt_feed = 0.000172728;
  
  EXPECT_EQ(py_tot_mach, design_params.first);
  EXPECT_NEAR(py_opt_feed, design_params.second, tol_qty);
  
}
  
  
  } // namespace enrichfunctiontests
} // namespace mbmore
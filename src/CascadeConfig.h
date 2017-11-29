#ifndef MBMORE_SRC_CASCADE_H_
#define MBMORE_SRC_CASCADE_H_

#include <map>
#include <string>

#include "CentrifugeConfig.h"
#include "StageConfig.h"

namespace mbmore {
// LAPACK solver for system of linear equations
extern "C" {
void dgesv_(int *n, int *nrhs, double *a, int *lda, int *ipivot, double *b,
            int *ldb, int *info);
}

class CascadeConfig {
 public:
  CascadeConfig() { ; }
  CascadeConfig(double f_assay, double p_assay, double t_assay,
                double max_feed_flow, int max_centrifuge);
  void BuildIdealCascade(double f_assay, double p_assay, double w_assay,
                         double precision = 1e-16);
  int FindTotalMachines();
  void CalcFeedFlows();
  void CalcStageFeatures();
  void DesignCascade(double max_feed, int max_centrifuges);
  CascadeConfig Compute_Assay(double feed_assay, double precision);

  // Number of machines in the cascade given the target feed rate and target
  // assays and flow rates
  // Avery 62
  double MachinesPerCascade(double del_U_machine, double product_assay,
                            double waste_assay, double feed_flow,
                            double product_flow);

  double FeedFlow() { return feed_flow; }
  CentrifugeConfig centrifuge;
  std::map<int, StageConfig> stgs_config;
  int n_enrich;
  int n_strip;

 private:
  int n_machines;
  double feed_flow;
  double feed_assay;
  double design_product_assay;
  double design_tail_assay;

  double Diff_enrichment(CascadeConfig actual_enrichments,
                         CascadeConfig previous_enrichement);

  std::map<int, StageConfig> Update_enrichment(CascadeConfig cascade,
                                               double feed_assay);
};

}  // namespace mbmore

#endif  // MBMORE_SRC_CASCADE_H_

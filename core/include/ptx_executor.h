//
// Created by gevtushenko on 7/11/21.
//

#ifndef PTXSYNTHESIST_PTX_EXECUTOR_H
#define PTXSYNTHESIST_PTX_EXECUTOR_H

#include <vector>
#include <memory>

#include "kernel_param.h"

struct PTXExecutorImpl;

class Measurement
{
  float min_time {};
  float median_time {};
  float max_time {};

  std::string name;
  std::vector<float> elapsed_times;

public:
  Measurement(const std::string &name,
              std::vector<float> &&elapsed_times);

  [[nodiscard]] const char *get_name() const { return name.c_str(); }
  [[nodiscard]] float get_min() const { return min_time; }
  [[nodiscard]] float get_median() const { return median_time; }
  [[nodiscard]] float get_max() const { return max_time; }
  [[nodiscard]] const std::vector<float>& get_elapsed_times() { return elapsed_times; }
};

class PTXExecutor
{
  int device_id {};
  int device_count {};

  std::unique_ptr<char[]> device_name;

public:
  PTXExecutor(int device_id, bool initialize = false);
  ~PTXExecutor();

  int get_device_count() const { return device_count; }
  const char *get_device_name() const;

  std::vector<Measurement> execute(const std::vector<KernelParameter> &params,
                                   const char *code);

private:
  std::unique_ptr<PTXExecutorImpl> impl;
};

#endif //PTXSYNTHESIST_PTX_EXECUTOR_H

#ifndef DPU_CPL_HPP
#define DPU_CPL_HPP
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace MetaPB {
/// @brief A stateless dpu compile proxy.
class DPUCompileProxy {
public:
  struct compileArgs {
    int optimizationLevel = 2;           // Set -O2 as default.
    int taskletNum = 12;                 // Aligned with SimplePIM
    std::string outputFilePath{"./foo"}; // A meanless placeholder.
    std::vector<std::string> postfixes =
        {}; // Postfixes that able to be customized by user.
  };
  void compileWith(compileArgs &args) const noexcept;

private:
  enum compileToken {
    compilerBin, // DPU compiler's binary name, set to constant
    optimizationLevel,
    taskletNum, // NR_TASKLET by convention, in SimplePIM fixed to constant 12.
    includeDir, // Explicit specified header directories.
    outputFilePath, // Single output binary path.
  };
  static inline std::map<compileToken, std::string> tokenLUT = {
      {compilerBin, "dpu-upmem-dpurte-clang"},
      {optimizationLevel, " -O"},
      {taskletNum, " -DNR_TASKLETS="},
      {outputFilePath, " -o"}};
  std::string parse(compileArgs &args) const noexcept;
}; // DPUCompileProxy

} // namespace MetaPB 
#endif

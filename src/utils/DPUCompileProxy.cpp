#ifndef DPU_CPL_SRC
#define DPU_CPL_SRC
#include "utils/DPUCompileProxy.hpp"

namespace utils{

/// @brief Compile dpu program with provided arguments
/// @param args Provided packed arguments.
void DPUCompileProxy::compileWith(compileArgs &args)const noexcept{
  auto cmd = parse(args);
  auto ret = std::system(cmd.c_str());
  // TODO: add error handling.
  if(ret){
    std::cerr<< "encountered error while compiling.";
  }
}


/// @brief Parse packed input args and return actual shell command.
/// @param args Input args
/// @return Shell command ( assuming dev environment exist ).
std::string DPUCompileProxy::parse(compileArgs &args)const noexcept{
  std::string cmd = tokenLUT[compilerBin]; // Begin with dpu compiler binary.
  cmd += (tokenLUT[optimizationLevel] + std::to_string(args.optimizationLevel));
  cmd += (tokenLUT[taskletNum] + std::to_string(args.taskletNum));
  cmd += (tokenLUT[outputFilePath] + args.outputFilePath);

  // For extensible postfixes.
  for(auto&& postfix : args.postfixes){
    cmd += (" " + postfix);
  }
  return cmd;
}
} // namespace utils
#endif
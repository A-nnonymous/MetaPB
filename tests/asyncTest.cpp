#include <stdexec/execution.hpp>
#include <exec/static_thread_pool.hpp>
#include <vector>
using std::vector;


template<typename T>
void VA(vector<T> &dst, const vector<T> &src0, const vector<T> &src1){
  const size_t vLen = src0.size();
  #pragma omp parallel for
  #pragma unroll 8
  for(size_t i = 0; i != vLen; ++i){
    dst[i] = src0[i] + src1[i];
  }
}

template<typename T>
void VM(vector<T> &dst, const vector<T> &src0, const vector<T> &src1){
  const size_t vLen = src0.size();

  #pragma omp parallel for
  #pragma unroll 8
  for(size_t i = 0; i != vLen; ++i){
    dst[i] = src0[i] * src1[i];
  }
}

int main()
{
    //TODO: Use stdexec to perform a fine grain asynchronous 
    //      triangle Operator DAG computing  DST = (SRC0 + SRC1) * (SRC2 + SRC3)
    exec::static_thread_pool pool(std::thread::hardware_concurrency());

    // Get a handle to the thread pool:
    auto sched = pool.get_scheduler();

    // Describe some work:
    // Creates 3 sender pipelines that are executed concurrently by passing to `when_all`
    // Each sender is scheduled on `sched` using `on` and starts with `just(n)` that creates a
    // Sender that just forwards `n` to the next sender.
    // After `just(n)`, we chain `then(fun)` which invokes `fun` using the value provided from `just()`
    // Note: No work actually happens here. Everything is lazy and `work` is just an object that statically
    // represents the work to later be executed
    auto fun = [](int i) { return i*i; };
    auto work = stdexec::when_all(
        stdexec::on(sched, stdexec::just(0) | stdexec::then(fun)),
        stdexec::on(sched, stdexec::just(1) | stdexec::then(fun)),
        stdexec::on(sched, stdexec::just(2) | stdexec::then(fun))
    );
    // Launch the work and wait for the result
    auto [i, j, k] = stdexec::sync_wait(std::move(work)).value();

    // Print the results:
    std::printf("%d %d %d\n", i, j, k);

}

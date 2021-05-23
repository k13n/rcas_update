#ifndef BENCHMARK_PERF_WRAPPER_HPP_
#define BENCHMARK_PERF_WRAPPER_HPP_

#include <sstream>
#include <iostream>
#include <functional>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <chrono>
#include <thread>


namespace benchmark {

// thanks to:
// https://muehe.org/posts/profiling-only-parts-of-your-code-with-perf/
struct PerfWrapper {

  static void profile(const std::string& name, std::function<void()> body) {
    std::string filename = name.find(".data") == std::string::npos ? (name + ".data") : name;

    // Launch profiler
    pid_t pid;
    std::stringstream pid_s;
    pid_s << getpid();
    pid = fork();
    if (pid == 0) {
      auto fd=open("/dev/null",O_RDWR);
      dup2(fd,1);
      dup2(fd,2);
      exit(execl("/usr/bin/perf","perf","record",
            /* "--user-callchains", */
            /* "--event", "task-clock,cycles,instructions,cache-references,cache-misses", */
            /* "--event", "cache-references,cache-misses", */
            /* "--event", "instructions,cache-misses", */
            /* "--event", "task-clock,instructions,cycles,cache-references,cache-misses", */
            "--event", "task-clock,instructions,cycles,cache-references,cache-misses,branch-instructions,branch-misses",
            "--output", filename.c_str(),
            "-g",
            "--pid", pid_s.str().c_str(),
            nullptr));
    }


    std::this_thread::sleep_for(std::chrono::seconds{5});
    // Run body
    body();
    std::this_thread::sleep_for(std::chrono::seconds{5});

    // Kill profiler
    kill(pid,SIGINT);
    waitpid(pid,nullptr,0);
  }

  static void profile(std::function<void()> body) {
    profile("perf.data",body);
  }
};

}; // namespace benchmark

#endif // BENCHMARK_PERF_WRAPPER_HPP_

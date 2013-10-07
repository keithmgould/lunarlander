#include <numeric>
#include <boost/nondet_random.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <cstdlib>
#include <cstdio>
#include <vector>

#include "utility.hpp"
#include "simulator.hpp"
#include "lunar_lander_agent.hpp"
#include "framework.hpp"

int main (int argc, char* argv[]) {

  unsigned int seed = boost::random::random_device()();
  if (argc > 1) seed = std::atoi (argv[1]);
  //std::cout << "# Using seed: " << seed << std::endl;

  boost::random::mt19937 agent_rng(seed);
  boost::random::mt19937 init_rng(0);

  const double dt = 0.1;
  const int agent_time_steps = 5;

  const double lambda = 0.75;
  const double alpha_v = 0.1;
  const double alpha_u = 0.1;
  const double initial_value = 1;
  const int num_features = 1<<20;
  const double tile_weight_exponent = 0.5; // 1 for no weighting
  const bool trunc_normal = true;

  std::vector<int> subspaces;
  subspaces.push_back(0);
  subspaces.push_back(1);
  subspaces.push_back(2);
  subspaces.push_back(6);

  framework f(lunar_lander_simulator(),
              lunar_lander_agent(lambda, alpha_v, alpha_u, initial_value, num_features,
                                 tile_weight_exponent, trunc_normal, subspaces),
              dt,
              agent_time_steps);

  for (int i = 0; i < 20000; i++) {
    f.set_visualiser (i % 1000 == 0 ? stdout : 0);
    std::vector<double> rewards = f.run_episode(init_rng, agent_rng);
    //std::fprintf(stdout, "%g\n", f.get_return());
    // if (f.simulator.get_landed()) std::printf("1 ");
    // else std::printf("0 ");
    // std::printf("%g\n", f.time_elapsed);
  }



  // while (true) {
  //   std::vector<double> rewards = f.run_episode(rng);
  //   double Return = std::accumulate(rewards.begin(), rewards.end(), 0.0);
  //   std::fprintf(stderr, "Return: %g\n", Return);
  //   if (Return < -50) {
  //     for (unsigned int i = 0; i < rewards.size(); i++) {
  //       printf("%g\n", rewards[i]);
  //     }
  //     break;
  //   }
  // }

  return 0;
}

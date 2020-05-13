#include "run.hpp"

#include <armadillo>
#include <cassert>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <random>
#include <regex>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#define EPSILON 0.00000001

void
Sim::initializeParameters(void)
{
  // BM settings
  lambda_reg1 = 0.01;
  lambda_reg2 = 0.01;
  step_max = 2000;
  error_max = 0.00001;
  save_parameters = 20;
  // step_check = step_max;
  random_seed = 1;

  // Learning rate settings
  epsilon_0_h = 0.01;
  epsilon_0_J = 0.001;
  adapt_up = 1.5;
  adapt_down = 0.6;
  min_step_h = 0.001;
  max_step_h = 2.5;
  min_step_J = 0.00001;
  max_step_J_N = 2.5; // divide by N later
  error_min_update = -1;

  // sampling time settings
  t_wait_0 = 10000;
  delta_t_0 = 100;
  check_ergo = true;
  adapt_up_time = 1.5;
  adapt_down_time = 0.600;

  output_binary = true;

  // importance sampling settings
  step_importance_max = 1;
  coherence_min = 0.9999;

  // mcmc settings
  M = 1000;            // importance sampling max iterations
  count_max = 10;      // number of independent MCMC runs
  init_sample = false; // flag to load first position for mcmc seqs
  temperature = 1.0;   // temperature at which to sample mcmc
};

void
Sim::checkParameters(void)
{
  // Ensure that the set of ergodiciy checks is disabled if M=1
  if ((M == 1) && check_ergo) {
    check_ergo = false;
    std::cerr << "WARNING: disabling 'check_ergo' when M=1." << std::endl;
  }
}

void
Sim::writeParameters(std::string output_file)
{
  std::ofstream stream(output_file);

  // Header
  stream << "[bmDCA]" << std::endl;

  // BM settings
  stream << "lambda_reg1=" << lambda_reg1 << std::endl;
  stream << "lambda_reg2=" << lambda_reg2 << std::endl;
  stream << "step_max=" << step_max << std::endl;
  stream << "error_max=" << error_max << std::endl;
  stream << "save_parameters=" << save_parameters << std::endl;
  // stream << "step_check=" << step_check << std::endl;
  stream << "random_seed=" << random_seed << std::endl;

  // Learning rate settings
  stream << "epsilon_0_h=" << epsilon_0_h << std::endl;
  stream << "epsilon_0_J=" << epsilon_0_J << std::endl;
  stream << "adapt_up=" << adapt_up << std::endl;
  stream << "adapt_down=" << adapt_down << std::endl;
  stream << "min_step_h=" << min_step_h << std::endl;
  stream << "max_step_h=" << max_step_h << std::endl;
  stream << "min_step_J=" << min_step_J << std::endl;
  stream << "max_step_J_N=" << max_step_J_N << std::endl;
  stream << "error_min_update=" << error_min_update << std::endl;

  // sampling time settings
  stream << "t_wait_0=" << t_wait_0 << std::endl;
  stream << "delta_t_0=" << delta_t_0 << std::endl;
  stream << "check_ergo=" << check_ergo << std::endl;
  stream << "adapt_up_time=" << adapt_up_time << std::endl;
  stream << "adapt_down_time=" << adapt_down_time << std::endl;

  // importance sampling settings
  stream << "step_importance_max=" << step_importance_max << std::endl;
  stream << "coherence_min=" << coherence_min << std::endl;

  // mcmc settings
  stream << "M=" << M << std::endl;
  stream << "count_max=" << count_max << std::endl;
  stream << "init_sample=" << init_sample << std::endl;
  stream << "init_sample_file=" << init_sample_file << std::endl;
  stream << "temperature=" << temperature << std::endl;

  // // check routine settings
  // stream << "t_wait_check=" << t_wait_check << std::endl;
  // stream << "delta_t_check=" << delta_t_check << std::endl;
  // stream << "M_check=" << M_check << std::endl;
  // stream << "count_check=" << count_check << std::endl;

  stream << "output_binary=" << output_binary << std::endl;
};

bool
Sim::compareParameters(std::string file_name)
{
  std::ifstream file(file_name);
  bool reading_bmdca_section = false;
  bool all_same = true;
  if (file.is_open()) {
    std::string line;
    while (std::getline(file, line)) {
      if (line[0] == '#' || line.empty()) {
        continue;
      } else if (line[0] == '[') {
        if (line == "[bmDCA]") {
          reading_bmdca_section = true;
          continue;
        } else {
          reading_bmdca_section = false;
          continue;
        }
      }
      if (reading_bmdca_section) {
        auto delim_pos = line.find("=");
        auto key = line.substr(0, delim_pos);
        auto value = line.substr(delim_pos + 1);
        all_same = all_same & compareParameter(key, value);
      }
    }
  } else {
    std::cerr << "ERROR: " << file_name << " not found." << std::endl;
    std::exit(EXIT_FAILURE);
  }
  return all_same;
};

void
Sim::loadParameters(std::string file_name)
{
  std::ifstream file(file_name);
  bool reading_bmdca_section = false;
  if (file.is_open()) {
    std::string line;
    while (std::getline(file, line)) {
      if (line[0] == '#' || line.empty()) {
        continue;
      } else if (line[0] == '[') {
        if (line == "[bmDCA]") {
          reading_bmdca_section = true;
          continue;
        } else {
          reading_bmdca_section = false;
          continue;
        }
      }
      if (reading_bmdca_section) {
        auto delim_pos = line.find("=");
        auto key = line.substr(0, delim_pos);
        auto value = line.substr(delim_pos + 1);
        setParameter(key, value);
      }
    }
  } else {
    std::cerr << "ERROR: " << file_name << " not found." << std::endl;
    std::exit(EXIT_FAILURE);
  }
};

bool
Sim::compareParameter(std::string key, std::string value)
{
  bool same = true;
  // It's not possible to use switch blocks on strings because they are char*
  // arrays, not actual types.
  if (key == "lambda_reg1") {
    same = same & (lambda_reg1 == std::stod(value));
  } else if (key == "lambda_reg2") {
    same = same & (lambda_reg2 == std::stod(value));
  } else if (key == "step_max") {
    // same = same & (step_max == std::stoi(value));
  } else if (key == "error_max") {
    // same = same & (error_max == std::stod(value));
  } else if (key == "save_parameters") {
    // same = same & (save_parameters == std::stoi(value));
  } else if (key == "random_seed") {
    same = same & (random_seed == std::stoi(value));
  } else if (key == "epsilon_0_h") {
    same = same & (epsilon_0_h == std::stod(value));
  } else if (key == "epsilon_0_J") {
    same = same & (epsilon_0_J == std::stod(value));
  } else if (key == "adapt_up") {
    same = same & (adapt_up == std::stod(value));
  } else if (key == "adapt_down") {
    same = same & (adapt_down == std::stod(value));
  } else if (key == "min_step_h") {
    same = same & (min_step_h == std::stod(value));
  } else if (key == "max_step_h") {
    same = same & (max_step_h == std::stod(value));
  } else if (key == "min_step_J") {
    same = same & (min_step_J == std::stod(value));
  } else if (key == "max_step_J_N") {
    same = same & (max_step_J_N == std::stod(value));
  } else if (key == "error_min_update") {
    same = same & (error_min_update == std::stod(value));
  } else if (key == "t_wait_0") {
    same = same & (t_wait_0 == std::stoi(value));
  } else if (key == "delta_t_0") {
    same = same & (delta_t_0 == std::stoi(value));
  } else if (key == "check_ergo") {
    if (value.size() == 1) {
      same = same & (check_ergo == (std::stoi(value) == 1));
    } else {
      same = same & (check_ergo == (value == "true"));
    }
  } else if (key == "adapt_up_time") {
    same = same & (adapt_up_time == std::stod(value));
  } else if (key == "adapt_down_time") {
    same = same & (adapt_down_time == std::stod(value));
  } else if (key == "step_importance_max") {
    same = same & (step_importance_max == std::stoi(value));
  } else if (key == "coherence_min") {
    same = same & (coherence_min == std::stod(value));
  } else if (key == "M") {
    same = same & (M == std::stoi(value));
  } else if (key == "count_max") {
    same = same & (count_max == std::stoi(value));
  } else if (key == "init_sample") {
    if (value.size() == 1) {
      same = same & (init_sample == (std::stoi(value) == 1));
    } else {
      same = same & (init_sample == (value == "true"));
    }
  } else if (key == "init_sample_file") {
    same = same & (init_sample_file == value);
  } else if (key == "temperature") {
    same = same & (temperature == std::stod(value));
  } else if (key == "output_binary") {
    if (value.size() == 1) {
      same = same & (output_binary == (std::stoi(value) == 1));
    } else {
      same = same & (output_binary == (value == "true"));
    }
  } else {
    std::cerr << "ERROR: unknown parameter '" << key << "'" << std::endl;
    std::exit(EXIT_FAILURE);
  }
  return same;
};

void
Sim::setParameter(std::string key, std::string value)
{
  // It's not possible to use switch blocks on strings because they are char*
  // arrays, not actual types.
  if (key == "lambda_reg1") {
    lambda_reg1 = std::stod(value);
  } else if (key == "lambda_reg2") {
    lambda_reg2 = std::stod(value);
  } else if (key == "step_max") {
    step_max = std::stoi(value);
  } else if (key == "error_max") {
    error_max = std::stod(value);
  } else if (key == "save_parameters") {
    save_parameters = std::stoi(value);
  } else if (key == "random_seed") {
    random_seed = std::stoi(value);
  } else if (key == "epsilon_0_h") {
    epsilon_0_h = std::stod(value);
  } else if (key == "epsilon_0_J") {
    epsilon_0_J = std::stod(value);
  } else if (key == "adapt_up") {
    adapt_up = std::stod(value);
  } else if (key == "adapt_down") {
    adapt_down = std::stod(value);
  } else if (key == "min_step_h") {
    min_step_h = std::stod(value);
  } else if (key == "max_step_h") {
    max_step_h = std::stod(value);
  } else if (key == "min_step_J") {
    min_step_J = std::stod(value);
  } else if (key == "max_step_J_N") {
    max_step_J_N = std::stod(value);
  } else if (key == "error_min_update") {
    error_min_update = std::stod(value);
  } else if (key == "t_wait_0") {
    t_wait_0 = std::stoi(value);
  } else if (key == "delta_t_0") {
    delta_t_0 = std::stoi(value);
  } else if (key == "check_ergo") {
    if (value.size() == 1) {
      check_ergo = (std::stoi(value) == 1);
    } else {
      check_ergo = (value == "true");
    }
  } else if (key == "adapt_up_time") {
    adapt_up_time = std::stod(value);
  } else if (key == "adapt_down_time") {
    adapt_down_time = std::stod(value);
  } else if (key == "step_importance_max") {
    step_importance_max = std::stoi(value);
  } else if (key == "coherence_min") {
    coherence_min = std::stod(value);
  } else if (key == "M") {
    M = std::stoi(value);
  } else if (key == "count_max") {
    count_max = std::stoi(value);
  } else if (key == "init_sample") {
    if (value.size() == 1) {
      init_sample = (std::stoi(value) == 1);
    } else {
      init_sample = (value == "true");
    }
  } else if (key == "init_sample_file") {
    init_sample_file = value;
  } else if (key == "temperature") {
    temperature = std::stod(value);
  } else if (key == "output_binary") {
    if (value.size() == 1) {
      output_binary = (std::stoi(value) == 1);
    } else {
      output_binary = (value == "true");
    }
  } else {
    std::cerr << "ERROR: unknown parameter '" << key << "'" << std::endl;
    std::exit(EXIT_FAILURE);
  }
};

Sim::Sim(MSAStats msa_stats,
         std::string config_file,
         std::string dest_dir,
         bool force_restart)
  : msa_stats(msa_stats)
{

  initializeParameters();
  if (!config_file.empty()) {
    std::cout << "loading hyperparameters... " << std::flush;
    loadParameters(config_file);
    std::cout << "done." << std::endl;
  } else if ((!force_restart) &
             (checkFileExists(dest_dir + "/" + hyperparameter_file))) {
    std::cout << "loading previously used hyperparameters..." << std::flush;
    loadParameters(dest_dir + "/" + hyperparameter_file);
    std::cout << "done." << std::endl;
  }
  checkParameters();

  if (!dest_dir.empty()) {
    chdir(dest_dir.c_str());
  }

  if ((!force_restart) & (checkFileExists(hyperparameter_file))) {
    if (!compareParameters(hyperparameter_file)) {
      std::cerr << "ERROR: current and previous hyperparameters mismatched. "
                   "restarting... "
                << std::endl;
      step_offset = 0;
      clearFiles(".");
      writeParameters(hyperparameter_file);
    } else {
      setStepOffset();
    }
  }

  if (step_offset == 0) {
    current_model = new Model(msa_stats, epsilon_0_h, epsilon_0_J);
    previous_model = new Model(msa_stats, epsilon_0_h, epsilon_0_J);
  } else {
    if (output_binary) {
      current_model =
        new Model("parameters_h_" + std::to_string(step_offset) + ".bin",
                  "parameters_J_" + std::to_string(step_offset) + ".bin",
                  "gradients_h_" + std::to_string(step_offset) + ".bin",
                  "gradients_J_" + std::to_string(step_offset) + ".bin",
                  "learning_rates_h_" + std::to_string(step_offset) + ".bin",
                  "learning_rates_J_" + std::to_string(step_offset) + ".bin");
      previous_model = new Model(
        "parameters_h_" + std::to_string(step_offset - 1) + ".bin",
        "parameters_J_" + std::to_string(step_offset - 1) + ".bin",
        "gradients_h_" + std::to_string(step_offset - 1) + ".bin",
        "gradients_J_" + std::to_string(step_offset - 1) + ".bin",
        "learning_rates_h_" + std::to_string(step_offset - 1) + ".bin",
        "learning_rates_J_" + std::to_string(step_offset - 1) + ".bin");
    } else {
      current_model =
        new Model("parameters_" + std::to_string(step_offset) + ".txt",
                  "gradients_" + std::to_string(step_offset) + ".txt",
                  "learning_rates_" + std::to_string(step_offset) + ".txt");
      previous_model =
        new Model("parameters_" + std::to_string(step_offset - 1) + ".txt",
                  "gradients_" + std::to_string(step_offset - 1) + ".txt",
                  "learning_rates_" + std::to_string(step_offset - 1) + ".txt");
    }
  }
  mcmc = new MCMC(msa_stats.getN(), msa_stats.getQ());
};

void
Sim::clearFiles(std::string dest_dir)
{
  DIR* dp;
  struct dirent* dirp;

  std::vector<std::string> files;
  dp = opendir(dest_dir.c_str());

  while ((dirp = readdir(dp)) != NULL) {
    std::string fname = dirp->d_name;

    if (fname.find("parameters_"))
      files.push_back(fname);
    else if (fname.find("gradients_"))
      files.push_back(fname);
    else if (fname.find("bmdca_"))
      files.push_back(fname);
    else if (fname.find("learning_rates_"))
      files.push_back(fname);
    else if (fname.find("MC_energies_"))
      files.push_back(fname);
    else if (fname.find("MC_samples_"))
      files.push_back(fname);
    else if (fname.find("msa_numerical"))
      files.push_back(fname);
    else if (fname.find("overlap_"))
      files.push_back(fname);
    else if (fname.find("ergo_"))
      files.push_back(fname);
    else if (fname.find("stat_MC_"))
      files.push_back(fname);
    else if (fname.find("stat_align_"))
      files.push_back(fname);
    else if (fname.find("rel_ent_grad_"))
      files.push_back(fname);
  }
  closedir(dp);

  for (auto it = files.begin(); it != files.end(); ++it) {
    std::remove((*it).c_str());
  }
};

void
Sim::setStepOffset(void)
{
  DIR* dp;
  struct dirent* dirp;

  dp = opendir(".");

  int run_count = 0;
  std::ifstream stream(run_log_file);
  if (stream) {
    std::string line;
    std::getline(stream, line); // skip the file header
    while (std::getline(stream, line)) {
      run_count++;
    }
  }
  stream.close();

  std::vector<int> steps;
  while ((dirp = readdir(dp)) != NULL) {
    std::string fname = dirp->d_name;
    if (output_binary) {
      if (fname.find("parameters_h") == std::string::npos)
        continue;

      const std::regex re_param_h("parameters_h_([0-9]+)\\.bin");
      std::smatch match_param_h;

      int idx = -1;
      if (std::regex_match(fname, match_param_h, re_param_h)) {
        if (match_param_h.size() == 2) {
          idx = std::stoi(match_param_h[1].str());
        }
      }

      if (checkFileExists("parameters_J_" + std::to_string(idx) + ".bin") &
          checkFileExists("gradients_h_" + std::to_string(idx) + ".bin") &
          checkFileExists("gradients_J_" + std::to_string(idx) + ".bin") &
          checkFileExists("learning_rates_h_" + std::to_string(idx) + ".bin") &
          checkFileExists("learning_rates_J_" + std::to_string(idx) + ".bin")) {
        steps.push_back(idx);
      } else {
        std::string file;
        file = "parameters_h_" + std::to_string(idx) + ".bin";
        if (checkFileExists(file))
          deleteFile(file);

        file = "parameters_J_" + std::to_string(idx) + ".bin";
        if (checkFileExists(file))
          deleteFile(file);

        file = "gradients_h_" + std::to_string(idx) + ".bin";
        if (checkFileExists(file))
          deleteFile(file);

        file = "gradients_J_" + std::to_string(idx) + ".bin";
        if (checkFileExists(file))
          deleteFile(file);

        file = "learning_rates_h_" + std::to_string(idx) + ".bin";
        if (checkFileExists(file))
          deleteFile(file);

        file = "learning_rates_J_" + std::to_string(idx) + ".bin";
        if (checkFileExists(file))
          deleteFile(file);
      }

    } else {
      if (fname.find("parameters") == std::string::npos)
        continue;

      const std::regex re_param("parameters_([0-9]+)\\.txt");
      std::smatch match_param;

      int idx = -1;
      if (std::regex_match(fname, match_param, re_param)) {
        if (match_param.size() == 2) {
          idx = std::stoi(match_param[1].str());
        }
      }

      if (checkFileExists("gradients_" + std::to_string(idx) + ".txt") &
          checkFileExists("learning_rates_" + std::to_string(idx) + ".txt")) {
        steps.push_back(idx);
      } else {
        std::string file;
        file = "parameters_" + std::to_string(idx) + ".txt";
        if (checkFileExists(file))
          deleteFile(file);

        file = "gradients_" + std::to_string(idx) + ".txt";
        if (checkFileExists(file))
          deleteFile(file);

        file = "learning_rates_" + std::to_string(idx) + ".txt";
        if (checkFileExists(file))
          deleteFile(file);
      }
    }
  }
  closedir(dp);

  int max = -1;
  for (auto it1 = steps.begin(); it1 != steps.end(); ++it1) {
    if (*it1 > max) {
      for (auto it2 = steps.begin(); it2 != steps.end(); ++it2) {
        if ((*it2 == (*it1 - 1)) & (*it2 <= run_count)) {
          max = *it1;
        }
      }
    }
  }
  if (max < 0) {
    step_offset = 0;
  } else {
    step_offset = max;
  }
};

void
Sim::setBurnTimes(void)
{
  std::ifstream stream(run_log_file);
  std::string line;
  std::getline(stream, line);
  while (!stream.eof()) {
    std::getline(stream, line);
    std::stringstream buffer(line);
    std::string field;

    std::getline(buffer, field, '\t');
    if (step_offset == std::stoi(field)) {
      std::string tmp1;
      std::string tmp2;
      std::getline(buffer, tmp1, '\t');
      std::getline(buffer, tmp1, '\t');
      std::getline(buffer, tmp2, '\t');
      t_wait = std::stoi(tmp1);
      delta_t = std::stoi(tmp2);
      break;
    }
  }
};

Sim::~Sim(void)
{
  delete current_model;
  delete previous_model;
  delete mcmc;
  delete mcmc_stats;
};

void
Sim::burnRNG(void)
{
  long int value = -1;
  std::ifstream stream(run_log_file);
  std::string line;
  std::getline(stream, line);
  while (!stream.eof()) {
    std::getline(stream, line);
    std::stringstream buffer(line);
    std::string field;

    std::getline(buffer, field, '\t');
    if (step_offset == std::stoi(field)) {
      std::vector<std::string> fields;
      std::stringstream ss;
      ss.str(line);
      std::string field;
      while (std::getline(ss, field, '\t')) {
        fields.push_back(field);
      }

      value = std::stol(fields.at(17));
      break;
    }
  }

  std::uniform_int_distribution<long int> dist(0, RAND_MAX - count_max);
  int counter = 1;
  while ((dist(rng) != value) &
         (counter < 100 * step_max * step_importance_max)) {
    counter++;
  }
};

void
Sim::readInitialSample(int N, int Q)
{
  std::ifstream input_stream(init_sample_file);

  if (!input_stream) {
    std::cerr << "ERROR: cannot read '" << init_sample_file << "'."
              << std::endl;
    exit(2);
  }

  std::string line;
  int aa;
  std::getline(input_stream, line);
  for (int n = 0; n < N; n++) {
    std::istringstream iss(line);
    iss >> aa;
    assert(aa < Q);
    initial_sample(n) = aa;
  }
  input_stream.close();
};

void
Sim::run(void)
{
  std::cout << "initializing run... " << std::flush;

  arma::wall_clock step_timer;

  arma::wall_clock timer;
  timer.tic();

  int N = current_model->N;
  int Q = current_model->Q;

  // Initialize sample data structure
  samples = arma::Cube<int>(M, N, count_max, arma::fill::zeros);
  mcmc_stats = new MCMCStats(&samples, &(current_model->params));

  if (init_sample) {
    initial_sample = arma::Col<int>(N, arma::fill::zeros);
    readInitialSample(N, Q);
  }

  // Instantiate the PCG random number generator and unifrom random
  // distribution.
  rng(random_seed);
  std::uniform_int_distribution<long int> dist(0, RAND_MAX - count_max);

  // Initialize the buffer.
  run_buffer = arma::Mat<double>(save_parameters, 19, arma::fill::zeros);

  if (step_offset == 0) {
    initializeRunLog();
    t_wait = t_wait_0;
    delta_t = delta_t_0;
  } else {
    burnRNG();
    setBurnTimes();
  }

  std::cout << timer.toc() << " sec" << std::endl << std::endl;

  // BM sampling loop
  for (step = 1 + step_offset; step <= step_max; step++) {
    step_timer.tic();
    std::cout << "Step: " << step << std::endl;

    run_buffer.at((step - 1) % save_parameters, 0) = step;
    run_buffer.at((step - 1) % save_parameters, 1) = count_max;
    run_buffer.at((step - 1) % save_parameters, 2) = t_wait;
    run_buffer.at((step - 1) % save_parameters, 3) = delta_t;

    std::cout << "loading params to mcmc... " << std::flush;
    timer.tic();
    mcmc->load(current_model->params);
    std::cout << timer.toc() << " sec" << std::endl;

    // Sampling from MCMC (keep trying until correct properties found)
    bool flag_mc = true;
    long int seed;
    while (flag_mc) {
      // Draw from MCMC
      std::cout << "sampling model with mcmc... " << std::flush;
      timer.tic();
      seed = dist(rng);
      run_buffer.at((step - 1) % save_parameters, 17) = seed;
      if (init_sample) {
        mcmc->sample_init(&samples,
                          count_max,
                          M,
                          N,
                          t_wait,
                          delta_t,
                          &initial_sample,
                          seed,
                          temperature);
      } else {
        mcmc->sample(
          &samples, count_max, M, N, t_wait, delta_t, seed, temperature);
      }
      std::cout << timer.toc() << " sec" << std::endl;

      std::cout << "updating mcmc with samples... " << std::flush;
      timer.tic();
      mcmc_stats->updateData(&samples, &(current_model->params));
      std::cout << timer.toc() << " sec" << std::endl;

      // Run checks and alter burn-in and wait times
      if (check_ergo) {
        std::cout << "computing sequence energies and correlations... "
                  << std::flush;
        timer.tic();
        mcmc_stats->computeEnergiesStats();
        mcmc_stats->computeCorrelations();
        std::cout << timer.toc() << " sec" << std::endl;

        std::vector<double> energy_stats = mcmc_stats->getEnergiesStats();
        std::vector<double> corr_stats = mcmc_stats->getCorrelationsStats();

        double auto_corr = corr_stats.at(2);
        double cross_corr = corr_stats.at(3);
        double check_corr = corr_stats.at(4);
        double cross_check_err = corr_stats.at(9);
        double auto_cross_err = corr_stats.at(8);

        double e_start = energy_stats.at(0);
        double e_start_sigma = energy_stats.at(1);
        double e_end = energy_stats.at(2);
        double e_end_sigma = energy_stats.at(3);
        double e_err = energy_stats.at(4);

        run_buffer.at((step - 1) % save_parameters, 4) = auto_corr;
        run_buffer.at((step - 1) % save_parameters, 5) = cross_corr;
        run_buffer.at((step - 1) % save_parameters, 6) = check_corr;
        run_buffer.at((step - 1) % save_parameters, 7) = auto_cross_err;
        run_buffer.at((step - 1) % save_parameters, 8) = cross_check_err;

        run_buffer.at((step - 1) % save_parameters, 9) = e_start;
        run_buffer.at((step - 1) % save_parameters, 10) = e_start_sigma;
        run_buffer.at((step - 1) % save_parameters, 11) = e_end;
        run_buffer.at((step - 1) % save_parameters, 12) = e_end_sigma;
        run_buffer.at((step - 1) % save_parameters, 13) = e_err;

        bool flag_deltat_up = true;
        bool flag_deltat_down = true;
        bool flag_twaiting_up = true;
        bool flag_twaiting_down = true;

        if (check_corr - cross_corr < cross_check_err) {
          flag_deltat_up = false;
        }
        if (auto_corr - cross_corr > auto_cross_err) {
          flag_deltat_down = false;
        }

        if (e_start - e_end < 2 * e_err) {
          flag_twaiting_up = false;
        }
        if (e_start - e_end > -2 * e_err) {
          flag_twaiting_down = false;
        }

        if (flag_deltat_up) {
          delta_t = (int)(round((double)delta_t * adapt_up_time));
          std::cout << "increasing wait time to " << delta_t << std::endl;
        } else if (flag_deltat_down) {
          delta_t = Max((int)(round((double)delta_t * adapt_down_time)), 1);
          std::cout << "decreasing wait time to " << delta_t << std::endl;
        }

        if (flag_twaiting_up) {
          t_wait = (int)(round((double)t_wait * adapt_up_time));
          std::cout << "increasing burn-in time to " << t_wait << std::endl;
        } else if (flag_twaiting_down) {
          t_wait = Max((int)(round((double)t_wait * adapt_down_time)), 1);
          std::cout << "decreasing burn-in time to " << t_wait << std::endl;
        }

        if (not flag_deltat_up and not flag_twaiting_up) {
          flag_mc = false;
        } else {
          std::cout << "resampling..." << std::endl;
        }
      } else {
        flag_mc = false;
      }
    }

    // Importance sampling loop
    int step_importance = 0;
    bool flag_coherence = true;
    while (step_importance < step_importance_max and flag_coherence == true) {
      step_importance++;
      if (step_importance > 1) {
        std::cout << "importance sampling step " << step_importance << "... "
                  << std::flush;
        timer.tic();
        mcmc_stats->computeSampleStatsImportance(&(current_model->params),
                                                 &(previous_model->params));
        std::cout << timer.toc() << " sec" << std::endl;

        double coherence = mcmc_stats->Z_ratio;
        if (coherence > coherence_min && 1.0 / coherence > coherence_min) {
          flag_coherence = true;
        } else {
          flag_coherence = false;
        }
      } else {
        std::cout << "computing mcmc 1p and 2p statistics... " << std::flush;
        timer.tic();
        mcmc_stats->computeSampleStats();
        std::cout << timer.toc() << " sec" << std::endl;
      }

      // Compute error reparametrization
      previous_model->gradient.h = current_model->gradient.h;
      previous_model->gradient.J = current_model->gradient.J;

      std::cout << "computing error and updating gradient... " << std::flush;
      timer.tic();
      bool converged = computeErrorReparametrization();
      std::cout << timer.toc() << " sec" << std::endl;

      if (converged) {
        std::cout << "writing results" << std::endl;
        writeRunLog(step % save_parameters);
        writeData(std::to_string(step) + "_final");
        return;
      }

      // Update learning rate
      previous_model->learning_rates.h = current_model->learning_rates.h;
      previous_model->learning_rates.J = current_model->learning_rates.J;
      std::cout << "update learning rate... " << std::flush;
      timer.tic();
      updateLearningRate();
      std::cout << timer.toc() << " sec" << std::endl;

      // Update parameters
      previous_model->params.h = current_model->params.h;
      previous_model->params.J = current_model->params.J;
      std::cout << "updating parameters... " << std::flush;
      timer.tic();
      updateReparameterization();
      std::cout << timer.toc() << " sec" << std::endl;

      run_buffer.at((step - 1) % save_parameters, 18) = step_timer.toc();

      // Save parameters
      if (step % save_parameters == 0 &&
          (step_importance == step_importance_max || flag_coherence == false)) {
        std::cout << "writing step " << step << "... " << std::flush;
        timer.tic();
        writeRunLog(step % save_parameters);
        writeData(step);
        std::cout << timer.toc() << " sec" << std::endl;
      }
    }
    std::cout << std::endl;
  }

  if (step_offset != step_max) {
    std::cout << "writing final results... " << std::flush;
    if ((step_max % save_parameters) != 0) {
      writeRunLog(step_max % save_parameters);
    }
    writeData(step_max);
    writeData("final");
  } else {
    std::cout << "all " << step_offset << " steps already... " << std::flush;
  }

  std::cout << "done" << std::endl;
  return;
};

bool
Sim::computeErrorReparametrization(void)
{
  double M_eff = msa_stats.getEffectiveM();
  int N = msa_stats.getN();
  int M = msa_stats.getM();
  int Q = msa_stats.getQ();

  double error_stat_1p = 0;
  double error_stat_2p = 0;
  double error_stat_tot = 0;
  double error_1p = 0;
  double error_2p = 0;
  double error_tot = 0;
  double delta;
  double delta_stat = 0;
  double deltamax_1 = 0;
  double deltamax_2 = 0;
  double rho = 0, beta = 0, den_beta = 0, num_beta = 0, num_rho = 0,
         den_stat = 0, den_mc = 0, c_mc_av = 0, c_stat_av = 0, rho_1p = 0,
         num_rho_1p = 0, den_stat_1p = 0, den_mc_1p = 0;

  double phi = 0;
  double lambda_h = lambda_reg1;
  double lambda_j = lambda_reg2;

  int count1 = 0;
  int count2 = 0;

  // Compute gradient
  for (int i = 0; i < N; i++) {
    for (int aa = 0; aa < Q; aa++) {
      phi = current_model->params.h.at(aa, i);
      for (int j = 0; j < N; j++) {
        if (i < j) {
          for (int bb = 0; bb < Q; bb++) {
            phi += current_model->params.J.at(i, j).at(aa, bb) *
                   msa_stats.frequency_1p.at(bb, j);
          }
        } else if (i > j) {
          for (int bb = 0; bb < Q; bb++) {
            phi += current_model->params.J.at(j, i).at(bb, aa) *
                   msa_stats.frequency_1p.at(bb, j);
          }
        }
      }
      delta = mcmc_stats->frequency_1p.at(aa, i) -
              msa_stats.frequency_1p.at(aa, i) + lambda_h * 0.5 * phi;
      delta_stat =
        (mcmc_stats->frequency_1p.at(aa, i) -
         msa_stats.frequency_1p.at(aa, i)) /
        (pow(msa_stats.frequency_1p.at(aa, i) *
                 (1. - msa_stats.frequency_1p.at(aa, i)) / M_eff +
               pow(mcmc_stats->frequency_1p_sigma.at(aa, i), 2) + EPSILON,
             0.5));
      error_1p += pow(delta, 2);
      error_stat_1p += pow(delta_stat, 2);
      if (pow(delta, 2) > pow(deltamax_1, 2))
        deltamax_1 = sqrt(pow(delta, 2));

      if (sqrt(pow(delta_stat, 2)) > error_min_update) {
        current_model->gradient.h.at(aa, i) = -delta;
        count1++;
      }
    }
  }

  double error_c = 0;
  double c_mc = 0, c_stat = 0;

  for (int i = 0; i < N; i++) {
    for (int j = i + 1; j < N; j++) {
      for (int aa1 = 0; aa1 < Q; aa1++) {
        for (int aa2 = 0; aa2 < Q; aa2++) {
          delta = -(msa_stats.frequency_2p.at(i, j).at(aa1, aa2) -
                    mcmc_stats->frequency_2p.at(i, j).at(aa1, aa2) +
                    (mcmc_stats->frequency_1p.at(aa1, i) -
                     msa_stats.frequency_1p.at(aa1, i)) *
                      msa_stats.frequency_1p.at(aa2, j) +
                    (mcmc_stats->frequency_1p.at(aa2, j) -
                     msa_stats.frequency_1p.at(aa2, j)) *
                      msa_stats.frequency_1p.at(aa1, i) -
                    lambda_j * current_model->params.J.at(i, j).at(aa1, aa2));
          delta_stat =
            (mcmc_stats->frequency_2p.at(i, j).at(aa1, aa2) -
             msa_stats.frequency_2p.at(i, j).at(aa1, aa2)) /
            (pow(msa_stats.frequency_2p.at(i, j).at(aa1, aa2) *
                     (1.0 - msa_stats.frequency_2p.at(i, j).at(aa1, aa2)) /
                     M_eff +
                   pow(mcmc_stats->frequency_2p.at(i, j).at(aa1, aa2), 2) +
                   EPSILON,
                 0.5));

          c_mc = mcmc_stats->frequency_2p.at(i, j).at(aa1, aa2) -
                 mcmc_stats->frequency_1p.at(aa1, i) *
                   mcmc_stats->frequency_1p.at(aa2, j);
          c_stat = msa_stats.frequency_2p.at(i, j).at(aa1, aa2) -
                   msa_stats.frequency_1p.at(aa1, i) *
                     msa_stats.frequency_1p.at(aa2, j);
          c_mc_av += c_mc;
          c_stat_av += c_stat;
          error_c += pow(c_mc - c_stat, 2);
          error_2p += pow(delta, 2);
          error_stat_2p += pow(delta_stat, 2);

          if (pow(delta, 2) > pow(deltamax_2, 2)) {
            deltamax_2 = sqrt(pow(delta, 2));
          }
          if (sqrt(pow(delta_stat, 2)) > error_min_update) {
            current_model->gradient.J.at(i, j).at(aa1, aa2) = -delta;
            count2++;
          }
        }
      }
    }
  }

  c_stat_av /= ((N * (N - 1) * Q * Q) / 2);
  c_mc_av /= ((N * (N - 1) * Q * Q) / 2);

  num_rho = num_beta = den_stat = den_mc = den_beta = 0;
  for (int i = 0; i < N; i++) {
    for (int j = i + 1; j < N; j++) {
      for (int aa1 = 0; aa1 < Q; aa1++) {
        for (int aa2 = 0; aa2 < Q; aa2++) {
          c_mc = mcmc_stats->frequency_2p.at(i, j).at(aa1, aa2) -
                 mcmc_stats->frequency_1p.at(aa1, i) *
                   mcmc_stats->frequency_1p.at(aa2, j);
          c_stat = msa_stats.frequency_2p.at(i, j).at(aa1, aa2) -
                   msa_stats.frequency_1p.at(aa1, i) *
                     msa_stats.frequency_1p.at(aa2, j);
          num_rho += (c_mc - c_mc_av) * (c_stat - c_stat_av);
          num_beta += (c_mc) * (c_stat);
          den_stat += pow(c_stat - c_stat_av, 2);
          den_mc += pow(c_mc - c_mc_av, 2);
          den_beta += pow(c_stat, 2);
        }
      }
    }
  }

  num_rho_1p = den_stat_1p = den_mc_1p = 0;
  for (int i = 0; i < N; i++) {
    for (int aa = 0; aa < Q; aa++) {
      num_rho_1p += (mcmc_stats->frequency_1p.at(aa, i) - 1.0 / Q) *
                    (msa_stats.frequency_1p.at(aa, i) - 1.0 / Q);
      den_stat_1p += pow(msa_stats.frequency_1p.at(aa, i) - 1.0 / Q, 2);
      den_mc_1p += pow(mcmc_stats->frequency_1p.at(aa, i) - 1.0 / Q, 2);
    }
  }

  beta = num_beta / den_beta;
  rho = num_rho / sqrt(den_mc * den_stat);
  rho_1p = num_rho_1p / sqrt(den_mc_1p * den_stat_1p);

  error_1p = sqrt(error_1p / (N * Q));
  error_2p = sqrt(error_2p / ((N * (N - 1) * Q * Q) / 2));

  error_stat_1p = sqrt(error_stat_1p / (N * Q));
  error_stat_2p = sqrt(error_stat_2p / (N * (N - 1) * Q * Q) / 2);

  error_c = sqrt(error_c / (N * (N - 1) * Q * Q) / 2);

  error_tot = error_1p + error_2p;
  error_stat_tot = error_stat_1p + error_stat_2p;

  run_buffer.at((step - 1) % save_parameters, 14) = error_1p;
  run_buffer.at((step - 1) % save_parameters, 15) = error_2p;
  run_buffer.at((step - 1) % save_parameters, 16) = error_tot;

  bool converged = false;
  if (error_tot < error_max) {
    std::cout << "converged" << std::endl;
    converged = true;
  }
  return converged;
};

void
Sim::updateLearningRate(void)
{
  double M_eff = msa_stats.getEffectiveM();
  int N = msa_stats.getN();
  int M = msa_stats.getM();
  int Q = msa_stats.getQ();
  double max_step_J = max_step_J_N / N;

  double alfa = 0;
  for (int i = 0; i < N; i++) {
    for (int j = i + 1; j < N; j++) {
      for (int a = 0; a < Q; a++) {
        for (int b = 0; b < Q; b++) {
          alfa = current_model->gradient.J.at(i, j).at(a, b) *
                 previous_model->gradient.J.at(i, j).at(a, b);
          alfa =
            Theta(alfa) * adapt_up + Theta(-alfa) * adapt_down + Delta(alfa);

          current_model->learning_rates.J.at(i, j).at(a, b) =
            Min(max_step_J,
                Max(min_step_J,
                    alfa * current_model->learning_rates.J.at(i, j).at(a, b)));
        }
      }
    }
  }

  for (int i = 0; i < N; i++) {
    for (int a = 0; a < Q; a++) {
      alfa = current_model->gradient.h.at(a, i) *
             previous_model->gradient.h.at(a, i);
      alfa = Theta(alfa) * adapt_up + Theta(-alfa) * adapt_down + Delta(alfa);
      current_model->learning_rates.h.at(a, i) =
        Min(max_step_h,
            Max(min_step_h, alfa * current_model->learning_rates.h.at(a, i)));
    }
  }
};

void
Sim::updateReparameterization(void)
{
  double M_eff = msa_stats.getEffectiveM();
  int N = msa_stats.getN();
  int M = msa_stats.getM();
  int Q = msa_stats.getQ();

  for (int i = 0; i < N; i++) {
    for (int j = i + 1; j < N; j++) {
      for (int a = 0; a < Q; a++) {
        for (int b = 0; b < Q; b++) {
          current_model->params.J.at(i, j).at(a, b) +=
            current_model->learning_rates.J.at(i, j).at(a, b) *
            current_model->gradient.J.at(i, j).at(a, b);
        }
      }
    }
  };

  arma::Mat<double> Dh = arma::Mat<double>(Q, N, arma::fill::zeros);
  for (int i = 0; i < N; i++) {
    for (int a = 0; a < Q; a++) {
      for (int j = 0; j < N; j++) {
        if (i < j) {
          for (int b = 0; b < Q; b++) {
            Dh.at(a, i) += msa_stats.frequency_1p.at(b, j) *
                           current_model->learning_rates.J.at(i, j).at(a, b) *
                           current_model->gradient.J.at(i, j).at(a, b);
          }
        }
        if (i > j) {
          for (int b = 0; b < Q; b++) {
            Dh.at(a, i) += msa_stats.frequency_1p.at(b, j) *
                           current_model->learning_rates.J.at(j, i).at(b, a) *
                           current_model->gradient.J.at(j, i).at(b, a);
          }
        }
      }
    }
  };

  for (int i = 0; i < N; i++) {
    for (int a = 0; a < Q; a++) {
      current_model->params.h.at(a, i) +=
        current_model->learning_rates.h.at(a, i) *
          current_model->gradient.h.at(a, i) +
        0.5 * Dh.at(a, i);
    }
  }
};

void
Sim::writeData(int step)
{
  if (output_binary) {
    previous_model->writeParams(
      "parameters_h_" + std::to_string(step - 1) + ".bin",
      "parameters_J_" + std::to_string(step - 1) + ".bin");
    previous_model->writeGradient(
      "gradients_h_" + std::to_string(step - 1) + ".bin",
      "gradients_J_" + std::to_string(step - 1) + ".bin");
    previous_model->writeLearningRates(
      "learning_rates_h_" + std::to_string(step - 1) + ".bin",
      "learning_rates_J_" + std::to_string(step - 1) + ".bin");
    current_model->writeParams("parameters_h_" + std::to_string(step) + ".bin",
                               "parameters_J_" + std::to_string(step) + ".bin");
    current_model->writeGradient("gradients_h_" + std::to_string(step) + ".bin",
                                 "gradients_J_" + std::to_string(step) +
                                   ".bin");
    current_model->writeLearningRates(
      "learning_rates_h_" + std::to_string(step) + ".bin",
      "learning_rates_J_" + std::to_string(step) + ".bin");

    mcmc_stats->writeFrequency1p("stat_MC_1p_" + std::to_string(step) + ".bin",
                                 "stat_MC_1p_sigma_" + std::to_string(step) +
                                   ".bin");
    mcmc_stats->writeFrequency2p("stat_MC_2p_" + std::to_string(step) + ".bin",
                                 "stat_MC_2p_sigma_" + std::to_string(step) +
                                   ".bin");
  } else {
    previous_model->writeParamsCompat("parameters_" + std::to_string(step - 1) +
                                      ".txt");
    previous_model->writeGradientCompat("gradients_" +
                                        std::to_string(step - 1) + ".txt");
    previous_model->writeLearningRatesCompat("learning_rates_" +
                                             std::to_string(step - 1) + ".txt");

    current_model->writeParamsCompat("parameters_" + std::to_string(step) +
                                     ".txt");
    current_model->writeGradientCompat("gradients_" + std::to_string(step) +
                                       ".txt");
    current_model->writeLearningRatesCompat("learning_rates_" +
                                            std::to_string(step) + ".txt");

    mcmc_stats->writeFrequency1pCompat(
      "stat_MC_1p_" + std::to_string(step) + ".txt",
      "stat_MC_1p_sigma_" + std::to_string(step) + ".txt");
    mcmc_stats->writeFrequency2pCompat(
      "stat_MC_2p_" + std::to_string(step) + ".txt",
      "stat_MC_2p_sigma_" + std::to_string(step) + ".txt");
  }
  mcmc_stats->writeSamples("MC_samples_" + std::to_string(step) + ".txt");
  mcmc_stats->writeSampleEnergies("MC_energies_" + std::to_string(step) +
                                  ".txt");

  if (check_ergo) {
    // mcmc_stats->writeSampleEnergiesRelaxation("energy_" +
    // std::to_string(step) + ".dat", delta_t);
    // mcmc_stats->writeEnergyStats("my_energies_start_" + std::to_string(step)
    // + ".txt",
    //                              "my_energies_end_" + std::to_string(step) +
    //                              ".txt", "my_energies_cfr_" +
    //                              std::to_string(step) + ".txt",
    //                              "my_energies_cfr_err_" +
    //                              std::to_string(step) + ".txt");
    mcmc_stats->writeCorrelationsStats(
      "overlap_" + std::to_string(step) + ".txt",
      "overlap_inf_" + std::to_string(step) + ".txt",
      "ergo_" + std::to_string(step) + ".txt");
  }
};

void
Sim::writeData(std::string id)
{
  if (output_binary) {
    current_model->writeParams("parameters_h_" + id + ".bin",
                               "parameters_J_" + id + ".bin");
    current_model->writeGradient("gradients_h_" + id + ".bin",
                                 "gradients_J_" + id + ".bin");
    current_model->writeLearningRates("learning_rates_h_" + id + ".bin",
                                      "learning_rates_J_" + id + ".bin");

    mcmc_stats->writeFrequency1p("stat_MC_1p_" + id + ".bin",
                                 "stat_MC_1p_sigma_" + id + ".bin");
    mcmc_stats->writeFrequency2p("stat_MC_2p_" + id + ".bin",
                                 "stat_MC_2p_sigma_" + id + ".bin");
  } else {
    current_model->writeParamsCompat("parameters_" + id + ".txt");
    current_model->writeGradientCompat("gradients_" + id + ".txt");
    current_model->writeLearningRatesCompat("learning_rates_" + id + ".txt");

    mcmc_stats->writeFrequency1pCompat("stat_MC_1p_" + id + ".txt",
                                       "stat_MC_1p_sigma_" + id + ".txt");
    mcmc_stats->writeFrequency2pCompat("stat_MC_2p_" + id + ".txt",
                                       "stat_MC_2p_sigma_" + id + ".txt");
  }
  mcmc_stats->writeSamples("MC_samples_" + id + ".txt");
  mcmc_stats->writeSampleEnergies("MC_energies_" + id + ".txt");

  if (check_ergo) {
    // mcmc_stats->writeSampleEnergiesRelaxation("energy_" + id + ".dat", delta_t);
    // mcmc_stats->writeEnergyStats("my_energies_start_" + id + ".txt",
    //                              "my_energies_end_" + id + ".txt",
    //                              "my_energies_cfr_" + id + ".txt",
    //                              "my_energies_cfr_err_" + id + ".txt");
    mcmc_stats->writeCorrelationsStats("overlap_" + id + ".txt",
                                       "overlap_inf_" + id + ".txt",
                                       "ergo_" + id + ".txt");
  }
};

void
Sim::initializeRunLog()
{
  std::ofstream stream{ run_log_file, std::ios_base::out };
  stream << "step"
         << "\t"
         << "reps"
         << "\t"
         << "burn-in"
         << "\t"
         << "burn-between"
         << "\t";
  if (check_ergo) {
    stream << "auto-corr"
           << "\t"
           << "cross-corr"
           << "\t"
           << "check-corr"
           << "\t"
           << "auto-cross-err"
           << "\t"
           << "cross-check-err"
           << "\t"
           << "energy-start-avg"
           << "\t"
           << "sigma-energy-start-sigma"
           << "\t"
           << "energy-end-avg"
           << "\t"
           << "energy-end-sigma"
           << "\t"
           << "energy-err"
           << "\t";
  }
  stream << "error-h"
         << "\t"
         << "error-J"
         << "\t"
         << "error-tot"
         << "\t"
         << "seed"
         << "\t"
         << "step-time" << std::endl;
  stream.close();
};

void
Sim::writeRunLog(int current_step)
{
  std::ofstream stream{ run_log_file, std::ios_base::app };

  int n_entries;
  if (current_step == 0) {
    n_entries = save_parameters;
  } else {
    n_entries = current_step;
  }
  for (int i = 0; i < n_entries; i++) {
    stream << (int)run_buffer.at(i, 0) << "\t";
    stream << (int)run_buffer.at(i, 1) << "\t";
    stream << (int)run_buffer.at(i, 2) << "\t";
    stream << (int)run_buffer.at(i, 3) << "\t";
    if (check_ergo) {
      stream << run_buffer.at(i, 4) << "\t";
      stream << run_buffer.at(i, 5) << "\t";
      stream << run_buffer.at(i, 6) << "\t";
      stream << run_buffer.at(i, 7) << "\t";
      stream << run_buffer.at(i, 8) << "\t";
      stream << run_buffer.at(i, 9) << "\t";
      stream << run_buffer.at(i, 10) << "\t";
      stream << run_buffer.at(i, 11) << "\t";
      stream << run_buffer.at(i, 12) << "\t";
      stream << run_buffer.at(i, 13) << "\t";
    }
    stream << run_buffer.at(i, 14) << "\t";
    stream << run_buffer.at(i, 15) << "\t";
    stream << run_buffer.at(i, 16) << "\t";
    stream << (long int)run_buffer.at(i, 17) << "\t";
    stream << run_buffer.at(i, 18) << std::endl;
  }
  run_buffer.zeros();
  stream.close();
};

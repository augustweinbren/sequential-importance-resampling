#define _USE_MATH_DEFINES

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <numeric>
#include <random>
#include <utility>
#include <vector>

#include <assert.h>
#include <math.h>

constexpr double xInitialValue_ = 0.1;
constexpr double varianceY_ = 10;
constexpr double varianceX_ = 1;
size_t timestep_ = 1;

void updateStateForecast(double& xValue, const size_t timeStep) {
  xValue = 0.5*xValue + 25*xValue/(1+std::pow(xValue, 2)) + 8*std::cos(1.2*timeStep);
}

double updatePredictionForecast(const double& xValue) {
  return std::pow(xValue, 2)/20;
}

void updateX_k(std::vector<double> &xValues, const double xVariance,
               const size_t timeStep){
  std::default_random_engine generator;
  std::normal_distribution<double> xNoiseDistribution(0.0, xVariance);
  for (size_t particle = 0; particle < xValues.size(); particle++) {
    double xObservationNoise = xNoiseDistribution(generator);
    updateStateForecast(xValues.at(particle), timeStep);
    xValues.at(particle) += xObservationNoise;
  }
}

void updateHOfX(std::vector<double> &hOfX, const std::vector<double> &x_k){
  for (size_t particle = 0; particle < x_k.size(); particle++) {
    hOfX.at(particle) = updatePredictionForecast(x_k.at(particle));
  }
}

double estimateLikelihood(const double& hOfX,
                          const double& forecastY,
                          const double& varianceY) {
  double likelihood = std::sqrt(0.5*M_1_PI*1/varianceY);
  likelihood *= std::exp(-1/(2*varianceY)*std::pow(forecastY - hOfX, 2));
  return likelihood;
}

void updateWeights(std::vector<double> &weightsVector,
                   const std::vector<double> &hOfX,
                   const double forecastY,
                   const double varianceY){
  for (size_t particle = 0; particle < hOfX.size(); particle++) {
    double likelihood = estimateLikelihood(hOfX.at(particle), forecastY, varianceY);
    weightsVector.at(particle) *= likelihood;
  }
  double weightSum = std::accumulate(weightsVector.begin(), weightsVector.end(), 0);
  auto normalize = [&weightSum](const double& n) { return n/weightSum; };
  std::for_each(weightsVector.begin(), weightsVector.end(), normalize);
}

void resampleX(std::vector<double> &x_k, std::vector<double> &weights){
  std::default_random_engine generator;
  std::uniform_real_distribution<double> distribution(0.0, 1.0/weights.size());
  std::vector<double> cdfParticles(1, 0.0);
  for (size_t particle = 1; particle < weights.size(); particle++) {
    cdfParticles.push_back(weights.at(particle) + cdfParticles.back());
  }
  double u_1 = distribution(generator);
  size_t j = 0;
  size_t i = 0;
  double u;
  while (j < weights.size()) {
    u = u_1 + 1.0 / weights.size() * j;
    if (cdfParticles.at(i) > u) {
      x_k.at(j) = x_k.at(i);
      weights.at(j) = 1.0/weights.size();
      j++;
    } else {
      i++;
    }
  }
}

double calculateRMSE(const std::vector<double> &observedValues, const double trueValue) {
  std::vector<double> squareErrors;
  for (size_t i = 0; i < observedValues.size(); i++) {
    squareErrors.push_back(std::pow(observedValues.at(i) - trueValue, 2));
  }
  double value = std::accumulate(squareErrors.begin(), squareErrors.end(), 0);
  value /= observedValues.size();
  value = std::sqrt(value);
  return value;
}

int main()
{
  const double xVariance = 1;
  const double yVariance = 10;
  size_t numberParticles;
  std::vector<double> hOfX;
  double xTrueValue = 0.1;
  double yTrueValue;
  std::cout << "Number of particles: ";
  std::cin >> numberParticles;
  assert(numberParticles > 0);
  double particleInitialWeights = 1.0/numberParticles;
  std::vector<double> weights(numberParticles, particleInitialWeights);
  std::vector<double> x_k(numberParticles, xTrueValue);
  std::cout << "\nNumber of time steps: ";
  int timeStepCount;
  std::cin >> timeStepCount;
  assert(timeStepCount > 0);
  std::vector<double> rmseX;
  std::vector<double> rmseY;
  for (size_t timeStep = 0; timeStep < timeStepCount; timeStep++) {
    updateStateForecast(xTrueValue, timeStep + 1);
    yTrueValue = updatePredictionForecast(xTrueValue);
    updateX_k(x_k, xVariance, timeStep + 1);
    updateHOfX(hOfX, x_k);
    updateWeights(weights, hOfX, yTrueValue, yVariance);
    resampleX(x_k, weights);
    updateHOfX(hOfX, x_k);
    rmseX.push_back(calculateRMSE(x_k, xTrueValue));
    rmseY.push_back(calculateRMSE(hOfX, yTrueValue));
    std::cout << "\nTime step: " << timeStep + 1 << std::endl;
    std::cout << "RMSE of x: " << rmseX.end() << std::endl;
    std::cout << "RMSE of y: " << rmseY.end() << std::endl;
  }
  return 0;
}

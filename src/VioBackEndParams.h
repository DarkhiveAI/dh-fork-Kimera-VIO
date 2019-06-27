/* ----------------------------------------------------------------------------
 * Copyright 2017, Massachusetts Institute of Technology,
 * Cambridge, MA 02139
 * All Rights Reserved
 * Authors: Luca Carlone, et al. (see THANKS for the full author list)
 * See LICENSE for the license information
 * -------------------------------------------------------------------------- */

/**
 * @file   VioBackEndParams.h
 * @brief  Class collecting the parameters of the Visual Inertial odometry pipeline
 * @author Luca Carlone
 */

#ifndef VioBackEndParams_H_
#define VioBackEndParams_H_

#include <stdlib.h>
#include <memory>
#include <unordered_map>
#include <boost/foreach.hpp>
#include <iostream>
#include <fstream>

#include <opencv2/core/core.hpp>

#include <gtsam/slam/SmartFactorParams.h>

#include <glog/logging.h>

namespace VIO {

enum InitializationModes { GT, IMU, ALIGNMENT };

///////////////////////////////////////////////////////////////////////////////////////
class VioBackEndParams
{
public:
  // TODO this should be a map from a string to a double
  VioBackEndParams(
      // IMU PARAMS
      const double gyroNoiseDensity = 0.00016968,
      const double accNoiseDensity = 0.002,
      const double gyroBiasSigma = 1.9393e-05,
      const double accBiasSigma = 0.003,
      const double imuIntegrationSigma = 1e-8,
      const gtsam::Vector3 n_gravity = gtsam::Vector3(0.0,0.0,-9.81), // gravity in navigation frame, according to
      const double nominalImuRate = 0.005,
      // INITIALIZATION SETTINGS
      const int autoInitialize = 0,
      const bool roundOnAutoInitialize = false,
      const double initialPositionSigma = 0.00001,
      const double initialRollPitchSigma = 10.0 / 180.0 * M_PI,
      const double initialYawSigma = 0.1 / 180.0 * M_PI,
      const double initialVelocitySigma = 1e-3,
      const double initialAccBiasSigma = 0.1,
      const double initialGyroBiasSigma = 0.01,
      // http://projects.asl.ethz.ch/datasets/doku.php?id=kmavvisualinertialdatasets, the x axis points upwards
      // VISION PARAMS
      const gtsam::LinearizationMode linMode = gtsam::HESSIAN,
      const gtsam::DegeneracyMode degMode = gtsam::ZERO_ON_DEGENERACY,
      const double smartNoiseSigma = 3,
      const double rankTolerance = 1, // we might also use 0.1
      const double landmarkDistanceThreshold = 20, // max distance to triangulate point in meters
      const double outlierRejection = 8, // max acceptable reprojection error // before tuning: 3
      const double retriangulationThreshold = 1e-3,
      const bool addBetweenStereoFactors = true,
      const double betweenRotationPrecision = 0.0, // inverse of variance
      const double betweenTranslationPrecision = 1/(0.1*0.1), // inverse of variance
      // OPTIMIZATION PARAMS
      const double relinearizeThreshold = 1e-2, // Before tuning: 1e-3
      const double relinearizeSkip = 1,
      const double zeroVelocitySigma = 1e-3, // zero velocity prior when disparity is low
      const double noMotionPositionSigma = 1e-3,
      const double noMotionRotationSigma = 1e-4,
      const double constantVelSigma = 1e-2,
      const size_t numOptimize = 2,
      const double horizon = 6, // in seconds
      const bool useDogLeg = false
  ) : initialPositionSigma_(initialPositionSigma), initialRollPitchSigma_(initialRollPitchSigma),
  initialYawSigma_(initialYawSigma), initialVelocitySigma_(initialVelocitySigma),
  initialAccBiasSigma_(initialAccBiasSigma), initialGyroBiasSigma_(initialGyroBiasSigma),
  gyroNoiseDensity_(gyroNoiseDensity), accNoiseDensity_(accNoiseDensity),
  imuIntegrationSigma_(imuIntegrationSigma), gyroBiasSigma_(gyroBiasSigma), accBiasSigma_(accBiasSigma),
  nominalImuRate_(nominalImuRate), n_gravity_(n_gravity), autoInitialize_(autoInitialize), roundOnAutoInitialize_(roundOnAutoInitialize),
  linearizationMode_(linMode), degeneracyMode_(degMode),
  smartNoiseSigma_(smartNoiseSigma),
  rankTolerance_(rankTolerance),
  landmarkDistanceThreshold_(landmarkDistanceThreshold), outlierRejection_(outlierRejection),
  retriangulationThreshold_(retriangulationThreshold),
  addBetweenStereoFactors_(addBetweenStereoFactors),betweenRotationPrecision_(betweenRotationPrecision), betweenTranslationPrecision_(betweenTranslationPrecision),
  relinearizeThreshold_(relinearizeThreshold), relinearizeSkip_(relinearizeSkip), horizon_(horizon), numOptimize_(numOptimize),useDogLeg_(useDogLeg),
  zeroVelocitySigma_(zeroVelocitySigma), noMotionPositionSigma_(noMotionPositionSigma),
  noMotionRotationSigma_(noMotionRotationSigma), constantVelSigma_(constantVelSigma)
  {
    // Trivial sanity checks.
    CHECK(horizon >= 0);
  }

  // Needed for virtual classes.
  virtual ~VioBackEndParams() = default;

  // initialization params
  double initialPositionSigma_, initialRollPitchSigma_, initialYawSigma_, initialVelocitySigma_, initialAccBiasSigma_, initialGyroBiasSigma_;

  // imu params
  double gyroNoiseDensity_, accNoiseDensity_, imuIntegrationSigma_, gyroBiasSigma_, accBiasSigma_, nominalImuRate_;
  gtsam::Vector3 n_gravity_;
  int autoInitialize_;
  bool roundOnAutoInitialize_;

  // Smart factor params
  gtsam::LinearizationMode linearizationMode_;
  gtsam::DegeneracyMode degeneracyMode_;
  double smartNoiseSigma_;
  double rankTolerance_, landmarkDistanceThreshold_, outlierRejection_, retriangulationThreshold_;
  bool addBetweenStereoFactors_;
  double betweenRotationPrecision_, betweenTranslationPrecision_;

  // iSAM params
  double relinearizeThreshold_, relinearizeSkip_, horizon_;
  int numOptimize_;
  bool useDogLeg_;

  // No Motion params
  double zeroVelocitySigma_, noMotionPositionSigma_, noMotionRotationSigma_, constantVelSigma_;

public:
  /* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
  virtual bool parseYAML(const std::string& filepath) {
    // make sure that each YAML file has %YAML:1.0 as first line
    cv::FileStorage fs;
    openFile(filepath, &fs);
    bool result = parseYAMLVioBackEndParams(fs);
    closeFile(&fs);
    return result;
  }

  /* ------------------------------------------------------------------------ */
  virtual bool equals(const VioBackEndParams& vp2, double tol = 1e-8) const {
    return equalsVioBackEndParams(vp2, tol);
  }

  /* ------------------------------------------------------------------------ */
  virtual void print() const {
    printVioBackEndParams();
  }

protected:
  void openFile(const std::string& filepath, cv::FileStorage* fs) const {
    CHECK_NOTNULL(fs);
    fs->open(filepath, cv::FileStorage::READ);
    if (!fs->isOpened()) {
      std::cout << "Cannot open file in parseYAML: " << filepath << std::endl;
      throw std::runtime_error("parseYAML (Vio): cannot open file (remember first line: %YAML:1.0)");
    }
  }

  void closeFile(cv::FileStorage* fs) {
    CHECK_NOTNULL(fs);
    fs->release();
  }

  /* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
  // Parse params YAML file
  bool parseYAMLVioBackEndParams(const cv::FileStorage& fs) {
    cv::FileNode file_handle;

    // IMU PARAMS
    file_handle = fs["gyroNoiseDensity"];
    CHECK(file_handle.type() != cv::FileNode::NONE); file_handle >> gyroNoiseDensity_;
    file_handle = fs["accNoiseDensity"];
    CHECK(file_handle.type() != cv::FileNode::NONE); file_handle >> accNoiseDensity_;
    file_handle = fs["imuIntegrationSigma"];
    CHECK(file_handle.type() != cv::FileNode::NONE); file_handle >> imuIntegrationSigma_;
    file_handle = fs["gyroBiasSigma"];
    CHECK(file_handle.type() != cv::FileNode::NONE); file_handle >> gyroBiasSigma_;
    file_handle = fs["accBiasSigma"];
    CHECK(file_handle.type() != cv::FileNode::NONE); file_handle >> accBiasSigma_;
    std::vector<double> n_gravity_stdVect;
    n_gravity_stdVect.clear();
    file_handle = fs["n_gravity"];
    CHECK(file_handle.type() != cv::FileNode::NONE);
    file_handle >> n_gravity_stdVect;
    for (int k = 0; k < 3; k++) {
      n_gravity_(k) = n_gravity_stdVect[k];
    }
    file_handle = fs["nominalImuRate"];
    CHECK(file_handle.type() != cv::FileNode::NONE); file_handle >> nominalImuRate_;
    // INITIALIZATION
    file_handle = fs["autoInitialize"];
    CHECK(file_handle.type() != cv::FileNode::NONE); file_handle >>autoInitialize_;
    file_handle = fs["roundOnAutoInitialize"];
    CHECK(file_handle.type() != cv::FileNode::NONE); file_handle >> roundOnAutoInitialize_;
    file_handle = fs["initialPositionSigma"];
    CHECK(file_handle.type() != cv::FileNode::NONE); file_handle >> initialPositionSigma_;
    file_handle = fs["initialRollPitchSigma"];
    CHECK(file_handle.type() != cv::FileNode::NONE); file_handle >> initialRollPitchSigma_;
    file_handle = fs["initialYawSigma"];
    CHECK(file_handle.type() != cv::FileNode::NONE); file_handle >> initialYawSigma_;
    file_handle = fs["initialVelocitySigma"];
    CHECK(file_handle.type() != cv::FileNode::NONE); file_handle >> initialVelocitySigma_;
    file_handle = fs["initialAccBiasSigma"];
    CHECK(file_handle.type() != cv::FileNode::NONE); file_handle >> initialAccBiasSigma_;
    file_handle = fs["initialGyroBiasSigma"];
    CHECK(file_handle.type() != cv::FileNode::NONE); file_handle >> initialGyroBiasSigma_;
    // VISION PARAMS
    int linearizationModeId;
    file_handle = fs["linearizationMode"];
    CHECK(file_handle.type() != cv::FileNode::NONE);
    file_handle >> linearizationModeId;
    switch(linearizationModeId){
    case 0:
      linearizationMode_ = gtsam::HESSIAN; break;
    case 1:
      linearizationMode_ = gtsam::IMPLICIT_SCHUR; break;
    case 2:
      linearizationMode_ = gtsam::JACOBIAN_Q; break;
    case 3:
      linearizationMode_ = gtsam::JACOBIAN_SVD; break;
    default:
      throw std::runtime_error("VIOparams parseYAML: wrong linearizationModeId"); break;
    }
    int degeneracyModeId;
    file_handle = fs["degeneracyMode"];
    CHECK(file_handle.type() != cv::FileNode::NONE);
    file_handle >> degeneracyModeId;
    switch(degeneracyModeId){
    case 0:
      degeneracyMode_ = gtsam::IGNORE_DEGENERACY; break;
    case 1:
      degeneracyMode_ = gtsam::ZERO_ON_DEGENERACY; break;
    case 2:
      degeneracyMode_ = gtsam::HANDLE_INFINITY; break;
    default:
      throw std::runtime_error("VIOparams parseYAML: wrong degeneracyMode_"); break;
    }
    file_handle = fs["smartNoiseSigma"];
    CHECK(file_handle.type() != cv::FileNode::NONE); file_handle >> smartNoiseSigma_;
    file_handle = fs["rankTolerance"];
    CHECK(file_handle.type() != cv::FileNode::NONE); file_handle >> rankTolerance_;
    file_handle = fs["landmarkDistanceThreshold"];
    CHECK(file_handle.type() != cv::FileNode::NONE); file_handle >> landmarkDistanceThreshold_;
    file_handle = fs["outlierRejection"];
    CHECK(file_handle.type() != cv::FileNode::NONE); file_handle >> outlierRejection_;
    file_handle = fs["retriangulationThreshold"];
    CHECK(file_handle.type() != cv::FileNode::NONE); file_handle >> retriangulationThreshold_;
    file_handle = fs["addBetweenStereoFactors"];
    CHECK(file_handle.type() != cv::FileNode::NONE); file_handle >> addBetweenStereoFactors_;
    file_handle = fs["betweenRotationPrecision"];
    CHECK(file_handle.type() != cv::FileNode::NONE); file_handle >> betweenRotationPrecision_;
    file_handle = fs["betweenTranslationPrecision"];
    CHECK(file_handle.type() != cv::FileNode::NONE); file_handle >> betweenTranslationPrecision_;

    // OPTIMIZATION PARAMS
    file_handle = fs["relinearizeThreshold"];
    CHECK(file_handle.type() != cv::FileNode::NONE); file_handle >> relinearizeThreshold_;
    file_handle = fs["relinearizeSkip"];
    CHECK(file_handle.type() != cv::FileNode::NONE); file_handle >> relinearizeSkip_;
    file_handle = fs["zeroVelocitySigma"];
    CHECK(file_handle.type() != cv::FileNode::NONE); file_handle >> zeroVelocitySigma_;
    file_handle = fs["noMotionPositionSigma"];
    CHECK(file_handle.type() != cv::FileNode::NONE); file_handle >> noMotionPositionSigma_;
    file_handle = fs["noMotionRotationSigma"];
    CHECK(file_handle.type() != cv::FileNode::NONE); file_handle >> noMotionRotationSigma_;
    file_handle = fs["constantVelSigma"];
    CHECK(file_handle.type() != cv::FileNode::NONE); file_handle >> constantVelSigma_;
    file_handle = fs["numOptimize"];
    CHECK(file_handle.type() != cv::FileNode::NONE); file_handle >> numOptimize_;
    file_handle = fs["horizon"];
    CHECK(file_handle.type() != cv::FileNode::NONE); file_handle >> horizon_;
    file_handle = fs["useDogLeg"];
    CHECK(file_handle.type() != cv::FileNode::NONE); file_handle >> useDogLeg_;

    return true;
  }

  /* ------------------------------------------------------------------------------------- */
  bool equalsVioBackEndParams(const VioBackEndParams& vp2, double tol = 1e-8) const{
    return
        // IMU PARAMS
        (fabs(gyroNoiseDensity_ - vp2.gyroNoiseDensity_) <= tol) &&
        (fabs(accNoiseDensity_ - vp2.accNoiseDensity_) <= tol) &&
        (fabs(imuIntegrationSigma_ - vp2.imuIntegrationSigma_) <= tol) &&
        (fabs(gyroBiasSigma_ - vp2.gyroBiasSigma_) <= tol) &&
        (fabs(accBiasSigma_ - vp2.accBiasSigma_) <= tol) &&
        (fabs(n_gravity_(0) - vp2.n_gravity_(0)) <= tol) &&
        (fabs(n_gravity_(1) - vp2.n_gravity_(1)) <= tol) &&
        (fabs(n_gravity_(2) - vp2.n_gravity_(2)) <= tol) &&
        (fabs(nominalImuRate_ - vp2.nominalImuRate_) <= tol) &&
        // INITIALIZATION
        (autoInitialize_ == vp2.autoInitialize_) &&
        (roundOnAutoInitialize_ == vp2.roundOnAutoInitialize_) &&
        (fabs(initialPositionSigma_ - vp2.initialPositionSigma_) <= tol) &&
        (fabs(initialRollPitchSigma_ - vp2.initialRollPitchSigma_) <= tol) &&
        (fabs(initialYawSigma_ - vp2.initialYawSigma_) <= tol) &&
        (fabs(initialVelocitySigma_ - vp2.initialVelocitySigma_) <= tol) &&
        (fabs(initialAccBiasSigma_ - vp2.initialAccBiasSigma_) <= tol) &&
        (fabs(initialGyroBiasSigma_ - vp2.initialGyroBiasSigma_) <= tol) &&
        // VISION PARAMS
        (linearizationMode_ == vp2.linearizationMode_) &&
        (degeneracyMode_ == vp2.degeneracyMode_) &&
        (fabs(smartNoiseSigma_ - vp2.smartNoiseSigma_) <= tol) &&
        (fabs(rankTolerance_ - vp2.rankTolerance_) <= tol) &&
        (fabs(landmarkDistanceThreshold_ - vp2.landmarkDistanceThreshold_) <= tol) &&
        (fabs(outlierRejection_ - vp2.outlierRejection_) <= tol) &&
        (fabs(retriangulationThreshold_ - vp2.retriangulationThreshold_) <= tol) &&
        (addBetweenStereoFactors_ == vp2.addBetweenStereoFactors_) &&
        (fabs(betweenRotationPrecision_ - vp2.betweenRotationPrecision_) <= tol) &&
        (fabs(betweenTranslationPrecision_ - vp2.betweenTranslationPrecision_) <= tol) &&
        // OPTIMIZATION PARAMS
        (fabs(relinearizeThreshold_ - vp2.relinearizeThreshold_) <= tol) &&
        (relinearizeSkip_ == vp2.relinearizeSkip_) &&
        (fabs(zeroVelocitySigma_ - vp2.zeroVelocitySigma_) <= tol) &&
        (fabs(noMotionPositionSigma_ - vp2.noMotionPositionSigma_) <= tol) &&
        (fabs(noMotionRotationSigma_ - vp2.noMotionRotationSigma_) <= tol) &&
        (fabs(constantVelSigma_ - vp2.constantVelSigma_) <= tol) &&
        (numOptimize_ == vp2.numOptimize_) &&
        (horizon_ == vp2.horizon_) &&
        (useDogLeg_ == vp2.useDogLeg_);
  }

  /* ------------------------------------------------------------------------------------- */
  void printVioBackEndParams() const {
    LOG(INFO) << "$$$$$$$$$$$$$$$$$$$$$ VIO PARAMETERS $$$$$$$$$$$$$$$$$$$$$\n"
              << "** IMU parameters **\n"
              << "gyroNoiseDensity_: " << gyroNoiseDensity_ << '\n'
              << "accNoiseDensity_: " << accNoiseDensity_ << '\n'
              << "imuIntegrationSigma_: " << imuIntegrationSigma_ << '\n'
              << "gyroBiasSigma_: " << gyroBiasSigma_ << '\n'
              << "accBiasSigma_: " << accBiasSigma_ << '\n'
              << "n_gravity_: " << n_gravity_.transpose() << '\n'
              << "nominalImuRate_: " << nominalImuRate_ << '\n'

              << "** INITIALIZATION parameters **\n"
              << "autoInitialize_: " << autoInitialize_ << '\n'
              << "roundOnAutoInitialize_: " << roundOnAutoInitialize_ << '\n'
              << "initialPositionSigma: " << initialPositionSigma_ << '\n'
              << "initialRollPitchSigma: " << initialRollPitchSigma_ << '\n'
              << "initialYawSigma: " << initialYawSigma_ << '\n'
              << "initialVelocitySigma: " << initialVelocitySigma_ << '\n'
              << "initialAccBiasSigma: " << initialAccBiasSigma_ << '\n'
              << "initialGyroBiasSigma: " << initialGyroBiasSigma_ << '\n'

              << "** VISION parameters **\n"
              << "linearizationMode_: " << linearizationMode_
              << " HESSIAN, IMPLICIT_SCHUR, JACOBIAN_Q, JACOBIAN_SVD \n"
              << "degeneracyMode_: " << degeneracyMode_
              << " IGNORE_DEGENERACY, ZERO_ON_DEGENERACY, HANDLE_INFINITY \n"
              << "rankTolerance_: " << rankTolerance_ << '\n'
              << "landmarkDistanceThreshold_: " << landmarkDistanceThreshold_ << '\n'
              << "outlierRejection_: " << outlierRejection_ << '\n'
              << "retriangulationThreshold_: " << retriangulationThreshold_ << '\n'
              << "addBetweenStereoFactors_: " << addBetweenStereoFactors_ << '\n'
              << "betweenRotationPrecision_: " << betweenRotationPrecision_ << '\n'
              << "betweenTranslationPrecision_: " << betweenTranslationPrecision_ << '\n'

              << "** OPTIMIZATION parameters **\n"
              << "relinearizeThreshold_: " << relinearizeThreshold_ << '\n'
              << "relinearizeSkip_: " << relinearizeSkip_ << '\n'
              << "zeroVelocitySigma_: " << zeroVelocitySigma_ << '\n'
              << "noMotionPositionSigma_: " << noMotionPositionSigma_ << '\n'
              << "noMotionRotationSigma_: " << noMotionRotationSigma_ << '\n'
              << "constantVelSigma_: " << constantVelSigma_ << '\n'
              << "numOptimize_: " << numOptimize_ << '\n'
              << "horizon_: " << horizon_ << '\n'
              << "useDogLeg_: " << useDogLeg_;
  }
};
typedef std::shared_ptr<VioBackEndParams> VioBackEndParamsPtr;
typedef std::shared_ptr<const VioBackEndParams> VioBackEndParamsConstPtr;

} // namespace VIO
#endif /* VioBackEndParams_H_ */


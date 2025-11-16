/* Parameter definitions for the plugin
 * 
 */


#pragma once

namespace Params
{
  // frequency defitions
  inline constexpr auto FREQ_20_HZ = 20.f;
  inline constexpr auto FREQ_20000_HZ = 20000.f;
  // default values
  inline constexpr auto DEFAULT_SKEW = 0.25f;       // non-linear skew
  inline constexpr auto DEFAULT_INTERVAL_FILTER = 1.f;
  inline constexpr auto DEFAULT_INTERVAL_GAIN = 0.5f;
  inline constexpr auto DEFAULT_SLOPE_STEP = 12;
  inline constexpr auto DEFAULT_SLOPE_COUNT = 5; // up to 48 dB/octave
  // min and maxes
  inline constexpr auto MIN_GAIN = -24.f;
  inline constexpr auto MAX_GAIN = 24.f;
  inline constexpr auto MIN_Q_FACTOR = 0.1f;
  inline constexpr auto MAX_Q_FACTOR = 10.f  ;
  // parameter IDS
  inline constexpr auto PID_LOCUT_FREQ = "LoCut Freq";
  inline constexpr auto PID_HICUT_FREQ = "HiCut Freq";
  inline constexpr auto PID_LOCUT_SLOPE = "LoCut Slope"; inline constexpr auto PID_HICUT_SLOPE = "HiCut Slope";
  inline constexpr auto PID_PEAK_FREQ = "Peak Freq";
  inline constexpr auto PID_PEAK_SLOPE = "Peak Slope";
  inline constexpr auto PID_PEAK_QUALITY = "Peak Quality";
  inline constexpr auto PID_PEAK_GAIN = "Peak Gain";
}

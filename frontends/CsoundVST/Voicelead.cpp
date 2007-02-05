/**
 * C S O U N D   V S T
 *
 * A VST plugin version of Csound, with Python scripting.
 *
 * L I C E N S E
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "Voicelead.hpp"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <iostream>
#include <map>
#include <vector>
#include <set>

std::ostream &operator << (std::ostream &stream,
                           const std::vector<double> &chord)
{
  stream << "[";
  for (size_t i = 0, n = chord.size(); i < n; i++) {
    if (i > 0) {
      stream << ", ";
    }
    stream << chord[i];
  }
  stream << "]";
  return stream;
}

namespace csound
{
  static int debug = 0;

  std::map<size_t, std::vector< std::vector<double> > > primeChordsForDivisionsPerOctave;
  std::map<size_t, std::map<double, double> > pForCForDivisionsPerOctave;
  std::map<size_t, std::map<double, double> > cForPForDivisionsPerOctave;
  std::map<size_t, std::map< std::vector<double>, double> > pForPrimeChordsForDivisionsPerOctave;

  void Voicelead::initializePrimeChordsForDivisionsPerOctave(size_t divisionsPerOctave)
  {
    if (primeChordsForDivisionsPerOctave.find(divisionsPerOctave) == primeChordsForDivisionsPerOctave.end()) {
      for (double C = 0.0, P = 0.0, N = std::pow(2.0, double(divisionsPerOctave)) - 1; C < N; ++C) {
	std::vector<double> chord = mToPitchClassSet(cToM(C, divisionsPerOctave), divisionsPerOctave);
	std::vector<double> normalChord_ = normalChord(chord);
	std::vector<double> zeroChord = toOrigin(normalChord_);
	if (normalChord_ == zeroChord) {
	  primeChordsForDivisionsPerOctave[divisionsPerOctave].push_back(zeroChord);
	  pForCForDivisionsPerOctave[divisionsPerOctave][C] = P;
	  cForPForDivisionsPerOctave[divisionsPerOctave][P] = C;
	  pForPrimeChordsForDivisionsPerOctave[divisionsPerOctave][zeroChord] = P;
	  P = P + 1.0;
	}
      }
    }
  }
  
  double round(double x)
  {
    return std::floor(x + 0.5);
  }

  std::vector<double> sort(const std::vector<double> &chord)
  {
    std::vector<double> sorted(chord);
    std::sort(sorted.begin(), sorted.end());
    return sorted;
  }

  double Voicelead::pc(double p, size_t divisionsPerOctave)
  {
    p = std::fabs(round(p));
    return double(int(round(p)) % divisionsPerOctave);
  }

  double Voicelead::pitchClassSetToM(const std::vector<double> &chord, size_t divisionsPerOctave)
  {
    std::set<double> pcs_;
    double M = 0.0;
    for (size_t i = 0, n = chord.size(); i < n; i++) {
      double pc_ = pc(chord[i], divisionsPerOctave);
      if (pcs_.find(pc_) == pcs_.end()) {
	pcs_.insert(pc_);
	M = M + std::pow(2.0, pc_);
      }
    }
    return M;
  }
  
  std::vector<double> Voicelead::mToPitchClassSet(double M, size_t divisionsPerOctave)
  {
    size_t M_ = size_t(round(M));
    std::vector<double> pcs;
    for (double i = 0.0; i < double(divisionsPerOctave); i = i + 1.0) {
      size_t p2 = size_t(std::pow(2.0, i));
      if ((p2 & M_) == p2) {
        pcs.push_back(i);
      }
    }
    return pcs;
  }

  std::vector<double> Voicelead::pitchClassSetToPandT(const std::vector<double> &pcs_, 
									size_t divisionsPerOctave)
  {
    std::vector<double> normalChord_ = normalChord(pcs_);
    std::vector<double> zeroChord_ = toOrigin(normalChord_);
    double M = pitchClassSetToM(zeroChord_, divisionsPerOctave);
    double C = mToC(M, divisionsPerOctave);
    double P = cToP(C, divisionsPerOctave);
    std::vector<double> result(2);
    result[0] = P;
    result[1] = normalChord_[0];
    return result;
  }


  std::vector<double> Voicelead::voiceleading(const std::vector<double> &a,
                                              const std::vector<double> &b)
  {
    std::vector<double> v(a.size());
    for (size_t i = 0, n = a.size(); i < n; i++) {
      v[i] = (b[i] - a[i]);
    }
    return v;
  }

  const std::vector<double> &Voicelead::simpler(const std::vector<double> &source,
                                                const std::vector<double> &destination1,
                                                const std::vector<double> &destination2,
                                                bool avoidParallels)
  {
    std::vector<double> v1 = voiceleading(source, destination1);
    std::sort(v1.begin(), v1.end());
    std::vector<double> v2 = voiceleading(source, destination2);
    std::sort(v2.begin(), v2.end());
    for (size_t i = v1.size() - 1; i >= 0; i--) {
      if(v1[i] < v2[i]) {
        return destination1;
      }
      if(v2[i] > v1[i]) {
        return destination2;
      }
    }
    return destination1;
  }

  double Voicelead::smoothness(const std::vector<double> &a,
                               const std::vector<double> &b)
  {
    double L1 = 0.0;
    for (size_t i = 0, n = a.size(); i < n; i++) {
      L1 += std::fabs(b[i] - a[i]);
    }
    return L1;
  }

  bool Voicelead::areParallel(const std::vector<double> &a,
                              const std::vector<double> &b)
  {
    for (size_t i = 0, n = a.size(); i < n; i++) {
      for (size_t j = 0, k = b.size(); j < k; j++) {
        if (i != j) {
          if ( ((a[i] - a[j]) ==  7.0 && (b[i] - b[j]) ==  7.0) ||
               ((a[i] - a[j]) == -7.0 && (b[i] - b[j]) == -7.0) ) {
            if (debug > 1) {
              std::cout << "Parallel fifth: " << std::endl;
              std::cout << " chord 1: " << a << std::endl;
              std::cout << " leading: " << voiceleading(a, b) << std::endl;
              std::cout << " chord 2: " << b << std::endl;
            }
            return true;
          }
        }
      }
    }
    return false;
  }

  const std::vector<double> &Voicelead::closer(const std::vector<double> &source,
                                               const std::vector<double> &destination1,
                                               const std::vector<double> &destination2,
                                               bool avoidParallels)
  {
    if (avoidParallels) {
      if (areParallel(source, destination1)) {
        return destination2;
      }
      if (areParallel(source, destination2)) {
        return destination1;
      }
    }
    double s1 = smoothness(source, destination1);
    double s2 = smoothness(source, destination2);
    if (s1 < s2) {
      return destination1;
    }
    if (s2 < s1) {
      return destination2;
    }
    return simpler(source, destination1, destination2, avoidParallels);
  }

  std::vector<double> Voicelead::rotate(const std::vector<double> &chord)
  {
    std::vector<double> inversion;
    for (size_t i = 1, n = chord.size(); i < n; i++) {
      inversion.push_back(chord[i]);
    }
    inversion.push_back(chord[0]);
    return inversion;
  }

  std::vector< std::vector<double> > Voicelead::rotations(const std::vector<double> &chord)
  {
    std::vector< std::vector<double> > rotations_;
    std::vector<double> inversion = pcs(chord);
    if (debug > 1) {
      std::cout << "rotating:   " << chord << std::endl;
      std::cout << "rotation 1: " << inversion << std::endl;
    }
    rotations_.push_back(inversion);
    for (size_t i = 1, n = chord.size(); i < n; i++) {
      inversion = rotate(inversion);
      if (debug > 1) {
        std::cout << "rotation " << (i+1) << ": " << inversion << std::endl;
      }
      rotations_.push_back(inversion);
    }
    if (debug > 1) {
      std::cout << std::endl;
    }
    return rotations_;
  }

  std::vector< std::vector<double> > pitchRotations(const std::vector<double> &chord)
  {
    std::vector< std::vector<double> > rotations_;
    std::vector<double> inversion = chord;
    rotations_.push_back(inversion);
    for (size_t i = 1, n = chord.size(); i < n; i++) {
      inversion = Voicelead::rotate(inversion);
      rotations_.push_back(inversion);
    }
    return rotations_;
  }

  std::vector<double> Voicelead::pcs(const std::vector<double> &chord, size_t divisionsPerOctave)
  {
    std::vector<double> pcs_(chord.size());
    for (size_t i = 0, n = chord.size(); i < n; i++) {
      pcs_[i] = pc(chord[i], divisionsPerOctave);
    }
    if (debug > 1) {
      std::cout << "chord: " << chord << std::endl;
      std::cout << "pcs: " << pcs_ << std::endl;
    }
    std::sort(pcs_.begin(), pcs_.end());
    return pcs_;
  }

  std::vector<double> Voicelead::uniquePcs(const std::vector<double> &chord, size_t divisionsPerOctave)
  {
    std::vector<double> pcs_;
    std::set<double> pcsSet;
    for (size_t i = 0, n = chord.size(); i < n; i++) {
      double pc_ = pc(chord[i], divisionsPerOctave);
      if (pcsSet.find(pc_) == pcsSet.end()) {
	pcsSet.insert(pc_);
	pcs_.push_back(pc_);
      }
    }
    std::sort(pcs_.begin(), pcs_.end());
    if (debug > 1) {
      std::cout << "chord: " << chord << std::endl;
      std::cout << "pcs: " << pcs_ << std::endl;
    }
    return pcs_;
  }

  const std::vector<double> Voicelead::closest(const std::vector<double> &source,
					       const std::vector< std::vector<double> > &targets,
					       bool avoidParallels)
  {
    if (targets.size() == 0) {
      return source;
    } else if (targets.size() == 1) {
      return targets[0];
    }
    std::vector<double> t1 = targets[0];
    for (size_t i = 1, n = targets.size(); i < n; i++) {
      t1 = closer(source, t1, targets[i], avoidParallels);
    }
    return t1;
  }

  // Recursively enumerate inversions of the original chord
  // that fit within the maximum pitch.

  void inversions(const std::vector<double> &original,
                  const std::vector<double> &iterator,
                  size_t voice,
                  double maximum,
                  std::set< std::vector<double> > &chords,
                  size_t divisionsPerOctave)
  {
    if (voice >= original.size()) {
      return;
    }
    std::vector<double> iterator_ = iterator;
    for (double pitch = original[voice]; pitch < maximum; pitch = pitch + double(divisionsPerOctave)) {
      iterator_[voice] = pitch;
      chords.insert(sort(iterator_));
      inversions(original, iterator_, voice + 1, maximum, chords, divisionsPerOctave);
    }
  }

  std::vector< std::vector<double> > Voicelead::voicings(const std::vector<double> &chord,
                                                         double lowest,
                                                         double range,
                                                         size_t divisionsPerOctave)
  {
    // Find the smallest inversion of the chord that is closest to the lowest pitch, but no lower.
    std::vector<double> inversion = pcs(chord, divisionsPerOctave);
    for(;;) {
      std::vector<double>::iterator it = std::min_element(inversion.begin(), inversion.end());
      if (lowest <= *it) {
        break;
      }
      inversion = invert(inversion);
    }
    // Generate all permutations of that inversion.
    std::vector< std::vector<double> > rotations_ = pitchRotations(inversion);
    // Iterate through all inversions of those permutations within the range.
    std::set< std::vector<double> > inversions_;
    for (size_t i = 0, n = rotations_.size(); i < n; i++) {
      std::vector<double> iterator = rotations_[i];
      csound::inversions(rotations_[i], iterator, 0, lowest + range, inversions_, divisionsPerOctave);
    }
    // To have a random access but ordered set, return it as a vector.
    std::vector< std::vector<double> > inversions__;
    for(std::set< std::vector<double > >::iterator it = inversions_.begin(); it != inversions_.end(); ++it) {
      inversions__.push_back(*it);
    }
    return inversions__;
  }

  /**
   * Bijective voiceleading first by closeness, then by simplicity,
   * with optional avoidance of parallel fifths.
   */
  std::vector<double> Voicelead::voicelead(const std::vector<double> &source_,
                                           const std::vector<double> &target_,
                                           double lowest,
                                           double range,
                                           bool avoidParallels,
                                           size_t divisionsPerOctave)
  {
    std::vector<double> source = source_;
    std::vector<double> target = target_;
    std::vector< std::vector<double> > voicings_ = voicings(target, lowest, range, divisionsPerOctave);
    std::vector<double> voicing = closest(source, voicings_, avoidParallels);
    if (debug) {
      std::cout << "   From: " << source_ << std::endl;
      std::cout << "     To: " << target_ << std::endl;
      std::cout << "Leading: " << voiceleading(source_, voicing) << std::endl;
      std::cout << "     Is: " << voicing << std::endl << std::endl;
    }
    return voicing;
  }

  void recursiveVoicelead_(const std::vector<double> &source,
                           const std::vector<double> &original,
                           const std::vector<double> &iterator,
                           std::vector<double> &target,
                           size_t voice,
                           double maximum,
                           bool avoidParallels,
                           size_t divisionsPerOctave)
  {
    if (voice >= original.size()) {
      return;
    }
    std::vector<double> iterator_ = iterator;
    for (double pitch = original[voice]; pitch < maximum; pitch = pitch + double(divisionsPerOctave)) {
      iterator_[voice] = pitch;
      target = Voicelead::closer(source, iterator_, target, avoidParallels);
      recursiveVoicelead_(source, original, iterator_, target, voice + 1, maximum, avoidParallels, divisionsPerOctave);
    }
  }

  /**
   * Bijective voiceleading first by closeness, then by simplicity,
   * with optional avoidance of parallel fifths.
   */
  std::vector<double> Voicelead::recursiveVoicelead(const std::vector<double> &source_,
                                                    const std::vector<double> &target_,
                                                    double lowest,
                                                    double range,
                                                    bool avoidParallels,
                                                    size_t divisionsPerOctave)
  {
    std::vector<double> source = source_;
    std::vector<double> target = target_;
    // Find the smallest inversion of the target chord
    // that is closest to the lowest pitch, but no lower.
    std::vector<double> inversion = pcs(target, divisionsPerOctave);
    for(;;) {
      std::vector<double>::iterator it = std::min_element(inversion.begin(), inversion.end());
      if (lowest <= *it) {
        break;
      }
      inversion = invert(inversion);
    }
    // Generate all permutations of that inversion.
    std::vector< std::vector<double> > rotations_ = pitchRotations(inversion);
    // Iterate through all inversions of those permutations within the range.
    std::set< std::vector<double> > inversions_;
    std::vector<double> voicing;
    for (size_t i = 0, n = rotations_.size(); i < n; i++) {
      std::vector<double> iterator = rotations_[i];
      if (i == 0) {
        voicing = iterator;
      }
      recursiveVoicelead_(source, rotations_[i], iterator,  voicing, 0,         lowest + range, avoidParallels, divisionsPerOctave);
    }
    if (debug) {
      std::cout << "   From: " << source_ << std::endl;
      std::cout << "     To: " << target_ << std::endl;
      std::cout << "Leading: " << voiceleading(source_, voicing) << std::endl;
      std::cout << "     Is: " << voicing << std::endl << std::endl;
    }
    return voicing;
  }

  double Voicelead::closestPitch(double pitch, const std::vector<double> &pitches_)
  {
    std::vector<double> pitches = pitches_;
    std::sort(pitches.begin(), pitches.end());
    // its.first is the first iterator not less than pitch, or end if all are less.
    // its.second is the first iterator greater than pitch, or end if none are greater.
    std::pair< std::vector<double>::iterator, std::vector<double>::iterator > its = std::equal_range(pitches.begin(), pitches.end(), pitch);
    if (its.first == pitches.end()) {
      return pitches.back();
    }
    if (its.first == pitches.begin()) {
      return pitches.front();
    }
    double lower = *its.first;
    if (pitch == lower) {
      return pitch;
    }
    // If lower isn't pitch,
    // then pitch always lies in the interval (first - 1, second).
    double lowerP = *(its.first - 1);
    double upperP = *its.second;
    double lowerD = std::fabs(pitch - lowerP);
    double upperD = std::fabs(upperP - pitch);
    if (lowerD <= upperD) {
      return lowerP;
    } else {
      return upperP;
    }
  }

  double Voicelead::conformToPitchClassSet(double pitch, const std::vector<double> &pcs, size_t divisionsPerOctave_)
  {
    double divisionsPerOctave = round(double(divisionsPerOctave_));
    double pc_ = pc(pitch);
    double closestPc = closestPitch(pc_, pcs);
    double octave = std::floor(pitch / divisionsPerOctave) * divisionsPerOctave;
    double closestPitch = octave + closestPc;
    return closestPitch;
  }

  double Voicelead::euclideanDistance(const std::vector<double> &chord1, const std::vector<double> &chord2)
  {
    double ss = 0.0;
    for (size_t i = 0, n = chord1.size(); i < n; i++) {
      ss  = ss + std::pow((chord1[i] - chord2[i]), 2.0);
    }
    return std::sqrt(ss);
  }

  std::vector<double>  Voicelead::toOrigin(const std::vector<double> &chord_)
  {
    std::vector<double> chord = chord_;
    double minimum = *std::min_element(chord.begin(), chord.end());
    for (size_t i = 0, n = chord.size(); i < n; i++) {
      chord[i] = chord[i] - minimum;
    }
    return chord;
  }

  std::vector<double> Voicelead::invert(const std::vector<double> &chord)
  {
    std::vector<double> inversion;
    for (size_t i = 1, n = chord.size(); i < n; i++) {
      inversion.push_back(chord[i]);
    }
    inversion.push_back(chord[0] + 12.0);
    return inversion;
  }

  std::vector< std::vector<double> > Voicelead::inversions(const std::vector<double> &chord)
  {
    std::vector< std::vector<double> > inversions_;
    std::vector<double> inversion = pcs(chord);
    inversions_.push_back(inversion);
    for (size_t i = 1, n = chord.size(); i < n; i++) {
      inversion = invert(inversion);
      inversions_.push_back(inversion);
    }
    return inversions_;
  }

  std::vector<double>  Voicelead::normalChord(const std::vector<double> &chord)
  {
    std::vector< std::vector<double> > inversions_ = inversions(chord);
    std::vector<double> origin(chord.size(), 0.0);
    std::vector<double> normalChord;
    double minDistance = 0;
    for (size_t i = 0, n = inversions_.size(); i < n; i++) {
      std::vector<double> zeroChordInversion = toOrigin(inversions_[i]);
      if (i == 0) {
        normalChord = inversions_[i];
        minDistance = euclideanDistance(zeroChordInversion, origin);
      } else {
        double distance = euclideanDistance(zeroChordInversion, origin);
        if (distance < minDistance) {
          minDistance = distance;
          normalChord = inversions_[i];
        }
      }
    }
    return normalChord;
  }

  std::vector<double> Voicelead::primeChord(const std::vector<double> &chord)
  {
    return toOrigin(normalChord(chord));
  }

  double Voicelead::nameToC(std::string name, size_t divisionsPerOctave_)
  {
    double M = Conversions::nameToM(name);
    return mToC(M, divisionsPerOctave_);
  }

  double Voicelead::mToC(double M, size_t divisionsPerOctave)
  {
    int C = int(std::fabs(M + 0.5)) - 1;
    int modulus = int(std::pow(2.0, double(divisionsPerOctave))) - 1;
    C = C % modulus;
    return double(C);
  }

  double Voicelead::cToM(double C, size_t divisionsPerOctave)
  {
    int M = int(std::fabs(C + 0.5)) + 1;
    int modulus = int(std::pow(2.0, double(divisionsPerOctave))) - 1;
    M = M % modulus;
    return double(M);
  }
  
  double Voicelead::cToP(double C, size_t divisionsPerOctave)
  {
    initializePrimeChordsForDivisionsPerOctave(divisionsPerOctave);
    double M = cToM(C, divisionsPerOctave);
    std::vector<double> pitchClassSet = mToPitchClassSet(M, divisionsPerOctave);
    std::vector<double> primeChord_ = primeChord(pitchClassSet);
    return pForPrimeChordsForDivisionsPerOctave[divisionsPerOctave][primeChord_];
  }
  
  double Voicelead::pToC(double P, size_t divisionsPerOctave)
  {
    initializePrimeChordsForDivisionsPerOctave(divisionsPerOctave);
    int p = int(std::fabs(P + 0.5));
    int modulus = int(primeChordsForDivisionsPerOctave.size());
    P = double(p % modulus);
    return cForPForDivisionsPerOctave[divisionsPerOctave][P];
  }

  std::vector<double> Voicelead::pToPrimeChord(double P, size_t divisionsPerOctave)
  {
    initializePrimeChordsForDivisionsPerOctave(divisionsPerOctave);
    size_t p = size_t(round(P));
    p = p % primeChordsForDivisionsPerOctave[divisionsPerOctave].size();
    return primeChordsForDivisionsPerOctave[divisionsPerOctave][p];
  }
  

  std::vector<double> Voicelead::pAndTtoPitchClassSet(double P, 
						      double T, 
						      size_t divisionsPerOctave)
  {
    std::vector<double> pitchClassSet = pToPrimeChord(P, divisionsPerOctave);
    for (size_t i = 0, n = pitchClassSet.size(); i < n; i++) {
      double pitch = pitchClassSet[i] + T;
      pitchClassSet[i] = pc(pitch, divisionsPerOctave);
    }
    std::sort(pitchClassSet.begin(), pitchClassSet.end());
    return pitchClassSet;
  }

  std::vector<double> Voicelead::orderedPcs(const std::vector<double> &chord, size_t divisionsPerOctave)
  {
    std::vector<double> pcs_(chord.size());
    for (size_t i = 0, n = chord.size(); i < n; i++) {
      pcs_[i] = pc(chord[i], divisionsPerOctave);
    }
    return pcs_;
  }

  struct AscendingDistanceComparator
  {
    double origin;
    size_t divisionsPerOctave;
    AscendingDistanceComparator(double origin_, size_t divisionsPerOctave_) : origin(origin_), divisionsPerOctave(divisionsPerOctave_)
    {
    }
    double ascendingDistance(double a, double b)
    {
      double pcA = Voicelead::pc(a, divisionsPerOctave);
      double pcB = Voicelead::pc(b, divisionsPerOctave);
      double d = pcB - pcA;
      if (d < 0.0) {
	d  = (pcB + double(divisionsPerOctave)) - pcA;
      }
      return d;  
    }
    bool operator()(double a, double b) 
    {
      double dA = ascendingDistance(origin, a);
      double dB = ascendingDistance(origin, b);
      return (dA < dB);
    }
  };

  struct MatrixCell
  {
    size_t i;
    size_t j;
    std::vector<double> s;
    std::vector<double> a;
    std::vector<double> b;
    std::vector<double> v;
    double d;
    MatrixCell() : i(0), j(0), d(0.0)
    {
    }
  };

  const MatrixCell &minimumCell(const MatrixCell &a, const MatrixCell &b, const MatrixCell &c)
  {
    if (a.d < b.d && a.d < c.d) {
      return a;
    } else if (b.d < a.d && b.d < c.d) {
      return b;
    } else {
      return c;
    }
  }

  std::vector< std::vector<MatrixCell> > createMatrix(const std::vector<double> &sourceMultiset_, 
						      const std::vector<double> &targetMultiset_, 
						      const std::vector<double> &sourceChord_)
  {
    std::vector<double> sourceMultiset = sourceMultiset_;
    std::vector<double> targetMultiset = targetMultiset_;
    std::vector<double> sourceChord =    sourceChord_;
    sourceMultiset.push_back(sourceMultiset[0]);
    targetMultiset.push_back(targetMultiset[0]);
    sourceChord.push_back   (sourceChord   [0]);
    size_t N = sourceMultiset.size();
    std::vector< std::vector<MatrixCell> > matrix;
    for (size_t i = 0; i < N; i++) {
      matrix.push_back(std::vector<MatrixCell>(N));
    }
    for (size_t i = 0; i < N; i++) {
      for (size_t j = 0; j < N; j++) {
	MatrixCell cell;
	if (i == 0 && j == 0) {
	  cell = matrix[i    ][j    ];
	} else if (i == 0 && j != 0) {
	  cell = matrix[i    ][j - 1];
	} else if (i != 0 && j == 0) {
	  cell = matrix[i - 1][j    ];
	} else {
	  const MatrixCell &a = matrix[i    ][j - 1];
	  const MatrixCell &b = matrix[i - j][j    ];
	  const MatrixCell &c = matrix[i - 1][j - 1];
	  cell = minimumCell(a, b, c);
	}
	cell.i = i;
	cell.j = j;
	cell.s.push_back(sourceChord[i]);
	cell.a.push_back(sourceMultiset[i]);
	cell.b.push_back(targetMultiset[j]);
	cell.v = Voicelead::voiceleading(cell.a, cell.b);
	cell.d = Voicelead::smoothness(cell.a, cell.b);
	matrix[i][j] = cell;
      }
    }
    return matrix;
  }

  std::vector<double> Voicelead::sortByAscendingDistance(const std::vector<double> &chord, size_t divisionsPerOctave)
  {
    std::vector<double> copy(chord);
    AscendingDistanceComparator comparator(chord[0], divisionsPerOctave);
    std::sort(copy.begin(), copy.end(), comparator);
    return copy;
  }

  std::vector< std::vector<double> > Voicelead::nonBijectiveVoicelead(const std::vector<double> &sourceChord, 
								     const std::vector<double> &targetPitchClassSet,
								     size_t divisionsPerOctave)
  {
    std::vector<double> sortedSourceChord = sortByAscendingDistance(sourceChord, divisionsPerOctave);
    std::vector<double> resultChord = sortedSourceChord;
    std::vector<double> sourceTones = orderedPcs(sortedSourceChord, divisionsPerOctave);
    std::vector<double> targetTones = orderedPcs(resultChord, divisionsPerOctave);
    std::vector<double> sourceMultiset = sortByAscendingDistance(sourceTones, divisionsPerOctave);
    std::vector<double> targetMultiset = sortByAscendingDistance(targetTones, divisionsPerOctave);
    std::vector< std::vector<double> > targetMultisets = rotations(targetMultiset);
    std::map<double, MatrixCell> cellsForDistances;
    for (size_t i = 0, n = targetMultisets.size(); i < n; i++) {
      const std::vector<double> &targetMultiset = targetMultisets[i];
      std::vector< std::vector<MatrixCell> > matrix = createMatrix(sourceMultiset, targetMultiset, sortedSourceChord);
      size_t corner = sourceMultiset.size();
      const MatrixCell &cell = matrix[corner][corner];
      cellsForDistances[cell.d] = cell;
    }
    MatrixCell resultCell = std::min_element(cellsForDistances.begin(), cellsForDistances.end(), cellsForDistances.value_comp())->second;
    std::vector<double> returnedVoiceleading(resultCell.v.begin(), resultCell.v.end() - 1);
    std::vector<double> returnedSourceChord(resultCell.s.begin(), resultCell.s.end() - 1);
    std::vector<double> returnedResultChord = returnedSourceChord;
    for (size_t i = 0, n = returnedVoiceleading.size(); i < n; i++) {
      returnedResultChord[i] = returnedSourceChord[i] + returnedVoiceleading[i]; 
    }
    std::vector< std::vector<double> > result;
    result.push_back(returnedSourceChord);
    result.push_back(returnedVoiceleading);
    result.push_back(returnedResultChord);
    return result;
  }
}

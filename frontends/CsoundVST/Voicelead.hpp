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
#ifndef CSOUND_VOICELEAD_HPP
#define CSOUND_VOICELEAD_HPP

#include "Platform.hpp"
#ifdef SWIG
%module CsoundVST
%{
#ifndef TEST
#include "Event.hpp"
#endif
#include <vector>
#include <algorithm>
#include <cmath>
  %}
%include "std_vector.i"
%template(ChordVector) std::vector< std::vector<double> >;
#else
#ifndef TEST
#include "Event.hpp"
#endif
#include <vector>
#endif

namespace csound
{
  /**
   * This class contains facilities for
   * voiceleading, harmonic progression,
   * and identifying chord types.
   *
   * See: http://ruccas.org/pub/Gogins/music_atoms.pdf
   */
  class Voicelead
  {
  public:
    /**
     * Return the pitch-class of the pitch.
     * The octave is always defined as 12 semitones.
     * If the number of divisions per octave is also 12,
     * then the pitch-class of a pitch is an integer.
     * If the number of divisions per octave is not 12,
     * then the pitch-class is not necessarily an integer;
     * but this method rounds off the pitch to its exact
     * pitch-class.
     */
    static double pc(double pitch, size_t divisionsPerOctave = 12);


    /**
     * Return the voice-leading vector (difference) 
     * between chord1 and chord2.
     */
    static std::vector<double> voiceleading(const std::vector<double> &chord1,
                                            const std::vector<double> &chord2);

    /**
     * Return the simpler (fewer motions) of the voiceleadings
     * between source chord and either destination1 or destination2,
     * optionally avoiding parallel fifths.
     */
    static const std::vector<double> &simpler(const std::vector<double> &source,
                                              const std::vector<double> &destination1,
                                              const std::vector<double> &destination2,
                                              bool avoidParallels);

    /**
     * Return the smoothness (distance by taxicab or L1 norm)
     * of the voiceleading between chord1 and chord2.
     */
    static double smoothness(const std::vector<double> &chord1,
                             const std::vector<double> &chord1);

    /**
     * Return the Euclidean distance between two chords,
     * which must have the same number of voices.
     */
    static double euclideanDistance(const std::vector<double> &chord1,
                                    const std::vector<double> &chord2);

    /*
     * Return whether the progression between chord1 and chord2
     * contains a parallel fifth.
     */
    static bool areParallel(const std::vector<double> &chord1,
                            const std::vector<double> &chord2);

    /**
     * Return the closer, first by smoothness then by simplicity.,
     * of the voiceleadings between source and either
     * destination1 or destination2, optionally avoiding
     * parallel fifths.
     */
    static const std::vector<double> &closer(const std::vector<double> &source,
                                             const std::vector<double> &destination1,
                                             const std::vector<double> &destination2,
                                             bool avoidParallels);

    /**
     * Return the chord with the first note rotated to the last note.
     */
    static std::vector<double> rotate(const std::vector<double> &chord);

    /**
     * Return the set of all rotations of the chord.
     */
    static std::vector< std::vector<double> > rotations(const std::vector<double> &chord);

    /**
     * Return the chord as the list of its pitch-classes.
     * Although the list is nominally unordered, it is
     * returned sorted in ascending order. Note that pitch-classes
     * may be doubled.
     */
    static std::vector<double> pcs(const std::vector<double> &chord, size_t divisionsPerOctave = 12);

    /**
     * Return the chord as the list of its pitch-classes.
     * Although the list is nominally unordered, it is
     * returned sorted in ascending order. Note that pitch-classes
     * are NOT doubled.
     */
    static std::vector<double> uniquePcs(const std::vector<double> &chord, size_t divisionsPerOctave = 12);

    /**
     * Convert a chord to a pitch-class set number
     * N = sum (2 ^ pc). These numbers form a multiplicative cyclic
     * group. Arithmetic on this group can perform many
     * harmonic and other manipulations of pitch.
     */
    static double numberFromChord(const std::vector<double> &chord, size_t divisionsPerOctave = 12);

    /**
     * Convert a pitch-class set number to a pitch-class set chord.
     */
    static std::vector<double> pcsFromNumber(double pcn, size_t divisionsPerOctave = 12);

    /**
     * Convert a pitch-class set to a prime chord number and a transposition.
     * Note that the prime chord numbers, and transpositions, form an additive cyclic group.
     */
    static void primeAndTranspositionFromPitchClassSet(std::vector<double> pcs, 
						       double &prime, 
						       double &transposition, 
						       size_t divisionsPerOctave = 12);

    /**
     * Return all voicings of the chord
     * within the specified range. These voices
     * include all unordered permutations of the pitch-classes
     * at whatever octave fits within the range.
     * The index of this list forms an additive cyclic group.
     * Arithmetic on this group can perform many operations
     * on the voices of the chord such as revoicing, arpeggiation, and so on.
     */
    static std::vector< std::vector<double> > voicings(const std::vector<double> &chord,
                                                       double lowest,
                                                       double range,
                                                       size_t divisionsPerOctave);

    /**
     * Return the closest voiceleading within the specified range,
     * first by smoothness then by simplicity,
     * between the source chord any of the destination chords,
     * optionally avoiding parallel fifths.
     */
    static const std::vector<double> closest(const std::vector<double> &source,
                                             const std::vector< std::vector<double> > &destinations,
                                             bool avoidParallels);

    /**
     * Return the closest voiceleading within the specified range,
     * first by smoothness then by simplicity,
     * between the source chord and the target pitch-class set,
     * optionally avoiding parallel fifths.
     * The algorithm uses a brute-force search through all
     * unordered chords, which are stored in a cache,
     * fitting the target pitch-class set within
     * the specified range. Although the time complexity
     * is exponential, this is still usable for non-real-time
     * operations in most cases of musical interest.
     */
    static std::vector<double> voicelead(const std::vector<double> &source,
                                         const std::vector<double> &targetPitchClassSet,
                                         double lowest,
                                         double range,
                                         bool avoidParallels,
                                         size_t divisionsPerOctave = 12);

    /**
     * Return the closest voiceleading within the specified range,
     * first by smoothness then by simplicity,
     * between the source chord and the target pitch-class set,
     * optionally avoiding parallel fifths.
     * The algorithm uses a brute-force search through all
     * unordered chords, which are recursively enumerated,
     * fitting the target pitch-class set within
     * the specified range. Although the time complexity
     * is exponential, the algorithm is still usable
     * for non-real-time operations in most cases of musical interest.
     */
    static std::vector<double> recursiveVoicelead(const std::vector<double> &source,
                                                  const std::vector<double> &targetPitchClassSet,
                                                  double lowest,
                                                  double range,
                                                  bool avoidParallels,
                                                  size_t divisionsPerOctave = 12);

    /**
     * Return the pitch in pitches that is closest to the specified pitch.
     */
    static double closestPitch(double pitch, const std::vector<double> &pitches);

    /**
     * Return the pitch that results from making the minimum adjustment
     * to the pitch-class of the pitch argument that is required to make
     * its pitch-class the same as one of the pitch-classes in the
     * pitch-class set argument. I.e., "round up or down" to make
     * the pitch fit into a chord or scale.
     */
    static double conformToPitchClassSet(double pitch, const std::vector<double> &pcs, size_t divisionsPerOctave = 12);

    /**
     * Invert by rotating the chord and adding an octave to its last pitch.
     */
    static std::vector<double> invert(const std::vector<double> &chord);

    /**
     * Return as many inversions of the pitch-classes in the chord
     * as there are voices in the chord.
     */
    static std::vector< std::vector<double> > inversions(const std::vector<double> &chord);

    /**
     * Return the chord transposed so its lowest pitch is at the origin.
     */
    static std::vector<double> zeroChord(const std::vector<double> &chord);

    /**
     * Return the normal chord: that inversion of the pitch-classes in the chord
     * which is closest to the orthogonal axis of the Tonnetz for that chord.
     * Similar to, but not identical with, "normal form."
     */
    static std::vector<double> normalChord(const std::vector<double> &chord);

    /**
     * Return the prime chord: that inversion of the pitch-classes in the chord
     * which is closest to the orthogonal axis of the Tonnetz for that chord,
     * transposed so that its lowest pitch is at the origin.
     * Similar to, but not identical with, "prime form."
     */
    static std::vector<double> primeChord(const std::vector<double> &chord);

  };

}
#endif


//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2002-2016 Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "realizedharmony.h"
#include "pitchspelling.h"
#include "staff.h"
#include "chordlist.h"
#include "harmony.h"

namespace Ms {

//---------------------------------------------------
//   RealizedHarmony
///   creates empty realized harmony
//---------------------------------------------------
RealizedHarmony::RealizedHarmony(Harmony* h) : _harmony(h),
      _notes(QMap<int, int>()), _voicing(Voicing::AUTO), _rhythm(Rhythm::AUTO), _dirty(1)
      {
      }

//---------------------------------------------------
//   setVoicing
///   sets the voicing and dirty flag if the passed
///   voicing is different than current
//---------------------------------------------------
void RealizedHarmony::setVoicing(Voicing v)
      {
      if (_voicing == v)
            return;
      _voicing = v;
      cascadeDirty(true);
      }

//---------------------------------------------------
//   setRhythm
///   sets the rhythm and dirty flag if the passed
///   rhythm is different than current
//---------------------------------------------------
void RealizedHarmony::setRhythm(Rhythm r)
      {
      if (_rhythm == r)
            return;
      _rhythm = r;
      cascadeDirty(true);
      }

//---------------------------------------------------
//   setLiteral
///   sets literal/jazz and dirty flag if the passed
///   bool is different than current
//---------------------------------------------------
void RealizedHarmony::setLiteral(bool literal)
      {
      if (_literal == literal)
            return;
      _literal = literal;
      cascadeDirty(true);
      }

//---------------------------------------------------
//   notes
///   returns the list of notes
//---------------------------------------------------
const QMap<int, int>& RealizedHarmony::notes() const
      {
      Q_ASSERT(!_dirty);
      //with the way that the code is currently structured, there should be no way to
      //get to this function with dirty flag set although in the future it may be
      //better to just update if dirty here
      return _notes;
      }

//---------------------------------------------------
//   notes
///   returns the list of notes
//---------------------------------------------------
const QMap<int, int> RealizedHarmony::generateNotes(int rootTpc, int bassTpc,
                                                bool literal, Voicing voicing, int transposeOffset) const
      {
      QMap<int, int> notes;
      int rootPitch = tpc2pitch(rootTpc) + transposeOffset;
      //euclidian mod, we need to treat this new pitch as a pitch between
      //0 and 11, so that voicing remains consistent across transposition
      if (rootPitch < 0)
            rootPitch += PITCH_DELTA_OCTAVE;
      else
            rootPitch %= PITCH_DELTA_OCTAVE;

      //create root note or bass note in second octave below middle C
      if (bassTpc != Tpc::TPC_INVALID && voicing != Voicing::ROOT_ONLY)
            notes.insert(tpc2pitch(bassTpc) + transposeOffset
                        + 3*PITCH_DELTA_OCTAVE, bassTpc);
      else
            notes.insert(rootPitch + 3*PITCH_DELTA_OCTAVE, rootTpc);


      switch (voicing) {
            case Voicing::ROOT_ONLY:
                  break;
            case Voicing::AUTO: //auto is close voicing for now since it is the most robust
                  // FALLTHROUGH
            case Voicing::CLOSE:
                  //Voices notes in close position in the first octave above middle C
                  {
                  notes.insert(rootPitch + 5*PITCH_DELTA_OCTAVE, rootTpc);
                  //ensure that notes fall under a specific range
                  //for now this range is between 5*12 and 6*12
                  QMap<int, int> intervals = getIntervals(rootTpc, literal);
                  QMapIterator<int, int> i(intervals);
                  while (i.hasNext()) {
                        i.next();
                        notes.insert((rootPitch + (i.key() % 128)) % PITCH_DELTA_OCTAVE +
                                      5*PITCH_DELTA_OCTAVE, i.value());
                        }
                  }
                  break;
            case Voicing::OPEN:
                  break;
            case Voicing::DROP_2:
                  {
                  //select 4 notes from list
                  QMap<int, int> intervals = normalizeNoteMap(getIntervals(rootTpc, literal), rootTpc, rootPitch, 4);
                  QMapIterator<int, int> i(intervals);
                  i.toBack();

                  int counter = 0; //counter to drop the second note
                  while (i.hasPrevious()) {
                        i.previous();
                        if (++counter == 2)
                              notes.insert(i.key() + 4*PITCH_DELTA_OCTAVE, i.value());
                        else
                              notes.insert(i.key() + 5*PITCH_DELTA_OCTAVE, i.value());
                        }
                  }
                  break;
            case Voicing::THREE_NOTE:
                  {
                  QMap<int, int> intervals = normalizeNoteMap(getIntervals(rootTpc, literal), rootTpc, rootPitch, 2, true);
                  QMapIterator<int, int> i(intervals);

                  i.next();
                  notes.insert(i.key() + 5*PITCH_DELTA_OCTAVE, i.value());

                  i.next();
                  notes.insert(i.key() + 5*PITCH_DELTA_OCTAVE, i.value());
                  }
                  break;
            case Voicing::FOUR_NOTE:
                  {
                  //four note drop 2
                  QMap<int, int> relIntervals = getIntervals(rootTpc, literal);
                  QMap<int, int> intervals = normalizeNoteMap(relIntervals, rootTpc, rootPitch, 3, true);
                  QMapIterator<int, int> i(intervals);
                  i.toBack();

                  int counter = 0; //how many notes have been added
                  while (i.hasPrevious()) {
                        i.previous();

                        if (counter % 2)
                              notes.insert(i.key() + 4*PITCH_DELTA_OCTAVE, i.value());
                        else
                              notes.insert(i.key() + 5*PITCH_DELTA_OCTAVE, i.value());
                        ++counter;
                        }
                  }
                  break;
            case Voicing::SIX_NOTE:
                  //six note voicing
                  {
                  QMap<int, int> relIntervals = getIntervals(rootTpc, literal);
                  QMap<int, int> intervals = normalizeNoteMap(relIntervals, rootTpc, rootPitch, 5, true);
                  QMapIterator<int, int> i(intervals);
                  i.toBack();

                  int counter = 0; //how many notes have been added
                  while (i.hasPrevious()) {
                        i.previous();

                        if (counter % 2)
                              notes.insert(i.key() + 4*PITCH_DELTA_OCTAVE, i.value());
                        else
                              notes.insert(i.key() + 5*PITCH_DELTA_OCTAVE, i.value());
                        ++counter;
                        }
                  }
                  break;
            default:
                  break;
            }
      return notes;
      }

//---------------------------------------------------
//   update
///   updates the current note map, this is where all
///   of the rhythm and voicing choices matter since
///   the voicing algorithms depend on this.
///
///   transposeOffset -- is the necessary adjustment
///   that is added to the root and bass
///   to get the correct sounding pitch
//---------------------------------------------------
void RealizedHarmony::update(int rootTpc, int bassTpc, int transposeOffset /*= 0*/)
      {
      //TODO - PHV: consider the design of this
      //on transposition the dirty flag is set by the harmony, but it's a little
      //bit risky design since these 3 parameters rely on the dirty bit and are not
      //otherwise checked by RealizedHarmony. This saves us 3 ints of space, but
      //has the added risk
      if (!_dirty)
            return;

      _notes = generateNotes(rootTpc, bassTpc, _literal, _voicing, transposeOffset);
      _dirty = false;
      }

//---------------------------------------------------
//   getIntervals
///   gets a map from intervals to TPCs based on
///   a passed root tpc (this allows for us to
///   keep pitches, but transpose notes on the score)
///
///   Weighting System:
///   - Rank 0: 3rd, altered 5th, suspensions (characteristic notes)
///   - Rank 1: 7th
///   - Rank 2: 9ths
///   - Rank 3: 13ths where applicable and (in minor chords) 11ths
///   - Rank 3: Other alterations and additions
///   - Rank 4: 5th and (in major/dominant chords) 11th
//---------------------------------------------------
QMap<int, int> RealizedHarmony::getIntervals(int rootTpc, bool literal) const
      {
      //RANKING SYSTEM
      static const int RANK_MULT = 128;
      static const int RANK_3RD = 0;
      static const int RANK_7TH = 1;
      static const int RANK_9TH = 2;
      static const int RANK_ADD = 3;
      static const int RANK_OMIT = 4;

      static const int FIFTH = 7 + RANK_MULT*RANK_OMIT;

      QMap<int, int> ret;

      const ParsedChord* p = _harmony->parsedForm();
      QString quality = p->quality();
      int ext = p->extension().toInt();
      const QStringList& modList = p->modifierList();

      int omit = 0; //omit flags for which notes to omit (for notes that are altered
                    //or specified to be omitted as a modification) so that they
                    //are not later added
      bool alt5 = false; //altered 5

      //handle modifiers
      for (QString s : modList) {
            //find number, split up mods
            bool modded = false;
            for (int c = 0; c < s.length(); ++c) {
                  if (s[c].isDigit()) {
                        Q_ASSERT(c > 0); //we shouldn't have just a number

                        int alter = 0;
                        int cutoff = c;
                        int deg = s.right(s.length() - c).toInt();
                        //account for if the flat/sharp is stuck to the end of add
                        if (s[c-1] == '#') {
                              cutoff -= 1;
                              alter = +1;
                              }
                        else if (s[c-1] == 'b') {
                              cutoff -= 1;
                              alter = -1;
                              }
                        QString extType = s.left(cutoff);
                        if (extType == "" || extType == "major") { //alteration
                              if (deg == 9)
                                    ret.insert(step2pitchInterval(deg, alter) + RANK_MULT*RANK_9TH, tpcInterval(rootTpc, deg, alter));
                              else
                                    ret.insert(step2pitchInterval(deg, alter) + RANK_MULT*RANK_ADD, tpcInterval(rootTpc, deg, alter));
                              if (deg == 5)
                                    alt5 = true;
                              omit |= 1 << deg;
                              modded = true;
                              }
                        else if (extType == "sus") {
                              ret.insert(step2pitchInterval(deg, alter) + RANK_MULT*RANK_3RD, tpcInterval(rootTpc, deg, alter));
                              omit |= 1 << 3;
                              modded = true;
                              }
                        else if (extType == "no") {
                              omit |= 1 << deg;
                              modded = true;
                              }
                        }
                  }

            //check for special chords, not modded but no number means we haven't found
            if (!modded) {
                  if (s == "phryg") {
                        ret.insert(step2pitchInterval(9, -1) + RANK_MULT*RANK_9TH, tpcInterval(rootTpc, 9, -1));
                        omit |= 1 << 9;
                        }
                  else if (s == "lyd") {
                        ret.insert(step2pitchInterval(11, +1) + RANK_MULT*RANK_ADD, tpcInterval(rootTpc, 11, +1));
                        omit |= 1 << 11;
                        }
                  else if (s == "blues") {
                        ret.insert(step2pitchInterval(9, +1) + RANK_MULT*RANK_ADD, tpcInterval(rootTpc, 9, +1));
                        omit |= 1 << 9;
                        }
                  else if (s == "alt") {
                        ret.insert(step2pitchInterval(5, -1) + RANK_MULT*RANK_ADD, tpcInterval(rootTpc, 5, -1));
                        ret.insert(step2pitchInterval(5, +1) + RANK_MULT*RANK_ADD, tpcInterval(rootTpc, 5, +1));
                        omit |= 1 << 5;
                        ret.insert(step2pitchInterval(9, -1) + RANK_MULT*RANK_9TH, tpcInterval(rootTpc, 9, -1));
                        ret.insert(step2pitchInterval(9, +1) + RANK_MULT*RANK_9TH, tpcInterval(rootTpc, 9, +1));
                        omit |= 1 << 9;
                        }
                  else  //no easy way to realize tristan chords since they are more analytical tools
                        omit = ~0;
                  }
            }

      //handle ext = 5: power chord so omit the 3rd
      if (ext == 5)
            omit |= 1 << 3;

      //handle chord quality
      if (quality == "minor") {
            if (!(omit & (1 << 3)))
                  ret.insert(step2pitchInterval(3, -1) + RANK_MULT*RANK_3RD, tpcInterval(rootTpc, 3, -1));     //min3
            if (!(omit & (1 << 5)))
                  ret.insert(step2pitchInterval(5, 0) + RANK_MULT*RANK_OMIT, tpcInterval(rootTpc, 5, 0));       //p5
            }
      else if (quality == "augmented") {
            if (!(omit & (1 << 3)))
                  ret.insert(step2pitchInterval(3, 0) + RANK_MULT*RANK_3RD, tpcInterval(rootTpc, 3, 0));      //maj3
            if (!(omit & (1 << 5)))
                  ret.insert(step2pitchInterval(5, +1) + RANK_MULT*RANK_3RD, tpcInterval(rootTpc, 5, +1));    //p5
            }
      else if (quality == "diminished" || quality == "half-diminished") {
            if (!(omit & (1 << 3)))
                  ret.insert(step2pitchInterval(3, -1) + RANK_MULT*RANK_3RD, tpcInterval(rootTpc, 3, -1));     //min3
            if (!(omit & (1 << 5)))
                  ret.insert(step2pitchInterval(5, -1) + RANK_MULT*RANK_3RD, tpcInterval(rootTpc, 5, -1));     //dim5
            alt5 = true;
            }
      else { //major or dominant
            if (!(omit & (1 << 3)))
                  ret.insert(step2pitchInterval(3, 0) + RANK_MULT*RANK_3RD, tpcInterval(rootTpc, 3, 0));      //maj3
            if (!(omit & (1 << 5)))
                  ret.insert(step2pitchInterval(5, 0) + RANK_MULT*RANK_OMIT, tpcInterval(rootTpc, 5, 0));      //p5
            }

      //handle extension
      switch (ext) {
            case 13:
                  if (!(omit & (1 << 13))) {
                        ret.insert(9 + RANK_MULT*RANK_ADD, tpcInterval(rootTpc, 13, 0));     //maj13
                        omit |= 1 << 13;
                        }
                  // FALLTHROUGH
            case 11:
                  if (!(omit & (1 << 11))) {
                        if (quality == "minor")
                              ret.insert(5 + RANK_MULT*RANK_ADD, tpcInterval(rootTpc, 11, 0));     //maj11
                        else if (literal)
                              ret.insert(5 + RANK_MULT*RANK_OMIT, tpcInterval(rootTpc, 11, 0));     //maj11
                        omit |= 1 << 11;
                        }
                  // FALLTHROUGH
            case 9:
                  if (!(omit & (1 << 9))) {
                        ret.insert(2 + RANK_MULT*RANK_9TH, tpcInterval(rootTpc, 9, 0));     //maj9
                        omit |= 1 << 9;
                        }
                  // FALLTHROUGH
            case 7:
                  if (!(omit & (1 << 7))) {
                        if (quality == "major")
                              ret.insert(11 + RANK_MULT*RANK_7TH, tpcInterval(rootTpc, 7, 0));
                        else if (quality == "diminished")
                              ret.insert(9 + RANK_MULT*RANK_7TH, tpcInterval(rootTpc, 7, -2));
                        else //dominant or augmented or minor
                              ret.insert(10 + RANK_MULT*RANK_7TH, tpcInterval(rootTpc, 7, -1));
                        }
                  break;
            case 6:
                  if (!(omit & (1 << 6))) {
                        ret.insert(9 + RANK_MULT*RANK_ADD, tpcInterval(rootTpc, 6, 0));     //maj6
                        omit |= 1 << 13; //no need to add/alter 6 chords
                        }
                  break;
            case 5:
                  //omitted third already
                  break;
            case 4:
                  if (!(omit & (1 << 4)))
                        ret.insert(5 + RANK_MULT*RANK_ADD, tpcInterval(rootTpc, 4, 0));     //p4
                  break;
            case 2:
                  if (!(omit & (1 << 2)))
                        ret.insert(2 + RANK_MULT*RANK_ADD, tpcInterval(rootTpc, 2, 0));     //maj2
                  omit |= 1 << 9; //make sure we don't add altered 9 when theres a 2
                  break;
            case 69:
                  ret.insert(9 + RANK_MULT*RANK_ADD, tpcInterval(rootTpc, 6, 0));     //maj6
                  ret.insert(2 + RANK_MULT*RANK_ADD, tpcInterval(rootTpc, 9, 0));     //maj6
                  omit = ~0; //no additions/alterations for a 69 chord
                  break;
            default:
                  break;
            }

      Harmony* next = _harmony->findNext();
      if (!_literal && next) {
            //jazz interpretation
            QString qNext = next->parsedForm()->quality();
            //pitch from current to next harmony normalized to a range between 0 and 12
            //add PITCH_DELTA_OCTAVE before modulo so that we can ensure arithmetic mod rather than computer mod
            int keyTpc = int(next->staff()->key(next->tick())) + 14; //tpc of key (ex. F# major would be Tpc::F_S)
            int pitchBetween = (tpc2pitch(next->rootTpc()) + PITCH_DELTA_OCTAVE - tpc2pitch(rootTpc)) % PITCH_DELTA_OCTAVE;

            //dont add 9 for chords with altered 5s
            if (!(omit & (1 << 9)) && !alt5) {
                  if (quality == "dominant" && pitchBetween == 5 && qNext == "minor"
                      /*((next->rootTpc() == keyTpc && qNext == "major") || (next->rootTpc() == keyTpcMinor && qNext == "minor"))*/) {
                        //flat 9 for dominant to a tonic chord a P4 up
                        //only for minor chords for now
                        ret.insert(1 + RANK_MULT*RANK_9TH, tpcInterval(rootTpc, 9, -1));
                        }
                  else //add major 9
                        ret.insert(2 + RANK_MULT*RANK_9TH, tpcInterval(rootTpc, 9, 0));
                  }

            if (!(omit & (1 << 13)) && !alt5) {
                  if (quality == "dominant" && pitchBetween == 5 && qNext == "minor") {
                        //flat 13 for dominant to chord a P4 up
                        //only for minor chords for now
                        ret.remove(FIFTH);
                        ret.insert(8 + RANK_MULT*RANK_ADD, tpcInterval(rootTpc, 13, -1));
                        }
                  //major 13 considered, but too dependent on melody and voicing of other chord
                  //no implementation for now
                  }
            }
      return ret;
      }

//---------------------------------------------------
//   normalizeNoteMap
///   normalize the note map from intervals to create pitches between 0 and 12
///   and resolve any weighting system.
///
///   enforceMaxEquals - enforce the max as a goal so that the max is how many notes is inserted
//---------------------------------------------------
QMap<int, int> RealizedHarmony::normalizeNoteMap(const QMap<int, int>& intervals, int rootTpc, int rootPitch, int max, bool enforceMaxAsGoal) const
      {
      QMap<int, int> ret;
      QMapIterator<int, int> itr(intervals);

      for (int i = 0; i < max; ++i) {
            if (!itr.hasNext())
                  break;
            itr.next();
            ret.insert((itr.key() % 128 + rootPitch) % PITCH_DELTA_OCTAVE, itr.value());
            }

      //redo insertions if we must have a specific number of notes with insertMulti
      if (enforceMaxAsGoal) {
            while (ret.size() < max) {
                  ret.insertMulti(rootPitch, rootTpc); //duplicate root

                  int size = max - ret.size();
                  itr = QMapIterator<int, int>(intervals); //reset iterator
                  for (int i = 0; i < size; ++i) {
                        if (!itr.hasNext())
                              break;
                        itr.next();
                        ret.insertMulti((itr.key() % 128 + rootPitch) % PITCH_DELTA_OCTAVE, itr.value());
                        }
                  }
            }
      else if (ret.size() < max) //insert another root if we have room in general
            ret.insertMulti(rootPitch, rootTpc);
      return ret;
      }

//---------------------------------------------------
//   cascadeDirty
///   cascades the dirty flag backwards so that everything is properly set
///   this is required since voicing algorithms may look ahead
///
///   NOTE: FOR NOW ALGORITHMS DO NOT LOOK BACKWARDS AND SO THERE IS NO
///   REQUIREMENT TO CASCADE FORWARD, IN THE FUTURE IT MAY BECOME IMPORTANT
///   TO CASCADE FORWARD AS WELL
//---------------------------------------------------
void RealizedHarmony::cascadeDirty(bool dirty)
      {
      if (dirty && !_dirty) { //only cascade when we want to set our clean realized harmony to dirty
            Harmony* prev = _harmony->findPrev();
            if (prev)
                  prev->realizedHarmony().cascadeDirty(dirty);
            }
      _dirty = dirty;
      }
}

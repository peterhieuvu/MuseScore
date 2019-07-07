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

#include "voicingSelect.h"

namespace Ms {

VoicingSelect::VoicingSelect(QWidget* parent)
   : QWidget(parent)
      {
      setupUi(this);

      //setup changed signals
      connect(interpretBox, SIGNAL(valueChanged(int)), SLOT(_voicingChanged()));
      connect(voicingBox, SIGNAL(valueChanged(int)), SLOT(_voicingChanged()));
      }

void VoicingSelect::_voicingChanged()
      {
      emit voicingChanged(interpretBox->currentIndex(), voicingBox->currentIndex());
      }

void VoicingSelect::blockVoicingSignals(bool val)
      {
      interpretBox->blockSignals(val);
      voicingBox->blockSignals(val);
      }

void VoicingSelect::setVoicing(int idx)
      {
      blockVoicingSignals(true);
      voicingBox->setCurrentIndex(idx);
      blockVoicingSignals(false);
      }

void VoicingSelect::setLiteral(bool literal) //TODO - PHV: literal is bool for now
      {
      blockVoicingSignals(true);
      interpretBox->setCurrentIndex(literal);
      blockVoicingSignals(false);
      }

}

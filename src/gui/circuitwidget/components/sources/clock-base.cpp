/***************************************************************************
 *   Copyright (C) 2010 by santiago González                               *
 *   santigoro@gmail.com                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 *                                                                         *
 ***************************************************************************/

#include "clock-base.h"
#include "pin.h"
#include "simulator.h"

static const char* ClockBase_properties[] = {
    QT_TRANSLATE_NOOP("App::Property","Freq"),
    QT_TRANSLATE_NOOP("App::Property","Always On")
};

ClockBase::ClockBase( QObject* parent, QString type, QString id )
         : LogicInput( parent, type, id )
{
    Q_UNUSED( ClockBase_properties );

    m_graphical = true;
    
    m_area = QRect( -14, -8, 22, 16 );

    m_stepsPC = 0;
    m_step = 0;
    m_alwaysOn = false;
    setFreq( 1000 );

    Simulator::self()->addToUpdateList( this );

    connect( Simulator::self(), &Simulator::rateChanged,
             this,              &ClockBase::rateChanged );
}
ClockBase::~ClockBase(){}

void ClockBase::stamp()
{
    m_step = 0;
    m_vOut = 0;
    m_lastVout = 0;
    setFreq( m_freq );
}

void ClockBase::updateStep()
{
    if( m_changed )
    {
        if( m_out->out() ) Simulator::self()->addToSimuClockList( this );
        else               Simulator::self()->remFromSimuClockList( this );

        m_changed = false;
    }
}

void ClockBase::setAlwaysOn( bool on )
{
    m_alwaysOn = on;
    if( on && !m_out->out() ) setOut( on );
    m_button->setVisible( !on );
    update();
}

double ClockBase::freq() { return m_freq; }

void ClockBase::setFreq( double freq )
{
    double stepsPerS = 1e9; //Simulator::self()->stepsPerus()*1e6;

    m_stepsPC = stepsPerS/freq;

    m_freq = freq;
    
    emit freqChanged();
}

void ClockBase::rateChanged()
{
    setFreq( m_freq );
}

void ClockBase::remove()
{
    Simulator::self()->remFromSimuClockList( this );

    LogicInput::remove();
}

#include "moc_clock-base.cpp"


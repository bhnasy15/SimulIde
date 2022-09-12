/***************************************************************************
 *   Copyright (C) 2017 by santiago González                               *
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

#include "clock.h"
#include "pin.h"
#include "simulator.h"

Component* Clock::construct( QObject* parent, QString type, QString id )
{
    return new Clock( parent, type, id );
}

LibraryItem* Clock::libraryItem()
{
    return new LibraryItem(
        tr( "Clock" ),
        tr( "Sources" ),
        "clock.png",
        "Clock",
        Clock::construct );
}

Clock::Clock( QObject* parent, QString type, QString id )
     : ClockBase( parent, type, id )
{
}
Clock::~Clock(){}

void Clock::simuClockStep()
{
    m_step += Simulator::self()->m_stepNano;

    if ( m_step >= m_stepsPC )
    {
        m_step -= m_stepsPC;
        m_vOut = m_voltHight;
    }
    else if( m_step >= m_halfW ) m_vOut = 0;

    if( m_vOut == m_lastVout ) return;
    m_lastVout = m_vOut;

    m_out->setVoltHigh( m_vOut );
    m_out->stampOutput();
}

void Clock::setFreq( double freq )
{
    ClockBase::setFreq( freq );
    m_halfW = m_stepsPC/2;
}

void Clock::paint( QPainter* p, const QStyleOptionGraphicsItem* option, QWidget* widget )
{
    if( m_hidden ) return;

    LogicInput::paint( p, option, widget );

    QPen pen;
    pen.setWidth(1);
    p->setPen( pen );
    
    p->drawLine(-11, 3,-11,-3 );
    p->drawLine(-11,-3,-5, -3 );
    p->drawLine( -5,-3,-5,  3 );
    p->drawLine( -5, 3, 1,  3 );
    p->drawLine(  1, 3, 1, -3 );
    p->drawLine(  1,-3, 4, -3 );
}

#include "moc_clock.cpp"


/***************************************************************************
 *   Copyright (C) 2020 by santiago González                               *
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

#include "termistor.h"
#include "itemlibrary.h"

static const char* Termistor_properties[] = {
    QT_TRANSLATE_NOOP("App::Property","B"),
    QT_TRANSLATE_NOOP("App::Property","R25")
};

Component* Termistor::construct( QObject* parent, QString type, QString id )
{ return new Termistor( parent, type, id ); }

LibraryItem* Termistor::libraryItem()
{
    return new LibraryItem(
            tr( "Termistor" ),
            tr( "Resistive Sensors" ),
            "termistor.png",
            "Termistor",
            Termistor::construct);
}

Termistor::Termistor( QObject* parent, QString type, QString id )
         : ThermistorBase( parent, type, id  )
{
    Q_UNUSED( Termistor_properties );

    m_bVal = 3455;
    m_r25  = 10000;

    Component::setValue( 25 );
    eResistor::setResSafe( m_r25 );
}
Termistor::~Termistor(){}


void Termistor::updateStep()
{
    if( !m_changed ) return;

    double t0 = 25+273.15;              // Temp in Kelvin
    double t = m_value*m_unitMult+273.15;
    double e = 2.7182;
    double k = t*t0/(t-t0);

    double res = m_r25/pow( e, m_bVal/k );

    eResistor::setResSafe( res );
    m_changed = false;
}

void Termistor::setBval( int bval )
{
    m_bVal = bval;
    m_changed = true;
}

void Termistor::setR25( int r25 )
{
    m_r25 = r25;
    m_changed = true;
}

void Termistor::paint( QPainter* p, const QStyleOptionGraphicsItem* option, QWidget* widget )
{
    if( m_hidden ) return;

    Component::paint( p, option, widget );

    p->drawRect( -10.5, -4, 21, 8 );
    
    p->drawLine(-8, 6,  6,-8 );
    p->drawLine( 6,-8, 10,-8 );
}

//#include "moc_ldr.cpp"

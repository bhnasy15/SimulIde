/***************************************************************************
 *   Copyright (C) 2012 by santiago González                               *
 *   santigoro@gmail.com                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
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

//#include <QDebug>
//#include <math.h>

#include "e-diode.h"
#include "e-node.h"
#include "simulator.h"

eDiode::eDiode( std::string id ) 
      : eResistor( id )
{
    m_imped = 0.6;
    m_threshold = 0.7;
    m_zenerV = 0;
}
eDiode::~eDiode(){}

void eDiode::stamp()
{
    eNode* node = m_ePin[0]->getEnode();
    if( node ) node->addToNoLinList( this );

    node = m_ePin[1]->getEnode();
    if( node ) node->addToNoLinList( this );
    eResistor::stamp();
}

void eDiode::resetState()
{
    m_admit = cero_doub;
    m_voltPN  = 0;
    m_current = 0;
    m_lastThCurrent = 0;
}

void eDiode::setVChanged()
{
    m_voltPN = m_ePin[0]->getVolt()-m_ePin[1]->getVolt();

    double ThCurrent = m_current = 0;
    double admit = cero_doub;

    double deltaV = m_voltPN-m_threshold;
    if( deltaV > -1e-12 )   // Conducing
    {
        admit = 1/m_imped;
        ThCurrent = m_threshold*admit;
    }
    else if( (m_zenerV > 0)&&(m_voltPN <-m_zenerV) )
    {
        admit = 1/m_imped;
        ThCurrent = -m_zenerV*admit;
    }
    if( admit != m_admit ) eResistor::setAdmit( admit );

    if( ThCurrent == m_lastThCurrent ) return;
    m_lastThCurrent = ThCurrent;

    m_ePin[0]->stampCurrent( ThCurrent );
    m_ePin[1]->stampCurrent(-ThCurrent );
}

void eDiode::setThreshold( double threshold )
{
    m_threshold = threshold;
}

double eDiode::res()
{
    return m_imped;
}

void eDiode::setRes( double resist )
{
    bool pauseSim = Simulator::self()->isRunning();
    if( pauseSim ) Simulator::self()->pauseSim();

    if( resist == 0 ) resist = 0.1;
    m_imped = resist;

    if( pauseSim )
    {
        setVChanged();
        Simulator::self()->resumeSim();
    }
}

void  eDiode::setZenerV( double zenerV ) 
{ 
    if( zenerV > 0 ) m_zenerV = zenerV; 
    else             m_zenerV = 0;
    setResSafe( m_imped );
}

void eDiode::updateVI()
{
    m_current = 0;
    
    if( m_admit == cero_doub ) return;

    if( m_ePin[0]->isConnected() && m_ePin[1]->isConnected() )
    {
        double volt = m_voltPN - m_threshold;
        if( volt>0 ) m_current = volt*m_admit;
        //qDebug() << "m_voltPN"<<m_voltPN<<"m_deltaV"<<m_deltaV<<"volt"<<volt;
    }
}


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

#ifndef TERMISTOR_H
#define TERMISTOR_H

#include "thermistorbase.h"

class LibraryItem;

class MAINMODULE_EXPORT Termistor : public ThermistorBase
{
    Q_OBJECT
    Q_PROPERTY( int    B     READ bVal    WRITE setBval    DESIGNABLE true USER true )
    Q_PROPERTY( int    R25   READ r25     WRITE setR25     DESIGNABLE true USER true )

    public:
        Termistor( QObject* parent, QString type, QString id );
        ~Termistor();

        static Component* construct( QObject* parent, QString type, QString id );
        static LibraryItem* libraryItem();

        int bVal() { return m_bVal; }
        void setBval( int bval );

        int r25() { return m_r25; }
        void setR25( int r25 );

        virtual void updateStep();

        virtual void paint( QPainter* p, const QStyleOptionGraphicsItem* option, QWidget *widget );

    private:
        bool m_ptc;

        int m_bVal;
        int m_r25;
};

#endif

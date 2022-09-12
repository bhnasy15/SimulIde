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

#ifndef PLOTBASE_H
#define PLOTBASE_H

#include "component.h"
#include "e-element.h"

class PlotDisplay;
class DataChannel;

class MAINMODULE_EXPORT PlotBase : public Component, public eElement
{
    Q_OBJECT
    Q_PROPERTY( int Basic_X   READ baSizeX  WRITE setBaSizeX  DESIGNABLE true USER true )
    Q_PROPERTY( int Basic_Y   READ baSizeY  WRITE setBaSizeY  DESIGNABLE true USER true )
    Q_PROPERTY( QStringList Tunnels  READ tunnels  WRITE setTunnels )
    Q_PROPERTY( quint64 hTick  READ timeDiv  WRITE setTimeDiv )

    public:
        PlotBase( QObject* parent, QString type, QString id );
        ~PlotBase();

        int baSizeX() { return m_baSizeX; }
        void setBaSizeX( int size );

        int baSizeY() { return m_baSizeY; }
        void setBaSizeY( int size );

        uint64_t timeDiv() { return m_timeDiv; }
        virtual void setTimeDiv( uint64_t td );

        virtual QStringList tunnels()=0;
        virtual void setTunnels( QStringList tunnels )=0;

        virtual void expand( bool e )=0;
        void toggleExpand();

        virtual void channelChanged( int ch, QString name ){;}

        PlotDisplay* display() { return m_display; }

        QColor getColor( int c ) { return m_color[c]; }

        virtual void paint( QPainter* p, const QStyleOptionGraphicsItem* option, QWidget* widget );

    protected:
        int m_bufferSize;

        bool m_expand;

        int m_screenSizeX;
        int m_screenSizeY;
        int m_baSizeX;
        int m_baSizeY;

        double m_dataSize;

        uint64_t m_timeDiv;

        QColor m_color[5];

        PlotDisplay* m_display;

        QGraphicsProxyWidget* m_proxy;
};

#endif



#
/*
 *      SDRunoPlugin_1090 is based on and contains source code from dump1090
 *      Copyright (C) 2012 by Salvatore Sanfilippo <antirez@gmail.com>
 *      all rights acknowledged.
 *
 *	Copyright (C) 2018
 *	Jan van Katwijk (J.vanKatwijk@gmail.com)
 *	Lazy Chair Computing
 *
 *	This file is part of the SDRunoPlugin_1090
 *
 *    SDRunoPlugin_1090 is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    SDRunoPlugin_1090 is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SDRunoPlugin_1090; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef	__HTTP_HANDLER_H
#define	__HTTP_HANDLER_H

#include	<QString>
#include	<thread>
#include	<atomic>
#include	<string>
#include	<complex>
class	qt1090;

class	httpHandler {
public:
	httpHandler	(qt1090 *, std::complex<float>,
	                 const QString &, bool autoBrowser);
	~httpHandler	();
void	start		();
void	stop		();
void	run		();

private:
	qt1090		*parent;
	std::complex<float>	homeAddress;

#ifdef  __MINGW32__
        std::wstring    browserAddress;
#else
        std::string     browserAddress;
#endif

	QString		mapPort;
	bool		autoBrowser;
	std::atomic<bool>	running;
	std::thread	threadHandle;
	std::string     theMap	(std::complex<float>);
//	std::string     theMap	(const QString &fileName);
};

#endif

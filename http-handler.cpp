#
/*
 *      The http handling in SDRunoPlugin_1090 is
 *	based on and contains source code from dump1090
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

#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<sys/types.h> 
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<netdb.h>
#include	<arpa/inet.h>
#include	<err.h>
#include	<cstring>

#include	"http-handler.h"
#include	"qt-1090.h"
#include	"aircraft-handler.h"

	httpHandler::httpHandler (qt1090 *parent, std::string mapFile) {
	this	-> parent	= parent;
	this	-> mapFile	= mapFile;
	this	-> running. store (false);
}

	httpHandler::~httpHandler	() {
	if (running. load ()) {
	   running. store (false);
	   threadHandle. join ();
	}
}

void	httpHandler::start	() {
	threadHandle = std::thread (&httpHandler::run, this);
}

void	httpHandler::stop	() {
	if (running. load ()) {
           running. store (false);
           threadHandle. join ();
        }
}

void	httpHandler::run	() {
char	buffer [4096];
bool	keepalive;
char	*url;
int one = 1, client_fd;
struct sockaddr_in svr_addr, cli_addr;
std::string	content;
std::string	ctype;

	running. store (true);
	socklen_t sin_len = sizeof (cli_addr);
	int sock = socket (AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
	   running. store (false);
	   return;
	}

	setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));
	int port = 8080;
	svr_addr.sin_family = AF_INET;
	svr_addr.sin_addr.s_addr = INADDR_ANY;
	svr_addr.sin_port = htons(port);
 
	if (bind(sock, (struct sockaddr *) &svr_addr,
	                                  sizeof(svr_addr)) == -1) {
	   close (sock);
	   running. store (false);
	   return;
	}
 
	listen (sock, 5);
	while (running. load ()) {
	   client_fd = accept (sock,
	                    (struct sockaddr *) &cli_addr, &sin_len);
	   if (client_fd == -1) 
	      continue;

	   while (running) {
	      if (read (client_fd, buffer, 4096) < 0)
	         break;
	      fprintf (stderr, "%s\n", buffer);
	      int httpver = (strstr (buffer, "HTTP/1.1") != NULL) ? 11 : 10;
	      if (httpver == 11) 
//	HTTP 1.1 defaults to keep-alive, unless close is specified. 
	         keepalive = strstr (buffer, "Connection: close") == NULL;
	      else // httpver == 10
	         keepalive = strstr (buffer, "Connection: keep-alive") != NULL;

	/* Identify he URL. */
	      char *p = strchr (buffer,' ');
	      if (p == NULL) 
	         break;
	      url = ++p; // Now this should point to the requested URL. 
	      p = strchr (p, ' ');
	      if (p == NULL)
	         break;
	      *p = '\0';

//	Select the content to send, we have just two so far:
//	 "/" -> Our google map application.
//	 "/data.json" -> Our ajax request to update planes. */
	   bool jsonUpdate	= false;
	      if (strstr (url, "/data.json")) {
	         QString xx	= aircraftsToJson (parent -> planeList);
	         content	= std::string (xx. toUtf8 (). data ());
                 ctype		= "application/json;charset=utf-8";
	         jsonUpdate	= true;
	      }
	      else {
	         content	= theMap (mapFile);
	         ctype		= "text/html;charset=utf-8";
	      }

//	Create the header 
	      char hdr [2048];
	      sprintf (hdr,
	               "HTTP/1.1 200 OK\r\n"
	               "Server: dump1090\r\n"
	               "Content-Type: %s\r\n"
	               "Connection: %s\r\n"
	               "Content-Length: %d\r\n"
//	               "Access-Control-Allow-Origin: *\r\n"
	               "\r\n",
	               ctype. c_str (),
	               keepalive ? "keep-alive" : "close",
	               (int)(strlen (content. c_str ())));
	      int hdrlen = strlen (hdr);
	      fprintf (stderr, "%s \n", hdr);
	      if (jsonUpdate) {
	         fprintf (stderr, "Json update requested\n");
	         fprintf (stderr, "%s\n", content. c_str ());
	      }
//	and send the reply
	      if (write (client_fd, hdr, hdrlen) != hdrlen ||
	          write (client_fd, content. c_str (),
	                          content. size ()) != content. size ())  {
	         fprintf (stderr, "WRITE PROBLEM\n");
	         break;
	      }
	   }
	}
}
 
std::string	httpHandler::theMap (std::string fileName) {
FILE	*fd;
std::string res;
int	bodySize;
char	*body;
	fd	=  fopen (fileName. c_str (), "r");
	fseek (fd, 0L, SEEK_END);
	bodySize	= ftell (fd);
	fseek (fd, 0L, SEEK_SET);
        body =  (char *)malloc (bodySize);
        fread (body, 1, bodySize, fd);
        fclose (fd);
	res	= std::string (body);
	free (body);
	return res;
}


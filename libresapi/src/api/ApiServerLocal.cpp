/*
 * libresapi local socket server
 * Copyright (C) 2016  Gioacchino Mazzurco <gio@eigenlab.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ApiServerLocal.h"
#include "JsonStream.h"

namespace resource_api{

ApiServerLocal::ApiServerLocal(ApiServer* server, QObject *parent) :
    QObject(parent), serverThread(this),
    localListener(server) // Must have no parent to be movable to other thread
{
	localListener.moveToThread(&serverThread);
	serverThread.start();
}

ApiServerLocal::~ApiServerLocal() { serverThread.quit(); }

ApiLocalListener::ApiLocalListener(ApiServer *server, QObject *parent) :
    QObject(parent), mApiServer(server), mLocalServer(this)
{
	mLocalServer.removeServer(serverName());
#if QT_VERSION >= 0x050000
	mLocalServer.setSocketOptions(QLocalServer::UserAccessOption);
#endif
	connect(&mLocalServer, SIGNAL(newConnection()), this, SLOT(handleConnection()));
	mLocalServer.listen(serverName());
}

void ApiLocalListener::handleConnection()
{
	new ApiLocalConnectionHandler(mApiServer,
	                              mLocalServer.nextPendingConnection(), this);
}

ApiLocalConnectionHandler::ApiLocalConnectionHandler(
        ApiServer* apiServer, QLocalSocket* sock, QObject *parent) :
    QObject(parent), mApiServer(apiServer), mLocalSocket(sock),
    mState(WAITING_PATH)
{
	connect(mLocalSocket, SIGNAL(disconnected()), this, SLOT(deleteLater()));
	connect(sock, SIGNAL(readyRead()), this, SLOT(handlePendingRequests()));
}

ApiLocalConnectionHandler::~ApiLocalConnectionHandler()
{
	mLocalSocket->close();
	delete mLocalSocket;
}

void ApiLocalConnectionHandler::handlePendingRequests()
{
	switch(mState)
	{
	case WAITING_PATH:
	{
		if(mLocalSocket->canReadLine())
		{
            readPath:
			reqPath = mLocalSocket->readLine().constData();
			mState = WAITING_DATA;

			/* Because QLocalSocket is SOCK_STREAM some clients implementations
			 * like the one based on QLocalSocket feel free to send the whole
			 * request (PATH + DATA) in a single write(), causing readyRead()
			 * signal being emitted only once, in that case we should continue
			 * processing without waiting for readyRead() being fired again, so
			 * we don't break here as there may be more lines to read */
		}
	}
	case WAITING_DATA:
	{
		if(mLocalSocket->canReadLine())
		{
			resource_api::JsonStream reqJson;
			reqJson.setJsonString(std::string(mLocalSocket->readLine().constData()));
			resource_api::Request req(reqJson);
			req.setPath(reqPath);
			std::string resultString = mApiServer->handleRequest(req);
			mLocalSocket->write(resultString.c_str(), resultString.length());
			mLocalSocket->write("\n\0");
			mState = WAITING_PATH;

			/* Because QLocalSocket is SOCK_STREAM some clients implementations
			 * like the one based on QLocalSocket feel free to coalesce multiple
			 * upper level write() into a single socket write(), causing
			 * readyRead() signal being emitted only once, in that case we should
			 * keep processing without waiting for readyRead() being fired again */
			if(mLocalSocket->canReadLine()) goto readPath;

			// Now there are no more requests to process we can break
			break;
		}
	}
	}
}

} // namespace resource_api

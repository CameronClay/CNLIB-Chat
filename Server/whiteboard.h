#ifndef WHITEBOARD_H
#define WHITEBOARD_H

#include <QtWidgets>
#include <unordered_map>
#include <vector>
#include <mutex>
#include "CNLIB/TCPServInterface.h"
#include "whiteboardargs.h"

struct WBClientData {

};

class Whiteboard
{
public:
    Whiteboard(const QString& creator, ClientData* creatorClient, const WhiteboardArgs& wbArgs);

    void AddClient(ClientData* pc);
    void RemoveClient(ClientData* pc);
    std::vector<ClientData*> GetClients();

    bool IsCreator(const QString& user) const;
    ClientData* GetCreatorClient() const;
    const WhiteboardArgs& GetArgs() const;

private:
    QString creator;
    ClientData* creatorClient;
    std::unordered_map<ClientData*, WBClientData> clients;
    std::mutex clientsMut;
    WhiteboardArgs wbArgs;
};

#endif // WHITEBOARD_H

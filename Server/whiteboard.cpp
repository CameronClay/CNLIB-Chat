#include "whiteboard.h"
#include <algorithm>

Whiteboard::Whiteboard(const QString& creator, ClientData* creatorClient, const WhiteboardArgs& wbArgs)
:
    creator(creator),
    creatorClient(creatorClient),
    wbArgs(wbArgs)
{

}

void Whiteboard::AddClient(ClientData* pc) {
    std::lock_guard lck_guard{clientsMut};
    clients.emplace(pc , WBClientData{});
}
void Whiteboard::RemoveClient(ClientData* pc) {
    std::lock_guard lck_guard{clientsMut};
    auto it = clients.find(pc);
    if(it != std::end(clients)) {
        clients.erase(it);
    }
}
std::vector<ClientData*> Whiteboard::GetClients() {
    std::lock_guard lck_guard{clientsMut};
    std::vector<ClientData*> clientsVec;
    std::transform(std::cbegin(clients), std::cend(clients), std::back_inserter(clientsVec), [](const auto& it) { return it.first; });
    return clientsVec;
}


bool Whiteboard::IsCreator(const QString& user) const {
    return creator == user;
}

ClientData* Whiteboard::GetCreatorClient() const {
    return creatorClient;
}

const WhiteboardArgs& Whiteboard::GetArgs() const {
    return wbArgs;
}

#include "Channel.hpp"

Channel::Channel()
    : name(),
      topic(),
      key(),
      members(),
      operators(),
      inviteOnly(false),
      topicOpOnly(false),
      hasKey(false),
      hasUserLimit(false),
      userLimit(-1)
{}

Channel::Channel(const string& name)
    : name(name),
      topic(),
      key(),
      members(),
      operators(),
      inviteOnly(false),
      topicOpOnly(false),
      hasKey(false),
      hasUserLimit(false),
      userLimit(-1)
{}

void Channel::addMember(Client* client)
{
  if (client)
    members.insert(client);
}

void Channel::removeMember(Client* client)
{
    members.erase(client);
    operators.erase(client);
}

bool Channel::isMember(Client* client) const
{
  if (members.find(client) != members.end())
    return true;
  else
    return false;
}

bool Channel::isOperator(Client* client) const
{
  if (operators.find(client) != operators.end())
    return true;
  else
    return false;
}

void Channel::addOperator(Client* client)
{
    if (isMember(client))
        operators.insert(client);
}

void Channel::removeOperator(Client* client)
{
	operators.erase(client);
}

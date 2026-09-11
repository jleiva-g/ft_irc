#include "Channel.hpp"

Channel::Channel()
	: name(),
	  topic(),
	  key(),
	  members(),
	  operators(),
	  invited(),
	  modes(0),
	  userLimit(-1)
{}

Channel::Channel(const string& name)
	: name(name),
	  topic(),
	  key(),
	  members(),
	  operators(),
	  invited(),
	  modes(0),
	  userLimit(-1)
{}

/**
 * @brief Changes a channel mode.
 * @details Enables or disables the bit associated with the supplied mode
 *  without changing any other active mode.
 *
 * @param[in] mode Mode character: `i`, `t`, `k` or `l`.
 * @param[in] enable True to enable the mode, false to disable it.
 */
void Channel::changeMode(char mode, bool enable)
{
	unsigned int modeBit;

	if (mode == 'i')
		modeBit = MODE_INVITE_ONLY;
	else if (mode == 't')
		modeBit = MODE_TOPIC_OP_ONLY;
	else if (mode == 'k')
		modeBit = MODE_KEY;
	else if (mode == 'l')
		modeBit = MODE_USER_LIMIT;
	else
		return;

	if (enable)
		modes |= modeBit;
	else
		modes &= ~modeBit;
}

const string& Channel::getName() const
{
	return name;
}

const string& Channel::getTopic() const
{
	return topic;
}

const set<Client*>& Channel::getMembers() const
{
	return members;
}

const set<Client*>& Channel::getOperators() const
{
	return operators;
}

const string& Channel::getKey() const
{
	return key;
}

size_t Channel::getUserLimit() const
{
	return userLimit;
}

bool Channel::hasModeInviteOnly() const
{
	return (modes & MODE_INVITE_ONLY) != 0;
}

bool Channel::hasModeTopicOpOnly() const
{
	return (modes & MODE_TOPIC_OP_ONLY) != 0;
}

bool Channel::hasModeKey() const
{
	return (modes & MODE_KEY) != 0;
}

bool Channel::hasModeUserLimit() const
{
	return (modes & MODE_USER_LIMIT) != 0;
}

bool Channel::isFull() const
{
	return hasModeUserLimit() && members.size() >= userLimit;
}

/**
 * @brief Sets the channel name.
 * @param[in] value New channel name.
 */
void Channel::setName(const string& value)
{
	name = value;
}

/**
 * @brief Sets the channel topic.
 * @param[in] value New channel topic.
 */
void Channel::setTopic(const string& value)
{
	topic = value;
}

/**
 * @brief Sets the channel key.
 * @param[in] value New channel key.
 */
void Channel::setKey(const string& value)
{
	key = value;
}

/**
 * @brief Enables or disables invite-only mode.
 * @param[in] value True to enable invite-only mode.
 */
void Channel::setInviteOnly(bool value)
{
	changeMode('i', value);
}

/**
 * @brief Enables or disables operator-only topic mode.
 * @param[in] value True to enable operator-only topic mode.
 */
void Channel::setTopicOpOnly(bool value)
{
	changeMode('t', value);
}

/**
 * @brief Enables or disables key protection.
 * @param[in] value True to enable key protection.
 */
void Channel::setHasKey(bool value)
{
	changeMode('k', value);
}

/**
 * @brief Sets the channel user limit.
 * @param[in] value Maximum number of users allowed in the channel.
 */
void Channel::setUserLimit(size_t value)
{
	userLimit = value;
	changeMode('l', value);
}

/**
 * @brief Adds a client to the channel.
 * @details Null client pointers are ignored.
 * @param[in] client Client to add to the channel.
 */
void Channel::addMember(Client* client)
{
	if (client)
		members.insert(client);
}

/**
 * @brief Removes a client from the channel.
 * @details A removed client is also removed from the operator set.
 * @param[in] client Client to remove from the channel.
 */
void Channel::removeMember(Client* client)
{
	members.erase(client);
	operators.erase(client);
}

/**
 * @brief Adds a client to the the invitation list.
 * @details Null client pointers are ignored.
 * @param[in] client Client to add to the set.
 */
void Channel::addInvited(Client* client)
{
	if (client)
		invited.insert(client);
}

/**
 * @brief Removes a client from the invitation list.
 * @param[in] client Client to remove from the set.
 */
void Channel::removeInvited(Client* client)
{
	invited.erase(client);
}

/**
 * @brief Checks whether a client belongs to the channel.
 * @param[in] client Client to look up.
 * @return True when the client is a channel member.
 */
bool Channel::isMember(Client* client) const
{
	if (members.find(client) != members.end())
		return true;
	else
		return false;
}

/**
 * @brief Checks whether a client belongs to the invitations list.
 * @param[in] client Client to look up.
 * @return True when the client has being invited.
 */
bool Channel::isInvited(Client* client) const
{
	if (invited.find(client) != invited.end())
		return true;
	else
		return false;
}

/**
 * @brief Checks whether a client has channel operator status.
 * @details If the channel has no operators, the first member is promoted to
 * operator before the membership check is performed.
 *
 * @param[in] client Client to look up.
 * @return True when the client is a channel operator, false otherwise.
 */
bool Channel::isOperator(Client* client)
{
	if (operators.empty())
	{
		operators.insert(*(members.begin()));
	}
	if (operators.find(client) != operators.end())
		return true;
	else
		return false;
}

/**
 * @brief Grants channel operator status to a member.
 * @details Clients that are not channel members are ignored.
 * @param[in] client Member to promote to channel operator.
 */
void Channel::addOperator(Client* client)
{
	if (isMember(client))
		operators.insert(client);
}

/**
 * @brief Revokes channel operator status from a client.
 * @param[in] client Client whose operator status is removed.
 */
void Channel::removeOperator(Client* client)
{
	if (client)
		operators.erase(client);
}
